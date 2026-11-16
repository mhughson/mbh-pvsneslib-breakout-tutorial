# Write a Breakout-style SNES game with PVSnesLib

This is a **build-it-yourself tutorial**, not a tour of a finished program.
Start with an empty project inside your installed PVSnesLib folder. Write a
little code, build it, try it, and then add the next feature.

You should know basic C: variables, functions, loops, arrays, pointers, and
structs. You do not need assembly or prior SNES programming experience.

By the end you will have a paddle, a bouncing ball, fifty colored bricks,
scoring, three lives, and win/game-over screens with restarting. This first
version has one level and no sound. It is an original Breakout-style exercise.

![What we are working toward](screenshots/ready.png)

## Tutorial versus finished reference

**Do not copy this repository's `src` folder into your project.** It contains
the finished reference game, not your starting point. You will create your own
source files below. You only need the supplied [sprites.png](sprites.png)
artwork and [third-party notices](THIRD_PARTY_NOTICES.txt) from this repository,
plus, optionally, the [test file](tests/test_game.c) described at the end.

To get those files, clone this repository or use GitHub's **Code** button and
**Download ZIP**, and keep the copy outside your SDK folder. `sprites.png` and
`THIRD_PARTY_NOTICES.txt` sit next to this README, and `test_game.c` is in its
`tests` folder. The steps below say when to copy each one.

The reference [main.c](src/main.c), [game.h](src/game.h), and
[game.c](src/game.c) are available for comparison **after** you complete the
steps. The repository's [Makefile](Makefile) builds that reference using a local
SDK under `vendor/pvsneslib`; that is a separate convenience, not the learner's
working directory or installation requirement.

## How to follow the code edits

Every step identifies the file to edit. When a whole file is given, create or
replace that file. Later steps use **diff blocks with surrounding unchanged
code**, so you can see exactly where an addition belongs:

```diff
 int existing_line;
-old_line();
+new_line();
 next_existing_line();
```

Keep the unmarked lines, remove lines starting with `-`, and add lines starting
with `+`. Do not type those markers, the `---`/`+++` filenames, or the `@@`
separators into your C files. Separate `@@` sections describe separate edits
in the same file. A diff is an editing guide, not a complete file to paste.

Prose names files with Windows-style backslashes (`src\main.c`), while diff
headers and shell commands use forward slashes (`src/main.c`), which work on
every operating system. Both mean the same file. Either kind of line ending
(LF or Windows CRLF) is fine in your source files.

Follow the steps in order; each edit assumes the previous checkpoint is done.
There are no omitted function bodies to retrieve from the finished source.

## Contents

