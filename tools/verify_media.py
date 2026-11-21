"""Check local Markdown links, PNG integrity, and tutorial checkpoint images."""

import argparse
from pathlib import Path
import re
import struct
import unicodedata
from urllib.parse import unquote, urlsplit
import zlib


ROOT = Path(__file__).resolve().parents[1]
TUTORIALS = ("README.md", "README-part2.md", "README-part3.md", "README-part4.md")


def headings(text):
    counts = {}
    result = set()
    for heading in re.findall(r"^#{1,6}\s+(.+)$", text, re.MULTILINE):
        slug = re.sub(r"[^\w\- ]", "", heading.lower()).replace(" ", "-")
        occurrence = counts.get(slug, 0)
        counts[slug] = occurrence + 1
        result.add(slug if not occurrence else f"{slug}-{occurrence}")
    result.update(re.findall(r"""\bid=["']([^"']+)["']""", text))
    return result


def local_links(text):
    # Ignore code examples; their placeholder paths are not document links.
    text = re.sub(r"```.*?```", "", text, flags=re.DOTALL)
    definitions = dict(re.findall(
        r"^\[([^\]]+)\]:\s+(\S+)", text, re.MULTILINE,
    ))
    links = re.findall(r"!?\[[^\]]*\]\(([^)\n]+)\)", text)
    for label in re.findall(r"!?\[[^\]]*\]\[([^\]]+)\]", text):
        if label not in definitions:
            raise ValueError(f"Undefined reference link: {label}")
        links.append(definitions[label])
    links.extend(re.findall(r"(?:src|href|poster)=[\"']([^\"']+)[\"']", text))
    return [link.split(' "')[0].strip("<>") for link in links]


def verify_link(document, link):
    url = urlsplit(link)
    if url.scheme or url.netloc:
        return
    destination = (
        document.parent / unquote(url.path) if url.path else document
    ).resolve()
    if not destination.is_relative_to(ROOT):
        raise ValueError(f"{document.name}: link escapes repository: {link}")
    if not destination.exists():
        raise ValueError(f"{document.name}: missing local link: {link}")
    if url.fragment and destination.suffix == ".md":
        fragment = unicodedata.normalize("NFC", unquote(url.fragment))
        if fragment not in headings(destination.read_text(encoding="utf-8")):
            raise ValueError(f"{document.name}: missing heading: {link}")


def verify_png(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"Invalid PNG signature: {path}")
    offset = 8
    dimensions = None
    compressed = bytearray()
    finished = False
    while offset < len(data):
        if offset + 12 > len(data):
            raise ValueError(f"Truncated PNG: {path}")
        length = struct.unpack_from(">I", data, offset)[0]
        end = offset + 12 + length
        if end > len(data):
            raise ValueError(f"Truncated PNG chunk: {path}")
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        crc = struct.unpack_from(">I", data, offset + 8 + length)[0]
        if zlib.crc32(kind + payload) & 0xFFFFFFFF != crc:
            raise ValueError(f"Invalid PNG checksum: {path}")
        if kind == b"IHDR":
            dimensions = struct.unpack_from(">II", payload)
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            finished = True
            if end != len(data):
                raise ValueError(f"Trailing PNG data: {path}")
        offset = end
    if not finished or not dimensions or not all(dimensions):
        raise ValueError(f"Incomplete PNG: {path}")
    try:
        pixels = zlib.decompress(compressed)
    except zlib.error as error:
        raise ValueError(f"Invalid PNG pixel data: {path}") from error
    if not pixels:
        raise ValueError(f"Empty PNG pixel data: {path}")
    if path.parent.name == "screenshots" and dimensions != (256, 224):
        raise ValueError(f"Screenshot must be a native 256 x 224 frame: {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoints", action="store_true",
                        help="Require a checkpoint image in every tutorial section")
    args = parser.parse_args()
    documents = [
        path for path in ROOT.glob("*.md")
    ] + list((ROOT / "vendor").glob("*/README.md"))
    for document in documents:
        for link in local_links(document.read_text(encoding="utf-8")):
            verify_link(document, link)
    images = [
        path for directory in ("screenshots", "part2", "part3", "part4")
        for path in (ROOT / directory).glob("*.png")
    ] + [ROOT / "sprites.png"]
    for path in images:
        verify_png(path)
    if args.checkpoints:
        for part, name in enumerate(TUTORIALS, 1):
            text = (ROOT / name).read_text(encoding="utf-8")
            sections = list(re.finditer(r"^## (\d+)\. .+$", text, re.MULTILINE))
            for index, match in enumerate(sections):
                end = sections[index + 1].start() if index + 1 < len(sections) else len(text)
                section = text[match.end():end]
                number = match[1]
                if "**Checkpoint:**" not in section:
                    raise ValueError(f"{name}: section {number} has no checkpoint")
                if not re.search(
                    rf"!\[[^\]]+\]\(screenshots/part{part}-section{number}-[^)]+\.png\)",
                    section,
                ):
                    raise ValueError(f"{name}: section {number} has no checkpoint screenshot")
    print(f"PASS: local links in {len(documents)} documents and {len(images)} PNG files")


if __name__ == "__main__":
    main()
