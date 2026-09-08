#!/usr/bin/env python3
"""将 Pencil 最终 UI 的 HTML 导出转换为 SquareLine Studio 1.6.1 工程。"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import math
import re
import shutil
import subprocess
import time
from collections import defaultdict
from dataclasses import dataclass, field
from html.parser import HTMLParser
from pathlib import Path


CANVAS_WIDTH = 410
CANVAS_HEIGHT = 502
VISIBLE_SHAPE = "rounded_rect"
CORNER_RADIUS = 110
CRITICAL_CONTENT_CLEARANCE = 8
SETTINGS_PAGE_2_STATE_NAME = "[UI][STATE][05-01][settings_page_2] 设置·第二页"
SETTINGS_PAGE_2_CONTENT_OFFSET_Y = -344
SETTINGS_PAGE_2_THUMB_Y = 248
MODE_FOUR_LAYOUT_STATE_NAME = (
    "[UI][STATE][mode_layout][four_modes] 模式与电源·四模式参考（Pro/Max）"
)
MODE_TITLE_RESERVE_NAME = "ui_lbl_current_mode"
MODE_TITLE_RESERVE_X = 500
SETTINGS_RAISE_WAKE_NAME = "ui_row_raise_wake_instance"
SETTINGS_RAISE_WAKE_SOURCE_NAME = "ui_row_haptics_instance"


PAGE_STATES = {
    "AM1lb": "home_normal",
    "PQeA3": "home_unbound",
    "cFi0R": "home_disconnected",
    "C1zXo": "control_connected",
    "c0Qzy": "control_disconnected",
    "VUJnm": "mode_default",
    "oVN5R": "mode_disconnected",
    "hqXGZ": "alerts_empty",
    "T7Vzre": "alerts_readonly",
    "IrPIl": "payment_required",
    "nI2yN": "payment_verify_failed",
    "mH17G": "settings",
    "W027H": "device_info",
    "r8yvU": "maintenance",
    "Bnqio": "ble_empty",
    "DpfuC": "ble_candidates",
    "wOBTZ": "selftest_idle",
    "EYtIu": "selftest_progress",
    "l8MgJa": "selftest_manual",
    "kqXCS": "selftest_result",
    "f7Vzs": "selftest_gps_context",
    "Q6Y3P": "selftest_retry_detail",
    "QTmUu": "clear_binding_confirm",
}

VARIANT_STATES = {
    "AMI7P": "selftest_manual_touch",
    "w6kvw": "ble_scanning",
    "bxZQZ": "selftest_retry_auto",
    "lLjZ6": "selftest_manual_vibration",
    "mNNUk": "selftest_result_all_passed",
    "UiLdo": "selftest_gps_submit_loading",
    "rFoTg": "selftest_gps_submit_failed",
}

VARIANT_BASE_STATES = {
    "ble_scanning": "ble_empty",
    "selftest_retry_auto": "selftest_progress",
    "selftest_manual_touch": "selftest_manual",
    "selftest_manual_vibration": "selftest_manual",
    "selftest_result_all_passed": "selftest_result",
    "selftest_gps_submit_loading": "selftest_gps_context",
    "selftest_gps_submit_failed": "selftest_gps_context",
}

# 仅保留机器可读的结构化标题；中文展示标题不参与匹配。
PAGE_TITLES = {state: state for state in PAGE_STATES.values()}

STATE_CODES = {
    "home_normal": "01-01",
    "home_unbound": "01-02",
    "home_disconnected": "01-03",
    "control_connected": "02-01",
    "control_disconnected": "02-02",
    "mode_default": "03-01",
    "mode_disconnected": "03-02",
    "alerts_empty": "04-01",
    "alerts_readonly": "04-02",
    "payment_required": "04-03",
    "payment_verify_failed": "04-04",
    "settings": "05-01",
    "device_info": "05-02",
    "maintenance": "06-01",
    "ble_empty": "07-01",
    "ble_candidates": "07-02",
    "selftest_idle": "08-01",
    "selftest_progress": "08-02",
    "selftest_manual": "08-03",
    "selftest_result": "08-04",
    "selftest_gps_context": "08-05",
    "selftest_retry_detail": "08-06",
    "clear_binding_confirm": "09-01",
    "ble_scanning": "07-01",
    "selftest_retry_auto": "08-02",
    "selftest_manual_touch": "08-03",
    "selftest_manual_vibration": "08-03",
    "selftest_result_all_passed": "08-04",
    "selftest_gps_submit_loading": "08-05",
    "selftest_gps_submit_failed": "08-05",
}

STATE_PAGE_IDS = {state: node_id for node_id, state in PAGE_STATES.items()}
STATE_SOURCE_IDS = {**STATE_PAGE_IDS, **{state: node_id for node_id, state in VARIANT_STATES.items()}}

ROOT_STATES = {
    "ui_page_home_root": ["home_normal", "home_unbound", "home_disconnected"],
    "ui_page_control_root": ["control_connected", "control_disconnected"],
    "ui_page_mode_power_root": ["mode_default", "mode_disconnected"],
    "ui_page_alerts_root": ["alerts_empty", "alerts_readonly", "payment_required", "payment_verify_failed"],
    "ui_page_settings_root": ["settings"],
    "ui_page_device_info_root": ["device_info"],
    "ui_page_maintenance_root": ["maintenance"],
    "ui_page_ble_root": ["ble_empty", "ble_candidates", "ble_scanning"],
    "ui_page_selftest_progress_root": [
        "selftest_idle",
        "selftest_progress",
        "selftest_manual",
        "selftest_retry_auto",
        "selftest_manual_touch",
        "selftest_manual_vibration",
    ],
    "ui_page_selftest_result_root": ["selftest_result", "selftest_result_all_passed"],
    "ui_page_selftest_gps_context_root": [
        "selftest_gps_context",
        "selftest_gps_submit_loading",
        "selftest_gps_submit_failed",
    ],
    "ui_page_selftest_retry_detail_root": ["selftest_retry_detail"],
    "ui_modal_clear_binding_root": ["clear_binding_confirm"],
}

SCREEN_STATES = [
    ("ui_scr_main_shell", ["ui_page_home_root", "ui_page_control_root", "ui_page_mode_power_root", "ui_page_alerts_root"], False, "HOR"),
    ("ui_scr_settings", ["ui_page_settings_root"], True, "NONE"),
    ("ui_scr_device_info", ["ui_page_device_info_root"], True, "NONE"),
    ("ui_scr_maintenance", ["ui_page_maintenance_root", "ui_modal_clear_binding_root"], True, "NONE"),
    ("ui_scr_ble_candidates", ["ui_page_ble_root"], True, "NONE"),
    ("ui_scr_selftest_progress", ["ui_page_selftest_progress_root"], True, "NONE"),
    ("ui_scr_selftest_result", ["ui_page_selftest_result_root"], True, "NONE"),
    ("ui_scr_selftest_gps_context", ["ui_page_selftest_gps_context_root"], True, "NONE"),
    ("ui_scr_selftest_retry_detail", ["ui_page_selftest_retry_detail_root"], True, "NONE"),
]

RESIDENT_SCREEN_NAMES = {"ui_scr_main_shell"}

# Phase 1/C2 曾经使用无版本前缀的 Screen GUID。SquareLine 会按工程路径和
# 旧 GUID 合并内部对象身份，导致主壳保存时回退为历史名称 uiscrmainshell。
# Phase 2 使用新的稳定命名空间，既避开旧缓存，又保持重复生成确定性。
SCREEN_GUID_NAMESPACE = "ui-state-closure-phase-2-v1"

# SquareLine 1.6.1 uses these bits to distinguish a component-instance override
# from an inherited master value.  Without them Studio accepts the generated
# project, but restores the master position/text/visibility on the next save.
COMPONENT_VALUE_OVERRIDE_FLAG = 1 << 16
COMPONENT_IDENTITY_OVERRIDE_FLAG = 1 << 17

COMPONENT_SOURCES = {
    "cmp_setting_toggle": ("settings", "ui_row_haptics_instance", 3),
    "cmp_alert_row": ("alerts_readonly", "ui_alert_exo_battery_warning_instance", 3),
    "cmp_ble_candidate_row": ("ble_candidates", "ui_device_candidate_1", 4),
    "cmp_selftest_result_row": ("selftest_result", "ui_result_display", 8),
}

COMPONENT_VISUAL_EVIDENCE = {
    "cmp_setting_toggle": {
        "states": ["on", "off"],
        "evidence": ["[UI][CMP][setting_toggle][on]", "[UI][CMP][setting_toggle][off]"],
    },
    "cmp_alert_row": {
        "states": ["warning", "danger"],
        "evidence": ["[UI][CMP][alert_row][warning]", "[UI][CMP][alert_row][danger]"],
    },
    "cmp_ble_candidate_row": {
        "states": ["default", "connecting", "failed"],
        "evidence": [
            "[UI][CMP][ble_candidate_row][default]",
            "[UI][STATE][ble_candidate][connecting]",
            "[UI][STATE][ble_candidate][failed]",
        ],
    },
    "cmp_selftest_result_row": {
        "states": ["pass", "fail", "skip"],
        "evidence": [
            "[UI][CMP][selftest_result_row][pass]",
            "[UI][PAGE][08-04][selftest_result]#ui_result_gps_state",
            "[UI][PAGE][08-04][selftest_result]#ui_result_audio_state",
        ],
    },
}

NON_INTERACTIVE_CONTROLS = {
    "selftest_gps_submit_loading": {"ui_btn_gps_outdoor", "ui_btn_gps_indoor", "ui_btn_back"},
}

REQUIRED_FACT_SNAPSHOT = [
    "binding_present",
    "ble_connected",
    "payment_state",
    "verify_in_flight",
    "payment_required",
    "payment_attempted",
    "selftest_state",
    "selftest_running",
    "alerts",
    "preserved_shell_slot",
]

FACT_ALIAS_NORMALIZATION = {
    "policy": "phase_3_binding_normalizes_aliases_and_authoritative_inputs_before_route_resolution_and_rejects_conflicts",
    "fields": [
        {"canonical": "binding_present", "aliases": ["has_binding", "is_bound"], "type": "bool"},
        {"canonical": "ble_connected", "aliases": ["is_ble_connected", "connected"], "type": "bool"},
        {"canonical": "payment_required", "aliases": ["pay_required", "payment_unverified"], "type": "bool"},
        {"canonical": "payment_attempted", "aliases": ["pay_attempted", "payment_verification_attempted"], "type": "bool"},
        {"canonical": "selftest_running", "aliases": ["selftest_in_progress", "diagnostic_running"], "type": "bool"},
        {"canonical": "alerts", "aliases": ["active_alerts", "alert_list"], "type": "array"},
        {"canonical": "preserved_shell_slot", "aliases": ["shell_slot", "origin_shell_slot"], "type": "enum:01|02|03|04"},
    ],
    "authoritative_inputs": [
        {
            "field": "payment_state",
            "type": "enum",
            "canonical_values": ["UNVERIFIED", "VERIFYING", "VERIFY_FAILED", "VERIFIED"],
            "input_normalization": {
                "UNVERIFIED": "UNVERIFIED",
                "VERIFYING": "VERIFYING",
                "VERIFY_FAILED": "VERIFY_FAILED",
                "VERIFIED": "VERIFIED",
                "unverified": "UNVERIFIED",
                "verifying": "VERIFYING",
                "verify_failed": "VERIFY_FAILED",
                "verified": "VERIFIED",
            },
        },
        {"field": "verify_in_flight", "type": "bool"},
        {
            "field": "selftest_state",
            "type": "enum",
            "canonical_values": ["IDLE", "CANCELLED", "RUNNING", "MANUAL", "FINISHED"],
            "input_normalization": {
                "IDLE": "IDLE",
                "CANCELLED": "CANCELLED",
                "RUNNING": "RUNNING",
                "MANUAL": "MANUAL",
                "AWAITING_MANUAL": "MANUAL",
                "FINISHED": "FINISHED",
                "idle": "IDLE",
                "cancelled": "CANCELLED",
                "running": "RUNNING",
                "running_auto": "RUNNING",
                "manual": "MANUAL",
                "awaiting_manual": "MANUAL",
                "finished": "FINISHED",
            },
        },
    ],
    "derived_fields": [
        {
            "canonical": "payment_required",
            "from": "payment_state",
            "value_map": {
                "UNVERIFIED": True,
                "VERIFYING": True,
                "VERIFY_FAILED": True,
                "VERIFIED": False,
            },
        },
        {
            "canonical": "payment_attempted",
            "from": "payment_state",
            "value_map": {
                "UNVERIFIED": False,
                "VERIFYING": True,
                "VERIFY_FAILED": True,
                "VERIFIED": True,
            },
        },
        {
            "canonical": "selftest_running",
            "from": "selftest_state",
            "value_map": {
                "IDLE": False,
                "CANCELLED": False,
                "RUNNING": True,
                "MANUAL": True,
                "FINISHED": False,
            },
        },
    ],
    "consistency_rules": [
        {
            "inputs": ["payment_state", "verify_in_flight"],
            "valid_pairs": [
                ["UNVERIFIED", False],
                ["VERIFYING", True],
                ["VERIFY_FAILED", False],
                ["VERIFIED", False],
            ],
        },
        {
            "inputs": ["canonical_alias", "derived_value"],
            "rule": "when_both_are_present_the_values_must_be_equal",
        },
    ],
    "missing_or_conflicting_value": "fact_snapshot_invalid_preserve_current_page_log_and_retry",
}

SELFTEST_LOCK_CONTROLS = {
    "control_connected": {
        "gear_minus": "ui_btn_gear_minus",
        "gear_plus": "ui_btn_gear_plus",
        "assist_toggle": "ui_toggle_assist",
    },
    "mode_default": {
        "mode_eco": "ui_btn_mode_eco",
        "mode_extreme": "ui_btn_mode_extreme",
        "mode_standard": "ui_btn_mode_standard",
        "mode_sport": "ui_btn_mode_sport",
        "poweroff": "ui_btn_exo_poweroff",
    },
}

SHELL_NAV_SOURCES = {
    "nav_slot_01": "ui_nav_home_button",
    "nav_slot_02": "ui_nav_control_button",
    "nav_slot_03": "ui_nav_mode_power_button",
    "nav_slot_04": "ui_nav_alerts_button",
}

MODEL_EXPLICIT_BINDINGS = {
    "ui_page_home_root": {
        "time": [("home_normal", "ui_txt_time"), ("home_unbound", "ui_txt_time"), ("home_disconnected", "ui_txt_time")],
        "watch_battery": [
            ("home_normal", "ui_rail_watch_battery_value"),
            ("home_unbound", "ui_rail_watch_battery_value"),
            ("home_disconnected", "ui_rail_watch_battery_value"),
        ],
        "exo_battery": [("home_normal", "ui_val_exo_battery")],
        "gear": [("home_normal", "ui_val_gear")],
        "steps": [("home_normal", "ui_val_steps")],
    },
    "ui_page_control_root": {
        "gear": [("control_connected", "ui_val_actual_gear")],
    },
    "ui_page_mode_power_root": {
        "drive_mode": [("mode_default", "ui_lbl_current_mode")],
    },
    "ui_page_device_info_root": {
        "firmware_version": [("device_info", "ui_row_firmware_value")],
    },
    "ui_page_maintenance_root": {
        "cellular_status": [("maintenance", "ui_row_cellular_value")],
        "gps_status": [("maintenance", "ui_row_gps_value")],
        "selftest_status": [("maintenance", "ui_row_selftest_value")],
    },
    "ui_page_selftest_progress_root": {
        "selftest_item": [
            ("selftest_progress", "ui_val_current_item"),
            ("selftest_retry_auto", "ui_val_current_item"),
            ("selftest_manual", "ui_val_manual_item"),
            ("selftest_manual_touch", "ui_val_manual_item"),
        ],
        "selftest_progress": [
            ("selftest_progress", "ui_val_progress"),
        ],
        "manual_prompt": [
            ("selftest_manual", "ui_txt_manual_question"),
            ("selftest_manual_touch", "ui_txt_manual_question"),
        ],
        "manual_countdown": [
            ("selftest_manual", "ui_val_manual_countdown"),
            ("selftest_manual_touch", "ui_val_manual_countdown"),
        ],
    },
    "ui_page_selftest_result_root": {
        "pass_count": [
            ("selftest_result", "ui_txt_result_pass_count"),
            ("selftest_result_all_passed", "ui_txt_result_pass_count"),
        ],
        "failure_count": [("selftest_result", "ui_txt_result_failure_count")],
        "skip_count": [
            ("selftest_result", "ui_txt_result_skip_count"),
            ("selftest_result_all_passed", "ui_txt_result_skip_count"),
        ],
    },
    "ui_page_selftest_retry_detail_root": {
        "selected_failed_item": [("selftest_retry_detail", "ui_val_retry_item")],
    },
}

MODEL_VALUE_FORMATS = {
    "binding_present": "bool",
    "ble_connected": "bool",
    "payment_required": "bool",
    "payment_attempted": "bool",
    "selftest_running": "bool",
    "assist_enabled": "bool",
    "power_action_enabled": "bool",
    "haptics_enabled": "bool",
    "click_audio_enabled": "bool",
    "raise_to_wake_enabled": "bool",
    "brightness_level": "enum:low|medium|high",
    "screen_timeout_seconds": "enum:5|15|30",
    "clear_binding_pending": "bool",
    "leg_positions_available": "bool",
    "exoskeleton_model": "watch_exoskeleton_model",
    "left_leg_position": "signed_degrees_or_placeholder",
    "right_leg_position": "signed_degrees_or_placeholder",
    "watch_battery": "percent_integer",
    "exo_battery": "percent_integer",
    "selftest_progress": "percent_integer",
    "time": "HH:mm",
    "alerts": "alert_collection",
    "ble_candidates": "ble_candidate_collection",
    "selftest_results": "selftest_result_collection",
    "failed_items": "selftest_failure_collection",
}

PATCH_ORDER = [
    "baseline_reset",
    "canonical_state_patch",
    "full_page_variant_patch",
    "selftest_lock_overlay",
    "model_projection",
]

ROOT_MODEL_FIELDS = {
    "ui_page_home_root": [
        "binding_present",
        "ble_connected",
        "time",
        "watch_battery",
        "exo_battery",
        "gear",
        "steps",
        "assist_enabled",
    ],
    "ui_page_control_root": ["ble_connected", "selftest_running", "gear", "assist_enabled"],
    "ui_page_mode_power_root": [
        "ble_connected",
        "selftest_running",
        "drive_mode",
        "power_action_enabled",
        "exoskeleton_model",
    ],
    "ui_page_alerts_root": ["payment_required", "payment_attempted", "alerts"],
    "ui_page_settings_root": [
        "haptics_enabled",
        "click_audio_enabled",
        "raise_to_wake_enabled",
        "brightness_level",
        "screen_timeout_seconds",
    ],
    "ui_page_device_info_root": [
        "device_model",
        "serial_number",
        "firmware_version",
        "binding_id",
        "leg_positions_available",
        "left_leg_position",
        "right_leg_position",
    ],
    "ui_page_maintenance_root": ["cellular_status", "gps_status", "selftest_status", "binding_present"],
    "ui_page_ble_root": ["binding_present", "ble_scan_state", "ble_candidates", "ble_connection_state"],
    "ui_page_selftest_progress_root": ["selftest_running", "selftest_item", "selftest_progress", "manual_prompt", "manual_countdown"],
    "ui_page_selftest_result_root": ["selftest_results", "pass_count", "failure_count", "skip_count"],
    "ui_page_selftest_gps_context_root": ["gps_context", "gps_submit_state"],
    "ui_page_selftest_retry_detail_root": ["failed_items", "selected_failed_item", "retry_state"],
    "ui_modal_clear_binding_root": ["binding_present", "clear_binding_pending"],
}

INTENT_GATE_MODEL_FIELDS = {
    ("ui_page_control_root", "selftest_running"):
        "controls_remain_hittable_and_intent_gate_rejects_during_selftest",
    ("ui_page_mode_power_root", "selftest_running"):
        "controls_remain_hittable_and_intent_gate_rejects_during_selftest",
    ("ui_page_mode_power_root", "power_action_enabled"):
        "poweroff_remains_hittable_and_intent_gate_rejects_when_unavailable",
}

ROOT_ROUTE_REFS = {
    "ui_page_home_root": ["route_shell_slot_01", "route_binding_entry", "route_ble_reconnect_success"],
    "ui_page_control_root": ["route_shell_slot_02", "route_selftest_lock_overlay"],
    "ui_page_mode_power_root": ["route_shell_slot_03", "route_selftest_lock_overlay"],
    "ui_page_alerts_root": ["route_shell_slot_04", "route_payment_lock", "route_payment_success"],
    "ui_page_settings_root": ["route_settings_children", "route_back_to_shell"],
    "ui_page_device_info_root": ["route_back_to_settings"],
    "ui_page_maintenance_root": ["route_maintenance_children", "route_back_to_settings"],
    "ui_page_ble_root": ["route_ble_flow", "route_binding_success", "route_back_to_shell"],
    "ui_page_selftest_progress_root": ["route_selftest_flow", "route_back_to_maintenance"],
    "ui_page_selftest_result_root": ["route_selftest_result", "route_back_to_maintenance"],
    "ui_page_selftest_gps_context_root": ["route_selftest_gps_context", "route_back_to_maintenance"],
    "ui_page_selftest_retry_detail_root": ["route_selftest_retry", "route_back_to_selftest_result"],
    "ui_modal_clear_binding_root": ["route_clear_binding_modal"],
}

ROUTE_DEFINITIONS = {
    "route_shell_slot_01": "resolve binding_present and ble_connected after a fresh fact snapshot",
    "route_shell_slot_02": "use 02-02 only for physical BLE disconnection; selftest lock keeps 02-01 disabled",
    "route_shell_slot_03": "use 03-02 only for physical BLE disconnection; selftest lock keeps 03-01 disabled",
    "route_shell_slot_04": "payment lock has priority over ordinary alerts within the main shell; full settings hierarchy remains accessible",
    "route_binding_entry": "open the BLE on-demand screen from an unbound home state",
    "route_ble_reconnect_success": "refresh all facts and preserve the current shell slot when allowed",
    "route_payment_success": "refresh all facts and resolve slot 01; never hardcode 01-01",
    "route_payment_lock": "force 04-03 or 04-04 only within the main shell while payment remains unresolved; preserve the full settings hierarchy",
    "route_selftest_lock_overlay": "disable gear, assist, all four mode controls and poweroff without changing connected page identity; preserve pager, swipe and page navigation",
    "route_settings_children": "open device info or maintenance as on-demand screens",
    "route_maintenance_children": "open selftest flow or create the clear-binding modal on demand",
    "route_ble_flow": "resolve empty, scanning, candidates, connecting and failure from BLE facts",
    "route_binding_success": "refresh all facts before resolving the destination",
    "route_selftest_flow": "resolve progress, retry and manual states from selftest facts",
    "route_selftest_result": "resolve completed result and retry entry from selftest facts",
    "route_selftest_gps_context": "resolve default, submitting and failed GPS context states",
    "route_selftest_retry": "retry only the selected failed item",
    "route_clear_binding_modal": "create modal on click; destroy it after cancel or completion",
    "route_back_to_shell": "return to the preserved shell slot through phase 3 binding",
    "route_back_to_settings": "release current temporary screen and return to settings",
    "route_back_to_maintenance": "release current temporary screen and return to maintenance",
    "route_back_to_selftest_result": "release retry detail and return to selftest result",
}

PALETTE = {
    "bg": "#000000",
    "text": "#FFFFFF",
    "focus": "#D7FF00",
    "success": "#30D158",
    "warning": "#FFD60A",
    "danger": "#FF453A",
    "cyan": "#64D2FF",
    "blue": "#0A84FF",
    "muted": "#8E98A7",
    "disabled": "#484D56",
    "divider": "#30343B",
    "surface": "#16181C",
    "surface_2": "#262A31",
}

# Pencil 中三条动作带都是上下边平行、左右边由画布裁切的斜切色块。
# LVGL 8 没有任意多边形面板，因此使用“透明裁切父面板 + 旋转实色子面板”复刻。
# angle 的单位是 0.1°；pivot 是相对实色子面板左上角的像素坐标。
SKEW_BANDS = {
    "ui_gear_motion_band": {"fill_height": 78, "angle": -42},
    "ui_bind_action_band": {"fill_height": 72, "angle": -36},
    "ui_reconnect_action_band": {"fill_height": 60, "angle": -30},
}

ASCII_DYNAMIC = "".join(chr(code) for code in range(0x20, 0x7F))
FONT_GLYPH_CONTRACT = "font_glyph_contract.json"


@dataclass
class HtmlNode:
    tag: str
    attrs: dict[str, str]
    children: list["HtmlNode"] = field(default_factory=list)
    direct_text: list[str] = field(default_factory=list)
    raw_children: list[tuple[str, dict[str, str]]] = field(default_factory=list)

    @property
    def node_id(self) -> str:
        return self.attrs.get("data-pencil-id", "")

    @property
    def name(self) -> str:
        return self.attrs.get("data-pencil-name", "")


class PencilHtmlParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.roots: list[HtmlNode] = []
        self.stack: list[HtmlNode] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attr_map = {key: value or "" for key, value in attrs}
        if "data-pencil-id" in attr_map or "data-pencil-name" in attr_map:
            node = HtmlNode(tag=tag, attrs=attr_map)
            if self.stack:
                self.stack[-1].children.append(node)
            else:
                self.roots.append(node)
            self.stack.append(node)
        elif self.stack:
            self.stack[-1].raw_children.append((tag, attr_map))
            if tag == "br":
                self.stack[-1].direct_text.append("\n")

    def handle_endtag(self, tag: str) -> None:
        if self.stack and self.stack[-1].tag == tag:
            self.stack.pop()

    def handle_data(self, data: str) -> None:
        if self.stack and data.strip():
            self.stack[-1].direct_text.append(data.strip())


def parse_style(style: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for item in style.split(";"):
        if ":" in item:
            key, value = item.split(":", 1)
            result[key.strip()] = value.strip()
    return result


def parse_tailwind_style(class_text: str) -> dict[str, str]:
    """解析 watch-lvgl.html 实际使用到的 Tailwind 子集。"""
    result: dict[str, str] = {}
    for token in class_text.split():
        arbitrary = re.fullmatch(r"(w|h|left|top)-\[(-?[0-9.]+px)\]", token)
        if arbitrary:
            key = {"w": "width", "h": "height"}.get(arbitrary.group(1), arbitrary.group(1))
            result[key] = arbitrary.group(2)
            continue
        if token in {"left-0", "top-0"}:
            result[token.removesuffix("-0")] = "0px"
            continue
        color = re.fullmatch(r"(bg|text)-\[(#[0-9A-Fa-f]{6,8})\]", token)
        if color:
            result["background-color" if color.group(1) == "bg" else "color"] = color.group(2)
            continue
        text_size = re.fullmatch(r"text-\[([0-9.]+px)\]/\[([^]]+)\]", token)
        if text_size:
            result["font-size"] = text_size.group(1)
            result["line-height"] = text_size.group(2)
            continue
        font_family = re.fullmatch(r"font-\[(.+)\]", token)
        if font_family:
            result["font-family"] = font_family.group(1).replace("_", " ").replace("'", "")
            continue
        if token in {"font-medium", "font-semibold", "font-bold"}:
            result["font-weight"] = {"font-medium": "500", "font-semibold": "600", "font-bold": "700"}[token]
            continue
        if token in {"text-left", "text-center", "text-right"}:
            result["text-align"] = token.removeprefix("text-")
            continue
        tracking = re.fullmatch(r"tracking-\[(-?[0-9.]+px)\]", token)
        if tracking:
            result["letter-spacing"] = tracking.group(1)
            continue
        border = re.fullmatch(r"\[border:([0-9.]+px)_solid_(#[0-9A-Fa-f]{6,8})\]", token)
        if border:
            result["border"] = f"{border.group(1)} solid {border.group(2)}"
            continue
        radius = re.fullmatch(r"rounded-\[([0-9.]+px)\]", token)
        if radius:
            result["border-radius"] = radius.group(1)
            continue
        if token == "rounded-full":
            result["border-radius"] = "50%"
            continue
        if token == "overflow-hidden":
            result["overflow"] = "hidden"
            continue
        if token == "overflow-visible":
            result["overflow"] = "visible"
            continue
        if token == "flex":
            result["display"] = "flex"
            continue
        if token == "flex-col":
            result["flex-direction"] = "column"
            continue
        if token == "justify-center":
            result["justify-content"] = "center"
            continue
        clip_path = re.fullmatch(r"\[clip-path:(.+)\]", token)
        if clip_path:
            result["clip-path"] = clip_path.group(1)
    return result


def node_style(node: HtmlNode) -> dict[str, str]:
    style = parse_tailwind_style(node.attrs.get("class", ""))
    style.update(parse_style(node.attrs.get("style", "")))
    return style


def normalized_text(node: HtmlNode) -> str:
    parts: list[str] = []
    for chunk in node.direct_text:
        if chunk == "\n":
            if parts and parts[-1] != "\\n":
                parts.append("\\n")
            continue
        collapsed = re.sub(r"\s+", " ", chunk).strip()
        if not collapsed:
            continue
        if parts and parts[-1] != "\\n":
            parts.append(" ")
        parts.append(collapsed)
    return "".join(parts).strip()


def round_half_away(value: float) -> int:
    return math.floor(value + 0.5) if value >= 0 else math.ceil(value - 0.5)


def px(value: str | None, default: float = 0) -> int:
    if not value:
        return round_half_away(default)
    match = re.search(r"-?[0-9]+(?:\.[0-9]+)?", value)
    return round_half_away(float(match.group(0))) if match else round_half_away(default)


def rgba(value: str | None, fallback: str = "#00000000") -> list[int]:
    text = (value or fallback).strip()
    if text.startswith("rgba"):
        nums = re.findall(r"[0-9.]+", text)
        alpha = round(float(nums[3]) * 255) if len(nums) > 3 else 255
        return [int(nums[0]), int(nums[1]), int(nums[2]), alpha]
    if re.fullmatch(r"#[0-9A-Fa-f]{3}", text):
        text = "#" + "".join(ch * 2 for ch in text[1:])
    if re.fullmatch(r"#[0-9A-Fa-f]{8}", text):
        return [int(text[1:3], 16), int(text[3:5], 16), int(text[5:7], 16), int(text[7:9], 16)]
    if re.fullmatch(r"#[0-9A-Fa-f]{6}", text):
        return [int(text[1:3], 16), int(text[3:5], 16), int(text[5:7], 16), 255]
    return rgba(fallback)


def signed_hash(text: str) -> int:
    value = int(hashlib.sha1(text.encode("utf-8")).hexdigest()[:8], 16)
    return value - 2**32 if value >= 2**31 else value


def legacy_dotnet_string_hash(text: str) -> int:
    """Match the deterministic string hash used by SquareLine's Mono runtime."""
    hash1 = 5381
    hash2 = 5381
    index = 0
    while index < len(text):
        hash1 = (((hash1 << 5) + hash1) ^ ord(text[index])) & 0xFFFFFFFF
        if index == len(text) - 1:
            break
        hash2 = (((hash2 << 5) + hash2) ^ ord(text[index + 1])) & 0xFFFFFFFF
        index += 2
    value = (hash1 + hash2 * 1566083941) & 0xFFFFFFFF
    return value - 2**32 if value >= 2**31 else value