1. [Create an empty project and display text](#1-create-an-empty-project-and-display-text)
2. [Draw a paddle and move it](#2-draw-a-paddle-and-move-it)
3. [Attach a ball and launch it](#3-attach-a-ball-and-launch-it)
4. [Move the ball and bounce off walls](#4-move-the-ball-and-bounce-off-walls)
5. [Bounce off the paddle](#5-bounce-off-the-paddle)
6. [Add a wall of destructible bricks](#6-add-a-wall-of-destructible-bricks)
7. [Add lives, winning, and restarting](#7-add-lives-winning-and-restarting)
8. [Draw the score, lives, and prompts](#8-draw-the-score-lives-and-prompts)
9. [Test the complete game](#9-test-the-complete-game)

See also [troubleshooting](#troubleshooting),
[gameplay tests](#running-the-gameplay-tests), [licensing](#licensing), and
[official references](#official-references).

---

## 1. Create an empty project and display text

### Install and check PVSnesLib

This tutorial targets **PVSnesLib 4.6.0**. Newer releases exist (4.7.1 was the
latest when this tutorial was tested), so download the 4.6.0 archive for your
operating system instead of following the "latest" link:

| Operating system | Archive |
|---|---|
| Windows | [pvsneslib_460_64b_windows_release.zip][windows-zip] |
| Linux | [pvsneslib_460_64b_linux_release.zip][linux-zip] |
| macOS | [pvsneslib_460_64b_darwin_release.zip][macos-zip] |

These prebuilt [release archives][release] contain the compiler and tools. Do
not use GitHub's "Source code" download or a `git clone` of the repository
instead: they hold only the sources, with no compiler in `devkitsnes\bin`.
Also follow the official [installation guide][installation] for your operating
system's prerequisites (for example, macOS needs `gnu-sed` from Homebrew).

Later releases may work too: in one test, the finished game built and played
correctly on 4.7.1 on Windows. But the steps below were written and checked
against 4.6.0.

On Windows, use **MSYS2 UCRT64** for the build commands, as recommended by the
official guide, whose MSYS2 section covers installing and updating it. Open its
terminal from the Start menu entry named **MSYS2 UCRT64** (not "MSYS2 MSYS" or
"MSYS2 MINGW64"). You can edit files in VS Code or any other editor. In that
terminal, install Make if needed:

```sh
pacman -S make
```

The guide also has you update MSYS2 first with `pacman -Suuy`. That update may
close the terminal when it finishes; if it does, open a new MSYS2 UCRT64
terminal and run the command again.

Extract the archive to a path without spaces. It contains one top-level
`pvsneslib` folder, which becomes your **SDK root**. This tutorial uses
`C:\snesdev` as the example location; substitute your own:

```text
C:\snesdev\pvsneslib\          <- the SDK root
    devkitsnes\                compiler, linker, converters, build rules
    pvsneslib\                 the library: headers and sources
    snes-examples\             example projects; your project goes here
    vscode-template\           optional VS Code project template
```

The SDK root is the folder that directly contains `devkitsnes`. The inner
`pvsneslib` folder contains the library itself. If you extract into a folder
that is itself called `pvsneslib`, the SDK root ends up one level deeper than
you might expect. If you have already installed PVSnesLib 4.6.0, use that
installation; do not install another SDK inside this tutorial repository.

The shell commands below use MSYS2's `/c/...` spelling for a Windows `C:\...`
path. This is the spelling expected by PVSnesLib's Make rules, even on Windows.
On Linux/macOS, substitute your installation's absolute path.

The official guide also suggests a permanent `PVSNESLIB_HOME` environment
variable. This tutorial does not need one. If you have one, it must name the
SDK root in the same `/c/...` form (a path with backslashes breaks the Make
rules), and it overrides the default in your project's Makefile below. With the
layout above that means `/c/snesdev/pvsneslib`; the guide's example value,
`/c/snesdev`, would be one level too high. Open a new terminal after changing
it.

First prove the toolchain works:

```sh
cd /c/snesdev/pvsneslib
export PVSNESLIB_HOME="$(pwd)"
make -C snes-examples/hello_world
```

This example prints a short `COMPILATION SUMMARY` that says
`No warnings or errors found.`, followed by ROM and RAM usage. (The long output
described below is what your own project prints.) The build creates
`snes-examples/hello_world/hello_world.sfc`; open that ROM in a SNES emulator.
[Mesen](https://www.mesen.ca/) is one option.

**Checkpoint:** Hello World displays its text. Fix installation problems here,
before writing the game. The official [Hello World walkthrough][hello] can help.

### Create your project inside the installation

From the SDK root, create a **new, empty** example directory:

```sh
mkdir -p snes-examples/breakout/src
cd snes-examples/breakout
```

If `breakout` already contains another project, use a different new directory
name instead; do not overwrite it. Keep the same two-level nesting under the
SDK root. (The SDK's own, unrelated Breakout example lives deeper, at
`snes-examples/systems/games/breakout`.)

Your working directory is now `C:\snesdev\pvsneslib\snes-examples\breakout`.
Every subsequent `make` command is run **here**, not in this repository and
not in the SDK root. You can open this new directory in VS Code.

Create `Makefile` in that directory. Its name is exactly `Makefile`, with no
extension (some editors, such as Notepad, quietly add `.txt`):

```make
export PVSNESLIB_HOME ?= $(abspath $(CURDIR)/../..)

ifeq ($(wildcard $(PVSNESLIB_HOME)/devkitsnes/snes_rules),)
$(error PVSnesLib not found at $(PVSNESLIB_HOME). Place this project in snes-examples/breakout or set PVSNESLIB_HOME)
endif

export ROMNAME := breakout
export ROMTITLE := BREAKOUT TUTORIAL

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

.PHONY: all clean

all: $(ROMNAME).sfc

clean: cleanBuildRes cleanRom cleanGfx
```

There are no command recipes in this first Makefile; the SDK supplies them.
The default SDK path goes up two directories from your new project. An existing
environment variable or command-line setting overrides it. Thus `make` also
works in a new terminal without exporting the variable again, provided your
project is in this layout.

The SDK rules find the `.c` and `.asm` files in your project folder, in `src`,
and in folders up to two levels below `src`; they compile those files and link a
ROM. PVSnesLib 4.6.0 generates `hdr.asm` automatically; **do not write a custom
ROM header**. See the [header guide][header].

Create `src\main.c` with this entire program:

```c
#include <snes.h>

int main(void)
{
    setScreenOff();
    consoleInitDefaultText(0);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x6800, SC_32x32);
    setMode(BG_MODE1, 0);
    bgSetDisable(1);
    bgSetDisable(2);
    consoleDrawText(7, 12, "MY FIRST BREAKOUT");
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
    return 0;
}
```

Build:

```sh
make
```

The output is long, and the linker prints harmless messages such as
`Label "SECTIONSTART_.rodata" was defined more than once` and
`DISCARD: ... was discarded`. The build worked if the output ends with
`Build finished successfully !` and `breakout.sfc` now exists. A real failure
stops with an error message and never prints that line.

Open **your new project's** `breakout.sfc` in the emulator.

**Checkpoint:** a text-only screen says `MY FIRST BREAKOUT`. There is no
paddle, ball, or game yet. You have built the first version yourself.

<img width="770" height="729" alt="image" src="https://github.com/user-attachments/assets/6e5ef5b1-95f3-4b7b-93d3-5e2368dd265e" />


### Just enough graphics terminology

The SNES assembles its picture from tiles, rather than drawing arbitrary
rectangles into a modern framebuffer:

| Term | What we will use it for |
|---|---|
| Tile | An 8-by-8 piece of artwork. |
| Palette | Colors selected by the artwork's color indices. |
| Background | A tile grid; ours holds text and a border. |
| Sprite | A movable object: paddle, ball, or brick. |
| VRAM | Graphics memory holding tiles and background maps. |
| OAM | The table of sprite positions, sizes, and artwork. |
| VBlank | The pause between frames when the library transfers graphics updates. |

Our screen is 256 by 224 pixels. X increases rightward; Y increases downward.
Sprites use pixel coordinates, but console text uses 8-by-8 character cells.
`consoleDrawText(2, 1, "HELLO")` starts at pixel `(16, 8)`.

---

## 2. Draw a paddle and move it

### Add artwork, not finished game code

Copy only [sprites.png](sprites.png) into your new project's root. Also copy
[THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt) there to preserve the notice
for the PVSnesLib code/font that will be linked into the ROM. Take both files
from your copy of this repository.

<img width="938" height="147" alt="image" src="https://github.com/user-attachments/assets/b170c99a-0b09-403d-9710-ea8a90616833" />


The original sheet is a 128-by-16 indexed PNG. Across the top are five brick
colors, a paddle segment, and a ball. The lower eight rows are transparent.
No graphics editor or Python installation is needed for this tutorial.

Add the conversion rule to **your** `Makefile`, between `all` and `clean`:

```diff
--- a/Makefile
+++ b/Makefile
@@
 all: $(ROMNAME).sfc

+sprites.pic: sprites.png
+	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<
+
+src/data.obj: sprites.pic
+
 clean: cleanBuildRes cleanRom cleanGfx
```

The new `$(GFXCONV)` command must start with a real **tab**, not spaces.
The `clean` target still has no recipe of its own.

The converter writes `sprites.pic` (1,024 bytes of tile data) and `sprites.pal`
(32 bytes of palette data). It also writes `sprites.inc` and `sprites_data.as`,
which this tutorial does not use. `-R` preserves tile order, `-u 16` selects
4-bit pixels, `-o 16` outputs sixteen colors, and `-p` requests a palette.
Do not deduplicate this sheet: our tile numbers depend on its layout.

Create `src\data.asm`:

```asm
.include "hdr.asm"

.section ".breakout_assets" superfree

sprite_tiles:
.incbin "sprites.pic"

sprite_palette:
.incbin "sprites.pal"

.ends
```

These labels let C refer to the binary graphics. The Makefile dependency ensures
conversion happens before this file is assembled.

### Keep gameplay separate from SNES drawing

Create `src\game.h`. For now, the game contains **only a paddle position**:

```c
#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#define FIELD_LEFT 16
#define FIELD_RIGHT 240

#define PADDLE_WIDTH 48
#define PADDLE_HEIGHT 8
#define PADDLE_Y 200
#define PADDLE_SPEED 3

typedef struct Game {
    int paddle_x;
} Game;

void game_reset(Game *game);
void game_tick(Game *game, int movement, int action_pressed);

#endif
```

Create `src\game.c`:

```c
#include "game.h"

void game_reset(Game *game)
{
    game->paddle_x = (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2;
}

void game_tick(Game *game, int movement, int action_pressed)
{
    (void)action_pressed;
    game->paddle_x += movement * PADDLE_SPEED;
    if (game->paddle_x < FIELD_LEFT)
        game->paddle_x = FIELD_LEFT;
    if (game->paddle_x + PADDLE_WIDTH > FIELD_RIGHT)
        game->paddle_x = FIELD_RIGHT - PADDLE_WIDTH;
}
```

`movement` will be -1, 0, or 1. The unused action parameter is reserved for
launching in the next step. Signed positions allow subtraction without unsigned
wraparound; our values fit the SNES compiler's 16-bit `int`.

Add the header dependency to your `Makefile`:

```diff
--- a/Makefile
+++ b/Makefile
@@
 src/data.obj: sprites.pic
+src/main.obj src/game.obj: src/game.h

 clean: cleanBuildRes cleanRom cleanGfx
```

Now replace the text-only `src\main.c` with this complete paddle version:

```c
#include <snes.h>
#include "game.h"

extern u8 sprite_tiles[];
extern u8 sprite_palette[];

Game game;

static void draw_game(void)
{
    int i;

    for (i = 0; i < 3; ++i)
        oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
}

static void init_video(void)
{
    int row;
    int i;

    setScreenOff();
    consoleInitDefaultText(0);
    bgSetGfxPtr(0, 0x3000);
    bgSetMapPtr(0, 0x6800, SC_32x32);
    oamInitGfxSet(sprite_tiles, 1024, sprite_palette, 32,
                  0, 0x0000, OBJ_SIZE8_L16);

    setMode(BG_MODE1, 0);
    bgSetDisable(1);
    bgSetDisable(2);
    oamClear(0, 0);
    for (i = 0; i < 3; ++i) {
        oamSet((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y,
               3, 0, 0, 10, 0);
        oamSetEx((i + 1) * 4, OBJ_LARGE, OBJ_SHOW);
    }

    consoleDrawText(1, 3, "##############################");
    for (row = 4; row < 28; ++row) {
        consoleDrawText(1, row, "#");
        consoleDrawText(30, row, "#");
    }
    draw_game();
    setScreenOn();
}

int main(void)
{
    u16 held;
    u16 pressed;
    int movement;

    game_reset(&game);
    init_video();

    while (1) {
        WaitForVBlank();
        held = padsCurrent(0);
        pressed = padsDown(0);
        movement = 0;
        if (held & KEY_LEFT)
            --movement;
        if (held & KEY_RIGHT)
            ++movement;
        game_tick(&game, movement, pressed & (KEY_B | KEY_START));
        draw_game();
    }
    return 0;
}
```

The paddle is three 16-by-16 sprites. Only their top eight pixels are visible,
so its collision rectangle will be 48 by 8. The SNES does not offer a 16-by-8
hardware sprite size.

`oamSet` takes an OAM **byte offset**, X, Y, priority, flip X, flip Y, tile
number, and palette. Entries 1, 2, and 3 use offsets 4, 8, and 12. Entry 0 is
reserved for our future ball. `OBJ_SIZE8_L16` selects 8-by-8 small sprites and
16-by-16 large sprites.

There are sixteen tiles per row in this sheet. A large sprite starting at tile
10 uses tiles 10, 11, 26, and 27. The transparent bottom row keeps the paddle
eight pixels tall.

Our VRAM plan uses word addresses: sprite tiles at `0x0000`, font at `0x3000`,
and text map at `0x6800`. Upload lengths, such as 1,024, are in **bytes**.
Keep these non-overlapping allocations unchanged for now.

`padsCurrent(0)` reads held buttons; `padsDown(0)` reads newly pressed buttons.
Left and Right together cancel out. `WaitForVBlank()` keeps input refreshing
and lets the library transfer its sprite/text buffers. Do not add an
`oamUpdate()` call or replace the frame wait with a delay.

```sh
make
```

**Checkpoint:** only a border and paddle appear. Left/Right moves the paddle,
and it stops at both walls. There is still no ball and nothing to launch. If the
paddle does not move, map controller 1 (the D-pad, B, and Start) in your
emulator's input settings and click its window so that it receives the input.

<img width="770" height="729" alt="image" src="https://github.com/user-attachments/assets/a87aa365-605b-4c9b-9f66-324ba34ac55d" />


---

## 3. Attach a ball and launch it

We will first implement the ball's **state**, without motion. That gives us a
small testable change before physics.

Edit `src\game.h`:

```diff
--- a/src/game.h
+++ b/src/game.h
@@
 #define FIELD_RIGHT 240

+#define BALL_SIZE 8
 #define PADDLE_WIDTH 48
@@
 #define PADDLE_SPEED 3

+enum GameState {
+    STATE_READY,
+    STATE_PLAYING
+};
+
 typedef struct Game {
     int paddle_x;
+    int ball_x;
+    int ball_y;
+    int vx;
+    int vy;
+    int state;
 } Game;
```

Add `attach_ball` immediately after the include in `src\game.c`, then update
the two existing functions:

```diff
--- a/src/game.c
+++ b/src/game.c
@@
 #include "game.h"

+static void attach_ball(Game *game)
+{
+    game->ball_x = game->paddle_x + (PADDLE_WIDTH - BALL_SIZE) / 2;
+    game->ball_y = PADDLE_Y - BALL_SIZE;
+    game->vx = 1;
+    game->vy = -3;
+}
+
 void game_reset(Game *game)
 {
     game->paddle_x = (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2;
+    game->state = STATE_READY;
+    attach_ball(game);
 }
@@
 void game_tick(Game *game, int movement, int action_pressed)
 {
-    (void)action_pressed;
     game->paddle_x += movement * PADDLE_SPEED;
@@
     if (game->paddle_x + PADDLE_WIDTH > FIELD_RIGHT)
         game->paddle_x = FIELD_RIGHT - PADDLE_WIDTH;
+
+    if (game->state == STATE_READY) {
+        attach_ball(game);
+        if (action_pressed)
+            game->state = STATE_PLAYING;
+        return;
+    }
 }
```

The centering offset is `(48 - 8) / 2`, or 20 pixels. `vx` and `vy` are
velocities; negative Y will mean upward movement. A new B/Start press launches,
not a continuously held button.

Add the ball's drawing and initial sprite definition in `src\main.c`. Both
insertions are shown next to the existing paddle code:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 static void draw_game(void)
 {
     int i;

+    oamSetXY(0, game.ball_x, game.ball_y);
     for (i = 0; i < 3; ++i)
         oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
@@
     bgSetDisable(2);
     oamClear(0, 0);
+    oamSet(0, game.ball_x, game.ball_y, 3, 0, 0, 12, 0);
+    oamSetEx(0, OBJ_SMALL, OBJ_SHOW);
     for (i = 0; i < 3; ++i) {
```

The ball uses tile 12, offset 0, and the small 8-by-8 sprite size.

```sh
make
```

**Checkpoint:** the ball follows the paddle. Press B or Start: it stays at its
launch position while the paddle can move away. This frozen launched ball is
intentional; we have not written its movement yet.

<img width="770" height="729" alt="image" src="https://github.com/user-attachments/assets/193de774-a885-4ece-ad7b-676b66ef90d7" />


---

## 4. Move the ball and bounce off walls

Add the vertical bounds and substep limit in `src\game.h`:

```diff
--- a/src/game.h
+++ b/src/game.h
@@
 #define FIELD_LEFT 16
 #define FIELD_RIGHT 240
+#define FIELD_TOP 32
+#define FIELD_BOTTOM 224

 #define BALL_SIZE 8
+#define BALL_SUBSTEPS 3
```

Instead of jumping several pixels and then checking overlap, test each
one-pixel **candidate position**. If it crosses a wall, reverse the velocity
and keep the old position. This will also prevent jumping through thin bricks
later. X and Y are handled separately, with X first at corners.

In `src\game.c`, insert `magnitude` before `attach_ball`, and the movement
helpers between `game_reset` and `game_tick`:

```diff
--- a/src/game.c
+++ b/src/game.c
@@
 #include "game.h"

+static int magnitude(int value)
+{
+    if (value < 0)
+        return -value;
+    return value;
+}
+
 static void attach_ball(Game *game)
@@
     attach_ball(game);
 }

+static void move_ball_x(Game *game)
+{
+    int next_x;
+
+    next_x = game->ball_x + (game->vx > 0 ? 1 : -1);
+    if (next_x < FIELD_LEFT || next_x + BALL_SIZE > FIELD_RIGHT) {
+        game->vx = -game->vx;
+    } else {
+        game->ball_x = next_x;
+    }
+}
+
+static void move_ball_y(Game *game)
+{
+    int next_y;
+
+    next_y = game->ball_y + (game->vy > 0 ? 1 : -1);
+    if (next_y < FIELD_TOP) {
+        game->vy = -game->vy;
+    } else {
+        game->ball_y = next_y;
+    }
+}
+
 void game_tick(Game *game, int movement, int action_pressed)
 {
+    int step;
+
     game->paddle_x += movement * PADDLE_SPEED;
@@
         if (action_pressed)
             game->state = STATE_PLAYING;
         return;
     }
+
+    for (step = 0; step < BALL_SUBSTEPS; ++step) {
+        if (step < magnitude(game->vx))
+            move_ball_x(game);
+        if (step < magnitude(game->vy))
+            move_ball_y(game);
+        if (game->ball_y >= FIELD_BOTTOM) {
+            game->state = STATE_READY;
+            attach_ball(game);
+            return;
+        }
+    }
 }
```

There is no bottom wall. For now, falling offscreen simply reattaches the ball
so you can launch again. Lives will replace that temporary behavior in step 7.

Keep every velocity component's magnitude at most `BALL_SUBSTEPS` (currently
3). The loop deliberately skips an axis when its velocity is zero.
One tick is one frame; these speeds are tuned for NTSC's 60 Hz. PAL's 50 Hz
will be slower in real time until you add timing compensation.

```sh
make
```

**Checkpoint:** first move the paddle to the right wall, then press B or Start.
The ball travels up and right, rebounds off the right wall, rebounds off the top
wall, falls offscreen, and reattaches. It ignores the paddle for now. No bricks
should exist yet.

The ball always launches toward the right for now. If you launch from the
centered paddle, the right-wall rebound happens down at paddle height, a split
second before the ball falls, and from the left wall there is no side rebound
at all. The left wall comes into play once step 5 lets the paddle send the ball
left.

<img width="770" height="729" alt="image" src="https://github.com/user-attachments/assets/c24b731f-ab87-412d-8835-2213afe23f93" />


---

## 5. Bounce off the paddle

A paddle contact must be downward, cross the paddle's top from above, and
overlap horizontally. Otherwise a missed ball could be caught from underneath.

Add `paddle_bounce` between `move_ball_x` and `move_ball_y` in `src\game.c`,
then extend the existing Y movement's `if`/`else` chain:

```diff
--- a/src/game.c
+++ b/src/game.c
@@
         game->ball_x = next_x;
     }
 }

+static void paddle_bounce(Game *game)
+{
+    int hit_position;
+
+    hit_position = game->ball_x + BALL_SIZE / 2 - game->paddle_x;
+    if (hit_position < PADDLE_WIDTH / 3) {
+        game->vx = -2;
+        game->vy = -2;
+    } else if (hit_position >= 2 * PADDLE_WIDTH / 3) {
+        game->vx = 2;
+        game->vy = -2;
+    } else {
+        game->vx = game->vx < 0 ? -1 : 1;
+        game->vy = -3;
+    }
+}
+
 static void move_ball_y(Game *game)
@@
     next_y = game->ball_y + (game->vy > 0 ? 1 : -1);
     if (next_y < FIELD_TOP) {
         game->vy = -game->vy;
+    } else if (game->vy > 0 &&
+               game->ball_y + BALL_SIZE <= PADDLE_Y &&
+               next_y + BALL_SIZE > PADDLE_Y &&
+               game->ball_x + BALL_SIZE > game->paddle_x &&
+               game->ball_x < game->paddle_x + PADDLE_WIDTH) {
+        game->ball_y = PADDLE_Y - BALL_SIZE;
+        paddle_bounce(game);
     } else {
         game->ball_y = next_y;
     }
 }
```

We place the ball just above the paddle on contact. Its center determines the
rebound zone:

| Paddle zone | `vx` | `vy` |
|---|---:|---:|
| Left third | -2 | -2 |
| Middle third | -1 or +1, keeping its horizontal direction | -3 |
| Right third | +2 | -2 |

This approximates shot angles without trigonometry. It does not preserve
identical mathematical speed in every zone.

```sh
make
```

**Checkpoint:** catch a descending ball. It rebounds upward; catches near the
left/right ends send it in those directions. Deliberately miss and verify
that it still reattaches for another launch.

---

## 6. Add a wall of destructible bricks

### Store the grid

Extend `src\game.h` with brick dimensions and data:

```diff
--- a/src/game.h
+++ b/src/game.h
@@
 #define PADDLE_SPEED 3

+#define BRICK_COLS 10
+#define BRICK_ROWS 5
+#define BRICK_COUNT (BRICK_COLS * BRICK_ROWS)
+#define BRICK_X 30
+#define BRICK_Y 48
+#define BRICK_WIDTH 16
+#define BRICK_HEIGHT 8
+#define BRICK_PITCH_X 20
+#define BRICK_PITCH_Y 16
+
 enum GameState {
@@
     int vx;
     int vy;
+    unsigned int score;
+    int bricks_left;
     int state;
+    unsigned char bricks[BRICK_COUNT];
 } Game;
```

`bricks[index]` is 1 when alive and 0 when destroyed. The pitch includes the gap:
`index = row * BRICK_COLS + col`, X is `BRICK_X + col * BRICK_PITCH_X`, and
Y is `BRICK_Y + row * BRICK_PITCH_Y`.

Initialize all bricks and score in `src\game.c`:

```diff
--- a/src/game.c
+++ b/src/game.c
@@
 void game_reset(Game *game)
 {
+    int i;
+
     game->paddle_x = (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2;
+    game->score = 0;
+    game->bricks_left = BRICK_COUNT;
     game->state = STATE_READY;
+    for (i = 0; i < BRICK_COUNT; ++i)
+        game->bricks[i] = 1;
     attach_ball(game);
 }
```

### Test candidate rectangles against live bricks

Two rectangles overlap only when all four inequalities hold:

```c
x + BALL_SIZE > brick_x &&
x < brick_x + BRICK_WIDTH &&
y + BALL_SIZE > brick_y &&
y < brick_y + BRICK_HEIGHT
```

Merely touching an edge is not overlap. We reject non-overlapping rows first
to avoid examining every brick when the ball is far from the wall.

Insert this complete helper **between `game_reset` and `move_ball_x`** in
`src\game.c`. Then add its calls to the existing X/Y collision chains:

```diff
--- a/src/game.c
+++ b/src/game.c
@@
     attach_ball(game);
 }

+static int hit_brick(Game *game, int x, int y)
+{
+    int row;
+    int col;
+    int brick_x;
+    int brick_y;
+    int index;
+
+    if (y + BALL_SIZE <= BRICK_Y ||
+        y >= BRICK_Y + (BRICK_ROWS - 1) * BRICK_PITCH_Y + BRICK_HEIGHT)
+        return 0;
+
+    for (row = 0; row < BRICK_ROWS; ++row) {
+        brick_y = BRICK_Y + row * BRICK_PITCH_Y;
+        if (y + BALL_SIZE <= brick_y || y >= brick_y + BRICK_HEIGHT)
+            continue;
+
+        for (col = 0; col < BRICK_COLS; ++col) {
+            index = row * BRICK_COLS + col;
+            if (!game->bricks[index])
+                continue;
+            brick_x = BRICK_X + col * BRICK_PITCH_X;
+            if (x + BALL_SIZE > brick_x && x < brick_x + BRICK_WIDTH) {
+                game->bricks[index] = 0;
+                game->score += 10;
+                --game->bricks_left;
+                return 1;
+            }
+        }
+    }
+    return 0;
+}
+
 static void move_ball_x(Game *game)
@@
     if (next_x < FIELD_LEFT || next_x + BALL_SIZE > FIELD_RIGHT) {
         game->vx = -game->vx;
+    } else if (hit_brick(game, next_x, game->ball_y)) {
+        game->vx = -game->vx;
     } else {
         game->ball_x = next_x;
@@
         game->ball_y = PADDLE_Y - BALL_SIZE;
         paddle_bounce(game);
+    } else if (hit_brick(game, game->ball_x, next_y)) {
+        game->vy = -game->vy;
     } else {
         game->ball_y = next_y;
```

Each hit removes one live brick, adds ten points, and reverses the tested axis.
The candidate position is rejected, so the ball is not left inside the brick.

### Draw only changed bricks

Add the cache and renderer **above `draw_game`** in `src\main.c`, then call
it after drawing the paddle. The initialization edit is next to `oamClear`:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 Game game;

+static unsigned char shown_alive[BRICK_COUNT];
+static int shown_bricks = -1;
+
+static void draw_bricks(void)
+{
+    int index;
+    int id;
+
+    if (shown_bricks == game.bricks_left)
+        return;
+
+    for (index = 0; index < BRICK_COUNT; ++index) {
+        if (shown_alive[index] == game.bricks[index])
+            continue;
+
+        id = (index + 4) * 4;
+        if (game.bricks[index]) {
+            oamSet(id, BRICK_X + (index % BRICK_COLS) * BRICK_PITCH_X,
+                   BRICK_Y + (index / BRICK_COLS) * BRICK_PITCH_Y,
+                   3, 0, 0, (index / BRICK_COLS) * 2, 0);
+            oamSetEx(id, OBJ_LARGE, OBJ_SHOW);
+        } else {
+            oamSetVisible(id, OBJ_HIDE);
+        }
+        shown_alive[index] = game.bricks[index];
+    }
+    shown_bricks = game.bricks_left;
+}
+
 static void draw_game(void)
@@
     for (i = 0; i < 3; ++i)
         oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
+    draw_bricks();
 }
@@
     bgSetDisable(2);
     oamClear(0, 0);
+    for (i = 0; i < BRICK_COUNT; ++i)
+        shown_alive[i] = 0;
+
     oamSet(0, game.ball_x, game.ball_y, 3, 0, 0, 12, 0);
```

Brick rows start at tiles 0, 2, 4, 6, and 8. A 16-by-16 sprite at tile 0 uses
tiles 0, 1, 16, and 17; it does not use four consecutive tile numbers.

The cache starts at zero because all sprites are initially hidden. Defining a
restored brick with `oamSet` also restores its position: hiding a sprite moves
it offscreen, so simply showing it again is insufficient.

The scene uses 54 of the SNES's 128 sprite entries. On a brick scanline there
are at most eleven sprites and 21 eight-pixel tile slivers, below the limits
of 32 sprites and 34 slivers. Transparent lower halves still count toward
hardware limits; the 16-pixel vertical pitch keeps rows from overlapping.

```sh
make
```

**Checkpoint:** fifty bricks appear in five colored rows. Hits remove bricks
and rebound the ball; misses preserve the damage. Score is being accumulated,
but is not displayed yet. Clearing the wall does not end play yet either.

<img width="770" height="729" alt="image" src="https://github.com/user-attachments/assets/541d0559-2166-438c-bbf5-9db34542bdb3" />


---

## 7. Add lives, winning, and restarting

Add the terminal states and life count in `src\game.h`:

```diff
--- a/src/game.h
+++ b/src/game.h
@@
 enum GameState {
     STATE_READY,
-    STATE_PLAYING
+    STATE_PLAYING,
+    STATE_GAME_OVER,
+    STATE_WON
 };
@@
     int vy;
     unsigned int score;
+    int lives;
     int bricks_left;
```

In `src\game.c`, initialize lives, detect the last brick, and add `lose_life`
**between `move_ball_y` and `game_tick`**. Finally replace the temporary
offscreen reset and add state checks inside the substep loop:

```diff
--- a/src/game.c
+++ b/src/game.c
@@
     game->paddle_x = (FIELD_LEFT + FIELD_RIGHT - PADDLE_WIDTH) / 2;
     game->score = 0;
+    game->lives = 3;
     game->bricks_left = BRICK_COUNT;
@@
                 game->bricks[index] = 0;
                 game->score += 10;
                 --game->bricks_left;
+                if (game->bricks_left == 0)
+                    game->state = STATE_WON;
                 return 1;
@@
         game->ball_y = next_y;
     }
 }

+static void lose_life(Game *game)
+{
+    --game->lives;
+    if (game->lives == 0) {
+        game->state = STATE_GAME_OVER;
+    } else {
+        game->state = STATE_READY;
+        attach_ball(game);
+    }
+}
+
 void game_tick(Game *game, int movement, int action_pressed)
 {
     int step;

+    if (game->state == STATE_GAME_OVER || game->state == STATE_WON) {
+        if (action_pressed)
+            game_reset(game);
+        return;
+    }
+
     game->paddle_x += movement * PADDLE_SPEED;
@@
         return;
     }

+    /* Check each pixel, rather than jumping across a thin brick. */
     for (step = 0; step < BALL_SUBSTEPS; ++step) {
         if (step < magnitude(game->vx))
             move_ball_x(game);
+        if (game->state != STATE_PLAYING)
+            return;
         if (step < magnitude(game->vy))
             move_ball_y(game);
+        if (game->state != STATE_PLAYING)
+            return;
         if (game->ball_y >= FIELD_BOTTOM) {
-            game->state = STATE_READY;
-            attach_ball(game);
+            lose_life(game);
             return;
         }
     }
```

Those early returns matter: once the last brick is destroyed, remaining
substeps must not keep moving or changing the score.

```text
READY --new B/Start press--> PLAYING
PLAYING --miss with lives left--> READY
PLAYING --last miss--> GAME_OVER
PLAYING --last brick--> WON
GAME_OVER or WON --new B/Start press--> fresh READY
```

A missed ball costs one life and preserves score/bricks. A new game restores
everything, including the centered paddle. Restarting returns immediately,
so a **separate** button press is needed to launch again.

```sh
make
```

**Checkpoint:** miss three times, relaunching between misses. After the third,
paddle movement stops. Release and press B/Start: all bricks return and the
ball reattaches. Release and press again to launch. There are no status messages
yet; we will make these states visible next.

---

## 8. Draw the score, lives, and prompts

Text should update when values change, not on every pixel movement. The maximum
score is 50 times 10, or 500, so a three-digit string is enough.

In `src\main.c`, add the text buffers and caches next to the brick cache:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
 static unsigned char shown_alive[BRICK_COUNT];
+static char score_text[] = "000";
+static char lives_text[] = "3";
 static int shown_bricks = -1;
+static int shown_score = -1;
+static int shown_lives = -1;
+static int shown_state = -1;

 static void draw_bricks(void)
```

Now insert `draw_text` **between `draw_bricks` and `draw_game`**, call it at
the end of `draw_game`, and add the HUD labels above the border:

```diff
--- a/src/main.c
+++ b/src/main.c
@@
     shown_bricks = game.bricks_left;
 }

+static void draw_text(void)
+{
+    if (shown_score != game.score) {
+        score_text[0] = '0' + game.score / 100;
+        score_text[1] = '0' + (game.score / 10) % 10;
+        score_text[2] = '0' + game.score % 10;
+        consoleDrawText(8, 1, score_text);
+        shown_score = game.score;
+    }
+    if (shown_lives != game.lives) {
+        lives_text[0] = '0' + game.lives;
+        consoleDrawText(20, 1, lives_text);
+        shown_lives = game.lives;
+    }
+
+    if (shown_state == game.state)
+        return;
+
+    consoleDrawText(3, 18, "                          ");
+    consoleDrawText(3, 20, "                          ");
+    if (game.state == STATE_READY) {
+        consoleDrawText(7, 18, "B / START TO LAUNCH");
+        consoleDrawText(6, 20, "LEFT / RIGHT TO MOVE");
+    } else if (game.state == STATE_GAME_OVER) {
+        consoleDrawText(11, 18, "GAME OVER");
+        consoleDrawText(7, 20, "B / START: NEW GAME");
+    } else if (game.state == STATE_WON) {
+        consoleDrawText(12, 18, "YOU WIN!");
+        consoleDrawText(7, 20, "B / START: NEW GAME");
+    }
+    shown_state = game.state;
+}
+
 static void draw_game(void)
@@
         oamSetXY((i + 1) * 4, game.paddle_x + i * 16, PADDLE_Y);
     draw_bricks();
+    draw_text();
 }
@@
         oamSetEx((i + 1) * 4, OBJ_LARGE, OBJ_SHOW);
     }

+    consoleDrawText(2, 1, "SCORE       LIVES");
     consoleDrawText(1, 3, "##############################");
```

The string initializers already include the terminating zero byte. Writing
digits directly avoids printf-style formatting in the game loop. Clearing
messages with spaces prevents remnants when a new message is shorter.

The existing `draw_game()` call in `init_video` now draws the initial HUD
before turning the screen on. The main loop already calls it each frame;
no other call sites need to change.

```sh
make
```

**Checkpoint:** the ready screen shows score `000`, lives `3`, and launch/move
prompts. Launching clears the prompts. Hits add ten points; misses decrease
lives. Game over displays a restart prompt.

![The game-over screen you have now added](screenshots/game-over.png)

---

## 9. Test the complete game

You now have this project in **your SDK installation**, built incrementally:

```text
pvsneslib\
    devkitsnes\
    pvsneslib\
    snes-examples\
        breakout\
            Makefile
            sprites.png
            THIRD_PARTY_NOTICES.txt
            src\
                main.c
                game.h
                game.c
                data.asm
```

Generated graphics, header, object files, and ROM will also be present. For a
clean rebuild from your `breakout` directory:

```sh
make clean
make
```

Reopen the generated ROM, or reload it in the emulator. Test in this order:

1. Before launching, move to both walls: paddle clamps; ball stays centered on
   the paddle.
2. Press B/Start: the ball launches; holding it does not repeatedly launch.
3. Watch left, right, and top wall rebounds.
4. Hit a brick: it disappears, adds ten points, and cannot score again.
5. Catch the ball on each paddle third: verify the outgoing direction.
6. Miss: exactly one life is lost; previous score and brick damage remain.
7. Lose the last life: movement stops and `GAME OVER` appears.
8. Restart: score `000`, lives `3`, all fifty bricks, and a waiting ball.
9. Clear the wall: score `500`, `YOU WIN!`, and no further movement.
10. Restart after winning, then use a separate press to launch.

Silence is expected. There is no pause button in this version.

Only **now**, if you want to compare your work, open the finished reference
[game.h](src/game.h), [game.c](src/game.c), [main.c](src/main.c), and
[data.asm](src/data.asm). Your learner Makefile intentionally has a different
SDK default from the repository's [reference Makefile](Makefile).

Emulator testing is not proof of correct behavior on a physical SNES. Test
accurate emulators and real hardware when available before publishing a game.

## Troubleshooting

| Symptom | Check |
|---|---|
| SDK not found | Your project belongs at `<SDK root>\snes-examples\breakout`. The SDK root (the folder that directly contains `devkitsnes`) must also contain `pvsneslib` and `snes-examples`. If `PVSNESLIB_HOME` is set in your environment, it overrides the Makefile default and must name that folder in `/c/...` form. |
| Build cannot find shell utilities | Use MSYS2 UCRT64 on Windows and install GNU Make. |
| `No targets specified and no makefile found` | Run `make` from your project directory; `ls` should list `Makefile`. Check that your editor did not save it as `Makefile.txt`. |
| Many `Label ... defined more than once` or `DISCARD` lines | Harmless linker messages. The build succeeded if it ends with `Build finished successfully !`. |
| `missing separator` | Makefile command recipes need tabs; do not paste diff markers. |
| A malformed library path during linking | Keep paths free of spaces. Use an absolute MSYS2 SDK path, such as `/c/snesdev/pvsneslib`, not Windows backslashes in `PVSNESLIB_HOME`. |
| Undefined function/field | Apply the steps in order; check the file and neighboring code named in the diff. |
| Old behavior after rebuilding | Open the ROM from your new learner project, not the reference repository or Hello World directory. |
| No controller response | Configure controller 1, focus the emulator, and keep `WaitForVBlank` in the loop. |
| Ball freezes after launching in step 3 | Expected until step 4 adds movement. |
| Ball passes through paddle in step 4 | Expected until step 5 adds paddle collisions. |
| Bricks disappear but no score is visible in step 6 | Expected until step 8 adds the HUD. |
| Scrambled artwork | Preserve the indexed sheet and tile order; use the specified converter options. |
| Unexpected sprite lower half | Tiles 16 through 31 should be transparent. |
| Restored bricks stay offscreen | Restore positions with `oamSet`, not visibility alone. |
| Parts of a larger wall disappear | Check scanline sprite/sliver limits, including transparent sprite areas. |
| Corrupt text/sprites after moving graphics | Check for overlapping VRAM allocations. |
| Slower on PAL | Speeds are frame-based and NTSC-tuned; compensation is not implemented. |

If you change the sheet dimensions or palette size, change the upload lengths
in your `init_video` too. Do not upload 1,024 bytes from a smaller asset.

## Running the gameplay tests

After step 7, the rules can be tested without SNES graphics. Copy the optional
[test_game.c](tests/test_game.c) from your copy of this repository into a new
`tests` directory in your learner project (not `src`, where the ROM build would
discover it). Run these commands from your learner project's root. This part is
optional and does not change your ROM.

On Linux/macOS with a native C compiler:

```sh
mkdir -p build
cc -std=c89 -pedantic -Wall -Wextra -Werror -Isrc \
  tests/test_game.c src/game.c -o build/test_game
./build/test_game
```

On Windows you need a native C compiler, which neither the PVSnesLib toolchain
nor a default MSYS2 install provides. Pick one of two options.

**Option A: GCC from MSYS2** (the smaller download). In an MSYS2 UCRT64
terminal, install it once:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc
```

Then, in the same terminal, change to your learner project and run the
Linux/macOS commands above unchanged.

**Option B: Microsoft's compiler.** Install the **Build Tools for Visual
Studio** (or Visual Studio with the "Desktop development with C++" workload).
Then open an **x64 Native Tools Command Prompt for Visual Studio**, change to
your learner project, and run:

```bat
if not exist build mkdir build
cl /nologo /W4 /WX /Isrc /Fobuild\ /Febuild\test_game.exe tests\test_game.c src\game.c
build\test_game.exe
```

Expected output:

```text
PASS: 8 gameplay test groups
```

Microsoft's compiler also prints the names of the files it compiles and
`Generating Code...` before that line.

The tests cover reset/launch, paddle limits, wall/brick collisions, gaps and
corners, rebound zones, lives, scoring, winning, and restarting. They do not
replace the emulator checkpoints for input, graphics, or timing.

The repository's [GitHub Actions workflow](.github/workflows/test.yml) runs
these tests against the finished reference on pushes and pull requests; it
does not build a ROM.

## Extend one thing at a time

Once all checkpoints pass, try multiple level arrays, a paused state, better
artwork, sound via the SDK's sound examples, background-based bricks, or PAL
timing compensation. Make one change, rebuild, and repeat relevant tests.

The optional [make_sprites.py](tools/make_sprites.py) regenerates the original
sheet using Python's standard library; it is not needed for a normal build.

## Licensing

[THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt) preserves the PVSnesLib
notice for the library and built-in font linked into the ROM. It is not a
license for the original tutorial, source, or artwork. No project-wide license
has been selected for those files.

## Official references

- [PVSnesLib 4.6.0 release][release]
- [Installation guide][installation]
- [Compiling Hello World][hello]
- [ROM header configuration][header]
- [Sprite and OAM API (`sprite.h`)][sprites]
- [Background API (`background.h`)][backgrounds]
- [Text console API (`textfont.h`)][text]
- [Controller input API (`input.h`)][input]
- [VBlank and interrupts (`interrupt.h`)][interrupts]
- [gfx4snes options, pinned to 4.6.0][gfx]
- [SimpleSprite example, pinned to 4.6.0][sprite-example]

The header links are pinned to 4.6.0. The same headers are installed in
`pvsneslib\include\snes` inside your SDK root. The generated
[online API documentation](https://alekmaul.github.io/pvsneslib/api/)
describes the latest release, which may differ from 4.6.0.

[release]: https://github.com/alekmaul/pvsneslib/releases/tag/4.6.0
[windows-zip]: https://github.com/alekmaul/pvsneslib/releases/download/4.6.0/pvsneslib_460_64b_windows_release.zip
[linux-zip]: https://github.com/alekmaul/pvsneslib/releases/download/4.6.0/pvsneslib_460_64b_linux_release.zip
[macos-zip]: https://github.com/alekmaul/pvsneslib/releases/download/4.6.0/pvsneslib_460_64b_darwin_release.zip
[installation]: https://github.com/alekmaul/pvsneslib/wiki/Installation
[hello]: https://github.com/alekmaul/pvsneslib/wiki/Compiling-helloworld-example
[header]: https://github.com/alekmaul/pvsneslib/wiki/SNES-ROM-Header
[sprites]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/sprite.h
[backgrounds]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/background.h
[text]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/textfont.h
[input]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/input.h
[interrupts]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/pvsneslib/include/snes/interrupt.h
[gfx]: https://github.com/alekmaul/pvsneslib/blob/4.6.0/tools/gfx4snes/readme.md
[sprite-example]: https://github.com/alekmaul/pvsneslib/tree/4.6.0/snes-examples/graphics/Sprites/SimpleSprite
