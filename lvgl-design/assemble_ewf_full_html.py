#!/usr/bin/env python3
"""将 Pencil 多帧导出物组装为 EWF 单风格、离线、可校验 HTML。"""

from __future__ import annotations

import argparse
import base64
import html as html_lib
import re
from pathlib import Path

from postprocess_ewf_direction_html import extract_div, normalize_frame


ROOT_RE = re.compile(
    r'data-pencil-name="(\[UI\]\[STYLE:(?P<style>[A-Z0-9-]+)\]'
    r'\[PAGE:(?P<page>[A-Z_]+)\]\[ST:(?P<state>[A-Z0-9_]+)\])"'
)


def frame_names(source: str, style: str) -> list[str]:
    names: list[str] = []
    seen: set[str] = set()
    for match in ROOT_RE.finditer(source):
        if match.group("style") != style:
            continue
        name = match.group(1)
        if name not in seen:
            seen.add(name)
            names.append(name)
    return names


MATERIAL_MARKER = "[CMP:style-signature][VAR:texture]"
MATERIAL_TILE = (
    Path(__file__).resolve().parent.parent / "miniapp-design" / "assets" / "paper-fiber-128.png"
)
MATERIAL_TILE_PX = 128

_INLINE_IMAGE = re.compile(r"background-image:\s*url\('data:[^']*'\);\s*")


def strip_material_inline(frame: str) -> str:
    """把材质节点的内联位图降级为一个标记属性。

    Pencil 会按**节点尺寸**把材质重新编码成位图并内联（实测 350x684 约 82KB），
    而派生帧是克隆 BASE 帧得到的，同一张底纹会在交付物里重复十几遍。
    这里只保留几何与混合模式，真正的底纹由 build() 用原始平铺资产注入一次。
    """
    if MATERIAL_MARKER not in frame:
        return frame

    def visit(match: re.Match[str]) -> str:
        opening = match.group(0)
        if MATERIAL_MARKER not in opening:
            return opening
        opening = _INLINE_IMAGE.sub("", opening)
        if "data-ewf-material" not in opening:
            opening = opening[:-1] + ' data-ewf-material="true">'
        return opening

    return re.sub(r"<[^!/][^>]*>", visit, frame)


def material_rule() -> str:
    """材质底纹的样式规则；整份文档只内联一次。"""
    if not MATERIAL_TILE.exists():
        return ""
    encoded = base64.b64encode(MATERIAL_TILE.read_bytes()).decode("ascii")
    return (
        '      [data-ewf-material="true"] {\n'
        f"        background-image: url('data:image/png;base64,{encoded}');\n"
        "        background-repeat: repeat;\n"
        f"        background-size: {MATERIAL_TILE_PX}px {MATERIAL_TILE_PX}px;\n"
        "      }\n"
    )


