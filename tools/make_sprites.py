"""Regenerate the original indexed sprite sheet; Python is optional for building."""

from pathlib import Path
import struct
import zlib


def chunk(kind, data):
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    )


def main():
    width, height = 128, 16
    pixels = bytearray(width * height)
    colors = [
        (0, 0, 0),
        (232, 64, 72),
        (248, 144, 40),
        (240, 216, 56),
        (72, 200, 96),
        (64, 200, 224),
        (72, 112, 232),
        (248, 248, 248),
        (248, 224, 200),
        (64, 64, 88),
    ] + [(0, 0, 0)] * 246

    for block in range(6):
        for y in range(8):
            for x in range(16):
                color = block + 1
                if y == 0 or x == 0:
                    color = 8
                elif y == 7 or x == 15:
                    color = 9
                pixels[y * width + block * 16 + x] = color

    for y in range(8):
        for x in range(8):
            if (2 * x - 7) ** 2 + (2 * y - 7) ** 2 <= 58:
                pixels[y * width + 96 + x] = 7

    scanlines = b"".join(
        b"\x00" + pixels[y * width : (y + 1) * width] for y in range(height)
    )
    palette = bytes(component for color in colors for component in color)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
        + chunk(b"PLTE", palette)
        + chunk(b"IDAT", zlib.compress(scanlines))
        + chunk(b"IEND", b"")
    )
    destination = Path(__file__).resolve().parents[1] / "sprites.png"
    destination.write_bytes(png)
    print(f"Wrote {destination} ({len(png)} bytes)")


if __name__ == "__main__":
    main()
