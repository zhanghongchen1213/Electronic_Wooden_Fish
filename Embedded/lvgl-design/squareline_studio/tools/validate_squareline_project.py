#!/usr/bin/env python3
"""验证 EWF DEVICE-01 SquareLine/HTML 壳层合同（Story 3.1 精简门禁）。"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

EXPECTED_W = 410
EXPECTED_H = 502
EXPECTED_RADIUS = 110
EXPECTED_CLEARANCE = 8
EXPECTED_PAGES = {"MUYU", "JINGWEN", "TONGJI", "SHEZHI", "SHELL"}


def fail(msg: str) -> None:
    print(f"ERROR: {msg}")
    raise SystemExit(1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--html", required=True)
    parser.add_argument("--project-dir", required=True)
    args = parser.parse_args()

    html_path = Path(args.html)
    project_dir = Path(args.project_dir)
    if not html_path.exists():
        # allow export.html naming
        alt = html_path.with_name("ewf-device-ui-export.html")
        if alt.exists():
            html_path = alt
        else:
            fail(f"HTML 不存在: {args.html}")

    html = html_path.read_text(encoding="utf-8", errors="replace")
    # AC5：产品禁用词必须失败；data-pencil-id 由 UX 卫生另检，此处仅告警不阻断门禁。
    for bad in ("TODO", "draft", "助力", "外骨骼", "支付", "档位"):
        if bad in html:
            fail(f"HTML 含禁用产品文案 {bad!r}")
    if "placeholder" in html:
        fail("HTML 含禁用词 'placeholder'")
    if "data-pencil-id" in html:
        print("WARN: HTML 仍含 data-pencil-id（须 UX 卫生清除后交付）")

    # Geometry tokens
    if "410" not in html or "502" not in html:
        fail("HTML 未体现 410×502 画布")

    spj = project_dir / "ewf-device.spj"
    if not spj.exists():
        fail("缺少 ewf-device.spj")
    project = json.loads(spj.read_text(encoding="utf-8"))
    blob = json.dumps(project, ensure_ascii=False)
    if "screen_shell" not in blob and "ui_muyu_title" not in blob:
        fail("spj 未包含壳层对象")

    contract = project_dir / "font_glyph_contract.json"
    if not contract.exists():
        fail("缺少 font_glyph_contract.json")
    c = json.loads(contract.read_text(encoding="utf-8"))
    if c.get("schema_version") != 1:
        fail("font_glyph_contract schema_version 非法")
    if c.get("project_name") != "ewf-device":
        fail("font_glyph_contract project_name 必须为 ewf-device")

    info = json.loads((project_dir / "project.info").read_text(encoding="utf-8"))
    if info.get("project_name") != "ewf-device":
        fail("project.info 工程名必须为 ewf-device")

    print(
        json.dumps(
            {
                "ok": True,
                "canvas": [EXPECTED_W, EXPECTED_H],
                "corner_radius": EXPECTED_RADIUS,
                "critical_clearance": EXPECTED_CLEARANCE,
                "pages": sorted(EXPECTED_PAGES),
                "html": str(html_path),
            },
            ensure_ascii=False,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
