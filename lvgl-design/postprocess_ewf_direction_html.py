#!/usr/bin/env python3
"""把 Pencil 导出的 EWF 方向稿收敛为单帧、离线、自包含 HTML。"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


def extract_div(source: str, marker: str) -> str:
    marker_start = source.find(marker)
    if marker_start < 0:
        raise ValueError(f"找不到目标帧: {marker}")
    start = source.rfind("<div", 0, marker_start)
    if start < 0:
        raise ValueError("目标帧没有 div 根节点")
    tokens = re.compile(r"</?div\b[^>]*>", re.I)
    depth = 0
    for match in tokens.finditer(source, start):
        depth += -1 if match.group(0).startswith("</") else 1
        if depth == 0:
            return source[start : match.end()]
    raise ValueError("目标帧 div 未闭合")


def normalize_frame(frame: str, surface: str) -> str:
    dimensions = ("410", "502") if surface == "device" else ("390", "844")
    opening_end = frame.find(">")
    if opening_end < 0:
        raise ValueError("目标帧缺少开始标签")
    opening = frame[:opening_end]
    opening = re.sub(r"\sdata-ewf-frame=\"[^\"]*\"", "", opening)
    opening = opening.replace(">", "")
    opening = re.sub(r"\sleft:\s*[-\d.]+px", " left: 0px", opening, count=1)
    opening = re.sub(r"\stop:\s*[-\d.]+px", " top: 0px", opening, count=1)
    opening = opening.replace("data-pencil-id=", 'data-ewf-frame="true" data-sample="true" data-pencil-id=', 1)
    opening = opening + ">"
    return opening + frame[opening_end + 1 :]


def build(source: str, surface: str, frame_name: str) -> str:
    frame = normalize_frame(extract_div(source, f'data-pencil-name="{frame_name}"'), surface)
    width, height = ("410", "502") if surface == "device" else ("390", "844")
    background = "#17130f" if surface == "device" else "#faf7f2"
    styles = "\n".join(re.findall(r"<style\b[^>]*>(.*?)</style>", source, re.I | re.S))
    styles += "\n*{box-sizing:border-box}body{margin:0;background:" + background + ";font-family:\"Noto Sans SC\",system-ui,sans-serif}.ewf-export{position:relative;width:" + width + "px;height:" + height + "px;overflow:hidden}"
    return (
        '<!doctype html>\n'
        '<html lang="zh-CN">\n'
        '<head>\n'
        '  <meta charset="UTF-8">\n'
        f'  <meta name="ewf-surface" content="{surface}">\n'
        '  <meta name="ewf-scripture-sample" content="non-authoritative">\n'
        '  <style>\n'
        + styles
        + '\n  </style>\n'
        '</head>\n'
        '<body>\n'
        f'  <main class="ewf-export" data-ewf-surface="{surface}">\n'
        + frame
        + '\n  </main>\n'
        '</body>\n'
        '</html>\n'
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--surface", choices=("device", "miniapp"), required=True)
    parser.add_argument("--frame-name", required=True)
    args = parser.parse_args()
    output = build(args.input.read_text(encoding="utf-8"), args.surface, args.frame_name)
    args.output.write_text(output, encoding="utf-8")
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
