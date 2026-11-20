"""Regenerate Part 4's original capsule; not required to build the ROM."""

from pathlib import Path
import struct
import zlib

from make_sprites import chunk


def main():
    rows = (
        "03333330",
        "32222223",
        "32111123",
        "32444423",
        "32444423",
        "32111123",
        "32222223",
        "03333330",
    )
    colors = [
        (0, 0, 0), (64, 200, 224), (176, 240, 248),
        (24, 96, 136), (248, 248, 248), (248, 224, 104),
        (224, 224, 240), (128, 128, 168), (16, 24, 48),
    ] + [(0, 0, 0)] * 247
    pixels = b"".join(b"\0" + bytes(int(c) for c in row) for row in rows)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", 8, 8, 8, 3, 0, 0, 0))
        + chunk(b"PLTE", bytes(c for color in colors for c in color))
        + chunk(b"IDAT", zlib.compress(pixels))
        + chunk(b"IEND", b"")
    )
    destination = Path(__file__).resolve().parents[1] / "part4" / "capsule.png"
    destination.write_bytes(png)
    print(f"Wrote {destination}")


if __name__ == "__main__":
    main()