def annotate_components(frame: str, surface: str) -> str:
    def annotate(match: re.Match[str]) -> str:
        opening = match.group(0)
        name_match = re.search(r'data-pencil-name="([^"]+)"', opening)
        if not name_match:
            return opening
        name = html_lib.unescape(name_match.group(1))
        attrs = ""
        if "[CMP:tap-rings]" in name and "[VAR:master]" in name:
            attrs += ' data-ewf-ring-count="3" data-ewf-duration-ms="160" data-ewf-ring-lifecycle="idle-flash-idle"'
        if surface == "device" and re.search(r'\[CMP:ring-(outer|middle|inner)\]', name):
            ring = re.search(r'\[CMP:ring-(outer|middle|inner)\]', name).group(1)
            attrs += f' data-ewf-ring="{ring}" data-ewf-flash-stroke="#e6bd69" data-ewf-duration-ms="160"'
        if "[CMP:scripture-history]" in name and "[VAR:scrollable]" in name:
            attrs += ' data-ewf-scroll-axis="vertical" data-ewf-scroll-anchor="tail" data-ewf-append-only="true" data-ewf-future-preview="false"'
        if "[CMP:scripture-history]" in name and re.search(r"\[VAR:line-(?:1|2)\]", name):
            attrs += ' data-ewf-line-char-count="13" data-ewf-punctuation-counted="true"'
        if "[CMP:scripture-history]" in name and "[VAR:line-3-prefix]" in name:
            attrs += ' data-ewf-line-char-count="1" data-ewf-line-slot-start="1" data-ewf-punctuation-counted="true"'
        if "[CMP:scripture-history]" in name and "[VAR:line-3-placeholders]" in name:
            attrs += ' data-ewf-placeholder-slots="8"'
        if "[CMP:settings-list]" in name and "[VAR:viewport]" in name:
            attrs += ' data-ewf-scroll-axis="vertical" data-ewf-max-visible-rows="4" data-ewf-row-height="80" data-ewf-row-gap="8" data-ewf-page-count="2" data-ewf-content-height="520"'
        if "[CMP:device-identity]" in name:
            attrs += ' data-ewf-identity-row="true"'
        if "[PAGE:TONGJI]" in name and "[VAR:stat-card]" in name:
            attrs += ' data-ewf-stat-card="true"'
        if re.match(r'^\[UI\]\[STYLE:[A-Z0-9-]+\]\[PAGE:TONGJI\]\[ST:[A-Z0-9_]+\]$', name):
            attrs += ' data-ewf-stats-scope="today-total-only"'
        if surface == "device" and "[CMP:glyph-current]" in name:
            anchor = "scripture-history-tail" if "[VAR:latest-inline]" in name else "charcell-tail"
            attrs += f' data-ewf-latest="true" data-ewf-anchor="{anchor}"'
            if "[VAR:latest-inline]" in name:
                attrs += ' data-ewf-line-index="2" data-ewf-slot-index="9"'
        if surface == "miniapp" and "[CMP:char-focus]" in name:
            attrs += ' data-ewf-latest="true" data-ewf-underline="latest"'
        return opening[:-1] + attrs + ">"

    return re.sub(r"<[^!/][^>]*>", annotate, frame)


def set_style_property(opening: str, prop: str, value: str) -> str:
    pattern = re.compile(rf"(?<![-\w])({re.escape(prop)}\s*:\s*)[^;\"']+", re.I)
    if pattern.search(opening):
        return pattern.sub(rf"\g<1>{value}", opening, count=1)
    if 'style="' in opening:
        return opening.replace('style="', f'style="{prop}: {value}; ', 1)
    if "style='" in opening:
        return opening.replace("style='", f"style='{prop}: {value}; ", 1)
    return opening[:-1] + f' style="{prop}: {value}">'


def element_span(frame: str, marker: str) -> tuple[int, int] | None:
    """定位含嵌套子元素的完整元素区间（起始偏移, 结束偏移）。

    组件实例在导出物中是带嵌套的容器（如 today-taps 下挂 label/value 两个
    div），用非贪婪正则只会匹配到第一个内层闭合标签，从而把同级的后续内容
    一并丢弃。这里按标签名做嵌套计数，与 extract_div 保持同一口径。
    """
    marker_start = frame.find(marker)
    if marker_start < 0:
        return None
    start = frame.rfind("<", 0, marker_start)
    if start < 0:
        return None
    open_end = frame.find(">", marker_start)
    if open_end < 0:
        return None
    if frame[open_end - 1] == "/":
        return (start, open_end + 1)
    tag_match = re.match(r"<\s*([a-zA-Z0-9]+)", frame[start : open_end + 1])
    if not tag_match:
        return None
    tokens = re.compile(rf"</?{tag_match.group(1)}\b[^>]*>", re.I)
    depth = 0
    for match in tokens.finditer(frame, start):
        depth += -1 if match.group(0).startswith("</") else 1
        if depth == 0:
            return (start, match.end())
    return None


def element_style(frame: str, marker: str) -> str:
    """取元素开标签的 style 串，用于读取既有几何（如进度轨道宽度）。"""
    span = element_span(frame, marker)
    if span is None:
        return ""
    match = re.search(r'style="([^"]*)"', frame[span[0] : span[1]])
    return match.group(1) if match else ""


def css_px(style: str, prop: str) -> float | None:
    match = re.search(rf"(?<![-\w]){prop}\s*:\s*([\d.]+)px", style, re.I)
    return float(match.group(1)) if match else None


