#!/usr/bin/env python3
"""把 Pencil 导出的 EWF 方向稿收敛为单帧、离线、自包含 HTML。"""

from __future__ import annotations

import argparse
import base64
import re
from pathlib import Path


def inline_local_assets(value: str) -> str:
    asset = Path(__file__).resolve().parent / "assets" / "woodfish-reference.png"
    if not asset.exists():
        return value
    encoded = base64.b64encode(asset.read_bytes()).decode("ascii")
    data_url = f"data:image/png;base64,{encoded}"
    for needle in ("url('assets/woodfish-reference.png')", 'url("assets/woodfish-reference.png")', "url(assets/woodfish-reference.png)"):
        value = value.replace(needle, f"url({data_url})")
    return value


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
    opening_end = frame.find(">")
    if opening_end < 0:
        raise ValueError("目标帧缺少开始标签")
    opening = frame[: opening_end + 1]
    root_name_match = re.search(r'data-pencil-name="([^"]+)"', opening)
    root_name = root_name_match.group(1) if root_name_match else "[UI][PAGE:UNKNOWN][ST:BASE]"
    root_prefix = root_name.split("[CMP:", 1)[0].split("[VAR:", 1)[0]
    opening = re.sub(r'\sdata-ewf-frame="[^"]*"', "", opening)
    opening = re.sub(r'\sdata-style-id="[^"]*"', "", opening)
    opening = re.sub(r"\sleft:\s*[-\d.]+px", " left: 0px", opening, count=1)
    opening = re.sub(r"\stop:\s*[-\d.]+px", " top: 0px", opening, count=1)
    style_match = re.search(r"\[STYLE:([A-Z0-9-]+)\]", opening)
    style_attr = f' data-style-id="{style_match.group(1)}"' if style_match else ""
    bare_counter = 0

    def qualify_name(match: re.Match[str]) -> str:
        nonlocal bare_counter
        value = match.group(1)
        if value.startswith("[UI]"):
            return match.group(0)
        bare_counter += 1
        slug = re.sub(r"[^a-z0-9-]+", "-", value.lower()).strip("-") or "node"
        qualified = f"{root_prefix}[CMP:{slug}][VAR:auto-{bare_counter}]"
        return f'data-pencil-name="{qualified}"'

    opening = re.sub(r'data-pencil-name="([^"]+)"', qualify_name, opening)
    opening = opening[:-1] + f' data-ewf-frame="true"{style_attr}>'
    normalized = opening + frame[opening_end + 1 :]
    normalized = re.sub(r'data-pencil-name="([^"]+)"', qualify_name, normalized)
    normalized = re.sub(r'\sdata-pencil-id="[^"]*"', "", normalized)
    normalized = inline_local_assets(normalized.replace("system-ui", "sans-serif"))
    return re.sub(r"[ \t]+\n", "\n", normalized)


def build(source: str, surface: str, frame_name: str) -> str:
    frame = normalize_frame(extract_div(source, f'data-pencil-name="{frame_name}"'), surface)
    width, height = ("410", "502") if surface == "device" else ("390", "844")
    background = "#17130f" if surface == "device" else "#faf7f2"
    styles = "\n".join(re.findall(r"<style\b[^>]*>(.*?)</style>", source, re.I | re.S))
    styles += "\n*{box-sizing:border-box}body{margin:0;background:" + background + ";font-family:\"Noto Sans SC\",sans-serif}.ewf-export{position:relative;width:" + width + "px;height:" + height + "px;overflow:hidden}"
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
