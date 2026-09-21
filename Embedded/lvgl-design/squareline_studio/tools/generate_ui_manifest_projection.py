#!/usr/bin/env python3
"""从 SquareLine manifest 生成固件静态状态投影。"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def enum_name(state_key: str) -> str:
    return "UI_MANIFEST_STATE_" + re.sub(r"[^A-Z0-9]+", "_", state_key.upper())


def load_exported_symbols(generated_dir: Path) -> set[str]:
    symbols: set[str] = set()
    pattern = re.compile(r"extern\s+lv_obj_t\s*\*\s*(ui_[A-Za-z0-9_]+)\s*;")
    for header in generated_dir.rglob("*.h"):
        symbols.update(pattern.findall(header.read_text(encoding="utf-8")))
    return symbols


def load_component_macros(generated_dir: Path) -> set[str]:
    macros: set[str] = set()
    pattern = re.compile(r"^#define\s+(UI_COMP_[A-Z0-9_]+)\s+\d+\s*$", re.MULTILINE)
    for header in (generated_dir / "components").glob("*.h"):
        macros.update(pattern.findall(header.read_text(encoding="utf-8")))
    return macros


def state_entries(manifest: dict) -> list[tuple[str, dict]]:
    entries: list[tuple[str, dict]] = []
    for state in manifest["state_registry"]:
        patch = state.get("state_patch", {})
        required = {"baseline_reset", "show", "hide", "overrides"}
        missing = required - set(patch)
        if missing:
            raise SystemExit(f"{state['state_key']} 缺少 state_patch 字段: {sorted(missing)}")

        baseline_names = {item["target"]["logical_name"] for item in patch["baseline_reset"]}
        show_names = {item["logical_name"] for item in patch["show"]}
        hide_names = {item["logical_name"] for item in patch["hide"]}
        override_names = {item["target"]["logical_name"] for item in patch["overrides"].values()}
        if show_names & hide_names or show_names | hide_names != baseline_names:
            raise SystemExit(f"{state['state_key']} 的 show/hide 与 baseline_reset 不一致")
        if show_names != override_names:
            raise SystemExit(f"{state['state_key']} 的 show 与 overrides 不一致")
        entries.append((state["state_key"], patch))
    return entries


def target_expression(descriptor: dict, root_symbol: str, component_by_guid: dict[str, str]) -> tuple[str, str | None]:
    target = descriptor["target"]
    component_path = target.get("component_path")
    if component_path and len(component_path.get("coid_path", [])) > 1:
        component_name = component_by_guid.get(component_path["cid"])
        if component_name is None:
            raise SystemExit(f"未知组件 GUID: {component_path['cid']}")
        macro = "UI_COMP_" + component_name.upper() + "_" + target["logical_name"].upper()
        return f"ui_comp_get_child({root_symbol}, {macro})", macro
    return "ui_" + target["logical_name"], None


def walk_descriptor_targets(
    descriptor: dict,
    component_by_guid: dict[str, str],
    component_root_symbol: str = "",
):
    target = descriptor["target"]
    component_path = target.get("component_path")
    next_component_root = component_root_symbol
    if component_path and len(component_path.get("coid_path", [])) == 1:
        next_component_root = "ui_" + target["logical_name"]
    elif not component_path:
        next_component_root = ""
    expression, macro = target_expression(
        descriptor,
        next_component_root,
        component_by_guid,
    )
    yield descriptor, expression, macro
    for child in descriptor.get("children", []):
        yield from walk_descriptor_targets(
            child,
            component_by_guid,
            next_component_root,
        )


def c_string(value: str) -> str:
    return json.dumps(value.replace("\\n", "\n"), ensure_ascii=False)


def color_hex(args: list[int]) -> str:
    if len(args) < 3:
        raise SystemExit(f"颜色操作参数不足: {args}")
    return f"0x{args[0]:02X}{args[1]:02X}{args[2]:02X}"


def render_operation(symbol: str, operation: dict) -> str:
    op = operation.get("op")
    args = operation.get("args", [])
    target = f"{symbol}"
    if op == "lv_obj_set_pos" and len(args) == 2:
        return f"            lv_obj_set_pos({target}, {args[0]}, {args[1]});"
    if op == "lv_obj_set_size" and len(args) == 2:
        return f"            lv_obj_set_size({target}, {args[0]}, {args[1]});"
    if op in {"lv_obj_add_flag", "lv_obj_clear_flag"} and args == ["HIDDEN"]:
        return f"            {op}({target}, LV_OBJ_FLAG_HIDDEN);"
    if op in {"lv_obj_add_state", "lv_obj_clear_state"} and args in (["DISABLED"], ["CHECKED"]):
        return f"            {op}({target}, LV_STATE_{args[0]});"
    if op == "lv_label_set_text" and len(args) == 1:
        return f"            lv_label_set_text({target}, {c_string(args[0])});"
    if op in {"lv_obj_set_style_text_color", "lv_obj_set_style_bg_color", "lv_obj_set_style_border_color"}:
        return f"            {op}({target}, lv_color_hex({color_hex(args)}), LV_PART_MAIN | LV_STATE_DEFAULT);"
    if op == "lv_obj_set_style_text_font" and len(args) == 1:
        return f"            {op}({target}, &ui_font_{args[0]}, LV_PART_MAIN | LV_STATE_DEFAULT);"
    if op == "lv_obj_set_style_text_align" and len(args) == 1:
        return f"            {op}({target}, LV_TEXT_ALIGN_{args[0]}, LV_PART_MAIN | LV_STATE_DEFAULT);"
    if op in {"lv_obj_set_style_bg_opa", "lv_obj_set_style_radius", "lv_obj_set_style_border_width"} and len(args) == 1:
        return f"            {op}({target}, {args[0]}, LV_PART_MAIN | LV_STATE_DEFAULT);"
    raise SystemExit(f"不支持的 manifest 操作: {operation}")


def render_header(entries: list[tuple[str, dict]]) -> str:
    enum_lines = [f"        {enum_name(key)}, /**< {key} 静态投影。 */" for key, _ in entries]
    return """/**
 * @file     ui_manifest_projection.h
 * @brief    SquareLine manifest 静态状态投影接口
 * @details  由工具机械生成对象基线与状态覆盖；不包含业务事实读取或页面路由。
 * @author   ZHC
 * @date     2026-07-20
 */