def replace_node(
    frame: str,
    name: str,
    content: str | None = None,
    *,
    color: str | None = None,
    background_color: str | None = None,
    opacity: int | None = None,
    variant: str | None = None,
    new_name: str | None = None,
    font_size: int | None = None,
    font_weight: int | None = None,
    left: int | None = None,
    top: int | None = None,
    height: int | None = None,
    width: int | None = None,
    line_height: int | None = None,
    latest: bool | None = None,
    required: bool = False,
) -> str:
    target = f'data-pencil-name="{name}"'
    span = element_span(frame, target)
    if span is None:
        if required:
            raise ValueError(f"replace_node 未命中目标（说明节点被重命名或移动）: {name}")
        return frame
    start, end = span
    element = frame[start:end]
    open_end = element.find(">") + 1
    opening = element[:open_end]

    if new_name:
        opening = opening.replace(f'data-pencil-name="{name}"', f'data-pencil-name="{new_name}"', 1)
    if color:
        opening = set_style_property(opening, "color", color)
    if background_color:
        opening = set_style_property(opening, "background-color", background_color)
    if opacity is not None:
        opening = set_style_property(opening, "opacity", str(opacity))
    for prop, value in (
        ("font-size", f"{font_size}px" if font_size is not None else None),
        ("font-weight", str(font_weight) if font_weight is not None else None),
        ("left", f"{left}px" if left is not None else None),
        ("top", f"{top}px" if top is not None else None),
        ("height", f"{height}px" if height is not None else None),
        ("width", f"{width}px" if width is not None else None),
        ("line-height", f"{line_height}px" if line_height is not None else None),
    ):
        if value is not None:
            opening = set_style_property(opening, prop, value)
    if latest is False:
        opening = re.sub(r'\sdata-ewf-latest="true"', "", opening)
        opening = re.sub(r'\sdata-ewf-anchor="[^"]+"', "", opening)
    elif latest is True:
        opening = re.sub(r'\sdata-ewf-latest="true"', "", opening)
        opening = re.sub(r'\sdata-ewf-anchor="[^"]+"', "", opening)
        opening = opening[:-1] + ' data-ewf-latest="true" data-ewf-anchor="charcell-tail">'
    if variant:
        opening = opening[:-1] + f' data-ewf-component-variant="{variant}">'

    tail = element[open_end:]
    if content is None:
        body = tail
    else:
        close_idx = tail.rfind("</")
        closing = tail[close_idx:] if close_idx >= 0 else ""
        body = content + closing
    return frame[:start] + opening + body + frame[end:]


def replace_first_named_token(frame: str, token: str, content: str, *, color: str | None = None, variant: str | None = None) -> str:
    token_pattern = re.escape(token).replace("_", "[-_]")
    pattern = re.compile(r'(<[^>]*data-pencil-name="[^"]*' + token_pattern + r'[^"]*"[^>]*>)(.*?)(</[^>]+>)', re.I | re.S)

    def replace(match: re.Match[str]) -> str:
        opening = match.group(1)
        if color:
            opening = set_style_property(opening, "color", color)
        if variant:
            opening = opening[:-1] + f' data-ewf-component-variant="{variant}">'
        return opening + content + match.group(3)

    return pattern.sub(replace, frame, count=1)


