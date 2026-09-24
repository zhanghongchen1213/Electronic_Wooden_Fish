"""Story 3.1 UI 壳层与字体合同主机扫描。"""
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
SL = ROOT / "lvgl-design/squareline_studio"
GEN = ROOT / "components/ui/generated"
BIND = ROOT / "components/ui/bindings"

FORBIDDEN = ("TODO", "draft", "placeholder", "data-pencil-id", "助力", "外骨骼", "支付", "档位")

def test_geometry_header():
    text = (ROOT / "components/ui/ewf_ui_geometry.h").read_text(encoding="utf-8")
    assert "EWF_UI_CANVAS_W 410" in text
    assert "EWF_UI_CANVAS_H 502" in text
    assert "EWF_UI_CORNER_RADIUS 110" in text
    assert "裁决 A" in text and "device_nav_service" in text

def test_font_contract_schema():
    c = json.loads((SL / "font_glyph_contract.json").read_text(encoding="utf-8"))
    assert c["schema_version"] == 1
    assert c["project_name"] == "ewf-device"
    assert c["canonical_scripture_version"] == "HS-1.0.0"
    assert "runtime_sources" in c and "runtime_labels" in c

def test_generated_has_no_legacy_screens():
    screens = {p.name for p in (GEN / "screens").glob("*.c")}
    assert "ui_scr_shell.c" in screens
    assert "ui_scr_shezhi.c" in screens
    assert not any("ble" in n or "selftest" in n or "maintenance" in n for n in screens)

def test_generated_no_forbidden_copy():
    for path in list((GEN / "screens").rglob("*.c")) + list((BIND).glob("*.c")):
        text = path.read_text(encoding="utf-8")
        for bad in FORBIDDEN:
            assert bad not in text, f"{path} contains {bad}"

def test_generated_no_nvs_https_business():
    for path in (GEN).rglob("*.c"):
        text = path.read_text(encoding="utf-8")
        assert "nvs_flash" not in text
        assert "esp_https" not in text
        assert "nvs_open" not in text
        assert "tap_round_cursor" not in text
        assert "progress_service" not in text

def test_muyu_constants_contract():
    text = (BIND / "ui_muyu_constants.h").read_text(encoding="utf-8")
    assert "EWF_UI_MUYU_TAP_RING_COUNT 3" in text
    assert "EWF_UI_MUYU_TAP_FLASH_MS 160" in text
    assert "EWF_UI_MUYU_TOUCH_MIN_PX 96" in text
    assert "0xE6BD69" in text
    assert "0xD9A441" in text
    assert "EWF_UI_MUYU_BELT_SLOTS 7" in text
    assert "EWF_UI_MUYU_MODAL_CARD_W 338" in text
    assert "EWF_UI_MUYU_MODAL_CARD_H 190" in text
    assert "EWF_UI_MUYU_MODAL_OVERLAY_OPA 0x99" in text
    assert "本轮完成" in text
    assert "从头开始" in text
    assert "退出" in text

def test_muyu_shell_has_belt_and_rings():
    text = (GEN / "screens/ui_scr_shell.c").read_text(encoding="utf-8")
    assert "ui_muyu_glyph_slots" in text
    assert "ui_muyu_tap_rings" in text
    assert "ui_muyu_scripture_progress" in text
    assert "心经进度" in text
    assert "ui_muyu_modal_done" in text
    assert "ui_muyu_modal_btn_restart" in text
    assert "ui_muyu_modal_btn_exit" in text
    assert "本轮完成" in text
    assert "从头开始" in text
    assert "退出" in text
    assert "ui_scr_overlay" not in text.lower()
    assert "confetti" not in text.lower()
    # 注释可含「禁止…OVERLAY」，但不得另建独立 overlay screen。
    assert "ui_scr_done_overlay" not in text.lower()

def test_serif_belt_fonts_present():
    fonts = GEN / "fonts"
    for name in ("ui_font_serif700_52.c", "ui_font_serif500_26.c", "ui_font_serif400_26.c"):
        assert (fonts / name).is_file(), name

def test_jingwen_constants_contract():
    text = (BIND / "ui_jingwen_constants.h").read_text(encoding="utf-8")
    assert "EWF_UI_JINGWEN_ROW_SLOTS 13" in text
    assert "EWF_UI_JINGWEN_HISTORY_W 362" in text
    assert "EWF_UI_JINGWEN_HISTORY_H 286" in text
    assert "0x121212" in text

