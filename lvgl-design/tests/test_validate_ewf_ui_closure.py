import tempfile
import unittest
from pathlib import Path

from validate_ewf_ui_closure import expand_contract, validate


class ClosureValidatorTest(unittest.TestCase):
    def test_expands_slash_states_and_ignores_shell(self):
        with tempfile.TemporaryDirectory() as tmp:
            contract = Path(tmp) / "contract.md"
            contract.write_text("```\n[MUYU][BASE] base\n[SHEZHI][SYNC_BUSY/OK] sync\n[SHELL][VAR:charging] shell\n```\n", encoding="utf-8")
            self.assertEqual(
                expand_contract(contract),
                {"MUYU.BASE", "SHEZHI.SYNC_BUSY", "SHEZHI.OK"},
            )

    def test_direction_frame_passes_minimum_contract(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            contract = root / "contract.md"
            html = root / "direction.html"
            contract.write_text("```\n[MUYU][BASE] base\n```\n", encoding="utf-8")
            html.write_text(
                """<!doctype html><div data-ewf-frame="true" data-pencil-name="[UI][PAGE:MUYU][ST:BASE]" class="w-[410px] h-[502px]">
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:statusbar]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:charcell]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:progress-pill]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:today-count]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:woodfish]" class="w-[160px] h-[120px]"></div>
                </div>""",
                encoding="utf-8",
            )
            result = validate("device", html, contract, "direction")
            self.assertTrue(result["ok"], result)

    def test_miniapp_rejects_woodfish(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            contract = root / "contract.md"
            html = root / "direction.html"
            contract.write_text("```\n[READING][LIVE] live\n```\n", encoding="utf-8")
            html.write_text(
                """<div data-ewf-frame="true" data-pencil-name="[UI][PAGE:READING][ST:LIVE]" class="w-[390px] h-[844px]">
                <div data-pencil-name="[UI][PAGE:READING][ST:LIVE][CMP:readingline]"></div>
                <div data-pencil-name="[UI][PAGE:READING][ST:LIVE][CMP:char-focus]"></div>
                <div data-pencil-name="[UI][PAGE:READING][ST:LIVE][CMP:bottom-nav]"></div>
                <span>电子木鱼</span></div>""",
                encoding="utf-8",
            )
            result = validate("miniapp", html, contract, "direction")
            self.assertFalse(result["ok"])
            self.assertTrue(any(item["code"] == "miniapp_forbidden_woodfish" for item in result["errors"]))


if __name__ == "__main__":
    unittest.main()
