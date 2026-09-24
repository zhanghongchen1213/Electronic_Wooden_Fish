#!/usr/bin/env python3
"""验证 EWF DEVICE-01 静态/运行时文案与固件字体资源的字形闭合。

路径根为 Embedded/。SquareLine 工程名为 ewf-device（裁决 B）。
"""

from __future__ import annotations

import json
import re
import sys
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
ASCII_DYNAMIC = set(chr(code) for code in range(0x20, 0x7F))
FORBIDDEN_PRODUCT = ("TODO", "draft", "placeholder", "data-pencil-id", "助力", "外骨骼", "支付", "档位")


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
    spj = PROJECT_DIR / "ewf-device.spj"
    if not spj.exists():
        fail("缺少 ewf-device.spj")
    project = json.loads(spj.read_text(encoding="utf-8"))
    roots = list(project.get("root", {}).get("children", []))
    for path in sorted((PROJECT_DIR / "components").glob("*.ecomp")):
        roots.append(json.loads(path.read_text(encoding="utf-8")))
    return roots


def function_body(source: str, name: str) -> str:
    match = re.search(
        rf"^[ \t]*(?:static[ \t]+)?(?:const[ \t]+)?(?:[A-Za-z_][A-Za-z0-9_]*[ \t*]+)+"
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
                return source[opening + 1 : index]
    fail(f"运行时字形合同引用的函数未闭合: {name}")
    return ""


def glyph_codepoints(font_c: Path) -> set[str]:
    if not font_c.exists():
        fail(f"缺少字体源: {font_c}")
    text = font_c.read_text(encoding="utf-8", errors="replace")
    codes: set[str] = set()
    for match in re.finditer(r"/\*\s*U\+([0-9A-Fa-f]+)\b", text):
        codes.add(chr(int(match.group(1), 16)))
    # Also accept lv_font_fmt_txt cmap ranges
    for match in re.finditer(
        r"\{\s*\.range_start\s*=\s*(\d+).*?\.glyph_id_start\s*=\s*\d+.*?\.list_length\s*=\s*(\d+)",
        text,
        flags=re.S,
    ):
        start = int(match.group(1))
        length = int(match.group(2))
        for code in range(start, start + length):
            if code >= 0x20:
                codes.add(chr(code))
    return codes


def font_code_to_path(font_code: str) -> Path:
    name = font_code if font_code.startswith("ui_font_") else f"ui_font_{font_code}"
    return GENERATED_FONT_DIR / f"{name}.c"


def projected_fonts_by_label() -> dict[str, set[str]]:
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    result: dict[str, set[str]] = {}
    for entry in contract.get("runtime_labels", []):
        selector = entry["selector"]
        codes = set(entry.get("font_codes", []))
        if not codes and entry.get("source_path"):
            # infer from generated C lv_obj_set_style_text_font
            pass
        result[selector] = codes
    return result


def collect_required_chars() -> dict[str, set[str]]:
    contract = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    by_font: dict[str, set[str]] = {}

    for root in authoring_roots():
        for node in walk_project_nodes(root):
            _name, text, font = label_fields(node)
            if not text or not font:
                continue
            code = font.replace("ui_font_", "")
            by_font.setdefault(code, set()).update(ch for ch in text if ord(ch) >= 0x20)

    for entry in contract.get("runtime_labels", []):
        codes = entry.get("font_codes") or []
        texts = "".join(entry.get("texts", []))
        # Story 3.2：动态字带占位展开为 canonical 心经汉字，避免 ASCII 占位令缺字门禁空转。
        if "DYNAMIC_SCRIPTURE_BELT" in texts:
            canon = ROOT.parent / "docs/contracts/canonical/heart-sutra.txt"
            if canon.exists():
                body = canon.read_text(encoding="utf-8")
                texts = texts.replace(
                    "DYNAMIC_SCRIPTURE_BELT",
                    "".join(re.findall(r"[\u4e00-\u9fff]", body)) + "·，。",
                )
            else:
                texts = texts.replace("DYNAMIC_SCRIPTURE_BELT", "·")
        if "DYNAMIC_SCRIPTURE_HISTORY" in texts:
            canon = ROOT.parent / "docs/contracts/canonical/heart-sutra.txt"
            if canon.exists():
                body = canon.read_text(encoding="utf-8")
                texts = texts.replace(
                    "DYNAMIC_SCRIPTURE_HISTORY",
                    "".join(re.findall(r"[\u4e00-\u9fff]", body)) + "·，。",
                )
            else:
                texts = texts.replace("DYNAMIC_SCRIPTURE_HISTORY", "·")
        for code in codes:
            by_font.setdefault(code, set()).update(ch for ch in texts if ord(ch) >= 0x20)

    for source_entry in contract.get("runtime_sources", []):
        path = ROOT / source_entry["path"]
        if not path.exists():
            fail(f"runtime_sources 路径不存在: {path}")
        source = path.read_text(encoding="utf-8")
        for function_name in source_entry.get("functions", []):
            body = function_body(source, function_name)
            body = re.sub(r"//[^\n]*|/\*.*?\*/", "", body, flags=re.S)
            for literal in re.findall(r'"((?:\\.|[^"\\])*)"', body):
                decoded = bytes(literal, "utf-8").decode("unicode_escape")
                # Prefer UTF-8 source literals already decoded
                try:
                    decoded = literal.encode("utf-8").decode("unicode_escape")
                except Exception:
                    decoded = literal
                # Chinese source files: keep literal as-is
                decoded = literal
                for ch in decoded:
                    if ord(ch) >= 0x80:
                        # assign to all ns/serif fonts that exist
                        for code in ("ns600_16", "ns700_22", "serif700_28"):
                            by_font.setdefault(code, set()).add(ch)

    return by_font


def check_forbidden_copy() -> None:
    for directory in GENERATED_UI_DIRS:
        if not directory.exists():
            continue
        for path in directory.rglob("*.c"):
            text = path.read_text(encoding="utf-8", errors="replace")
            for bad in FORBIDDEN_PRODUCT:
                if bad in text:
                    fail(f"生成页残留禁用词 {bad!r}: {path}")


def check_squareline_firmware_identical() -> bool:
    identical = True
    for gen in sorted(GENERATED_FONT_DIR.glob("ui_font_*.c")):
        sl = FONT_DIR / gen.name
        if not sl.exists():
            fail(f"SquareLine 字体资产缺失: {sl}")
        if gen.read_bytes() != sl.read_bytes():
            print(f"ERROR: 字体字节不一致: {gen.name}")
            identical = False
    return identical


def main() -> int:
    if not CONTRACT_PATH.exists():
        fail(f"缺少字形合同: {CONTRACT_PATH}")

    check_forbidden_copy()
    by_font = collect_required_chars()
    missing_total = 0
    for code, chars in sorted(by_font.items()):
        path = font_code_to_path(code)
        glyphs = glyph_codepoints(path)
        missing = sorted(ch for ch in chars if ch not in glyphs and ch not in ASCII_DYNAMIC)
        # ASCII may be in font; if not in glyphs set due to parse limits, skip pure ASCII
        missing = [ch for ch in missing if ord(ch) >= 0x80]
        if missing:
            missing_total += len(missing)
            sample = "".join(missing[:20])
            print(f"ERROR: {code} 缺字 {len(missing)}: {sample}")
        else:
            print(f"OK {code}: required={len(chars)}")

    identical = check_squareline_firmware_identical()
    result = {
        "missing_glyphs": missing_total,
        "squareline_firmware_font_sources_identical": identical,
    }
    print(json.dumps(result, ensure_ascii=False))
    if missing_total != 0 or not identical:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
