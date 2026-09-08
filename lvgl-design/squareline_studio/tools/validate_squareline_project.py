#!/usr/bin/env python3
"""静态验证阶段二 Pencil HTML 与 SquareLine Studio 工程机器合同。"""

from __future__ import annotations

import argparse
import ast
import json
import re
from collections import Counter
from pathlib import Path

from generate_squareline_project import (
    COMPONENT_SOURCES,
    COMPONENT_VISUAL_EVIDENCE,
    FACT_ALIAS_NORMALIZATION,
    PAGE_STATES,
    PATCH_ORDER,
    RESIDENT_SCREEN_NAMES,
    REQUIRED_FACT_SNAPSHOT,
    ROOT_MODEL_FIELDS,
    ROOT_ROUTE_REFS,
    ROOT_STATES,
    SCREEN_GUID_NAMESPACE,
    SCREEN_STATES,
    SETTINGS_PAGE_2_CONTENT_OFFSET_Y,
    SETTINGS_PAGE_2_STATE_NAME,
    SETTINGS_PAGE_2_THUMB_Y,
    SELFTEST_LOCK_CONTROLS,
    SHELL_NAV_SOURCES,
    STATE_CODES,
    STATE_SOURCE_IDS,
    VARIANT_BASE_STATES,
    VARIANT_STATES,
    PencilHtmlParser,
    ProjectBuilder,
    guid,
    legacy_dotnet_string_hash,
    node_style,
    normalized_text,
    px,
)


EXPECTED_CANVAS_WIDTH = 410
EXPECTED_CANVAS_HEIGHT = 502
EXPECTED_VISIBLE_SHAPE = "rounded_rect"
EXPECTED_CORNER_RADIUS = 110
EXPECTED_CRITICAL_CLEARANCE = 8
TITLE_BEARING_STATES = {
    "settings",
    "device_info",
    "maintenance",
    "ble_empty",
    "ble_candidates",
    "ble_scanning",
    "selftest_idle",
    "selftest_progress",
    "selftest_retry_auto",
    "selftest_manual",
    "selftest_manual_touch",
    "selftest_manual_vibration",
    "selftest_result",
    "selftest_result_all_passed",
    "selftest_gps_context",
    "selftest_gps_submit_loading",
    "selftest_gps_submit_failed",
    "selftest_retry_detail",
}
EXPECTED_SHARED_TITLE_OBJECTS = 8
MODE_RESERVE_STATES = {"mode_default", "mode_disconnected"}
MODE_TITLE_RESERVE_OBJECT = "ui_lbl_current_mode"
MODE_LAYOUT_RESERVE_PREFIXES = ("ui_btn_mode_", "ui_icon_mode_")
SETTINGS_RAISE_WAKE_SOURCE_OBJECT = "ui_row_raise_wake_instance"
TWO_MODE_REFERENCE_STATES = {
    "[UI][STATE][mode_layout][two_modes] 模式与电源·两模式参考（标准/健身）": {
        "ui_btn_mode_standard": (16, 96, 181, 128),
        "ui_btn_mode_sport": (213, 96, 181, 128),
        "ui_btn_exo_poweroff": (16, 282, 378, 112),
    },
    "[UI][STATE][mode_layout][two_modes_disconnected] 模式与电源·两模式断连参考（标准/健身）": {
        "ui_btn_mode_standard_selected_disabled": (16, 96, 181, 128),
        "ui_btn_mode_sport_disabled": (213, 96, 181, 128),
        "ui_btn_exo_poweroff_disabled": (16, 282, 378, 112),
    },
}
DOCUMENTATION_EXPORT_MARKERS = (
    "[SPEC]",
    "[UI][STATE][mode_layout]",
    "来源入口｜",
    "owner_type｜",
    "canonical_page_id｜",
    "export_to_squareline｜",
    "规范映射字段",
    "ui_rail_two_mode_reference",
    "ui_rail_two_mode_disconnected_reference",
    "ui_two_mode_nav_reference",
    "ui_two_mode_disconnected_nav_reference",
)


# 与生成器刻意分离的审查基准：防止“生成器猜错、验证器照抄”形成同源误报。
INDEPENDENT_MODEL_BINDING_EXPECTATIONS = {
    ("ui_page_home_root", "time"): {
        ("home_normal", "ui_txt_time"),
        ("home_unbound", "ui_txt_time"),
        ("home_disconnected", "ui_txt_time"),
    },
    ("ui_page_home_root", "watch_battery"): {
        ("home_normal", "ui_rail_watch_battery_value"),
        ("home_unbound", "ui_rail_watch_battery_value"),
        ("home_disconnected", "ui_rail_watch_battery_value"),
    },
    ("ui_page_home_root", "exo_battery"): {("home_normal", "ui_val_exo_battery")},
    ("ui_page_home_root", "gear"): {("home_normal", "ui_val_gear")},
    ("ui_page_home_root", "steps"): {("home_normal", "ui_val_steps")},
    ("ui_page_control_root", "gear"): {("control_connected", "ui_val_actual_gear")},
    ("ui_page_mode_power_root", "drive_mode"): {("mode_default", "ui_lbl_current_mode")},
    ("ui_page_device_info_root", "firmware_version"): {("device_info", "ui_row_firmware_value")},
    ("ui_page_maintenance_root", "cellular_status"): {("maintenance", "ui_row_cellular_value")},
    ("ui_page_maintenance_root", "gps_status"): {("maintenance", "ui_row_gps_value")},
    ("ui_page_maintenance_root", "selftest_status"): {("maintenance", "ui_row_selftest_value")},
    ("ui_page_selftest_progress_root", "selftest_item"): {
        ("selftest_progress", "ui_val_current_item"),
        ("selftest_retry_auto", "ui_val_current_item"),
        ("selftest_manual", "ui_val_manual_item"),
        ("selftest_manual_touch", "ui_val_manual_item"),
    },
    ("ui_page_selftest_progress_root", "selftest_progress"): {("selftest_progress", "ui_val_progress")},
    ("ui_page_selftest_progress_root", "manual_prompt"): {
        ("selftest_manual", "ui_txt_manual_question"),
        ("selftest_manual_touch", "ui_txt_manual_question"),
    },
    ("ui_page_selftest_progress_root", "manual_countdown"): {
        ("selftest_manual", "ui_val_manual_countdown"),
        ("selftest_manual_touch", "ui_val_manual_countdown"),
    },
    ("ui_page_selftest_result_root", "pass_count"): {
        ("selftest_result", "ui_txt_result_pass_count"),
        ("selftest_result_all_passed", "ui_txt_result_pass_count"),
    },
    ("ui_page_selftest_result_root", "failure_count"): {("selftest_result", "ui_txt_result_failure_count")},
    ("ui_page_selftest_result_root", "skip_count"): {
        ("selftest_result", "ui_txt_result_skip_count"),
        ("selftest_result_all_passed", "ui_txt_result_skip_count"),
    },
    ("ui_page_selftest_retry_detail_root", "selected_failed_item"): {
        ("selftest_retry_detail", "ui_val_retry_item")
    },
}

COMPONENT_REQUIRED_VISUAL_ROLES = {
    "cmp_setting_toggle": {"icon", "value", "arrow"},
    "cmp_alert_row": {"accent", "icon", "label", "state"},
    "cmp_ble_candidate_row": {"container", "icon", "name", "status", "arrow"},
    "cmp_selftest_result_row": {"icon", "state"},
}

COMPONENT_MASTER_CHILD_COUNTS = {
    "cmp_setting_toggle": 4,
    "cmp_alert_row": 4,
    "cmp_ble_candidate_row": 4,
    "cmp_selftest_result_row": 3,
}


def property_of(node: dict, strtype: str) -> dict | None:
    if not isinstance(node, dict) or not isinstance(node.get("properties", []), list):
        return None
    return next(
        (
            item
            for item in node.get("properties", [])
            if isinstance(item, dict) and item.get("strtype") == strtype
        ),
        None,
    )


def object_name(node: dict) -> str:
    item = property_of(node, "OBJECT/Name")
    return item.get("strval", "") if item else ""


def walk_nodes(node: dict):
    if not isinstance(node, dict):
        return
    yield node
    children = node.get("children", [])
    if isinstance(children, list):
        for child in children:
            if isinstance(child, dict):
                yield from walk_nodes(child)


def walk_nodes_with_parent(node: dict, parent: dict | None = None):
    if not isinstance(node, dict):
        return
    yield node, parent
    children = node.get("children", [])
    if isinstance(children, list):
        for child in children:
            if isinstance(child, dict):
                yield from walk_nodes_with_parent(child, node)


def walk_dicts(value):
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from walk_dicts(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk_dicts(child)


def style_contract_value(node: dict, style_property: str, value_property: str, field: str):
    style = property_of(node, style_property)
    if style is None:
        return None
    value = next(
        (item for item in walk_dicts(style.get("childs", [])) if item.get("strtype") == value_property),
        None,
    )
    return value.get(field) if value else None


def object_size(node: dict) -> list[int] | None:
    size = property_of(node, "OBJECT/Size")
    return size.get("intarray") if size else None


def object_position(node: dict) -> list[int] | None:
    position = property_of(node, "OBJECT/Position")
    return position.get("intarray") if position else None


def walk_html(node):
    yield node
    for child in node.children:
        yield from walk_html(child)


def validate_device_info_html_contract(root, errors: list[dict]) -> None:
    nodes = {node.name: node for node in walk_html(root)}
    expected = {
        "ui_row_firmware_value": {"text": "1.0.0"},
        "ui_row_watch_id_label": {"x": 66, "width": 160},
        "ui_row_watch_id_value": {
            "x": 220,
            "width": 142,
        },
        "ui_row_bound_mac_label": {"x": 66, "width": 130},
        "ui_row_bound_mac_value": {
            "x": 198,
            "width": 164,
        },
        "ui_row_left_leg_position_label": {"x": 66, "width": 114},
        "ui_row_left_leg_position_value": {"x": 66, "width": 114, "text": "-2°"},
        "ui_row_right_leg_position_label": {"x": 208, "width": 136},
        "ui_row_right_leg_position_value": {"x": 208, "width": 136, "text": "20°"},
    }
    failures: list[dict] = []
    for name, contract in expected.items():
        node = nodes.get(name)
        if node is None:
            failures.append({"name": name, "actual": None})
            continue
        style = node_style(node)
        actual = {
            key: (
                normalized_text(node)
                if key == "text"
                else px(style.get("left"))
                if key == "x"
                else px(style.get("width"))
                if key == "width"
                else style.get("overflow")
            )
            for key in contract
        }
        if actual != contract:
            failures.append({"name": name, "expected": contract, "actual": actual})
    if failures:
        add_error(
            errors,
            "device_info_html_contract",
            "05-02 HTML 必须固定设备标识、版本占位与左右腿位置区域",
            failures=failures,
        )


def validate_device_info_project_contract(project_nodes: list[dict], errors: list[dict]) -> None:
    nodes = {object_name(node): node for node in project_nodes}
    expected = {
        "ui_device_info_row_firmware_value": {
            "position": [238, 29],
            "size": [124, 23],
            "text": "1.0.0",
        },
        "ui_device_info_row_watch_id_label": {
            "position": [66, 27],
            "size": [160, 27],
        },
        "ui_device_info_row_watch_id_value": {
            "position": [220, 31],
            "size": [142, 18],
            "long_mode": "CLIP",
        },
        "ui_device_info_row_bound_mac_label": {
            "position": [66, 29],
            "size": [130, 22],
        },
        "ui_device_info_row_bound_mac_value": {
            "position": [198, 31],
            "size": [164, 18],
            "long_mode": "CLIP",
        },
        "ui_device_info_row_left_leg_position_label": {
            "position": [66, 11],
            "size": [114, 22],
            "text": "左腿位置",
        },
        "ui_device_info_row_left_leg_position_value": {
            "position": [66, 40],
            "size": [114, 23],
            "text": "-2°",
        },
        "ui_device_info_row_right_leg_position_label": {
            "position": [208, 11],
            "size": [136, 22],
            "text": "右腿位置",
        },
        "ui_device_info_row_right_leg_position_value": {
            "position": [208, 40],
            "size": [136, 23],
            "text": "20°",
        },
    }
    failures: list[dict] = []
    for name, contract in expected.items():
        node = nodes.get(name)
        if node is None:
            failures.append({"name": name, "actual": None})
            continue
        actual = {
            key: (
                object_position(node)
                if key == "position"
                else object_size(node)
                if key == "size"
                else (property_of(node, "LABEL/Text") or {}).get("strval")
                if key == "text"
                else (property_of(node, "LABEL/Long_mode") or {}).get("strval")
            )
            for key in contract
        }
        if actual != contract:
            failures.append({"name": name, "expected": contract, "actual": actual})
    if failures:
        add_error(
            errors,
            "device_info_squareline_contract",
            "05-02 SquareLine 工程必须固定设备标识、版本占位与左右腿位置区域",
            failures=failures,
        )


def count_nodes(node: dict) -> int:
    return sum(1 for _ in walk_nodes(node))


def component_path_for(node: dict, parent_by_guid: dict[str, dict | None]) -> dict | None:
    chain: list[dict] = []
    current: dict | None = node
    component_root: dict | None = None
    while current is not None:
        chain.append(current)
        if current.get("cid"):
            component_root = current
            break
        current = parent_by_guid.get(current.get("guid"))
    if component_root is None:
        return None
    chain.reverse()
    return {
        "cid": component_root.get("cid"),
        "cgid": component_root.get("cgid"),
        "coid_path": [item.get("coid") for item in chain],
    }


def collect_calls(source: dict) -> list[str]:
    names: list[str] = []

    def visit(value) -> None:
        if isinstance(value, dict):
            if value.get("strtype") == "CALL FUNCTION/Function_name" and value.get("strval"):
                names.append(value["strval"])
            for child in value.values():
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)

    visit(source)
    return names


def parse_json(path: Path, errors: list[dict]) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        errors.append({"code": "invalid_json", "file": str(path), "message": str(error)})
        return {}


def add_error(errors: list[dict], code: str, message: str, **details) -> None:
    errors.append({"code": code, "message": message, **details})