def synthesize_device_states(frames: list[tuple[str, str]], style: str) -> list[tuple[str, str]]:
    by_key: dict[str, str] = {}
    for name, frame in frames:
        match = ROOT_RE.search(f'data-pencil-name="{name}"')
        if match:
            by_key[f"{match.group('page')}.{match.group('state')}"] = frame

    def clone(page: str, source_state: str, target_state: str) -> str:
        source = by_key[f"{page}.{source_state}"]
        cloned = source.replace(f"[ST:{source_state}]", f"[ST:{target_state}]")
        return cloned.replace(
            'data-ewf-frame="true"',
            f'data-ewf-frame="true" data-ewf-screen-variant="false" data-ewf-component-state="{target_state}"',
            1,
        )

    def progress(frame: str, state: str, label: str, filled_segments: int) -> str:
        prefix = f"[UI][STYLE:{style}][PAGE:MUYU][ST:{state}][CMP:scripture-progress]"
        frame = replace_node(frame, f"{prefix}[VAR:master]", variant=state)
        frame = replace_node(frame, f"{prefix}[VAR:label]", label)
        for index in range(8):
            frame = replace_node(
                frame,
                f"{prefix}[VAR:segment-{index}]",
                background_color="#d9a441" if index < filled_segments else "#252525",
            )
        return frame

    def normal_slot(frame: str, state: str, source_cmp: str, source_var: str, slot: int, content: str, color: str) -> str:
        source_name = f"[UI][STYLE:{style}][PAGE:MUYU][ST:{state}][CMP:{source_cmp}][VAR:{source_var}]"
        target_name = f"[UI][STYLE:{style}][PAGE:MUYU][ST:{state}][CMP:charcell][VAR:slot-{slot}]"
        return replace_node(
            frame,
            source_name,
            content,
            new_name=target_name,
            color=color,
            font_size=24,
            font_weight=500,
            left=15 + (slot - 1) * 48,
            top=20,
            height=34,
            line_height=28,
            latest=False,
        )

    def latest_slot(frame: str, state: str, source_var: str, slot: int, content: str) -> str:
        source_name = f"[UI][STYLE:{style}][PAGE:MUYU][ST:{state}][CMP:charcell][VAR:{source_var}]"
        target_name = f"[UI][STYLE:{style}][PAGE:MUYU][ST:{state}][CMP:glyph-current][VAR:latest]"
        return replace_node(
            frame,
            source_name,
            content,
            new_name=target_name,
            color="#d9a441",
            font_size=48,
            font_weight=700,
            left=15 + (slot - 1) * 48,
            top=7,
            height=54,
            line_height=54,
            latest=True,
        )

    values = {
        "BASE": ("立即同步", "#cfc4b0"),
        "SYNC_BUSY": ("同步中", "#d9a441"),
        "SYNC_OK": ("已同步", "#7fa98f"),
        "SYNC_PENDING": ("待同步", "#d9a441"),
        "SYNC_FAIL": ("同步失败", "#b3705c"),
    }
    out = list(frames)
    if "MUYU.BASE" in by_key:
        empty = clone("MUYU", "BASE", "EMPTY")
        empty = replace_node(empty, f"[UI][STYLE:{style}][PAGE:MUYU][ST:EMPTY][CMP:charcell][VAR:recent-window]", variant="EMPTY")
        empty = normal_slot(empty, "EMPTY", "glyph-current", "latest", 3, "·", "#a6a29a")
        for slot in (1, 2, 4, 5, 6, 7):
            empty = replace_node(
                empty,
                f"[UI][STYLE:{style}][PAGE:MUYU][ST:EMPTY][CMP:charcell][VAR:slot-{slot}]",
                "·",
                color="#a6a29a",
            )
        empty = progress(empty, "EMPTY", "心经进度 0 / 260 字 · 0%", 0)
        empty = replace_node(empty, f"[UI][STYLE:{style}][PAGE:MUYU][ST:EMPTY][CMP:today-taps][VAR:confirmed]", variant="EMPTY")
        empty = replace_first_named_token(empty, "cmp_today_taps_value", "0 次", color="#fbfaf0")
        empty = replace_node(empty, f"[UI][STYLE:{style}][PAGE:MUYU][ST:EMPTY][CMP:total-taps][VAR:confirmed]", "累计敲击 3,328 次", variant="EMPTY")
        out.append((f"[UI][STYLE:{style}][PAGE:MUYU][ST:EMPTY]", empty))

        mid = clone("MUYU", "BASE", "MID")
        mid = replace_node(mid, f"[UI][STYLE:{style}][PAGE:MUYU][ST:MID][CMP:charcell][VAR:recent-window]", variant="MID")
        mid = normal_slot(mid, "MID", "glyph-current", "latest", 3, "在", "#fbfaf0")
        for slot, value in ((4, "行"), (5, "深"), (6, "般")):
            mid = replace_node(mid, f"[UI][STYLE:{style}][PAGE:MUYU][ST:MID][CMP:charcell][VAR:slot-{slot}]", value, color="#fbfaf0")
        mid = latest_slot(mid, "MID", "slot-7", 7, "若")
        mid = progress(mid, "MID", "心经进度 84 / 260 字 · 32.3%", 3)
        out.append((f"[UI][STYLE:{style}][PAGE:MUYU][ST:MID]", mid))

        full = clone("MUYU", "BASE", "FULL")
        full = replace_node(full, f"[UI][STYLE:{style}][PAGE:MUYU][ST:FULL][CMP:charcell][VAR:recent-window]", variant="FULL")
        full = replace_node(full, f"[UI][STYLE:{style}][PAGE:MUYU][ST:FULL][CMP:charcell][VAR:slot-1]", "色", color="#fbfaf0")
        full = replace_node(full, f"[UI][STYLE:{style}][PAGE:MUYU][ST:FULL][CMP:charcell][VAR:slot-2]", "即", color="#fbfaf0")
        full = normal_slot(full, "FULL", "glyph-current", "latest", 3, "是", "#fbfaf0")
        full = latest_slot(full, "FULL", "slot-4", 4, "空")
        full = progress(full, "FULL", "心经进度 196 / 260 字 · 75.4%", 6)
        out.append((f"[UI][STYLE:{style}][PAGE:MUYU][ST:FULL]", full))

        untrusted = clone("MUYU", "BASE", "UNTRUSTED_TIME")
        untrusted = replace_node(
            untrusted,
            f"[UI][STYLE:{style}][PAGE:MUYU][ST:UNTRUSTED_TIME][CMP:today-taps][VAR:confirmed]",
            variant="UNTRUSTED_TIME",
        )
        untrusted = replace_first_named_token(untrusted, "cmp_today_taps_value", "待校时", color="#a6a29a")
        out.append((f"[UI][STYLE:{style}][PAGE:MUYU][ST:UNTRUSTED_TIME]", untrusted))

    if "TONGJI.BASE" in by_key:
        statistics = clone("TONGJI", "BASE", "UNTRUSTED_TIME")
        today_target = f"[UI][STYLE:{style}][PAGE:TONGJI][ST:UNTRUSTED_TIME][CMP:today-taps][VAR:stat-card]"
        if today_target in statistics:
            statistics = replace_first_named_token(statistics, "cmp_stat_card_value", "待校时", color="#a6a29a", variant="UNTRUSTED_TIME")
            statistics = replace_first_named_token(statistics, "cmp_stat_card_unit", "", color="#a6a29a", variant="UNTRUSTED_TIME")
        else:
            today_target = f"[UI][STYLE:{style}][PAGE:TONGJI][ST:UNTRUSTED_TIME][CMP:today-taps][VAR:confirmed]"
            statistics = replace_node(statistics, today_target, "今日敲击 待校时", variant="UNTRUSTED_TIME")
        out.append((f"[UI][STYLE:{style}][PAGE:TONGJI][ST:UNTRUSTED_TIME]", statistics))

    if "SHEZHI.BASE" not in by_key:
        return out
    base = by_key["SHEZHI.BASE"]
    for state in ("SYNC_BUSY", "SYNC_OK", "SYNC_PENDING", "SYNC_FAIL"):
        value, color = values[state]
        name = f"[UI][STYLE:{style}][PAGE:SHEZHI][ST:{state}]"
        cloned = base.replace("[ST:BASE]", f"[ST:{state}]")
        cloned = cloned.replace('data-ewf-frame="true"', f'data-ewf-frame="true" data-ewf-screen-variant="false" data-ewf-component-state="{state}"', 1)
        target = f"[UI][STYLE:{style}][PAGE:SHEZHI][ST:{state}][CMP:sync-btn][VAR:value]"
        cloned = replace_node(cloned, target, value, color=color, variant=state)
        out.append((name, cloned))
    return out


