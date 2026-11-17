# Part 2: Backgrounds, dynamic sprites, and sound

This is **Part 2** of the build-it-yourself Breakout tutorial. It starts from the
game you finished in [Part 1](README.md) and changes it one feature at a time.
As before, you write every line yourself, build it, and test a checkpoint before
moving on.

By the end of this part your game will:

- paint a starfield **backdrop** on a second background layer;
- draw the **walls, HUD frame, and bricks** as background tiles instead of
  `#` characters and sprites, erasing a brick's tiles when it breaks;
- use PVSnesLib's **dynamic sprite** engine for the paddle and ball, with a
  short **glimmer animation** when they hit something;
- play **music and sound effects** through the SDK's sound driver.

![Part 2 in play](screenshots/part2-playing.png)

## Before you start

You need the finished Part 1 project at `<SDK root>\snes-examples\breakout`,
where `<SDK root>` is your PVSnesLib folder (the one that directly contains
`devkitsnes`, `pvsneslib`, and `snes-examples`). It should build, and every
Part 1 checkpoint should pass. The diffs below assume your code matches the end
of Part 1. If you changed things, adapt each edit to your version.

Save a copy of your Part 1 project first, either by committing it to Git or by
copying the `breakout` folder somewhere **outside** the SDK. That way you can
always compare against a working version.

### Get the new artwork

Part 2 adds four indexed PNG files. They are in the `part2` folder of this
repository. If you cloned or downloaded the repository before Part 2 existed,
update your copy (`git pull`, or download the ZIP again).

Copy these four files from your repository copy's `part2` folder into your
project's root, next to `sprites.png`:

| File | Size | Contents |
|---|---|---|
| [backdrop.png](part2/backdrop.png) | 256 x 256 | Four-color diamond pattern with a few stars |
| [playfield.png](part2/playfield.png) | 256 x 256 | HUD frame, side walls, and a gapless 14 x 5 wall of bricks |
| [paddle.png](part2/paddle.png) | 128 x 48 | Left, middle, and right paddle pieces, each with 6 animation frames |
| [ball.png](part2/ball.png) | 128 x 8 | Sixteen 8 x 8 ball frames (6 used) |

Keep `sprites.png` for now; section 4 stops using it.

### Check the audio files

The music and sound effects come from the SDK's own sound example, so they are
not included in this repository. Check that these two files exist:

```text
<SDK root>\snes-examples\audio\effectsandmusic\res\effectssfx.it
<SDK root>\snes-examples\audio\effectsandmusic\res\pollen8.it
```

You do not need to copy them. In section 6, your Makefile copies them into
your project automatically.

## Tutorial versus finished reference

**Do not copy this repository's `part2\src` folder into your project.** It is
the finished result of this part, for comparison **after** you complete the
steps. The reference [part2 Makefile](part2/Makefile) builds it with a local SDK
under `vendor/pvsneslib`; your learner Makefile keeps its Part 1 SDK default.

## How to follow the code edits

The edits use the same diff blocks as Part 1, with surrounding unchanged code
so you can see where each change belongs:

```diff
 int existing_line;
-old_line();
+new_line();
 next_existing_line();
```

Keep the unmarked lines, remove `-` lines, and add `+` lines. Do not type the
markers, the `---`/`+++` file names, or the `@@` separators. Separate `@@`
sections are separate edits in the same file, in top-to-bottom order. Makefile
command lines still need a real **tab** at the start.

Build each checkpoint from your project folder in the MSYS2 UCRT64 terminal,
exactly as in Part 1:

```sh
make
```

Then open or reload `breakout.sfc` in your emulator. If a build behaves
strangely after you edit the Makefile or artwork, run `make clean`, then `make`.

## Contents

