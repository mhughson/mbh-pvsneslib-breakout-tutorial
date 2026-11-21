"""Focused regressions for strict tutorial edit replay."""

import json
import unittest
from unittest.mock import Mock

from tools import verify_tutorials as verifier


FUNCTION = 'static int value(void)\n{\n    return 1;\n}'
NEXT = 'void next(void)\n{\n}\n'
PROJECT = "#include <snes.h>\n\n" + FUNCTION + "\n\n" + NEXT


class ReplayEditsTests(unittest.TestCase):
    def test_document_reader_prefers_utf8_and_accepts_legacy_windows_encoding(self):
        text = "**Open file — apply these changes:**\n"
        for encoding in ("utf-8", "cp1252"):
            with self.subTest(encoding=encoding):
                path = Mock()
                path.read_bytes.return_value = text.encode(encoding)
                self.assertEqual(verifier.read_document(path), text)

    def test_complete_function_replacement_ignores_comment_and_string_braces(self):
        original = (
            'static int value(void)\n{\n'
            '    const char *text = "}"; /* { */\n'
            '    if (text) {\n        return 1;\n    }\n'
            '    return 0;\n}\n\n' + NEXT
        )
        files = {"src/game.c": original}
        replacement = FUNCTION.replace("return 1", "return 2") + "\n"
        verifier.apply_edit(files, {"file": "src/game.c", "op": "replace-function",
                                    "name": "value"}, replacement)
        self.assertEqual(files["src/game.c"], replacement + "\n" + NEXT)

    def test_duplicate_missing_and_partial_functions_are_rejected_without_mutation(self):
        edit = {"file": "src/game.c", "op": "replace-function", "name": "value"}
        for source, code in (
            (FUNCTION + "\n\n" + FUNCTION, FUNCTION + "\n"),
            (NEXT, FUNCTION + "\n"),
            (PROJECT, "    return 2;\n"),
            (PROJECT, FUNCTION + "\n\n" + NEXT),
        ):
            with self.subTest(source=source, code=code):
                files = {"src/game.c": source}
                with self.assertRaises(ValueError):
                    verifier.apply_edit(files, edit, code)
                self.assertEqual(files["src/game.c"], source)

    def test_fake_definition_in_comment_is_not_a_duplicate(self):
        files = {"src/game.c": "/*\n" + FUNCTION + "\n*/\n" + PROJECT}
        verifier.apply_edit(files, {"file": "src/game.c", "op": "replace-function",
                                    "name": "value"}, FUNCTION + "\n")
        self.assertIn("/*\n" + FUNCTION, files["src/game.c"])

    def test_named_additions_preserve_order_and_deletion_restores_file(self):
        for placement, target in (("before", "next"), ("after", "value")):
            with self.subTest(placement=placement):
                files = {"src/game.c": PROJECT}
                helper = "void helper(void)\n{\n}\n"
                verifier.apply_edit(files, {"file": "src/game.c", "op": "add-function",
                                            "name": "helper", placement: target}, helper)
                self.assertLess(files["src/game.c"].index("value("),
                                files["src/game.c"].index("helper("))
                self.assertLess(files["src/game.c"].index("helper("),
                                files["src/game.c"].index("next("))
                with self.assertRaises(ValueError):
                    verifier.apply_edit(files, {"file": "src/game.c", "op": "add-function",
                                                "name": "helper", placement: target}, helper)
                verifier.apply_edit(files, {"file": "src/game.c", "op": "delete-function",
                                            "name": "helper"}, helper)
                self.assertEqual(files["src/game.c"], PROJECT)

    def test_anchor_replacement_additions_and_deletion(self):
        files = {"src/game.c": "FIRST\nLAST\n"}
        verifier.apply_edit(files, {"file": "src/game.c", "op": "insert-before",
                                    "anchor": "LAST\n"}, "BEFORE\n")
        verifier.apply_edit(files, {"file": "src/game.c", "op": "insert-after",
                                    "anchor": "FIRST\n"}, "AFTER\n")
        verifier.apply_edit(files, {"file": "src/game.c", "op": "replace-anchor",
                                    "anchor": "BEFORE"}, "MIDDLE\n")
        verifier.apply_edit(files, {"file": "src/game.c", "op": "delete-anchor",
                                    "anchor": "AFTER\n"}, "AFTER\n")
        self.assertEqual(files["src/game.c"], "FIRST\nMIDDLE\nLAST\n")

    def test_region_replacement_is_scoped_to_named_function(self):
        first = "void first(void)\n{\n    act();\n}\n"
        second = "void second(void)\n{\n    act();\n}\n"
        files = {"src/game.c": first + "\n" + second}
        verifier.apply_edit(
            files, {"file": "src/game.c", "op": "replace-region",
                    "function": "second", "anchor": "    act();"},
            "    changed();\n",
        )
        self.assertEqual(files["src/game.c"], first + "\n" + second.replace("act()", "changed()"))
        with self.assertRaises(ValueError):
            verifier.apply_edit(
                files, {"file": "src/game.c", "op": "replace-region",
                        "function": "first", "anchor": "    changed();"},
                "    wrong();\n",
            )

    def test_region_replacement_rejects_ambiguity_within_the_function(self):
        source = "void first(void)\n{\n    act();\n    act();\n}\n"
        files = {"src/game.c": source}
        with self.assertRaises(ValueError):
            verifier.apply_edit(
                files, {"file": "src/game.c", "op": "replace-region",
                        "function": "first", "anchor": "    act();"},
                "    changed();\n",
            )
        self.assertEqual(files["src/game.c"], source)

    def test_anchors_must_be_unique_including_overlapping_matches(self):
        for source, anchor in (("aaaa", "aa"), ("one", ""), ("one", "missing")):
            with self.subTest(anchor=anchor):
                files = {"src/game.c": source}
                with self.assertRaises(ValueError):
                    verifier.apply_edit(files, {"file": "src/game.c", "op": "replace-anchor",
                                                "anchor": anchor}, "new\n")
                self.assertEqual(files["src/game.c"], source)

    def test_full_file_write_and_checked_file_deletion(self):
        files = {}
        verifier.apply_edit(files, {"file": "Makefile", "op": "write"}, "all:\n")
        with self.assertRaises(ValueError):
            verifier.apply_edit(files, {"file": "Makefile", "op": "delete-file"}, "wrong\n")
        verifier.apply_edit(files, {"file": "Makefile", "op": "delete-file"}, "all:\n")
        self.assertEqual(files, {})
        for path in ("../game.c", "src/other.c", "C:\\outside.c"):
            with self.assertRaises(ValueError):
                verifier.apply_edit(files, {"file": path, "op": "write"}, "anything\n")

    def test_delete_function_must_show_the_current_function(self):
        files = {"src/game.c": PROJECT}
        with self.assertRaises(ValueError):
            verifier.apply_edit(files, {"file": "src/game.c", "op": "delete-function",
                                        "name": "value"}, FUNCTION.replace("1", "2") + "\n")
        self.assertEqual(files["src/game.c"], PROJECT)

    def test_markdown_requires_visible_file_operation_and_placement(self):
        edit = {"file": "src/game.c", "op": "add-function", "name": "value",
                "before": "next"}
        label = "**`src\\game.c` — add the entire `value` function before `next`:**"
        document = ("<!-- tutorial-edit: " + json.dumps(edit) + " -->\n" + label
                    + "\n\n```c\n" + FUNCTION + "\n```")
        self.assertEqual(list(verifier.markdown_edits(document)), [(edit, FUNCTION + "\n")])
        for bad in (document.replace(label, "**Add this:**"),
                    document.replace("before `next`", "somewhere"),
                    document.replace("```c", "```asm"),
                    "```c\n" + FUNCTION + "\n```",
                    "```diff\n+not_copyable\n```"):
            with self.subTest(document=bad), self.assertRaises(ValueError):
                list(verifier.markdown_edits(bad))

    def test_paired_markdown_replays_the_visible_before_and_after(self):
        edit = {"file": "src/game.c", "op": "replace-region", "function": "value"}
        document = (
            "<!-- tutorial-edit: " + json.dumps(edit) + " -->\n"
            "**`src\\game.c` — in `value`, find this:**\n\n"
            "```c\n    return 1;\n```\n\n"
            "**`src\\game.c` — change it to:**\n\n"
            "```c\n    return 2;\n```"
        )
        parsed = list(verifier.markdown_edits(document))
        self.assertEqual(parsed, [(dict(edit, anchor="    return 1;"), "    return 2;\n")])
        files = {"src/game.c": PROJECT}
        for operation, code in parsed:
            verifier.apply_edit(files, operation, code)
        self.assertEqual(files["src/game.c"], PROJECT.replace("return 1", "return 2"))
        for invalid in (
            document.split("**`src\\game.c` — change it to:")[0],
            document.replace("in `value`", "in the function"),
            document.replace("**`src\\game.c` — change it to:", "**`src\\main.c` — change it to:"),
            document.replace("```c\n    return 2;", "```asm\n    return 2;"),
        ):
            with self.subTest(document=invalid), self.assertRaises(ValueError):
                list(verifier.markdown_edits(invalid))

    def test_annotated_c_diff_derives_both_sides_and_replays_clean_source(self):
        edit = {"file": "src/game.c", "op": "diff-region", "function": "value"}
        document = (
            "<!-- tutorial-edit: " + json.dumps(edit) + " -->\n"
            "**`src\\game.c` — in `value`, apply these changes:**\n\n"
            "```c\n"
            "    return 1; // [!code --]\n"
            "    return 2; // [!code ++]\n"
            "```"
        )
        operations = list(verifier.markdown_edits(document))
        self.assertEqual(operations, [(dict(edit, anchor="    return 1;"), "    return 2;\n")])
        files = {"src/game.c": PROJECT}
        for operation, code in operations:
            verifier.apply_edit(files, operation, code)
        self.assertEqual(files["src/game.c"], PROJECT.replace("return 1", "return 2"))
        self.assertNotIn("[!code", files["src/game.c"])

    def test_annotated_make_and_asm_preserve_tabs_and_meaningful_comments(self):
        cases = (
            ("make", "\told # actual comment # [!code --]\n\tnew # [!code ++]\n",
             "\told # actual comment\n", "\tnew\n"),
            ("asm", '.incbin "old.pic" ; [!code --]\n.incbin "new.pic" ; [!code ++]\n',
             '.incbin "old.pic"\n', '.incbin "new.pic"\n'),
            ("c", " /* explain */ // [!code --]\n // [!code ++]\n",
             " /* explain */\n", "\n"),
        )
        for language, code, before, after in cases:
            with self.subTest(language=language):
                self.assertEqual(verifier.annotated_versions(code, language), (before, after))

    def test_annotated_diff_rejects_wrong_comment_style_and_missing_changes(self):
        meta = {"file": "src/game.c", "op": "diff-region", "function": "value"}
        prefix = ("<!-- tutorial-edit: " + json.dumps(meta) + " -->\n"
                  "**`src\\game.c` — in `value`, apply these changes:**\n\n```c\n")
        for code in ("    return 1;\n",
                     "    return 1; # [!code --]\n    return 2; // [!code ++]\n",
                     "    return 1; // [!code --] trailing\n",
                     "    return 2; // [!code ++]\n"):
            with self.subTest(code=code), self.assertRaises(ValueError):
                list(verifier.markdown_edits(prefix + code + "```"))

    def test_annotated_complete_function_addition_and_deletion(self):
        files = {"src/game.c": PROJECT}
        code = "void helper(void)\n{\n}\n"
        for op, flag, verb in (("add-function", "++", "add"), ("delete-function", "--", "remove")):
            edit = {"file": "src/game.c", "op": op, "name": "helper"}
            placement = ""
            if op == "add-function":
                edit["before"] = "next"
                placement = " before `next`"
            annotated = "".join(line + f" // [!code {flag}]\n" for line in code.splitlines())
            document = (
                "<!-- tutorial-edit: " + json.dumps(edit) + " -->\n"
                f"**`src\\game.c` — {verb} the entire `helper` function{placement}:**\n\n"
                "```c\n" + annotated + "```"
            )
            for operation, source in verifier.markdown_edits(document):
                verifier.apply_edit(files, operation, source)
        self.assertEqual(files["src/game.c"], PROJECT)

    def test_three_line_context_allows_function_boundaries_but_rejects_missing_context(self):
        meta = {"file": "src/game.c", "op": "diff-region", "function": "value", "context": 3}
        prefix = ("<!-- tutorial-edit: " + json.dumps(meta) + " -->\n"
                  "**`src\\game.c` — in `value`, apply these changes:**\n\n```c\n")
        changed = "    return 1; // [!code --]\n    return 2; // [!code ++]\n"
        complete = prefix + "static int value(void)\n{\n" + changed + "}\n```"
        files = {"src/game.c": PROJECT}
        edit, code = next(verifier.markdown_edits(complete))
        verifier.apply_edit(files, edit, code)
        self.assertEqual(files["src/game.c"], PROJECT.replace("return 1", "return 2"))
        files = {"src/game.c": PROJECT}
        edit, code = next(verifier.markdown_edits(prefix + changed + "```"))
        with self.assertRaisesRegex(ValueError, "context"):
            verifier.apply_edit(files, edit, code)

    def test_open_file_headings_are_not_edits_and_are_required_at_file_switches(self):
        def write(file, language, code):
            return ("<!-- tutorial-edit: " + json.dumps({"file": file, "op": "write"}) + " -->\n"
                    + "**`" + file.replace("/", "\\") + "` — create or replace the entire file:**\n\n"
                    + "```" + language + "\n" + code + "```")
        first = "---\n\n### Open `Makefile`\n\n" + write("Makefile", "make", "all:\n")
        second = "\n\n---\n\n### Open `src\\game.c`\n\n" + write("src/game.c", "c", FUNCTION + "\n")
        self.assertEqual(len(list(verifier.markdown_edits(first + second, require_headings=True))), 2)
        for invalid in (write("Makefile", "make", "all:\n"),
                        first + "\n\n" + write("src/game.c", "c", FUNCTION + "\n"),
                        (first + second).replace("### Open `src\\game.c`", "### Open `src\\main.c`"),
                        first.replace("---\n\n", "")):
            with self.subTest(document=invalid), self.assertRaises(ValueError):
                list(verifier.markdown_edits(invalid, require_headings=True))

    def test_reference_allowances_do_not_hide_recipe_changes(self):
        files = {name: (verifier.ROOT / name).read_text() for name in verifier.SOURCES}
        files["Makefile"] = files["Makefile"].replace(
            "export PVSNESLIB_HOME ?= $(CURDIR)/vendor/pvsneslib", verifier.LEARNER_SDK,
        ).replace(verifier.REFERENCE_ERROR, verifier.LEARNER_ERROR)
        verifier.compare_reference(files, 1)
        files["Makefile"] = files["Makefile"].replace("-R", "-X")
        with self.assertRaises(ValueError):
            verifier.compare_reference(files, 1)

    def test_output_rejects_source_and_repository_parent_directories(self):
        for path in (verifier.ROOT, verifier.ROOT.parent, verifier.ROOT / "src"):
            with self.subTest(path=path), self.assertRaises(ValueError):
                verifier.safe_output(path)
        self.assertEqual(verifier.safe_output(verifier.ROOT / "build" / "replay"),
                         (verifier.ROOT / "build" / "replay").resolve())


if __name__ == "__main__":
    unittest.main()