def rounded_safe_point(x: int, y: int) -> bool:
    clearance = EXPECTED_CRITICAL_CLEARANCE
    inner_radius = EXPECTED_CORNER_RADIUS - clearance
    if not (
        clearance <= x <= EXPECTED_CANVAS_WIDTH - clearance
        and clearance <= y <= EXPECTED_CANVAS_HEIGHT - clearance
    ):
        return False
    corner_checks = [
        (EXPECTED_CORNER_RADIUS, EXPECTED_CORNER_RADIUS, x < EXPECTED_CORNER_RADIUS and y < EXPECTED_CORNER_RADIUS),
        (
            EXPECTED_CANVAS_WIDTH - EXPECTED_CORNER_RADIUS,
            EXPECTED_CORNER_RADIUS,
            x > EXPECTED_CANVAS_WIDTH - EXPECTED_CORNER_RADIUS and y < EXPECTED_CORNER_RADIUS,
        ),
        (
            EXPECTED_CORNER_RADIUS,
            EXPECTED_CANVAS_HEIGHT - EXPECTED_CORNER_RADIUS,
            x < EXPECTED_CORNER_RADIUS and y > EXPECTED_CANVAS_HEIGHT - EXPECTED_CORNER_RADIUS,
        ),
        (
            EXPECTED_CANVAS_WIDTH - EXPECTED_CORNER_RADIUS,
            EXPECTED_CANVAS_HEIGHT - EXPECTED_CORNER_RADIUS,
            x > EXPECTED_CANVAS_WIDTH - EXPECTED_CORNER_RADIUS
            and y > EXPECTED_CANVAS_HEIGHT - EXPECTED_CORNER_RADIUS,
        ),
    ]
    return all(
        not applies or (x - center_x) ** 2 + (y - center_y) ** 2 <= inner_radius**2
        for center_x, center_y, applies in corner_checks
    )


def rect_intersection(
    first: tuple[int, int, int, int] | None,
    second: tuple[int, int, int, int],
) -> tuple[int, int, int, int] | None:
    if first is None:
        return second
    left = max(first[0], second[0])
    top = max(first[1], second[1])
    right = min(first[2], second[2])
    bottom = min(first[3], second[3])
    return None if right <= left or bottom <= top else (left, top, right, bottom)


def is_critical_html_node(node) -> bool:
    if "hidden" in node.attrs.get("class", "").split():
        return False
    name = node.name.lower()
    return bool(
        normalized_text(node)
        or node.tag == "svg"
        or re.search(r"(^|_)(btn|button|toggle|candidate|target|action|option|hitbox|touch)(_|$)", name)
        or name.startswith("ui_row_")
        or name.endswith("_row")
    )


def is_scroll_clip(node) -> bool:
    style = node_style(node)
    name = node.name.lower()
    return style.get("overflow") == "hidden" and ("scroll" in name or "list" in name)


def validate_state_geometry(
    state: str,
    root,
    errors: list[dict],
    expected_title_count_override: int | None = None,
) -> dict[str, int]:
    root_style = node_style(root)
    root_actual = {
        "width": px(root_style.get("width")),
        "height": px(root_style.get("height")),
        "corner_radius": px(root_style.get("border-radius")),
        "overflow": root_style.get("overflow"),
    }
    root_expected = {
        "width": EXPECTED_CANVAS_WIDTH,
        "height": EXPECTED_CANVAS_HEIGHT,
        "corner_radius": EXPECTED_CORNER_RADIUS,
        "overflow": "hidden",
    }
    if root_actual != root_expected:
        add_error(
            errors,
            "html_display_geometry",
            "HTML 页面/变体根必须保持 410×502、R110 并裁切四角",
            state=state,
            expected=root_expected,
            actual=root_actual,
        )

    violations: list[dict] = []
    title_violations: list[dict] = []
    critical_count = 0
    title_count = 0

    def visit(node, offset_x: int, offset_y: int, clip: tuple[int, int, int, int] | None) -> None:
        nonlocal critical_count, title_count
        style = node_style(node)
        left = offset_x + px(style.get("left"))
        top = offset_y + px(style.get("top"))
        width = px(style.get("width"))
        height = px(style.get("height"))
        box = (left, top, left + width, top + height)
        visible_box = rect_intersection(clip, box)

        if node.name == "ui_txt_page_title":
            title_count += 1
            center_x = left + width / 2
            if center_x != EXPECTED_CANVAS_WIDTH / 2 or style.get("text-align") != "center":
                title_violations.append(
                    {
                        "name": node.name,
                        "center_x": center_x,
                        "text_align": style.get("text-align"),
                        "box": box,
                    }
                )

        is_mode_layout_reserve = (
            state in MODE_RESERVE_STATES
            and node.name.startswith(MODE_LAYOUT_RESERVE_PREFIXES)
        )
        if is_critical_html_node(node) and not is_mode_layout_reserve:
            if width <= 0 or height <= 0:
                violations.append({"name": node.name, "reason": "zero_geometry", "box": box})
            elif visible_box is not None:
                critical_count += 1
                x1, y1, x2, y2 = visible_box
                if not all(
                    rounded_safe_point(x, y)
                    for x, y in ((x1, y1), (x2, y1), (x1, y2), (x2, y2))
                ):
                    violations.append({"name": node.name, "box": box, "visible_box": visible_box})

        child_clip = rect_intersection(clip, box) if is_scroll_clip(node) else clip
        if is_scroll_clip(node) and clip is not None and child_clip is None:
            return
        for child in node.children:
            visit(child, left, top, child_clip)

    # Pencil 把各画板排布在总文档坐标中；页面根的 left/top 不属于屏内坐标。
    for child in root.children:
        visit(child, 0, 0, None)

    if violations:
        add_error(
            errors,
            "html_critical_clearance",
            "关键文字、图标、操作或可见滚动内容未完全进入 8 px 圆角净空区域",
            state=state,
            violations=violations,
        )
    if title_violations:
        add_error(
            errors,
            "html_page_title_alignment",
            "ui_txt_page_title 必须以屏幕 x=205 为中心并使用居中文本对齐",
            state=state,
            violations=title_violations,
        )
    expected_title_count = (
        expected_title_count_override
        if expected_title_count_override is not None
        else 1 if state in TITLE_BEARING_STATES else 0
    )
    if title_count != expected_title_count:
        add_error(
            errors,
            "html_page_title_count",
            "每个登记状态必须精确符合标题存在合同，不能通过删除标题绕过居中校验",
            state=state,
            expected=expected_title_count,
            actual=title_count,
        )
    return {"critical_objects": critical_count, "page_titles": title_count}


def html_geometry_metadata(root) -> dict[str, dict[str, bool]]:
    metadata: dict[str, dict[str, bool]] = {}
    for node in walk_html(root):
        if not node.name:
            continue
        entry = metadata.setdefault(node.name, {"critical": False, "scroll": False, "title": False})
        entry["critical"] = entry["critical"] or is_critical_html_node(node)
        entry["scroll"] = entry["scroll"] or is_scroll_clip(node)
        entry["title"] = entry["title"] or node.name == "ui_txt_page_title"
    return metadata


def descriptor_operation(descriptor: dict, operation_name: str) -> list | None:
    matches = [
        operation.get("args")
        for operation in descriptor.get("operations", [])
        if isinstance(operation, dict) and operation.get("op") == operation_name
    ]
    return matches[0] if len(matches) == 1 and isinstance(matches[0], list) else None


