#!/usr/bin/env python3
"""校验电子木鱼 pen/HTML 设计稿的页面、状态与组件闭包。

校验器不读取 .pen 加密文件，只读取 Pencil 导出的静态 HTML 与 UI_CONTRACT。
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from html.parser import HTMLParser
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
UX_DIR = ROOT / "_bmad-output/planning-artifacts/ux-designs/ux-Electronic_Wooden_Fish-2026-09-08"
DEVICE_CONTRACT = UX_DIR / "UI_CONTRACT-device.md"
MINI_CONTRACT = UX_DIR / "UI_CONTRACT-miniapp.md"
DEVICE_DIRECTION = ROOT / "lvgl-design/ewf-device-direction.html"
MINI_DIRECTION = ROOT / "miniapp-design/ewf-miniapp-direction.html"
DEVICE_FULL = ROOT / "lvgl-design/ewf-device-ui.html"
MINI_FULL = ROOT / "miniapp-design/ewf-miniapp-ui.html"
DEVICE_MANIFEST = ROOT / "lvgl-design/device-style-options.json"
MINI_MANIFEST = ROOT / "miniapp-design/miniapp-style-options.json"

NAME_RE = re.compile(
    r"^\[UI\](?:\[STYLE:(?P<style>[A-Z0-9-]+)\])?\[PAGE:(?P<page>[A-Z_]+)\]\[ST:(?P<state>[A-Z0-9_]+)\]"
    r"(?:\[CMP:(?P<cmp>[a-z0-9-]+)\])?(?:\[VAR:(?P<var>[A-Za-z0-9_-]+)\])?$"
)
COLOR_RE = re.compile(r"#[0-9a-fA-F]{6,8}")


def expand_contract(path: Path) -> set[str]:
    """读取契约状态代码块，并展开 / 分隔的状态名。"""
    expected: set[str] = set()
    in_block = False
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line == "```":
            in_block = not in_block
            continue
        if not in_block:
            continue
        match = re.match(r"^\[([A-Z_]+)\]\[([A-Z0-9_]+(?:/[A-Z0-9_]+)*)\]", line)
        if not match:
            continue
        page, states = match.groups()
        if page == "SHELL":
            continue
        for state in states.split("/"):
            expected.add(f"{page}.{state}")
    return expected


def frame_key(name: str) -> str | None:
    match = NAME_RE.match(name)
    if not match:
        return None
    prefix = f"{match.group('style')}:" if match.group("style") else ""
    return f"{prefix}{match.group('page')}.{match.group('state')}"


def component_name(name: str) -> str | None:
    match = NAME_RE.match(name)
    return match.group("cmp") if match else None


def css_dimension(attrs: dict[str, str], axis: str) -> int | None:
    style = attrs.get("style", "")
    style_match = re.search(rf"(?:^|;)\s*{axis}\s*:\s*(\d+)px", style)
    if style_match:
        return int(style_match.group(1))
    classes = attrs.get("class", "")
    class_match = re.search(rf"(?:^|\s){'w' if axis == 'width' else 'h'}-\[(\d+)px\]", classes)
    return int(class_match.group(1)) if class_match else None


class DesignParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.names: list[tuple[str, dict[str, str], int]] = []
        self.frames: dict[str, dict[str, object]] = {}
        self._depth = 0
        self._frame_stack: list[str] = []

    def handle_starttag(self, tag: str, attrs_list: list[tuple[str, str | None]]) -> None:
        attrs = {key: value or "" for key, value in attrs_list}
        name = attrs.get("data-pencil-name")
        if name:
            self.names.append((name, attrs, self._depth))
            key = frame_key(name)
            is_frame = attrs.get("data-ewf-frame", "").lower() == "true" or (
                key is not None and "[CMP:" not in name and "[VAR:" not in name
            )
            if is_frame and key:
                self.frames.setdefault(
                    key,
                    {
                        "name": name,
                        "attrs": attrs,
                        "components": set(),
                        "component_attrs": {},
                        "component_counts": {},
                        "names": [],
                        "nodes": [],
                        "depth": self._depth,
                    },
                )
                self._frame_stack.append(key)
            elif self._frame_stack:
                current = self.frames[self._frame_stack[-1]]
                current["names"].append(name)
                current["nodes"].append((name, attrs))
                cmp = component_name(name)
                if cmp:
                    current["components"].add(cmp)
                    # Count only the instance/master root. Descendant layers keep
                    # the same CMP namespace for mapping, but are not additional
                    # component instances.
                    if "[VAR:master]" in name:
                        current["component_counts"][cmp] = int(current["component_counts"].get(cmp, 0)) + 1
                    current["component_attrs"].setdefault(cmp, []).append(attrs)
        self._depth += 1

    def handle_endtag(self, tag: str) -> None:
        self._depth = max(0, self._depth - 1)
        while self._frame_stack:
            key = self._frame_stack[-1]
            frame_depth = int(self.frames[key]["depth"])
            if self._depth <= frame_depth:
                self._frame_stack.pop()
            else:
                break


def required_components(surface: str, key: str) -> set[str]:
    page = key.split(":", 1)[-1].split(".", 1)[0]
    if surface == "device":
        return {
            "MUYU": {"statusbar", "charcell", "scripture-progress", "today-taps", "total-taps", "woodfish", "woodfish-anatomy"},
            "JINGWEN": {"statusbar", "glyph-current", "scripture-progress"},
            "TONGJI": {"statusbar", "today-taps", "total-taps"},
            "SHEZHI": {"statusbar", "row", "slider", "sync-btn"},
            "OVERLAY": {"modal-done"},
        }.get(page, set())
    return {
        "READING": {"readingline", "char-focus", "scripture-progress", "bottom-nav"},
        "RECORDS": {"statcard", "bottom-nav"},
        "DEVICE": {"devstatus-row", "state-banner", "sync-action", "bottom-nav"},
        "SETTINGS": {"setting-mirror", "bottom-nav"},
        "OVERLAY": {"modal-done", "confetti"},
    }.get(page, set())


def contrast_ratio(foreground: str, background: str) -> float:
    def channel(value: int) -> float:
        value /= 255
        return value / 12.92 if value <= 0.03928 else ((value + 0.055) / 1.055) ** 2.4

    def luminance(color: str) -> float:
        raw = color.lstrip("#")[:6]
        rgb = [int(raw[index : index + 2], 16) for index in (0, 2, 4)]
        return 0.2126 * channel(rgb[0]) + 0.7152 * channel(rgb[1]) + 0.0722 * channel(rgb[2])

    light, dark = sorted((luminance(foreground), luminance(background)), reverse=True)
    return (light + 0.05) / (dark + 0.05)


def structural_fingerprint(frame: dict[str, object]) -> str:
    """提取去除 style/variant/color 后的结构指纹，仅用于发现近重复候选。"""
    parts: list[str] = []
    for name, attrs in frame.get("nodes", []):
        style = attrs.get("style", "")
        style = re.sub(r"(?:background(?:-color)?|color|fill|stroke|box-shadow)\s*:[^;]+;?", "", style, flags=re.I)
        style = re.sub(r"#[0-9a-f]{3,8}", "", style, flags=re.I)
        style = re.sub(r"\s+", " ", style).strip()
        if "[CMP:" in name:
            semantic = re.sub(r"\[STYLE:[A-Z0-9-]+\]", "", name)
            semantic = re.sub(r"\[VAR:[^\]]+\]", "", semantic)
            parts.append(f"{semantic}|{style}|class={attrs.get('class','')}")
    return "||".join(parts)


def validate(
    surface: str,
    html_path: Path,
    contract_path: Path,
    stage: str,
    style_id: str | None = None,
    manifest_path: Path | None = None,
) -> dict[str, object]:
    errors: list[dict[str, object]] = []
    warnings: list[dict[str, object]] = []
    if not html_path.exists():
        return {"surface": surface, "stage": stage, "html": str(html_path), "errors": [{"code": "missing_html"}], "ok": False}

    source = html_path.read_text(encoding="utf-8")
    parser = DesignParser()
    parser.feed(source)
    found = set(parser.frames)
    style_ids = sorted({match.group("style") for frame in parser.frames.values() if (match := NAME_RE.match(str(frame["name"]))) and match.group("style")})
    expected_full = expand_contract(contract_path)
    if stage == "candidates":
        expected = set()
        expected_style_ids = (
            {f"DEVICE-{index:02d}" for index in range(1, 11)}
            if surface == "device"
            else {f"MINI-{index:02d}" for index in range(1, 11)}
        )
        missing = []
        unexpected = []
        if len(found) != 10:
            errors.append({"code": "candidate_count", "expected": 10, "actual": len(found)})
        if set(style_ids) != expected_style_ids:
            errors.append({"code": "candidate_style_ids", "expected": sorted(expected_style_ids), "actual": style_ids})
        if manifest_path:
            if not manifest_path.exists():
                errors.append({"code": "missing_style_manifest", "path": str(manifest_path)})
            else:
                manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
                manifest_styles = manifest.get("styles", [])
                manifest_ids = {str(item.get("id")) for item in manifest_styles}
                if len(manifest_styles) != 10 or manifest_ids != expected_style_ids:
                    errors.append(
                        {
                            "code": "style_manifest_ids",
                            "expected": sorted(expected_style_ids),
                            "actual": sorted(manifest_ids),
                        }
                    )
                advisory_axes = []
                for item in manifest_styles:
                    axes = {str(axis).lower() for axis in item.get("axes", []) if str(axis).lower() != "color"}
                    if len(axes) < 3:
                        advisory_axes.append({"style": item.get("id"), "axes": sorted(axes)})
                if advisory_axes:
                    warnings.append({"code": "style_axes_advisory", "items": advisory_axes})
        fingerprints: dict[str, str] = {}
        for key, frame in parser.frames.items():
            match = NAME_RE.match(str(frame["name"]))
            if not match or not match.group("style"):
                continue
            fingerprint = structural_fingerprint(frame)
            if fingerprint in fingerprints:
                errors.append({"code": "style_structure_duplicate", "styles": [fingerprints[fingerprint], match.group("style")]})
            else:
                fingerprints[fingerprint] = match.group("style")
        for style_id in expected_style_ids & set(style_ids):
            style_frames = [frame for key, frame in parser.frames.items() if key.startswith(style_id + ":")]
            signatures = {
                match.group("var")
                for frame in style_frames
                for name in frame["names"]
                if (match := NAME_RE.match(name)) and match.group("cmp") == "style-signature" and match.group("var")
            }
            if not signatures:
                errors.append({"code": "missing_style_signature", "style": style_id})
    else:
        expected = {"MUYU.BASE"} if surface == "device" and stage == "direction" else expected_full
        if surface == "miniapp" and stage == "direction":
            expected = {"READING.LIVE"}
        if style_id:
            expected = {f"{style_id}:{item}" for item in expected}
        missing = sorted(expected - found)
        unexpected = sorted(found - expected)
    if missing:
        errors.append({"code": "missing_frames", "items": missing})
    if unexpected:
        errors.append({"code": "unexpected_frames", "items": unexpected})

    names = [item[0] for item in parser.names]
    duplicates = sorted({name for name in names if "[STYLE:" in name and names.count(name) > 1})
    if duplicates:
        errors.append({"code": "duplicate_names", "items": duplicates})
    invalid_names = sorted(name for name in names if not NAME_RE.match(name))
    if invalid_names:
        errors.append({"code": "invalid_pencil_names", "items": invalid_names[:20], "count": len(invalid_names)})

    if re.search(r"(?:src|href)\s*=\s*[\"']https?://|url\(\s*[\"']?https?://|<script[^>]+src=", source, re.I):
        errors.append({"code": "external_resource"})
    if re.search(r"url\(\s*[\"']?(?:\.\.?/)?assets/|(?:src|href)\s*=\s*[\"'](?:\.\.?/)?assets/", source, re.I):
        errors.append({"code": "non_embedded_asset"})
    if re.search(r"font-family\s*:[^;}]*(?:Inter|Roboto|Arial|system-ui)", source, re.I):
        errors.append({"code": "uncontrolled_font"})
    if "data-pencil-id" in source:
        errors.append({"code": "node_id_leak"})

    for key, frame in parser.frames.items():
        attrs = frame["attrs"]
        page = key.split(":", 1)[-1].split(".", 1)[0]
        width = css_dimension(attrs, "width")
        height = css_dimension(attrs, "height")
        expected_size = (410, 502) if surface == "device" else (390, 844)
        if (width, height) != expected_size:
            errors.append({"code": "frame_size", "frame": key, "expected": expected_size, "actual": (width, height)})
        root_style = attrs.get("style", "")
        if surface == "device" and (
            not re.search(r"border-radius\s*:\s*110px", root_style, re.I)
            or not re.search(r"overflow\s*:\s*hidden", root_style, re.I)
        ):
            errors.append({"code": "device_arc_frame", "frame": key, "expected": "border-radius:110px; overflow:hidden"})
        components = set(frame["components"])
        required = required_components(surface, key)
        absent = sorted(required - components)
        if absent:
            errors.append({"code": "missing_components", "frame": key, "items": absent})

        component_attrs = frame["component_attrs"]
        component_counts = frame["component_counts"]
        if surface == "device":
            statusbars = component_attrs.get("statusbar", [])
            if not any(
                css_dimension(item, "width") == 314
                and css_dimension(item, "height") == 24
                and re.search(r"left\s*:\s*48px", item.get("style", ""))
                and re.search(r"top\s*:\s*20px", item.get("style", ""))
                for item in statusbars
            ):
                errors.append({"code": "statusbar_geometry", "frame": key, "expected": "x48 y20 314x24"})
        if surface == "miniapp" and page == "READING":
            if component_counts.get("scripture-progress", 0) != 1:
                errors.append({"code": "single_scripture_progress", "frame": key, "actual": component_counts.get("scripture-progress", 0)})
            navs = component_attrs.get("bottom-nav", [])
            if not any(
                css_dimension(item, "width") == 350
                and css_dimension(item, "height") == 56
                and re.search(r"left\s*:\s*20px", item.get("style", ""))
                and re.search(r"top\s*:\s*768px", item.get("style", ""))
                for item in navs
            ):
                errors.append({"code": "bottom_nav_geometry", "frame": key, "expected": "x20 y768 350x56"})

        if surface == "device" and "woodfish-anatomy" in components:
            for name, node_attrs, _depth in parser.names:
                if component_name(name) == "woodfish-anatomy" and "[VAR:idle]" in name:
                    size = (css_dimension(node_attrs, "width"), css_dimension(node_attrs, "height"))
                    if not size[0] or not size[1] or size[0] < 96 or size[1] < 96:
                        errors.append({"code": "touch_target", "frame": key, "actual": size})
            for node_attrs in component_attrs.get("woodfish", []):
                if "[VAR:tap-zone]" not in node_attrs.get("data-pencil-name", ""):
                    continue
                size = (css_dimension(node_attrs, "width"), css_dimension(node_attrs, "height"))
                if not size[0] or not size[1] or size[0] < 96 or size[1] < 96:
                    errors.append({"code": "touch_target", "frame": key, "actual": size})

    if surface == "miniapp" and re.search(r"woodfish|device_touch|电子木鱼", source, re.I):
        errors.append({"code": "miniapp_forbidden_woodfish"})
    if re.search(r'data-scripture-role\s*=\s*["\']future["\']', source, re.I):
        errors.append({"code": "future_scripture_preview"})

    ratios = {
        "device": contrast_ratio("#f5efe2", "#17130f"),
        "miniapp": contrast_ratio("#2a2620", "#faf7f2"),
    }
    minimum = 7.0 if surface == "device" else 4.5
    if ratios[surface] < minimum:
        errors.append({"code": "contrast", "ratio": round(ratios[surface], 3), "minimum": minimum})

    return {
        "surface": surface,
        "stage": stage,
        "html": str(html_path),
        "expected_frames": 10 if stage == "candidates" else len(expected),
        "found_frames": len(found),
        "frames": sorted(found),
        "contrast_ratio": round(ratios[surface], 3),
        "errors": errors,
        "warnings": warnings,
        "ok": not errors,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--surface", choices=("device", "miniapp"), required=True)
    parser.add_argument("--stage", choices=("direction", "candidates", "full"), default="full")
    parser.add_argument("--style", help="作者选中的 style id，用于 full 阶段按单一 style 校验")
    parser.add_argument("--html", type=Path)
    parser.add_argument("--contract", type=Path)
    parser.add_argument("--manifest", type=Path, help="候选 style manifest；每个候选至少声明 3 个非颜色变化轴")
    args = parser.parse_args(argv)

    defaults = {
        "device": (DEVICE_DIRECTION if args.stage == "direction" else DEVICE_FULL, DEVICE_CONTRACT),
        "miniapp": (MINI_DIRECTION if args.stage == "direction" else MINI_FULL, MINI_CONTRACT),
    }
    html_path, contract_path = defaults[args.surface]
    manifest_path = args.manifest
    if args.stage == "candidates" and manifest_path is None:
        manifest_path = DEVICE_MANIFEST if args.surface == "device" else MINI_MANIFEST
    result = validate(args.surface, args.html or html_path, args.contract or contract_path, args.stage, args.style, manifest_path)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
