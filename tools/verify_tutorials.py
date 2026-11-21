"""Replay the tutorials' copyable edits from an empty project.

Only SDK-location/error text and two reference-only Makefile comments differ
between the learner project and references. All other source is compared exactly.
Checkpoints are written to OUTPUT/partN/sectionN; --build builds every ROM.
"""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ("Makefile", "src/game.h", "src/game.c", "src/main.c", "src/data.asm")
EDIT_FILES = SOURCES
DOCUMENTS = ("README.md", "README-part2.md", "README-part3.md", "README-part4.md")
SECTION_COUNTS = (8, 7, 9, 7)
LEARNER_SDK = "export PVSNESLIB_HOME ?= $(abspath $(CURDIR)/../..)"
LEARNER_ERROR = (
    "$(error PVSnesLib not found at $(PVSNESLIB_HOME). Place this project in "
    "snes-examples/breakout or set PVSNESLIB_HOME)"
)
REFERENCE_ERROR = (
    "$(error PVSnesLib 4.6.0 not found at $(PVSNESLIB_HOME). Extract the SDK "
    "into vendor/pvsneslib or set PVSNESLIB_HOME)"
)
REFERENCE_AUDIO_COMMENT = (
    "# The sample music and effects are not redistributed in this repository;\n"
    "# copy them from the SDK's own sound example.\n"
)


def read_document(path):
    """Prefer UTF-8 while accepting the tutorials' former Windows encoding."""
    data = path.read_bytes()
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return data.decode("cp1252")


def function_spans(text, name):
    """Find complete named definitions, ignoring braces in comments/strings."""
    signature = re.compile(
        r"^[A-Za-z_][\w \t*]*\b" + re.escape(name)
        + r"\([^;\n]*\)[ \t]*\n\{", re.MULTILINE,
    )
    non_code = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
                          r"'(?:\\.|[^'\\])*'", re.DOTALL)
    masked = non_code.sub(lambda m: re.sub(r"[^\n]", " ", m[0]), text)
    spans = []
    for match in signature.finditer(masked):
        depth = 0
        for token in re.finditer(r"[{}]", masked[match.end() - 1:]):
            if token[0] == "{":
                depth += 1
            elif token[0] == "}":
                depth -= 1
                if depth == 0:
                    spans.append((match.start(), match.end() - 1 + token.end()))
                    break
        else:
            raise ValueError(f"Unclosed function: {name}")
    return spans


def unique_function(text, name):
    if not isinstance(name, str) or not re.fullmatch(r"[A-Za-z_]\w*", name):
        raise ValueError(f"Invalid function name: {name!r}")
    spans = function_spans(text, name)
    if len(spans) != 1:
        raise ValueError(f"Expected exactly one function {name}; found {len(spans)}")
    return spans[0]


def unique_anchor(text, anchor):
    if not isinstance(anchor, str) or not anchor:
        raise ValueError("An anchor must be a nonempty string")
    starts = [match.start() for match in re.finditer(
        "(?=" + re.escape(anchor) + ")", text,
    )]
    if len(starts) != 1:
        raise ValueError(f"Expected exactly one anchor; found {len(starts)}")
    return starts[0], starts[0] + len(anchor)