1. [Paint a backdrop on background 3](#1-paint-a-backdrop-on-background-3)
2. [Draw the walls, HUD, and bricks on background 2](#2-draw-the-walls-hud-and-bricks-on-background-2)
3. [Erase and restore bricks in the tile map](#3-erase-and-restore-bricks-in-the-tile-map)
4. [Switch the paddle and ball to dynamic sprites](#4-switch-the-paddle-and-ball-to-dynamic-sprites)
5. [Make the paddle and ball glimmer](#5-make-the-paddle-and-ball-glimmer)
6. [Play music](#6-play-music)
7. [Add sound effects](#7-add-sound-effects)
8. [Test the finished game](#8-test-the-finished-game)

See also [troubleshooting](#troubleshooting),
[gameplay tests](#running-the-gameplay-tests), [licensing](#licensing), and
[official references](#official-references).

---

## 1. Paint a backdrop on background 3

### How the Mode 1 layers fit together

Part 1 called `setMode(BG_MODE1, 0)` but used only background 1 (`BG1`) for
text and disabled the other two layers. Mode 1 gives you three background
layers:

| Layer | Colors per tile | Front-to-back order |
|---|---|---|
| BG1 | 16 (4 bits per pixel) | In front |
| BG2 | 16 (4 bits per pixel) | Middle |
| BG3 | 4 (2 bits per pixel) | Behind |

Sprites with priority 3, like ours, appear in front of all three. In
PVSnesLib, the layers are numbered from zero: `0` is BG1, `1` is BG2, and `2`
is BG3.

A background is made of two pieces of data in video memory (VRAM):

- **tiles** (character data): 8 x 8 pixel images, with no position;
- a **map**: a 32 x 32 grid of 16-bit entries, each choosing a tile number and
  a palette for one 8 x 8 cell of the screen.

By the end of this part, VRAM will hold this data:

| Data | VRAM address (16-bit words) |
|---|---|
| 16 x 16 sprite frames | `0x0000` |
| 8 x 8 sprite frames | `0x1000` |
| Font tiles (BG1) | `0x3000` |
| Playfield tiles (BG2) | `0x4000` |
| Backdrop tiles (BG3) | `0x5000` |
| Text map (BG1) | `0x6800` |
| Playfield map (BG2) | `0x6C00` |
| Backdrop map (BG3) | `0x7000` |

Tile addresses must be multiples of `0x1000`. Each 32 x 32 map takes `0x400`
words, so the three maps sit side by side without overlapping.

Colors live in **CGRAM**, a 256-entry palette memory. In Mode 1, BG3's 4-color
palette 0 uses colors 0 to 3, which are also the first colors of BG1's palette
0. Part 1's font used palette 0. If it kept doing so, the backdrop and the
font would overwrite each other's colors. So the font moves to palette 1
(colors 16 to 31). Our layers will use these palettes:

| User | Palette | CGRAM colors |
|---|---|---|
| Backdrop (BG3) | 0 | 0-3 |
| Font (BG1) | 1 | 16-31 |
| Playfield (BG2) | 2 | 32-47 |
| Sprites | sprite palette 0 | 128-143 |

### Convert the backdrop

Add a conversion rule for the backdrop to your `Makefile`, after the
`sprites.pic` rule:

```diff
--- a/Makefile
+++ b/Makefile
@@
 sprites.pic: sprites.png
 	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<
 
-src/data.obj: sprites.pic
+backdrop.pic: backdrop.png
+	$(GFXCONV) -s 8 -o 4 -u 4 -e 0 -p -m -t png -i $<
+
+src/data.obj: sprites.pic backdrop.pic
 src/main.obj src/game.obj: src/game.h
 
 clean: cleanBuildRes cleanRom cleanGfx
```

The options mean:

- `-s 8`: cut the picture into 8 x 8 tiles;
- `-u 4` and `-o 4`: use four colors (2 bits per pixel) and write four palette
  colors;
- `-e 0`: store palette number 0 in every map entry;
- `-p` and `-m`: also write a palette (`backdrop.pal`) and a map
  (`backdrop.map`).

Unlike `sprites.pic`, there is no `-R`, so `gfx4snes` removes duplicate tiles.
The 256 x 256 picture has 1,024 cells but only 18 different tiles. The map
records which tile goes in each cell, so `backdrop.pic` stays tiny (288 bytes).

Add the converted files to the end of `src\data.asm`:

```diff
--- a/src/data.asm
+++ b/src/data.asm
@@
 sprite_palette:
 .incbin "sprites.pal"
 
+backdrop_tiles:
+.incbin "backdrop.pic"
+backdrop_tiles_end:
+
+backdrop_map:
+.incbin "backdrop.map"
+
+backdrop_palette:
+.incbin "backdrop.pal"
+
 .ends
```

The `backdrop_tiles_end` label marks the end of the tile data. C can subtract
the two addresses to get the size, so you do not have to hard-code a number
that changes whenever the artwork does.

### Load the backdrop

In `src\main.c`, name the backdrop map address and declare the new labels:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 #include <snes.h>
 #include "game.h"
 
+#define BACKDROP_MAP 0x7000
+
 extern u8 sprite_tiles[];
 extern u8 sprite_palette[];
+extern u8 backdrop_tiles[], backdrop_tiles_end[];
+extern u8 backdrop_map[];
+extern u8 backdrop_palette[];
 
 Game game;
```

Then, in `init_video`, move the font to palette 1, load the backdrop, and stop
disabling BG3:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     setScreenOff();
-    consoleInitDefaultText(0);
+    consoleInitDefaultText(1);
     bgSetGfxPtr(0, 0x3000);
     bgSetMapPtr(0, 0x6800, SC_32x32);
+    bgInitTileSet(2, backdrop_tiles, backdrop_palette, 0,
+                  backdrop_tiles_end - backdrop_tiles, 4 * 2,
+                  BG_4COLORS, 0x5000);
+    bgInitMapSet(2, backdrop_map, 32 * 32 * 2, SC_32x32, BACKDROP_MAP);
     oamInitGfxSet(sprite_tiles, 1024, sprite_palette, 32,
                   0, 0x0000, OBJ_SIZE8_L16);
 
     setMode(BG_MODE1, 0);
     bgSetDisable(1);
-    bgSetDisable(2);
     oamClear(0, 0);
```

`bgInitTileSet` copies the tiles to VRAM `0x5000` and the palette to CGRAM. Its
arguments are:

1. the layer (`2`, which is BG3);
2. the tiles;
3. the palette;
4. the palette number (`0`);
5. the tile data size;
6. the palette size in bytes (4 colors x 2 bytes each);
7. the color mode;
8. the VRAM address.

`bgInitMapSet` copies the 2,048-byte map (32 x 32 entries x 2 bytes each) to
`BACKDROP_MAP` and points BG3 at it. Both functions write to VRAM, which is
safe here because the screen is off.

**Checkpoint:** Build and run. A dark blue diamond pattern with a few stars now
fills the screen behind everything else. The text is still white and readable,
and the `#` walls, bricks, paddle, and ball look and play exactly as before.

---

## 2. Draw the walls, HUD, and bricks on background 2

### Why move bricks to a background

In Part 1, every brick was a 16 x 16 sprite. The SNES can show 128 sprites in
total and only 32 on one scanline, so a wall of sprites cannot grow much. A
background has neither limit: every 8 x 8 cell of the screen is a map entry. A
16 x 8 brick is just two map entries, and erasing it means writing two "empty"
entries.

`playfield.png` is a full 256 x 224 screen (plus unused rows) drawn on an 8 x 8
grid:

- a HUD frame on tile rows 0 to 3, around the score and lives text;
- side walls on tile columns 1 and 30;
- 70 bricks in 5 rows of 14, with no gaps, starting at pixel (16, 48).

Gameplay already treats pixels 16 to 239 as the field, and 14 bricks x 16
pixels fills exactly that width.

### Convert the playfield

Add another rule to the `Makefile`:

```diff
--- a/Makefile
+++ b/Makefile
@@
 sprites.pic: sprites.png
 	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<
 
+playfield.pic: playfield.png
+	$(GFXCONV) -s 8 -o 16 -u 16 -e 2 -p -m -t png -i $<
+
 backdrop.pic: backdrop.png
 	$(GFXCONV) -s 8 -o 4 -u 4 -e 0 -p -m -t png -i $<
 
-src/data.obj: sprites.pic backdrop.pic
+src/data.obj: sprites.pic playfield.pic backdrop.pic
 src/main.obj src/game.obj: src/game.h
```

This time the tiles use 16 colors, and `-e 2` stores palette number 2 in each
map entry. Add the labels to the end of `src\data.asm`:

```diff
--- a/src/data.asm
+++ b/src/data.asm
@@
 backdrop_palette:
 .incbin "backdrop.pal"
 
+playfield_tiles:
+.incbin "playfield.pic"
+playfield_tiles_end:
+
+playfield_map:
+.incbin "playfield.map"
+
+playfield_palette:
+.incbin "playfield.pal"
+
 .ends
```

### Close the gaps in the brick grid

Change the brick layout in `src\game.h`. The pitch macros go away, because
bricks now touch: the distance from one brick to the next is just its width or
height.

```diff
--- a/src/game.h
+++ b/src/game.h
@@
 #define PADDLE_SPEED 3
 
-#define BRICK_COLS 10
+#define BRICK_COLS 14
 #define BRICK_ROWS 5
 #define BRICK_COUNT (BRICK_COLS * BRICK_ROWS)
-#define BRICK_X 30
+#define BRICK_X 16
 #define BRICK_Y 48
 #define BRICK_WIDTH 16
 #define BRICK_HEIGHT 8
-#define BRICK_PITCH_X 20
-#define BRICK_PITCH_Y 16
 
 enum GameState {
```

`BRICK_X` and `BRICK_Y` must match where the bricks are painted in
`playfield.png`. Both are multiples of 8, so every brick lines up with the tile
grid.

### Find bricks with arithmetic instead of a search

Part 1's `hit_brick` checked every live brick in the touched rows. With 14
columns that loop gets expensive. The 65816 CPU has no instruction to multiply
or divide 16-bit numbers, so the compiler turns `col * BRICK_PITCH_X` into a
call to a slow library routine. `hit_brick` runs up to six times per frame
(three substeps, for both X and Y), and when a frame's work takes too long,
the game visibly slows down.

With no gaps, there is a shortcut: the ball (8 x 8) can only touch the bricks
under its corners. Divide its position by the brick size to get at most two
rows and two columns. In `src\game.c`, **replace the whole `hit_brick`
function** with:

```c
static int hit_brick(Game *game, int x, int y)
{
    int first_row;
    int last_row;
    int first_col;
    int last_col;
    int row;
    int col;
    int index;

    if (y + BALL_SIZE <= BRICK_Y ||
        y >= BRICK_Y + BRICK_ROWS * BRICK_HEIGHT ||
        x + BALL_SIZE <= BRICK_X ||
        x >= BRICK_X + BRICK_COLS * BRICK_WIDTH)
        return 0;

    /* The grid has no gaps, so only the bricks under the ball's corners can
       be hit. Working them out directly is much cheaper than a full scan. */
    first_row = y < BRICK_Y ? 0 : (unsigned int)(y - BRICK_Y) / BRICK_HEIGHT;
    last_row = (unsigned int)(y + BALL_SIZE - 1 - BRICK_Y) / BRICK_HEIGHT;
    if (last_row >= BRICK_ROWS)
        last_row = BRICK_ROWS - 1;
    first_col = x < BRICK_X ? 0 : (unsigned int)(x - BRICK_X) / BRICK_WIDTH;
    last_col = (unsigned int)(x + BALL_SIZE - 1 - BRICK_X) / BRICK_WIDTH;
    if (last_col >= BRICK_COLS)
        last_col = BRICK_COLS - 1;

    for (row = first_row; row <= last_row; ++row) {
        for (col = first_col; col <= last_col; ++col) {
            index = row * BRICK_COLS + col;
            if (game->bricks[index]) {
                game->bricks[index] = 0;
                game->score += 10;
                --game->bricks_left;
                if (game->bricks_left == 0)
                    game->state = STATE_WON;
                return 1;
            }
        }
    }
    return 0;
}
```

The early `return 0` handles a ball that is entirely outside the wall. After
that, at least one row and column overlap, so the clamped ranges are never
empty. The `(unsigned int)` casts matter: dividing an unsigned value by 8 or 16
compiles to quick bit shifts, while signed division has to round negative
numbers toward zero and needs the slow routine. Only one multiplication,
`row * BRICK_COLS`, remains, and it runs only for the one to four candidate
bricks. Checking rows top to bottom and columns left to right keeps the same
"first hit wins" behavior as Part 1.

### Show the playfield

In `src\main.c`, add the playfield's map address and labels. `playfield_map`
is declared as `u16`, because each map entry is a 16-bit value; section 3 reads
individual entries from it.

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 #include <snes.h>
 #include "game.h"
 
+#define PLAYFIELD_MAP 0x6C00
 #define BACKDROP_MAP 0x7000
 
 extern u8 sprite_tiles[];
 extern u8 sprite_palette[];
 extern u8 backdrop_tiles[], backdrop_tiles_end[];
 extern u8 backdrop_map[];
 extern u8 backdrop_palette[];
+extern u8 playfield_tiles[], playfield_tiles_end[];
+extern u16 playfield_map[];
+extern u8 playfield_palette[];
 
 Game game;
```

The bricks are now part of the background, so **delete the whole
`draw_bricks` function**, from `static void draw_bricks(void)` to its closing
brace. Leave the `shown_alive` and `shown_bricks` variables above it; section 3
uses them again. Then remove the call to it:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     oamSetXY(0, game.ball_x, game.ball_y);
     for (i = 0; i < 3; ++i)
         oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
-    draw_bricks();
     draw_text();
 }
```

Finally, update `init_video`. Load BG2, stop disabling it, and delete the old
brick reset loop and the `#` walls:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 static void init_video(void)
 {
-    int row;
     int i;
 
     setScreenOff();
     consoleInitDefaultText(1);
     bgSetGfxPtr(0, 0x3000);
     bgSetMapPtr(0, 0x6800, SC_32x32);
+    bgInitTileSet(1, playfield_tiles, playfield_palette, 2,
+                  playfield_tiles_end - playfield_tiles, 16 * 2,
+                  BG_16COLORS, 0x4000);
+    bgInitMapSet(1, (u8 *)playfield_map, 32 * 32 * 2, SC_32x32,
+                 PLAYFIELD_MAP);
     bgInitTileSet(2, backdrop_tiles, backdrop_palette, 0,
                   backdrop_tiles_end - backdrop_tiles, 4 * 2,
                   BG_4COLORS, 0x5000);
@@
     setMode(BG_MODE1, 0);
-    bgSetDisable(1);
     oamClear(0, 0);
-    for (i = 0; i < BRICK_COUNT; ++i)
-        shown_alive[i] = 0;
 
     oamSet(0, game.ball_x, game.ball_y, 3, 0, 0, 12, 0);
@@
     consoleDrawText(2, 1, "SCORE       LIVES");
-    consoleDrawText(1, 3, "##############################");
-    for (row = 4; row < 28; ++row) {
-        consoleDrawText(1, row, "#");
-        consoleDrawText(30, row, "#");
-    }
     draw_game();
     setScreenOn();
 }
```

The `(u8 *)` cast is needed because `bgInitMapSet` takes a byte pointer.
Palette number `2` matches the `-e 2` used in conversion, and `16 * 2` is the
palette size in bytes.

**Checkpoint:** Build and run. Metal walls frame the field and the HUD, the
score and lives sit inside the HUD frame, and a gapless wall of 70 bricks in
five colors fills the top. The ball and paddle still use Part 1's sprites. Play
for a moment: the ball bounces off bricks and the score goes up, but **the
bricks do not disappear yet**. Section 3 fixes that.

If you set up Part 1's optional gameplay tests, they now fail because they
check the old layout. After section 5, replace them as described in
[Running the gameplay tests](#running-the-gameplay-tests).

---

## 3. Erase and restore bricks in the tile map

### Change VRAM only during vertical blank

The SNES video chip reads VRAM constantly while it draws the screen. Your code
may write to VRAM only during **forced blank** (screen off, as in
`init_video`) or **vertical blank** (vblank), the short pause after each frame
is drawn. `WaitForVBlank()` returns at the start of vblank, so VRAM writes
belong right after it.

Vblank is short: only a few kilobytes can be copied by DMA in one NTSC
vblank, and the library already uses some of that time. The text console
copies its whole 2 KB map to VRAM in any frame where text changed. A copy that
runs past the end of vblank does not crash. It silently drops data, so some
bricks just stay on screen.

So the plan is:

1. keep a RAM copy of the five brick rows of the map (`brick_map`);
2. when bricks change, update that copy and flag the changed rows;
3. after `WaitForVBlank()`, copy **at most one** flagged row to VRAM.

One row is 14 bricks x 2 tiles x 2 bytes = 56 bytes, which easily fits. A
restart redraws all five rows over five frames, about a twelfth of a second.

### Track and upload changed rows

Add these definitions to `src\main.c`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 #define PLAYFIELD_MAP 0x6C00
 #define BACKDROP_MAP 0x7000
+#define EMPTY_TILE 0x0800
+#define BRICK_TILE_X (BRICK_X / 8)
+#define BRICK_TILE_Y (BRICK_Y / 8)
```

`BRICK_TILE_X` and `BRICK_TILE_Y` are the bricks' map column (2) and row (6).
`EMPTY_TILE` is the map entry `gfx4snes` wrote for empty areas of
`playfield.png`: tile 0, which is fully transparent, with palette 2 in bits 10
to 12 (`2 << 10 = 0x0800`).

Add the RAM copy and the row flags, then the two new functions, between the
variables and `draw_text`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 Game game;
 
+static u16 brick_map[BRICK_ROWS][BRICK_COLS * 2];
+static unsigned char row_changed[BRICK_ROWS];
 static unsigned char shown_alive[BRICK_COUNT];
 static char score_text[] = "000";
 static char lives_text[] = "3";
 static int shown_bricks = -1;
 static int shown_score = -1;
 static int shown_lives = -1;
 static int shown_state = -1;
 
+static void prepare_bricks(void)
+{
+    int index;
+    int row;
+    int col;
+    int tile;
+
+    if (shown_bricks == game.bricks_left)
+        return;
+
+    index = 0;
+    for (row = 0; row < BRICK_ROWS; ++row) {
+        tile = (BRICK_TILE_Y + row) * 32 + BRICK_TILE_X;
+        for (col = 0; col < BRICK_COLS * 2; col += 2) {
+            if (shown_alive[index] != game.bricks[index]) {
+                shown_alive[index] = game.bricks[index];
+                if (game.bricks[index]) {
+                    brick_map[row][col] = playfield_map[tile + col];
+                    brick_map[row][col + 1] = playfield_map[tile + col + 1];
+                } else {
+                    brick_map[row][col] = EMPTY_TILE;
+                    brick_map[row][col + 1] = EMPTY_TILE;
+                }
+                row_changed[row] = 1;
+            }
+            ++index;
+        }
+    }
+    shown_bricks = game.bricks_left;
+}
+
+static void upload_bricks(void)
+{
+    int row;
+
+    /* One row per frame keeps the copy inside vblank. */
+    for (row = 0; row < BRICK_ROWS; ++row) {
+        if (row_changed[row]) {
+            dmaCopyVram((u8 *)brick_map[row],
+                        PLAYFIELD_MAP + (BRICK_TILE_Y + row) * 32 + BRICK_TILE_X,
+                        BRICK_COLS * 4);
+            row_changed[row] = 0;
+            return;
+        }
+    }
+}
+
 static void draw_text(void)
```

`prepare_bricks` works like Part 1's `draw_bricks`. It does nothing unless the
brick count changed, then compares each brick with what is on screen. Here,
`col` counts map entries, two per brick. A broken brick gets two `EMPTY_TILE`
entries. A restored brick gets its original two entries back, read from
`playfield_map` in ROM, so the colors always match the artwork.

`dmaCopyVram(source, address, size)` copies `size` bytes to a VRAM word
address. A row's address is the map's base address, plus 32 entries per map
row, plus the bricks' starting column.

### Initialize everything explicitly

`init_video` must fill `brick_map` from the artwork and clear `row_changed`.
Do not rely on C's rule that global variables start at zero: the PVSnesLib
toolchain does **not** clear uninitialized globals and statics. On real
hardware (and in accurate emulators) they start with leftover values, so a
"zero" flag might start as `1`. Variables with an initializer, like
`shown_score = -1`, are set correctly.

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     oamSetXY(0, game.ball_x, game.ball_y);
     for (i = 0; i < 3; ++i)
         oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
+    prepare_bricks();
     draw_text();
 }
 
 static void init_video(void)
 {
+    int row;
+    int col;
     int i;
 
     setScreenOff();
@@
         oamSetEx((i + 1) * 4, OBJ_LARGE, OBJ_SHOW);
     }
 
+    for (row = 0; row < BRICK_ROWS; ++row) {
+        row_changed[row] = 0;
+        for (col = 0; col < BRICK_COLS * 2; ++col)
+            brick_map[row][col] =
+                playfield_map[(BRICK_TILE_Y + row) * 32 + BRICK_TILE_X + col];
+    }
+    for (i = 0; i < BRICK_COUNT; ++i)
+        shown_alive[i] = 1;
+    shown_bricks = BRICK_COUNT;
+
     consoleDrawText(2, 1, "SCORE       LIVES");
     draw_game();
     setScreenOn();
```

The map loaded by `bgInitMapSet` already shows all 70 bricks, so the startup
state says exactly that: every brick shown, nothing to upload.

Finally, upload a row right after waiting for vblank in `main`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     while (1) {
         WaitForVBlank();
+        upload_bricks();
         held = padsCurrent(0);
```

**Checkpoint:** Build and run. Hit bricks: each one vanishes, showing the
backdrop behind it, and adds ten points. Lose all three lives and restart: all
70 bricks return. Clear the wall (it takes a while) and check for
`YOU WIN!` and a score of `700`, then restart. Nothing should flicker, and no
brick should be left behind.

---

## 4. Switch the paddle and ball to dynamic sprites

### What dynamic sprites do

With `oamInitGfxSet`, Part 1 copied the whole sprite sheet into VRAM once and
picked tiles by number. That works for a small sheet. But a game with many
animation frames quickly runs out of sprite VRAM, which holds only 16 KB.

PVSnesLib's **dynamic sprite** engine keeps all frames in ROM. Each frame it
copies **only the frames currently on screen** into VRAM. You describe each
sprite in the `oambuffer` array: position, frame number, attributes, and a
pointer to its graphics in ROM. Then each frame you:

1. set positions and frames, and call `oamDynamic16Draw(id)` or
   `oamDynamic8Draw(id)` for each visible sprite;
2. call `oamInitDynamicSpriteEndFrame()` to finish the frame's sprite list;
3. call `WaitForVBlank()`;
4. call `oamVramQueueUpdate()` to copy the queued frames into VRAM, during
   vblank.

Steps 1 and 2 must happen **before** the vblank wait and step 4 right after
it, so the game loop changes shape. All game logic now runs first, and the
loop ends by waiting for vblank and then doing the VRAM copies.

### Convert the new sprite sheets

The sprite sheets are laid out for the engine. Frames are numbered left to
right, then top to bottom, 128 pixels per row:

- `paddle.png` has one row of eight 16 x 16 frames for each paddle piece (left,
  middle, right). The pieces use only the top 8 pixel rows of each frame. So
  frame `0` is the left piece, `8` the middle, and `16` the right; the next five
  frames of each row are its glimmer animation.
- `ball.png` has sixteen 8 x 8 frames; frame `0` is the normal ball.

Replace the `sprites.pic` rule in the `Makefile`:

```diff
--- a/Makefile
+++ b/Makefile
@@
 all: $(ROMNAME).sfc
 
-sprites.pic: sprites.png
+paddle.pic: paddle.png
+	$(GFXCONV) -s 16 -o 16 -u 16 -p -t png -i $<
+
+ball.pic: ball.png
 	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<
 
 playfield.pic: playfield.png
 	$(GFXCONV) -s 8 -o 16 -u 16 -e 2 -p -m -t png -i $<
 
 backdrop.pic: backdrop.png
 	$(GFXCONV) -s 8 -o 4 -u 4 -e 0 -p -m -t png -i $<
 
-src/data.obj: sprites.pic playfield.pic backdrop.pic
+src/data.obj: paddle.pic ball.pic playfield.pic backdrop.pic
 src/main.obj src/game.obj: src/game.h
```

`-s 16` cuts the paddle sheet into 16 x 16 blocks. Without `-m`, `gfx4snes`
keeps every block in order, so `paddle.pic` is 24 frames x 128 bytes. The ball
keeps Part 1's `-R` option. Both sheets share one 16-color palette, so we load
only `paddle.pal`.

Update the start of `src\data.asm`:

```diff
--- a/src/data.asm
+++ b/src/data.asm
@@
 .section ".breakout_assets" superfree
 
-sprite_tiles:
-.incbin "sprites.pic"
+paddle_tiles:
+.incbin "paddle.pic"
+
+ball_tiles:
+.incbin "ball.pic"
 
 sprite_palette:
-.incbin "sprites.pal"
+.incbin "paddle.pal"
 
 backdrop_tiles:
```

You can run `make clean` and then delete `sprites.png` from your project;
nothing uses it now. (`make clean` removes the generated `sprites.pic`,
`sprites.pal`, and `sprites_data.as` files.)

### Set up and draw dynamic sprites

In `src\main.c`, replace the old sprite label:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
-extern u8 sprite_tiles[];
+extern u8 paddle_tiles[];
+extern u8 ball_tiles[];
 extern u8 sprite_palette[];
```

Replace the whole `draw_game` function with `draw_sprites` and a small helper,
`init_sprite`. `prepare_bricks` and `draw_text` move into the main loop.

```diff
--- a/src/main.c
+++ b/src/main.c
@@
-static void draw_game(void)
+static void draw_sprites(void)
 {
     int i;
 
-    oamSetXY(0, game.ball_x, game.ball_y);
-    for (i = 0; i < 3; ++i)
-        oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
-    prepare_bricks();
-    draw_text();
+    oambuffer[0].oamx = game.ball_x;
+    oambuffer[0].oamy = game.ball_y;
+    oamDynamic8Draw(0);
+
+    for (i = 0; i < 3; ++i) {
+        oambuffer[i + 1].oamx = game.paddle_x + i * 16;
+        oambuffer[i + 1].oamy = PADDLE_Y;
+        oamDynamic16Draw(i + 1);
+    }
+}
+
+static void init_sprite(int id, u8 *graphics, int frame, int size)
+{
+    oambuffer[id].oamframeid = frame;
+    oambuffer[id].oamattribute = OBJ_PAL(0) | OBJ_PRIO(3) | size;
+    oambuffer[id].oamgraphics = graphics;
+    oambuffer[id].oamrefresh = 1;
 }
 
 static void init_video(void)
```

Sprite `0` is the ball and sprites `1` to `3` are the paddle pieces. Note the
plain sprite numbers: dynamic sprites index `oambuffer` directly, unlike
Part 1's `oamSet`, which used multiples of 4.

`init_sprite` fills in what never changes:

- `oamgraphics` points at the sheet in ROM, and `oamframeid` selects a frame in
  it;
- `oamattribute` holds the sprite palette, priority, and the **size bit**;
- `oamrefresh = 1` tells the engine to copy the frame's graphics into VRAM.

The size bit matters. `OBJ_SIZES` marks a small (8 x 8) sprite and
`OBJ_SIZEL` a large (16 x 16) one. The engine stores small frames at the second
VRAM address given to `oamInitDynamicSprite`, and this bit makes the hardware
look there. If the ball uses `OBJ_SIZEL`, it shows part of the paddle instead.

In `init_video`, replace the `oamInitGfxSet`, `oamClear`, `oamSet`, and
`oamSetEx` calls:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     bgInitMapSet(2, backdrop_map, 32 * 32 * 2, SC_32x32, BACKDROP_MAP);
-    oamInitGfxSet(sprite_tiles, 1024, sprite_palette, 32,
-                  0, 0x0000, OBJ_SIZE8_L16);
+
+    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
+    setPalette(sprite_palette, 128, 16 * 2);
+    init_sprite(0, ball_tiles, 0, OBJ_SIZES);
+    for (i = 0; i < 3; ++i)
+        init_sprite(i + 1, paddle_tiles, i * 8, OBJ_SIZEL);
 
     setMode(BG_MODE1, 0);
-    oamClear(0, 0);
-
-    oamSet(0, game.ball_x, game.ball_y, 3, 0, 0, 12, 0);
-    oamSetEx(0, OBJ_SMALL, OBJ_SHOW);
-    for (i = 0; i < 3; ++i) {
-        oamSet((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y,
-               3, 0, 0, 10, 0);
-        oamSetEx((i + 1) * 4, OBJ_LARGE, OBJ_SHOW);
-    }
 
     for (row = 0; row < BRICK_ROWS; ++row) {
```

`oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16)` puts large frames
at VRAM `0x0000` and small frames at `0x1000`, starts both sprite lists at
entry 0, and keeps Part 1's 8 x 8 small / 16 x 16 large sizes. It also resets
the sprite table, so `oamClear` is no longer needed. `setPalette` copies the 16
sprite colors to CGRAM 128, where sprite palette 0 begins.

At the end of `init_video`, `draw_game` no longer exists. Draw the text
instead; the sprites appear on the first pass through the loop:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     consoleDrawText(2, 1, "SCORE       LIVES");
-    draw_game();
+    draw_text();
     setScreenOn();
 }
```

Now reshape the main loop:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     while (1) {
-        WaitForVBlank();
-        upload_bricks();
         held = padsCurrent(0);
         pressed = padsDown(0);
         movement = 0;
         if (held & KEY_LEFT)
             --movement;
         if (held & KEY_RIGHT)
             ++movement;
         game_tick(&game, movement, pressed & (KEY_B | KEY_START));
-        draw_game();
+
+        draw_sprites();
+        prepare_bricks();
+        draw_text();
+
+        oamInitDynamicSpriteEndFrame();
+        WaitForVBlank();
+        oamVramQueueUpdate();
+        upload_bricks();
     }
     return 0;
 }
```

Everything that only prepares data in RAM happens first. The vblank wait
comes next, and both VRAM copies (sprite frames and brick row) come right
after it, while vblank is still going.

**Checkpoint:** Build and run. The paddle is now a cyan bar with rounded ends,
and the ball is a small silver ball. Both move and collide exactly as before,
and bricks still disappear and come back on restart. If the ball looks like a
piece of the paddle, check its `OBJ_SIZES` attribute.

---

## 5. Make the paddle and ball glimmer

### Report hits from the game logic

The glimmer should start when the ball hits something. Only `game.c` knows
when that happens, but `game.c` must not know about sprites. So the game
records **what** was hit during the current tick in a set of bit flags, and
`main.c` decides how to show it.

In `src\game.h`, add the flags and a field. Put `hits` before the `bricks`
array:

```diff
--- a/src/game.h
+++ b/src/game.h
@@
 #define BRICK_WIDTH 16
 #define BRICK_HEIGHT 8
 
+#define HIT_WALL 1
+#define HIT_PADDLE 2
+#define HIT_BRICK 4
+
 enum GameState {
@@
     int bricks_left;
     int state;
+    int hits;
     unsigned char bricks[BRICK_COUNT];
 } Game;
```

In `src\game.c`, clear the flags in `game_reset` and at the start of every
tick, and set one at each kind of bounce:

```diff
--- a/src/game.c
+++ b/src/game.c
@@ void game_reset(Game *game)
     game->lives = 3;
     game->bricks_left = BRICK_COUNT;
     game->state = STATE_READY;
+    game->hits = 0;
     for (i = 0; i < BRICK_COUNT; ++i)
         game->bricks[i] = 1;
@@ static int hit_brick(Game *game, int x, int y)
             index = row * BRICK_COLS + col;
             if (game->bricks[index]) {
+                game->hits |= HIT_BRICK;
                 game->bricks[index] = 0;
                 game->score += 10;
@@ static void move_ball_x(Game *game)
     next_x = game->ball_x + (game->vx > 0 ? 1 : -1);
     if (next_x < FIELD_LEFT || next_x + BALL_SIZE > FIELD_RIGHT) {
+        game->hits |= HIT_WALL;
         game->vx = -game->vx;
     } else if (hit_brick(game, next_x, game->ball_y)) {
@@ static void move_ball_y(Game *game)
     next_y = game->ball_y + (game->vy > 0 ? 1 : -1);
     if (next_y < FIELD_TOP) {
+        game->hits |= HIT_WALL;
         game->vy = -game->vy;
     } else if (game->vy > 0 &&
                game->ball_y + BALL_SIZE <= PADDLE_Y &&
                next_y + BALL_SIZE > PADDLE_Y &&
                game->ball_x + BALL_SIZE > game->paddle_x &&
                game->ball_x < game->paddle_x + PADDLE_WIDTH) {
+        game->hits |= HIT_PADDLE;
         game->ball_y = PADDLE_Y - BALL_SIZE;
         paddle_bounce(game);
@@ void game_tick(Game *game, int movement, int action_pressed)
 {
     int step;
 
+    game->hits = 0;
     if (game->state == STATE_GAME_OVER || game->state == STATE_WON) {
```

The text after `@@` names the function each change is in. `|=` sets a bit
without clearing the others, so one tick can report a wall **and** a brick.

### Play the animation

Each glimmer is five frames (1 to 5) shown for three video frames each, then
back to frame 0. In `src\main.c`, add the timing constants and two timers:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 #define BRICK_TILE_Y (BRICK_Y / 8)
 
+#define GLIMMER_FRAMES 5
+#define GLIMMER_TICKS 3
+
 extern u8 paddle_tiles[];
@@
 static int shown_state = -1;
+static int paddle_glimmer;
+static int ball_glimmer;
 
 static void prepare_bricks(void)
```

Add two helpers above `draw_sprites`, and make `draw_sprites` start the timers
and pick frames:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     shown_state = game.state;
 }
 
+static int glimmer_frame(int *timer)
+{
+    if (*timer == 0)
+        return 0;
+    --*timer;
+    return GLIMMER_FRAMES - *timer / GLIMMER_TICKS;
+}
+
+static void show_frame(int id, int frame)
+{
+    if (oambuffer[id].oamframeid != frame) {
+        oambuffer[id].oamframeid = frame;
+        oambuffer[id].oamrefresh = 1;
+    }
+}
+
 static void draw_sprites(void)
 {
     int i;
+    int paddle_frame;
+    int ball_frame;
+
+    if (game.hits & HIT_PADDLE)
+        paddle_glimmer = GLIMMER_FRAMES * GLIMMER_TICKS;
+    if (game.hits)
+        ball_glimmer = GLIMMER_FRAMES * GLIMMER_TICKS;
+    paddle_frame = glimmer_frame(&paddle_glimmer);
+    ball_frame = glimmer_frame(&ball_glimmer);
 
     oambuffer[0].oamx = game.ball_x;
     oambuffer[0].oamy = game.ball_y;
+    show_frame(0, ball_frame);
     oamDynamic8Draw(0);
 
     for (i = 0; i < 3; ++i) {
         oambuffer[i + 1].oamx = game.paddle_x + i * 16;
         oambuffer[i + 1].oamy = PADDLE_Y;
+        show_frame(i + 1, i * 8 + paddle_frame);
         oamDynamic16Draw(i + 1);
     }
 }
```

A hit sets a timer to 15. `glimmer_frame` counts it down and turns 14...0 into
frames 1, 1, 1, 2, 2, 2, ... 5, 5, 5; at zero it returns frame 0. Another hit
restarts the animation. Every hit makes the ball glimmer; only paddle hits make
the paddle glimmer. Each paddle piece adds its row's first frame (`i * 8`).

`show_frame` sets `oamrefresh` only when the frame actually changes, so the
engine copies new graphics to VRAM only when needed.

Remember the uninitialized-globals rule from section 3. Set both timers in
`init_video`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
     setPalette(sprite_palette, 128, 16 * 2);
+    paddle_glimmer = 0;
+    ball_glimmer = 0;
     init_sprite(0, ball_tiles, 0, OBJ_SIZES);
```

**Checkpoint:** Build and run. Launch the ball. A white and yellow flash sweeps
across the ball whenever it touches a wall, brick, or the paddle, and a shine
runs along the paddle from left to right when the ball lands on it. Between
hits, both look normal. The glimmer never starts on its own when the game
boots.

---

## 6. Play music

### How SNES sound works

The SNES has a separate sound CPU, the SPC700, with its own 64 KB of RAM.
PVSnesLib includes **snesmod**, a sound driver for that CPU:

- `spcBoot()` uploads the driver;
- `spcSetBank()` tells it where the sound data is in ROM;
- `spcLoad()` copies a song into the sound RAM;
- `spcPlay()` starts the song, and `spcStop()` stops it.

From then on, the main CPU must call `spcProcess()` once per frame to keep the
driver fed.

Songs and sound effects start as Impulse Tracker (`.it`) modules. The SDK's
`smconv` tool converts a list of modules into a **soundbank**: an assembly file
and a header with an ID for each module. The PVSnesLib build rules already
know how to run it; you just list the files.

### Build a soundbank

Update your `Makefile`. The new lines go in two places: the audio variables
**before** the `include` line, and everything else after it.

```diff
--- a/Makefile
+++ b/Makefile
@@
 $(error PVSnesLib not found at $(PVSNESLIB_HOME). Place this project in snes-examples/breakout or set PVSNESLIB_HOME)
 endif
 
+AUDIOFILES := res/effectssfx.it res/pollen8.it
+export SOUNDBANK := res/soundbank
+
 export ROMNAME := breakout
 export ROMTITLE := BREAKOUT TUTORIAL
 
 include $(PVSNESLIB_HOME)/devkitsnes/snes_rules
 
-.PHONY: all clean
-
-all: $(ROMNAME).sfc
+SMCONVFLAGS := -s -o $(SOUNDBANK) -V -b 5 -f
+
+.PHONY: all clean musics
+
+all: musics $(ROMNAME).sfc
+
+musics: $(SOUNDBANK).obj
+
+res/%.it: $(PVSNESLIB_HOME)/snes-examples/audio/effectsandmusic/res/%.it
+	mkdir -p res
+	cp $< $@
 
 paddle.pic: paddle.png
@@
 src/main.obj src/game.obj: src/game.h
 
-clean: cleanBuildRes cleanRom cleanGfx
+clean: cleanBuildRes cleanRom cleanGfx cleanAudio
```

What each part does:

- `AUDIOFILES` lists the modules. **The effects module must be first**:
  snesmod takes sound effects from the first module in the bank.
- `SOUNDBANK` is where the generated files go: `res/soundbank.asm`,
  `res/soundbank.h`, and `res/soundbank.bnk`.
- `SMCONVFLAGS` makes a soundbank (`-s`) in ROM bank 5 (`-b 5`), checks module
  sizes against the effects module (`-f`), and prints memory use (`-V`).
  Part 1's `hdr.asm` has 8 banks of 32 KB, so bank 5 exists.
- `musics` builds the soundbank before the ROM.
- The `res/%.it` rule copies each module from the SDK's sound example the first
  time it is needed. `$<` is the SDK file and `$@` is the file in your `res`
  folder. The `mkdir -p` and `cp` lines start with a **tab**.
- `cleanAudio` deletes the generated soundbank on `make clean`.

### Start the music

At the top of `src\main.c`, include the generated header and declare the
soundbank label. `smconv` creates `SOUNDBANK__` in `soundbank.asm`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 #include <snes.h>
 #include "game.h"
+#include "res/soundbank.h"
 
 #define PLAYFIELD_MAP 0x6C00
@@
 #define GLIMMER_TICKS 3
 
+extern char SOUNDBANK__;
+
 extern u8 paddle_tiles[];
```

Add `start_music` above `init_sprite`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
         oamDynamic16Draw(i + 1);
     }
 }
 
+static void start_music(void)
+{
+    spcStop();
+    spcLoad(MOD_POLLEN8);
+    spcPlay(0);
+}
+
 static void init_sprite(int id, u8 *graphics, int frame, int size)
```

`MOD_POLLEN8` comes from `res/soundbank.h`; `MOD_EFFECTSSFX` is the effects
module. `spcPlay(0)` starts from the song's first pattern.

In `main`, boot the driver before anything else, start the music after the
screen is set up, and call `spcProcess` every frame before waiting for vblank:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     int movement;
 
+    spcBoot();
+    spcSetBank(&SOUNDBANK__);
+
     game_reset(&game);
     init_video();
+    start_music();
 
     while (1) {
@@
         draw_text();
 
+        spcProcess();
         oamInitDynamicSpriteEndFrame();
         WaitForVBlank();
```

Uploading the song takes most of a second, so the game appears and then
responds to the controller a moment later. This happens once, at power-on.

**Checkpoint:** Run `make`. The build now prints
`Compiling Soundbank ...` and `smconv`'s memory report, and your project has a
`res` folder with the two `.it` files and the generated soundbank. Run the
ROM: music starts shortly after the game appears and keeps looping while you
play. The game, glimmer, and bricks still work as in section 5.

---

## 7. Add sound effects

### Load and trigger effects

The effects module contains five samples:

| Index | Sound |
|---|---|
| 0 | Tada |
| 1 | Hall strings |
| 2 | Honky-tonk piano |
| 3 | Marimba |
| 4 | Cowbell |

`spcLoadEffect(index)` copies one into sound RAM. Loading a song with
`spcLoad` replaces the loaded effects, so they must be loaded again **after**
every `spcLoad`. `spcEffect(pitch, index, volpan)` plays one:

- `pitch`: 4 is the sample's natural pitch (16 kHz), 2 is an octave lower,
  and higher numbers are higher;
- `volpan`: the volume (0 to 15) times 16, plus the pan position (0 left, 8
  center, 15 right).

The game uses eight sounds:

| Event | Sound |
|---|---|
| Launch | Piano |
| Ball hits a brick | Cowbell |
| Ball hits the paddle | Marimba |
| Ball hits a wall | Marimba, higher |
| Life lost | Strings |
| Game over | Music stops; low strings |
| Win | Music stops; tada |
| New game after game over or a win | Music restarts |

### Name the effects and load them

Add names for the indexes in `src\main.c`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 #define GLIMMER_TICKS 3
 
+#define SFX_TADA 0
+#define SFX_STRINGS 1
+#define SFX_PIANO 2
+#define SFX_MARIMBA 3
+#define SFX_COWBELL 4
+
 extern char SOUNDBANK__;
```

Add `play_effect` above `start_music`, and load the effects after the song:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
         oamDynamic16Draw(i + 1);
     }
 }
 
+static void play_effect(int sound, int pitch)
+{
+    spcEffect(pitch, sound, 15 * 16 + 8);
+}
+
 static void start_music(void)
 {
+    int i;
+
     spcStop();
     spcLoad(MOD_POLLEN8);
+    for (i = 0; i < 5; ++i)
+        spcLoadEffect(i);
     spcPlay(0);
 }
```

Every effect plays at full volume (15), centered (8).

### Choose a sound for each event

State changes are the most important events, so `play_sounds` checks them
first. It compares the state before and after the tick, then falls back to the
hit flags from section 5. Add it after `start_music`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
         spcLoadEffect(i);
     spcPlay(0);
 }
 
+static void play_sounds(int old_state)
+{
+    if (game.state != old_state) {
+        if (game.state == STATE_PLAYING) {
+            play_effect(SFX_PIANO, 4);
+        } else if (game.state == STATE_GAME_OVER) {
+            spcStop();
+            play_effect(SFX_STRINGS, 2);
+        } else if (game.state == STATE_WON) {
+            spcStop();
+            play_effect(SFX_TADA, 4);
+        } else if (old_state == STATE_PLAYING) {
+            play_effect(SFX_STRINGS, 3);
+        } else {
+            start_music();
+        }
+    } else if (game.hits & HIT_BRICK) {
+        play_effect(SFX_COWBELL, 4);
+    } else if (game.hits & HIT_PADDLE) {
+        play_effect(SFX_MARIMBA, 4);
+    } else if (game.hits & HIT_WALL) {
+        play_effect(SFX_MARIMBA, 6);
+    }
+}
+
 static void init_sprite(int id, u8 *graphics, int frame, int size)
```

The game enters `STATE_READY` in two ways. Coming from `STATE_PLAYING` means
a life was lost. Coming from game over or a win means a new game, so the music
starts again. `spcStop` only stops the song; an effect started right after it
still plays. Only one hit sound plays per tick, chosen in the order brick,
paddle, wall.

Finally, remember the state before each tick in `main` and play the sounds
after it:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     u16 held;
     u16 pressed;
     int movement;
+    int old_state;
 
     spcBoot();
@@
         if (held & KEY_RIGHT)
             ++movement;
+        old_state = game.state;
         game_tick(&game, movement, pressed & (KEY_B | KEY_START));
 
+        play_sounds(old_state);
         draw_sprites();
         prepare_bricks();
         draw_text();
```

**Checkpoint:** Build and run. Launching plays a piano note. The ball makes a
marimba sound on the paddle, a higher marimba on walls, and a cowbell on
bricks, all over the music. Losing a life plays strings. Losing the last life
stops the music with low strings. A new game brings the music back.

---

## 8. Test the finished game

Your project folder now looks like this:

```text
pvsneslib\
    snes-examples\
        breakout\
            Makefile
            backdrop.png
            ball.png
            paddle.png
            playfield.png
            THIRD_PARTY_NOTICES.txt
            res\
                effectssfx.it
                pollen8.it
            src\
                main.c
                game.h
                game.c
                data.asm
            tests\              (only if you set up the optional tests)
                test_game.c
```

Generated graphics, the soundbank, object files, and the ROM will also be
there. For a clean rebuild:

```sh
make clean
make
```

Test in this order:

1. At power-on: backdrop, walls, HUD frame, 70 bricks, score `000`, lives `3`,
   no glimmer, and music after a short pause.
2. Move to both walls: the paddle stops at each; the ball stays centered on it.
3. Launch (piano): the ball moves; holding the button does not launch again.
4. Walls: higher marimba and a ball glimmer on each side and the top.
5. Paddle: marimba, a ball glimmer, and a shine across the paddle; check the
   three rebound zones from Part 1.
6. Bricks: cowbell and glimmer; the brick vanishes, showing the backdrop; ten
   points each; a hole cannot score twice.
7. Miss: strings, one life lost, bricks and score kept.
8. Lose the last life: music stops, low strings, `GAME OVER`.
9. New game: music restarts, score `000`, lives `3`, all 70 bricks restored.
10. Clear the wall: music stops, tada, `YOU WIN!`, score `700`. Start a new
    game again.

Only **now**, if you want to compare your work, open the finished reference
[main.c](part2/src/main.c), [game.h](part2/src/game.h),
[game.c](part2/src/game.c), and [data.asm](part2/src/data.asm).

Emulator testing is not proof of correct behavior on a physical SNES. Test with
accurate emulators and real hardware before publishing a game.

## Troubleshooting

| Symptom | Check |
|---|---|
| `No rule to make target 'backdrop.png'` (or another PNG) | Copy the four PNG files from the repository's `part2` folder into your project root. |
| `No rule to make target 'res/effectssfx.it'` | Check that both `.it` files exist in `<SDK root>\snes-examples\audio\effectsandmusic\res`, and that the `res/%.it` rule is spelled exactly as shown. |
| `missing separator` | Makefile command lines need a real tab. |
| `res/soundbank.h: No such file` | Add `musics` before `$(ROMNAME).sfc` in `all`, and put `AUDIOFILES`/`SOUNDBANK` **before** the `include` line. |
| Font text invisible or wrong colors after section 1 | Use `consoleInitDefaultText(1)`, not `0`. |
| Backdrop or playfield missing | Remove the matching `bgSetDisable` call, and check the layer numbers (`1` is BG2, `2` is BG3). |
| Garbage tiles on a background | Check that the VRAM addresses in the table in section 1 do not overlap. |
| Bricks bounce the ball but stay visible | Expected after section 2; section 3 erases them. Afterwards, check that `upload_bricks` runs right after `WaitForVBlank`. |
| Some bricks stay visible or do not come back | `upload_bricks` must copy only one row and then `return`. Check `row_changed` is cleared in `init_video`. |
| The game slows down near bricks | Use the arithmetic `hit_brick` from section 2, with the `unsigned int` casts. |
| Ball looks like part of the paddle | The ball's `init_sprite` call needs `OBJ_SIZES`; the paddle uses `OBJ_SIZEL`. |
| Paddle or ball missing | Call `oamInitDynamicSpriteEndFrame()` before `WaitForVBlank()` and `oamVramQueueUpdate()` after it. |
| Wrong paddle pieces | Each piece's base frame is `i * 8`: 0, 8, and 16. |
| Glimmer at power-on, or a random first sound | Set `paddle_glimmer`, `ball_glimmer`, and `row_changed` to 0 in `init_video`; uninitialized globals are not zeroed. |
| No sound at all | Call `spcBoot` and `spcSetBank` before `start_music`, and `spcProcess` every frame. |
| Music plays but no effects | Load the effects with `spcLoadEffect` **after** `spcLoad`, and keep `effectssfx.it` first in `AUDIOFILES`. |
| Other problems | See the [Part 1 troubleshooting table](README.md#troubleshooting). |

## Running the gameplay tests

The gameplay rules can still be tested without the SNES. After section 5, copy
[part2/tests/test_game.c](part2/tests/test_game.c) from your copy of this
repository over your project's `tests\test_game.c` (create the `tests` folder
if you skipped this in Part 1). Part 1's test file no longer fits the new brick
layout.

Then run exactly the commands from
[Running the gameplay tests](README.md#running-the-gameplay-tests) in Part 1.
The expected output is the same:

```text
PASS: 8 gameplay test groups
```

These tests also cover the gapless grid, neighboring bricks and corners, and
the hit flags. They do not test graphics, sprites, or sound; the emulator
checkpoints do.

## Licensing

[THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt) preserves the PVSnesLib
notice for the library code and font linked into the ROM. The Part 2 PNG
artwork is original to this tutorial and can be regenerated with the optional
[make_art.py](part2/tools/make_art.py) using Python's standard library. No
project-wide license has been selected for the tutorial, source, or artwork.

The music and effects modules come from the PVSnesLib SDK's sound example.
This repository does not include them; your Makefile copies them from your SDK
at build time. Check their terms before you distribute a ROM that contains
them.

## Official references

- [Dynamic sprites (wiki)][dynamic-wiki]
- [Backgrounds (wiki)][backgrounds-wiki]
- [Sounds and musics (wiki)][sound-wiki]
- [Sprite and OAM API (`sprite.h`)][sprites]
- [Background API (`background.h`)][backgrounds]
- [DMA API (`dma.h`)][dma]
- [Sound API (`sound.h`)][sound]
- [gfx4snes options, pinned to 4.6.0][gfx]
- [Effects and music example, pinned to 4.6.0][sound-example]

The header links are pinned to PVSnesLib 4.6.0; the same headers are in
`pvsneslib\include\snes` inside your SDK root. The wiki describes the latest
release, which may differ slightly from 4.6.0.

[dynamic-wiki]: https://github.com/alekmaul/pvsneslib/wiki/Dynamic-Sprites
[backgrounds-wiki]: https://github.com/alekmaul/pvsneslib/wiki/Backgrounds
[sound-wiki]: https://github.com/alekmaul/pvsneslib/wiki/Sounds-and-Musics
[sprites]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/sprite.h
[backgrounds]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/background.h
[dma]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/dma.h
[sound]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/sound.h
[gfx]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/tools/gfx4snes/readme.md
[sound-example]: https://github.com/alekmaul/pvsneslib/tree/4.6.0/snes-examples/audio/effectsandmusic
