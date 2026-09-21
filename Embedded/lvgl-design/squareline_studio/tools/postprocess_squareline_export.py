#!/usr/bin/env python3
"""规范化 SquareLine 导出并将 09-01 模板转为按需 modal 工厂。"""

from __future__ import annotations

import argparse
from pathlib import Path


TEXT_SUFFIXES = {".c", ".h", ".json", ".txt"}

HOST_EVENTS = """lv_obj_add_event_cb(ui_ui_maintenance_btn_back, ui_event_ui_maintenance_btn_back, LV_EVENT_ALL, NULL);
lv_obj_add_event_cb(ui_ui_maintenance_row_cellular, ui_event_ui_maintenance_row_cellular, LV_EVENT_ALL, NULL);
lv_obj_add_event_cb(ui_ui_maintenance_row_selftest, ui_event_ui_maintenance_row_selftest, LV_EVENT_ALL, NULL);
lv_obj_add_event_cb(ui_ui_maintenance_row_clear_binding, ui_event_ui_maintenance_row_clear_binding, LV_EVENT_ALL, NULL);"""

MODAL_POINTERS = (
    "ui_ui_modal_clear_binding_root",
    "ui_ui_clear_binding_confirm_modal_panel",
    "ui_ui_clear_binding_confirm_icon_clear_binding",
    "ui_ui_clear_binding_confirm_txt_confirm_title",
    "ui_ui_clear_binding_confirm_confirm_rule",
    "ui_ui_clear_binding_confirm_txt_confirm_effect",
    "ui_ui_clear_binding_confirm_txt_confirm_rebind",
    "ui_ui_clear_binding_confirm_btn_confirm_clear_binding",
    "ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon",
    "ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label",
    "ui_ui_clear_binding_confirm_btn_cancel",
    "ui_ui_clear_binding_confirm_btn_cancel_icon",
    "ui_ui_clear_binding_confirm_btn_cancel_label",
    "ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine",
    "ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow",
    "ui_ui_clear_binding_confirm_txt_clear_binding_scope",
)


def normalize_text(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    normalized = "\n".join(line.rstrip(" \t\r") for line in text.splitlines())
    path.write_text(normalized.rstrip() + "\n", encoding="utf-8")


def detach_modal(source_path: Path, header_path: Path) -> bool:
    source = source_path.read_text(encoding="utf-8")
    header = header_path.read_text(encoding="utf-8")
    create_signature = "void ui_ui_clear_binding_modal_create(lv_obj_t *parent)"
    destroy_signature = "void ui_ui_clear_binding_modal_destroy(void)"
    if create_signature in source and destroy_signature in source:
        if "ui_ui_modal_clear_binding_root = lv_obj_create(ui_ui_scr_maintenance);" in source:
            raise SystemExit("modal 同时存在常驻创建与按需工厂")
        return False

    modal_marker = (
        "ui_ui_modal_clear_binding_root = "
        "lv_obj_create(ui_ui_scr_maintenance);"
    )
    screen_destroy_marker = "void ui_ui_scr_maintenance_screen_destroy(void)"
    modal_start = source.find(modal_marker)
    screen_destroy_start = source.find(screen_destroy_marker)
    if modal_start < 0 or screen_destroy_start < 0 or modal_start >= screen_destroy_start:
        raise SystemExit("SquareLine 维护页 modal 布局结构与预期不一致")

    modal_block = source[modal_start:screen_destroy_start]
    if HOST_EVENTS not in modal_block:
        raise SystemExit("SquareLine 维护页 host 事件块缺失")
    modal_block = modal_block.replace(HOST_EVENTS + "\n", "", 1)
    modal_block = modal_block.replace(
        modal_marker,
        "ui_ui_modal_clear_binding_root = lv_obj_create(parent);",
        1,
    )

    destroy_lines = [
        destroy_signature,
        "{",
        "if (ui_ui_modal_clear_binding_root) lv_obj_del(ui_ui_modal_clear_binding_root);",
        *(f"{name}= NULL;" for name in MODAL_POINTERS),
        "}",
        "",
    ]
    factory_prefix = (
        HOST_EVENTS
        + "\n\n}\n\n"
        + create_signature
        + "\n{\n"
        + "if (parent == NULL || ui_ui_modal_clear_binding_root != NULL) return;\n\n"
    )
    source = (
        source[:modal_start]
        + factory_prefix
        + modal_block
        + "\n".join(destroy_lines)
        + "\n"
        + source[screen_destroy_start:]
    )
    destroy_body_marker = screen_destroy_marker + "\n{\n"
    if destroy_body_marker not in source:
        raise SystemExit("SquareLine 维护页 destroy 签名与预期不一致")
    source = source.replace(
        destroy_body_marker,
        destroy_body_marker + "   ui_ui_clear_binding_modal_destroy();\n",
        1,
    )

    header_marker = "extern void ui_ui_scr_maintenance_screen_destroy(void);"
    if header_marker not in header:
        raise SystemExit("SquareLine 维护页头文件缺少 screen destroy 声明")
    header = header.replace(
        header_marker,
        header_marker
        + "\nextern void ui_ui_clear_binding_modal_create(lv_obj_t *parent);"
        + "\nextern void ui_ui_clear_binding_modal_destroy(void);",
        1,
    )
    source_path.write_text(source, encoding="utf-8")
    header_path.write_text(header, encoding="utf-8")
    return True


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--generated", type=Path, required=True)
    args = parser.parse_args()
    generated = args.generated.resolve()
    source = generated / "screens/ui_ui_scr_maintenance.c"
    header = generated / "screens/ui_ui_scr_maintenance.h"
    changed = detach_modal(source, header)
    for path in generated.rglob("*"):
        if path.is_file() and (
            path.suffix.lower() in TEXT_SUFFIXES or path.name == "CMakeLists.txt"
        ):
            normalize_text(path)
    print("postprocessed modal=detached" if changed else "postprocessed modal=already-detached")


if __name__ == "__main__":
    main()