def independent_font_line_height(project_dir: Path, font_code: str) -> int:
    """直接从 LVGL 字体资源读取行高，避免复用生成器的几何实现。"""
    size_match = re.search(r"_([0-9]+)$", font_code)
    fallback_size = int(size_match.group(1)) if size_match else 1
    path = project_dir / "assets" / "Fonts" / f"ui_font_{font_code}.c"
    if path.exists():
        source = path.read_text(encoding="utf-8", errors="ignore")
        line_match = re.search(r"\.line_height\s*=\s*(-?[0-9]+)", source)
        if line_match:
            return int(line_match.group(1))
    return max(1, (fallback_size * 12 + 5) // 10)


def independent_centered_line_geometry(
    node,
    project_dir: Path,
    font_code: str,
) -> tuple[list[int], list[int]]:
    """按 HTML 原始盒子和字体行高独立计算单行内容的 LVGL 几何。"""
    style = node_style(node)
    x, y = px(style.get("left")), px(style.get("top"))
    width, height = px(style.get("width"), 1), px(style.get("height"), 1)
    line_height = independent_font_line_height(project_dir, font_code)
    delta = height - line_height
    centered_offset = (delta + 1) // 2 if delta >= 0 else -((-delta + 1) // 2)
    return [x, y + centered_offset], [width, line_height]


def independent_raw_geometry(node) -> tuple[list[int], list[int]]:
    """直接返回 HTML 原始盒子，覆盖非居中文字和多行文字的保持规则。"""
    style = node_style(node)
    return (
        [px(style.get("left")), px(style.get("top"))],
        [px(style.get("width"), 1), px(style.get("height"), 1)],
    )


def validate_state_patch_geometry(
    state: str,
    html_root,
    overrides: dict,
    errors: list[dict],
    project_dir: Path,
) -> tuple[int, dict[str, dict[str, bool]]]:
    metadata = html_geometry_metadata(html_root)
    sources_by_name: dict[str, list] = {}
    for node in walk_html(html_root):
        if node.name:
            sources_by_name.setdefault(node.name, []).append(node)
    project_metadata: dict[str, dict[str, bool]] = {}
    violations: list[dict] = []
    title_violations: list[dict] = []
    vertical_violations: list[dict] = []
    critical_count = 0

    def visit(
        descriptor: dict,
        offset_x: int,
        offset_y: int,
        clip: tuple[int, int, int, int] | None,
    ) -> None:
        nonlocal critical_count
        source_object = descriptor.get("source_object", "")
        source_metadata = metadata.get(source_object)
        target = descriptor.get("target", {})
        target_guid = target.get("guid") if isinstance(target, dict) else None
        if (
            source_metadata is None
            and state in MODE_RESERVE_STATES
            and source_object == MODE_TITLE_RESERVE_OBJECT
        ):
            source_metadata = {
                "critical": False,
                "scroll": False,
                "title": False,
            }
        elif (
            source_metadata is not None
            and state in MODE_RESERVE_STATES
            and source_object.startswith(MODE_LAYOUT_RESERVE_PREFIXES)
        ):
            source_metadata = {**source_metadata, "critical": False}
        if source_metadata is None or not target_guid:
            violations.append(
                {"source_object": source_object, "target_guid": target_guid, "reason": "missing_source_identity"}
            )
            return
        target_metadata = project_metadata.setdefault(
            target_guid,
            {"critical": False, "scroll": False, "title": False},
        )
        for key in target_metadata:
            target_metadata[key] = target_metadata[key] or source_metadata[key]

        position = descriptor_operation(descriptor, "lv_obj_set_pos")
        size = descriptor_operation(descriptor, "lv_obj_set_size")
        if not position or len(position) != 2 or not size or len(size) != 2:
            violations.append(
                {"source_object": source_object, "target_guid": target_guid, "reason": "missing_pos_or_size"}
            )
            return
        font_args = descriptor_operation(descriptor, "lv_obj_set_style_text_font")
        if font_args and len(font_args) == 1 and isinstance(font_args[0], str):
            font_code = font_args[0]
            candidates = []
            for source in sources_by_name.get(source_object, []):
                source_style = node_style(source)
                is_lucide = (
                    source.tag == "svg"
                    and source.attrs.get("data-icon-name")
                    and font_code.startswith("lucide_")
                )
                is_flex_text = (
                    source.tag == "div"
                    and "font-family" in source_style
                    and "\\n" not in normalized_text(source)
                    and source_style.get("display") == "flex"
                    and source_style.get("flex-direction") == "column"
                    and source_style.get("justify-content") == "center"
                    and not font_code.startswith("lucide_")
                )
                if source_object == "ui_icon_assist_state" and is_lucide:
                    candidates.append(independent_raw_geometry(source))
                elif is_lucide or is_flex_text:
                    candidates.append(
                        independent_centered_line_geometry(source, project_dir, font_code)
                    )
                elif (
                    source.tag == "div"
                    and "font-family" in source_style
                    and not font_code.startswith("lucide_")
                ):
                    candidates.append(independent_raw_geometry(source))
            geometry_matches = any(
                position == expected_position and size == expected_size
                for expected_position, expected_size in candidates
            )
            if candidates and not geometry_matches:
                vertical_violations.append(
                    {
                        "source_object": source_object,
                        "target_guid": target_guid,
                        "font": font_code,
                        "actual": {"position": position, "size": size},
                        "expected": [
                            {"position": expected_position, "size": expected_size}
                            for expected_position, expected_size in candidates
                        ],
                    }
                )
        left = offset_x + position[0]
        top = offset_y + position[1]
        box = (left, top, left + size[0], top + size[1])
        visible_box = rect_intersection(clip, box)
        if source_metadata["critical"] and visible_box is not None:
            critical_count += 1
            x1, y1, x2, y2 = visible_box
            if not all(
                rounded_safe_point(x, y)
                for x, y in ((x1, y1), (x2, y1), (x1, y2), (x2, y2))
            ):
                violations.append(
                    {
                        "source_object": source_object,
                        "target_guid": target_guid,
                        "box": box,
                        "visible_box": visible_box,
                    }
                )
        if source_metadata["title"]:
            text_align = descriptor_operation(descriptor, "lv_obj_set_style_text_align")
            if left + size[0] / 2 != EXPECTED_CANVAS_WIDTH / 2 or text_align != ["CENTER"]:
                title_violations.append(
                    {
                        "source_object": source_object,
                        "target_guid": target_guid,
                        "box": box,
                        "text_align": text_align,
                    }
                )
        child_clip = rect_intersection(clip, box) if source_metadata["scroll"] else clip
        if source_metadata["scroll"] and clip is not None and child_clip is None:
            return
        for child in descriptor.get("children", []):
            if isinstance(child, dict):
                visit(child, left, top, child_clip)

    for descriptor in overrides.values():
        if isinstance(descriptor, dict):
            visit(descriptor, 0, 0, None)
    if violations:
        add_error(
            errors,
            "state_patch_critical_clearance",
            "manifest state_patch 应用后的最终 LVGL 几何必须满足 8 px 圆角净空",
            state=state,
            violations=violations,
        )
    if title_violations:
        add_error(
            errors,
            "state_patch_page_title_alignment",
            "state_patch 中的页面标题必须保持 x=205 与 CENTER",
            state=state,
            violations=title_violations,
        )
    if vertical_violations:
        add_error(
            errors,
            "state_patch_vertical_centering",
            "state_patch 中 flex 单行文字和 Lucide 图标必须按字体行高居中，其余文字保持原始盒子",
            state=state,
            violations=vertical_violations,
        )
    return critical_count, project_metadata


def validate_static_project_geometry(
    root_nodes: dict[str, dict],
    geometry_by_guid: dict[str, dict[str, bool]],
    errors: list[dict],
) -> dict[str, int]:
    violations: list[dict] = []
    title_violations: list[dict] = []
    critical_count = 0
    title_count = 0

    def visit(node: dict, offset_x: int, offset_y: int, clip: tuple[int, int, int, int] | None) -> None:
        nonlocal critical_count, title_count
        metadata = geometry_by_guid.get(node.get("guid"), {})
        position = object_size(node)
        pos_property = property_of(node, "OBJECT/Position")
        pos = pos_property.get("intarray") if pos_property else None
        if not pos or len(pos) != 2 or not position or len(position) != 2:
            if metadata.get("critical"):
                violations.append({"name": object_name(node), "reason": "missing_pos_or_size"})
            return
        left = offset_x + pos[0]
        top = offset_y + pos[1]
        box = (left, top, left + position[0], top + position[1])
        visible_box = rect_intersection(clip, box)
        if metadata.get("critical") and visible_box is not None:
            critical_count += 1
            x1, y1, x2, y2 = visible_box
            if not all(
                rounded_safe_point(x, y)
                for x, y in ((x1, y1), (x2, y1), (x1, y2), (x2, y2))
            ):
                violations.append({"name": object_name(node), "box": box, "visible_box": visible_box})
        if metadata.get("title"):
            title_count += 1
            alignments = [
                value.get("strval")
                for value in walk_dicts(node.get("properties", []))
                if value.get("strtype") == "_style/Text_Align"
            ]
            if left + position[0] / 2 != EXPECTED_CANVAS_WIDTH / 2 or alignments != ["CENTER"]:
                title_violations.append(
                    {"name": object_name(node), "box": box, "text_align": alignments}
                )
        child_clip = rect_intersection(clip, box) if metadata.get("scroll") else clip
        if metadata.get("scroll") and clip is not None and child_clip is None:
            return
        for child in node.get("children", []):
            visit(child, left, top, child_clip)

    for root in root_nodes.values():
        for child in root.get("children", []):
            visit(child, 0, 0, None)
    if violations:
        add_error(
            errors,
            "squareline_static_critical_clearance",
            "watch-lvgl.spj 静态对象几何必须满足 8 px 圆角净空",
            violations=violations,
        )
    if title_violations or title_count != EXPECTED_SHARED_TITLE_OBJECTS:
        add_error(
            errors,
            "squareline_static_page_title_alignment",
            "SquareLine 共享标题对象必须恰为 8 个且全部保持 x=205/CENTER",
            expected=EXPECTED_SHARED_TITLE_OBJECTS,
            actual=title_count,
            violations=title_violations,
        )
    return {"critical_objects": critical_count, "page_titles": title_count}


def validate_html(html_path: Path, errors: list[dict]) -> dict[str, int | str]:
    parser = PencilHtmlParser()
    parser.feed(html_path.read_text(encoding="utf-8"))
    found: dict[str, str] = {}
    explicit_pencil_ids = 0
    critical_objects = 0
    page_titles = 0
    for root in parser.roots:
        for node in walk_html(root):
            if "· Surface" in node.name:
                continue
            structured = ProjectBuilder.structured_source_name(node.name)
            if not structured:
                continue
            kind, page_code, state = structured
            if state not in STATE_SOURCE_IDS:
                add_error(
                    errors,
                    "unknown_html_page" if kind == "PAGE" else "unknown_html_variant",
                    "HTML 出现未登记的第 24 个页面或额外变体",
                    kind=kind,
                    page_code=page_code,
                    state=state,
                )
                continue
            if state in found:
                add_error(errors, "duplicate_html_source", "HTML 结构化状态键重复", state=state)
                continue
            found[state] = kind
            if node.node_id:
                explicit_pencil_ids += 1
            if node.node_id and node.node_id != STATE_SOURCE_IDS[state]:
                add_error(
                    errors,
                    "html_pencil_id_mismatch",
                    "HTML 显式携带的 Pencil 节点 ID 与锁定映射不一致",
                    state=state,
                    expected=STATE_SOURCE_IDS[state],
                    actual=node.node_id,
                )
            if page_code != STATE_CODES[state]:
                add_error(
                    errors,
                    "html_page_code_mismatch",
                    "HTML 页码与状态登记不一致",
                    state=state,
                    expected=STATE_CODES[state],
                    actual=page_code,
                )
            expected_kind = "PAGE" if state in PAGE_STATES.values() else "VAR"
            if kind != expected_kind:
                add_error(errors, "html_state_kind_mismatch", "HTML 状态类型错误", state=state, expected=expected_kind, actual=kind)
            if state == "device_info":
                validate_device_info_html_contract(node, errors)
            geometry = validate_state_geometry(state, node, errors)
            critical_objects += geometry["critical_objects"]
            page_titles += geometry["page_titles"]

    settings_page_2_states = [
        node
        for root in parser.roots
        for node in walk_html(root)
        if node.name == SETTINGS_PAGE_2_STATE_NAME
    ]
    if len(settings_page_2_states) != 1:
        add_error(
            errors,
            "settings_page_2_state_count",
            "HTML 必须精确包含一个设置第二页可见状态，但不得增加 Screen 或规范页面。",
            expected_name=SETTINGS_PAGE_2_STATE_NAME,
            actual=len(settings_page_2_states),
        )
    else:
        settings_page_2 = settings_page_2_states[0]
        nodes_by_name = {node.name: node for node in walk_html(settings_page_2) if node.name}
        required_names = {
            "ui_settings_list_page_1",
            "ui_settings_list_page_2",
            "ui_row_haptics_instance",
            "ui_row_click_audio_instance",
            "ui_row_brightness",
            "ui_row_screen_timeout",
            "ui_row_device_info",
            "ui_row_maintenance",
            "ui_gesture_zone_settings_return",
            "ui_hint_pull_up_handle",
        }
        content = nodes_by_name.get("ui_settings_list_content")
        thumb = nodes_by_name.get("ui_settings_page_thumb")
        missing = sorted(required_names - set(nodes_by_name))
        content_y = None if content is None else px(node_style(content).get("top"))
        thumb_y = None if thumb is None else px(node_style(thumb).get("top"))
        if (
            missing
            or content_y != SETTINGS_PAGE_2_CONTENT_OFFSET_Y
            or thumb_y != SETTINGS_PAGE_2_THUMB_Y
        ):
            add_error(
                errors,
                "settings_page_2_visual_contract",
                "设置第二页必须显示顺延后的设备信息与维护，并保持底部返回手势区。",
                missing=missing,
                content_y=content_y,
                thumb_y=thumb_y,
            )
        validate_state_geometry(
            "settings_page_2",
            settings_page_2,
            errors,
            expected_title_count_override=1,
        )

    missing = sorted(set(STATE_SOURCE_IDS) - set(found))
    if missing:
        add_error(errors, "missing_html_sources", "HTML 缺少结构化页面或变体", states=missing)
    if page_titles != len(TITLE_BEARING_STATES):
        add_error(
            errors,
            "html_page_title_total",
            "HTML 必须精确包含 18 个登记状态标题",
            expected=len(TITLE_BEARING_STATES),
            actual=page_titles,
        )
    return {
        "identity_mode": (
            "all_explicit_pencil_ids"
            if len(found) == len(STATE_SOURCE_IDS) and explicit_pencil_ids == len(STATE_SOURCE_IDS)
            else "structured_identity_with_locked_pencil_id_map"
        ),
        "explicit_pencil_ids": explicit_pencil_ids,
        "critical_objects": critical_objects,
        "page_titles": page_titles,
    }


def html_concrete_routes(html_path: Path) -> list[dict[str, str]]:
    parser = PencilHtmlParser()
    parser.feed(html_path.read_text(encoding="utf-8"))
    route_card = next(
        (
            node
            for root in parser.roots
            for node in walk_html(root)
            if node.node_id == "yEaCt" or node.name == "[SPEC][ROUTE][global] 全局页面切换规则"
        ),
        None,
    )
    if route_card is None:
        return []
    routes: list[dict[str, str]] = []
    for section in route_card.children:
        for line in normalized_text(section).split("\\n"):
            if not line.startswith("route_"):
                continue
            fields = [field.strip() for field in line.split("｜")]
            if len(fields) == 7:
                routes.append(
                    dict(
                        zip(
                            ["route_id", "from", "event", "guard", "to", "fallback", "history_policy"],
                            fields,
                            strict=True,
                        )
                    )
                )
    return routes


def validate_events(project_dir: Path, sources: list[dict], errors: list[dict]) -> int:
    path = project_dir / "watch-lvgl_events.py"
    text = path.read_text(encoding="utf-8")

    defined: set[str] = set()
    try:
        tree = ast.parse(text, filename=str(path))
        forbidden_symbols = {
            "watch_state",
            "nvs_get",
            "nvs_set",
            "ble_service",
            "payment_service",
            "selftest_service",
            "screen_load",
            "load_scr",
            "scroll_to",
        }
        dependencies = sorted(
            {
                node.id
                for node in ast.walk(tree)
                if isinstance(node, ast.Name) and node.id in forbidden_symbols
            }
            | {
                node.attr
                for node in ast.walk(tree)
                if isinstance(node, ast.Attribute) and node.attr in forbidden_symbols
            }
        )
        imports = [type(node).__name__ for node in ast.walk(tree) if isinstance(node, (ast.Import, ast.ImportFrom))]
        if dependencies or imports:
            add_error(errors, "event_business_logic", "事件文件包含业务依赖或 import", symbols=dependencies, imports=imports)
        for node in tree.body:
            if isinstance(node, (ast.Expr, ast.FunctionDef)):
                if isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant) and isinstance(node.value.value, str):
                    continue
                if isinstance(node, ast.FunctionDef):
                    defined.add(node.name)
                    signature_ok = (
                        [argument.arg for argument in node.args.args] == ["event_struct"]
                        and not node.args.posonlyargs
                        and not node.args.kwonlyargs
                        and node.args.vararg is None
                        and node.args.kwarg is None
                        and not node.args.defaults
                        and not node.args.kw_defaults
                    )
                    if not signature_ok:
                        add_error(
                            errors,
                            "event_stub_signature",
                            "事件空桩签名必须恰为 (event_struct)",
                            function=node.name,
                        )
                    if len(node.body) != 1 or not isinstance(node.body[0], ast.Return) or node.body[0].value is not None:
                        add_error(errors, "event_not_empty_stub", "事件函数不是空 return", function=node.name)
                    continue
            add_error(errors, "event_file_extra_statement", "事件文件包含空桩之外的语句", statement=type(node).__name__)
    except SyntaxError as error:
        add_error(errors, "event_syntax_error", "事件文件语法错误", detail=str(error))

    referenced = {name for source in sources for name in collect_calls(source) if name}
    missing = sorted(referenced - defined)
    extra = sorted(defined - referenced)
    if missing:
        add_error(errors, "missing_event_stubs", "工程引用了未定义事件空桩", functions=missing)
    if extra:
        add_error(errors, "orphan_event_stubs", "事件文件存在未被工程引用的空桩", functions=extra)
    return len(defined)


