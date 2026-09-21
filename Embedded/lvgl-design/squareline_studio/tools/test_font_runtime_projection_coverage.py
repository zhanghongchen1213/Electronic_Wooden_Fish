#!/usr/bin/env python3
"""验证运行时文案覆盖同一标签在各状态投影中实际使用的全部字体。"""

from __future__ import annotations

import importlib.util
import json
import sys
import unittest
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parent
VALIDATOR_PATH = TOOLS_DIR / "validate_font_coverage.py"
sys.path.insert(0, str(TOOLS_DIR))
SPEC = importlib.util.spec_from_file_location(
    "validate_font_coverage",
    VALIDATOR_PATH,
)
VALIDATOR = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(VALIDATOR)


class RuntimeProjectionFontCoverageTest(unittest.TestCase):
    """冻结状态投影和 C 代码字体切换曾出现方框的回归合同。"""

    def test_runtime_label_tracks_every_state_projection_font(self) -> None:
        fonts = VALIDATOR.projected_fonts_by_label()

        self.assertEqual(
            {"ns700_20", "ns700_23"},
            fonts["ui_selftest_manual_txt_manual_question"],
        )

    def test_touch_question_runtime_text_exists_in_touch_projection_font(self) -> None:
        glyphs = VALIDATOR.glyph_codepoints(
            VALIDATOR.GENERATED_FONT_DIR / "ui_font_ns700_20.c"
        )

        self.assertTrue(
            set("点击目标，滑块超过 50% 即可") <= glyphs,
            "触摸页 ns700_20 缺少运行时问题文案字形",
        )

    def test_code_driven_font_overrides_are_explicit_in_contract(self) -> None:
        contract = json.loads(
            VALIDATOR.CONTRACT_PATH.read_text(encoding="utf-8")
        )
        entries = {
            entry["selector"]: entry
            for entry in contract["runtime_labels"]
        }

        self.assertEqual(
            {"ns700_22", "ns700_24"},
            set(
                entries["ui_mode_default_btn_mode_standard_label"][
                    "font_codes"
                ]
            ),
        )
        self.assertEqual(
            {"lucide_22", "lucide_28"},
            set(
                entries["ui_home_normal_rail_watch_battery_icon"][
                    "font_codes"
                ]
            ),
        )

    def test_small_step_text_and_footprints_cover_every_runtime_font(self) -> None:
        contract = json.loads(
            VALIDATOR.CONTRACT_PATH.read_text(encoding="utf-8")
        )
        entries = {
            entry["selector"]: entry
            for entry in contract["runtime_labels"]
        }
        self.assertEqual(
            {"健身", "小碎步"},
            set(entries["ui_mode_default_btn_mode_sport_label"]["texts"]),
        )
        self.assertEqual(
            {"", ""},
            set(entries["ui_mode_default_icon_mode_sport"]["texts"]),
        )
        for font_code in ("ns700_22", "ns700_24"):
            glyphs = VALIDATOR.glyph_codepoints(
                VALIDATOR.GENERATED_FONT_DIR / f"ui_font_{font_code}.c"
            )
            self.assertTrue(
                set("小碎步") <= glyphs,
                f"{font_code} 缺少小碎步运行时文案字形",
            )
        for font_code in ("lucide_40", "lucide_44"):
            glyphs = VALIDATOR.glyph_codepoints(
                VALIDATOR.GENERATED_FONT_DIR / f"ui_font_{font_code}.c"
            )
            self.assertIn("", glyphs, f"{font_code} 缺少 footprints U+E3B9")


if __name__ == "__main__":
    unittest.main()