#ifndef LEGBOT_UI_MANIFEST_PROJECTION_H
#define LEGBOT_UI_MANIFEST_PROJECTION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** manifest 中 23 个 canonical 与 7 个整页变体。 */
    typedef enum
    {
""" + "\n".join(enum_lines) + """
        UI_MANIFEST_STATE_COUNT /**< manifest 静态投影数量。 */
    } ui_manifest_state_t;

    /**
     * @brief 按 baseline_reset、canonical、variant 顺序应用一个 manifest 静态状态
     * @param state 静态状态 ID
     * @return true 已应用，false 表示状态或对象生命周期无效
     */
    bool ui_manifest_projection_apply(ui_manifest_state_t state);

    /**
     * @brief 返回静态状态对应的 manifest state_key
     * @param state 静态状态 ID
     * @return 稳定 state_key；参数无效时返回 invalid
     */
    const char *ui_manifest_projection_key(ui_manifest_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_MANIFEST_PROJECTION_H */
"""


def render_source(
    entries: list[tuple[str, dict]],
    exported: set[str],
    component_macros: set[str],
    component_by_guid: dict[str, str],
) -> str:
    cases: list[str] = []
    missing: list[str] = []
    keys: list[str] = []
    for key, patch in entries:
        keys.append(key)
        targets: list[tuple[dict, str]] = []
        for descriptor in patch["baseline_reset"]:
            expression, _ = target_expression(descriptor, "", component_by_guid)
            targets.append((descriptor, expression))
            if expression not in exported:
                missing.append(expression)
        for override in patch["overrides"].values():
            for descriptor, expression, macro in walk_descriptor_targets(
                override,
                component_by_guid,
            ):
                targets.append((descriptor, expression))
                if macro is None and expression not in exported:
                    missing.append(expression)
                if macro is not None and macro not in component_macros:
                    raise SystemExit(f"组件子对象宏未导出: {macro}")

        lines = [f"        case {enum_name(key)}:"]
        expressions = list(dict.fromkeys(expression for _, expression in targets))
        for expression in expressions:
            lines.append(f"            if ({expression} == NULL)")
            lines.append("            {")
            lines.append("                return false;")
            lines.append("            }")
        for descriptor in patch["baseline_reset"]:
            expression, _ = target_expression(descriptor, "", component_by_guid)
            lines.extend(render_operation(expression, operation) for operation in descriptor["operations"])
        for override in patch["overrides"].values():
            for descriptor, expression, _ in walk_descriptor_targets(
                override,
                component_by_guid,
            ):
                lines.extend(render_operation(expression, operation) for operation in descriptor["operations"])
        lines.append("            return true;")
        cases.append("\n".join(lines))
    if missing:
        raise SystemExit("manifest 目标未在 SquareLine 导出头文件中声明: " + ", ".join(sorted(set(missing))))
    return """/**
 * @file     ui_manifest_projection.c
 * @brief    SquareLine manifest 静态状态投影实现
 * @details  由工具机械生成，严格执行对象基线与 canonical/variant 的完整覆盖操作。
 * @author   ZHC
 * @date     2026-07-22
 */

#include "ui_manifest_projection.h"

#include "ui.h"

bool ui_manifest_projection_apply(ui_manifest_state_t state)
{
    switch (state)
    {
""" + "\n".join(cases) + """
        default:
            return false;
    }
}

const char *ui_manifest_projection_key(ui_manifest_state_t state)
{
    static const char *const keys[UI_MANIFEST_STATE_COUNT] = {
""" + "\n".join(f'        [{enum_name(key)}] = "{key}",' for key in keys) + """
    };
    return state >= 0 && state < UI_MANIFEST_STATE_COUNT ? keys[state] : "invalid";
}
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    entries = state_entries(manifest)
    if len(entries) != 30:
        raise SystemExit(f"预期 30 个 manifest 静态状态，实际 {len(entries)}")
    exported = load_exported_symbols(args.generated)
    component_macros = load_component_macros(args.generated)
    component_by_guid = {
        item["component_guid"]: item["squareline_component"] for item in manifest["component_registry"]
    }
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "ui_manifest_projection.h").write_text(render_header(entries), encoding="utf-8")
    (args.output / "ui_manifest_projection.c").write_text(
        render_source(entries, exported, component_macros, component_by_guid), encoding="utf-8"
    )
    print(f"generated {len(entries)} states")


if __name__ == "__main__":
    main()
