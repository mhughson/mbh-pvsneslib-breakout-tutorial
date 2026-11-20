"""Replay Part 4's documented edits and optionally build every checkpoint."""

import argparse
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ("Makefile", "src/game.h", "src/game.c", "src/main.c", "src/data.asm")


def apply_diff(project, block):
    for patch in re.split(r"(?=^--- a/)", block, flags=re.MULTILINE):
        if not patch.strip():
            continue
        lines = patch.splitlines(keepends=True)
        name = lines[0].removeprefix("--- a/").strip()
        if name not in SOURCES or lines[1].strip() != "+++ b/" + name:
            raise ValueError(f"Unexpected patch path: {name}")
        path = project / name
        original = path.read_text().splitlines(keepends=True)
        result = []
        cursor = 0
        index = 2
        while index < len(lines):
            header = re.fullmatch(
                r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@ [^\s\n][^\n]*\n?",
                lines[index],
            )
            if not header:
                raise ValueError(
                    f"Invalid hunk or missing function/placement label in {name}: "
                    f"{lines[index]!r}"
                )
            start = int(header[1]) - 1
            if start < cursor:
                raise ValueError(f"Overlapping hunk in {name}")
            result.extend(original[cursor:start])
            cursor = start
            index += 1
            removed = added = 0
            while index < len(lines) and not lines[index].startswith("@@ "):
                line = lines[index]
                if line[0] in " -":
                    if cursor >= len(original) or original[cursor] != line[1:]:
                        raise ValueError(f"Context mismatch in {name}:{cursor + 1}")
                    cursor += 1
                    removed += 1
                if line[0] in " +":
                    result.append(line[1:])
                    added += 1
                if line[0] not in " +-":
                    raise ValueError(f"Unexpected patch line in {name}: {line!r}")
                index += 1
            if removed != int(header[2] or 1) or added != int(header[4] or 1):
                raise ValueError(f"Incorrect hunk length in {name}")
        result.extend(original[cursor:])
        path.write_text("".join(result), newline="\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--sdk", help="PVSnesLib root; needed only for ROM builds")
    args = parser.parse_args()
    if args.build and not args.sdk:
        parser.error("--build requires --sdk")
    output = args.output.resolve()
    if output == ROOT or ROOT.is_relative_to(output):
        parser.error("--output must not be the repository or one of its parents")
    document = (ROOT / "README-part4.md").read_text()
    sections = re.split(r"^## ([1-8])\. .+$", document, flags=re.MULTILINE)
    if sections[1::2] != list("12345678"):
        raise ValueError("Expected eight tutorial sections")
    previous = None
    for stage, section in zip(sections[1::2], sections[2::2]):
        if stage == "8":
            break
        project = output / f"section{stage}"
        for name in SOURCES:
            target = project / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile((previous or ROOT / "part3") / name, target)
        for image in (ROOT / "part4").glob("*.png"):
            shutil.copyfile(image, project / image.name)
        blocks = re.findall(r"```diff\n(.*?)```", section, flags=re.DOTALL)
        if not blocks:
            raise ValueError(f"Section {stage} has no edits")
        for block in blocks:
            apply_diff(project, block)
        if args.build:
            sdk = re.sub(
                r"^([A-Za-z]):[/\\]",
                lambda match: "/" + match[1].lower() + "/",
                args.sdk,
            ).replace("\\", "/")
            with (project / "build.log").open("w") as log:
                completed = subprocess.run(
                    ["make", f"PVSNESLIB_HOME={sdk}"], cwd=project,
                    stdout=log, stderr=subprocess.STDOUT, check=False,
                )
            if completed.returncode:
                raise RuntimeError(f"Section {stage} build failed: {project / 'build.log'}")
        print(f"PASS: section {stage}" + (" ROM built" if args.build else " edits replayed"))
        previous = project
    for name in SOURCES:
        if (previous / name).read_text() != (ROOT / "part4" / name).read_text():
            raise ValueError(f"Final tutorial result differs from part4/{name}")
    print("PASS: tutorial result matches the finished reference")


if __name__ == "__main__":
    main()
