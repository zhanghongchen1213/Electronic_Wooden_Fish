import tempfile
import unittest
import json
from pathlib import Path

from validate_ewf_ui_closure import expand_contract, validate


class ClosureValidatorTest(unittest.TestCase):
    def test_style_manifests_declare_three_non_color_axes(self):
        repo = Path(__file__).resolve().parents[2]
        for manifest_path in (repo / "lvgl-design/device-style-options.json", repo / "miniapp-design/miniapp-style-options.json"):
            styles = json.loads(manifest_path.read_text(encoding="utf-8"))["styles"]
            self.assertEqual(len(styles), 10)
            self.assertEqual(len({style.get("layout") for style in styles}), 10)
            for style in styles:
                axes = {str(axis).lower() for axis in style.get("axes", []) if str(axis).lower() != "color"}
                self.assertGreaterEqual(len(axes), 3, style["id"])

    def test_same_structure_with_different_style_ids_is_flagged(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            contract = root / "contract.md"
            html = root / "candidates.html"
            contract.write_text("```\n[MUYU][BASE] base\n```\n", encoding="utf-8")
            frames = []
            for style in ("DEVICE-01", "DEVICE-02"):
                frames.append(
                    f'<div data-ewf-frame="true" data-style-id="{style}" data-pencil-name="[UI][STYLE:{style}][PAGE:MUYU][ST:BASE]" style="width:410px;height:502px;border-radius:110px;overflow:hidden">'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:statusbar][VAR:master]" style="width:314px;height:24px;left:48px;top:20px"></div>'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:charcell][VAR:recent-window]" style="width:362px;height:64px"></div>'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:scripture-progress][VAR:master]" style="width:346px;height:58px"></div>'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:today-taps][VAR:confirmed]" style="width:170px;height:28px"></div>'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:total-taps][VAR:confirmed]" style="width:170px;height:28px"></div>'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:woodfish][VAR:tap-zone]" style="width:180px;height:140px"></div>'
                    '<div data-pencil-name="[UI][STYLE:' + style + '][PAGE:MUYU][ST:BASE][CMP:woodfish-anatomy][VAR:idle]" style="width:180px;height:140px"></div></div>'
                )
            html.write_text("".join(frames), encoding="utf-8")
            result = validate("device", html, contract, "candidates")
            self.assertTrue(any(item["code"] == "style_structure_duplicate" for item in result["errors"]), result)

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
                """<!doctype html><div data-ewf-frame="true" data-pencil-name="[UI][PAGE:MUYU][ST:BASE]" class="w-[410px] h-[502px]" style="border-radius:110px;overflow:hidden">
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:statusbar]" style="width:314px;height:24px;left:48px;top:20px"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:charcell]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:scripture-progress]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:today-taps]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:total-taps]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:woodfish][VAR:tap-zone]" class="w-[160px] h-[120px]"></div>
                <div data-pencil-name="[UI][PAGE:MUYU][ST:BASE][CMP:woodfish-anatomy][VAR:idle]" class="w-[160px] h-[120px]"></div>
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
                <div data-pencil-name="[UI][PAGE:READING][ST:LIVE][CMP:scripture-progress]"></div>
                <div data-pencil-name="[UI][PAGE:READING][ST:LIVE][CMP:bottom-nav]"></div>
                <span>电子木鱼</span></div>""",
                encoding="utf-8",
            )
            result = validate("miniapp", html, contract, "direction")
            self.assertFalse(result["ok"])
            self.assertTrue(any(item["code"] == "miniapp_forbidden_woodfish" for item in result["errors"]))


if __name__ == "__main__":
    unittest.main()