def test_jingwen_shell_has_history_no_woodfish():
    text = (GEN / "screens/ui_scr_shell.c").read_text(encoding="utf-8")
    assert "ui_jingwen_history" in text
    assert "ui_jingwen_scripture_progress" in text
    # JINGWEN 页不得挂 woodfish / tap-rings（MUYU 专属符号仍可出现在同文件）。
    jingwen_block = text.split("ui_jingwen_title")[1].split("ui_tongji_title")[0]
    assert "woodfish" not in jingwen_block.lower()
    assert "tap_ring" not in jingwen_block.lower()
    assert "ui_woodfish_mark_create" not in jingwen_block

def test_tongji_constants_contract():
    text = (BIND / "ui_tongji_constants.h").read_text(encoding="utf-8")
    assert "EWF_UI_TONGJI_CARD_W 378" in text
    assert "EWF_UI_TONGJI_PROGRESS_DENOM" in text
    assert "0xD9A441" in text
    assert "near_7" not in text
    assert "near_30" not in text
    assert "streak_" not in text

def test_tongji_shell_has_cards_no_woodfish():
    text = (GEN / "screens/ui_scr_shell.c").read_text(encoding="utf-8")
    assert "ui_tongji_today_card" in text
    assert "ui_tongji_total_card" in text
    assert "ui_tongji_progress_arc" in text
    assert "今日敲击" in text
    assert "累计敲击" in text
    # 仅取「统计」标题创建到 destroy 之前的布局段。
    marker = 'lv_label_set_text(ui_tongji_title, "统计");'
    assert marker in text
    tongji_block = text.split(marker, 1)[1].split("ui_scr_shell_screen_destroy", 1)[0]
    assert "ui_woodfish" not in tongji_block
    assert "ui_muyu_tap_rings" not in tongji_block
    assert "近7日" not in tongji_block
    assert "近30日" not in tongji_block
    assert "连续天数" not in tongji_block

def test_tongji_policy_no_streak_fields():
    text = (BIND / "ui_tongji_stats_policy.h").read_text(encoding="utf-8")
    assert "near_7" not in text
    assert "near_30" not in text
    assert "streak_" not in text
    assert "today_untrusted" in text

def test_shezhi_constants_contract():
    text = (BIND / "ui_shezhi_constants.h").read_text(encoding="utf-8")
    assert "EWF_UI_SHEZHI_VIEWPORT_W 378" in text
    assert "EWF_UI_SHEZHI_VIEWPORT_H 344" in text
    assert "EWF_UI_SHEZHI_CONTENT_H 520" in text
    assert "EWF_UI_SHEZHI_PAGE1_ROW_COUNT 4" in text
    assert "EWF_UI_SHEZHI_PAGE2_TOP 344" in text
    assert "0xD9A441" in text
    assert "CHARGING_PAUSE" in text  # 裁决登记：明确不实现

def test_shezhi_screen_two_pages_no_woodfish():
    text = (GEN / "screens/ui_scr_shezhi.c").read_text(encoding="utf-8")
    assert "屏幕亮度" in text
    assert "自动熄屏" in text
    assert "木鱼版本" in text
    assert "木鱼ID" in text
    assert "ui_shezhi_viewport" in text
    assert "ui_shezhi_sync_action_label" in text
    assert "woodfish" not in text.lower()
    assert "tap_ring" not in text.lower()
    assert "ui_scr_shezhi_sync_" not in text  # 禁止五态复制整屏
    assert text.count("make_row(ui_shezhi_content") == 6  # 4+2 行，非 5 份整屏

def test_shezhi_policy_sync_vocab_and_no_medium():
    text = (BIND / "ui_shezhi_settings_policy.c").read_text(encoding="utf-8")
    for phrase in ("立即同步", "同步中", "已同步", "待同步", "同步失败"):
        assert phrase in text
    assert '"medium"' not in text
    assert "成功了" not in text
    assert "失败了" not in text

def test_shezhi_no_charging_pause_gate():
    for path in (BIND / "ui_shezhi_projection.c", BIND / "ui_runtime_events.c"):
        text = path.read_text(encoding="utf-8")
        assert "CHARGING_PAUSE" not in text
        assert "charging_pause" not in text
        assert "tap_input_service_submit_device_touch" not in text
