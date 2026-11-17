"""Regenerate the Part 2 indexed PNG artwork; Python is optional for building."""

from pathlib import Path
import struct
import zlib

PART2 = Path(__file__).resolve().parents[1]

SPRITE_COLORS = [
    (0, 0, 0),
    (64, 200, 224),
    (176, 240, 248),
    (24, 96, 136),
    (248, 248, 248),
    (248, 224, 104),
    (224, 224, 240),
    (128, 128, 168),
    (16, 24, 48),
] + [(0, 0, 0)] * 7

PLAYFIELD_COLORS = [
    (0, 0, 0),
    (16, 16, 40),
    (216, 216, 232),
    (136, 144, 168),
    (64, 68, 96),
    (224, 64, 72),
    (240, 144, 48),
    (232, 208, 64),
    (72, 192, 96),
    (72, 128, 232),
    (248, 240, 224),
    (40, 32, 48),
] + [(0, 0, 0)] * 4

BACKDROP_COLORS = [
    (8, 8, 32),
    (24, 24, 64),
    (48, 48, 104),
    (128, 128, 208),
]


def chunk(kind, data):
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    )


def write_png(name, width, height, pixels, colors):
    scanlines = b"".join(
        b"\x00" + bytes(pixels[y * width : (y + 1) * width]) for y in range(height)
    )
    palette = bytes(component for color in colors for component in color)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0))
        + chunk(b"PLTE", palette)
        + chunk(b"IDAT", zlib.compress(scanlines, 9))
        + chunk(b"IEND", b"")
    )
    destination = PART2 / name
    destination.write_bytes(png)
    print(f"Wrote {destination} ({len(png)} bytes)")


def paddle_pixel(x, y, shine_x):
    """Color of pixel (x, y) of the whole 48x8 paddle; shine_x < 0 means idle."""
    if (x in (0, 47)) and (y in (0, 7)):
        return 0
    if x in (0, 47) or y in (0, 7):
        return 8
    color = 1
    if y == 1:
        color = 2
    elif y == 6:
        color = 3
    if shine_x >= 0:
        distance = abs(x - shine_x + (y - 4))
        if distance <= 1:
            color = 4
        elif distance <= 3:
            color = 5
    return color


def make_paddle():
    # Rows are the left, middle, and right 16-pixel pieces; columns are frames.
    # Frame 0 is idle and frames 1-5 sweep a shine across the paddle.
    width, height = 128, 48
    pixels = bytearray(width * height)
    for piece in range(3):
        for frame in range(6):
            shine_x = -1 if frame == 0 else frame * 10 - 2
            for y in range(8):
                for x in range(16):
                    color = paddle_pixel(piece * 16 + x, y, shine_x)
                    pixels[(piece * 16 + y) * width + frame * 16 + x] = color
    write_png("paddle.png", width, height, pixels, SPRITE_COLORS)


def make_ball():
    # Sixteen 8x8 frames in one row: frame 0 is idle, frames 1-5 glimmer.
    width, height = 128, 8
    pixels = bytearray(width * height)
    for frame in range(6):
        for y in range(8):
            for x in range(8):
                if (2 * x - 7) ** 2 + (2 * y - 7) ** 2 > 58:
                    continue
                color = 7 if x + y >= 10 else 6
                if frame == 1:
                    color = 5
                elif frame == 2:
                    color = 4
                elif frame == 3:
                    color = 4 if x in (3, 4) or y in (3, 4) else 5
                elif frame == 4:
                    color = 5 if x + y >= 10 else 4
                elif frame == 5:
                    color = 4 if x + y <= 4 else color
                pixels[y * width + frame * 8 + x] = color
    write_png("ball.png", width, height, pixels, SPRITE_COLORS)


def make_playfield():
    # 32x32 tiles: HUD frame on rows 0-3, side walls on columns 1 and 30,
    # and 14x5 gapless 16x8 bricks starting at tile column 2, tile row 6.
    width, height = 256, 256
    pixels = bytearray(width * height)

    def put_tile(tile_x, tile_y, painter):
        for y in range(8):
            for x in range(8):
                pixels[(tile_y * 8 + y) * width + tile_x * 8 + x] = painter(x, y)

    def bar(x, y):
        if y in (0, 7):
            return 4
        return 2 if y == 1 else 3

    def post(x, y):
        if x in (0, 7):
            return 4
        return 2 if x == 1 else 3

    for tile_x in range(1, 31):
        put_tile(tile_x, 0, bar)
        put_tile(tile_x, 3, bar)
    for tile_x in range(2, 30):
        put_tile(tile_x, 1, lambda x, y: 1)
        put_tile(tile_x, 2, lambda x, y: 1)
    for tile_y in range(1, 28):
        if tile_y != 3:
            put_tile(1, tile_y, post)
            put_tile(30, tile_y, post)

    for row in range(5):
        for col in range(14):
            for y in range(8):
                for x in range(16):
                    if x == 15 or y == 7:
                        color = 11
                    elif x == 0 or y == 0:
                        color = 10
                    else:
                        color = 5 + row
                    px = 16 + col * 16 + x
                    py = 48 + row * 8 + y
                    pixels[py * width + px] = color
    write_png("playfield.png", width, height, pixels, PLAYFIELD_COLORS)


def make_backdrop():
    # A 32-pixel repeating diamond pattern with a few stars.
    width, height = 256, 256
    pixels = bytearray(width * height)
    for y in range(height):
        for x in range(width):
            cx = x % 32
            cy = y % 32
            if abs(cx - 16) + abs(cy - 16) == 12:
                pixels[y * width + x] = 1
            elif cx == 16 and cy == 16:
                pixels[y * width + x] = 2
    for x, y in ((40, 72), (200, 120), (88, 168), (168, 40), (120, 216)):
        for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            pixels[(y + dy) * width + x + dx] = 3 if (dx, dy) == (0, 0) else 2
    write_png("backdrop.png", width, height, pixels, BACKDROP_COLORS)


def main():
    make_paddle()
    make_ball()
    make_playfield()
    make_backdrop()


if __name__ == "__main__":
    main()
