#!/usr/bin/env python3
"""验证 SquareLine 静态/运行时文案与固件字体资源的字形闭合。"""

from __future__ import annotations

import json
import re
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
PROJECT_DIR = ROOT / "lvgl-design/squareline_studio"
CONTRACT_PATH = PROJECT_DIR / "font_glyph_contract.json"
FONT_DIR = PROJECT_DIR / "assets/Fonts"
GENERATED_FONT_DIR = ROOT / "components/ui/generated/fonts"
GENERATED_UI_DIRS = (
    ROOT / "components/ui/generated/screens",
    ROOT / "components/ui/generated/components",
)
MANIFEST_PROJECTION_PATH = ROOT / "components/ui/bindings/ui_manifest_projection.c"
PROJECT_MANIFEST_PATH = PROJECT_DIR / "project_manifest.json"
ASCII_DYNAMIC = set(chr(code) for code in range(0x20, 0x7F))


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    raise SystemExit(1)


def walk_project_nodes(node: dict):
    yield node
    for child in node.get("children", []):
        yield from walk_project_nodes(child)


def iter_properties(value):
    if isinstance(value, dict):
        if "strtype" in value:
            yield value
        for child in value.values():
            yield from iter_properties(child)
    elif isinstance(value, list):
        for child in value:
            yield from iter_properties(child)


def label_fields(node: dict) -> tuple[str, str, str]:
    properties = list(iter_properties(node.get("properties", [])))

    def value(property_type: str) -> str:
        return next(
            (item.get("strval", "") for item in properties if item.get("strtype") == property_type),
            "",
        )

    return value("OBJECT/Name"), value("LABEL/Text"), value("_style/Text_Font")


def authoring_roots() -> list[dict]:
    project = json.loads((PROJECT_DIR / "watch-lvgl.spj").read_text(encoding="utf-8"))
    roots = list(project.get("root", {}).get("children", []))
    for path in sorted((PROJECT_DIR / "components").glob("*.ecomp")):
        roots.append(json.loads(path.read_text(encoding="utf-8")))
    return roots


def function_body(source: str, name: str) -> str:
    match = re.search(
        rf"^[ \t]*(?:static[ \t]+)?(?:const[ \t]+)?"
        rf"[A-Za-z_][A-Za-z0-9_]*[ \t]+(?:\*[ \t]*)?"
        rf"{re.escape(name)}\s*\([^;{{}}]*\)\s*\{{",
        source,
        flags=re.M | re.S,
    )
    if match is None:
        fail(f"运行时字形合同引用了不存在的函数: {name}")
    opening = source.find("{", match.start())
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1:index]
    fail(f"运行时字形合同引用的函数未闭合: {name}")
    return ""


def runtime_non_ascii_literals(contract: dict) -> set[str]:
    literals: set[str] = set()
    for source_entry in contract.get("runtime_sources", []):
        path = ROOT / source_entry["path"]
        source = path.read_text(encoding="utf-8")
        for function_name in source_entry.get("functions", []):
            body = function_body(source, function_name)
            body = re.sub(r"//[^\n]*|/\*.*?\*/", "", body, flags=re.S)
            for literal in re.findall(r'"((?:\\.|[^"\\])*)"', body):
                if any(ord(character) >= 0x80 for character in literal):
                    literals.add(literal)
    return literals


def glyph_codepoints(path: Path) -> set[str]:
    source = path.read_text(encoding="utf-8")
    return {
        chr(int(codepoint, 16))
        for codepoint in re.findall(r"/\* U\+([0-9A-Fa-f]{4,6})", source)
    }


def manifest_projection_labels() -> list[tuple[str, str]]:
    """提取状态投影中同一标签实际写入的文本和字体。"""
    source = MANIFEST_PROJECTION_PATH.read_text(encoding="utf-8")
    labels: list[tuple[str, str]] = []
    text_pattern = re.compile(
        r'lv_label_set_text\(\s*(.+?)\s*,\s*"((?:\\.|[^"\\])*)"\s*\);'
    )
    font_pattern = re.compile(
        r"lv_obj_set_style_text_font\(\s*(.+?)\s*,\s*&ui_font_"
        r"([A-Za-z0-9_]+)\s*,"
    )
    for text_match in text_pattern.finditer(source):
        selector = text_match.group(1).strip()
        text = json.loads(f'"{text_match.group(2)}"')
        next_text = text_pattern.search(source, text_match.end())
        operation_end = next_text.start() if next_text is not None else len(source)
        font_match = next(
            (
                match
                for match in font_pattern.finditer(
                    source, text_match.end(), operation_end
                )
                if match.group(1).strip() == selector
            ),
            None,
        )
        if font_match is None:
            fail(f"状态投影标签缺少可追踪字体: {selector} -> {text}")
        labels.append((font_match.group(2), text))
    return labels


