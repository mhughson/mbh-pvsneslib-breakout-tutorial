# Contributing

The four tutorial parts are designed to be completed in sequence. A change to
a lesson should leave the reader with a buildable project at every checkpoint,
not just at the end of the part.

## Repository layout

| Location | Purpose |
|---|---|
| [README.md](README.md) | Series introduction and Part 1: The Core Mechanics |
| [README-part2.md](README-part2.md) | Backgrounds, dynamic sprites, and sound |
| [README-part3.md](README-part3.md) | Screens, effects, saving, and mouse input |
| [README-part4.md](README-part4.md) | Scrolling, levels, and power-ups |
| [README-whats-next.md](README-whats-next.md) | Suggested projects after completing the series |
| [src](src), [tests](tests), [Makefile](Makefile) | Finished Part 1 reference |
| [part2](part2), [part3](part3), [part4](part4) | Finished references and source artwork for later parts |
| [screenshots](screenshots) | Native-resolution emulator frames |
| [videos](videos) | Optional music and sound demonstrations |
| [tools](tools) | Contributor validation and artwork generation |
| [vendor/pvsneslib](vendor/pvsneslib) | Optional local SDK for reference builds; SDK files are not tracked |

Part 1 lives at the repository root so the repository's main README is also
the starting lesson. Parts 2-4 have self-contained reference projects. Learners
work in their own SDK example directory; they do not need this repository's
tooling to build the game.

Keep generated graphics, audio banks, object files, symbols, editor files, and
test output out of Git. The downloadable ROMs are intentional build artifacts.
Source PNG files remain beside the Makefile that converts them.

The browser players use generated ROM copies with distinct basenames.
EmulatorJS 4.2.3 caches ROMs by basename and byte length, so using
`breakout.sfc` for every chapter would load the wrong same-sized ROM.
Keep the chapter-specific names even though the original download URLs remain available.
Generated ROM hashes also version the player URLs, so same-sized rebuilds
refresh cached graphics while keeping the save-file basenames unchanged.

The development server watches the canonical Markdown pages and regenerates
the site copies after edits. Keep the live preview updated while reviewing
reader feedback; do not edit generated `site/content` files.

## Writing lessons

Each code edit should have:

1. A short introduction stating the goal.
2. A visible filename and an unambiguous **create**, **add**, **replace**, or
   **remove** instruction.
3. A focused, C-highlighted comparison of the old and new code.
4. An explanation of the important behavior after the code.
5. A build-and-run checkpoint with an image showing that stage.

Keep each comparison within one function or logical edit. Include enough
unchanged context to locate the change: show 2-4 unchanged lines above and
below each change, stopping at the start or end of the function. Identify the
file and function visibly; do not rely on editor line numbers. Avoid replacing
whole functions for small changes. File-scope additions need a named insertion point.
Machine-readable edit comments support replay validation but must not be the
only placement instructions visible to the reader.

When moving to another file, add a horizontal divider and an **Open `filename`**
heading. Group edits to the same file beneath that heading, and repeat it when
returning to a previously edited file. Place the divider and heading before
the edit instruction, never between the instruction and its code.

Use `c` code fences with `// [!code --]` on removed lines and
`// [!code ++]` on added lines. The site renders these as **Before** and
**After** panes, preserving syntax colors and applying only background colors
to changed lines. Its copy button copies the After code without annotations.
Makefile and assembly examples use their own comment syntax. The GitHub
Markdown remains the source of truth; do not separately maintain site lessons.

All C examples, reference code, and tests use braces for every branch and loop,
with each opening and closing brace on a separate line. Use the repository's
`.clang-format` configuration when formatting C.

