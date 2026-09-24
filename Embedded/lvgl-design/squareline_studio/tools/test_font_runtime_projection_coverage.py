#!/usr/bin/env python3
"""EWF 运行时投影字体覆盖单测（Story 3.1）。"""

from __future__ import annotations

import importlib.util
import json
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
VALIDATOR_PATH = TOOLS_DIR / "validate_font_coverage.py"
sys.path.insert(0, str(TOOLS_DIR))
SPEC = importlib.util.spec_from_file_location("validate_font_coverage", VALIDATOR_PATH)
VALIDATOR = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(VALIDATOR)


class RuntimeProjectionFontCoverageTest(unittest.TestCase):
    def test_contract_font_codes_cover_shell_titles(self) -> None:
        contract = json.loads(VALIDATOR.CONTRACT_PATH.read_text(encoding="utf-8"))
        entries = {e["selector"]: e for e in contract["runtime_labels"]}
        self.assertEqual({"serif700_28"}, set(entries["ui_muyu_title"]["font_codes"]))
        self.assertEqual({"serif700_28"}, set(entries["ui_jingwen_title"]["font_codes"]))
        self.assertEqual({"ns700_22"}, set(entries["ui_tongji_title"]["font_codes"]))
        self.assertIn("待同步", entries["statusbar_txt_sync"]["texts"])
        self.assertNotIn("已同步成功", entries["statusbar_txt_sync"]["texts"])

    def test_shell_title_glyphs_present(self) -> None:
        serif = VALIDATOR.glyph_codepoints(
            VALIDATOR.GENERATED_FONT_DIR / "ui_font_serif700_28.c"
        )
        sans = VALIDATOR.glyph_codepoints(
            VALIDATOR.GENERATED_FONT_DIR / "ui_font_ns700_22.c"
        )
        self.assertTrue(set("木鱼") <= serif)
        self.assertTrue(set("经文") <= serif)
        self.assertTrue(set("统计") <= sans)

    def test_muyu_belt_font_codes_and_glyphs(self) -> None:
        contract = json.loads(VALIDATOR.CONTRACT_PATH.read_text(encoding="utf-8"))
        entries = {e["selector"]: e for e in contract["runtime_labels"]}
        self.assertEqual(
            {"serif700_52", "serif500_26", "serif400_26"},
            set(entries["ui_muyu_glyph_slots"]["font_codes"]),
        )
        self.assertIn("DYNAMIC_SCRIPTURE_BELT", entries["ui_muyu_glyph_slots"]["texts"])
        glyph52 = VALIDATOR.glyph_codepoints(
            VALIDATOR.GENERATED_FONT_DIR / "ui_font_serif700_52.c"
        )
        glyph26 = VALIDATOR.glyph_codepoints(
            VALIDATOR.GENERATED_FONT_DIR / "ui_font_serif500_26.c"
        )
        empty26 = VALIDATOR.glyph_codepoints(
            VALIDATOR.GENERATED_FONT_DIR / "ui_font_serif400_26.c"
        )
        self.assertTrue(set("观自在") <= glyph52)
        self.assertTrue(set("观自在") <= glyph26)
        self.assertTrue(set("·") <= empty26)
        progress = entries["ui_muyu_scripture_progress"]["texts"]
        self.assertTrue(any("%" in t for t in progress))

    def test_projected_fonts_by_label_nonempty(self) -> None:
        fonts = VALIDATOR.projected_fonts_by_label()
        self.assertIn("ui_muyu_title", fonts)
        self.assertTrue(fonts["ui_muyu_title"])


if __name__ == "__main__":
    raise SystemExit(unittest.main())