def generated_export_labels() -> list[tuple[str, str]]:
    """从固件实际编译的 SquareLine 导出 C 中提取基础标签文本和字体。"""
    text_pattern = re.compile(
        r'lv_label_set_text\(\s*([^,]+?)\s*,\s*"((?:\\.|[^"\\])*)"\s*\);'
    )
    font_pattern = re.compile(
        r"lv_obj_set_style_text_font\(\s*([^,]+?)\s*,\s*&ui_font_"
        r"([A-Za-z0-9_]+)\s*,"
    )
    labels: list[tuple[str, str]] = []
    for directory in GENERATED_UI_DIRS:
        for path in sorted(directory.glob("*.c")):
            source = path.read_text(encoding="utf-8")
            fonts_by_selector: dict[str, set[str]] = defaultdict(set)
            for match in font_pattern.finditer(source):
                fonts_by_selector[match.group(1).strip()].add(match.group(2))
            for match in text_pattern.finditer(source):
                selector = match.group(1).strip()
                font_codes = fonts_by_selector.get(selector, set())
                if len(font_codes) != 1:
                    fail(
                        "固件导出标签字体不可唯一追踪: "
                        f"{path.relative_to(ROOT)} -> {selector} -> "
                        f"{sorted(font_codes)}"
                    )
                labels.append(
                    (
                        next(iter(font_codes)),
                        json.loads(f'"{match.group(2)}"'),
                    )
                )
    return labels


def projected_fonts_by_label() -> dict[str, set[str]]:
    """提取每个逻辑标签在所有状态投影中实际绑定的字体集合。"""
    manifest = json.loads(PROJECT_MANIFEST_PATH.read_text(encoding="utf-8"))
    result: dict[str, set[str]] = defaultdict(set)

    def walk_descriptor(descriptor: dict):
        yield descriptor
        for child in descriptor.get("children", []):
            yield from walk_descriptor(child)

    for state in manifest.get("state_registry", []):
        patch = state.get("state_patch")
        if not patch:
            continue
        for override in patch.get("overrides", {}).values():
            for descriptor in walk_descriptor(override):
                operations = descriptor.get("operations", [])
                font_codes = [
                    operation.get("args", [""])[0]
                    for operation in operations
                    if operation.get("op") == "lv_obj_set_style_text_font"
                ]
                if not font_codes:
                    continue
                if len(font_codes) != 1:
                    fail(
                        "状态投影标签字体不唯一: "
                        f"{state.get('state_key')} -> "
                        f"{descriptor.get('target', {}).get('logical_name', '')}"
                    )
                selector = descriptor.get("target", {}).get("logical_name", "")
                if selector:
                    result[selector].add(font_codes[0])
    return result