This presentation follows the filename-labelled examples and incremental
verification used in the [Django introductory tutorial](https://docs.djangoproject.com/en/5.2/intro/tutorial01/)
and the example-first explanations in [MDN's Local Library tutorial](https://developer.mozilla.org/en-US/docs/Learn_web_development/Extensions/Server-side/Express_Nodejs/Tutorial_local_library_website).

Write for a reader encountering the material for the first time. Keep
discussion of repository maintenance and validation here rather than in the
lessons. Explain technical constraints where they matter instead of referring
to prior conversations or troubleshooting sessions.

## Gameplay tests

Run from the repository root with a native C compiler:

```sh
mkdir -p build
cc -std=c89 -pedantic -Wall -Wextra -Werror -Isrc tests/test_game.c src/game.c -o build/test_part1
cc -std=c89 -pedantic -Wall -Wextra -Werror -Ipart2/src part2/tests/test_game.c part2/src/game.c -o build/test_part2
cc -std=c89 -pedantic -Wall -Wextra -Werror -Ipart3/src part3/tests/test_game.c part3/src/game.c -o build/test_part3
cc -std=c89 -pedantic -Wall -Wextra -Werror -Ipart4/src part4/tests/test_game.c part4/src/game.c -o build/test_part4
./build/test_part1
./build/test_part2
./build/test_part3
./build/test_part4
```

On Windows, use MSYS2 UCRT64 and `gcc` in place of `cc`. The expected counts
are 8, 8, 9, and 11 passing gameplay groups. These tests exercise game rules,
not the PPU, audio processor, mouse hardware, or save persistence.

Replay the actual lesson edits from an empty project and compare each finished
part with its reference:

```sh
python -m unittest discover -s tools/tests
python tools/verify_tutorials.py --output build/tutorial-replay --gcc cc
```

Choose a fresh output directory for each run; existing checkpoints are never
overwritten. Add `--build --sdk /path/to/pvsneslib` to build every checkpoint
ROM. On Windows, pass the SDK's Windows path and use MSYS2's `make` and `gcc`.
The verifier checks 30 implementation checkpoints and their exact final
sources, allowing only the documented SDK-location and reference-only
Makefile comments.
Part 1 section 1 is an external SDK Hello World readiness check; the learner
project starts in section 2.

## Reference builds

Use PVSnesLib 4.6.0. Either install the SDK as described in
[vendor/pvsneslib/README.md](vendor/pvsneslib/README.md), or pass its root:

```sh
make PVSNESLIB_HOME=/path/to/pvsneslib
make -C part2 PVSNESLIB_HOME=/path/to/pvsneslib
make -C part3 PVSNESLIB_HOME=/path/to/pvsneslib
make -C part4 PVSNESLIB_HOME=/path/to/pvsneslib
```

For Windows MSYS2, use an SDK path such as `/c/snesdev/pvsneslib`.
Rebuild downloadable ROMs after changing their sources or artwork.

Part 3's mouse lesson and both later reference entry points use a C
`init_mouse` initializer to avoid PVSnesLib 4.6.0's incorrect `RTS` return
in `initMouse`. Keep the SDK untouched. Recheck the upstream routine before
removing this version-specific workaround when changing SDK versions.

## Tutorial site

The site uses VitePress. Install Node.js 22 or newer, then run:

```sh
npm ci
npm run docs:test
npm run docs:build
npm run docs:preview
```

On Windows PowerShell, use `npm.cmd` if script execution policy prevents
running `npm.ps1`. Open the preview URL with
`/mbh-pvsneslib-breakout-tutorial/` appended. `npm run docs:dev` starts the
development server after preparing the current lessons.

The ROM panels progressively enhance into click-to-load EmulatorJS players.
Their normal HTML download links still work on GitHub. The theme permits one
player at a time and closes it on navigation. Each player runs in its own
iframe; the core uses no threads, so GitHub Pages needs no special headers.

Site preparation downloads the pinned EmulatorJS 4.2.3 runtime and only the
Snes9x cores, checks their SHA-256 hashes, and caches them under
`build/emulator-cache`. The first build needs internet access; later builds
reuse the verified cache. All browser runtime assets are served by the site,
not a CDN. Keep upstream licenses and source links alongside the player when
updating it. EmulatorJS is GPL-3.0; Snes9x has a non-commercial license.

`site/prepare.mjs` copies canonical Markdown and public media into ignored
site directories; the development server regenerates them after canonical
Markdown edits. Run preparation or a build after replacing artwork or ROMs.
Generated HTML goes to `build/site`. Source-file links point to GitHub;
screenshots, videos, and downloadable ROMs are served by the site itself.
The Vite override pins a patched, plugin-compatible release; validate both
the site build and preview when updating dependencies.

To publish, select **GitHub Actions** as the repository's Pages source and
run the Pages workflow. Local builds do not publish anything.

## Artwork and media

The optional artwork generators use only Python's standard library:

```sh
python tools/make_sprites.py
python tools/make_part2_art.py
python tools/make_part4_capsule.py
```

Keep checkpoint screenshots at **256 x 224**, without window borders, menus,
debug overlays, or rescaling. Use the filename pattern
`screenshots/partN-sectionM-description.png`. Capture the corresponding
checkpoint ROM, not the completed ROM with later features disabled.

```sh
python tools/verify_media.py --checkpoints
```

The media check verifies local links and headings, PNG checksums and compressed
pixel data, image dimensions, and a screenshot for each numbered checkpoint.
Visual comparison and emulator checks are still needed to establish that the
picture matches the lesson. Videos supplement the screenshots; they are not
required for completing a checkpoint.

Use a separate emulator profile for automation. Tests may change controller
settings and save RAM. Integration fixtures can place a ball or capsule in
WRAM to reproduce a specific collision or screen; record such use when
reporting validation. A fixture-assisted capture is not evidence of an
unassisted playthrough. Accurate emulators and physical hardware remain
important before publishing a finished game.

### Emulator checkpoints

After building replay checkpoints, run Mesen 2 with a separate profile and the
appropriate Lua script. Both scripts stop with a nonzero exit code on failure.
Enable Lua file access with
`--debug.scriptWindow.allowIoOsAccess=true` and use
`--testRunner --enableStdout --doNotSaveSettings`.

For Parts 1-3, `tools/verify_checkpoints.lua` reads:

| Environment variable | Value |
|---|---|
| `TUTORIAL_PART` | Part number, 1-3 |
| `TUTORIAL_SECTION` | Numbered lesson section |
| `TUTORIAL_OUTPUT` | Existing directory for screenshots and logs |
| `TUTORIAL_SYMBOLS` | Checkpoint's `breakout.sym` |
| `TUTORIAL_HEADER` | Checkpoint's `src/game.h` |

Set port 1 to `SnesController`; use `SnesMouse` on port 2 only for Part 3,
section 9. 
That scenario tests mouse start, movement, launch, pause, and resume.
Its mouse injection targets Mesen 2.1.1, whose Lua `setInput` reads the device
port from the third argument; the duplicated port index is intentional.
Part 1's project checks start at section 2; its installation screenshot comes
from the SDK's Hello World ROM.

For Part 4, `tools/verify_part4.lua` reads `PART4_STAGE`, `PART4_OUTPUT`,
and `PART4_SYMBOLS`. Stages 1-7 use their matching checkpoint ROMs; stage 8
uses section 7 to verify all three levels and save a 1460-point record.
Run stage 9 against the same ROM and profile to check SRAM reload. These
checks include scrolling, entrance gating, brick VRAM updates, fractional
angles, capsules, the wide-paddle timer, pause, campaign transitions, and
record preservation.

## Third-party material

[THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt) retains the PVSnesLib notice
for the library and built-in font linked into the reference ROMs. The music
and effects are taken from the SDK's sound example; check their terms before
redistributing them. No project-wide license has been selected for the
original tutorial, source code, or artwork.