def apply_edit(files, edit, code):
    """Apply a validated edit to an in-memory project; never guess a location."""
    name, op = edit.get("file"), edit.get("op")
    if name not in EDIT_FILES:
        raise ValueError(f"Unexpected edit path: {name!r}")
    if not code.endswith("\n"):
        raise ValueError("Code blocks must end with a newline")
    if op == "write":
        files[name] = code
        return
    if name not in files:
        raise ValueError(f"Cannot {op} missing file: {name}")
    text = files[name]
    if op in ("replace-region", "diff-region"):
        scope_start, scope_end = (0, len(text))
        if "function" in edit:
            scope_start, scope_end = unique_function(text, edit["function"])
        elif "scope_before" in edit:
            scope_end = unique_function(text, edit["scope_before"])[0]
        start, end = unique_anchor(text[scope_start:scope_end], edit.get("anchor"))
        if op == "diff-region" and "context" in edit:
            if edit["context"] != 3:
                raise ValueError("Annotated tutorial hunks must request three context lines")
            scope = text[scope_start:scope_end]
            available_before = len(scope[:start].splitlines())
            available_after = len(scope[end:].removeprefix("\n").splitlines())
            for side, available in (("leading", available_before), ("trailing", available_after)):
                count = edit.get("_" + side + "_context")
                if not isinstance(count, int) or count < min(3, available):
                    raise ValueError(f"Missing unchanged {side} context in annotated hunk")
        start += scope_start
        end += scope_start
        text = text[:start] + code.rstrip("\n") + text[end:]
    elif op in ("replace-function", "add-function", "delete-function"):
        function = edit.get("name")
        replacement = code.rstrip("\n")
        if unique_function(replacement, function) != (0, len(replacement)):
            raise ValueError(f"Expected only the complete {function} function")
        if op == "add-function":
            if function_spans(text, function):
                raise ValueError(f"Function already exists: {function}")
            placements = [key for key in ("before", "after") if key in edit]
            if len(placements) != 1:
                raise ValueError("An added function needs exactly one named placement")
            placement = placements[0]
            start, end = unique_function(text, edit[placement])
            if placement == "before":
                text = text[:start] + replacement + "\n\n" + text[start:]
            else:
                text = text[:end] + "\n\n" + replacement + text[end:]
        else:
            start, end = unique_function(text, function)
            if op == "replace-function":
                text = text[:start] + replacement + text[end:]
            else:
                if text[start:end] != replacement:
                    raise ValueError(f"Deletion snippet differs from {function}")
                text = text[:start] + text[end:].removeprefix("\n\n")
    elif op in ("replace-anchor", "delete-anchor", "insert-before", "insert-after"):
        start, end = unique_anchor(text, edit.get("anchor"))
        if op == "replace-anchor":
            text = text[:start] + code.rstrip("\n") + text[end:]
        elif op == "delete-anchor":
            if code.rstrip("\n") != edit["anchor"].rstrip("\n"):
                raise ValueError("Deletion snippet differs from anchor")
            text = text[:start] + text[end:]
        elif op == "insert-before":
            text = text[:start] + code + text[start:]
        else:
            text = text[:end] + code + text[end:]
    elif op == "delete-file":
        if text != code:
            raise ValueError("Deleted file snippet differs from current file")
        del files[name]
        return
    else:
        raise ValueError(f"Unknown edit operation: {op!r}")
    files[name] = text