def validate_font_coverage() -> dict[str, object]:
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    if contract.get("schema_version") != 1:
        fail("不支持的字体字形合同版本")

    required_by_font: dict[str, set[str]] = defaultdict(set)
    fonts_by_label: dict[str, set[str]] = defaultdict(set)
    static_label_count = 0
    for root in authoring_roots():
        for node in walk_project_nodes(root):
            if node.get("saved_objtypeKey") != "LABEL":
                continue
            selector, text, font_code = label_fields(node)
            if not selector or not font_code:
                fail("SquareLine 标签缺少逻辑对象名或字体绑定")
            static_label_count += 1
            fonts_by_label[selector].add(font_code)
            required_by_font[font_code].update(text)
            if re.fullmatch(r"(?:ns|mo)[0-9]+_[0-9]+", font_code):
                required_by_font[font_code].update(ASCII_DYNAMIC)

    for selector, font_codes in projected_fonts_by_label().items():
        fonts_by_label[selector].update(font_codes)

    generated_labels = generated_export_labels()
    for font_code, text in generated_labels:
        required_by_font[font_code].update(text)
        if re.fullmatch(r"(?:ns|mo)[0-9]+_[0-9]+", font_code):
            required_by_font[font_code].update(ASCII_DYNAMIC)

    projection_labels = manifest_projection_labels()
    for font_code, text in projection_labels:
        required_by_font[font_code].update(text)
        if re.fullmatch(r"(?:ns|mo)[0-9]+_[0-9]+", font_code):
            required_by_font[font_code].update(ASCII_DYNAMIC)

    contract_texts: set[str] = set()
    for entry in contract.get("runtime_labels", []):
        selector = entry.get("selector", "")
        font_codes = fonts_by_label.get(selector, set())
        explicit_font_code = entry.get("font_code")
        explicit_font_codes = entry.get("font_codes")
        if explicit_font_code and explicit_font_codes:
            fail(f"运行时标签同时声明 font_code/font_codes: {selector}")
        if explicit_font_codes is not None:
            if (
                not isinstance(explicit_font_codes, list)
                or not explicit_font_codes
                or not all(
                    isinstance(font_code, str) and font_code
                    for font_code in explicit_font_codes
                )
            ):
                fail(f"运行时标签 font_codes 无效: {selector}")
            runtime_font_codes = set(explicit_font_codes)
        elif explicit_font_code:
            runtime_font_codes = {explicit_font_code}
        else:
            runtime_font_codes = font_codes
        if not runtime_font_codes:
            fail(f"运行时标签没有可追踪字体: {selector}")
        texts = entry.get("texts", [])
        if not texts or not all(isinstance(text, str) for text in texts):
            fail(f"运行时标签缺少有效 texts: {selector}")
        for text in texts:
            contract_texts.add(text)
            for font_code in runtime_font_codes:
                required_by_font[font_code].update(text)
        for font_code in runtime_font_codes:
            if re.fullmatch(r"(?:ns|mo)[0-9]+_[0-9]+", font_code):
                required_by_font[font_code].update(ASCII_DYNAMIC)

    for required in required_by_font.values():
        required.difference_update(
            character
            for character in tuple(required)
            if ord(character) < 0x20 or ord(character) == 0x7F
        )

    runtime_literals = runtime_non_ascii_literals(contract)
    registered_literals = {
        text for text in contract_texts if any(ord(character) >= 0x80 for character in text)
    }
    unregistered = sorted(runtime_literals - registered_literals)
    sourced_literals: set[str] = set()
    for entry in contract.get("runtime_labels", []):
        source_path = entry.get("source_path")
        if not source_path:
            continue
        source = (ROOT / source_path).read_text(encoding="utf-8")
        for value in entry.get("texts", []):
            if value not in source:
                fail(f"字形合同来源未包含登记文案: {source_path} -> {value}")
            if any(ord(character) >= 0x80 for character in value):
                sourced_literals.add(value)
    stale_contract = sorted(registered_literals - runtime_literals - sourced_literals)
    if unregistered:
        fail("运行时出现未登记的非 ASCII 文案: " + " | ".join(unregistered))
    if stale_contract:
        fail("字形合同含无运行时来源的文案: " + " | ".join(stale_contract))

    runtime_source_text = "\n".join(
        (ROOT / entry["path"]).read_text(encoding="utf-8")
        for entry in contract.get("runtime_sources", [])
    )
    for entry in contract.get("bounded_external_text", []):
        selector = entry.get("selector", "")
        font_codes = fonts_by_label.get(selector, set())
        if not font_codes:
            fail(f"外部文本边界标签没有可追踪字体: {selector}")
        enforcement = entry.get("enforced_by", "")
        if not enforcement or enforcement not in runtime_source_text:
            fail(f"外部文本字符边界未落实: {selector} -> {enforcement}")

    missing: list[str] = []
    for font_code, required in sorted(required_by_font.items()):
        stem = f"ui_font_{font_code}"
        asset_c = FONT_DIR / f"{stem}.c"
        asset_bin = FONT_DIR / f"{stem}.bin"
        config_path = FONT_DIR / f"{stem}.fcfg"
        firmware_c = GENERATED_FONT_DIR / f"{stem}.c"
        for path in (asset_c, asset_bin, config_path, firmware_c):
            if not path.is_file() or path.stat().st_size <= 64:
                missing.append(f"{font_code}: 缺少或无效资源 {path.relative_to(ROOT)}")
        if any(not path.is_file() for path in (asset_c, config_path, firmware_c)):
            continue
        if asset_c.read_bytes() != firmware_c.read_bytes():
            missing.append(f"{font_code}: SquareLine 与固件字体 C 源不一致")
        actual = glyph_codepoints(asset_c)
        absent = sorted(required - actual, key=ord)
        if absent:
            rendered = " ".join(f"{character}(U+{ord(character):04X})" for character in absent)
            missing.append(f"{font_code}: 缺少 {rendered}")
        config = json.loads(config_path.read_text(encoding="utf-8"))
        configured = set(config.get("symbols", ""))
        configured.update(ASCII_DYNAMIC if config.get("ranges") == ["0x20-0x7e"] else set())
        absent_from_config = sorted(required - configured, key=ord)
        if absent_from_config:
            rendered = " ".join(
                f"{character}(U+{ord(character):04X})" for character in absent_from_config
            )
            missing.append(f"{font_code}: .fcfg 缺少 {rendered}")

    if missing:
        fail("字体覆盖未闭合:\n  " + "\n  ".join(missing))

    return {
        "ok": True,
        "static_labels": static_label_count,
        "generated_labels": len(generated_labels),
        "projection_labels": len(projection_labels),
        "runtime_labels": len(contract.get("runtime_labels", [])),
        "runtime_non_ascii_literals": len(runtime_literals),
        "fonts": len(required_by_font),
        "required_font_glyph_pairs": sum(len(symbols) for symbols in required_by_font.values()),
        "missing_glyphs": 0,
        "squareline_firmware_font_sources_identical": True,
    }


def main() -> None:
    print(json.dumps(validate_font_coverage(), ensure_ascii=False))


if __name__ == "__main__":
    try:
        main()
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        print(f"ERROR: {error}")
        sys.exit(1)
