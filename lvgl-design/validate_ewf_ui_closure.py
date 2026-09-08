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

NAME_RE = re.compile(
    r"^\[UI\]\[PAGE:(?P<page>[A-Z_]+)\]\[ST:(?P<state>[A-Z0-9_]+)\]"
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
    return f"{match.group('page')}.{match.group('state')}"


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
                        "names": [],
                        "depth": self._depth,
                    },
                )
                self._frame_stack.append(key)
            elif self._frame_stack:
                current = self.frames[self._frame_stack[-1]]
                current["names"].append(name)
                cmp = component_name(name)
                if cmp:
                    current["components"].add(cmp)
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
    page = key.split(".", 1)[0]
    if surface == "device":
        return {
            "MUYU": {"statusbar", "charcell", "progress-pill", "today-count", "woodfish"},
            "JINGWEN": {"statusbar", "glyph-current", "progress-pill"},
            "TONGJI": {"statusbar", "today-count"},
            "SHEZHI": {"statusbar", "row", "slider", "sync-btn"},
            "OVERLAY": {"modal-done"},
        }.get(page, set())
    return {
        "READING": {"readingline", "char-focus", "bottom-nav"},
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


def validate(surface: str, html_path: Path, contract_path: Path, stage: str) -> dict[str, object]:
    errors: list[dict[str, object]] = []
    if not html_path.exists():
        return {"surface": surface, "stage": stage, "html": str(html_path), "errors": [{"code": "missing_html"}], "ok": False}

    source = html_path.read_text(encoding="utf-8")
    parser = DesignParser()
    parser.feed(source)
    expected_full = expand_contract(contract_path)
    expected = {"MUYU.BASE"} if surface == "device" and stage == "direction" else expected_full
    if surface == "miniapp" and stage == "direction":
        expected = {"READING.LIVE"}

    found = set(parser.frames)
    missing = sorted(expected - found)
    unexpected = sorted(found - expected)
    if missing:
        errors.append({"code": "missing_frames", "items": missing})
    if unexpected:
        errors.append({"code": "unexpected_frames", "items": unexpected})

    names = [item[0] for item in parser.names]
    duplicates = sorted({name for name in names if names.count(name) > 1})
    if duplicates:
        errors.append({"code": "duplicate_names", "items": duplicates})
    invalid_names = sorted(name for name in names if not NAME_RE.match(name))
    if invalid_names:
        errors.append({"code": "invalid_pencil_names", "items": invalid_names[:20], "count": len(invalid_names)})

    if re.search(r"(?:src|href)\s*=\s*[\"']https?://|url\(\s*[\"']?https?://|<script[^>]+src=", source, re.I):
        errors.append({"code": "external_resource"})

    for key, frame in parser.frames.items():
        attrs = frame["attrs"]
        width = css_dimension(attrs, "width")
        height = css_dimension(attrs, "height")
        expected_size = (410, 502) if surface == "device" else (390, 844)
        if (width, height) != expected_size:
            errors.append({"code": "frame_size", "frame": key, "expected": expected_size, "actual": (width, height)})
        components = set(frame["components"])
        required = required_components(surface, key)
        absent = sorted(required - components)
        if absent:
            errors.append({"code": "missing_components", "frame": key, "items": absent})

        if surface == "device" and "woodfish" in components:
            for name, node_attrs, _depth in parser.names:
                if component_name(name) == "woodfish" and "[VAR:idle]" in name:
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
        "expected_frames": len(expected),
        "found_frames": len(found),
        "frames": sorted(found),
        "contrast_ratio": round(ratios[surface], 3),
        "errors": errors,
        "ok": not errors,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--surface", choices=("device", "miniapp"), required=True)
    parser.add_argument("--stage", choices=("direction", "full"), default="full")
    parser.add_argument("--html", type=Path)
    parser.add_argument("--contract", type=Path)
    args = parser.parse_args(argv)

    defaults = {
        "device": (DEVICE_DIRECTION if args.stage == "direction" else DEVICE_FULL, DEVICE_CONTRACT),
        "miniapp": (MINI_DIRECTION if args.stage == "direction" else MINI_FULL, MINI_CONTRACT),
    }
    html_path, contract_path = defaults[args.surface]
    result = validate(args.surface, args.html or html_path, args.contract or contract_path, args.stage)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