def markdown_edits(section, require_headings=False):
    """Read actual fenced snippets, requiring independently useful visible labels."""
    pattern = re.compile(
        r"<!-- tutorial-edit: (?P<meta>[^\n]+) -->\n"
        r"(?P<label>\*\*[^\n]+\*\*)\n\n"
        r"```(?P<language>c|asm|make)\n(?P<code>.*?)```"
        r"(?:\n\n(?P<after_label>\*\*[^\n]+change it to:[^\n]*\*\*)\n\n"
        r"```(?P<after_language>c|asm|make)\n(?P<after_code>.*?)```)?",
        re.DOTALL,
    )
    matches = list(pattern.finditer(section))
    if require_headings:
        previous_file = None
        previous_end = 0
        for match in matches:
            edit = json.loads(match["meta"])
            headings = list(re.finditer(r"^### Open `([^`\n]+)`$", section[:match.start()],
                                        re.MULTILINE))
            if not headings:
                raise ValueError("Each section's first edit must have an Open file heading")
            heading = headings[-1]
            if heading[1] != edit.get("file", "").replace("/", "\\"):
                raise ValueError("Open file heading differs from the edit file")
            if not section[:heading.start()].rstrip().endswith("---"):
                raise ValueError("An Open file heading needs a preceding horizontal divider")
            if edit.get("file") != previous_file and heading.start() < previous_end:
                raise ValueError("A file switch needs a new Open file heading")
            previous_file = edit.get("file")
            previous_end = match.end()
    if section.count("<!-- tutorial-edit:") != len(matches):
        raise ValueError("Malformed edit metadata, visible label, or code fence")
    if re.search(r"^```diff\b", section, re.MULTILINE):
        raise ValueError("Learner-facing diff blocks are not permitted")
    for fence in re.finditer(r"^```(?:c|asm|make)\n.*?^```", section,
                             re.MULTILINE | re.DOTALL):
        if not any(match.start() <= fence.start() < match.end() for match in matches):
            preceding = section[:fence.start()].rstrip().splitlines()[-1:]
            if not preceding or not re.fullmatch(
                r"\*\*[^*\n]*example[^*\n]*no file edit[^*\n]*\*\*",
                preceding[0], re.IGNORECASE,
            ):
                raise ValueError("Unmarked source block: add edit metadata and a visible label")
    for match in matches:
        edit = json.loads(match["meta"])
        if not isinstance(edit, dict):
            raise ValueError("Edit metadata must be an object")
        name = edit.get("file", "")
        label = match["label"]
        visible_name = name.replace("/", "\\")
        if f"`{visible_name}`" not in label:
            raise ValueError(f"Visible label must name {visible_name}")
        operation = edit.get("op", "")
        if operation == "diff-region":
            if require_headings and edit.get("context") != 3:
                raise ValueError("Tutorial diff metadata must request three context lines")
            if match["after_code"] is not None:
                raise ValueError("An annotated diff uses one code fence, not paired fences")
            before, after = annotated_versions(match["code"], match["language"])
            if before == after:
                raise ValueError("An annotated diff must contain a change")
            if not before.strip() or not after.strip():
                raise ValueError("Use named addition/deletion for an empty diff side")
            if "apply" not in label.lower() and "replace" not in label.lower():
                raise ValueError("Visible diff label must describe how to apply changes")
            if "function" in edit and f"`{edit['function']}`" not in label:
                raise ValueError("Visible diff label must identify containing function")
            if "scope_before" in edit and f"before `{edit['scope_before']}`" not in label:
                raise ValueError("Visible diff label must identify file-scope placement")
            edit["anchor"] = before.rstrip("\n")
            if "context" in edit:
                lines = match["code"].splitlines()
                changed = [n for n, line in enumerate(lines)
                           if re.search(r"\[!code (?:\+\+|--)\]$", line)]
                edit["_leading_context"] = changed[0]
                edit["_trailing_context"] = len(lines) - changed[-1] - 1
            replay_code = after
        if operation == "replace-region":
            if "find this" not in label.lower() or match["after_code"] is None:
                raise ValueError("A focused region needs visible find/change labels and both snippets")
            if "function" in edit and f"`{edit['function']}`" not in label:
                raise ValueError("Visible find label must identify containing function")
            if f"`{visible_name}`" not in match["after_label"]:
                raise ValueError("Visible change label must identify the edited file")
            if match["after_language"] != match["language"]:
                raise ValueError("Before and after snippets must have the same language")
            edit["anchor"] = match["code"].rstrip("\n")
        elif match["after_code"] is not None:
            raise ValueError("Only replace-region may have paired snippets")
        verb = "replace" if operation.startswith(("write", "replace")) else (
            "add" if operation.startswith(("add", "insert")) else "remove"
        )
        if operation not in ("replace-region", "diff-region") and verb not in label.lower():
            raise ValueError(f"Visible label must describe operation {operation}")
        if "function" in operation and f"`{edit.get('name')}`" not in label:
            raise ValueError("Visible label must identify the edited function")
        for placement in ("before", "after"):
            if placement in edit and f"{placement} `{edit[placement]}`" not in label:
                raise ValueError("Visible label must identify function placement")
        expected_language = "make" if name == "Makefile" else (
            "asm" if name.endswith(".asm") else "c"
        )
        if match["language"] != expected_language:
            raise ValueError(f"Wrong code language for {name}")
        if operation == "replace-region":
            replay_code = match["after_code"]
        elif operation != "diff-region":
            before, after = annotated_versions(match["code"], match["language"])
            if operation.startswith("delete"):
                replay_code = before
            else:
                if before != after and operation not in ("add-function", "write"):
                    raise ValueError(f"Unexpected annotations for operation {operation}")
                replay_code = after
        yield edit, replay_code


