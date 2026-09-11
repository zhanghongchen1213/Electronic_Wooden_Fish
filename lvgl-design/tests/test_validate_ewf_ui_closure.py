import tempfile
import unittest
import json
import re
from pathlib import Path

from postprocess_ewf_direction_html import extract_div
from ui_text_hygiene import VisibleTextParser
from validate_ewf_ui_closure import DesignParser, expand_contract, validate


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

    def test_full_artifacts_lock_selected_styles(self):
        repo = Path(__file__).resolve().parents[2]
        ux = repo / "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08"
        cases = (
            ("device", "DEVICE-01", repo / "lvgl-design/ewf-device-ui.html", ux / "UI_CONTRACT-device.md"),
            ("miniapp", "MINI-06", repo / "miniapp-design/ewf-miniapp-ui.html", ux / "UI_CONTRACT-miniapp.md"),
        )
        for surface, style, html, contract in cases:
            result = validate(surface, html, contract, "full", style)
            self.assertTrue(result["ok"], result)

    def test_full_rejects_unselected_style(self):
        repo = Path(__file__).resolve().parents[2]
        ux = repo / "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08"
        result = validate("device", repo / "lvgl-design/ewf-device-ui.html", ux / "UI_CONTRACT-device.md", "full", "DEVICE-02")
        self.assertFalse(result["ok"])
        self.assertTrue(any(item["code"] == "selected_style_not_allowed" for item in result["errors"]))

    def test_device_latest_and_component_variant_contracts(self):
        repo = Path(__file__).resolve().parents[2]
        source = (repo / "lvgl-design/ewf-device-ui.html").read_text(encoding="utf-8")
        parser = DesignParser()
        parser.feed(source)
        muyu = extract_div(source, 'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:MUYU][ST:BASE]"')
        self.assertEqual(muyu.count('data-ewf-ring="'), 3)
        self.assertIn('data-ewf-ring-count="3"', muyu)
        self.assertIn('data-ewf-duration-ms="160"', muyu)
        self.assertEqual(len(re.findall(r'\[CMP:glyph-current\]\[VAR:latest\]', muyu)), 1)
        jingwen = extract_div(source, 'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:JINGWEN][ST:BASE]"')
        self.assertIn('data-ewf-scroll-anchor="tail"', jingwen)
        self.assertIn('[CMP:glyph-current][VAR:latest-inline]', jingwen)
        self.assertIn('data-ewf-anchor="scripture-history-tail"', jingwen)
        self.assertEqual(jingwen.count('data-ewf-line-char-count="13"'), 2)
        self.assertIn('data-ewf-line-index="2"', jingwen)
        self.assertIn('data-ewf-slot-index="9"', jingwen)
        for state in ("EMPTY", "MID", "FULL", "UNTRUSTED_TIME"):
            frame = extract_div(source, f'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:MUYU][ST:{state}]"')
            self.assertIn('data-ewf-screen-variant="false"', frame)
            self.assertIn(f'data-ewf-component-state="{state}"', frame)
            if state == "EMPTY":
                self.assertNotIn('[CMP:glyph-current][VAR:latest]', frame)
            else:
                self.assertEqual(len(re.findall(r'\[CMP:glyph-current\]\[VAR:latest\]', frame)), 1)
            positions = [int(value) for value in re.findall(r'left:\s*(\d+)px', frame) if int(value) in {15, 63, 111, 159, 207, 255, 303}]
            self.assertEqual(len(positions), len(set(positions)), state)
        statistics = extract_div(source, 'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:TONGJI][ST:UNTRUSTED_TIME]"')
        self.assertIn('data-ewf-screen-variant="false"', statistics)
        settings_rows = {}
        for key, frame in parser.frames.items():
            if ":SHEZHI." not in key:
                continue
            rows = []
            for name, attrs in frame["nodes"]:
                match = re.search(r"\[CMP:row\]\[VAR:(brightness|timeout)(?:-[^\]]+)?\]", name)
                if match:
                    rows.append((match.group(1), re.sub(r"\s+", " ", attrs.get("style", "")).strip()))
            settings_rows[key.split(".", 1)[1]] = tuple(sorted(rows))
        for state, rows in settings_rows.items():
            if state != "BASE":
                self.assertEqual(rows, settings_rows["BASE"], state)
        settings = extract_div(source, 'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:SHEZHI][ST:BASE]"')
        self.assertNotIn('[CMP:statusbar]', settings)
        self.assertIn('data-ewf-scroll-axis="vertical"', settings)
        self.assertIn('data-ewf-max-visible-rows="4"', settings)
        self.assertIn('data-ewf-page-count="2"', settings)
        self.assertIn('data-ewf-content-height="520"', settings)
        self.assertEqual(settings.count('data-ewf-identity-row="true"'), 2)
        statistics_base = extract_div(source, 'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:TONGJI][ST:BASE]"')
        self.assertEqual(statistics_base.count('data-ewf-stat-card="true"'), 2)
        self.assertIn('data-ewf-stats-scope="today-total-only"', statistics_base)
        for state in ("SYNC_BUSY", "SYNC_OK", "SYNC_PENDING", "SYNC_FAIL"):
            frame = extract_div(source, f'data-pencil-name="[UI][STYLE:DEVICE-01][PAGE:SHEZHI][ST:{state}]"')
            self.assertIn('data-ewf-screen-variant="false"', frame)
            self.assertIn(f'data-ewf-component-state="{state}"', frame)

    def test_miniapp_latest_underline_and_component_states(self):
        repo = Path(__file__).resolve().parents[2]
        source = (repo / "miniapp-design/ewf-miniapp-ui.html").read_text(encoding="utf-8")
        for state in ("LIVE", "REPLAY", "OFFLINE", "EMPTY", "DONE"):
            frame = extract_div(source, f'data-pencil-name="[UI][STYLE:MINI-06][PAGE:READING][ST:{state}]"')
            self.assertIn('[CMP:char-focus][VAR:latest]', frame)
            self.assertIn('[CMP:char-focus][VAR:underline-latest]', frame)
            self.assertEqual(frame.count('[CMP:scripture-progress][VAR:master]'), 1)
            if state != "LIVE":
                self.assertIn('data-ewf-screen-variant="false"', frame)

    def test_device_font_contract_tracks_design_text(self):
        repo = Path(__file__).resolve().parents[2]
        contract = json.loads((repo / "lvgl-design/squareline_studio/font_glyph_contract.json").read_text(encoding="utf-8"))
        source = (repo / contract["design_source"]).read_text(encoding="utf-8")
        visible = VisibleTextParser()
        visible.feed(source)
        self.assertEqual(contract["schema_version"], 1)
        self.assertEqual(contract["status"], "design-locked")
        self.assertEqual(contract["last_verified"], "2026-09-11")
        self.assertTrue(contract["pending_canonical_source"])
        self.assertEqual(contract["design_snapshot"]["source_path"], contract["design_source"])
        self.assertEqual(set(contract["design_snapshot"]["characters"]), set("".join(visible.visible)))
        for entry in contract["runtime_labels"]:
            self.assertTrue(entry["font_codes"])
            for text in entry["texts"]:
                self.assertIn(text, source, entry["selector"])

    def test_device_completion_overlay_has_one_owner(self):
        repo = Path(__file__).resolve().parents[2]
        contract = (repo / "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08/UI_CONTRACT-device.md").read_text(encoding="utf-8")
        self.assertIn("MUYU.DONE_OVERLAY", contract)
        self.assertNotIn("[OVERLAY][DONE]", contract)

    def test_ewf_squareline_plan_matches_pen_contract(self):
        repo = Path(__file__).resolve().parents[2]
        plan = json.loads((repo / "lvgl-design/squareline_studio/ewf_project_manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(plan["status"], "design-plan")
        self.assertEqual(plan["source"]["selected_style"], "DEVICE-01")
        self.assertEqual(plan["evidence_from_legbot_watch"]["canvas"]["width"], 410)
        self.assertEqual(plan["evidence_from_legbot_watch"]["canvas"]["height"], 502)
        self.assertEqual(plan["mount_model"]["resident_screen"], "ui_scr_main_shell")
        self.assertEqual(plan["settings_contract"]["viewport"]["max_visible_rows"], 4)
        component_ids = {item["id"] for item in plan["component_registry"]}
        self.assertTrue({"statusbar", "woodfish", "woodfish-anatomy", "tap-rings", "scripture-history", "stat-card", "settings-list", "device-identity"}.issubset(component_ids))
        statusbar = next(item for item in plan["component_registry"] if item["id"] == "statusbar")
        self.assertEqual(statusbar["master"], "VphYz")
        self.assertEqual(statusbar["pencil_name"], "cmp_statusbar_gGgAm")
        woodfish = next(item for item in plan["component_registry"] if item["id"] == "woodfish")
        self.assertIn("woodfish-anatomy", woodfish["children"])

    def test_statusbar_uses_single_signal_slot_tree(self):
        repo = Path(__file__).resolve().parents[2]
        source = (repo / "lvgl-design/ewf-device-ui-export.html").read_text(encoding="utf-8")
        for legacy in ("signal-4g-connected", "signal-4g-no-signal", "signal-4g-disabled", "signal-wifi-connected", "signal-ble-connected", "signal-gps-connected"):
            self.assertNotIn(legacy, source)


if __name__ == "__main__":
    unittest.main()