def synthesize_miniapp_states(frames: list[tuple[str, str]], style: str) -> list[tuple[str, str]]:
    out = list(frames)
    by_key: dict[str, str] = {}
    for name, frame in frames:
        match = ROOT_RE.search(f'data-pencil-name="{name}"')
        if match:
            by_key[f"{match.group('page')}.{match.group('state')}"] = frame

    def clone(page: str, source_state: str, target_state: str) -> str:
        source = by_key[f"{page}.{source_state}"]
        cloned = source.replace(f"[ST:{source_state}]", f"[ST:{target_state}]")
        return cloned.replace('data-ewf-frame="true"', f'data-ewf-frame="true" data-ewf-screen-variant="false" data-ewf-component-state="{target_state}"', 1)

    login = clone("LOGIN", "BASE", "PERM_ERROR")
    login = replace_node(login, f"[UI][STYLE:{style}][PAGE:LOGIN][ST:PERM_ERROR][CMP:state-banner][VAR:status]", "授权失败", color="#a64c3e", variant="PERM_ERROR", required=True)
    login = replace_node(login, f"[UI][STYLE:{style}][PAGE:LOGIN][ST:PERM_ERROR][CMP:state-banner][VAR:permission-copy]", "授权失败，请重新授权", required=True)
    login = replace_node(login, f"[UI][STYLE:{style}][PAGE:LOGIN][ST:PERM_ERROR][CMP:login-action][VAR:label]", "重新授权", required=True)
    out.append((f"[UI][STYLE:{style}][PAGE:LOGIN][ST:PERM_ERROR]", login))

    reading_states = {
        "REPLAY": "回放中 · 新事件排队",
        "OFFLINE": "网络不可用",
        "EMPTY": "等待设备诵读",
        "DONE": "本轮已完成",
    }
    for state, label in reading_states.items():
        reading = clone("READING", "LIVE", state)
        prefix = f"[UI][STYLE:{style}][PAGE:READING][ST:{state}]"
        progress = f"{prefix}[CMP:scripture-progress]"
        reading = replace_node(reading, f"{prefix}[CMP:state-banner][VAR:status]", label, variant=state, required=True)
        # 轨道宽度从 pen 导出物实测，避免把进度几何硬编码进脚本
        track_width = css_px(element_style(reading, f"{progress}[VAR:track]"), "width")
        if state == "EMPTY":
            for suffix in ("prefix-line-1", "prefix-line-2"):
                reading = replace_node(reading, f"{prefix}[CMP:readingline][VAR:{suffix}]", "", required=True)
            reading = replace_node(reading, f"{prefix}[CMP:char-focus][VAR:latest]", "", opacity=0, required=True)
            # 空态不得伪造进度（FR-F-002 / AD-2）：计数归零、填充归零
            reading = replace_node(reading, f"{progress}[VAR:count]", "心经进度 0 / 260 字", required=True)
            reading = replace_node(reading, f"{progress}[VAR:percent]", "0%", required=True)
            reading = replace_node(reading, f"{progress}[VAR:value]", width=0, required=True)
            # 最新字已隐藏，焦点下划线必须同步隐藏，否则空纸上留一条孤立刻线
            reading = replace_node(reading, f"{prefix}[CMP:char-focus][VAR:underline-latest]", opacity=0, required=True)
        if state == "DONE":
            reading = replace_node(reading, f"{progress}[VAR:count]", "心经进度 260 / 260 字", required=True)
            reading = replace_node(reading, f"{progress}[VAR:percent]", "100%", required=True)
            # 完成态文案已是 100%，填充必须满格，否则自相矛盾
            if track_width is not None:
                reading = replace_node(reading, f"{progress}[VAR:value]", width=int(track_width), required=True)
        out.append((f"[UI][STYLE:{style}][PAGE:READING][ST:{state}]", reading))

    records = clone("RECORDS", "BASE", "EMPTY")
    records = replace_node(records, f"[UI][STYLE:{style}][PAGE:RECORDS][ST:EMPTY][CMP:state-banner][VAR:status]", "尚无诵读记录", variant="EMPTY", required=True)
    for suffix in ("today", "week", "month", "total", "streak"):
        records = replace_node(records, f"[UI][STYLE:{style}][PAGE:RECORDS][ST:EMPTY][CMP:statcard][VAR:{suffix}-value]", "暂无", required=True)
    out.append((f"[UI][STYLE:{style}][PAGE:RECORDS][ST:EMPTY]", records))

    device = clone("DEVICE", "BASE", "FAIL_RETRY")
    device = replace_node(device, f"[UI][STYLE:{style}][PAGE:DEVICE][ST:FAIL_RETRY][CMP:state-banner][VAR:copy]", "同步失败，请重试", color="#a64c3e", variant="FAIL_RETRY", required=True)
    device = replace_node(device, f"[UI][STYLE:{style}][PAGE:DEVICE][ST:FAIL_RETRY][CMP:sync-action][VAR:label]", "重试", required=True)
    out.append((f"[UI][STYLE:{style}][PAGE:DEVICE][ST:FAIL_RETRY]", device))

    settings = clone("SETTINGS", "BASE", "PENDING")
    settings = replace_node(settings, f"[UI][STYLE:{style}][PAGE:SETTINGS][ST:PENDING][CMP:state-banner][VAR:status]", "待设备应用", color="#a66b3a", variant="PENDING", required=True)
    settings = replace_node(settings, f"[UI][STYLE:{style}][PAGE:SETTINGS][ST:PENDING][CMP:setting-mirror][VAR:state-value]", "待设备应用", color="#a66b3a", required=True)
    out.append((f"[UI][STYLE:{style}][PAGE:SETTINGS][ST:PENDING]", settings))
    return out