def annotated_versions(code, language):
    """Derive exact pre/post-edit source from Shiki gutter annotations."""
    comment = {"c": "//", "make": "#", "asm": ";"}[language]
    annotation = re.compile(
        r"^(?P<line>.*) " + re.escape(comment)
        + r" \[!code (?P<change>\+\+|--)\]$",
    )
    before, after = [], []
    for line in code.splitlines(keepends=True):
        content = line.removesuffix("\n")
        match = annotation.fullmatch(content)
        if match:
            clean = match["line"] + "\n"
            if match["change"] == "--":
                before.append(clean)
            else:
                after.append(clean)
        else:
            if "[!code " in content:
                raise ValueError(f"Malformed {language} diff annotation: {content!r}")
            before.append(line)
            after.append(line)
    return "".join(before), "".join(after)


def compare_reference(files, part, root=ROOT):
    reference = root if part == 1 else root / f"part{part}"
    for name in SOURCES:
        actual = files.get(name)
        expected = (reference / name).read_text(encoding="utf-8")
        if name == "Makefile":
            reference_sdk = "export PVSNESLIB_HOME ?= " + (
                "$(CURDIR)/vendor/pvsneslib" if part == 1
                else "$(abspath $(CURDIR)/../vendor/pvsneslib)"
            )
            # Verify exact known differences, rather than ignoring whole rules
            # or arbitrary comments that could hide a learner regression.
            if expected.count(reference_sdk) != 1 or expected.count(REFERENCE_ERROR) != 1:
                raise ValueError("Reference Makefile SDK convention changed")
            expected = expected.replace(reference_sdk, LEARNER_SDK)
            expected = expected.replace(REFERENCE_ERROR, LEARNER_ERROR)
            if part >= 2:
                if expected.count(REFERENCE_AUDIO_COMMENT) != 1:
                    raise ValueError("Reference Makefile audio comment changed")
                expected = expected.replace(REFERENCE_AUDIO_COMMENT, "")
        if actual != expected:
            raise ValueError(f"Part {part} learner result differs from {reference / name}")


def safe_output(output, root=ROOT):
    output = output.resolve()
    if output == root or root.is_relative_to(output):
        raise ValueError("--output must not be the repository or one of its parents")
    if output.is_relative_to(root):
        relative = output.relative_to(root)
        if relative.parts[0] not in ("build", "tutorial-replay"):
            raise ValueError("Repository outputs must be under build or tutorial-replay")
    return output


def msys_path(path):
    return re.sub(r"^([A-Za-z]):[/\\]", lambda m: "/" + m[1].lower() + "/",
                  str(path)).replace("\\", "/")


def write_checkpoint(files, project, part, root=ROOT):
    if project.exists():
        raise ValueError(f"Checkpoint already exists; choose a fresh --output: {project}")
    project.mkdir(parents=True)
    for name, text in files.items():
        path = project / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8", newline="\n")
    # Artwork, unlike executable source, is copied from the finished references.
    for image_root in [root] + [root / f"part{n}" for n in range(2, part + 1)]:
        for image in image_root.glob("*.png"):
            shutil.copyfile(image, project / image.name)


def run_build(project, sdk, make):
    with (project / "build.log").open("w") as log:
        result = subprocess.run(
            [str(make), f"PVSNESLIB_HOME={msys_path(sdk)}"],
            cwd=project, stdout=log, stderr=subprocess.STDOUT, check=False,
        )
    roms = list(project.glob("*.sfc"))
    if result.returncode or len(roms) != 1 or roms[0].stat().st_size == 0:
        raise RuntimeError(f"ROM build failed: {project / 'build.log'}")


