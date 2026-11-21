"""Focused regression checks for the repository's media validator."""

from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_media import headings, local_links, verify_png


def chunk(kind, payload):
    return (
        struct.pack(">I", len(payload)) + kind + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def png(width, height):
    pixels = b"".join(b"\0" + b"\0\0\0" * width for _ in range(height))
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(pixels))
        + chunk(b"IEND", b"")
    )


class MediaTests(unittest.TestCase):
    def test_markdown_links_ignore_code_and_resolve_references(self):
        text = (
            "[Part 2](README-part2.md#contents)\n![Frame][shot]\n"
            '[shot]: screenshots/frame.png\n'
            '<video src="videos/demo.mp4" poster="screenshots/poster.png"></video>\n'
            "```c\n[not a link](missing.png)\n```\n"
        )
        self.assertEqual(local_links(text), [
            "README-part2.md#contents", "screenshots/frame.png",
            "videos/demo.mp4", "screenshots/poster.png",
        ])
        with self.assertRaisesRegex(ValueError, "Undefined reference"):
            local_links("![Frame][missing]")

    def test_duplicate_heading_anchors(self):
        self.assertEqual(headings(
            "# Part 1: Start\n## `game_tick`\n## `game_tick`\n"
            '<details><summary><h2 id="troubleshooting">Troubleshooting</h2></summary></details>\n',
        ), {"part-1-start", "game_tick", "game_tick-1", "troubleshooting"})

    def test_corrupt_and_non_native_screenshots_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "screenshots" / "frame.png"
            path.parent.mkdir()
            path.write_bytes(png(256, 224))
            verify_png(path)
            data = bytearray(path.read_bytes())
            data[29] ^= 1
            path.write_bytes(data)
            with self.assertRaisesRegex(ValueError, "checksum"):
                verify_png(path)
            path.write_bytes(png(512, 448))
            with self.assertRaisesRegex(ValueError, "native 256 x 224"):
                verify_png(path)
            path.write_bytes(png(256, 224)[:-5])
            with self.assertRaisesRegex(ValueError, "Truncated PNG"):
                verify_png(path)


if __name__ == "__main__":
    unittest.main()