def synthesize_component_only_states(frames: list[tuple[str, str]], surface: str, style: str) -> list[tuple[str, str]]:
    if surface == "device" and style == "DEVICE-01":
        return synthesize_device_states(frames, style)
    if surface == "miniapp" and style == "MINI-06":
        return synthesize_miniapp_states(frames, style)
    return frames


def clean_scaffold(source: str) -> str:
    source = re.sub(r"<link\b[^>]*>\s*", "", source, flags=re.I)
    source = re.sub(r"@import\s+url\([^)]*\);?", "", source, flags=re.I)
    source = re.sub(r"\sdata-pencil-id=\"[^\"]*\"", "", source, flags=re.I)
    return source


def build(source: str, surface: str, style: str) -> str:
    source = clean_scaffold(source)
    names = frame_names(source, style)
    if not names:
        raise ValueError(f"未找到 {style} 的页面根帧")
    extracted: list[tuple[str, str]] = []
    for name in names:
        frame = normalize_frame(extract_div(source, f'data-pencil-name="{name}"'), surface)
        extracted.append((name, strip_material_inline(annotate_components(frame, surface))))
    extracted = synthesize_component_only_states(extracted, surface, style)
    frames = [frame for _, frame in extracted]
    width, height = ((410, 502) if surface == "device" else (390, 844))
    frame_markup = "\n".join(frames)
    return f'''<!doctype html>
<html lang="zh-CN">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="ewf-surface" content="{surface}">
    <meta name="ewf-selected-style" content="{style}">
    <style>
      *, ::before, ::after {{ box-sizing: border-box; }}
      html, body {{ margin: 0; padding: 0; background: transparent; }}
      body {{ font-family: "Noto Sans SC", sans-serif; }}
      .ewf-full-export {{ display: flex; flex-wrap: wrap; gap: 24px; align-items: flex-start; background: transparent; }}
      .ewf-full-export > [data-ewf-frame="true"] {{ position: relative !important; left: auto !important; top: auto !important; flex: 0 0 {width}px; width: {width}px !important; height: {height}px !important; }}
      [data-ewf-ring-count="3"] [data-ewf-ring] {{ transition: outline-color 40ms linear, opacity 120ms linear; }}
      [data-ewf-ring-count="3"]:hover [data-ewf-ring],
      [data-ewf-ring-count="3"]:active [data-ewf-ring],
      [data-ewf-ring-count="3"][data-ewf-motion="flash"] [data-ewf-ring] {{ outline-color: #e6bd69 !important; opacity: 1 !important; }}
      @media (prefers-reduced-motion: reduce) {{ [data-ewf-ring-count="3"] [data-ewf-ring] {{ transition: none; }} }}
{material_rule()}    </style>
  </head>
  <body>
    <main class="ewf-full-export" data-ewf-surface="{surface}" data-ewf-style="{style}">
{frame_markup}
    </main>
  </body>
</html>
'''


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--surface", choices=("device", "miniapp"), required=True)
    parser.add_argument("--style", required=True)
    args = parser.parse_args()
    args.output.write_text(build(args.input.read_text(encoding="utf-8"), args.surface, args.style), encoding="utf-8")
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
