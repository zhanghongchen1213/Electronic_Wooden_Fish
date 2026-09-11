#!/usr/bin/env python3
"""检查 EWF 交付页面是否泄漏内部节点、作者说明或错误展示文案。"""

from __future__ import annotations

import argparse
import json
import re
import sys
from html.parser import HTMLParser
from pathlib import Path


FORBIDDEN = re.compile(
    r"AI|思维链|中间思考|内部推理|推理过程|调试说明|调试|开发说明|Node\s*ID|data-pencil-id|TODO|draft|placeholder|设计说明|作者提示|视觉样例|qljP7|WfAs7|Z6Qge|e8Sgp|gGgAm|NtM6r",
    re.I,
)
FORBIDDEN_DEVICE = re.compile(r"(?:device_touch|木鱼)", re.I)
STANDALONE_MARK = re.compile(r"^(?:[。．._＿]+|[-‐‑—]+)$")
PROGRESS_TEXT = re.compile(r"心经进度|scripture\s*progress", re.I)


class VisibleTextParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.visible: list[str] = []
        self.attrs: list[tuple[str, dict[str, str]]] = []
        self._hidden = 0

    def handle_starttag(self, tag: str, attrs_list: list[tuple[str, str | None]]) -> None:
        attrs = {key: value or "" for key, value in attrs_list}
        self.attrs.append((tag, attrs))
        if tag.lower() in {"script", "style"}:
            self._hidden += 1

    def handle_endtag(self, tag: str) -> None:
        if tag.lower() in {"script", "style", "template"}:
            self._hidden = max(0, self._hidden - 1)

    def handle_data(self, data: str) -> None:
        value = " ".join(data.split())
        if value and not self._hidden:
            self.visible.append(value)


def validate(path: Path, surface: str, mode: str) -> dict[str, object]:
    source = path.read_text(encoding="utf-8")
    parser = VisibleTextParser()
    if path.suffix.lower() in {".html", ".htm", ".vue"}:
        parser.feed(source)
    else:
        parser.visible = re.findall(r"[\u4e00-\u9fffA-Za-z0-9%·/]+", source)

    errors: list[dict[str, object]] = []
    for value in parser.visible:
        if FORBIDDEN.search(value):
            errors.append({"code": "forbidden_visible_text", "value": value})
        if STANDALONE_MARK.match(value):
            errors.append({"code": "standalone_punctuation", "value": value})
        if surface == "miniapp" and FORBIDDEN_DEVICE.search(value):
            errors.append({"code": "miniapp_forbidden_device_input", "value": value})

    if "data-pencil-id" in source:
        errors.append({"code": "node_id_attribute"})
    if mode == "production" and re.search(r"data-pencil-name=", source, re.I):
        errors.append({"code": "design_annotation_attribute"})
    if re.search(r"(?:src|href)\s*=\s*[\"']https?://|<script[^>]+src=|@import\s+url\(", source, re.I):
        errors.append({"code": "external_resource"})
    progress_count = len(PROGRESS_TEXT.findall("\n".join(parser.visible)))
    frame_count = max(1, source.count('data-ewf-frame="true"'))
    if progress_count > frame_count:
        errors.append({"code": "duplicate_progress_text", "count": progress_count, "frames": frame_count})
    legacy_tokens = re.compile(
        r"(?<!\[VAR:)(?<![A-Za-z0-9_-])(?:line-[123]|prefix-note|focus-rule)(?![A-Za-z0-9_-])|\bWfAs7\b|\bZ6Qge\b",
        re.I,
    )
    if legacy_tokens.search(source):
        errors.append({"code": "legacy_demo_structure"})

    return {"path": str(path), "surface": surface, "mode": mode, "visible_text_count": len(parser.visible), "errors": errors, "ok": not errors}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--surface", choices=("device", "miniapp"), required=True)
    parser.add_argument("--html", type=Path, required=True)
    parser.add_argument("--mode", choices=("candidates", "production"), default="candidates")
    args = parser.parse_args()
    result = validate(args.html, args.surface, args.mode)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