def validate(project_dir: Path, html_path: Path) -> dict:
    errors: list[dict] = []
    warnings: list[dict] = []
    html_stats = validate_html(html_path, errors)
    html_parser = PencilHtmlParser()
    html_parser.feed(html_path.read_text(encoding="utf-8"))
    html_nodes_by_name: dict[str, list] = {}

    def collect_html_node(node) -> None:
        if node.name:
            html_nodes_by_name.setdefault(node.name, []).append(node)
        for child in node.children:
            collect_html_node(child)

    for html_root in html_parser.roots:
        collect_html_node(html_root)

    for state_name, expected_objects in TWO_MODE_REFERENCE_STATES.items():
        state_nodes = html_nodes_by_name.get(state_name, [])
        if len(state_nodes) != 1:
            add_error(
                errors,
                "two_mode_reference_state",
                "两模式正常/断连参考稿必须各存在一次",
                state=state_name,
                matches=len(state_nodes),
            )
            continue
        state_node = state_nodes[0]
        descendants: dict[str, list] = {}

        def collect_reference_node(node) -> None:
            if node.name:
                descendants.setdefault(node.name, []).append(node)
            for child in node.children:
                collect_reference_node(child)

        collect_reference_node(state_node)
        title_nodes = descendants.get("ui_lbl_current_mode", [])
        expected_title_color = (
            "#8E98A7"
            if "two_modes_disconnected" in state_name
            else "#FFFFFF"
        )
        if (
            len(title_nodes) != 1
            or normalized_text(title_nodes[0]) != "标准"
            or (
                px(node_style(title_nodes[0]).get("left")),
                px(node_style(title_nodes[0]).get("top")),
                px(node_style(title_nodes[0]).get("width")),
                px(node_style(title_nodes[0]).get("height")),
            )
            != (40, 48, 120, 36)
            or node_style(title_nodes[0]).get("color")
            != expected_title_color
        ):
            add_error(
                errors,
                "two_mode_reference_title",
                "两模式参考稿必须保留位置、文案和连接态一致的当前模式标题",
                state=state_name,
            )
        for reference_object_name, expected_geometry in expected_objects.items():
            objects = descendants.get(reference_object_name, [])
            if len(objects) != 1:
                add_error(
                    errors,
                    "two_mode_reference_object",
                    "两模式参考稿缺少唯一的标准、健身或关机对象",
                    state=state_name,
                    object=reference_object_name,
                    matches=len(objects),
                )
                continue
            style = node_style(objects[0])
            actual_geometry = (
                px(style.get("left")),
                px(style.get("top")),
                px(style.get("width")),
                px(style.get("height")),
            )
            if actual_geometry != expected_geometry:
                add_error(
                    errors,
                    "two_mode_reference_geometry",
                    "两模式参考稿几何必须匹配运行时合同",
                    state=state_name,
                    object=reference_object_name,
                    expected=expected_geometry,
                    actual=actual_geometry,
                )
        if any(
            name in descendants
            for name in (
                "ui_btn_mode_extreme",
                "ui_btn_mode_extreme_disabled",
                "ui_btn_mode_eco",
                "ui_btn_mode_eco_disabled",
            )
        ):
            add_error(
                errors,
                "two_mode_reference_hidden_modes",
                "两模式参考稿不得包含极限或下山卡片",
                state=state_name,
            )
    if SETTINGS_RAISE_WAKE_SOURCE_OBJECT not in html_path.read_text(
        encoding="utf-8"
    ):
        warnings.append(
            {
                "code": "settings_raise_wake_source_reserve_missing",
                "message": "当前 Pencil HTML 缺少既有抬腕亮屏行；生成器仅为兼容已发布固件合同保留该对象，设计真源需另行恢复。",
            }
        )

    manifest = parse_json(project_dir / "project_manifest.json", errors)
    project = parse_json(project_dir / "watch-lvgl.spj", errors)
    project_info = parse_json(project_dir / "watch-lvgl.sll", errors)
    parse_json(project_dir / "watch-lvgl.slp", errors)
    expected_canvas = {
        "width": EXPECTED_CANVAS_WIDTH,
        "height": EXPECTED_CANVAS_HEIGHT,
        "shape": EXPECTED_VISIBLE_SHAPE,
        "corner_radius": EXPECTED_CORNER_RADIUS,
        "critical_content_clearance": EXPECTED_CRITICAL_CLEARANCE,
        "color_depth": "RGB565",
        "lvgl": "8.3.11",
        "squareline": "1.6.1",
    }
    if manifest.get("canvas") != expected_canvas:
        add_error(
            errors,
            "manifest_display_geometry",
            "manifest 必须完整登记 410×502 圆角矩形、R110 与 8 px 关键内容净空",
            expected=expected_canvas,
            actual=manifest.get("canvas"),
        )
    expected_source_policy = {
        "html": "../watch-lvgl.html",
        "pencil": "../watch-lvgl.pen",
        "identity_policy": "优先 data-pencil-id；HTML 未携带 ID 时使用 [UI][PAGE|VAR][page_code][state] 结构键，不使用中文标题匹配。",
        "html_identity_mode": html_stats["identity_mode"],
        "locked_pencil_id_count": len(STATE_SOURCE_IDS),
        "studio_identity_policy": "逻辑合同使用 logical_name；SquareLine 对象解析使用稳定 GUID，组件实例 descendant 再以 cid/cgid/coid_path 交叉校验。",
    }
    if manifest.get("source") != expected_source_policy:
        add_error(
            errors,
            "source_identity_contract",
            "manifest 必须记录 HTML 身份模式与 30 个锁定 Pencil ID 的回退合同",
            expected=expected_source_policy,
            actual=manifest.get("source"),
        )
    for source_name, info in (("watch-lvgl.spj", project.get("info", {})), ("watch-lvgl.sll", project_info)):
        actual_project_geometry = {
            "width": info.get("width"),
            "height": info.get("height"),
            "shape": info.get("shape"),
            "lvgl_version": info.get("lvgl_version"),
            "BitDepth": info.get("BitDepth"),
        }
        expected_project_geometry = {
            "width": EXPECTED_CANVAS_WIDTH,
            "height": EXPECTED_CANVAS_HEIGHT,
            # SquareLine 只提供矩形/圆形面板枚举；圆角由下方 Screen/PANEL mask 实现。
            "shape": "RECTANGLE",
            "lvgl_version": "8.3.11",
            "BitDepth": 16,
        }
        if actual_project_geometry != expected_project_geometry:
            add_error(
                errors,
                "squareline_project_geometry",
                "SquareLine 工程边界框必须保持 410×502、RGB565 与 LVGL 8.3.11",
                file=source_name,
                expected=expected_project_geometry,
                actual=actual_project_geometry,
            )
    if project.get("info", {}).get("editor_version") != "1.6.1" or project_info.get("editor_version") != "1.6.1":
        add_error(errors, "editor_version", "工程与项目信息必须固定为 SquareLine Studio 1.6.1")
    expected_normalization_contract = {
        "screen_order": "non_semantic_resolve_by_studio_guid",
        "screen_name": "logical_name_authoritative_legacy_alias_only_with_legacy_guid",
        "component_instance_root_name": "studio_may_normalize_to_master_name_resolve_by_guid_cid_cgid_coid_path",
        "property_nid": "serializer_property_identity_may_repeat_and_is_not_an_object_key",
        "component_instance_visuals": "state_patch_projection_is_authoritative_when_studio_restores_master_defaults",
        "component_instance_events": "inherit_component_master_event_without_instance_override",
    }
    if manifest.get("editor", {}).get("save_normalization_contract") != expected_normalization_contract:
        add_error(errors, "save_normalization_contract", "manifest 必须记录 SquareLine 1.6.1 保存规范化语义")
    editor_contract = manifest.get("editor", {})
    expected_editor_license = {
        "license_at_generation": "personal",
        "license_observation": "save_dialog_reports_150_widget_personal_limit",
        "trial_expiry": "not_observed_in_static_files",
        "trial_full_feature_validation": False,
        "trial_status_at_validation": "not_active_personal_license_observed",
        "trial_supports_current_project_if_active": True,
        "current_session_trial_supports_current_project": False,
        "gui_save_roundtrip": "blocked_by_personal_license_over_150_widgets",
        "commercial_release_requires_appropriate_license": True,
    }
    if any(editor_contract.get(key) != value for key, value in expected_editor_license.items()):
        add_error(errors, "editor_license_contract", "必须记录真实 Personal 许可证与超过 150 widgets 的保存阻断")
    if not isinstance(editor_contract.get("gui_partial_normalization_adopted"), bool):
        add_error(errors, "studio_adoption_contract", "必须明确是否接纳 GUI 保存前的部分规范化结果")
    screens = project.get("root", {}).get("children", [])
    expected_screens = [item[0] for item in SCREEN_STATES]
    mount_model = manifest.get("mount_model", {})
    manifest_screens = mount_model.get("screens", [])
    if [item.get("logical_name") for item in manifest_screens if isinstance(item, dict)] != expected_screens:
        add_error(errors, "screen_manifest_order", "manifest Screen 映射必须按权威逻辑顺序登记")
    manifest_screen_by_name = {
        item.get("logical_name"): item
        for item in manifest_screens
        if isinstance(item, dict) and item.get("logical_name")
    }
    expected_screen_guids = {
        name: manifest_screen_by_name.get(name, {}).get("studio_guid")
        for name in expected_screens
    }
    invalid_screen_guids = {}
    for name, screen_guid in expected_screen_guids.items():
        allowed_guids = {
            guid(f"{SCREEN_GUID_NAMESPACE}:screen:{name}"),
            guid(f"screen:{name}"),
        }
        if screen_guid not in allowed_guids:
            invalid_screen_guids[name] = screen_guid
    if invalid_screen_guids:
        add_error(errors, "screen_manifest_guid", "Screen GUID 必须是 Phase 2 salted GUID 或已观测的 C2 legacy GUID", screens=invalid_screen_guids)
    logical_screen_by_guid = {value: key for key, value in expected_screen_guids.items()}
    screens_by_guid = {screen.get("guid"): screen for screen in screens if screen.get("guid")}
    actual_screen_guids = set(screens_by_guid)
    if actual_screen_guids != set(logical_screen_by_guid):
        add_error(
            errors,
            "screen_identity_mismatch",
            "Screen 必须匹配 manifest 稳定 GUID；显示顺序不参与合同",
            missing=sorted(set(logical_screen_by_guid) - actual_screen_guids),
            unknown=sorted(actual_screen_guids - set(logical_screen_by_guid)),
        )
    screen_names = {
        logical_screen_by_guid[screen_guid]: object_name(screen)
        for screen_guid, screen in screens_by_guid.items()
        if screen_guid in logical_screen_by_guid
    }
    bad_screen_names = {}
    for logical_name, actual_name in screen_names.items():
        screen_guid = expected_screen_guids[logical_name]
        legacy_alias = (
            logical_name == "ui_scr_main_shell"
            and screen_guid == guid("screen:ui_scr_main_shell")
            and actual_name == "uiscrmainshell"
        )
        if actual_name != logical_name and not legacy_alias:
            bad_screen_names[logical_name] = actual_name
        elif legacy_alias:
            warnings.append(
                {
                    "code": "legacy_main_shell_saved_name_alias",
                    "message": "主壳逻辑名仍为 ui_scr_main_shell；Studio 因 C2 legacy GUID 缓存保存为 uiscrmainshell，运行时须按 GUID 解析。",
                }
            )
    if bad_screen_names:
        add_error(
            errors,
            "screen_saved_name_mismatch",
            "SquareLine 保存后的 Screen 名必须保留权威逻辑名",
            names=bad_screen_names,
        )

    temporary_by_screen = {}
    for screen_guid, screen in screens_by_guid.items():
        logical_name = logical_screen_by_guid.get(screen_guid)
        if logical_name is None:
            continue
        prop = property_of(screen, "SCREEN/Temporary")
        temporary_by_screen[logical_name] = bool(prop and prop.get("strval") == "True")
    expected_temporary = {name: temporary for name, _roots, temporary, _scrolling in SCREEN_STATES}
    if temporary_by_screen != expected_temporary:
        add_error(errors, "screen_lifecycle_mismatch", "Screen Temporary 属性不一致", expected=expected_temporary, actual=temporary_by_screen)
    actual_resident = {name for name, temporary in temporary_by_screen.items() if not temporary}
    if actual_resident != RESIDENT_SCREEN_NAMES:
        add_error(errors, "resident_screen_mismatch", "常驻 Screen 必须且只能是主壳", expected=sorted(RESIDENT_SCREEN_NAMES), actual=sorted(actual_resident))

    expected_manifest_screens = [
        {
            "name": name,
            "logical_name": name,
            "studio_guid": expected_screen_guids[name],
            "studio_saved_name": screen_names.get(name),
            "temporary": temporary,
            "page_state_root_objects": root_names,
        }
        for name, root_names, temporary, _scrolling in SCREEN_STATES
    ]
    if (
        mount_model.get("resident_screen") != "ui_scr_main_shell"
        or mount_model.get("resident_screen_guid") != expected_screen_guids["ui_scr_main_shell"]
        or mount_model.get("screens") != expected_manifest_screens
    ):
        add_error(errors, "screen_manifest_identity", "manifest 必须保存逻辑 Screen 名与稳定 Studio GUID 的映射")

    project_nodes = [node for screen in screens for node in walk_nodes(screen)]
    validate_device_info_project_contract(project_nodes, errors)
    project_node_by_guid = {
        node["guid"]: node
        for node in project_nodes
        if node.get("guid")
    }
    project_parent_by_guid = {
        node["guid"]: parent
        for screen in screens
        for node, parent in walk_nodes_with_parent(screen)
        if node.get("guid")
    }
    root_occurrences = {
        root_name: sum(1 for node in project_nodes if object_name(node) == root_name)
        for root_name in ROOT_STATES
    }
    bad_roots = {name: count for name, count in root_occurrences.items() if count != 1}
    if bad_roots:
        add_error(errors, "page_root_mismatch", "13 个页面状态根对象必须各出现一次", roots=bad_roots)

    expected_root_hosts = {
        root_name: screen_name
        for screen_name, root_names, _temporary, _scrolling in SCREEN_STATES
        for root_name in root_names
    }
    actual_root_hosts: dict[str, str] = {}
    root_nodes: dict[str, dict] = {}
    for screen in screens:
        screen_name = logical_screen_by_guid.get(screen.get("guid"), object_name(screen))
        for node in walk_nodes(screen):
            name = object_name(node)
            if name in ROOT_STATES:
                actual_root_hosts[name] = screen_name
                root_nodes[name] = node
    if actual_root_hosts != expected_root_hosts:
        add_error(
            errors,
            "root_screen_mount_mismatch",
            "每个页面根对象必须挂载到指定 Screen",
            expected=expected_root_hosts,
            actual=actual_root_hosts,
        )

    rounded_mask_failures: list[dict] = []
    panel_mask_names = {
        *ROOT_STATES,
        "ui_main_shell_pager",
        "ui_main_shell_page_home",
        "ui_main_shell_page_control",
        "ui_main_shell_page_mode",
        "ui_main_shell_page_alerts",
    }
    panel_mask_counts = Counter(
        object_name(node)
        for node in project_nodes
        if object_name(node) in panel_mask_names
    )
    invalid_panel_mask_counts = {
        name: panel_mask_counts.get(name, 0)
        for name in sorted(panel_mask_names)
        if panel_mask_counts.get(name, 0) != 1
    }
    if invalid_panel_mask_counts:
        add_error(
            errors,
            "squareline_rounded_mask_identity",
            "13 个状态根及主壳 pager/4 页包装必须逐名各出现一次",
            counts=invalid_panel_mask_counts,
        )
    rounded_targets = [
        (logical_screen_by_guid.get(screen.get("guid"), object_name(screen)), screen, "SCREEN/Style_main", False)
        for screen in screens
    ]
    rounded_targets.extend(
        (object_name(node), node, "PANEL/Style_main", True)
        for node in project_nodes
        if object_name(node) in panel_mask_names
    )
    for name, node, style_property, require_size in rounded_targets:
        radius = style_contract_value(node, style_property, "_style/Bg_Radius", "integer")
        clip_corner = style_contract_value(node, style_property, "_style/Clip_corner", "strval")
        size = object_size(node) if require_size else [EXPECTED_CANVAS_WIDTH, EXPECTED_CANVAS_HEIGHT]
        if radius != EXPECTED_CORNER_RADIUS or clip_corner != "True" or size != [EXPECTED_CANVAS_WIDTH, EXPECTED_CANVAS_HEIGHT]:
            rounded_mask_failures.append(
                {"name": name, "size": size, "corner_radius": radius, "clip_corner": clip_corner}
            )
    if len(rounded_targets) != 27 or rounded_mask_failures:
        add_error(
            errors,
            "squareline_rounded_mask_contract",
            "9 个 Screen、13 个状态根及主壳 pager/4 页包装必须以 410×502、R110 裁切",
            expected_targets=27,
            actual_targets=len(rounded_targets),
            failures=rounded_mask_failures,
        )

    expected_scrollable_roots = {
        "ui_page_ble_root",
        "ui_page_selftest_result_root",
        "ui_page_selftest_retry_detail_root",
    }
    actual_scrollable_roots = {
        name
        for name, node in root_nodes.items()
        if (property_of(node, "OBJECT/Scrollable") or {}).get("strval") == "True"
    }
    if actual_scrollable_roots != expected_scrollable_roots:
        add_error(
            errors,
            "squareline_scroll_root_contract",
            "07-02、08-04、08-06 所属状态根必须启用纵向滚动，其他状态根不得启用",
            expected=sorted(expected_scrollable_roots),
            actual=sorted(actual_scrollable_roots),
        )

    result_list = next(
        (node for node in project_nodes if object_name(node) == "ui_selftest_result_result_list"),
        None,
    )
    result_list_scroll = {
        key: (property_of(result_list, key) or {}).get("strval")
        if result_list is not None
        else None
        for key in (
            "OBJECT/Scrollable",
            "OBJECT/Scrollbar_mode",
            "OBJECT/Scroll_direction",
        )
    }
    expected_result_list_scroll = {
        "OBJECT/Scrollable": "True",
        "OBJECT/Scrollbar_mode": "OFF",
        "OBJECT/Scroll_direction": "VER",
    }
    if result_list_scroll != expected_result_list_scroll:
        add_error(
            errors,
            "selftest_result_list_scroll_contract",
            "08-04 的 8 行结果列表必须可纵向滚动且不叠加系统滚动条",
            expected=expected_result_list_scroll,
            actual=result_list_scroll,
        )

    sibling_duplicates: list[dict] = []
    for screen in screens:
        for parent in walk_nodes(screen):
            named_children = [child for child in parent.get("children", []) if object_name(child)]
            names = [object_name(child) for child in named_children]
            duplicates = []
            for name in sorted({value for value in names if names.count(value) > 1}):
                same_name = [child for child in named_children if object_name(child) == name]
                component_instance_group = (
                    all(child.get("cid") for child in same_name)
                    and len({child.get("cid") for child in same_name}) == 1
                    and len({child.get("guid") for child in same_name}) == len(same_name)
                    and len({child.get("cgid") for child in same_name}) == len(same_name)
                )
                if not component_instance_group:
                    duplicates.append(name)
            if duplicates:
                sibling_duplicates.append({"parent": object_name(parent) or parent.get("guid", ""), "names": duplicates})
    if sibling_duplicates:
        add_error(errors, "duplicate_sibling_names", "同一父节点下的非空对象名必须唯一", duplicates=sibling_duplicates)

    modal = next((node for node in project_nodes if object_name(node) == "ui_modal_clear_binding_root"), None)
    modal_hidden = property_of(modal, "OBJECT/Hidden") if modal else None
    if not modal_hidden or modal_hidden.get("strval") != "True":
        add_error(errors, "modal_not_hidden_blueprint", "清除绑定 modal 根对象必须以隐藏蓝图存在")
    if "ui_modal_clear_binding_root" in screen_names.values():
        add_error(errors, "modal_is_screen", "清除绑定 modal 不得成为 Screen")

    modal_contract = manifest.get("mount_model", {}).get("on_demand_modal", {})
    expected_modal_contract = {
        "page_state_root_object": "ui_modal_clear_binding_root",
        "authoring_template_host_screen": "ui_scr_maintenance",
        "authoring_template_hidden": True,
        "runtime_initial_state": "template_detached_after_host_init",
        "runtime_create": "clone_template_on_clear_binding_click_then_clear_hidden_before_display",
        "runtime_clone_steps": ["clone_template", "clear_hidden", "attach_to_active_layer", "display"],
        "runtime_release": ["cancel", "clear_binding_completed", "host_screen_release"],
        "runtime_owner": "phase_3_binding",
    }
    if modal_contract != expected_modal_contract:
        add_error(
            errors,
            "modal_lifecycle_contract",
            "09-01 必须作为 SquareLine 隐藏模板，并由阶段三在点击时克隆、取消或完成后释放",
            expected=expected_modal_contract,
            actual=modal_contract,
        )

    component_dir = project_dir / "components"
    component_files = sorted(path.stem for path in component_dir.glob("*.ecomp"))
    expected_components = sorted(COMPONENT_SOURCES)
    if component_files != expected_components:
        add_error(errors, "component_file_mismatch", "组件文件必须恰好四个", expected=expected_components, actual=component_files)

    components = {}
    component_guid_to_name = {}
    valid_component_masters: set[str] = set()

    def component_master_shape_errors(component_name: str, component) -> list[str]:
        issues: list[str] = []
        if not isinstance(component, dict):
            return ["root_not_object"]
        if not isinstance(component.get("guid"), str) or not component.get("guid"):
            issues.append("root_guid_missing")
        if not isinstance(component.get("properties"), list) or any(
            not isinstance(prop, dict) for prop in component.get("properties", [])
        ):
            issues.append("root_properties_not_object_list")
        children = component.get("children")
        expected_child_count = COMPONENT_MASTER_CHILD_COUNTS[component_name]
        if not isinstance(children, list):
            issues.append("root_children_not_list")
            return issues
        if len(children) != expected_child_count:
            issues.append(f"root_child_count_expected_{expected_child_count}_actual_{len(children)}")
        for index, child in enumerate(children):
            if not isinstance(child, dict):
                issues.append(f"child_{index}_not_object")
                continue
            if not isinstance(child.get("guid"), str) or not child.get("guid"):
                issues.append(f"child_{index}_guid_missing")
            if not isinstance(child.get("properties"), list) or any(
                not isinstance(prop, dict) for prop in child.get("properties", [])
            ):
                issues.append(f"child_{index}_properties_not_object_list")
            if not isinstance(child.get("children"), list):
                issues.append(f"child_{index}_children_not_list")
            elif child.get("children"):
                issues.append(f"child_{index}_unexpected_descendants")
        return issues

    for component_name in expected_components:
        path = component_dir / f"{component_name}.ecomp"
        if not path.exists():
            continue
        component = parse_json(path, errors)
        components[component_name] = component
        if isinstance(component, dict) and component.get("guid"):
            component_guid_to_name[component.get("guid")] = component_name
        shape_issues = component_master_shape_errors(component_name, component)
        if shape_issues:
            add_error(
                errors,
                "component_master_shape",
                "组件 master 必须具有完整 root/properties/children/leaf 结构",
                component=component_name,
                issues=shape_issues,
            )
            continue
        valid_component_masters.add(component_name)
        if object_name(component) != component_name:
            add_error(errors, "component_root_name_mismatch", "组件根对象名不一致", component=component_name, actual=object_name(component))

    component_sibling_duplicates: list[dict] = []
    for component_name, component in components.items():
        if component_name not in valid_component_masters:
            continue
        for parent in walk_nodes(component):
            names = [object_name(child) for child in parent.get("children", []) if object_name(child)]
            duplicates = sorted({name for name in names if names.count(name) > 1})
            if duplicates:
                component_sibling_duplicates.append(
                    {"component": component_name, "parent": object_name(parent) or parent.get("guid", ""), "names": duplicates}
                )
    if component_sibling_duplicates:
        add_error(
            errors,
            "duplicate_component_sibling_names",
            "组件 master 同一父节点下的非空对象名必须唯一",
            duplicates=component_sibling_duplicates,
        )

    actual_instances = {name: 0 for name in expected_components}
    missing_component_ids = []
    component_structure_errors: list[dict] = []

    def master_signature(node: dict):
        return (
            legacy_dotnet_string_hash(node.get("guid", "")),
            tuple(master_signature(child) for child in node.get("children", []) if isinstance(child, dict)),
        )

    def instance_signature(node: dict):
        return (
            node.get("coid"),
            tuple(instance_signature(child) for child in node.get("children", []) if isinstance(child, dict)),
        )

    for node in project_nodes:
        cid = node.get("cid")
        if not cid:
            continue
        component_name = component_guid_to_name.get(cid)
        if not component_name:
            missing_component_ids.append(cid)
        else:
            actual_instances[component_name] += 1
            master = components[component_name]
            instance_children = node.get("children", []) if isinstance(node.get("children", []), list) else []
            expected_child_count = COMPONENT_MASTER_CHILD_COUNTS[component_name]
            if (
                component_name not in valid_component_masters
                or len(instance_children) != expected_child_count
                or any(not isinstance(child, dict) for child in instance_children)
                or instance_signature(node) != master_signature(master)
            ):
                component_structure_errors.append({"component": component_name, "instance_guid": node.get("guid")})
    expected_instances = {name: spec[2] for name, spec in COMPONENT_SOURCES.items()}
    if actual_instances != expected_instances:
        add_error(errors, "component_instance_mismatch", "四个组件的实例数量不一致", expected=expected_instances, actual=actual_instances)
    if missing_component_ids:
        add_error(errors, "missing_component_reference", "工程存在找不到 .ecomp 的组件引用", cids=sorted(set(missing_component_ids)))
    if component_structure_errors:
        add_error(
            errors,
            "component_instance_structure_mismatch",
            "组件实例必须按 coid 树与对应 master 保持同构",
            instances=component_structure_errors,
        )
    component_event_names = sorted({name for component in components.values() for name in collect_calls(component)})
    expected_component_events = ["ui_evt_cmp_ble_candidate_row_clicked", "ui_evt_cmp_setting_toggle_clicked"]
    if component_event_names != expected_component_events:
        add_error(
            errors,
            "component_event_identity",
            "组件事件必须使用通用组件身份，不得沿用 haptics/candidate_1 样例名",
            expected=expected_component_events,
            actual=component_event_names,
        )

    all_sources = [*screens, *components.values()]
    serialized_sources = [project.get("root", {}), *components.values()]
    change_screen_actions = [
        value
        for source in serialized_sources
        for value in walk_dicts(source)
        if value.get("strval") == "CHANGE SCREEN" or str(value.get("strtype", "")).startswith("CHANGE SCREEN/")
    ]
    if change_screen_actions:
        add_error(errors, "change_screen_action_present", "Generated 层不得包含 CHANGE SCREEN", count=len(change_screen_actions))

    guids = [value["guid"] for source in serialized_sources for value in walk_dicts(source) if value.get("guid")]
    duplicate_guids = sorted(value for value, count in Counter(guids).items() if count > 1)
    if duplicate_guids:
        add_error(errors, "duplicate_guids", "工程与四组件 master 的 GUID 必须全局唯一", guids=duplicate_guids)
    # NID 是属性类型/序列化标识，不是对象主键。SquareLine 1.6.1 保存时会把
    # 默认属性归一为共享 NID（例如 75/1020/1040），跨对象重复是合法行为。
    # 对象身份由 GUID；组件实例映射由 cid/cgid/coid/compnid 共同承担。

    event_count = validate_events(project_dir, all_sources, errors)

    if manifest.get("schema_version") != 2:
        add_error(errors, "manifest_schema", "manifest 必须使用 schema v2", actual=manifest.get("schema_version"))
    for key in ["mount_model", "state_registry", "component_registry", "route_registry", "counts"]:
        if key not in manifest:
            add_error(errors, "manifest_missing_registry", "manifest 缺少阶段二机器合同", key=key)

    state_registry = manifest.get("state_registry", [])
    expected_builder: ProjectBuilder | None = None
    expected_projection_components: dict[str, dict] = {}
    try:
        lucide_map = parse_json(
            project_dir / "assets" / "font_sources" / "lucide" / "codepoints-1.16.0.json",
            errors,
        )
        expected_builder = ProjectBuilder(project_dir, html_path, lucide_map)
        expected_projection_components = expected_builder.reusable_components()
        for root_name in ROOT_STATES:
            expected_builder.page_root(root_name, expected_projection_components)
    except Exception as error:
        add_error(errors, "state_projection_failed", "无法从 HTML 重建状态补丁基准", detail=str(error))
    registry_states = [item.get("state") for item in state_registry]
    expected_states = [*PAGE_STATES.values(), *VARIANT_STATES.values()]
    if registry_states != expected_states:
        add_error(errors, "state_registry_mismatch", "状态登记必须包含 23 个正式页面和 7 个整页变体", expected=expected_states, actual=registry_states)
    if any("squareline_component" in item for item in state_registry):
        add_error(errors, "page_declared_as_component", "页面或整页变体不得使用 squareline_component 字段")
    root_by_state = {state: root_name for root_name, states in ROOT_STATES.items() for state in states}
    state_patch_critical_objects = 0
    project_geometry_by_guid: dict[str, dict[str, bool]] = {}
    for item in state_registry:
        state = item.get("state")
        if state not in STATE_SOURCE_IDS:
            continue
        if item.get("pencil_node_id") != STATE_SOURCE_IDS[state]:
            add_error(errors, "pencil_node_id_mismatch", "状态的 Pencil 节点 ID 不一致", state=state)
        if item.get("base_state") != VARIANT_BASE_STATES.get(state):
            add_error(errors, "variant_base_mismatch", "整页变体基础状态不一致", state=state)
        if item.get("page_state_root_object") not in ROOT_STATES:
            add_error(errors, "state_root_missing", "状态没有合法页面根对象", state=state)
        required_fields = ["canonical_id", "state_key", "pen_node_id", "screen", "root", "lifecycle", "model_fields", "events", "component_refs", "route_refs"]
        missing_fields = [field for field in required_fields if field not in item]
        if missing_fields:
            add_error(errors, "state_registry_fields", "状态登记缺少稳定映射字段", state=state, fields=missing_fields)
        if item.get("canonical_id") != STATE_CODES[state] or item.get("state_key") != state or item.get("pen_node_id") != STATE_SOURCE_IDS[state]:
            add_error(errors, "state_registry_identity", "状态稳定身份字段不一致", state=state)
        expected_root = root_by_state[state]
        expected_host = expected_root_hosts[expected_root]
        expected_screen = None if expected_root == "ui_modal_clear_binding_root" else expected_host
        expected_lifecycle = (
            "on_demand_modal"
            if expected_root == "ui_modal_clear_binding_root"
            else "resident_shell_slot"
            if expected_host == "ui_scr_main_shell"
            else "on_demand_screen"
        )
        if (
            item.get("root") != expected_root
            or item.get("page_state_root_object") != expected_root
            or item.get("screen") != expected_screen
            or item.get("lifecycle") != expected_lifecycle
            or item.get("authoring_template_host_screen") != (expected_host if expected_screen is None else None)
        ):
            add_error(
                errors,
                "state_mount_mapping",
                "状态的 Screen/root/lifecycle 映射不一致",
                state=state,
                expected={"root": expected_root, "screen": expected_screen, "lifecycle": expected_lifecycle},
            )
        if item.get("model_fields") != ROOT_MODEL_FIELDS[expected_root]:
            add_error(errors, "state_model_fields", "状态 model_fields 与根对象合同不一致", state=state)
        if item.get("route_refs") != ROOT_ROUTE_REFS[expected_root]:
            add_error(errors, "state_route_refs", "状态 route_refs 与根对象合同不一致", state=state)
        patch = item.get("state_patch")
        if not isinstance(patch, dict) or set(patch) != {"baseline_reset", "show", "hide", "overrides"}:
            add_error(errors, "state_patch_shape", "每个状态必须有 baseline_reset/show/hide/overrides 补丁", state=state)
            continue
        if expected_builder is not None:
            if patch != expected_builder.state_patches.get(state):
                add_error(errors, "state_patch_projection", "状态补丁必须与 HTML 重新投影结果逐项一致", state=state)
            if item.get("events") != expected_builder._manifest_event_names(state):
                add_error(errors, "state_event_projection", "状态事件必须与 HTML/组件 master 投影结果一致", state=state)
            if item.get("component_refs") != sorted(expected_builder.state_component_refs.get(state, [])):
                add_error(errors, "state_component_projection", "状态组件引用必须与 HTML 投影结果一致", state=state)
        root_node = root_nodes.get(expected_root)
        owned_by_guid = {
            child.get("guid"): child
            for child in root_node.get("children", [])
            if child.get("guid")
        } if root_node else {}
        owned_guids = set(owned_by_guid)
        show = patch.get("show") if isinstance(patch.get("show"), list) else []
        hide = patch.get("hide") if isinstance(patch.get("hide"), list) else []
        overrides = patch.get("overrides") if isinstance(patch.get("overrides"), dict) else {}
        baseline = patch.get("baseline_reset") if isinstance(patch.get("baseline_reset"), list) else []
        if expected_builder is not None:
            state_critical_count, state_project_metadata = validate_state_patch_geometry(
                state,
                expected_builder.state_pages[state],
                overrides,
                errors,
                project_dir,
            )
            state_patch_critical_objects += state_critical_count
            for target_guid, metadata in state_project_metadata.items():
                target_metadata = project_geometry_by_guid.setdefault(
                    target_guid,
                    {"critical": False, "scroll": False, "title": False},
                )
                for key in target_metadata:
                    target_metadata[key] = target_metadata[key] or metadata[key]

        def ref_guid(value) -> str | None:
            return value.get("guid") if isinstance(value, dict) else None

        show_guids = {ref_guid(value) for value in show}
        hide_guids = {ref_guid(value) for value in hide}
        baseline_targets = {
            ref_guid(entry.get("target"))
            for entry in baseline
            if isinstance(entry, dict)
        }
        all_refs = [
            *show,
            *hide,
            *[
                entry.get("target")
                for entry in baseline
                if isinstance(entry, dict)
            ],
        ]
        for target_ref in all_refs:
            target_guid = ref_guid(target_ref)
            actual_node = owned_by_guid.get(target_guid)
            expected_component_path = component_path_for(actual_node, project_parent_by_guid) if actual_node else None
            if (
                not isinstance(target_ref, dict)
                or not target_ref.get("logical_name")
                or actual_node is None
                or target_ref.get("component_path") != expected_component_path
                or (expected_component_path is None and target_ref.get("logical_name") != object_name(actual_node))
            ):
                add_error(
                    errors,
                    "state_patch_target_identity",
                    "状态补丁 target ref 必须使用稳定 GUID，并以 cid/cgid/coid_path 描述组件实例",
                    state=state,
                    target=target_guid,
                )
        referenced_guids = show_guids | hide_guids | set(overrides) | baseline_targets
        if None in referenced_guids or referenced_guids - owned_guids:
            add_error(
                errors,
                "state_patch_foreign_object",
                "状态补丁 GUID 引用了不属于当前根对象的对象",
                state=state,
                objects=sorted(str(value) for value in referenced_guids - owned_guids),
            )
        if (
            show_guids & hide_guids
            or show_guids | hide_guids != owned_guids
            or set(overrides) != show_guids
            or baseline_targets != owned_guids
        ):
            add_error(
                errors,
                "state_patch_coverage",
                "状态补丁必须完整划分所属根对象，且 overrides 与 show 一致",
                state=state,
            )
        for top_guid, descriptor in overrides.items():
            top_node = owned_by_guid.get(top_guid)
            owned_nested = {node.get("guid") for node in walk_nodes(top_node)} if top_node else set()

            def validate_descriptor(value: dict, actual_node: dict | None, component_scope: bool = False) -> None:
                target = value.get("target")
                target_guid = ref_guid(target)
                operations = value.get("operations")
                expected_component_path = component_path_for(actual_node, project_parent_by_guid) if actual_node else None
                if (
                    actual_node is None
                    or target_guid != actual_node.get("guid")
                    or target_guid not in owned_nested
                    or not isinstance(target, dict)
                    or not target.get("logical_name")
                    or target.get("component_path") != expected_component_path
                    or not isinstance(operations, list)
                    or any(
                    not isinstance(operation, dict)
                    or not str(operation.get("op", "")).startswith("lv_")
                    or not isinstance(operation.get("args"), list)
                    for operation in operations
                    )
                ):
                    add_error(
                        errors,
                        "state_patch_lvgl_operation",
                        "状态补丁必须用稳定 GUID、组件路径和 LVGL 级操作表达",
                        state=state,
                        target=target_guid,
                    )
                component_scope = component_scope or bool(expected_component_path)
                for child in value.get("children", []):
                    if isinstance(child, dict):
                        child_guid = ref_guid(child.get("target"))
                        actual_child = next((node for node in (actual_node or {}).get("children", []) if node.get("guid") == child_guid), None)
                        validate_descriptor(child, actual_child, component_scope)

            if isinstance(descriptor, dict):
                validate_descriptor(descriptor, top_node)
            else:
                add_error(errors, "state_patch_descriptor", "状态补丁 descriptor 必须是对象", state=state, target=top_guid)
        expected_component_refs = sorted(
            {
                component_guid_to_name[node.get("cid")]
                for target_guid in show_guids
                if target_guid in owned_by_guid
                for node in walk_nodes(owned_by_guid[target_guid])
                if node.get("cid") in component_guid_to_name
            }
        )
        if item.get("component_refs") != expected_component_refs:
            add_error(
                errors,
                "state_component_refs",
                "component_refs 必须按具体状态的实际可见组件登记",
                state=state,
                expected=expected_component_refs,
                actual=item.get("component_refs"),
            )
        if state in VARIANT_STATES.values() and not patch.get("overrides"):
            add_error(errors, "variant_patch_missing", "整页变体必须实际读取 HTML 并生成属性补丁", state=state)

    static_geometry_stats = validate_static_project_geometry(root_nodes, project_geometry_by_guid, errors)

    loading_state = next((item for item in state_registry if item.get("state") == "selftest_gps_submit_loading"), {})
    loading_button_descriptors: list[dict] = []

    def collect_loading_buttons(value) -> None:
        if isinstance(value, dict):
            if value.get("source_object") in {"ui_btn_gps_outdoor", "ui_btn_gps_indoor"}:
                loading_button_descriptors.append(value)
            for child in value.values():
                collect_loading_buttons(child)
        elif isinstance(value, list):
            for child in value:
                collect_loading_buttons(child)

    collect_loading_buttons(loading_state.get("state_patch", {}).get("overrides", {}))
    if loading_state.get("events") or not loading_button_descriptors or any(
        not any(operation.get("op") == "lv_obj_add_state" and operation.get("args") == ["DISABLED"] for operation in descriptor.get("operations", []))
        for descriptor in loading_button_descriptors
    ):
        add_error(errors, "gps_loading_interaction", "GPS submit_loading 必须禁用提交按钮且不生成事件")

    component_registry = manifest.get("component_registry", [])
    registry_components = [item.get("squareline_component") for item in component_registry]
    if registry_components != list(COMPONENT_SOURCES):
        add_error(errors, "component_registry_mismatch", "组件登记不一致", expected=list(COMPONENT_SOURCES), actual=registry_components)
    if {item.get("squareline_component"): item.get("actual_instances") for item in component_registry} != actual_instances:
        add_error(errors, "component_registry_instance_count", "组件登记的实例数量与工程不一致")
    for item in component_registry:
        component_name = item.get("squareline_component")
        if component_name not in COMPONENT_SOURCES:
            continue
        if component_name not in components:
            add_error(
                errors,
                "component_master_missing",
                "组件登记引用的 .ecomp master 文件不存在或无法解析",
                component=component_name,
            )
            continue
        if component_name not in valid_component_masters:
            # component_master_shape 已提供具体错误；避免派生 visual selector 时再次触发 IndexError/KeyError。
            continue
        source_state, source_object, max_instances = COMPONENT_SOURCES[component_name]
        visual_contracts = item.get("visual_state_contracts", [])
        if [entry.get("state") for entry in visual_contracts if isinstance(entry, dict)] != COMPONENT_VISUAL_EVIDENCE[component_name]["states"]:
            add_error(errors, "component_visual_state_set", "组件视觉状态合同集合不完整", component=component_name)

        master_parent_by_guid = {
            node.get("guid"): parent
            for node, parent in walk_nodes_with_parent(components[component_name])
            if node.get("guid")
        }
        master_node_by_guid = {
            node.get("guid"): node
            for node in walk_nodes(components[component_name])
            if node.get("guid")
        }
        for visual_state in visual_contracts:
            projections = visual_state.get("projections", []) if isinstance(visual_state, dict) else []
            roles = [projection.get("role") for projection in projections if isinstance(projection, dict)]
            if set(roles) != COMPONENT_REQUIRED_VISUAL_ROLES[component_name] or len(roles) != len(set(roles)):
                add_error(
                    errors,
                    "component_visual_required_roles",
                    "组件每个视觉状态必须覆盖独立审查基准规定的全部 descendant roles",
                    component=component_name,
                    state=visual_state.get("state") if isinstance(visual_state, dict) else None,
                    expected=sorted(COMPONENT_REQUIRED_VISUAL_ROLES[component_name]),
                    actual=sorted(str(role) for role in roles),
                )
            if not projections:
                add_error(errors, "component_visual_projection_empty", "组件每个视觉状态必须至少有一个可执行投影", component=component_name)
            for projection in projections:
                role = projection.get("role") if isinstance(projection, dict) else None
                selector = projection.get("selector", {}) if isinstance(projection, dict) else {}
                descendant = master_node_by_guid.get(selector.get("master_descendant_guid"))
                chain = []
                current = descendant
                while current is not None:
                    chain.append(current)
                    current = master_parent_by_guid.get(current.get("guid"))
                chain.reverse()
                expected_coid_path = [legacy_dotnet_string_hash(node.get("guid", "")) for node in chain]
                operations = projection.get("operations", []) if isinstance(projection, dict) else []
                if (
                    selector.get("component_guid") != components[component_name].get("guid")
                    or descendant is None
                    or (descendant is components[component_name] and role != "container")
                    or selector.get("logical_name") != object_name(descendant)
                    or selector.get("coid_path") != expected_coid_path
                    or not operations
                    or any(
                        not isinstance(operation, dict)
                        or not str(operation.get("op", "")).startswith("lv_")
                        or not isinstance(operation.get("args"), list)
                        for operation in operations
                    )
                ):
                    add_error(
                        errors,
                        "component_visual_projection_invalid",
                        "组件视觉状态必须解析到 master descendant GUID/coid_path 并提供 LVGL 操作",
                        component=component_name,
                        state=visual_state.get("state") if isinstance(visual_state, dict) else None,
                    )
                operation_names = {operation.get("op") for operation in operations if isinstance(operation, dict)}
                state_name = visual_state.get("state") if isinstance(visual_state, dict) else None
                required_operation = None
                if component_name == "cmp_setting_toggle" and role == "value":
                    required_operation = "lv_label_set_text"
                elif component_name == "cmp_alert_row" and role == "accent":
                    required_operation = "lv_obj_set_style_bg_color"
                elif component_name == "cmp_alert_row" and role == "state":
                    required_operation = "lv_label_set_text"
                elif component_name == "cmp_ble_candidate_row" and role == "container":
                    required_operation = "lv_obj_add_state" if state_name == "connecting" else "lv_obj_clear_state"
                elif component_name == "cmp_ble_candidate_row" and role == "status" and state_name in {"connecting", "failed"}:
                    required_operation = "lv_label_set_text"
                elif component_name == "cmp_selftest_result_row" and role == "state":
                    required_operation = "lv_label_set_text"
                if required_operation and required_operation not in operation_names:
                    add_error(
                        errors,
                        "component_visual_role_operation",
                        "关键组件 role 缺少独立要求的可执行操作",
                        component=component_name,
                        state=state_name,
                        role=role,
                        required_operation=required_operation,
                    )
                if component_name == "cmp_selftest_result_row" and role == "icon":
                    expected_glyph = {"pass": "\ue226", "fail": "\ue084", "skip": "\ue07e"}.get(state_name)
                    if not any(
                        operation.get("op") == "lv_label_set_text" and operation.get("args") == [expected_glyph]
                        for operation in operations
                        if isinstance(operation, dict)
                    ):
                        add_error(
                            errors,
                            "component_selftest_icon_glyph",
                            "自检结果 pass/fail/skip 左图标必须同时切换对应 glyph，不能只改颜色",
                            state=state_name,
                            expected_glyph=f"U+{ord(expected_glyph):04X}" if expected_glyph else None,
                        )
        expected_metadata = {
            "component_guid": components[component_name].get("guid"),
            "source_state": source_state,
            "source_object": source_object,
            "max_instances": max_instances,
            "actual_instances": actual_instances[component_name],
            "visual_states": COMPONENT_VISUAL_EVIDENCE[component_name]["states"],
            "visual_evidence": COMPONENT_VISUAL_EVIDENCE[component_name]["evidence"],
            "visual_state_contracts": (
                expected_builder._component_visual_state_contracts(expected_projection_components)[component_name]
                if expected_builder is not None and set(expected_projection_components) == set(COMPONENT_SOURCES)
                else visual_contracts
            ),
        }
        actual_metadata = {key: item.get(key) for key in expected_metadata}
        if actual_metadata != expected_metadata:
            add_error(
                errors,
                "component_registry_metadata",
                "四组件的 source/max/actual/视觉状态证据必须固定",
                component=component_name,
                expected=expected_metadata,
                actual=actual_metadata,
            )

    if manifest.get("state_patch_order") != PATCH_ORDER:
        add_error(errors, "state_patch_order", "状态补丁顺序不一致", expected=PATCH_ORDER, actual=manifest.get("state_patch_order"))

    route = manifest.get("route_registry", {})
    if route.get("owner") != "phase_3_binding":
        add_error(errors, "route_owner", "业务路由必须由阶段三 binding 持有")
    lock = route.get("selftest_lock", {})
    if lock.get("ble_connected") != "keep_02-01_and_03-01_then_disable_controls":
        add_error(errors, "selftest_lock_contract", "自检锁定不得改成 02-02/03-02")
    if route.get("fact_read_failure") != "preserve_current_page_log_failure_and_schedule_fact_retry":
        add_error(errors, "fact_failure_contract", "事实读取失败必须保持当前页、记录失败并调度重试")
    if route.get("required_fact_snapshot") != REQUIRED_FACT_SNAPSHOT:
        add_error(
            errors,
            "required_fact_snapshot",
            "成功事件必须重新读取完整事实快照",
            expected=REQUIRED_FACT_SNAPSHOT,
            actual=route.get("required_fact_snapshot"),
        )
    if route.get("fact_alias_normalization") != FACT_ALIAS_NORMALIZATION:
        add_error(errors, "fact_alias_normalization", "事实快照必须先按显式字段别名合同归一化")
    fact_normalization = route.get("fact_alias_normalization", {})
    authoritative_inputs = {
        item.get("field"): item
        for item in fact_normalization.get("authoritative_inputs", [])
        if isinstance(item, dict) and item.get("field")
    }
    payment_input = authoritative_inputs.get("payment_state", {})
    selftest_input = authoritative_inputs.get("selftest_state", {})
    verify_input = authoritative_inputs.get("verify_in_flight", {})
    derived_by_name = {
        item.get("canonical"): item
        for item in fact_normalization.get("derived_fields", [])
        if isinstance(item, dict) and item.get("canonical")
    }
    if (
        payment_input.get("canonical_values") != ["UNVERIFIED", "VERIFYING", "VERIFY_FAILED", "VERIFIED"]
        or payment_input.get("input_normalization", {}).get("VERIFYING") != "VERIFYING"
        or verify_input.get("type") != "bool"
        or selftest_input.get("canonical_values") != ["IDLE", "CANCELLED", "RUNNING", "MANUAL", "FINISHED"]
        or selftest_input.get("input_normalization", {}).get("AWAITING_MANUAL") != "MANUAL"
        or derived_by_name.get("payment_required", {}).get("value_map") != {
            "UNVERIFIED": True,
            "VERIFYING": True,
            "VERIFY_FAILED": True,
            "VERIFIED": False,
        }
        or derived_by_name.get("payment_attempted", {}).get("value_map") != {
            "UNVERIFIED": False,
            "VERIFYING": True,
            "VERIFY_FAILED": True,
            "VERIFIED": True,
        }
        or derived_by_name.get("selftest_running", {}).get("value_map") != {
            "IDLE": False,
            "CANCELLED": False,
            "RUNNING": True,
            "MANUAL": True,
            "FINISHED": False,
        }
        or fact_normalization.get("missing_or_conflicting_value")
        != "fact_snapshot_invalid_preserve_current_page_log_and_retry"
    ):
        add_error(
            errors,
            "fact_authoritative_derivation",
            "payment_state/verify_in_flight/selftest_state 必须按 HTML 权威大写词表派生 canonical facts，并保留冲突 fallback",
        )
    expected_success_keys = {
        "ble_reconnect_succeeded",
        "binding_succeeded",
        "payment_verification_succeeded",
    }
    success_events = route.get("success_events", {})
    if set(success_events) != expected_success_keys:
        add_error(errors, "success_event_keys", "三类成功事件必须完整登记", expected=sorted(expected_success_keys), actual=sorted(success_events))
    success_values = list(success_events.values())
    if any(any(page_id in value for page_id in ["01-01", "02-01", "03-01"]) for value in success_values):
        add_error(errors, "hardcoded_success_route", "成功事件不得写死目标页面")
    concrete_routes = route.get("concrete_routes", [])
    required_route_fields = {"route_id", "from", "event", "guard", "to", "fallback", "history_policy"}
    if (
        len(concrete_routes) != 41
        or len({item.get("route_id") for item in concrete_routes if isinstance(item, dict)}) != 41
        or any(not isinstance(item, dict) or set(item) != required_route_fields or not all(item.values()) for item in concrete_routes)
    ):
        add_error(errors, "concrete_routes", "全局路由卡 yEaCt 必须投影为恰好 41 条七字段路由")
    if concrete_routes != html_concrete_routes(html_path):
        add_error(errors, "concrete_routes_projection", "manifest concrete_routes 必须与 HTML 全局路由卡 yEaCt 逐条一致")
    if route.get("concrete_routes_source") != {"pencil_node_id": "yEaCt", "count": 41}:
        add_error(errors, "concrete_routes_source", "41 条路由必须锁定 Pencil 全局路由卡 yEaCt")
    concrete_route_ids = {
        item.get("route_id")
        for item in concrete_routes
        if isinstance(item, dict) and item.get("route_id")
    }
    route_definitions = route.get("routes", {})
    missing_route_refs = sorted(
        {
            ref
            for item in state_registry
            for ref in item.get("route_refs", [])
            if ref not in route_definitions
        }
    )
    if missing_route_refs:
        add_error(errors, "route_ref_missing", "状态引用了未登记路由", refs=missing_route_refs)

    def validate_project_selector(selector, contract_code: str, **details) -> dict | None:
        if not isinstance(selector, dict):
            add_error(errors, contract_code, "机器合同 selector 必须是对象", **details)
            return None
        node = project_node_by_guid.get(selector.get("guid"))
        expected_component_path = component_path_for(node, project_parent_by_guid) if node else None
        if (
            node is None
            or selector.get("logical_name") != object_name(node)
            or selector.get("component_path") != expected_component_path
        ):
            add_error(errors, contract_code, "机器合同 selector 无法解析到工程对象或组件路径", selector=selector, **details)
            return None
        return node

    def valid_lvgl_operations(operations, args_key: str = "args") -> bool:
        return bool(operations) and all(
            isinstance(operation, dict)
            and str(operation.get("op", "")).startswith("lv_")
            and isinstance(operation.get(args_key), list)
            for operation in operations
        )

    event_bindings = route.get("event_bindings", [])
    state_event_pairs = {
        (item.get("state"), callback)
        for item in state_registry
        for callback in item.get("events", [])
    }
    binding_pairs = {
        (binding.get("state"), binding.get("callback"))
        for binding in event_bindings
        if isinstance(binding, dict)
    }
    generated_callbacks = {
        callback
        for source in all_sources
        for callback in collect_calls(source)
        if callback
    }
    if binding_pairs != state_event_pairs or {pair[1] for pair in binding_pairs} != generated_callbacks:
        add_error(
            errors,
            "event_binding_coverage",
            "每个 state event 与每个 generated callback 都必须有 event_binding",
            missing_pairs=sorted(state_event_pairs - binding_pairs),
            extra_pairs=sorted(binding_pairs - state_event_pairs),
            missing_callbacks=sorted(generated_callbacks - {pair[1] for pair in binding_pairs}),
        )
    for binding in event_bindings:
        if not isinstance(binding, dict):
            add_error(errors, "event_binding_shape", "event_binding 必须是对象")
            continue
        abstract_refs = binding.get("abstract_route_refs", [])
        concrete_refs = binding.get("concrete_route_ids", [])
        if (
            not binding.get("semantic_intent")
            or binding.get("routing_mode") not in {"route_resolution", "typed_intent_only", "typed_intent_then_async_result"}
            or not isinstance(abstract_refs, list)
            or not isinstance(concrete_refs, list)
            or any(ref not in route_definitions for ref in abstract_refs)
            or any(ref not in concrete_route_ids for ref in concrete_refs)
            or (binding.get("routing_mode") == "typed_intent_only" and concrete_refs)
        ):
            add_error(errors, "event_binding_route", "event_binding 的语义、抽象路由或具体路由引用无效", binding=binding)
        selectors = binding.get("source_selectors", [])
        if not selectors:
            add_error(errors, "event_binding_selector", "event_binding 必须至少有一个稳定 source selector", binding=binding)
        callback_found = False
        for selector in selectors:
            node = validate_project_selector(
                selector,
                "event_binding_selector",
                state=binding.get("state"),
                callback=binding.get("callback"),
            )
            if node and binding.get("callback") in collect_calls(node.get("properties", [])):
                callback_found = True
        if selectors and not callback_found:
            add_error(errors, "event_binding_callback_source", "source selector 对象未直接绑定该 callback", binding=binding)

    expected_nav_routes = {
        "nav_slot_01": "route_shell_01",
        "nav_slot_02": "route_shell_02",
        "nav_slot_03": "route_shell_03",
        "nav_slot_04": "route_shell_04",
    }
    for intent, expected_route in expected_nav_routes.items():
        nav_bindings = [binding for binding in event_bindings if binding.get("semantic_intent") == intent]
        if not nav_bindings or any(binding.get("concrete_route_ids") != [expected_route] for binding in nav_bindings):
            add_error(errors, "shell_navigation_event_route", "主壳导航 callback 必须精确映射 route_shell_01..04", intent=intent)

    expected_gps_origin_contract = {
        "field": "selftest_gps_origin",
        "type": "enum",
        "values": ["full_selftest", "retry_detail"],
        "history_policy": "preserve_origin",
        "phase3_resolution": {
            "full_selftest": "return_to_full_selftest_flow",
            "retry_detail": "return_to_08-06_retry_detail",
        },
        "missing_or_invalid_origin": "preserve_current_page_log_and_retry",
    }
    gps_back_bindings = [
        binding
        for binding in event_bindings
        if binding.get("state") in {"selftest_gps_context", "selftest_gps_submit_failed"}
        and "btn_back" in str(binding.get("callback", ""))
    ]
    if (
        {binding.get("state") for binding in gps_back_bindings}
        != {"selftest_gps_context", "selftest_gps_submit_failed"}
        or any(
            binding.get("semantic_intent") != "back_to_origin"
            or binding.get("routing_mode") != "typed_intent_only"
            or binding.get("abstract_route_refs") != ["route_selftest_gps_context"]
            or binding.get("concrete_route_ids") != []
            or binding.get("origin_contract") != expected_gps_origin_contract
            for binding in gps_back_bindings
        )
    ):
        add_error(
            errors,
            "gps_back_origin_contract",
            "08-05 返回必须保留 full_selftest/retry_detail 来源并由阶段三 back_to_origin 解析，不能固定返回维护页",
        )

    if expected_builder is not None:
        try:
            expected_bindings = expected_builder._event_bindings(project_node_by_guid)
            if event_bindings != expected_bindings:
                add_error(errors, "event_binding_projection", "event_binding 必须与 HTML/工程事件投影一致")
        except Exception as error:
            add_error(errors, "event_binding_projection_failed", "无法重建 event_binding", detail=str(error))

    lock_overlay = route.get("selftest_lock_overlay", {})
    control_bindings = lock_overlay.get("controls", []) if isinstance(lock_overlay, dict) else []
    expected_control_pairs = {
        (state, control)
        for state, controls in SELFTEST_LOCK_CONTROLS.items()
        for control in controls
    }
    actual_control_pairs = {
        (item.get("state"), item.get("control"))
        for item in control_bindings
        if isinstance(item, dict)
    }
    if actual_control_pairs != expected_control_pairs or len(control_bindings) != 8:
        add_error(errors, "selftest_lock_overlay_controls", "自检锁定必须覆盖 02-01/03-01 的 8 个控件")
    for item in control_bindings:
        validate_project_selector(item.get("selector"), "selftest_lock_overlay_selector", control=item.get("control"))
        if (
            item.get("page_identity") != STATE_CODES.get(item.get("state"))
            or not valid_lvgl_operations(item.get("apply_operations"))
            or not valid_lvgl_operations(item.get("baseline_restore_operations"))
            or not any(
                operation.get("op") == "lv_obj_add_state" and operation.get("args") == ["DISABLED"]
                for operation in item.get("apply_operations", [])
            )
            or not any(
                operation.get("op") == "lv_obj_clear_state" and operation.get("args") == ["DISABLED"]
                for operation in item.get("baseline_restore_operations", [])
            )
        ):
            add_error(errors, "selftest_lock_overlay_operation", "自检锁定必须提供 DISABLED 应用、视觉操作和 baseline restore", control=item.get("control"))
    navigation = lock_overlay.get("navigation", {}) if isinstance(lock_overlay, dict) else {}
    validate_project_selector(navigation.get("pager_selector"), "selftest_lock_navigation_pager")
    nav_bindings = navigation.get("bindings", []) if isinstance(navigation, dict) else []
    expected_nav_pairs = {(state, intent) for state in SELFTEST_LOCK_CONTROLS for intent in SHELL_NAV_SOURCES}
    actual_nav_pairs = {
        (item.get("state"), item.get("navigation_intent"))
        for item in nav_bindings
        if isinstance(item, dict)
    }
    if (
        actual_nav_pairs != expected_nav_pairs
        or navigation.get("swipe_policy") != "preserve_horizontal_scroll_and_page_change"
        or not valid_lvgl_operations(navigation.get("pager_preserve_operations"))
    ):
        add_error(errors, "selftest_lock_navigation_contract", "自检锁定必须保留 pager、滑动与 8 个状态导航 selector")
    for item in nav_bindings:
        validate_project_selector(item.get("selector"), "selftest_lock_navigation_selector", state=item.get("state"))
        if not valid_lvgl_operations(item.get("preserve_operations")):
            add_error(errors, "selftest_lock_navigation_operation", "导航 selector 必须有保持可用操作", state=item.get("state"))
    if expected_builder is not None:
        try:
            logical_screens = {
                logical_screen_by_guid[screen.get("guid")]: screen
                for screen in screens
                if screen.get("guid") in logical_screen_by_guid
            }
            if lock_overlay != expected_builder._selftest_lock_overlay(logical_screens):
                add_error(errors, "selftest_lock_overlay_projection", "自检锁定 overlay 必须与真实工程对象投影一致")
        except Exception as error:
            add_error(errors, "selftest_lock_overlay_projection_failed", "无法重建自检锁定 overlay", detail=str(error))

    model_projection = route.get("model_projection", {})
    model_selector_owners: dict[str, list[dict]] = {}
    if set(model_projection) != set(ROOT_MODEL_FIELDS):
        add_error(errors, "model_projection_roots", "model_projection 必须覆盖 13 个页面根对象")
    for root_name, fields in ROOT_MODEL_FIELDS.items():
        entries = model_projection.get(root_name, [])
        if [entry.get("field") for entry in entries if isinstance(entry, dict)] != fields:
            add_error(errors, "model_projection_fields", "model_projection 字段集合或顺序不一致", root=root_name)
        for entry in entries:
            bindings = entry.get("bindings", []) if isinstance(entry, dict) else []
            resolver = entry.get("required_phase3_resolver") if isinstance(entry, dict) else None
            field_name = entry.get("field") if isinstance(entry, dict) else None
            expected_pairs = INDEPENDENT_MODEL_BINDING_EXPECTATIONS.get((root_name, field_name), set())
            actual_pairs = {
                (binding.get("state"), binding.get("source_object"))
                for binding in bindings
                if isinstance(binding, dict)
            }
            if actual_pairs != expected_pairs or len(bindings) != len(expected_pairs):
                add_error(
                    errors,
                    "model_projection_independent_binding",
                    "model binding 必须匹配独立显式白名单；禁止字段名子串猜测和截断 descendant",
                    root=root_name,
                    field=field_name,
                    expected=sorted(expected_pairs),
                    actual=sorted(actual_pairs),
                )
            if not entry.get("value_format") or (not bindings and not resolver):
                add_error(errors, "model_projection_empty", "每个 model field 必须有 selector/operation 或明确 phase3 resolver", root=root_name, field=entry.get("field"))
            for binding in bindings:
                node = validate_project_selector(binding.get("selector"), "model_projection_selector", root=root_name, field=entry.get("field"))
                operation = binding.get("operation", {}) if isinstance(binding, dict) else {}
                if (
                    node is None
                    or node.get("saved_objtypeKey") != "LABEL"
                    or operation.get("op") != "lv_label_set_text"
                    or not isinstance(operation.get("args_template"), list)
                ):
                    add_error(errors, "model_projection_operation", "model field binding 必须提供 LVGL operation 与参数模板", root=root_name, field=entry.get("field"))
                selector_guid = binding.get("selector", {}).get("guid") if isinstance(binding, dict) else None
                if selector_guid:
                    model_selector_owners.setdefault(selector_guid, []).append(
                        {
                            "root": root_name,
                            "field": field_name,
                            "state": binding.get("state"),
                            "allow_shared_selector": binding.get("allow_shared_selector", False),
                            "shared_selector_reason": binding.get("shared_selector_reason"),
                        }
                    )
            if resolver and (
                resolver.get("owner") != "phase_3_binding"
                or resolver.get("root") != root_name
                or resolver.get("field") != entry.get("field")
                or not resolver.get("strategy")
                or not resolver.get("reason")
                or not resolver.get("must_emit")
            ):
                add_error(errors, "model_projection_resolver", "无法静态推断的 model field 必须给出非空 phase3 resolver", root=root_name, field=entry.get("field"))
            if expected_pairs and resolver:
                add_error(errors, "model_projection_unnecessary_resolver", "已有独立精确绑定的字段不得同时伪装成未解析", root=root_name, field=field_name)
            if not expected_pairs and bindings:
                add_error(errors, "model_projection_unsafe_binding", "无独立白名单的字段必须使用 phase3 resolver，不得伪造 selector", root=root_name, field=field_name)
    for selector_guid, owners in model_selector_owners.items():
        owner_fields = {(owner["root"], owner["field"]) for owner in owners}
        if len(owner_fields) <= 1:
            continue
        if not all(owner["allow_shared_selector"] and owner["shared_selector_reason"] for owner in owners):
            add_error(
                errors,
                "model_projection_selector_conflict",
                "同一 selector 不得被多个 model field 共享，除非逐项显式声明原因",
                selector_guid=selector_guid,
                owners=owners,
            )
    if expected_builder is not None and model_projection != expected_builder._model_projection(project_node_by_guid):
        add_error(errors, "model_projection_source", "model_projection 必须由真实状态对象稳定投影")
    expected_lock = {
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
    }
    if lock != expected_lock:
        add_error(errors, "selftest_lock_controls", "自检锁定必须禁用控制并保留 pager/swipe/navigation", expected=expected_lock, actual=lock)
    expected_interaction_guards = {
        "payment_verifying": ["disable_verify_submit", "reject_duplicate_submit"],
        "clear_binding_clearing": ["disable_confirm", "disable_cancel", "reject_duplicate_clear"],
        "ble_scanning": ["disable_rescan", "reject_duplicate_scan"],
        "gps_submit_loading": ["disable_context_submit", "reject_duplicate_submit"],
    }
    if route.get("interaction_guards") != expected_interaction_guards:
        add_error(errors, "interaction_guards", "验证中/清除中/扫描中/提交中必须防止重复操作")

    code_only = manifest.get("code_only_registry", [])
    if len(code_only) != 1:
        add_error(errors, "amoled_registry_count", "AMOLED 必须且只能有一条 code_only 登记")
    elif code_only[0] != {
        "state": "selftest_amoled_five_color",
        "lifecycle": "code_only_overlay",
        "phase_3_object_name": "ui_selftest_amoled_surface",
        "colors": ["black", "white", "red", "green", "blue"],
        "duration_ms_each": 2000,
        "total_duration_ms": 10000,
        "screen": None,
        "page_state_root_object": None,
        "squareline_component": None,
    }:
        add_error(errors, "amoled_registry_contract", "AMOLED code_only 登记不一致")
    forbidden_amoled_objects = sorted(
        name
        for name in (
            object_name(node)
            for node in [*project_nodes, *(node for component in components.values() for node in walk_nodes(component))]
        )
        if name == "ui_selftest_amoled_surface"
        or name.startswith("ui_scr_amoled")
        or name.startswith("ui_page_amoled")
        or name.startswith("cmp_amoled")
        or name in {"ui_amoled_black", "ui_amoled_white", "ui_amoled_red", "ui_amoled_green", "ui_amoled_blue"}
    )
    if forbidden_amoled_objects or any("amoled" in name for name in component_files):
        add_error(
            errors,
            "amoled_squareline_object",
            "AMOLED 不得生成 Screen、根对象、组件或五色对象",
            objects=forbidden_amoled_objects,
        )

    screen_counts = {
        logical_name: count_nodes(screens_by_guid[screen_guid])
        for logical_name, screen_guid in expected_screen_guids.items()
        if screen_guid in screens_by_guid
    }
    screen_subtree_widgets = sum(screen_counts.values())
    component_master_widgets = sum(count_nodes(component) for component in components.values())
    project_widgets = screen_subtree_widgets + component_master_widgets
    resident_widgets = screen_counts.get("ui_scr_main_shell", 0)
    peak_widgets = resident_widgets + max(
        (count for name, count in screen_counts.items() if name != "ui_scr_main_shell"),
        default=0,
    )
    expected_counts = {
        "canonical_states": 23,
        "full_page_variants": 7,
        "screens": 9,
        "page_state_root_objects": 13,
        "squareline_components": 4,
        "code_only_states": 1,
        "studio_screen_subtree_widgets": screen_subtree_widgets,
        "studio_component_master_widgets": component_master_widgets,
        "studio_project_widgets": project_widgets,
        "runtime_resident_widgets": resident_widgets,
        "runtime_peak_static_widgets": peak_widgets,
        "component_count": 4,
    }
    if manifest.get("counts") != expected_counts:
        add_error(errors, "capacity_count_mismatch", "manifest 容量计数与工程不一致", expected=expected_counts, actual=manifest.get("counts"))

    capacity = manifest.get("capacity", {})
    if capacity.get("screen_widget_counts") != screen_counts:
        add_error(errors, "screen_widget_count_mismatch", "逐 Screen 对象数不一致")
    editor_visible_widget_counter = screen_subtree_widgets - len(screens)
    if (
        capacity.get("editor_visible_widget_counter") != editor_visible_widget_counter
        or capacity.get("editor_visible_widget_counter") != 509
        or capacity.get("project_widget_counter_including_screens_and_component_masters") != project_widgets
        or project_widgets != 537
    ):
        add_error(
            errors,
            "capacity_editor_project_counters",
            "容量必须同时记录编辑器可见 509 widgets 与含 Screen/组件 master 的工程 537 对象",
            editor_visible=editor_visible_widget_counter,
            project=project_widgets,
        )
    if (
        capacity.get("trial_supports_current_project_if_active") is not True
        or capacity.get("current_session_trial_supports_current_project") is not False
    ):
        add_error(errors, "trial_capacity_context", "Trial 理论能力与当前未激活会话必须分开登记")
    if (
        capacity.get("active_license") != "personal"
        or capacity.get("active_license_supports_current_project") is not False
        or capacity.get("studio_save_status") != "blocked_by_personal_license_over_150_widgets"
    ):
        add_error(errors, "active_license_capacity", "当前 Personal 许可证必须登记为无法保存本工程")
    if capacity.get("runtime_memory_32k_verified") is not False:
        add_error(errors, "runtime_memory_claim", "阶段二不得宣称 32 KiB 运行时内存已验证")
    if not capacity.get("personal_supports_current_project"):
        warnings.append(
            {
                "code": "personal_license_incompatible",
                "message": "当前 Personal 的 1 组件/150 widgets 限制不满足；GUI 保存往返被许可证阻断。",
            }
        )
        warnings.append(
            {
                "code": "studio_save_roundtrip_blocked_by_license",
                "message": "SquareLine 1.6.1 已打开工程，但 Cmd+S 被 Personal 150 widgets 限制阻断；未宣称保存往返通过。",
            }
        )

    font_audit = manifest.get("font_coverage_audit", {})
    expected_font_audit = {
        "status": "machine_enforced",
        "severity": "error_on_missing_glyph",
        "contract": "font_glyph_contract.json",
        "validator": "tools/validate_font_coverage.py",
        "scope": "squareline_static_labels_runtime_label_contract_and_bounded_external_text",
        "generation_policy": "always_rebuild_and_sync_squareline_font_sources_to_firmware",
    }
    if font_audit != expected_font_audit:
        add_error(
            errors,
            "font_coverage_audit",
            "字体覆盖必须由既有字形合同和静态验证器机器闭合",
            expected=expected_font_audit,
            actual=font_audit,
        )
    blocking_warnings = manifest.get("blocking_warnings", [])
    blocking_by_code = {
        item.get("code"): item
        for item in blocking_warnings
        if isinstance(item, dict) and item.get("code")
    }
    license_warning = blocking_by_code.get("studio_save_roundtrip_blocked_by_license", {})
    if (
        len(blocking_warnings) != 1
        or license_warning.get("status") != "open"
        or license_warning.get("resolution")
        != "activate_trial_or_appropriate_commercial_license_then_repeat_save_close_reopen"
    ):
        add_error(errors, "blocking_warnings", "Personal 许可证保存阻断必须继续保持开放，不能伪造通过")

    legacy_files = [
        Path(__file__).with_name("generate_squareline_project.py"),
        project_dir / "project_manifest.json",
        project_dir / "watch-lvgl.spj",
        project_dir / "watch-lvgl_events.py",
        *sorted(component_dir.glob("*.ecomp")),
    ]
    legacy_hits = [str(path) for path in legacy_files if path.exists() and "home_unavailable" in path.read_text(encoding="utf-8")]
    if legacy_hits:
        add_error(errors, "legacy_home_name", "SquareLine 范围仍残留 home_unavailable", files=legacy_hits)

    project_root = project_dir.parents[1]
    documentation_export_files = [
        project_dir / "watch-lvgl.spj",
        *sorted(component_dir.glob("*.ecomp")),
        *sorted((project_root / "components" / "ui" / "generated").rglob("*.c")),
        *sorted((project_root / "components" / "ui" / "generated").rglob("*.h")),
    ]
    documentation_leaks = []
    for path in documentation_export_files:
        if not path.exists():
            continue
        export_text = path.read_text(encoding="utf-8")
        markers = [
            marker
            for marker in DOCUMENTATION_EXPORT_MARKERS
            if marker in export_text
        ]
        if markers:
            documentation_leaks.append({"file": str(path), "markers": markers})
    if documentation_leaks:
        add_error(
            errors,
            "documentation_object_export_leak",
            "页面说明、规范映射或运行时参考稿不得进入 SquareLine 工程和固件 generated C",
            files=documentation_leaks,
        )

    font_dir = project_dir / "assets" / "Fonts"
    configs = sorted(font_dir.glob("ui_font_*.fcfg"))
    missing_font_outputs = [
        cfg.stem
        for cfg in configs
        if not (font_dir / f"{cfg.stem}.bin").exists() or not (font_dir / f"{cfg.stem}.c").exists()
    ]
    if missing_font_outputs:
        add_error(errors, "font_outputs_missing", "现有字体配置缺少已登记静态输出；阶段二禁止现场转换字体", fonts=missing_font_outputs)

    utf8_files = [
        Path(__file__),
        Path(__file__).with_name("generate_squareline_project.py"),
        project_dir / "project_manifest.json",
        project_dir / "watch-lvgl.spj",
        project_dir / "watch-lvgl.sll",
        project_dir / "watch-lvgl.slp",
        project_dir / "watch-lvgl_events.py",
        *sorted(component_dir.glob("*.ecomp")),
    ]
    bom_files = []
    mojibake_files = []
    mojibake_markers = [chr(0x7EAD), chr(0x951B), chr(0x6D93), chr(0x5BEE) + chr(0x20AC)]
    for path in utf8_files:
        if path.exists() and path.read_bytes().startswith(b"\xef\xbb\xbf"):
            bom_files.append(str(path))
        if path.exists() and any(marker in path.read_text(encoding="utf-8") for marker in mojibake_markers):
            mojibake_files.append(str(path))
    if bom_files:
        add_error(errors, "utf8_bom", "生成文本必须是 UTF-8 无 BOM", files=bom_files)
    if mojibake_files:
        add_error(errors, "mojibake", "生成文本疑似包含中文乱码", files=mojibake_files)

    return {
        "ok": not errors,
        "summary": {
            **expected_counts,
            "html_critical_objects": html_stats["critical_objects"],
            "centered_page_titles": html_stats["page_titles"],
            "html_identity_mode": html_stats["identity_mode"],
            "state_patch_critical_objects": state_patch_critical_objects,
            "squareline_static_critical_objects": static_geometry_stats["critical_objects"],
            "squareline_static_page_titles": static_geometry_stats["page_titles"],
            "component_instances": actual_instances,
            "events": event_count,
            "change_screen_actions": len(change_screen_actions),
            "font_configs": len(configs),
            "two_mode_reference_states": len(TWO_MODE_REFERENCE_STATES),
            "documentation_export_leaks": len(documentation_leaks),
            "license": "personal_save_blocked",
            "compiled": False,
        },
        "warnings": warnings,
        "errors": errors,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--html", type=Path, required=True)
    parser.add_argument("--project-dir", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = validate(args.project_dir, args.html)
    except Exception as error:  # 验证器始终输出结构化错误，不暴露 Python traceback。
        result = {
            "ok": False,
            "summary": {},
            "warnings": [],
            "errors": [{"code": "validator_failed", "message": str(error)}],
        }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    raise SystemExit(0 if result["ok"] else 1)


if __name__ == "__main__":
    main()