def run_native(project, gcc):
    source = project / "src" / "game.c"
    if not source.exists():
        return False
    native = project / ".native-check"
    native.mkdir()
    smoke = native / "smoke.c"
    smoke.write_text(
        '#include "../src/game.h"\n\n'
        "int main(void)\n{\n"
        "    Game game;\n"
        "    game_reset(&game);\n"
        "    game_tick(&game, 0, 0);\n"
        "    return 0;\n"
        "}\n", newline="\n",
    )
    executable = native / ("smoke.exe" if os.name == "nt" else "smoke")
    with (project / "native.log").open("w") as log:
        result = subprocess.run(
            [str(gcc), "-std=c89", "-Wall", "-Wextra", "-pedantic",
             str(source), str(smoke), "-o", str(executable)],
            stdout=log, stderr=subprocess.STDOUT, check=False,
        )
        if result.returncode == 0:
            result = subprocess.run(
                [str(executable)], stdout=log, stderr=subprocess.STDOUT,
                check=False, timeout=10,
            )
    if result.returncode:
        raise RuntimeError(f"Native compile/smoke failed: {project / 'native.log'}")
    return True


def replay(output, build=False, sdk=None, make="make", gcc=None, root=ROOT):
    output = safe_output(output, root)
    files = {}
    total = checkpoints = native_count = 0
    for part, (document, expected_sections) in enumerate(
        zip(DOCUMENTS, SECTION_COUNTS), 1,
    ):
        text = read_document(root / document)
        sections = re.split(r"^## (\d+)\. [^\n]+$", text, flags=re.MULTILINE)
        if sections[1::2] != [str(n) for n in range(1, expected_sections + 1)]:
            raise ValueError(f"Unexpected numbered sections in {document}")
        if "<!-- tutorial-edit:" in sections[0]:
            raise ValueError(f"Edits outside a numbered section in {document}")
        # Also check the legend for obsolete learner-facing diff examples.
        if re.search(r"^```diff\b", text, re.MULTILINE):
            raise ValueError(f"Diff block remains in {document}")
        for stage, section in zip(sections[1::2], sections[2::2]):
            edits = list(markdown_edits(section, require_headings=True))
            if not edits:
                if part == 1 and stage == "1" and "<!-- tutorial-setup: sdk-hello-world -->" in section:
                    print("INFO part1/section1: external SDK Hello World readiness check; "
                          "learner project begins in section2")
                    continue
                if int(stage) < expected_sections:
                    raise ValueError(f"Missing edits in {document} section {stage}")
                continue
            for index, (edit, code) in enumerate(edits, 1):
                try:
                    apply_edit(files, edit, code)
                except ValueError as error:
                    raise ValueError(f"{document} section {stage}, edit {index}: {error}") from error
            project = output / f"part{part}" / f"section{stage}"
            write_checkpoint(files, project, part, root)
            if build:
                run_build(project, sdk, make)
            if gcc and run_native(project, gcc):
                native_count += 1
            total += len(edits)
            checkpoints += 1
            print(f"PASS part{part}/section{stage}: {len(edits)} edits"
                  + ("; ROM built" if build else ""))
        compare_reference(files, part, root)
        print(f"PASS part{part}: all final source matches reference (known Makefile allowances)")
    print(f"PASS: {total} edits, {checkpoints} checkpoints, "
          f"{checkpoints if build else 0} ROM builds, {native_count} native compile/smokes")
    return files


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build", action="store_true")
    parser.add_argument("--sdk", type=Path, help="PVSnesLib SDK root for ROM builds")
    parser.add_argument("--make", default="make", help="Make executable")
    parser.add_argument("--gcc", help="Optional native C89 compiler/smoke for each game.c checkpoint")
    args = parser.parse_args(argv)
    if args.build and not args.sdk:
        parser.error("--build requires --sdk")
    replay(args.output, args.build, args.sdk, args.make, args.gcc)


if __name__ == "__main__":
    main()