def guid(text: str) -> str:
    # SquareLine's proprietary GUID strings are decimal-only (besides GUID/-/S).
    # Keeping the native shape matters for its component registry even though the
    # project serializer otherwise treats the value as an opaque string.
    digits = str(int(hashlib.sha1(text.encode("utf-8")).hexdigest(), 16)).zfill(48)
    return f"GUID{digits[:8]}-{digits[8:14]}S{digits[14:22]}"


def walk_project_nodes(node: dict):
    yield node
    for child in node.get("children", []):
        yield from walk_project_nodes(child)


class ProjectBuilder:
    def __init__(self, project_dir: Path, html_path: Path, lucide_map: dict[str, int]) -> None:
        self.project_dir = project_dir
        self.html_path = html_path
        self.lucene = lucide_map
        self.nid_counter = 1_100_000
        self.name_counts: dict[str, defaultdict[str, int]] = defaultdict(lambda: defaultdict(int))
        self.font_symbols: dict[tuple[str, int, int], set[str]] = defaultdict(set)
        self.icon_symbols: dict[int, set[str]] = defaultdict(set)
        self.font_metric_cache: dict[str, tuple[int, int]] = {}
        self.state_patches: dict[str, dict] = {}
        self.state_events: dict[str, list[str]] = defaultdict(list)
        self.state_component_refs: dict[str, list[str]] = defaultdict(list)
        self.component_structure_signatures: dict[str, str] = {}
        self.state_pages = self._read_pages()
        self.mode_title_reserve = self._read_mode_title_reserve()
        self.settings_page_2_state = self._read_settings_page_2_state()
        self._normalize_settings_contract()
        self.concrete_routes = self._read_concrete_routes()
        self.pages = {STATE_SOURCE_IDS[state]: page for state, page in self.state_pages.items()}

    def nid(self) -> int:
        self.nid_counter += 1
        return self.nid_counter

    @staticmethod
    def structured_source_name(name: str) -> tuple[str, str, str] | None:
        match = re.match(r"^\[UI\]\[(PAGE|VAR)\]\[([0-9]{2}-[0-9]{2})\]\[([a-z0-9_]+)\]", name)
        return match.groups() if match else None

    def _read_pages(self) -> dict[str, HtmlNode]:
        parser = PencilHtmlParser()
        parser.feed(self.html_path.read_text(encoding="utf-8"))
        found: dict[str, HtmlNode] = {}
        expected = set(STATE_SOURCE_IDS)

        def visit(node: HtmlNode) -> None:
            explicit_state = PAGE_STATES.get(node.node_id) or VARIANT_STATES.get(node.node_id)
            structured = self.structured_source_name(node.name)
            structured_state = structured[2] if structured and "· Surface" not in node.name else None
            state = explicit_state or structured_state
            if state in expected:
                expected_kind = "PAGE" if state in PAGE_STATES.values() else "VAR"
                if structured is None:
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "missing_structured_source_identity",
                                "state": state,
                                "hint": "页面/变体必须保留 [UI][PAGE|VAR][页码][状态键] 结构键。",
                            },
                            ensure_ascii=False,
                        )
                    )
                if structured_state != state:
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "source_state_mismatch",
                                "expected": state,
                                "actual": structured_state,
                            },
                            ensure_ascii=False,
                        )
                    )
                if expected_kind != structured[0]:
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "source_kind_mismatch",
                                "state": state,
                                "expected": expected_kind,
                                "actual": structured[0],
                            },
                            ensure_ascii=False,
                        )
                    )
                if STATE_CODES[state] != structured[1]:
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "source_code_mismatch",
                                "state": state,
                                "expected": STATE_CODES[state],
                                "actual": structured[1],
                            },
                            ensure_ascii=False,
                        )
                    )
                if node.node_id and node.node_id != STATE_SOURCE_IDS[state]:
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "source_pencil_id_mismatch",
                                "state": state,
                                "expected": STATE_SOURCE_IDS[state],
                                "actual": node.node_id,
                            },
                            ensure_ascii=False,
                        )
                    )
                if state in found:
                    raise RuntimeError(
                        json.dumps({"code": "duplicate_source", "state": state}, ensure_ascii=False)
                    )
                found[state] = node
            for child in node.children:
                visit(child)

        for root in parser.roots:
            visit(root)
        missing = expected - set(found)
        if missing:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "missing_structured_sources",
                        "states": sorted(missing),
                        "hint": "HTML 必须保留 [UI][PAGE|VAR][页码][状态键] 的结构化名称，中文标题不参与匹配。",
                    },
                    ensure_ascii=False,
                )
            )
        return found

    def _read_mode_title_reserve(self) -> HtmlNode:
        """从非导出四模式参考稿保留非 Old 型号所需的稳定标题对象。"""
        parser = PencilHtmlParser()
        parser.feed(self.html_path.read_text(encoding="utf-8"))
        matches: list[HtmlNode] = []

        def visit(node: HtmlNode) -> None:
            if node.name == MODE_FOUR_LAYOUT_STATE_NAME:
                matches.append(node)
            for child in node.children:
                visit(child)

        for root in parser.roots:
            visit(root)
        if len(matches) != 1:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "mode_title_reserve_state_mismatch",
                        "expected_name": MODE_FOUR_LAYOUT_STATE_NAME,
                        "matches": len(matches),
                    },
                    ensure_ascii=False,
                )
            )
        title = self._find_html(matches[0], MODE_TITLE_RESERVE_NAME)
        if title is None:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "mode_title_reserve_object_missing",
                        "state": MODE_FOUR_LAYOUT_STATE_NAME,
                        "source_object": MODE_TITLE_RESERVE_NAME,
                    },
                    ensure_ascii=False,
                )
            )
        reserve = copy.deepcopy(title)
        reserve.attrs["style"] = (
            f"{reserve.attrs.get('style', '')};left:{MODE_TITLE_RESERVE_X}px"
        )
        return reserve

    def _read_settings_page_2_state(self) -> HtmlNode:
        parser = PencilHtmlParser()
        parser.feed(self.html_path.read_text(encoding="utf-8"))
        matches: list[HtmlNode] = []

        def visit(node: HtmlNode) -> None:
            if node.name == SETTINGS_PAGE_2_STATE_NAME:
                matches.append(node)
            for child in node.children:
                visit(child)

        for root in parser.roots:
            visit(root)
        if len(matches) != 1:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "settings_page_2_state_mismatch",
                        "expected_name": SETTINGS_PAGE_2_STATE_NAME,
                        "matches": len(matches),
                    },
                    ensure_ascii=False,
                )
            )
        state = matches[0]
        canonical = self.state_pages["settings"]
        canonical_nodes = list(self._walk_html_nodes(canonical))
        state_nodes = list(self._walk_html_nodes(state))
        canonical_names = sorted(node.name for node in canonical_nodes[1:] if node.name)
        state_names = sorted(node.name for node in state_nodes[1:] if node.name)
        if state_names != canonical_names:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "settings_page_2_structure_mismatch",
                        "message": "第二页可见状态必须复用设置页完整对象树。",
                    },
                    ensure_ascii=False,
                )
            )
        content = self._find_html(state, "ui_settings_list_content")
        thumb = self._find_html(state, "ui_settings_page_thumb")
        required = {
            "ui_row_device_info",
            "ui_row_maintenance",
            "ui_gesture_zone_settings_return",
            "ui_hint_pull_up_handle",
        }
        actual = {node.name for node in self._walk_html_nodes(state)}
        if (
            content is None
            or thumb is None
            or px(node_style(content).get("top")) != SETTINGS_PAGE_2_CONTENT_OFFSET_Y
            or px(node_style(thumb).get("top")) != SETTINGS_PAGE_2_THUMB_Y
            or not required.issubset(actual)
        ):
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "settings_page_2_visual_state_invalid",
                        "content_y": None if content is None else px(node_style(content).get("top")),
                        "thumb_y": None if thumb is None else px(node_style(thumb).get("top")),
                        "missing": sorted(required - actual),
                    },
                    ensure_ascii=False,
                )
            )
        return state

    def _normalize_settings_contract(self) -> None:
        """保留已发布的六栏设置合同，避免旧 Pencil 缺项污染导出。"""
        for root in (self.state_pages["settings"], self.settings_page_2_state):
            if self._find_html(root, SETTINGS_RAISE_WAKE_NAME) is not None:
                continue
            source = self._find_html(root, SETTINGS_RAISE_WAKE_SOURCE_NAME)
            page_1 = self._find_html(root, "ui_settings_list_page_1")
            page_2 = self._find_html(root, "ui_settings_list_page_2")
            brightness = self._find_html(root, "ui_row_brightness")
            timeout = self._find_html(root, "ui_row_screen_timeout")
            device_info = self._find_html(root, "ui_row_device_info")
            maintenance = self._find_html(root, "ui_row_maintenance")
            required = {
                "source": source,
                "page_1": page_1,
                "page_2": page_2,
                "brightness": brightness,
                "timeout": timeout,
                "device_info": device_info,
                "maintenance": maintenance,
            }
            missing = sorted(name for name, node in required.items() if node is None)
            if missing:
                raise RuntimeError(
                    json.dumps(
                        {
                            "code": "settings_contract_reserve_missing",
                            "missing": missing,
                        },
                        ensure_ascii=False,
                    )
                )

            raise_wake = copy.deepcopy(source)
            raise_wake.attrs["data-pencil-id"] = "synthetic-settings-raise-wake"
            raise_wake.attrs["data-pencil-name"] = SETTINGS_RAISE_WAKE_NAME
            self._append_style(raise_wake, top="176px")
            icon = self._find_html(raise_wake, "component_setting_icon")
            label = self._find_html(raise_wake, "component_setting_label")
            value = self._find_html(raise_wake, "component_setting_value")
            if icon is None or label is None or value is None:
                raise RuntimeError(
                    json.dumps(
                        {"code": "settings_raise_wake_children_missing"},
                        ensure_ascii=False,
                    )
                )
            icon.attrs["data-icon-name"] = "eye"
            label.direct_text = ["抬腕亮屏"]
            value.direct_text = ["开启"]

            self._remove_html_child(root, timeout)
            brightness_index = page_1.children.index(brightness)
            page_1.children.insert(brightness_index, raise_wake)
            page_2.children.insert(0, timeout)
            self._append_style(brightness, top="264px")
            self._append_style(timeout, left="0px", top="0px")
            self._append_style(device_info, left="0px", top="88px")
            self._append_style(maintenance, left="0px", top="176px")
            self._append_style(page_2, height="264px")

    @staticmethod
    def _append_style(node: HtmlNode, **values: str) -> None:
        suffix = ";".join(f"{name}:{value}" for name, value in values.items())
        node.attrs["style"] = f"{node.attrs.get('style', '')};{suffix}"

    @staticmethod
    def _remove_html_child(root: HtmlNode, target: HtmlNode) -> bool:
        for index, child in enumerate(root.children):
            if child is target:
                del root.children[index]
                return True
            if ProjectBuilder._remove_html_child(child, target):
                return True
        return False

    @staticmethod
    def _walk_html_nodes(node: HtmlNode):
        yield node
        for child in node.children:
            yield from ProjectBuilder._walk_html_nodes(child)

    def _read_concrete_routes(self) -> list[dict[str, str]]:
        parser = PencilHtmlParser()
        parser.feed(self.html_path.read_text(encoding="utf-8"))
        route_card: HtmlNode | None = None

        def visit(node: HtmlNode) -> None:
            nonlocal route_card
            if node.node_id == "yEaCt" or node.name == "[SPEC][ROUTE][global] 全局页面切换规则":
                route_card = node
            for child in node.children:
                visit(child)

        for root in parser.roots:
            visit(root)
        if route_card is None:
            raise RuntimeError(
                json.dumps({"code": "missing_global_route_card", "pencil_node_id": "yEaCt"}, ensure_ascii=False)
            )

        routes: list[dict[str, str]] = []
        for section in route_card.children:
            for line in normalized_text(section).split("\\n"):
                if not line.startswith("route_"):
                    continue
                fields = [field.strip() for field in line.split("｜")]
                if len(fields) != 7:
                    raise RuntimeError(
                        json.dumps(
                            {"code": "invalid_concrete_route", "line": line, "field_count": len(fields)},
                            ensure_ascii=False,
                        )
                    )
                routes.append(
                    dict(
                        zip(
                            ["route_id", "from", "event", "guard", "to", "fallback", "history_policy"],
                            fields,
                            strict=True,
                        )
                    )
                )
        route_ids = [route["route_id"] for route in routes]
        if len(routes) != 41 or len(set(route_ids)) != 41:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "concrete_route_count_mismatch",
                        "expected": 41,
                        "actual": len(routes),
                        "duplicates": sorted({route_id for route_id in route_ids if route_ids.count(route_id) > 1}),
                    },
                    ensure_ascii=False,
                )
            )
        return routes

    def font_metrics(self, code: str, fallback_size: int) -> tuple[int, int]:
        if code in self.font_metric_cache:
            return self.font_metric_cache[code]
        path = self.project_dir / "assets" / "Fonts" / f"ui_font_{code}.c"
        line_height = max(1, round_half_away(fallback_size * 1.2))
        base_line = 0
        if path.exists():
            source = path.read_text(encoding="utf-8", errors="ignore")
            line_match = re.search(r"\.line_height\s*=\s*(-?[0-9]+)", source)
            base_match = re.search(r"\.base_line\s*=\s*(-?[0-9]+)", source)
            if line_match:
                line_height = int(line_match.group(1))
            if base_match:
                base_line = int(base_match.group(1))
        self.font_metric_cache[code] = (line_height, base_line)
        return line_height, base_line

    def prop(
        self,
        strtype: str,
        *,
        strval: str | None = None,
        intarray: list[int] | None = None,
        integer: int | None = None,
        flags: int | None = None,
        childs: list[dict] | None = None,
        inherited: int = 1,
        **extra,
    ) -> dict:
        item = {"nid": self.nid(), "strtype": strtype}
        if strval is not None:
            item["strval"] = strval
        if intarray is not None:
            item["intarray"] = intarray
        if integer is not None:
            item["integer"] = integer
        if flags is not None:
            item["flags"] = flags
        if childs is not None:
            item["childs"] = childs
        item.update(extra)
        item["InheritedType"] = inherited
        return item

    def layout_props(self, name: str, x: int, y: int, w: int, h: int, clickable: bool, scrollable: bool) -> list[dict]:
        gesture_bubble = not clickable or name in {
            "cmp_setting_toggle",
            "ui_row_haptics_instance",
            "ui_row_click_audio_instance",
            "ui_row_raise_wake_instance",
            "ui_settings_row_haptics_instance",
            "ui_settings_row_click_audio_instance",
            "ui_settings_row_raise_wake_instance",
            "ui_settings_row_device_info",
            "ui_settings_row_maintenance",
        }
        if name == "ui_settings_settings_list_viewport":
            gesture_bubble = False
        return [
            self.prop("OBJECT/Name", strval=name, inherited=10),
            self.prop("OBJECT/Layout"),
            self.prop(
                "OBJECT/Layout_type",
                strval="No_layout",
                inherited=13,
                Flow=0,
                Wrap=False,
                Reversed=False,
                MainAlignment=0,
                CrossAlignment=0,
                TrackAlignment=0,
                LayoutType=0,
            ),
            self.prop("OBJECT/Transform"),
            self.prop("OBJECT/Position", intarray=[x, y], flags=17, inherited=7),
            # 0x11 is SquareLine's absolute-pixel unit pair for width and height.
            # 0x12 makes one axis use a different sizing mode and severely distorts
            # fixed 410x502 Pencil geometry when Studio rebuilds the component.
            self.prop("OBJECT/Size", intarray=[max(1, w), max(1, h)], flags=17, inherited=7),
            self.prop("OBJECT/Align", strval="TOP_LEFT", inherited=3),
            self.prop("OBJECT/Flags", flags=1_048_576),
            self.prop("OBJECT/Hidden", strval="False", inherited=2),
            self.prop("OBJECT/Clickable", strval="True" if clickable else "False", inherited=2),
            self.prop("OBJECT/Checkable", strval="False", inherited=2),
            self.prop("OBJECT/Press_lock", strval="True", inherited=2),
            self.prop("OBJECT/Click_focusable", strval="True" if clickable else "False", inherited=2),
            self.prop("OBJECT/Adv_hittest", strval="False", inherited=2),
            self.prop("OBJECT/Ignore_layout", strval="False", inherited=2),
            self.prop("OBJECT/Floating", strval="False", inherited=2),
            self.prop("OBJECT/Flex_in_new_track", strval="False", inherited=2),
            self.prop("OBJECT/Event_bubble", strval="False", inherited=2),
            self.prop("OBJECT/Gesture_bubble", strval="True" if gesture_bubble else "False", inherited=2),
            self.prop("OBJECT/Snappable", strval="False", inherited=2),
            self.prop("OBJECT/Scrolling", flags=1_048_576),
            self.prop("OBJECT/Scrollable", strval="True" if scrollable else "False", inherited=2),
            self.prop("OBJECT/Scroll_elastic", strval="True", inherited=2),
            self.prop("OBJECT/Scroll_momentum", strval="True", inherited=2),
            self.prop("OBJECT/Scroll_on_focus", strval="False", inherited=2),
            self.prop("OBJECT/Scroll_chain", strval="False", inherited=2),
            self.prop("OBJECT/Scroll_one", strval="False", inherited=2),
            self.prop("OBJECT/Scrollbar_mode", strval="OFF" if not scrollable else "AUTO", inherited=3),
            self.prop("OBJECT/Scroll_direction", strval="VER" if scrollable else "ALL", inherited=3),
            self.prop("OBJECT/Scroll_snap_x", strval="NONE", inherited=3),
            self.prop("OBJECT/Scroll_snap_y", strval="NONE", inherited=3),
            self.prop("OBJECT/States", flags=1_048_576),
            self.prop("OBJECT/Checked", strval="False", inherited=2),
            self.prop("OBJECT/Disabled", strval="False", inherited=2),
            self.prop("OBJECT/Focused", strval="False", inherited=2),
            self.prop("OBJECT/Pressed", strval="False", inherited=2),
        ]

    def style_state(self, children: list[dict], state: str = "DEFAULT") -> dict:
        return self.prop("_style/StyleState", strval=state, childs=children)

    def rect_style(self, style: dict[str, str], w: int, h: int, *, force_bg: str | None = None, clip: bool = False) -> list[dict]:
        bg = force_bg or style.get("background-color", "#00000000")
        radius_text = style.get("border-radius", "0")
        radius = min(w, h) // 2 if "%" in radius_text else px(radius_text)
        children = [
            self.prop("_style/Bg_Color", intarray=rgba(bg), inherited=7),
            self.prop("_style/Bg_Radius", integer=max(0, radius), inherited=6),
            self.prop("_style/Clip_corner", strval="True" if clip or radius > 0 else "False", inherited=2),
            self.prop("_style/Padding", intarray=[0, 0, 0, 0], inherited=7),
            self.prop("_style/Padding_RowCol", intarray=[0, 0], inherited=7),
        ]
        border = style.get("border", "")
        match = re.search(r"([0-9.]+)px\s+\w+\s+(#[0-9A-Fa-f]{6,8})", border)
        if match:
            children.extend(
                [
                    self.prop("_style/Border_Color", intarray=rgba(match.group(2)), inherited=7),
                    self.prop("_style/Border width", integer=round(float(match.group(1))), inherited=6),
                    self.prop("_style/Border side", strval="FULL", inherited=3),
                ]
            )
        return children

    def call_function_action(self, function_name: str) -> dict:
        return self.prop(
            "_event/action",
            strval="CALL FUNCTION",
            inherited=10,
            childs=[
                self.prop("CALL FUNCTION/Name", strval="CALL FUNCTION", inherited=10),
                self.prop("CALL FUNCTION/Call", strval="<{Function_name}>( event_struct )", inherited=10),
                self.prop("CALL FUNCTION/CallC", strval="<{Function_name}>( e );", inherited=10),
                self.prop("CALL FUNCTION/Function_name", strval=function_name, inherited=10),
                self.prop("CALL FUNCTION/Dont_export_function", strval="False", inherited=2),
            ],
        )

    def event_prop(
        self,
        event_name: str,
        event_code: str,
        function_name: str,
    ) -> dict:
        actions = [self.call_function_action(function_name)]
        return self.prop(
            "_event/EventHandler",
            strval=event_code,
            inherited=4,
            disabled=False,
            childs=[
                self.prop("_custom/name", strval=event_name, inherited=10),
                self.prop("_custom/condition_C", strval="", inherited=10),
                self.prop("_custom/condition_P", strval="", inherited=10),
                *actions,
            ],
        )

    @staticmethod
    def is_clickable(original_name: str, state: str) -> bool:
        name = original_name.lower()
        if original_name in NON_INTERACTIVE_CONTROLS.get(state, set()):
            return False
        if "disabled" in name or state == "alerts_readonly":
            return False
        if state == "device_info" and name.startswith("ui_row_"):
            return False
        if state == "maintenance" and name == "ui_row_gps":
            return False
        if state == "settings":
            if name in {"ui_row_brightness", "ui_row_screen_timeout"}:
                return False
            if name == "ui_gesture_zone_settings_return":
                return True
        return (
            name.startswith("ui_btn_")
            or (name.startswith("ui_nav_") and name.endswith("_button"))
            or name.startswith("ui_row_")
            or name.startswith("ui_toggle_")
            or name.startswith("ui_device_candidate_") and not any(name.endswith(s) for s in ("_icon", "_name", "_mac", "_rssi", "_arrow"))
            or name.startswith("ui_alert_") and name.endswith("_row")
        )

    def unique_name(self, state: str, original_name: str) -> str:
        suffix = re.sub(r"[^a-z0-9_]+", "_", original_name.lower()).strip("_")
        suffix = re.sub(r"^ui_", "", suffix)
        base = f"ui_{state}_{suffix or 'object'}"
        self.name_counts[state][base] += 1
        count = self.name_counts[state][base]
        return base if count == 1 else f"{base}_{count}"

    def font_combo(self, style: dict[str, str]) -> tuple[str, int, int, str]:
        family = "noto" if "Noto Sans SC" in style.get("font-family", "") else "montserrat"
        weight = int(style.get("font-weight", "500"))
        size = px(style.get("font-size", "16px"))
        if style.get("font-size") == "15.5px":
            size = 16
        code = f"ns{weight}_{size}" if family == "noto" else f"mo{weight}_{size}"
        return family, weight, size, code

    def _normalize_lvgl_geometry(
        self,
        source: HtmlNode,
        style: dict[str, str],
        x: int,
        y: int,
        w: int,
        h: int,
        font_code: str | None,
        fallback_size: int,
    ) -> tuple[int, int, int, int]:
        is_lucide_icon = source.tag == "svg" and bool(source.attrs.get("data-icon-name"))
        is_flex_centered_text = (
            source.tag == "div"
            and "font-family" in style
            and "\\n" not in normalized_text(source)
            and style.get("display") == "flex"
            and style.get("flex-direction") == "column"
            and style.get("justify-content") == "center"
        )
        if source.name == "ui_icon_assist_state":
            return x, y, w, h
        if font_code is None or not (is_lucide_icon or is_flex_centered_text):
            return x, y, w, h
        line_height, _base_line = self.font_metrics(font_code, fallback_size)
        return x, y + round_half_away((h - line_height) / 2), w, line_height

    def make_label(self, state: str, html: HtmlNode, name: str, style: dict[str, str], x: int, y: int, w: int, h: int) -> dict:
        text = normalized_text(html)
        family, weight, size, font_code = self.font_combo(style)
        self.font_symbols[(family, weight, size)].update(text)
        self.font_symbols[(family, weight, size)].update(ASCII_DYNAMIC)
        align = {"center": "CENTER", "right": "RIGHT"}.get(style.get("text-align", "left"), "LEFT")
        line_height, _base_line = self.font_metrics(font_code, size)
        visual_x, visual_y, visual_w, visual_h = self._normalize_lvgl_geometry(
            html, style, x, y, w, h, font_code, size
        )
        css_line_height = style.get("line-height", "normal")
        line_spacing = 0 if css_line_height == "normal" else max(0, px(css_line_height) - line_height)
        props = self.layout_props(name, visual_x, visual_y, visual_w, visual_h, clickable=False, scrollable=False)
        props.extend(
            [
                self.prop("LABEL/Overflow_visible", strval="False", inherited=2),
                self.prop("LABEL/Scroll_with_arrow", strval="False", inherited=2),
                self.prop("LABEL/Edited", strval="False", inherited=2),
                self.prop("LABEL/User_1", strval="False", inherited=2),
                self.prop("LABEL/User_2", strval="False", inherited=2),
                self.prop("LABEL/User_3", strval="False", inherited=2),
                self.prop("LABEL/User_4", strval="False", inherited=2),
                self.prop("LABEL/Label"),
                self.prop(
                    "LABEL/Long_mode",
                    strval=(
                        "CLIP"
                        if style.get("overflow") == "hidden"
                        or html.name in {
                            "ui_row_watch_id_value",
                            "ui_row_bound_mac_value",
                        }
                        else "WRAP"
                    ),
                    inherited=3,
                ),
                self.prop("LABEL/Text", strval=text, inherited=10),
                self.prop(
                    "LABEL/Style_main",
                    strval="lv.PART.MAIN, Text, Rectangle, Pad, Transform",
                    inherited=11,
                    part="lv.PART.MAIN",
                    childs=[
                        self.style_state(
                            [
                                self.prop("_style/Text_Color", intarray=rgba(style.get("color", "#FFFFFF")), inherited=7),
                                self.prop("_style/Text_Font", strval=font_code, inherited=3),
                                self.prop("_style/Text_Align", strval=align, inherited=3),
                                self.prop(
                                    "_style/Text_Spacing",
                                    intarray=[px(style.get("letter-spacing", "0")), line_spacing],
                                    inherited=7,
                                ),
                            ]
                        )
                    ],
                ),
            ]
        )
        return self.node(name, "LABEL", props, [], f"{state}:{html.node_id or html.name}")

    def make_icon(self, state: str, html: HtmlNode, name: str, style: dict[str, str], x: int, y: int, w: int, h: int) -> dict:
        icon_name = html.attrs["data-icon-name"]
        codepoint = self.lucene.get(icon_name)
        if codepoint is None:
            raise RuntimeError(f"Lucide 缺少图标：{icon_name}")
        size = max(w, h)
        glyph = chr(codepoint)
        self.icon_symbols[size].add(glyph)
        fill = "#FFFFFF"
        for tag, attrs in html.raw_children:
            if tag == "path" and attrs.get("fill"):
                fill = attrs["fill"]
                break
        visual_x, visual_y, visual_w, visual_h = self._normalize_lvgl_geometry(
            html, style, x, y, w, h, f"lucide_{size}", size
        )
        props = self.layout_props(name, visual_x, visual_y, visual_w, visual_h, clickable=False, scrollable=False)
        props.extend(
            [
                self.prop("LABEL/Overflow_visible", strval="False", inherited=2),
                self.prop("LABEL/Scroll_with_arrow", strval="False", inherited=2),
                self.prop("LABEL/Edited", strval="False", inherited=2),
                self.prop("LABEL/User_1", strval="False", inherited=2),
                self.prop("LABEL/User_2", strval="False", inherited=2),
                self.prop("LABEL/User_3", strval="False", inherited=2),
                self.prop("LABEL/User_4", strval="False", inherited=2),
                self.prop("LABEL/Label"),
                self.prop("LABEL/Long_mode", strval="CLIP", inherited=3),
                self.prop("LABEL/Text", strval=glyph, inherited=10),
                self.prop(
                    "LABEL/Style_main",
                    strval="lv.PART.MAIN, Text, Rectangle, Pad, Transform",
                    inherited=11,
                    part="lv.PART.MAIN",
                    childs=[
                        self.style_state(
                            [
                                self.prop("_style/Text_Color", intarray=rgba(fill), inherited=7),
                                self.prop("_style/Text_Font", strval=f"lucide_{size}", inherited=3),
                                self.prop("_style/Text_Align", strval="CENTER", inherited=3),
                            ]
                        )
                    ],
                ),
            ]
        )
        return self.node(name, "LABEL", props, [], f"{state}:{html.node_id or html.name}")

    def make_rect(self, state: str, html: HtmlNode, name: str, style: dict[str, str], x: int, y: int, w: int, h: int, children: list[dict]) -> dict:
        if html.name in SKEW_BANDS:
            return self.make_skew_band(state, html, name, style, x, y, w, h)

        if html.name == "ui_settings_list_content":
            h = 608
        elif html.name == "ui_settings_list_page_2":
            h = 264

        clickable = self.is_clickable(html.name, state)
        objtype = "BUTTON" if clickable else "PANEL"
        disabled = (
            "disabled" in html.name.lower()
            or state == "alerts_readonly" and objtype == "BUTTON"
            or html.name in NON_INTERACTIVE_CONTROLS.get(state, set())
        )
        scrollable = name in {
            "ui_selftest_result_result_list",
            "ui_settings_settings_list_viewport",
        }
        props = self.layout_props(name, x, y, w, h, clickable=clickable and not disabled, scrollable=scrollable)
        if scrollable:
            next(
                item for item in props if item.get("strtype") == "OBJECT/Scrollbar_mode"
            )["strval"] = "OFF"
        if name == "ui_settings_settings_list_viewport":
            scroll_overrides = {
                "OBJECT/Scroll_one": "False",
                "OBJECT/Scroll_direction": "VER",
                "OBJECT/Scroll_snap_x": "NONE",
                "OBJECT/Scroll_snap_y": "NONE",
            }
            for item in props:
                if item.get("strtype") in scroll_overrides:
                    item["strval"] = scroll_overrides[item["strtype"]]
        if html.name in {"ui_settings_list_page_1", "ui_settings_list_page_2"}:
            next(
                item for item in props if item.get("strtype") == "OBJECT/Snappable"
            )["strval"] = "False"
        prefix = objtype
        props.extend(
            [
                self.prop(f"{prefix}/Overflow_visible", strval="False", inherited=2),
                self.prop(f"{prefix}/Scroll_with_arrow", strval="False", inherited=2),
                self.prop(f"{prefix}/Edited", strval="False", inherited=2),
                self.prop(f"{prefix}/User_1", strval="False", inherited=2),
                self.prop(f"{prefix}/User_2", strval="False", inherited=2),
                self.prop(f"{prefix}/User_3", strval="False", inherited=2),
                self.prop(f"{prefix}/User_4", strval="False", inherited=2),
            ]
        )
        if disabled:
            for item in props:
                if item.get("strtype") == "OBJECT/Disabled":
                    item["strval"] = "True"
        force_bg = None
        if html.tag == "svg":
            for tag, attrs in html.raw_children:
                if tag == "path" and attrs.get("fill"):
                    force_bg = attrs["fill"]
                    break
        props.append(
            self.prop(
                f"{prefix}/Style_main",
                strval="lv.PART.MAIN, Rectangle, Pad, Text, Transform",
                inherited=11,
                part="lv.PART.MAIN",
                childs=[
                    self.style_state(
                        self.rect_style(
                            style,
                            w,
                            h,
                            force_bg=force_bg,
                            clip=style.get("overflow") == "hidden",
                        )
                    )
                ],
            )
        )
        if html.name == "ui_gesture_zone_settings_return":
            props.append(
                self.event_prop(
                    "ui_evt_settings_gesture",
                    "GESTURE",
                    "ui_evt_settings_gesture",
                )
            )
        elif clickable and not disabled:
            action_suffix = name.removeprefix(f"ui_{state}_")
            event_name = f"ui_evt_{state}_{action_suffix}"
            props.append(self.event_prop(event_name, "CLICKED", event_name))
        return self.node(name, objtype, props, children, f"{state}:{html.node_id or html.name}")

    def make_skew_band(
        self,
        state: str,
        html: HtmlNode,
        name: str,
        style: dict[str, str],
        x: int,
        y: int,
        w: int,
        h: int,
    ) -> dict:
        spec = SKEW_BANDS[html.name]
        fill = "#00000000"
        for tag, attrs in html.raw_children:
            if tag == "path" and attrs.get("fill"):
                fill = attrs["fill"]
                break

        # 略微加宽旋转面板，让父面板在 x=0/w 处裁出 Pencil 的垂直侧边。
        fill_width = w + 8
        fill_height = spec["fill_height"]
        fill_x = -4
        fill_y = round((h - fill_height) / 2)
        fill_name = f"{name}_rotated_fill"
        fill_props = self.layout_props(fill_name, fill_x, fill_y, fill_width, fill_height, False, False)
        fill_props.extend(
            [
                self.prop("PANEL/Overflow_visible", strval="False", inherited=2),
                self.prop("PANEL/Scroll_with_arrow", strval="False", inherited=2),
                self.prop("PANEL/Edited", strval="False", inherited=2),
                self.prop("PANEL/User_1", strval="False", inherited=2),
                self.prop("PANEL/User_2", strval="False", inherited=2),
                self.prop("PANEL/User_3", strval="False", inherited=2),
                self.prop("PANEL/User_4", strval="False", inherited=2),
                self.prop(
                    "PANEL/Style_main",
                    strval="lv.PART.MAIN, Rectangle, Pad, Text, Transform",
                    inherited=11,
                    part="lv.PART.MAIN",
                    childs=[
                        self.style_state(
                            self.rect_style({}, fill_width, fill_height, force_bg=fill)
                            + [
                                self.prop("_style/Transform_rotation", integer=spec["angle"], inherited=6),
                                self.prop(
                                    "_style/Transform_pivot",
                                    intarray=[fill_width // 2, fill_height // 2],
                                    inherited=7,
                                ),
                            ]
                        )
                    ],
                ),
            ]
        )
        fill_node = self.node(fill_name, "PANEL", fill_props, [], f"{state}:{html.node_id}:rotated_fill")

        clip_props = self.layout_props(name, x, y, w, h, False, False)
        clip_props.extend(
            [
                self.prop("PANEL/Overflow_visible", strval="False", inherited=2),
                self.prop("PANEL/Scroll_with_arrow", strval="False", inherited=2),
                self.prop("PANEL/Edited", strval="False", inherited=2),
                self.prop("PANEL/User_1", strval="False", inherited=2),
                self.prop("PANEL/User_2", strval="False", inherited=2),
                self.prop("PANEL/User_3", strval="False", inherited=2),
                self.prop("PANEL/User_4", strval="False", inherited=2),
                self.prop(
                    "PANEL/Style_main",
                    strval="lv.PART.MAIN, Rectangle, Pad, Text, Transform",
                    inherited=11,
                    part="lv.PART.MAIN",
                    childs=[
                        self.style_state(
                            self.rect_style(style, w, h, force_bg="#00000000", clip=True)
                        )
                    ],
                ),
            ]
        )
        return self.node(name, "PANEL", clip_props, [fill_node], f"{state}:{html.node_id}")

    def node(self, name: str, objtype: str, properties: list[dict], children: list[dict], identity: str) -> dict:
        return {
            "guid": guid(f"node:{identity}:{name}"),
            "deepid": signed_hash(f"deep:{identity}"),
            "dont_export": False,
            "locked": False,
            "children": children,
            "properties": properties,
            "saved_objtypeKey": objtype,
        }

    def convert_html_node(self, state: str, html: HtmlNode) -> dict:
        style = node_style(html)
        x, y = px(style.get("left")), px(style.get("top"))
        w, h = px(style.get("width"), 1), px(style.get("height"), 1)
        name = self.unique_name(state, html.name)
        if html.tag == "svg" and html.attrs.get("data-icon-name"):
            return self.make_icon(state, html, name, style, x, y, w, h)
        if html.tag == "div" and "font-family" in style:
            return self.make_label(state, html, name, style, x, y, w, h)
        children = [self.convert_html_node(state, child) for child in html.children]
        return self.make_rect(state, html, name, style, x, y, w, h, children)

    @staticmethod
    def _set_property(node: dict, strtype: str, key: str, value) -> None:
        for item in node.get("properties", []):
            if item.get("strtype") == strtype:
                item[key] = value
                return

    @staticmethod
    def _html_structure_signature(node: HtmlNode) -> str:
        component_name = ProjectBuilder._component_for_html(node.name)
        if component_name:
            payload = {"component": component_name}
        else:
            style = node_style(node)
            payload = {
                "kind": "icon"
                if node.tag == "svg" and node.attrs.get("data-icon-name")
                else "label"
                if node.tag == "div" and "font-family" in style
                else "container",
                "tag": node.tag,
                "name": node.name,
                "children": [ProjectBuilder._html_structure_signature(child) for child in node.children],
            }
        return hashlib.sha1(json.dumps(payload, ensure_ascii=False, sort_keys=True).encode("utf-8")).hexdigest()

    @staticmethod
    def _component_structure_signature(node: HtmlNode) -> str:
        style = node_style(node)
        payload = {
            "kind": "icon"
            if node.tag == "svg" and node.attrs.get("data-icon-name")
            else "label"
            if node.tag == "div" and "font-family" in style
            else "container",
            "tag": node.tag,
            "children": [ProjectBuilder._component_structure_signature(child) for child in node.children],
        }
        return hashlib.sha1(json.dumps(payload, sort_keys=True).encode("utf-8")).hexdigest()

    @staticmethod
    def _stable_node_ref(target: dict, component_path: dict | None = None) -> dict:
        if component_path is None and target.get("cid"):
            component_path = {
                "cid": target["cid"],
                "cgid": target.get("cgid"),
                "coid_path": [target.get("coid")],
            }
        ref = {
            "guid": target["guid"],
            "logical_name": ProjectBuilder._object_name(target),
        }
        if component_path:
            ref["component_path"] = component_path
        return ref

    def _lvgl_patch_descriptor(
        self,
        target: dict,
        source: HtmlNode,
        state: str,
        component_path: dict | None = None,
    ) -> dict:
        if target.get("cid"):
            component_path = {
                "cid": target["cid"],
                "cgid": target.get("cgid"),
                "coid_path": [target.get("coid")],
            }
        elif component_path:
            component_path = {
                **component_path,
                "coid_path": [*component_path["coid_path"], target.get("coid")],
            }
        style = node_style(source)
        raw_x, raw_y = px(style.get("left")), px(style.get("top"))
        raw_w, raw_h = px(style.get("width"), 1), px(style.get("height"), 1)
        font_code: str | None = None
        font_size = 1
        if source.tag == "svg" and source.attrs.get("data-icon-name"):
            font_size = max(raw_w, raw_h)
            font_code = f"lucide_{font_size}"
        elif source.tag == "div" and "font-family" in style:
            _family, _weight, font_size, font_code = self.font_combo(style)
        x, y, w, h = self._normalize_lvgl_geometry(
            source, style, raw_x, raw_y, raw_w, raw_h, font_code, font_size
        )
        disabled = "disabled" in source.name.lower() or source.name in NON_INTERACTIVE_CONTROLS.get(state, set())
        operations: list[dict] = [
            {"op": "lv_obj_set_pos", "args": [x, y]},
            {"op": "lv_obj_set_size", "args": [w, h]},
            {"op": "lv_obj_clear_flag", "args": ["HIDDEN"]},
            {"op": "lv_obj_add_state" if disabled else "lv_obj_clear_state", "args": ["DISABLED"]},
        ]
        if source.tag == "svg" and source.attrs.get("data-icon-name"):
            codepoint = self.lucene.get(source.attrs["data-icon-name"])
            fill = next(
                (attrs["fill"] for tag, attrs in source.raw_children if tag == "path" and attrs.get("fill")),
                "#FFFFFF",
            )
            operations.extend(
                [
                    {"op": "lv_label_set_text", "args": [chr(codepoint)] if codepoint is not None else []},
                    {"op": "lv_obj_set_style_text_color", "args": rgba(fill)},
                    {"op": "lv_obj_set_style_text_font", "args": [font_code]},
                ]
            )
        elif source.tag == "div" and "font-family" in style:
            operations.extend(
                [
                    {"op": "lv_label_set_text", "args": [normalized_text(source)]},
                    {"op": "lv_obj_set_style_text_color", "args": rgba(style.get("color", "#FFFFFF"))},
                    {"op": "lv_obj_set_style_text_font", "args": [font_code]},
                    {
                        "op": "lv_obj_set_style_text_align",
                        "args": [{"left": "LEFT", "center": "CENTER", "right": "RIGHT"}.get(style.get("text-align", "left"), "LEFT")],
                    },
                ]
            )
        else:
            background = rgba(style.get("background-color", "#00000000"))
            radius_text = style.get("border-radius", "0")
            radius = min(w, h) // 2 if "%" in radius_text else px(radius_text)
            operations.extend(
                [
                    {"op": "lv_obj_set_style_bg_color", "args": background[:3]},
                    {"op": "lv_obj_set_style_bg_opa", "args": [background[3]]},
                    {"op": "lv_obj_set_style_radius", "args": [radius]},
                ]
            )
            border = re.search(r"([0-9.]+)px\s+\w+\s+(#[0-9A-Fa-f]{6,8})", style.get("border", ""))
            if border:
                operations.extend(
                    [
                        {"op": "lv_obj_set_style_border_width", "args": [round(float(border.group(1)))]},
                        {"op": "lv_obj_set_style_border_color", "args": rgba(border.group(2))[:3]},
                    ]
                )

        target_children = target.get("children", [])
        if len(target_children) < len(source.children):
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "state_patch_target_tree_mismatch",
                        "state": state,
                        "source_object": source.name,
                        "target": self._object_name(target),
                    },
                    ensure_ascii=False,
                )
            )
        return {
            "target": self._stable_node_ref(target, component_path),
            "source_object": source.name,
            "operations": operations,
            "children": [
                self._lvgl_patch_descriptor(target_child, source_child, state, component_path)
                for target_child, source_child in zip(target_children, source.children)
            ],
        }

    def _state_source_children(self, state: str) -> list[HtmlNode]:
        page = self.state_pages[state]
        if (
            len(page.children) == 1
            and self.structured_source_name(page.children[0].name)
            and "· Surface" in page.children[0].name
        ):
            children = page.children[0].children
        else:
            children = page.children
        if (
            state in {"mode_default", "mode_disconnected"}
            and self._find_html(page, MODE_TITLE_RESERVE_NAME) is None
        ):
            return [*children, copy.deepcopy(self.mode_title_reserve)]
        return children

    @staticmethod
    def _find_html(root: HtmlNode, name: str) -> HtmlNode | None:
        if root.name == name:
            return root
        for child in root.children:
            found = ProjectBuilder._find_html(child, name)
            if found:
                return found
        return None

    @staticmethod
    def _collect_function_names(value) -> list[str]:
        names: list[str] = []
        if isinstance(value, dict):
            if value.get("strtype") == "CALL FUNCTION/Function_name" and value.get("strval"):
                names.append(value["strval"])
            for child in value.values():
                names.extend(ProjectBuilder._collect_function_names(child))
        elif isinstance(value, list):
            for child in value:
                names.extend(ProjectBuilder._collect_function_names(child))
        return names

    def _state_event_names(self, target: dict, source: HtmlNode, state: str) -> list[str]:
        names: list[str] = []
        if self.is_clickable(source.name, state):
            for prop in target.get("properties", []):
                names.extend(self._collect_function_names(prop))
        for target_child, source_child in zip(target.get("children", []), source.children):
            for event_name in self._state_event_names(target_child, source_child, state):
                if event_name not in names:
                    names.append(event_name)
        return names

    @staticmethod
    def _walk_patch_descriptors(value):
        if isinstance(value, dict):
            if "target" in value and "source_object" in value:
                yield value
            for child in value.get("children", []):
                yield from ProjectBuilder._walk_patch_descriptors(child)

    def _descriptor_by_source(self, state: str, source_object: str) -> dict:
        patch = self.state_patches[state]
        matches = [
            descriptor
            for top in patch["overrides"].values()
            for descriptor in self._walk_patch_descriptors(top)
            if descriptor.get("source_object") == source_object
        ]
        if len(matches) != 1:
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "machine_contract_selector_mismatch",
                        "state": state,
                        "source_object": source_object,
                        "matches": len(matches),
                    },
                    ensure_ascii=False,
                )
            )
        return matches[0]

    @staticmethod
    def _event_semantics(callback: str, state: str, source_object: str) -> dict:
        if callback == "ui_evt_main_shell_gesture":
            return {
                "semantic_intent": "open_settings",
                "abstract_route_refs": [],
                "concrete_route_ids": ["route_settings_open"],
                "routing_mode": "route_resolution",
            }
        if callback == "ui_evt_settings_gesture":
            return {
                "semantic_intent": "back_to_shell",
                "abstract_route_refs": ["route_back_to_shell"],
                "concrete_route_ids": ["route_settings_back"],
                "routing_mode": "route_resolution",
            }
        settings_values = {
            "ui_evt_settings_btn_brightness_low": ("set_brightness", "low"),
            "ui_evt_settings_btn_brightness_medium": ("set_brightness", "medium"),
            "ui_evt_settings_btn_brightness_high": ("set_brightness", "high"),
            "ui_evt_settings_btn_screen_timeout_5s": ("set_screen_timeout", 5),
            "ui_evt_settings_btn_screen_timeout_15s": ("set_screen_timeout", 15),
            "ui_evt_settings_btn_screen_timeout_30s": ("set_screen_timeout", 30),
        }
        if callback in settings_values:
            semantic_intent, intent_value = settings_values[callback]
            return {
                "semantic_intent": semantic_intent,
                "intent_value": intent_value,
                "abstract_route_refs": [],
                "concrete_route_ids": [],
                "routing_mode": "typed_intent_only",
            }
        nav_routes = {
            "nav_home_button": ("nav_slot_01", "route_shell_slot_01", "route_shell_01"),
            "nav_control_button": ("nav_slot_02", "route_shell_slot_02", "route_shell_02"),
            "nav_mode_power_button": ("nav_slot_03", "route_shell_slot_03", "route_shell_03"),
            "nav_alerts_button": ("nav_slot_04", "route_shell_slot_04", "route_shell_04"),
        }
        for token, (intent, abstract, concrete) in nav_routes.items():
            if token in callback:
                return {
                    "semantic_intent": intent,
                    "abstract_route_refs": [abstract],
                    "concrete_route_ids": [concrete],
                    "routing_mode": "route_resolution",
                }

        rules = [
            ("choose_device", "open_ble_binding", ["route_binding_entry"], ["route_ble_open"], "route_resolution"),
            ("reconnect_now", "request_ble_reconnect", ["route_ble_reconnect_success"], ["route_ble_restore"], "typed_intent_then_async_result"),
            ("payment_verify", "request_payment_verification", ["route_payment_success"], ["route_payment_success", "route_payment_failed"], "typed_intent_then_async_result"),
            ("payment_retry", "request_payment_verification", ["route_payment_success"], ["route_payment_success", "route_payment_failed"], "typed_intent_then_async_result"),
            ("row_device_info", "open_device_info", ["route_settings_children"], ["route_device_open"], "route_resolution"),
            ("row_maintenance", "open_maintenance", ["route_settings_children"], ["route_maintenance_open"], "route_resolution"),
            ("row_cellular", "request_cellular_connect", [], [], "typed_intent_then_async_result"),
            ("row_selftest", "open_selftest", ["route_maintenance_children"], ["route_selftest_open"], "route_resolution"),
            ("row_clear_binding", "open_clear_binding", ["route_maintenance_children", "route_clear_binding_modal"], ["route_clear_open"], "route_resolution"),
            ("btn_rescan", "request_ble_scan", ["route_ble_flow"], ["route_ble_scan_empty", "route_ble_scan_candidates"], "typed_intent_then_async_result"),
            ("cmp_ble_candidate_row", "select_ble_candidate", ["route_ble_flow"], ["route_ble_select"], "route_resolution"),
            ("start_selftest", "start_selftest", ["route_selftest_flow"], ["route_selftest_start"], "route_resolution"),
            ("manual_fail", "submit_manual_fail", ["route_selftest_flow"], ["route_manual_next", "route_manual_complete"], "route_resolution"),
            ("manual_pass", "submit_manual_pass", ["route_selftest_flow"], ["route_manual_next", "route_manual_complete"], "route_resolution"),
            ("retry_failed", "open_failed_item_retry", ["route_selftest_result"], ["route_result_retry"], "route_resolution"),
            ("rerun_all", "rerun_all_selftests", ["route_selftest_result"], ["route_result_rerun"], "route_resolution"),
            ("gps_outdoor", "submit_gps_context_outdoor", ["route_selftest_gps_context"], ["route_context_success"], "typed_intent_then_async_result"),
            ("gps_indoor", "submit_gps_context_indoor", ["route_selftest_gps_context"], ["route_context_success"], "typed_intent_then_async_result"),
            ("failed_prev", "select_previous_failed_item", ["route_selftest_retry"], [], "typed_intent_only"),
            ("failed_next", "select_next_failed_item", ["route_selftest_retry"], [], "typed_intent_only"),
            ("retry_item", "retry_selected_failed_item", ["route_selftest_retry"], ["route_retry_gps", "route_retry_manual", "route_retry_auto"], "route_resolution"),
            ("confirm_clear_binding", "request_clear_binding", ["route_clear_binding_modal"], ["route_clear_success", "route_clear_failed"], "typed_intent_then_async_result"),
            ("btn_cancel", "cancel_clear_binding", ["route_clear_binding_modal"], ["route_clear_cancel"], "route_resolution"),
            ("leave_selftest", "leave_selftest_foreground", ["route_selftest_flow"], [], "typed_intent_only"),
            ("gear_minus", "decrease_gear", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("gear_plus", "increase_gear", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("toggle_assist", "toggle_assist", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("mode_standard", "select_mode_standard", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("mode_sport", "select_mode_sport", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("mode_extreme", "select_mode_extreme", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("mode_eco", "select_mode_eco", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("exo_poweroff", "request_exoskeleton_poweroff", ["route_selftest_lock_overlay"], [], "typed_intent_only"),
            ("cmp_setting_toggle", "toggle_setting", [], [], "typed_intent_only"),
        ]
        for token, intent, abstract, concrete, mode in rules:
            if token in callback:
                return {
                    "semantic_intent": intent,
                    "abstract_route_refs": abstract,
                    "concrete_route_ids": concrete,
                    "routing_mode": mode,
                }

        if "btn_back" in callback:
            if state in {"selftest_gps_context", "selftest_gps_submit_failed"}:
                return {
                    "semantic_intent": "back_to_origin",
                    "abstract_route_refs": ["route_selftest_gps_context"],
                    "concrete_route_ids": [],
                    "routing_mode": "typed_intent_only",
                    "origin_contract": {
                        "field": "selftest_gps_origin",
                        "type": "enum",
                        "values": ["full_selftest", "retry_detail"],
                        "history_policy": "preserve_origin",
                        "phase3_resolution": {
                            "full_selftest": "return_to_full_selftest_flow",
                            "retry_detail": "return_to_08-06_retry_detail",
                        },
                        "missing_or_invalid_origin": "preserve_current_page_log_and_retry",
                    },
                }
            back_contracts = {
                "device_info": ("back_to_settings", ["route_back_to_settings"], ["route_device_back"]),
                "maintenance": ("back_to_settings", ["route_back_to_settings"], ["route_maintenance_back"]),
                "ble_empty": ("back_to_unbound_home", ["route_back_to_shell"], ["route_ble_back"]),
                "ble_candidates": ("back_to_unbound_home", ["route_back_to_shell"], ["route_ble_back"]),
                "ble_scanning": ("back_to_unbound_home", ["route_back_to_shell"], ["route_ble_back"]),
                "selftest_retry_detail": ("back_to_selftest_result", ["route_back_to_selftest_result"], []),
            }
            intent, abstract, concrete = back_contracts.get(
                state,
                ("back_to_maintenance", ["route_back_to_maintenance"], []),
            )
            return {
                "semantic_intent": intent,
                "abstract_route_refs": abstract,
                "concrete_route_ids": concrete,
                "routing_mode": "route_resolution" if concrete else "typed_intent_only",
            }
        raise RuntimeError(
            json.dumps(
                {
                    "code": "unmapped_generated_callback",
                    "state": state,
                    "callback": callback,
                    "source_object": source_object,
                },
                ensure_ascii=False,
            )
        )

    def _manifest_event_names(self, state: str) -> list[str]:
        events = list(self.state_events[state])
        if state in {"home_normal", "home_unbound", "home_disconnected"}:
            events.append("ui_evt_main_shell_gesture")
        if state == "settings" and "ui_evt_settings_gesture" not in events:
            events.append("ui_evt_settings_gesture")
        return events

    def _event_bindings(self, project_nodes_by_guid: dict[str, dict]) -> list[dict]:
        bindings: list[dict] = []
        for state in [*PAGE_STATES.values(), *VARIANT_STATES.values()]:
            by_callback: defaultdict[str, list[dict]] = defaultdict(list)
            source_by_callback: defaultdict[str, set[str]] = defaultdict(set)
            for top in self.state_patches[state]["overrides"].values():
                for descriptor in self._walk_patch_descriptors(top):
                    node = project_nodes_by_guid.get(descriptor["target"]["guid"])
                    if node is None:
                        continue
                    for callback in self._collect_function_names(node.get("properties", [])):
                        if callback in self.state_events[state] and descriptor["target"] not in by_callback[callback]:
                            by_callback[callback].append(descriptor["target"])
                            source_by_callback[callback].add(descriptor["source_object"])
            for callback in self._manifest_event_names(state):
                selectors = sorted(by_callback[callback], key=lambda item: (item["logical_name"], item["guid"]))
                source_objects = sorted(source_by_callback[callback])
                if not selectors:
                    callback_nodes = [
                        node
                        for node in project_nodes_by_guid.values()
                        if callback in self._collect_function_names(node.get("properties", []))
                    ]
                    selectors = sorted(
                        (self._stable_node_ref(node) for node in callback_nodes),
                        key=lambda item: (item["logical_name"], item["guid"]),
                    )
                    source_objects = sorted(
                        {self._object_name(node) for node in callback_nodes if self._object_name(node)}
                    )
                if not selectors or not source_objects:
                    raise RuntimeError(
                        json.dumps(
                            {"code": "event_source_selector_missing", "state": state, "callback": callback},
                            ensure_ascii=False,
                        )
                    )
                contract = self._event_semantics(callback, state, source_objects[0])
                bindings.append(
                    {
                        "state": state,
                        "callback": callback,
                        "source_objects": source_objects,
                        "source_selectors": selectors,
                        **contract,
                    }
                )
        return bindings

    @staticmethod
    def _component_for_html(name: str) -> str | None:
        if name in {
            "ui_row_haptics_instance",
            "ui_row_click_audio_instance",
            "ui_row_raise_wake_instance",
        }:
            return "cmp_setting_toggle"
        if name.startswith("ui_alert_") and name.endswith("_instance"):
            return "cmp_alert_row"
        if re.fullmatch(r"ui_device_candidate_[1-4]", name):
            return "cmp_ble_candidate_row"
        if name in {
            "ui_result_display",
            "ui_result_touch",
            "ui_result_audio",
            "ui_result_gps",
            "ui_result_power",
            "ui_result_sensor",
            "ui_result_cellular",
            "ui_result_ble",
        }:
            return "cmp_selftest_result_row"
        return None

    def reusable_components(self) -> dict[str, dict]:
        components: dict[str, dict] = {}
        for component_name, (state, source_name, _max_instances) in COMPONENT_SOURCES.items():
            source = self._find_html(self.state_pages[state], source_name)
            if source is None:
                raise RuntimeError(
                    json.dumps(
                        {"code": "missing_component_source", "component": component_name, "source": source_name},
                        ensure_ascii=False,
                    )
                )
            self.name_counts[component_name].clear()
            self.component_structure_signatures[component_name] = self._component_structure_signature(source)
            root = self.convert_html_node(component_name, source)
            self._set_property(root, "OBJECT/Name", "strval", component_name)
            self._set_property(root, "OBJECT/Position", "intarray", [0, 0])
            if component_name == "cmp_setting_toggle":
                self._set_property(root, "OBJECT/Gesture_bubble", "strval", "True")
            self._rewrite_component_events(root, f"ui_evt_{component_name}_clicked")
            root["guid"] = guid(f"component:{component_name}")
            root["deepid"] = signed_hash(f"component:{component_name}")
            components[component_name] = root
        return components

    @staticmethod
    def _rewrite_component_events(value, event_name: str) -> None:
        if isinstance(value, dict):
            if value.get("strtype") in {"_custom/name", "CALL FUNCTION/Function_name"}:
                value["strval"] = event_name
            for child in value.values():
                ProjectBuilder._rewrite_component_events(child, event_name)
        elif isinstance(value, list):
            for child in value:
                ProjectBuilder._rewrite_component_events(child, event_name)

    @staticmethod
    def _overlay_property_values(target: dict, source: dict) -> None:
        # 组件实例的事件由 master 继承。覆盖实例事件会被 SquareLine 1.6.1
        # 保存时删除，并产生只在保存前存在的孤立事件空桩。
        if target.get("strtype") == "_event/EventHandler":
            return
        preserved = {"nid", "compnid", "InheritedType", "strtype", "childs"}
        if target.get("strtype") != "OBJECT/Name":
            for key in list(target):
                if key not in preserved and key in source:
                    target[key] = copy.deepcopy(source[key])
        target_children = target.get("childs", [])
        source_children = source.get("childs", [])
        source_by_type: defaultdict[str, list[dict]] = defaultdict(list)
        for child in source_children:
            source_by_type[child.get("strtype", "")].append(child)
        seen: defaultdict[str, int] = defaultdict(int)
        for child in target_children:
            strtype = child.get("strtype", "")
            index = seen[strtype]
            seen[strtype] += 1
            matches = source_by_type.get(strtype, [])
            if index < len(matches):
                ProjectBuilder._overlay_property_values(child, matches[index])

    @staticmethod
    def _overlay_component_visual(target: dict, source: dict) -> None:
        target_props = {item.get("strtype", ""): item for item in target.get("properties", [])}
        for source_prop in source.get("properties", []):
            target_prop = target_props.get(source_prop.get("strtype", ""))
            if target_prop:
                ProjectBuilder._overlay_property_values(target_prop, source_prop)
        if len(target.get("children", [])) != len(source.get("children", [])):
            raise RuntimeError(
                json.dumps(
                    {
                        "code": "component_visual_tree_mismatch",
                        "target": ProjectBuilder._object_name(target),
                        "target_children": len(target.get("children", [])),
                        "source_children": len(source.get("children", [])),
                    },
                    ensure_ascii=False,
                )
            )
        for target_child, source_child in zip(target.get("children", []), source.get("children", []), strict=True):
            ProjectBuilder._overlay_component_visual(target_child, source_child)

    def component_to_instance(
        self,
        component: dict,
        instance_key: str,
        instance_name: str,
        position: list[int],
        state: str,
        source_html: HtmlNode,
    ) -> dict:
        instance = copy.deepcopy(component)
        visual = self.convert_html_node(f"{state}_component_visual", source_html)
        self._overlay_component_visual(instance, visual)
        if self._object_name(component) == "cmp_setting_toggle":
            self._set_property(instance, "OBJECT/Gesture_bubble", "strval", "True")
        cgid = signed_hash(f"cgid:{instance_key}")
        component_guid = component["guid"]

        def mark_overrides(target: dict, master: dict, is_root: bool = False) -> None:
            ignored = {"nid", "compnid", "InheritedType", "flags", "childs"}
            for target_prop, master_prop in zip(
                target.get("properties", []),
                master.get("properties", []),
                strict=True,
            ):
                keys = (set(target_prop) | set(master_prop)) - ignored
                if any(target_prop.get(key) != master_prop.get(key) for key in keys):
                    target_prop["flags"] = target_prop.get("flags", 0) | COMPONENT_VALUE_OVERRIDE_FLAG
                if is_root and target_prop.get("strtype") in {"OBJECT/Name", "OBJECT/Position"}:
                    target_prop["flags"] = (
                        target_prop.get("flags", 0) & ~COMPONENT_VALUE_OVERRIDE_FLAG
                    ) | COMPONENT_IDENTITY_OVERRIDE_FLAG
            for target_child, master_child in zip(
                target.get("children", []),
                master.get("children", []),
                strict=True,
            ):
                mark_overrides(target_child, master_child)

        mark_overrides(instance, component, True)

        def bind_component_properties(value) -> None:
            if isinstance(value, dict):
                if "nid" in value:
                    value["compnid"] = value["nid"]
                    value["nid"] = self.nid()
                for child_value in value.values():
                    bind_component_properties(child_value)
            elif isinstance(value, list):
                for child_value in value:
                    bind_component_properties(child_value)

        bind_component_properties(instance)

        def assign(node: dict, is_root: bool = False) -> None:
            identity = node.get("deepid", 0)
            component_node_guid = node["guid"]
            node["guid"] = guid(f"instance:{instance_key}:{identity}:{component_node_guid}")
            node["coid"] = legacy_dotnet_string_hash(component_node_guid)
            node["cgid"] = cgid
            if is_root:
                for item in node.get("properties", []):
                    if item.get("strtype") == "OBJECT/Position":
                        item["intarray"] = position
                    elif item.get("strtype") == "OBJECT/Name":
                        item["strval"] = instance_name
            for child in node.get("children", []):
                assign(child)

        assign(instance, True)
        instance["cid"] = component_guid
        return instance

    def convert_page_node(
        self,
        root_name: str,
        state: str,
        html: HtmlNode,
        components: dict[str, dict],
        identity: str,
    ) -> dict:
        style = node_style(html)
        component_name = self._component_for_html(html.name)
        if component_name:
            actual_structure = self._component_structure_signature(html)
            expected_structure = self.component_structure_signatures[component_name]
            if actual_structure != expected_structure:
                raise RuntimeError(
                    json.dumps(
                        {
                            "code": "component_instance_structure_mismatch",
                            "component": component_name,
                            "state": state,
                            "source_object": html.name,
                            "expected_signature": expected_structure,
                            "actual_signature": actual_structure,
                        },
                        ensure_ascii=False,
                    )
                )
            instance_name = self.unique_name(state, html.name)
            if component_name not in self.state_component_refs[state]:
                self.state_component_refs[state].append(component_name)
            return self.component_to_instance(
                components[component_name],
                f"{root_name}:{state}:{identity}",
                instance_name,
                [px(style.get("left")), px(style.get("top"))],
                state,
                html,
            )

        x, y = px(style.get("left")), px(style.get("top"))
        w, h = px(style.get("width"), 1), px(style.get("height"), 1)
        name = self.unique_name(state, html.name)
        if html.tag == "svg" and html.attrs.get("data-icon-name"):
            return self.make_icon(state, html, name, style, x, y, w, h)
        if html.tag == "div" and "font-family" in style:
            return self.make_label(state, html, name, style, x, y, w, h)
        children = [
            self.convert_page_node(root_name, state, child, components, f"{identity}:{index}")
            for index, child in enumerate(html.children)
        ]
        return self.make_rect(state, html, name, style, x, y, w, h, children)

    def page_root(self, root_name: str, components: dict[str, dict]) -> dict:
        states = ROOT_STATES[root_name]
        default_state = states[0]
        visible_by_state: dict[str, list[str]] = {state: [] for state in states}
        overrides_by_state: dict[str, dict[str, dict]] = {state: {} for state in states}
        objects: dict[tuple[str, str], dict] = {}
        object_names: dict[tuple[str, str], str] = {}

        for state in states:
            self.name_counts[state].clear()
            occurrence: defaultdict[str, int] = defaultdict(int)
            for source in self._state_source_children(state):
                occurrence[source.name] += 1
                semantic = f"{source.name}#{occurrence[source.name]}"
                signature = self._html_structure_signature(source)
                key = (semantic, signature)
                if key not in objects:
                    node = self.convert_page_node(
                        root_name,
                        state,
                        source,
                        components,
                        f"{semantic}:{signature}",
                    )
                    objects[key] = node
                    object_names[key] = self._object_name(node)
                target_guid = objects[key]["guid"]
                visible_by_state[state].append(target_guid)
                overrides_by_state[state][target_guid] = self._lvgl_patch_descriptor(objects[key], source, state)
                for event_name in self._state_event_names(objects[key], source, state):
                    if event_name not in self.state_events[state]:
                        self.state_events[state].append(event_name)

        all_guids = [objects[key]["guid"] for key in objects]
        default_visible = set(visible_by_state[default_state])
        children = []
        for key, node in objects.items():
            hidden_value = "False" if node["guid"] in default_visible else "True"
            self._set_property(node, "OBJECT/Hidden", "strval", hidden_value)
            if node.get("cid") and hidden_value == "True":
                for item in node.get("properties", []):
                    if item.get("strtype") == "OBJECT/Hidden":
                        item["flags"] = item.get("flags", 0) | COMPONENT_VALUE_OVERRIDE_FLAG
                        break
            children.append(node)
        component_by_guid = {component["guid"]: component_name for component_name, component in components.items()}
        object_by_guid = {objects[key]["guid"]: objects[key] for key in objects}

        def refs_for(guids: set[str]) -> list[dict]:
            refs = [self._stable_node_ref(object_by_guid[target_guid]) for target_guid in guids]
            return sorted(refs, key=lambda ref: (ref["logical_name"], ref["guid"]))

        for state, visible in visible_by_state.items():
            visible_set = set(visible)
            self.state_component_refs[state] = sorted(
                {
                    component_by_guid[node.get("cid")]
                    for target_guid in visible
                    for node in walk_project_nodes(object_by_guid[target_guid])
                    if node.get("cid") in component_by_guid
                }
            )
            self.state_patches[state] = {
                "baseline_reset": [
                    {
                        "target": self._stable_node_ref(object_by_guid[target_guid]),
                        "operations": [
                            {"op": "lv_obj_add_flag", "args": ["HIDDEN"]},
                            {"op": "lv_obj_clear_state", "args": ["DISABLED"]},
                            {"op": "lv_obj_clear_state", "args": ["CHECKED"]},
                        ],
                    }
                    for target_guid in sorted(
                        all_guids,
                        key=lambda value: (self._object_name(object_by_guid[value]), value),
                    )
                ],
                "show": refs_for(visible_set),
                "hide": refs_for(set(all_guids) - visible_set),
                "overrides": overrides_by_state[state],
            }

        scrollable = root_name in {
            "ui_page_ble_root",
            "ui_page_selftest_result_root",
            "ui_page_selftest_retry_detail_root",
        }
        root_clickable = False
        props = self.layout_props(
            root_name,
            0,
            0,
            CANVAS_WIDTH,
            CANVAS_HEIGHT,
            root_clickable,
            scrollable,
        )
        props.extend(
            [
                self.prop("PANEL/Overflow_visible", strval="False", inherited=2),
                self.prop("PANEL/Scroll_with_arrow", strval="False", inherited=2),
                self.prop("PANEL/Edited", strval="False", inherited=2),
                self.prop("PANEL/User_1", strval="False", inherited=2),
                self.prop("PANEL/User_2", strval="False", inherited=2),
                self.prop("PANEL/User_3", strval="False", inherited=2),
                self.prop("PANEL/User_4", strval="False", inherited=2),
                self.prop(
                    "PANEL/Style_main",
                    strval="lv.PART.MAIN, Rectangle, Pad, Text, Transform",
                    inherited=11,
                    part="lv.PART.MAIN",
                    childs=[
                        self.style_state(
                            self.rect_style(
                                {"border-radius": f"{CORNER_RADIUS}px"},
                                CANVAS_WIDTH,
                                CANVAS_HEIGHT,
                                force_bg="#000000",
                                clip=True,
                            )
                        )
                    ],
                ),
            ]
        )
        root = self.node(root_name, "PANEL", props, children, f"page-root:{root_name}")
        if root_name == "ui_modal_clear_binding_root":
            self._set_property(root, "OBJECT/Hidden", "strval", "True")
        return root

    def screen(self, name: str, root_names: list[str], temporary: bool, scrolling: str, roots: dict[str, dict], index: int) -> dict:
        children = [copy.deepcopy(roots[root_name]) for root_name in root_names]
        if scrolling == "HOR":
            page_names = ("home", "control", "mode", "alerts")
            pages = []
            for slot, (root_name, page_root) in enumerate(zip(root_names, children)):
                page_name = f"ui_main_shell_page_{page_names[slot]}"
                page_props = self.layout_props(
                    page_name,
                    slot * CANVAS_WIDTH,
                    0,
                    CANVAS_WIDTH,
                    CANVAS_HEIGHT,
                    False,
                    False,
                )
                for item in page_props:
                    if item.get("strtype") == "OBJECT/Snappable":
                        item["strval"] = "True"
                page_props.extend(
                    [
                        self.prop("PANEL/Overflow_visible", strval="False", inherited=2),
                        self.prop("PANEL/Scroll_with_arrow", strval="False", inherited=2),
                        self.prop("PANEL/Edited", strval="False", inherited=2),
                        self.prop("PANEL/User_1", strval="False", inherited=2),
                        self.prop("PANEL/User_2", strval="False", inherited=2),
                        self.prop("PANEL/User_3", strval="False", inherited=2),
                        self.prop("PANEL/User_4", strval="False", inherited=2),
                        self.prop(
                            "PANEL/Style_main",
                            strval="lv.PART.MAIN, Rectangle, Pad, Text, Transform",
                            inherited=11,
                            part="lv.PART.MAIN",
                            childs=[
                                self.style_state(
                                    self.rect_style(
                                        {"border-radius": f"{CORNER_RADIUS}px"},
                                        CANVAS_WIDTH,
                                        CANVAS_HEIGHT,
                                        force_bg="#000000",
                                        clip=True,
                                    )
                                )
                            ],
                        ),
                    ]
                )
                pages.append(self.node(page_name, "PANEL", page_props, [page_root], f"{name}:page:{root_name}"))

            pager_name = "ui_main_shell_pager"
            pager_props = self.layout_props(pager_name, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, True, True)
            pager_overrides = {
                "OBJECT/Scroll_one": "True",
                "OBJECT/Scrollbar_mode": "OFF",
                "OBJECT/Scroll_direction": "HOR",
                "OBJECT/Scroll_snap_x": "CENTER",
                "OBJECT/Scroll_snap_y": "NONE",
            }
            for item in pager_props:
                if item.get("strtype") in pager_overrides:
                    item["strval"] = pager_overrides[item["strtype"]]
            pager_props.extend(
                [
                    self.prop("PANEL/Overflow_visible", strval="False", inherited=2),
                    self.prop("PANEL/Scroll_with_arrow", strval="False", inherited=2),
                    self.prop("PANEL/Edited", strval="False", inherited=2),
                    self.prop("PANEL/User_1", strval="False", inherited=2),
                    self.prop("PANEL/User_2", strval="False", inherited=2),
                    self.prop("PANEL/User_3", strval="False", inherited=2),
                    self.prop("PANEL/User_4", strval="False", inherited=2),
                    self.prop(
                        "PANEL/Style_main",
                        strval="lv.PART.MAIN, Rectangle, Pad, Text, Transform",
                        inherited=11,
                        part="lv.PART.MAIN",
                        childs=[
                            self.style_state(
                                self.rect_style(
                                    {"border-radius": f"{CORNER_RADIUS}px"},
                                    CANVAS_WIDTH,
                                    CANVAS_HEIGHT,
                                    force_bg="#000000",
                                    clip=True,
                                )
                            )
                        ],
                    ),
                    self.prop(
                        "PANEL/Style_scrollbar",
                        strval="lv.PART.SCROLLBAR, Rectangle, Pad, Transform",
                        inherited=11,
                        part="lv.PART.SCROLLBAR",
                        childs=[],
                    ),
                    self.event_prop(
                        "ui_evt_main_shell_gesture",
                        "GESTURE",
                        "ui_evt_main_shell_gesture",
                    ),
                ]
            )
            children = [self.node(pager_name, "PANEL", pager_props, pages, f"{name}:pager")]
        props = [
            self.prop("OBJECT/Name", strval=name, inherited=10),
            self.prop("OBJECT/Layout"),
            self.prop(
                "OBJECT/Layout_type",
                strval="No_layout",
                inherited=13,
                Flow=0,
                Wrap=False,
                Reversed=False,
                MainAlignment=0,
                CrossAlignment=0,
                TrackAlignment=0,
                LayoutType=0,
            ),
            self.prop("OBJECT/Transform"),
            self.prop("OBJECT/Flags", flags=1_048_576),
            self.prop("OBJECT/Scrolling", flags=1_048_576),
            self.prop("OBJECT/Scrollable", strval="False", inherited=2),
            self.prop("OBJECT/Scroll_elastic", strval="True", inherited=2),
            self.prop("OBJECT/Scroll_momentum", strval="True", inherited=2),
            self.prop("OBJECT/Scroll_chain", strval="False", inherited=2),
            self.prop("OBJECT/Scroll_one", strval="False", inherited=2),
            self.prop("OBJECT/Scrollbar_mode", strval="OFF", inherited=3),
            self.prop("OBJECT/Scroll_direction", strval="ALL", inherited=3),
            self.prop("OBJECT/Scroll_snap_x", strval="NONE", inherited=3),
            self.prop("OBJECT/Scroll_snap_y", strval="NONE", inherited=3),
            self.prop("OBJECT/States", flags=1_048_576),
            self.prop("SCREEN/Screen"),
            self.prop("SCREEN/Temporary", strval="True" if temporary else "False", inherited=2),
            self.prop(
                "SCREEN/Style_main",
                strval="lv.PART.MAIN, Rectangle, Pad, Text",
                inherited=11,
                part="lv.PART.MAIN",
                childs=[
                    self.style_state(
                        self.rect_style(
                            {"border-radius": f"{CORNER_RADIUS}px"},
                            CANVAS_WIDTH,
                            CANVAS_HEIGHT,
                            force_bg="#000000",
                            clip=True,
                        )
                    )
                ],
            ),
            self.prop("SCREEN/Style_scrollbar", strval="lv.PART.SCROLLBAR, Rectangle, Pad", inherited=11, part="lv.PART.SCROLLBAR", childs=[]),
        ]
        return {
            "guid": guid(f"{SCREEN_GUID_NAMESPACE}:screen:{name}"),
            "isPage": True,
            "editor_posx": 500 + index * 460,
            "editor_posy": -500,
            "children": children,
            "properties": props,
            "saved_objtypeKey": "SCREEN",
        }

    @staticmethod
    def inherited_to_component(value: int) -> str:
        return {
            1: "PropertyBlock",
            2: "PropertyBool",
            3: "PropertyChoice",
            4: "PropertyEvent",
            6: "PropertyInt",
            7: "PropertyIntArray",
            8: "PropertyAnimation",
            9: "PropertyGuid",
            10: "PropertyString",
            11: "PropertyStyle",
            13: "PropertyLayout",
        }.get(value, "PropertyBlock")

    def component_file(self, root: dict) -> dict:
        output = copy.deepcopy(root)

        def convert(value) -> None:
            if isinstance(value, dict):
                if isinstance(value.get("InheritedType"), int):
                    value["InheritedType"] = self.inherited_to_component(value["InheritedType"])
                value.pop("coid", None)
                value.pop("cgid", None)
                for child in value.values():
                    convert(child)
            elif isinstance(value, list):
                for child in value:
                    convert(child)

        convert(output)
        return output

    def _screens_by_logical_name(self, screens: list[dict]) -> dict[str, dict]:
        expected_roots = {
            name: set(root_names)
            for name, root_names, _temporary, _scrolling in SCREEN_STATES
        }
        resolved: dict[str, dict] = {}
        for screen in screens:
            mounted_roots = {
                self._object_name(node)
                for node in walk_project_nodes(screen)
                if self._object_name(node) in ROOT_STATES
            }
            matches = [name for name, roots in expected_roots.items() if roots == mounted_roots]
            if len(matches) != 1:
                raise RuntimeError(
                    json.dumps(
                        {
                            "code": "studio_screen_mount_identity_mismatch",
                            "screen_guid": screen.get("guid"),
                            "mounted_roots": sorted(mounted_roots),
                            "matches": matches,
                        },
                        ensure_ascii=False,
                    )
                )
            resolved[matches[0]] = screen
        if set(resolved) != {item[0] for item in SCREEN_STATES}:
            raise RuntimeError(
                json.dumps(
                    {"code": "studio_screen_count_mismatch", "screens": sorted(resolved)},
                    ensure_ascii=False,
                )
            )
        return resolved

    @staticmethod
    def _component_master_selector(component: dict, index_path: list[int]) -> dict:
        node = component
        chain = [component]
        for index in index_path:
            node = node["children"][index]
            chain.append(node)
        return {
            "component_guid": component["guid"],
            "master_descendant_guid": node["guid"],
            "coid_path": [legacy_dotnet_string_hash(item["guid"]) for item in chain],
            "logical_name": ProjectBuilder._object_name(node),
        }

    def _component_visual_state_contracts(self, components: dict[str, dict]) -> dict[str, list[dict]]:
        specs = {
            "cmp_setting_toggle": {
                "on": [
                    ("icon", [0], [{"op": "lv_obj_set_style_text_color", "args": [215, 255, 0]}]),
                    ("value", [2], [
                        {"op": "lv_label_set_text", "args": ["已开启"]},
                        {"op": "lv_obj_set_style_text_color", "args": [215, 255, 0]},
                    ]),
                    ("arrow", [3], [{"op": "lv_obj_set_style_text_color", "args": [215, 255, 0]}]),
                ],
                "off": [
                    ("icon", [0], [{"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]}]),
                    ("value", [2], [
                        {"op": "lv_label_set_text", "args": ["已关闭"]},
                        {"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]},
                    ]),
                    ("arrow", [3], [{"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]}]),
                ],
            },
            "cmp_alert_row": {
                "warning": [
                    ("accent", [0], [
                        {"op": "lv_obj_set_style_bg_color", "args": [255, 214, 10]},
                        {"op": "lv_obj_set_style_bg_opa", "args": [255]},
                    ]),
                    ("icon", [1], [{"op": "lv_obj_set_style_text_color", "args": [255, 214, 10]}]),
                    ("label", [2], [{"op": "lv_obj_set_style_text_color", "args": [255, 255, 255]}]),
                    ("state", [3], [
                        {"op": "lv_label_set_text", "args": ["警告"]},
                        {"op": "lv_obj_set_style_text_color", "args": [255, 214, 10]},
                    ]),
                ],
                "danger": [
                    ("accent", [0], [
                        {"op": "lv_obj_set_style_bg_color", "args": [255, 69, 58]},
                        {"op": "lv_obj_set_style_bg_opa", "args": [255]},
                    ]),
                    ("icon", [1], [{"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]}]),
                    ("label", [2], [{"op": "lv_obj_set_style_text_color", "args": [255, 255, 255]}]),
                    ("state", [3], [
                        {"op": "lv_label_set_text", "args": ["危险"]},
                        {"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]},
                    ]),
                ],
            },
            "cmp_ble_candidate_row": {
                "default": [
                    ("container", [], [
                        {"op": "lv_obj_clear_state", "args": ["DISABLED"]},
                        {"op": "lv_obj_set_style_border_width", "args": [0]},
                    ]),
                    ("icon", [0], [{"op": "lv_obj_set_style_text_color", "args": [100, 210, 255]}]),
                    ("name", [1], [{"op": "lv_obj_set_style_text_color", "args": [255, 255, 255]}]),
                    ("status", [2], [{"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]}]),
                    ("arrow", [3], [{"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]}]),
                ],
                "connecting": [
                    ("container", [], [
                        {"op": "lv_obj_add_state", "args": ["DISABLED"]},
                        {"op": "lv_obj_set_style_border_width", "args": [1]},
                        {"op": "lv_obj_set_style_border_color", "args": [100, 210, 255]},
                    ]),
                    ("icon", [0], [{"op": "lv_obj_set_style_text_color", "args": [100, 210, 255]}]),
                    ("name", [1], [{"op": "lv_obj_set_style_text_color", "args": [255, 255, 255]}]),
                    ("status", [2], [
                        {"op": "lv_label_set_text", "args": ["连接中"]},
                        {"op": "lv_obj_set_style_text_color", "args": [100, 210, 255]},
                    ]),
                    ("arrow", [3], [{"op": "lv_obj_set_style_text_color", "args": [100, 210, 255]}]),
                ],
                "failed": [
                    ("container", [], [
                        {"op": "lv_obj_clear_state", "args": ["DISABLED"]},
                        {"op": "lv_obj_set_style_border_width", "args": [1]},
                        {"op": "lv_obj_set_style_border_color", "args": [255, 69, 58]},
                    ]),
                    ("icon", [0], [{"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]}]),
                    ("name", [1], [{"op": "lv_obj_set_style_text_color", "args": [255, 255, 255]}]),
                    ("status", [2], [
                        {"op": "lv_label_set_text", "args": ["连接失败"]},
                        {"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]},
                    ]),
                    ("arrow", [3], [{"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]}]),
                ],
            },
            "cmp_selftest_result_row": {
                "pass": [
                    ("icon", [0], [
                        {"op": "lv_label_set_text", "args": ["\ue226"]},
                        {"op": "lv_obj_set_style_text_color", "args": [48, 209, 88]},
                    ]),
                    ("state", [2], [
                        {"op": "lv_label_set_text", "args": ["通过"]},
                        {"op": "lv_obj_set_style_text_color", "args": [48, 209, 88]},
                    ]),
                ],
                "fail": [
                    ("icon", [0], [
                        {"op": "lv_label_set_text", "args": ["\ue084"]},
                        {"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]},
                    ]),
                    ("state", [2], [
                        {"op": "lv_label_set_text", "args": ["失败"]},
                        {"op": "lv_obj_set_style_text_color", "args": [255, 69, 58]},
                    ]),
                ],
                "skip": [
                    ("icon", [0], [
                        {"op": "lv_label_set_text", "args": ["\ue07e"]},
                        {"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]},
                    ]),
                    ("state", [2], [
                        {"op": "lv_label_set_text", "args": ["跳过"]},
                        {"op": "lv_obj_set_style_text_color", "args": [142, 152, 167]},
                    ]),
                ],
            },
        }
        result: dict[str, list[dict]] = {}
        for component_name, states in specs.items():
            component = components[component_name]
            result[component_name] = [
                {
                    "state": state,
                    "projections": [
                        {
                            "role": role,
                            "selector": self._component_master_selector(component, index_path),
                            "operations": operations,
                        }
                        for role, index_path, operations in projections
                    ],
                }
                for state, projections in states.items()
            ]
        return result

    def _selftest_lock_overlay(self, screen_by_name: dict[str, dict]) -> dict:
        control_bindings = []
        navigation_bindings = []
        for state, controls in SELFTEST_LOCK_CONTROLS.items():
            for control, source_object in controls.items():
                selector = self._descriptor_by_source(state, source_object)["target"]
                control_bindings.append(
                    {
                        "state": state,
                        "page_identity": STATE_CODES[state],
                        "control": control,
                        "selector": selector,
                        "apply_operations": [
                            {"op": "lv_obj_add_state", "args": ["DISABLED"]},
                            {"op": "lv_obj_set_style_opa", "args": [96]},
                        ],
                        "baseline_restore_operations": [
                            {"op": "lv_obj_clear_state", "args": ["DISABLED"]},
                            {"op": "lv_obj_set_style_opa", "args": [255]},
                        ],
                    }
                )
            for nav_intent, source_object in SHELL_NAV_SOURCES.items():
                selector = self._descriptor_by_source(state, source_object)["target"]
                navigation_bindings.append(
                    {
                        "state": state,
                        "navigation_intent": nav_intent,
                        "selector": selector,
                        "preserve_operations": [
                            {"op": "lv_obj_clear_state", "args": ["DISABLED"]},
                            {"op": "lv_obj_clear_flag", "args": ["HIDDEN"]},
                        ],
                    }
                )
        return {
            "activation_guard": "ble_connected == true && selftest_running == true",
            "connected_page_identity": {"control": "02-01", "mode_power": "03-01"},
            "disconnected_identity_is_not_overlay": {"control": "02-02", "mode_power": "03-02"},
            "controls": control_bindings,
            "navigation": {
                "pager_selector": self._stable_node_ref(screen_by_name["ui_scr_main_shell"]),
                "pager_preserve_operations": [
                    {"op": "lv_obj_clear_state", "args": ["DISABLED"]},
                    {"op": "lv_obj_clear_flag", "args": ["HIDDEN"]},
                ],
                "swipe_policy": "preserve_horizontal_scroll_and_page_change",
                "bindings": navigation_bindings,
            },
        }

    def _model_projection(self, project_nodes_by_guid: dict[str, dict]) -> dict[str, list[dict]]:
        projection: dict[str, list[dict]] = {}
        for root_name, fields in ROOT_MODEL_FIELDS.items():
            entries = []
            for field_name in fields:
                candidates = []
                for state, source_object in MODEL_EXPLICIT_BINDINGS.get(root_name, {}).get(field_name, []):
                    descriptor = self._descriptor_by_source(state, source_object)
                    selector = descriptor["target"]
                    guid_value = selector["guid"]
                    node = project_nodes_by_guid.get(guid_value, {})
                    if node.get("saved_objtypeKey") != "LABEL":
                        raise RuntimeError(
                            json.dumps(
                                {
                                    "code": "explicit_model_binding_not_label",
                                    "root": root_name,
                                    "field": field_name,
                                    "state": state,
                                    "source_object": source_object,
                                },
                                ensure_ascii=False,
                            )
                        )
                    candidates.append(
                        {
                            "state": state,
                            "source_object": source_object,
                            "selector": selector,
                            "operation": {"op": "lv_label_set_text", "args_template": ["{formatted_value}"]},
                        }
                    )
                value_format = MODEL_VALUE_FORMATS.get(field_name, "utf8_text")
                entry = {
                    "field": field_name,
                    "value_format": value_format,
                    "bindings": candidates,
                }
                if not entry["bindings"]:
                    resolver = {
                        "owner": "phase_3_binding",
                        "strategy": "resolve_field_without_claiming_an_ambiguous_static_selector",
                        "root": root_name,
                        "field": field_name,
                        "reason": "no_safe_one_to_one_static_label_binding_declared",
                        "must_emit": "stable_selector_plus_lvgl_operation",
                    }
                    intent_gate_reason = INTENT_GATE_MODEL_FIELDS.get(
                        (root_name, field_name)
                    )
                    if intent_gate_reason is not None:
                        resolver.update(
                            strategy="resolve_as_intent_gate_without_visual_disabled_state",
                            reason=intent_gate_reason,
                            must_emit="stable_selector_plus_intent_operation",
                        )
                    entry["required_phase3_resolver"] = resolver
                entries.append(entry)
            projection[root_name] = entries
        return projection

    def build(
        self,
        font_converter: Path,
        generate_fonts: bool,
        adopt_studio_save: bool = False,
        generated_font_dir: Path | None = None,
    ) -> None:
        components = self.reusable_components()
        roots = {root_name: self.page_root(root_name, components) for root_name in ROOT_STATES}
        screens = [
            self.screen(name, root_names, temporary, scrolling, roots, index)
            for index, (name, root_names, temporary, scrolling) in enumerate(SCREEN_STATES)
        ]
        self.collect_final_font_symbols([*components.values(), *screens])
        if adopt_studio_save:
            project_path = self.project_dir / "watch-lvgl.spj"
            saved_project = json.loads(project_path.read_text(encoding="utf-8"))
            saved_screens = saved_project.get("root", {}).get("children", [])
            self._screens_by_logical_name(saved_screens)
            self.write_manifest(components, roots, saved_screens, studio_save_adopted=True)
            self.write_event_stubs(components, saved_screens)
            self.write_fonts(font_converter, generate_fonts, generated_font_dir)
            return

        component_dir = self.project_dir / "components"
        component_dir.mkdir(parents=True, exist_ok=True)
        for stale in component_dir.glob("*.ecomp"):
            stale.unlink()
        for component_name, root in components.items():
            path = component_dir / f"{component_name}.ecomp"
            path.write_text(json.dumps(self.component_file(root), ensure_ascii=False, separators=(",", ":")), encoding="utf-8")

        root = {
            "guid": guid("watch-lvgl-root"),
            "children": screens,
            "properties": [self.prop("STARTEVENTS/Name", strval="___initial_actions0", inherited=10)],
            "saved_objtypeKey": "STARTEVENTS",
        }
        project = {
            "root": root,
            "animations": [],
            "selected_theme": "Default",
            "selected_screen": screens[0]["guid"],
            "info": {
                "name": "watch-lvgl.spj",
                "depth": 2,
                "width": CANVAS_WIDTH,
                "height": CANVAS_HEIGHT,
                "rotation": 0,
                "offset_x": 0,
                "offset_y": 0,
                "shape": "RECTANGLE",
                "multilang": "DISABLE",
                "description": "阶段二：23 个正式状态、7 个整页变体、13 个页面根对象、4 个复用组件、9 个 Screen",
                "board": "Eclipse with SDL for development on PC",
                "board_version": "v1.0.2",
                "editor_version": "1.6.1",
                "image": "",
                "export_temp_image": False,
                "force_export_images": False,
                "flat_export": False,
                "advanced_alpha": False,
                "pointfilter": False,
                "theme_simplified": True,
                "theme_dark": True,
                "theme_color1": 13,
                "theme_color2": 0,
                "custom_variable_prefix": "uic",
                "separate_screen_save": False,
                "hierarchy_state_save": False,
                "reverse_event_order": False,
                "backup_cnt": 5,
                "autosave_cnt": 0,
                "group_color_cnt": len(PALETTE),
                "imagebytearrayprefix": None,
                "lvgl_version": "8.3.11",
                "callfuncsexport": "C_FILE",
                "imageexport": "SOURCE",
                "lvgl_include_path": None,
                "naming": "Name",
                "naming_force_lowercase": False,
                "naming_add_subcomponent": False,
                "nidcnt": self.nid_counter + 1,
                "BitDepth": 16,
                "Name": "watch-lvgl",
            },
        }
        (self.project_dir / "watch-lvgl.spj").write_text(json.dumps(project, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        (self.project_dir / "assets" / "palette.json").write_text(json.dumps(PALETTE, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        self.write_manifest(components, roots, screens, studio_save_adopted=False)
        self.write_event_stubs(components, screens)
        self.write_fonts(font_converter, generate_fonts, generated_font_dir)

        sll_path = self.project_dir / "watch-lvgl.sll"
        if sll_path.exists():
            info = json.loads(sll_path.read_text(encoding="utf-8"))
            info["description"] = project["info"]["description"]
            info["editor_version"] = "1.6.1"
            info["width"] = CANVAS_WIDTH
            info["height"] = CANVAS_HEIGHT
            info["shape"] = "RECTANGLE"
            info["lvgl_version"] = "8.3.11"
            info["BitDepth"] = 16
            info["nidcnt"] = self.nid_counter + 1
            sll_path.write_text(json.dumps(info, ensure_ascii=False, indent=4), encoding="utf-8")

    def write_manifest(
        self,
        components: dict[str, dict],
        roots: dict[str, dict],
        screens: list[dict],
        studio_save_adopted: bool,
    ) -> None:
        def count_nodes(node: dict) -> int:
            return 1 + sum(count_nodes(child) for child in node.get("children", []))

        screen_by_root = {
            root_name: screen_name
            for screen_name, root_names, _temporary, _scrolling in SCREEN_STATES
            for root_name in root_names
        }
        screen_by_name = self._screens_by_logical_name(screens)
        screen_guid_by_name = {name: screen["guid"] for name, screen in screen_by_name.items()}
        screen_saved_name_by_name = {name: self._object_name(screen) for name, screen in screen_by_name.items()}
        root_by_state = {
            state: root_name
            for root_name, states in ROOT_STATES.items()
            for state in states
        }
        canonical_states = set(PAGE_STATES.values())
        variant_states = set(VARIANT_STATES.values())
        state_registry = []
        for state in [*PAGE_STATES.values(), *VARIANT_STATES.values()]:
            root_name = root_by_state[state]
            authoring_host_screen = screen_by_root[root_name]
            screen_name = None if root_name == "ui_modal_clear_binding_root" else authoring_host_screen
            lifecycle = (
                "on_demand_modal"
                if root_name == "ui_modal_clear_binding_root"
                else "resident_shell_slot"
                if screen_name == "ui_scr_main_shell"
                else "on_demand_screen"
            )
            state_registry.append(
                {
                    "canonical_id": STATE_CODES[state],
                    "state_key": state,
                    "pen_node_id": STATE_SOURCE_IDS[state],
                    "page_code": STATE_CODES[state],
                    "state": state,
                    "kind": "canonical" if state in canonical_states else "full_page_variant",
                    "pencil_node_id": STATE_SOURCE_IDS[state],
                    "source_identity": f"[UI][{'PAGE' if state in canonical_states else 'VAR'}][{STATE_CODES[state]}][{state}]",
                    "screen": screen_name,
                    "authoring_template_host_screen": authoring_host_screen if screen_name is None else None,
                    "page_state_root_object": root_name,
                    "root": root_name,
                    "lifecycle": lifecycle,
                    "model_fields": ROOT_MODEL_FIELDS[root_name],
                    "events": self._manifest_event_names(state),
                    "route_refs": ROOT_ROUTE_REFS[root_name],
                    "component_refs": sorted(self.state_component_refs.get(state, [])),
                    "base_state": VARIANT_BASE_STATES.get(state),
                    "state_patch": self.state_patches[state],
                }
            )

        screen_counts = {name: count_nodes(screen_by_name[name]) for name, *_rest in SCREEN_STATES}
        resident_widgets = screen_counts["ui_scr_main_shell"]
        temporary_counts = [count for name, count in screen_counts.items() if name != "ui_scr_main_shell"]
        screen_subtree_widgets = sum(screen_counts.values())
        component_master_widgets = sum(count_nodes(component) for component in components.values())
        project_widgets = screen_subtree_widgets + component_master_widgets
        editor_visible_widget_counter = screen_subtree_widgets - len(screens)
        peak_widgets = resident_widgets + max(temporary_counts)
        modal_template_widgets = count_nodes(roots["ui_modal_clear_binding_root"])

        project_nodes_by_guid = {
            node["guid"]: node
            for screen in screen_by_name.values()
            for node in walk_project_nodes(screen)
            if node.get("guid")
        }
        component_visual_contracts = self._component_visual_state_contracts(components)
        event_bindings = self._event_bindings(project_nodes_by_guid)
        selftest_lock_overlay = self._selftest_lock_overlay(screen_by_name)
        model_projection = self._model_projection(project_nodes_by_guid)

        component_instance_counts = {component_name: 0 for component_name in components}
        component_guid_to_name = {root["guid"]: name for name, root in components.items()}
        for screen in screen_by_name.values():
            stack = [screen]
            while stack:
                node = stack.pop()
                component_name = component_guid_to_name.get(node.get("cid"))
                if component_name:
                    component_instance_counts[component_name] += 1
                stack.extend(node.get("children", []))

        manifest = {
            "schema_version": 2,
            "source": {
                "html": "../watch-lvgl.html",
                "pencil": "../watch-lvgl.pen",
                "identity_policy": "优先 data-pencil-id；HTML 未携带 ID 时使用 [UI][PAGE|VAR][page_code][state] 结构键，不使用中文标题匹配。",
                "html_identity_mode": (
                    "all_explicit_pencil_ids"
                    if all(self.state_pages[state].node_id for state in STATE_SOURCE_IDS)
                    else "structured_identity_with_locked_pencil_id_map"
                ),
                "locked_pencil_id_count": len(STATE_SOURCE_IDS),
                "studio_identity_policy": "逻辑合同使用 logical_name；SquareLine 对象解析使用稳定 GUID，组件实例 descendant 再以 cid/cgid/coid_path 交叉校验。",
            },
            "editor": {
                "squareline_version": "1.6.1",
                "license_at_generation": "personal",
                "license_observation": "save_dialog_reports_150_widget_personal_limit",
                "trial_expiry": "not_observed_in_static_files",
                "trial_full_feature_validation": False,
                "trial_status_at_validation": "not_active_personal_license_observed",
                "trial_supports_current_project_if_active": True,
                "current_session_trial_supports_current_project": False,
                "gui_save_roundtrip": "blocked_by_personal_license_over_150_widgets",
                "gui_partial_normalization_adopted": studio_save_adopted,
                "commercial_release_requires_appropriate_license": True,
                "save_normalization_contract": {
                    "screen_order": "non_semantic_resolve_by_studio_guid",
                    "screen_name": "logical_name_authoritative_legacy_alias_only_with_legacy_guid",
                    "component_instance_root_name": "studio_may_normalize_to_master_name_resolve_by_guid_cid_cgid_coid_path",
                    "property_nid": "serializer_property_identity_may_repeat_and_is_not_an_object_key",
                    "component_instance_visuals": "state_patch_projection_is_authoritative_when_studio_restores_master_defaults",
                    "component_instance_events": "inherit_component_master_event_without_instance_override",
                },
            },
            "canvas": {
                "width": CANVAS_WIDTH,
                "height": CANVAS_HEIGHT,
                "shape": VISIBLE_SHAPE,
                "corner_radius": CORNER_RADIUS,
                "critical_content_clearance": CRITICAL_CONTENT_CLEARANCE,
                "color_depth": "RGB565",
                "lvgl": "8.3.11",
                "squareline": "1.6.1",
            },
            "mount_model": {
                "resident_screen": "ui_scr_main_shell",
                "resident_screen_guid": screen_guid_by_name["ui_scr_main_shell"],
                "resident_shell_roots": SCREEN_STATES[0][1],
                "on_demand_screens": [name for name, _roots, temporary, _scrolling in SCREEN_STATES if temporary],
                "on_demand_modal": {
                    "page_state_root_object": "ui_modal_clear_binding_root",
                    "authoring_template_host_screen": "ui_scr_maintenance",
                    "authoring_template_hidden": True,
                    "runtime_initial_state": "template_detached_after_host_init",
                    "runtime_create": "clone_template_on_clear_binding_click_then_clear_hidden_before_display",
                    "runtime_clone_steps": ["clone_template", "clear_hidden", "attach_to_active_layer", "display"],
                    "runtime_release": ["cancel", "clear_binding_completed", "host_screen_release"],
                    "runtime_owner": "phase_3_binding",
                },
                "screens": [
                    {
                        "name": name,
                        "logical_name": name,
                        "studio_guid": screen_guid_by_name[name],
                        "studio_saved_name": screen_saved_name_by_name[name],
                        "temporary": temporary,
                        "page_state_root_objects": root_names,
                    }
                    for name, root_names, temporary, _scrolling in SCREEN_STATES
                ],
            },
            "state_patch_order": PATCH_ORDER,
            "state_registry": state_registry,
            "component_registry": [
                {
                    "squareline_component": component_name,
                    "component_guid": components[component_name]["guid"],
                    "source_state": source_state,
                    "source_object": source_object,
                    "max_instances": max_instances,
                    "actual_instances": component_instance_counts[component_name],
                    "node_count": count_nodes(components[component_name]),
                    "visual_states": COMPONENT_VISUAL_EVIDENCE[component_name]["states"],
                    "visual_evidence": COMPONENT_VISUAL_EVIDENCE[component_name]["evidence"],
                    "visual_state_contracts": component_visual_contracts[component_name],
                }
                for component_name, (source_state, source_object, max_instances) in COMPONENT_SOURCES.items()
            ],
            "route_registry": {
                "owner": "phase_3_binding",
                "generated_layer_policy": "仅上报点击事件，不决定页面、不读取 BLE/NVS/service/watch_state。",
                "routes": ROUTE_DEFINITIONS,
                "concrete_routes_source": {"pencil_node_id": "yEaCt", "count": len(self.concrete_routes)},
                "concrete_routes": self.concrete_routes,
                "required_fact_snapshot": REQUIRED_FACT_SNAPSHOT,
                "fact_alias_normalization": FACT_ALIAS_NORMALIZATION,
                "event_bindings": event_bindings,
                "success_events": {
                    "ble_reconnect_succeeded": "refresh_binding_ble_payment_selftest_alerts_and_preserve_shell_slot",
                    "binding_succeeded": "refresh_all_facts_then_resolve_slot_01",
                    "payment_verification_succeeded": "refresh_all_facts_then_resolve_slot_01",
                },
                "fact_read_failure": "preserve_current_page_log_failure_and_schedule_fact_retry",
                "selftest_lock": {
                    "ble_connected": "keep_02-01_and_03-01_then_disable_controls",
                    "ble_disconnected": "use_01-03_02-02_03-02",
                    "disabled_controls": [
                        "gear_minus",
                        "gear_plus",
                        "assist_toggle",
                        "mode_eco",
                        "mode_extreme",
                        "mode_standard",
                        "mode_sport",
                        "poweroff",
                    ],
                    "preserved_navigation": ["main_shell_pager", "swipe", "page_navigation"],
                },
                "selftest_lock_overlay": selftest_lock_overlay,
                "model_projection": model_projection,
                "payment_lock_priority": "force_04-03_or_04-04_in_main_shell_preserve_full_settings_hierarchy",
                "payment_lock_settings_exception": "allow_settings_device_info_maintenance_selftest_and_clear_binding",
                "interaction_guards": {
                    "payment_verifying": ["disable_verify_submit", "reject_duplicate_submit"],
                    "clear_binding_clearing": ["disable_confirm", "disable_cancel", "reject_duplicate_clear"],
                    "ble_scanning": ["disable_rescan", "reject_duplicate_scan"],
                    "gps_submit_loading": ["disable_context_submit", "reject_duplicate_submit"],
                },
            },
            "code_only_registry": [
                {
                    "state": "selftest_amoled_five_color",
                    "lifecycle": "code_only_overlay",
                    "phase_3_object_name": "ui_selftest_amoled_surface",
                    "colors": ["black", "white", "red", "green", "blue"],
                    "duration_ms_each": 2000,
                    "total_duration_ms": 10000,
                    "screen": None,
                    "page_state_root_object": None,
                    "squareline_component": None,
                }
            ],
            "counts": {
                "canonical_states": len(PAGE_STATES),
                "full_page_variants": len(VARIANT_STATES),
                "screens": len(screens),
                "page_state_root_objects": len(ROOT_STATES),
                "squareline_components": len(components),
                "component_count": len(components),
                "code_only_states": 1,
                "studio_screen_subtree_widgets": screen_subtree_widgets,
                "studio_component_master_widgets": component_master_widgets,
                "studio_project_widgets": project_widgets,
                "runtime_resident_widgets": resident_widgets,
                "runtime_peak_static_widgets": peak_widgets,
            },
            "capacity": {
                "count_basis": "每个 SquareLine 对象树节点计 1；包含 Screen、页面根、包装容器、组件实例及 4 个组件 master。",
                "studio_project_widgets_formula": "studio_screen_subtree_widgets + studio_component_master_widgets",
                "runtime_peak_formula": "runtime_resident_widgets + max(single on-demand screen or modal authoring tree)",
                "screen_widget_counts": screen_counts,
                "modal_authoring_template_widgets": modal_template_widgets,
                "editor_visible_widget_counter": editor_visible_widget_counter,
                "project_widget_counter_including_screens_and_component_masters": project_widgets,
                "trial_supports_current_project_if_active": True,
                "current_session_trial_supports_current_project": False,
                "active_license": "personal",
                "active_license_supports_current_project": False,
                "studio_save_status": "blocked_by_personal_license_over_150_widgets",
                "personal_limits": {"screens": 10, "widgets": 150, "components": 1},
                "personal_supports_current_project": len(screens) <= 10 and project_widgets <= 150 and len(components) <= 1,
                "runtime_memory_32k_verified": False,
                "runtime_memory_note": "只记录静态对象数；32 KiB LVGL 内存、碎片与性能留待阶段三编译和真机验证。",
            },
            "font_coverage_audit": {
                "status": "machine_enforced",
                "severity": "error_on_missing_glyph",
                "contract": "font_glyph_contract.json",
                "validator": "tools/validate_font_coverage.py",
                "scope": "squareline_static_labels_runtime_label_contract_and_bounded_external_text",
                "generation_policy": "always_rebuild_and_sync_squareline_font_sources_to_firmware",
            },
            "blocking_warnings": [
                {
                    "code": "studio_save_roundtrip_blocked_by_license",
                    "status": "open",
                    "resolution": "activate_trial_or_appropriate_commercial_license_then_repeat_save_close_reopen",
                },
            ],
            "palette": PALETTE,
            "font_source_pins": {
                "google_fonts": "01848217e069afd63f72175b9b075ad9e07b8df8",
                "lucide_static_1_16_0": "2214caa407f4147449c81ac27e30d36edfb7b40f",
            },
        }
        (self.project_dir / "project_manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def write_event_stubs(self, components: dict[str, dict], screens: list[dict]) -> None:
        functions: list[str] = []

        def visit(value) -> None:
            if isinstance(value, dict):
                if value.get("strtype") == "CALL FUNCTION/Function_name":
                    function_name = value.get("strval", "")
                    if function_name and function_name not in functions:
                        functions.append(function_name)
                for child in value.values():
                    visit(child)
            elif isinstance(value, list):
                for child in value:
                    visit(child)

        for component in components.values():
            visit(component)
        for screen in screens:
            visit(screen)

        blocks = ['"""SquareLine generated 事件空桩；业务路由由阶段三 binding 负责。"""']
        for name in functions:
            blocks.append(f"def {name}(event_struct):\n    return")
        source = "\n\n".join(blocks) + "\n"
        (self.project_dir / "watch-lvgl_events.py").write_text(source, encoding="utf-8")

    @staticmethod
    def _object_name(node: dict) -> str:
        for item in node.get("properties", []):
            if item.get("strtype") == "OBJECT/Name":
                return item["strval"]
        return ""

    def collect_final_font_symbols(self, roots: list[dict]) -> None:
        """从最终 SquareLine 对象树回收文本和图标子集。

        组件实例会在 master 建立后覆写子文案；只采集 HTML 源对象
        会漏掉“点击音效”、“供电”等实例文本。运行时才写入标签的
        文案由 font_glyph_contract.json 按逻辑对象名补充。
        """

        def iter_properties(value):
            if isinstance(value, dict):
                if "strtype" in value:
                    yield value
                for child in value.values():
                    yield from iter_properties(child)
            elif isinstance(value, list):
                for child in value:
                    yield from iter_properties(child)

        fonts_by_label: dict[str, set[str]] = defaultdict(set)
        for root in roots:
            for node in walk_project_nodes(root):
                if node.get("saved_objtypeKey") != "LABEL":
                    continue
                properties = list(iter_properties(node.get("properties", [])))
                object_name = next(
                    (item.get("strval", "") for item in properties if item.get("strtype") == "OBJECT/Name"),
                    "",
                )
                text = next(
                    (item.get("strval", "") for item in properties if item.get("strtype") == "LABEL/Text"),
                    "",
                )
                font_code = next(
                    (
                        item.get("strval", "")
                        for item in properties
                        if item.get("strtype") == "_style/Text_Font"
                    ),
                    "",
                )
                if object_name and font_code:
                    fonts_by_label[object_name].add(font_code)
                text_match = re.fullmatch(r"(ns|mo)([0-9]+)_([0-9]+)", font_code)
                if text_match:
                    prefix, weight, size = text_match.groups()
                    family = "noto" if prefix == "ns" else "montserrat"
                    self.font_symbols[(family, int(weight), int(size))].update(text)
                    self.font_symbols[(family, int(weight), int(size))].update(ASCII_DYNAMIC)
                    continue
                icon_match = re.fullmatch(r"lucide_([0-9]+)", font_code)
                if icon_match:
                    self.icon_symbols[int(icon_match.group(1))].update(text)

        def walk_patch_descriptors(descriptor: dict):
            yield descriptor
            for child in descriptor.get("children", []):
                yield from walk_patch_descriptors(child)

        for state_key, patch in self.state_patches.items():
            for descriptor in patch.get("overrides", {}).values():
                for patched_object in walk_patch_descriptors(descriptor):
                    operations = patched_object.get("operations", [])
                    texts = [
                        operation.get("args", [""])[0]
                        for operation in operations
                        if operation.get("op") == "lv_label_set_text"
                    ]
                    font_codes = [
                        operation.get("args", [""])[0]
                        for operation in operations
                        if operation.get("op") == "lv_obj_set_style_text_font"
                    ]
                    selector = patched_object.get("target", {}).get(
                        "logical_name",
                        "",
                    )
                    if selector and len(font_codes) == 1:
                        fonts_by_label[selector].add(font_codes[0])
                    if not texts:
                        continue
                    if len(font_codes) != 1:
                        raise RuntimeError(
                            json.dumps(
                                {
                                    "code": "state_patch_font_not_unique",
                                    "state": state_key,
                                    "selector": patched_object.get("target", {}).get(
                                        "logical_name",
                                        "",
                                    ),
                                    "font_codes": font_codes,
                                },
                                ensure_ascii=False,
                            )
                        )
                    symbols = "".join(texts)
                    font_code = font_codes[0]
                    text_match = re.fullmatch(
                        r"(ns|mo)([0-9]+)_([0-9]+)",
                        font_code,
                    )
                    if text_match:
                        prefix, weight, size = text_match.groups()
                        family = "noto" if prefix == "ns" else "montserrat"
                        self.font_symbols[(family, int(weight), int(size))].update(
                            symbols
                        )
                        self.font_symbols[(family, int(weight), int(size))].update(
                            ASCII_DYNAMIC
                        )
                        continue
                    icon_match = re.fullmatch(r"lucide_([0-9]+)", font_code)
                    if icon_match:
                        self.icon_symbols[int(icon_match.group(1))].update(symbols)
                        continue
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "state_patch_font_unsupported",
                                "state": state_key,
                                "font_code": font_code,
                            },
                            ensure_ascii=False,
                        )
                    )

        contract_path = self.project_dir / FONT_GLYPH_CONTRACT
        contract = json.loads(contract_path.read_text(encoding="utf-8"))
        if contract.get("schema_version") != 1:
            raise RuntimeError(
                json.dumps(
                    {"code": "font_glyph_contract_schema_unsupported", "path": str(contract_path)},
                    ensure_ascii=False,
                )
        )
        for entry in contract.get("runtime_labels", []):
            selector = entry.get("selector", "")
            font_codes = fonts_by_label.get(selector, set())
            explicit_font_code = entry.get("font_code")
            explicit_font_codes = entry.get("font_codes")
            if explicit_font_code and explicit_font_codes:
                raise RuntimeError(
                    json.dumps(
                        {
                            "code": "runtime_font_selector_conflict",
                            "selector": selector,
                        },
                        ensure_ascii=False,
                    )
                )
            if explicit_font_codes is not None:
                if (
                    not isinstance(explicit_font_codes, list)
                    or not explicit_font_codes
                    or not all(
                        isinstance(font_code, str) and font_code
                        for font_code in explicit_font_codes
                    )
                ):
                    raise RuntimeError(
                        json.dumps(
                            {
                                "code": "runtime_font_codes_invalid",
                                "selector": selector,
                            },
                            ensure_ascii=False,
                        )
                    )
                runtime_font_codes = set(explicit_font_codes)
            elif explicit_font_code:
                runtime_font_codes = {explicit_font_code}
            else:
                runtime_font_codes = font_codes
            if not runtime_font_codes:
                raise RuntimeError(
                    json.dumps(
                        {
                            "code": "runtime_font_selector_missing",
                            "selector": selector,
                            "font_codes": sorted(font_codes),
                        },
                        ensure_ascii=False,
                    )
                )
            symbols = "".join(entry.get("texts", []))
            for font_code in sorted(runtime_font_codes):
                text_match = re.fullmatch(
                    r"(ns|mo)([0-9]+)_([0-9]+)",
                    font_code,
                )
                if text_match:
                    prefix, weight, size = text_match.groups()
                    family = "noto" if prefix == "ns" else "montserrat"
                    self.font_symbols[(family, int(weight), int(size))].update(
                        symbols
                    )
                    self.font_symbols[(family, int(weight), int(size))].update(
                        ASCII_DYNAMIC
                    )
                    continue
                icon_match = re.fullmatch(r"lucide_([0-9]+)", font_code)
                if icon_match:
                    self.icon_symbols[int(icon_match.group(1))].update(symbols)
                    continue
                raise RuntimeError(
                    json.dumps(
                        {
                            "code": "runtime_font_selector_unsupported",
                            "selector": selector,
                            "font_code": font_code,
                        },
                        ensure_ascii=False,
                    )
                )

    def collect_existing_project_font_symbols(self) -> None:
        """从当前 SquareLine 工程文件重建完整字体字形集合。"""

        project = json.loads((self.project_dir / "watch-lvgl.spj").read_text(encoding="utf-8"))
        roots = list(project.get("root", {}).get("children", []))
        for component_path in sorted((self.project_dir / "components").glob("*.ecomp")):
            component = json.loads(component_path.read_text(encoding="utf-8"))
            roots.append(component)
        manifest = json.loads(
            (self.project_dir / "project_manifest.json").read_text(encoding="utf-8")
        )
        self.state_patches = {
            entry["state_key"]: entry["state_patch"]
            for entry in manifest.get("state_registry", [])
            if entry.get("state_patch")
        }
        self.font_symbols.clear()
        self.icon_symbols.clear()
        self.collect_final_font_symbols(roots)

    def write_fonts(
        self,
        converter: Path,
        generate_fonts: bool,
        generated_font_dir: Path | None = None,
    ) -> None:
        font_dir = self.project_dir / "assets" / "Fonts"
        font_dir.mkdir(parents=True, exist_ok=True)
        definitions: list[tuple[str, Path, int, str]] = []
        for (family, weight, size), symbols in sorted(self.font_symbols.items()):
            code = f"ns{weight}_{size}" if family == "noto" else f"mo{weight}_{size}"
            source = font_dir / (f"NotoSansSC-{weight}.ttf" if family == "noto" else f"Montserrat-{weight}.ttf")
            definitions.append((code, source, size, "".join(sorted(symbols))))
        lucide_source = font_dir / "Lucide-1.16.0.ttf"
        for size, symbols in sorted(self.icon_symbols.items()):
            definitions.append((f"lucide_{size}", lucide_source, size, "".join(sorted(symbols))))

        expected = {f"ui_font_{code}.fcfg" for code, *_ in definitions}
        for stale in font_dir.glob("ui_font_*.fcfg"):
            if stale.name not in expected:
                stale.unlink()
        for code, source, size, symbols in definitions:
            stem = f"ui_font_{code}"
            is_lucide = code.startswith("lucide_")
            ranges = [] if is_lucide else ["0x20-0x7e"]
            subset_symbols = symbols if is_lucide else "".join(ch for ch in symbols if ord(ch) >= 0x80)
            config = {
                "codename": code,
                "ttf_path": f"/assets/Fonts/{source.name}",
                "bin_path": f"/assets/Fonts/{stem}.bin",
                "c_path": f"/assets/Fonts/{stem}.c",
                "cfg_path": f"/assets/Fonts/{stem}.fcfg",
                "size": size,
                "bpp": 4,
                "letters": 0,
                "ranges": ranges,
                "symbols": subset_symbols,
                "customparams": "--no-compress --no-prefilter",
                "uploaded": False,
            }
            (font_dir / f"{stem}.fcfg").write_text(json.dumps(config, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
            if not generate_fonts:
                continue
            common = [
                str(converter),
                "--size",
                str(size),
                "--bpp",
                "4",
                "--font",
                str(source),
            ]
            if ranges:
                common.extend(["-r", ranges[0]])
            if subset_symbols:
                if code.startswith("mo"):
                    fallback_weight = re.search(r"mo(\d+)_", code).group(1)
                    common.extend(["--font", str(font_dir / f"NotoSansSC-{fallback_weight}.ttf"), "--symbols", subset_symbols])
                else:
                    common.extend(["--symbols", subset_symbols])
            common.extend(["--no-compress", "--no-prefilter"])
            bin_path = font_dir / f"{stem}.bin"
            c_path = font_dir / f"{stem}.c"
            self.run_font_converter(common + ["--format", "bin", "-o", str(bin_path)], bin_path)
            self.run_font_converter(
                common
                + [
                    "--format",
                    "lvgl",
                    "--lv-font-name",
                    stem,
                    "-o",
                    str(c_path),
                ],
                c_path,
            )
            if generated_font_dir is not None:
                generated_font_dir.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(c_path, generated_font_dir / c_path.name)

    @staticmethod
    def run_font_converter(command: list[str], output: Path) -> None:
        # 字形集合变化时旧产物无法自行失效；字体是离线生成资源，始终重建
        # 才能保证 SquareLine 配置、bin 与固件 C 源三者一致。
        output.unlink(missing_ok=True)
        last_error: subprocess.CalledProcessError | None = None
        for attempt in range(4):
            try:
                subprocess.run(command, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                return
            except subprocess.CalledProcessError as error:
                last_error = error
                output.unlink(missing_ok=True)
                time.sleep(0.25 * (attempt + 1))
        if last_error is not None:
            raise last_error


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--html", type=Path, required=True)
    parser.add_argument("--project-dir", type=Path, required=True)
    parser.add_argument(
        "--font-converter",
        type=Path,
        default=Path("/Applications/SquareLine_Studio.app/Contents/MacOS/lvgl/lv_font_conv-osx"),
    )
    parser.add_argument("--skip-font-binaries", action="store_true")
    parser.add_argument(
        "--fonts-only",
        action="store_true",
        help="仅从当前 .spj/.ecomp 重建字体，不改写 SquareLine 对象树和 manifest。",
    )
    parser.add_argument(
        "--sync-generated-fonts",
        type=Path,
        help="字体转换成功后同步 C 源到固件 generated/fonts 目录。",
    )
    parser.add_argument(
        "--adopt-studio-save",
        action="store_true",
        help="保留 SquareLine 已写入的 .spj，仅重建 GUID 稳定的 manifest 与事件空桩。",
    )
    args = parser.parse_args()
    try:
        lucide_map_path = args.project_dir / "assets" / "font_sources" / "lucide" / "codepoints-1.16.0.json"
        lucide_map = json.loads(lucide_map_path.read_text(encoding="utf-8"))
        builder = ProjectBuilder(args.project_dir, args.html, lucide_map)
        if args.fonts_only:
            builder.collect_existing_project_font_symbols()
            builder.write_fonts(
                args.font_converter,
                not args.skip_font_binaries,
                args.sync_generated_fonts,
            )
        else:
            builder.build(
                args.font_converter,
                not args.skip_font_binaries,
                adopt_studio_save=args.adopt_studio_save,
                generated_font_dir=args.sync_generated_fonts,
            )
        print(
            json.dumps(
                {
                    "ok": True,
                    "canonical_states": len(PAGE_STATES),
                    "full_page_variants": len(VARIANT_STATES),
                    "page_state_root_objects": len(ROOT_STATES),
                    "components": len(COMPONENT_SOURCES),
                    "screens": len(SCREEN_STATES),
                    "text_fonts": len(builder.font_symbols),
                    "icon_fonts": len(builder.icon_symbols),
                    "last_nid": builder.nid_counter,
                    "studio_save_adopted": args.adopt_studio_save,
                },
                ensure_ascii=False,
            )
        )
    except Exception as error:  # 生成器对用户输出结构化错误，不暴露 traceback。
        try:
            details = json.loads(str(error))
        except json.JSONDecodeError:
            details = {"code": "generation_failed", "message": str(error)}
        print(json.dumps({"ok": False, "errors": [details]}, ensure_ascii=False, indent=2))
        raise SystemExit(1)


if __name__ == "__main__":
    main()
