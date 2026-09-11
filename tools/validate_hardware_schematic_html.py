#!/usr/bin/env python3
"""离线硬件原理图 HTML 的静态一致性检查。"""

from __future__ import annotations

import json
import re
from html.parser import HTMLParser
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/hardware/电子木鱼-硬件网络清单.json"
HTML = ROOT / "docs/hardware/lceda/电子木鱼-完整原理图复刻.html"


class Checker(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.svg_pages: list[str] = []
        self.script_src = 0
        self.external_styles = 0
        self.text: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        if tag == "svg" and values.get("data-page"):
            self.svg_pages.append(values["data-page"] or "")
        if tag == "script" and values.get("src"):
            self.script_src += 1
        if tag == "link" and values.get("href"):
            self.external_styles += 1

    def handle_data(self, data: str) -> None:
        self.text.append(data)


def main() -> int:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    source = HTML.read_text(encoding="utf-8")
    parser = Checker()
    parser.feed(source)
    expected_pages = [p["id"] for p in manifest["schematic_pages"]]
    errors: list[str] = []
    if parser.svg_pages != expected_pages:
        errors.append(f"SVG 页序不一致: {parser.svg_pages!r} != {expected_pages!r}")
    if source.count("data-standard-page=") != 7:
        errors.append("HTML 必须包含 7 张标准符号原理图页")
    for symbol in ("R_PVDF_SER", "C_ADC", "L1", "L2", "D_CL+", "TLV2369", "TLV7042", "SPK_P", "R_SDA"):
        if symbol not in source:
            errors.append(f"HTML 缺少标准电路符号/关键位号: {symbol}")
    if source.count("<path") < 100:
        errors.append("标准电路图形路径数量不足，疑似退化为网络块图")
    if parser.script_src or parser.external_styles:
        errors.append("HTML 不应依赖外部 script/link 资源")
    for net in manifest["validation"]["required_nets"]:
        if net not in source:
            errors.append(f"HTML 缺少必需网络: {net}")
    for gpio in manifest["validation"]["required_gpio_signals"]:
        if gpio not in source:
            errors.append(f"HTML 缺少必需 GPIO signal: {gpio}")
    if not all(str(i) in source for i in range(1, 25)):
        errors.append("HTML 未闭合显示 FPC 1..24 针脚")
    forbidden = ["CH340X", "AMS1117", "TPS631000", "TYPE-C-31-M-12", "B2B-PH-K-S(LF)(SN)", "FH12-24S-0.5SH(55)"]
    for item in forbidden:
        if item in source:
            errors.append(f"HTML 出现禁止/过期器件口径: {item}")
    if not re.search(r"静态合同通过.*ERC/样机通过", source):
        errors.append("HTML 缺少非 ERC/样机验收声明")
    if errors:
        for error in errors:
            print("FAIL:", error)
        return 1
    print(f"PASS: HTML 自包含、7 页闭合、{len(manifest['validation']['required_nets'])} 个必需网络和 {len(manifest['validation']['required_gpio_signals'])} 个 GPIO 已出现")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
