#!/usr/bin/env python3
"""将包含 10 个 EWF style frames 的 Pencil 导出物整理成离线审阅 HTML。"""

from __future__ import annotations

import argparse
import base64
import html
import json
import re
from pathlib import Path

from postprocess_ewf_direction_html import extract_div, normalize_frame


def inline_local_assets(frame: str) -> str:
    assets = Path(__file__).resolve().parent / "assets"
    for relative, mime in (("assets/woodfish-reference.png", "image/png"),):
        path = assets / Path(relative).name
        if not path.exists():
            continue
        encoded = base64.b64encode(path.read_bytes()).decode("ascii")
        data_url = f"data:{mime};base64,{encoded}"
        frame = frame.replace(f"url('{relative}')", f"url('{data_url}')")
        frame = frame.replace(f'url("{relative}")', f'url("{data_url}")')
        frame = frame.replace(f"url({relative})", f"url({data_url})")
    return frame


STYLE_FRAME_RE = re.compile(r'data-pencil-name="(\[UI\]\[STYLE:([A-Z0-9-]+)\]\[PAGE:[A-Z_]+\]\[ST:[A-Z0-9_]+\])"')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--surface", choices=("device", "miniapp"), required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()

    source = args.input.read_text(encoding="utf-8")
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    names = {item["id"]: item["name"] for item in manifest.get("styles", [])}
    frames: list[tuple[str, str]] = []
    seen: set[str] = set()
    for match in STYLE_FRAME_RE.finditer(source):
        frame_name, style_id = match.groups()
        if style_id in seen:
            continue
        seen.add(style_id)
        frames.append((style_id, inline_local_assets(normalize_frame(extract_div(source, f'data-pencil-name="{frame_name}"'), args.surface))))
    frames.sort(key=lambda item: item[0])
    width, height = (410, 502) if args.surface == "device" else (390, 844)
    bg = "#0f0d0a" if args.surface == "device" else "#ece7df"
    card_bg = "#1a1611" if args.surface == "device" else "#fffdf8"
    style = f"""
      *{{box-sizing:border-box}}
      body{{margin:0;background:{bg};color:#eee8dc;font-family:'Noto Sans SC','Noto Serif SC',sans-serif}}
      h1{{font-size:24px;font-weight:600;margin:0 0 24px}}
      .board{{padding:32px;display:grid;grid-template-columns:repeat(2,{width + 32}px);gap:34px 28px;align-items:start}}
      .card{{padding:16px;background:{card_bg};border:1px solid #ffffff18;border-radius:18px;position:relative;height:{height + 64}px}}
      .card > [data-ewf-frame="true"]{{left:16px!important;top:32px!important}}
      .label{{font-size:14px;letter-spacing:.04em;margin:0 0 12px;color:#cfc4b0}}
      .ewf-export{{position:relative;width:{width}px;height:{height}px;overflow:hidden}}
    """
    body = [f'<main class="board"><div style="grid-column:1/-1"><h1>EWF {args.surface} · 10 style candidates</h1></div>']
    for style_id, frame in frames:
        title = html.escape(f"{style_id} · {names.get(style_id, style_id)}")
        body.append(f'<section class="card"><div class="label">{title}</div>{frame}</section>')
    body.append("</main>")
    args.output.write_text(
        '<!doctype html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="ewf-style-board" content="true"><style>'
        + style
        + "</style></head><body>"
        + "".join(body)
        + "</body></html>\n",
        encoding="utf-8",
    )
    print(f"wrote {args.output} ({len(frames)} styles)")
    return 0 if len(frames) == 10 else 1


if __name__ == "__main__":
    raise SystemExit(main())
