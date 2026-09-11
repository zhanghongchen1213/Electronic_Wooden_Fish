#!/usr/bin/env python3
"""标准符号风格的电子木鱼硬件原理图 SVG 页。

这里画的是给人工复刻用的逻辑原理图：元件采用常见 IEC/KiCad 风格符号，
网络名和推荐值来自硬件网络清单。对于尚未确认封装/引脚号的器件，图中使用
功能引脚名并在页脚注明“按所购数据手册核脚”，避免伪造封装针号。
"""

from __future__ import annotations

import html
from typing import Iterable, Sequence


INK = "#172033"
WIRE = "#1f2937"
GRID = "#dbe4ef"
BLUE = "#0b5cab"
GREEN = "#13795b"
AMBER = "#9a6700"
RED = "#b42318"
PURPLE = "#6941c6"
PAPER = "#ffffff"
NOTE = "#fff8db"
NOTE_LINE = "#d6a700"


def e(value: object) -> str:
    return html.escape(str(value), quote=True)


def tx(x: float, y: float, lines: Iterable[str], *, size: int = 14, color: str = INK,
       weight: str = "400", anchor: str = "start", family: str = "Arial, PingFang SC, sans-serif",
       gap: int | None = None) -> str:
    lines = list(lines)
    if not lines:
        return ""
    gap = gap or int(size * 1.25)
    spans = []
    for i, line in enumerate(lines):
        spans.append(f'<tspan x="{x:g}" dy="{0 if i == 0 else gap:g}">{e(line)}</tspan>')
    return f'<text x="{x:g}" y="{y:g}" fill="{color}" font-size="{size}px" font-weight="{weight}" text-anchor="{anchor}" font-family="{family}">{"".join(spans)}</text>'


def wrap(value: str, width: int = 52) -> list[str]:
    out: list[str] = []
    cur = ""
    for ch in str(value):
        if ch == "\n":
            out.append(cur)
            cur = ""
            continue
        # Chinese glyphs are approximately two ASCII cells wide.
        used = sum(2 if ord(c) > 127 else 1 for c in cur)
        if cur and used + (2 if ord(ch) > 127 else 1) > width:
            out.append(cur)
            cur = ""
        cur += ch
    if cur or not out:
        out.append(cur)
    return out


def defs() -> str:
    return f'''<defs>
      <pattern id="std-grid" width="40" height="40" patternUnits="userSpaceOnUse">
        <path d="M40 0H0V40" fill="none" stroke="{GRID}" stroke-width="0.8"/>
      </pattern>
      <marker id="std-arrow" markerWidth="9" markerHeight="7" refX="8" refY="3.5" orient="auto">
        <path d="M0 0L9 3.5L0 7Z" fill="{WIRE}"/>
      </marker>
      <marker id="std-arrow-blue" markerWidth="9" markerHeight="7" refX="8" refY="3.5" orient="auto">
        <path d="M0 0L9 3.5L0 7Z" fill="{BLUE}"/>
      </marker>
      <style>
        .std-wire {{ fill:none; stroke:{WIRE}; stroke-width:2; stroke-linejoin:round; stroke-linecap:round; }}
        .std-thin {{ fill:none; stroke:{WIRE}; stroke-width:1.3; }}
        .std-text {{ font-family:Arial,"PingFang SC","Noto Sans SC",sans-serif; fill:{INK}; }}
        .std-value {{ font-family:Arial,"PingFang SC","Noto Sans SC",sans-serif; fill:#344054; font-size:12px; }}
        .std-net {{ font-family:ui-monospace,SFMono-Regular,Menlo,monospace; fill:{BLUE}; font-size:12px; font-weight:700; }}
      </style>
    </defs>'''


def wire(points: Sequence[tuple[float, float]], *, color: str = WIRE, width: float = 2,
         dashed: bool = False, arrow: bool = False, label: str = "", label_at: tuple[float, float] | None = None) -> str:
    d = "M " + " L ".join(f"{x:g} {y:g}" for x, y in points)
    dash = ' stroke-dasharray="7 5"' if dashed else ""
    marker = f' marker-end="url(#{"std-arrow-blue" if color == BLUE else "std-arrow"})"' if arrow else ""
    result = f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width:g}" stroke-linejoin="round" stroke-linecap="round"{dash}{marker}/>'
    if label:
        x, y = label_at or points[len(points) // 2]
        pad = max(5, len(label) * 3.5)
        result += f'<rect x="{x-pad:g}" y="{y-13:g}" width="{2*pad:g}" height="17" rx="3" fill="{PAPER}" opacity="0.94"/>'
        result += tx(x, y, [label], size=11, color=color, weight="700", anchor="middle", family="ui-monospace,SFMono-Regular,Menlo,monospace")
    return result


def net_label(x: float, y: float, value: str, *, color: str = BLUE, anchor: str = "start") -> str:
    width = max(58, len(value) * 7 + 16)
    left = x if anchor == "start" else x - width
    return f'<rect x="{left:g}" y="{y-15:g}" width="{width:g}" height="21" rx="4" fill="{PAPER}" stroke="{color}" stroke-width="1"/>' + tx(left + (8 if anchor == "start" else width - 8), y, [value], size=11, color=color, weight="700", anchor=anchor, family="ui-monospace,SFMono-Regular,Menlo,monospace")


def gnd(x: float, y: float, label: str = "GND") -> str:
    return (f'<path d="M{x:g} {y:g}v10m-12 0h24m-8 6h16m-4 6h8" class="std-thin"/>'
            + tx(x + 18, y + 17, [label], size=11, color=INK))


def vcc(x: float, y: float, label: str) -> str:
    return (f'<path d="M{x:g} {y+18:g}V{y:g}m-7 7 7-7 7 7" class="std-thin"/>'
            + tx(x + 12, y + 4, [label], size=11, color=BLUE, weight="700"))


def tp(x: float, y: float, ref: str, net: str) -> str:
    return (f'<circle cx="{x:g}" cy="{y:g}" r="9" fill="{PAPER}" stroke="{PURPLE}" stroke-width="2"/>'
            + tx(x + 15, y - 3, [ref], size=11, color=PURPLE, weight="700")
            + tx(x + 15, y + 12, [net], size=10, color=PURPLE, family="ui-monospace,SFMono-Regular,Menlo,monospace"))


def resistor(x: float, y: float, ref: str, value: str, *, vertical: bool = False, color: str = INK) -> tuple[str, tuple[float, float], tuple[float, float]]:
    # IEC rectangular resistor; endpoints are 45 px apart.
    if vertical:
        body = f'<path d="M{x:g} {y-45:g}V{y-18:g}h-10v36h20v-36h-10v27M{x:g} {y+18:g}V{y+45:g}" fill="none" stroke="{color}" stroke-width="2"/>'
        text = tx(x + 18, y - 3, [ref, value], size=11, color=color, gap=14)
        return body + text, (x, y - 45), (x, y + 45)
    body = f'<path d="M{x-45:g} {y:g}H{x-18:g}v-10h36v20h-36v-10h27M{x+18:g} {y:g}H{x+45:g}" fill="none" stroke="{color}" stroke-width="2"/>'
    text = tx(x, y - 17, [ref], size=11, color=color, weight="700", anchor="middle") + tx(x, y + 28, [value], size=11, color="#344054", anchor="middle")
    return body + text, (x - 45, y), (x + 45, y)


def capacitor(x: float, y: float, ref: str, value: str, *, vertical: bool = True, polarized: bool = False) -> tuple[str, tuple[float, float], tuple[float, float]]:
    if vertical:
        pol = "+" if polarized else ""
        body = f'<path d="M{x:g} {y-45:g}V{y-9:g}M{x-15:g} {y-9:g}h30M{x-15:g} {y+9:g}h30M{x:g} {y+9:g}V{y+45:g}" class="std-thin"/>'
        text = tx(x + 21, y - 3, [ref, value], size=11, color=INK, gap=14) + (tx(x - 24, y - 10, [pol], size=13, color=RED, weight="700") if pol else "")
        return body + text, (x, y - 45), (x, y + 45)
    body = f'<path d="M{x-45:g} {y:g}H{x-9:g}M{x-9:g} {y-15:g}v30M{x+9:g} {y-15:g}v30M{x+9:g} {y:g}H{x+45:g}" class="std-thin"/>'
    text = tx(x, y - 22, [ref], size=11, color=INK, weight="700", anchor="middle") + tx(x, y + 30, [value], size=11, color="#344054", anchor="middle")
    return body + text, (x - 45, y), (x + 45, y)


def inductor(x: float, y: float, ref: str, value: str, *, vertical: bool = False) -> tuple[str, tuple[float, float], tuple[float, float]]:
    if vertical:
        body = f'<path d="M{x:g} {y-45:g}V{y-25:g}c-18 0-18 20 0 20s18 20 0 20-18 20 0 20 18-20 0-20M{x:g} {y+35:g}V{y+45:g}" fill="none" stroke="{INK}" stroke-width="2"/>'
        text = tx(x + 22, y - 4, [ref, value], size=11, color=INK, gap=14)
        return body + text, (x, y - 45), (x, y + 45)
    body = f'<path d="M{x-45:g} {y:g}H{x-25:g}c0-18 20-18 20 0s20 18 20 0 20-18 20 0 20 18 20 0 20-18 20 0M{x+25:g} {y:g}H{x+45:g}" fill="none" stroke="{INK}" stroke-width="2"/>'
    text = tx(x, y - 18, [ref], size=11, color=INK, weight="700", anchor="middle") + tx(x, y + 28, [value], size=11, color="#344054", anchor="middle")
    return body + text, (x - 45, y), (x + 45, y)


def diode(x: float, y: float, ref: str, value: str, *, vertical: bool = False, color: str = INK) -> tuple[str, tuple[float, float], tuple[float, float]]:
    if vertical:
        body = f'<path d="M{x:g} {y-45:g}V{y-14:g}M{x-14:g} {y-14:g}l14 24 14-24M{x:g} {y+10:g}v35" fill="none" stroke="{color}" stroke-width="2"/>'
        text = tx(x + 20, y - 2, [ref, value], size=11, color=color, gap=14)
        return body + text, (x, y - 45), (x, y + 45)
    body = f'<path d="M{x-45:g} {y:g}H{x-14:g}M{x-14:g} {y-14:g}l24 14-24 14M{x+10:g} {y-18:g}v36M{x+10:g} {y:g}H{x+45:g}" fill="none" stroke="{color}" stroke-width="2"/>'
    text = tx(x, y - 22, [ref], size=11, color=color, weight="700", anchor="middle") + tx(x, y + 30, [value], size=11, color="#344054", anchor="middle")
    return body + text, (x - 45, y), (x + 45, y)


def led(x: float, y: float, ref: str, value: str) -> tuple[str, tuple[float, float], tuple[float, float]]:
    body = f'<path d="M{x-45:g} {y:g}H{x-14:g}M{x-14:g} {y-14:g}l24 14-24 14M{x+10:g} {y-18:g}v36M{x+10:g} {y:g}H{x+45:g}" fill="none" stroke="{GREEN}" stroke-width="2"/><path d="M{x-1:g} {y-24:g}l12-12m-2 10h10m-20 22 12-12m-2 10h10" fill="none" stroke="{GREEN}" stroke-width="1.4"/>'
    text = tx(x, y - 28, [ref], size=11, color=GREEN, weight="700", anchor="middle") + tx(x, y + 32, [value], size=11, color="#344054", anchor="middle")
    return body + text, (x - 45, y), (x + 45, y)


def opamp(x: float, y: float, ref: str, value: str, *, comparator: bool = False) -> dict[str, object]:
    color = RED if comparator else GREEN
    body = f'<path d="M{x-45:g} {y-55:g}L{x+50:g} {y:g}L{x-45:g} {y+55:g}Z" fill="{PAPER}" stroke="{color}" stroke-width="2"/>'
    body += tx(x - 34, y - 22, ["−"], size=18, color=color, weight="700", anchor="middle")
    body += tx(x - 34, y + 29, ["+"], size=18, color=color, weight="700", anchor="middle")
    body += tx(x + 10, y - 82, [ref], size=12, color=color, weight="700", anchor="middle")
    body += tx(x + 10, y + 78, [value], size=11, color="#344054", anchor="middle")
    return {"svg": body, "in_minus": (x - 80, y - 28), "in_plus": (x - 80, y + 28), "out": (x + 50, y), "vcc": (x - 5, y - 80), "gnd": (x - 5, y + 80)}


def ic(x: float, y: float, w: float, h: float, ref: str, value: str, *, left: Sequence[str] = (), right: Sequence[str] = (), top: Sequence[str] = (), bottom: Sequence[str] = (), color: str = BLUE, dashed: bool = False) -> dict[str, object]:
    dash = ' stroke-dasharray="7 5"' if dashed else ""
    parts = [f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" rx="4" fill="{PAPER}" stroke="{color}" stroke-width="2"{dash}/>',
             tx(x + w/2, y + 25, [ref], size=15, color=color, weight="700", anchor="middle"),
             tx(x + w/2, y + 45, wrap(value, max(18, int(w/8))), size=11, color="#344054", anchor="middle")]
    points: dict[str, tuple[float, float]] = {}
    def side(items: Sequence[str], which: str) -> None:
        if not items:
            return
        n = len(items)
        for i, name in enumerate(items):
            if which in ("left", "right"):
                py = y + 70 + (i + 0.5) * max(22, (h - 88) / n)
                px = x if which == "left" else x + w
                outx = px - 26 if which == "left" else px + 26
                parts.append(f'<path d="M{outx:g} {py:g}H{px:g}" class="std-thin"/>')
                parts.append(tx(outx - 4 if which == "left" else outx + 4, py + 4, [name], size=10, color=color, anchor="end" if which == "left" else "start"))
                points[name] = (outx, py)
            else:
                n2 = len(items)
                px = x + 24 + (i + 0.5) * max(24, (w - 48) / n2)
                py = y if which == "top" else y + h
                outy = py - 25 if which == "top" else py + 25
                parts.append(f'<path d="M{px:g} {outy:g}V{py:g}" class="std-thin"/>')
                parts.append(tx(px, outy - 5 if which == "top" else outy + 15, [name], size=10, color=color, anchor="middle"))
                points[name] = (px, outy)
    side(left, "left"); side(right, "right"); side(top, "top"); side(bottom, "bottom")
    return {"svg": "".join(parts), "pins": points, "box": (x, y, w, h)}


def connector(x: float, y: float, w: float, h: float, ref: str, value: str, pins: Sequence[tuple[str, str]], *, color: str = AMBER, side: str = "right") -> dict[str, object]:
    parts = [f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" fill="{PAPER}" stroke="{color}" stroke-width="2"/>',
             tx(x + w/2, y + 22, [ref], size=14, color=color, weight="700", anchor="middle"),
             tx(x + w/2, y + 40, wrap(value, max(16, int(w/8))), size=10, color="#344054", anchor="middle")]
    points: dict[str, tuple[float, float]] = {}
    n = len(pins)
    for i, (number, name) in enumerate(pins):
        py = y + 60 + (i + 0.5) * max(18, (h - 72) / max(1, n))
        if side == "right":
            px, outx = x + w, x + w + 35
            parts.append(f'<path d="M{px:g} {py:g}H{outx:g}" class="std-thin"/>')
            parts.append(tx(outx + 5, py + 3, [f"{number} {name}"], size=10, color=color))
        else:
            px, outx = x, x - 35
            parts.append(f'<path d="M{outx:g} {py:g}H{px:g}" class="std-thin"/>')
            parts.append(tx(outx - 5, py + 3, [f"{number} {name}"], size=10, color=color, anchor="end"))
        points[name] = (outx, py)
    return {"svg": "".join(parts), "pins": points}


def note(x: float, y: float, w: float, h: float, title: str, lines: Iterable[str], *, color: str = NOTE_LINE) -> str:
    parts = [f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" rx="4" fill="{NOTE}" stroke="{color}" stroke-width="1.3"/>', tx(x + 12, y + 22, [title], size=12, color=color, weight="700")]
    for i, line in enumerate(lines):
        parts.append(tx(x + 12, y + 43 + i * 17, wrap(line, max(18, int((w - 24) / 7))), size=10, color=INK))
    return "".join(parts)


def title_block(page_no: int, title: str, subtitle: str) -> str:
    return (tx(48, 42, [f"P{page_no:02d}  {title}"], size=22, color=INK, weight="700")
            + tx(48, 66, wrap(subtitle, 175), size=11, color="#475467")
            + f'<rect x="1360" y="30" width="380" height="62" fill="{PAPER}" stroke="#98a2b3"/>'
            + tx(1375, 51, ["电子木鱼 / HARDWARE BASELINE"], size=11, color="#475467", weight="700")
            + tx(1375, 73, ["逻辑原理图 · 手工复刻参考 · V1.0"], size=10, color="#667085"))


def footer(page_no: int, note_text: str = "功能引脚名用于人工复刻；封装针号以所购器件数据手册和 EDA 库为准。") -> str:
    return (f'<path d="M48 1010H1750" stroke="#98a2b3"/>'
            + tx(48, 1033, [f"P{page_no:02d} · {note_text}"], size=10, color="#667085")
            + tx(1740, 1033, ["STATUS: BASELINE / NOT ERC"], size=10, color=RED, weight="700", anchor="end"))


def sheet(page_no: int, title: str, subtitle: str, body: str, *, footer_note: str = "") -> str:
    page_ids = {1: "P01_POWER_USB", 2: "P02_MCU_USB", 3: "P03_DISPLAY_TOUCH", 4: "P04_AUDIO_RGB", 5: "P05_PVDF", 6: "P06_MODEM", 7: "P07_SHARED_SENSORS"}
    page_id = page_ids.get(page_no, f"P{page_no:02d}")
    return f'''<svg class="std-sheet" data-page="{page_id}" data-standard-page="P{page_no:02d}" viewBox="0 0 1800 1060" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="{e(title)} 标准原理图">
      {defs()}<rect width="1800" height="1060" fill="{PAPER}"/><rect width="1800" height="1060" fill="url(#std-grid)"/>
      {title_block(page_no, title, subtitle)}{body}{footer(page_no, footer_note or "功能引脚名用于人工复刻；封装针号以所购器件数据手册和 EDA 库为准。")}
    </svg>'''


def p01_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 72, "P01 关键边界", ["J1 VBUS 只进 BQ VBUS；BQ SYS 仅保留系统侧/TP。M100 VIN 必须来自 VBAT_SW。USB D+/D− 只经 U6 FSW7227 选择，不能并联。"], color=AMBER))
    # Main components.
    j1 = connector(70, 205, 205, 275, "J1", "TYPEC-304-ACP16 / USB-C", [("A4/B4/A9/B9", "VBUS"), ("A6/B6", "D+"), ("A7/B7", "D−"), ("A5", "CC1"), ("B5", "CC2"), ("A1/B1/A12/B12", "GND"), ("SBU", "NC"), ("SH", "SHIELD")], color=AMBER, side="right")
    j2 = connector(70, 650, 205, 160, "J2", "B3B-PH 2.0 / 电池", [("1", "BATT+"), ("2", "BATT−"), ("3", "NTC")], color=AMBER, side="right")
    u1 = ic(430, 190, 330, 300, "U1", "BQ25895RTWR · C80200 · 0x6A", left=("VBUS", "BAT", "D+", "D−", "TS"), right=("SYS", "PMID", "OTG_EN", "SDA", "SCL"), top=("REGN", "BTST", "SW"), bottom=("GND", "ILIM", "STAT/INT"), color=AMBER)
    u2 = ic(900, 190, 255, 205, "U2", "TPS3424A11C13ADRLR", left=("VIN", "PB"), right=("PWR_LATCH", "INT_N"), bottom=("GND", "KILL"), color=AMBER)
    u3 = ic(1270, 190, 235, 205, "U3", "TPS22965DSGR · C122837", left=("IN", "ON"), right=("OUT",), bottom=("CT", "QOD", "GND"), color=AMBER)
    u4 = ic(1270, 520, 235, 205, "U4", "TLV62569DBVR · C141836", left=("VIN", "EN"), right=("SW", "FB"), bottom=("GND",), color=AMBER)
    u5 = ic(900, 540, 255, 190, "U5", "TPS22919DCKR · C2149796", left=("VIN", "ON"), right=("OUT",), bottom=("QOD", "GND"), color=AMBER)
    u6 = ic(430, 570, 330, 220, "U6", "FSW7227YMS10G/TR · C32713318", left=("COM_DP", "COM_DM"), right=("BQ_DP", "BQ_DM", "ESP_DP", "ESP_DM"), bottom=("SEL", "DSEL", "/EN", "VDD", "GND"), color=BLUE)
    b.extend([j1["svg"], j2["svg"], u1["svg"], u2["svg"], u3["svg"], u4["svg"], u5["svg"], u6["svg"]])
    f_in, f_a, f_z = resistor(350, 280, "F1", "2 A 输入保护候选", vertical=False); d_ovp, d_a, d_z = diode(350, 365, "D1", "USB TVS/OVP", vertical=True, color=RED); b.extend([f_in, d_ovp])
    # Wires and passives.
    b.extend([
        wire([j1["pins"]["VBUS"], (305, j1["pins"]["VBUS"][1]), f_a], color=AMBER, label="VBUS_RAW", label_at=(300, j1["pins"]["VBUS"][1]-6), arrow=False),
        wire([f_z, (404, f_z[1]), u1["pins"]["VBUS"]], color=AMBER, label="VBUS_5V", label_at=(390, f_z[1]-6), arrow=False),
        wire([(350, 280), (350, 320), d_a], color=RED, label="ESD/OVP", label_at=(375, 315), arrow=False), gnd(d_z[0], d_z[1]),
        wire([(350, 270), (350, 165), (790, 165), (790, u2["pins"]["VIN"][1])], color=AMBER, label="BAT_PROTECTED", label_at=(570, 158)),
        wire([j2["pins"]["BATT+"], (340, j2["pins"]["BATT+"][1]), (340, u1["pins"]["BAT"][1])], color=AMBER, label="BAT_PROTECTED", label_at=(345, 610)),
        wire([j2["pins"]["BATT−"], (315, j2["pins"]["BATT−"][1]), (315, 850)], color=WIRE, label="GND", label_at=(315, 838), arrow=False),
        wire([u1["pins"]["PMID"], (830, u1["pins"]["PMID"][1]), (830, u5["pins"]["VIN"][1])], color=AMBER, label="PMID_OTG_5V", label_at=(830, 505)),
        wire([u1["pins"]["SYS"], (790, u1["pins"]["SYS"][1]), (790, 470)], color=WIRE, dashed=True, label="BQ_SYS / TP", label_at=(790, 458), arrow=False),
        wire([u2["pins"]["PWR_LATCH"], (1210, u2["pins"]["PWR_LATCH"][1]), (1210, u3["pins"]["ON"][1])], color=AMBER, label="PWR_LATCH", label_at=(1210, 178)),
        wire([u3["pins"]["OUT"], (1550, u3["pins"]["OUT"][1])], color=AMBER, label="VBAT_SW", label_at=(1570, u3["pins"]["OUT"][1]-6)),
        wire([u4["pins"]["SW"], (1580, u4["pins"]["SW"][1])], color=AMBER, label="SW", label_at=(1555, u4["pins"]["SW"][1]-6), arrow=False),
        wire([u5["pins"]["OUT"], (1550, u5["pins"]["OUT"][1])], color=AMBER, label="AUDIO_5V", label_at=(1570, u5["pins"]["OUT"][1]-6)),
        wire([u6["pins"]["COM_DP"], (350, u6["pins"]["COM_DP"][1]), (350, j1["pins"]["D+"][1])], color=BLUE, label="USB_DP_C", label_at=(360, 520)),
        wire([u6["pins"]["COM_DM"], (335, u6["pins"]["COM_DM"][1]), (335, j1["pins"]["D−"][1])], color=BLUE, label="USB_DM_C", label_at=(350, 540)),
        wire([u6["pins"]["BQ_DP"], (805, u6["pins"]["BQ_DP"][1])], color=BLUE, label="USB_DP_BQ", label_at=(850, u6["pins"]["BQ_DP"][1]-6)),
        wire([u6["pins"]["BQ_DM"], (805, u6["pins"]["BQ_DM"][1])], color=BLUE, label="USB_DM_BQ", label_at=(850, u6["pins"]["BQ_DM"][1]-6)),
        wire([u6["pins"]["ESP_DP"], (805, u6["pins"]["ESP_DP"][1])], color=BLUE, label="USB_D+ → IO20", label_at=(875, u6["pins"]["ESP_DP"][1]-6)),
        wire([u6["pins"]["ESP_DM"], (805, u6["pins"]["ESP_DM"][1])], color=BLUE, label="USB_D− → IO19", label_at=(875, u6["pins"]["ESP_DM"][1]-6)),
    ])
    r1, a, z = resistor(310, 420, "R1", "5.1 kΩ", vertical=False); b.append(r1); b.append(wire([j1["pins"]["CC1"], (290, j1["pins"]["CC1"][1]), a], color=WIRE, arrow=False)); b.append(gnd(z[0], z[1]+28))
    r2, a, z = resistor(310, 470, "R2", "5.1 kΩ", vertical=False); b.append(r2); b.append(wire([j1["pins"]["CC2"], (290, j1["pins"]["CC2"][1]), a], color=WIRE, arrow=False)); b.append(gnd(z[0], z[1]+28))
    l1, a, z = inductor(1040, 450, "L1", "2.2 µH", vertical=False); b.append(l1); b.append(wire([u1["pins"]["SW"], (1040, u1["pins"]["SW"][1]), a], color=AMBER, arrow=False)); b.append(wire([z, (1200, z[1])], color=AMBER, label="SYS_PRE / BQ_SYS", label_at=(1120, z[1]-6), arrow=False))
    c1, a, z = capacitor(1190, 450, "C_SYS", "≈20 µF"); b.append(c1); b.append(wire([z, (1190, 505)], color=WIRE, arrow=False)); b.append(gnd(1190, 505))
    c2, a, z = capacitor(1210, 785, "C_A5", "60 µF"); b.append(c2); b.append(wire([u5["pins"]["OUT"], (1210, u5["pins"]["OUT"][1]), a], color=AMBER, arrow=False)); b.append(gnd(z[0], z[1]))
    l2, a, z = inductor(1640, u4["pins"]["SW"][1], "L2", "2.2 µH", vertical=False); b.append(l2); b.append(wire([(1580, u4["pins"]["SW"][1]), a], color=AMBER, arrow=False)); b.append(wire([z, (1740, z[1])], color=AMBER, label="3V3", label_at=(1770, z[1]-6), arrow=False))
    rfb1, a, z = resistor(1650, 760, "R_FB_TOP", "453 kΩ", vertical=True); rfb2, a2, z2 = resistor(1740, 760, "R_FB_BOT", "100 kΩ", vertical=True); b.extend([rfb1, rfb2, wire([(1740, u4["pins"]["FB"][1]), (1740, 715), a2], color=BLUE, label="3V3_FB", label_at=(1740, 700), arrow=False), wire([z2, (1740, 850)], color=WIRE, arrow=False), gnd(1740, 850), vcc(1650, 700, "3V3")])
    cin3, a, z = capacitor(1190, 680, "C_IN3V3", "10 µF"); b.extend([cin3, wire([u4["pins"]["VIN"], (1190, u4["pins"]["VIN"][1]), a], color=AMBER, arrow=False), gnd(z[0], z[1])])
    # BQ25895 的关键外围：用真实无源符号画出可调初值，网络名作为跨页核对点。
    r_ilim, a, z = resistor(810, 810, "R_ILIM", "180 Ω", vertical=False); b.extend([r_ilim, wire([u1["pins"]["ILIM"], (810, u1["pins"]["ILIM"][1]), a], color=AMBER, arrow=False), gnd(z[0], z[1]+22)])
    c_regn, a, z = capacitor(900, 810, "C_REGN", "4.7 µF"); c_btst, a2, z2 = capacitor(1020, 810, "C_BTST", "47 nF"); b.extend([c_regn, c_btst, wire([u1["pins"]["REGN"], (900, u1["pins"]["REGN"][1]), a], color=AMBER, arrow=False), wire([u1["pins"]["BTST"], (1020, u1["pins"]["BTST"][1]), a2], color=AMBER, arrow=False), gnd(z[0], z[1]), gnd(z2[0], z2[1])])
    r_ts1, a, z = resistor(300, 760, "R_TS_TOP", "5.23 kΩ", vertical=True); r_ts2, a2, z2 = resistor(390, 760, "R_TS_BOT", "30.1 kΩ", vertical=True); b.extend([r_ts1, r_ts2, vcc(300, 680, "REGN/TS_REF"), gnd(390, 840), wire([z, (345,760), a2], color=AMBER, label="TS", label_at=(345,745), arrow=False)])
    ntc, na, nz = resistor(480, 760, "NTC", "10 kΩ", vertical=True); b.extend([ntc, wire([(345,760),(480,760),na], color=AMBER, arrow=False), gnd(nz[0], nz[1])])
    c_vbus, a, z = capacitor(315, 540, "C_VBUS", "1 µF"); c_bat, a2, z2 = capacitor(315, 620, "C_BAT", "10 µF"); b.extend([c_vbus,c_bat,gnd(z[0],z[1]),gnd(z2[0],z2[1]),wire([(315,495),a],color=AMBER,arrow=False),wire([(315,575),a2],color=AMBER,arrow=False),vcc(315,495,"VBUS_5V"),vcc(315,575,"BAT_PROTECTED")])
    jp_bq, ja, jz = resistor(810, 770, "JP_BQ", "0 Ω / FORCE_BQ", vertical=False); jp_esp, ja2, jz2 = resistor(1010, 770, "JP_ESP", "0 Ω / FORCE_ESP", vertical=False); b.extend([jp_bq, jp_esp, wire([u6["pins"]["SEL"], (810, u6["pins"]["SEL"][1]), ja], color=BLUE, label="USB_MUX_SEL", label_at=(760, u6["pins"]["SEL"][1]-6), arrow=False), wire([u6["pins"]["DSEL"], (1010, u6["pins"]["DSEL"][1]), ja2], color=BLUE, label="USB_MUX_DSEL", label_at=(950, u6["pins"]["DSEL"][1]-6), arrow=False), net_label(jz[0]+8, jz[1], "FORCE_BQ", color=BLUE), net_label(jz2[0]+8, jz2[1], "FORCE_ESP", color=BLUE)])
    b.append(note(50, 880, 1650, 85, "推荐初值", ["BQ：RILIM≈180 Ω（约 2 A 输入限流）、ICHG≈1.5 A、TS=5.23 kΩ/30.1 kΩ + 10 kΩ NTC；U4：L=2.2 µH、FB=453 kΩ/100 kΩ；所有值在样机前保持可调。"], color=AMBER))
    b.extend([tp(250, 930, "TP_VBUS", "VBUS_5V"), tp(420, 930, "TP_BAT", "BAT_PROTECTED"), tp(600, 930, "TP_VBAT", "VBAT_SW"), tp(800, 930, "TP_3V3", "3V3"), tp(1000, 930, "TP_A5", "AUDIO_5V")])
    return sheet(1, "电源 / 充电 / USB 数据选择", "J1 单 USB-C · U1 BQ25895 · U2/U3 硬件锁存 · U4/U5 分轨 · U6 FSW7227", "".join(b), footer_note="电源路径和 USB 选择是首要人工复刻对象；不要把 BQ_SYS 画到 M100。")


def p02_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 72, "GPIO 合同", ["下列引脚名就是跨页网络标签。IO0/IO46 为启动绑带输入，EN 是独立复位；IO35–37、IO45 和模组保留脚不放置普通外设。"], color=BLUE))
    u = ic(520, 190, 700, 610, "U7", "ESP32-S3-WROOM-1-N16R8 · C2913202", left=("IO0 BOOT0", "IO1 I2C_SDA", "IO2 I2C_SCL", "IO3 RGB_DATA", "IO4 LCD_RST", "IO5 LCD_SCLK", "IO6 LCD_D0", "IO7 LCD_D1", "IO8 BQ_OTG_EN", "IO9 PVDF_ADC", "IO10 M100_DTR", "IO11 PVDF_WAKE", "IO12 LCD_D2", "IO13 I2S_DOUT", "IO14 I2S_WS", "IO15 M100_RST"), right=("IO16 NET_STATUS_COMPAT", "IO17 I2S_DIN", "IO18 I2S_BCLK", "IO19 USB_D−", "IO20 USB_D+", "IO21 I2S_MCLK", "IO38 TP_RST", "IO39 TP_INT", "IO40 LCD_CS", "IO41 IMU_INT1", "IO42 LCD_D3", "IO43 M100_TXD", "IO44 M100_RXD", "IO45 NC", "IO46 PWR_STATE", "IO47 LCD_EN", "IO48 PA_EN"), top=("3V3", "GND"), bottom=("EN RESET_N", "USB_SHIELD"), color=BLUE)
    b.append(u["svg"])
    # Standard switch, pull-up, and off-page labels.
    sw = connector(70, 215, 210, 150, "SW1", "BOOT / RESET", [("1", "BOOT0"), ("2", "RESET_N"), ("3", "GND")], color=AMBER, side="right"); b.append(sw["svg"])
    r, a, z = resistor(350, 275, "R_BOOT", "10 kΩ", vertical=False); b.append(r); b.append(wire([sw["pins"]["BOOT0"], (300, sw["pins"]["BOOT0"][1]), a], color=WIRE, arrow=False)); b.append(vcc(z[0], z[1]-25, "3V3"))
    b.append(wire([sw["pins"]["RESET_N"], (300, sw["pins"]["RESET_N"][1]), (440, sw["pins"]["RESET_N"][1])], color=BLUE, label="EN / RESET_N", label_at=(370, sw["pins"]["RESET_N"][1]-6), arrow=False))
    for idx, name in enumerate(["USB_D−", "USB_D+", "I2C_SDA", "I2C_SCL", "I2S_DOUT", "I2S_WS", "I2S_DIN", "I2S_BCLK", "I2S_MCLK", "LCD_QSPI", "PVDF_ADC", "PVDF_WAKE", "M100_UART"]):
        y = 185 + idx * 54
        b.append(net_label(1310, y, name, color=BLUE))
        b.append(wire([(1220, y), (1300, y)], color=BLUE, arrow=True))
    b.append(note(70, 520, 380, 220, "启动/复位落图", ["BOOT0(IO0) 下载时拉低", "PWR_STATE(IO46) 只读", "EN 独立复位，不能用软件关机替代", "USB-Serial-JTAG：IO19=D−、IO20=D+", "IO8 只去 BQ_OTG_EN，不去 GNSS_VCC"], color=BLUE))
    b.append(note(70, 780, 1420, 100, "手工检查", ["在 EDA 中使用真实 WROOM 符号后，将每一根外部线替换为全局标签；对照右侧 GPIO 表逐行核对，任何重复 GPIO 或使用保留脚都必须停止。"], color=RED))
    return sheet(2, "ESP32-S3 主控 / 启动复位 / USB", "U7 全 GPIO 合同；外设通过全局网络标签跨页连接", "".join(b), footer_note="GPIO 合同来自架构 spine；本页不伪造模组内部封装焊盘编号。")


def p03_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 70, "FPC 方向门禁", ["J3 采用 FH34SRJ-24S-0.5SH(50) / C324726 作为双面接触候选；实际 pin 1、接触面和第 8 脚 VBAT 必须拿到实物后复核。"], color=RED))
    fpc_sources = [(1,"TP-VDD_3V","3V3"),(2,"TP-SCL_3V","I2C_SCL"),(3,"TP-SDA_3V","I2C_SDA"),(4,"TP-INT_3V","TP_INT"),(5,"TP-RST_3V","TP_RST"),(6,"TP-GND","GND"),(7,"VDD_3V","3V3"),(8,"VBAT","LCD_VBAT=VBAT_SW"),(9,"MTP/NC","NC"),(10,"VDD_3V","3V3"),(11,"GND","GND"),(12,"L_RES","LCD_RST"),(13,"GND","GND"),(14,"L_IO0","LCD_D0"),(15,"GND","GND"),(16,"L_SCLK","LCD_SCLK"),(17,"GND","GND"),(18,"L_IO3","LCD_D3"),(19,"NC","NC"),(20,"L_IO2","LCD_D2"),(21,"CS","LCD_CS"),(22,"L_IO1","LCD_D1"),(23,"OLED_EN","LCD_EN"),(24,"LTE/TE","LCD_TE/TP")]
    pins = [(str(number), source) for number, source, _ in fpc_sources]
    # Connector on left; split labels into two columns for readability.
    j = connector(70, 185, 310, 700, "J3", "PY206-W38-V2 24P FPC", pins, color=AMBER, side="right"); b.append(j["svg"])
    u9 = ic(540, 220, 330, 220, "U9", "CO5300AF-51（模组内）", left=("LCD_RST", "LCD_CS", "LCD_SCLK", "LCD_D0", "LCD_D1", "LCD_D2", "LCD_D3", "LCD_EN"), right=("3V3", "LCD_VBAT", "GND"), color=GREEN, dashed=True)
    u10 = ic(540, 570, 330, 190, "U10", "CST9217（模组内）", left=("I2C_SDA", "I2C_SCL", "TP_RST", "TP_INT"), right=("3V3", "GND"), color=GREEN, dashed=True)
    b.extend([u9["svg"], u10["svg"]])
    # 以分开的短引线和全局网络标号表达代表性连接，避免不同 FPC 针脚互相穿越。
    for source, target, net in [("L_RES", "LCD_RST", "LCD_RST"), ("CS", "LCD_CS", "LCD_CS"), ("TP-SCL_3V", "I2C_SCL", "I2C_SCL"), ("TP-SDA_3V", "I2C_SDA", "I2C_SDA")]:
        sx, sy = j["pins"][source]; txp, typ = (u9 if target.startswith("LCD_") else u10)["pins"][target]
        b.append(wire([(sx, sy), (470, sy)], color=BLUE, arrow=False)); b.append(net_label(475, sy, net, color=BLUE))
        b.append(wire([(txp, typ), (900, typ)], color=BLUE, arrow=False)); b.append(net_label(905, typ, net, color=BLUE))
    b.append(note(960, 190, 700, 300, "显示 QSPI 网络", ["FPC12 L_RES → LCD_RST → IO4", "FPC14 L_IO0 → LCD_D0 → IO6", "FPC16 L_SCLK → LCD_SCLK → IO5", "FPC18 L_IO3 → LCD_D3 → IO42", "FPC20 L_IO2 → LCD_D2 → IO12", "FPC21 CS → LCD_CS → IO40", "FPC22 L_IO1 → LCD_D1 → IO7", "FPC23 OLED_EN → LCD_EN → IO47"], color=BLUE))
    b.append(note(960, 520, 700, 170, "触摸 / 供电", ["FPC1/7/10 = 3V3；FPC6/11/13/15/17 = GND", "FPC2/3 = I2C_SCL/SDA；FPC4/5 = TP_INT/TP_RST", "FPC8 = LCD_VBAT，首版接 VBAT_SW；FPC9/19 = NC", "FPC24 = LCD_TE，留 TP/NC"], color=GREEN))
    b.append(note(960, 740, 700, 120, "模组集成边界", ["OCP21351 已集成在屏模组内，主板不重复放置；CO5300/CST9217 的具体物理焊盘不在主板 BOM。"], color=RED))
    return sheet(3, "PY206-W38-V2 显示与触摸 FPC", "J3 24-pin / 0.5 mm；CO5300、CST9217、OCP21351 按模组内集成处理", "".join(b), footer_note="FPC 表是本页的针脚真源；任何方向疑问先停在 J3，不要试插带电。")


def p04_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 70, "音频电源与 BTL", ["U12 VCC 来自 AUDIO_5V，PA_EN 只有在电源稳定后才拉高；OUTP/OUTN 驱动 4 Ω/3 W 扬声器，SPK− 不接地。"], color=AMBER))
    u11 = ic(450, 200, 340, 270, "U11", "ES8311 · C962342 · I²C 0x18 候选", left=("SDA", "SCL", "I2S_DOUT", "I2S_WS", "I2S_DIN", "I2S_BCLK", "I2S_MCLK"), right=("OUTP", "OUTN"), top=("3V3", "AGND"), bottom=("PVDD", "DVDD", "AVDD"), color=GREEN)
    u12 = ic(1000, 200, 300, 270, "U12", "NS4150B · C189961", left=("INP", "INN", "PA_EN"), right=("OUTP", "OUTN"), top=("AUDIO_5V",), bottom=("GND", "BYPASS"), color=GREEN)
    spk = connector(1450, 260, 220, 150, "J5", "扬声器 4 Ω / 3 W", [("1", "SPK+"), ("2", "SPK−")], color=AMBER, side="left")
    led1 = led(1120, 690, "LED1", "WS2812C-2020-V1 · 5 V")
    b.extend([u11["svg"], u12["svg"], spk["svg"], led1[0]])
    b.extend([
        wire([u11["pins"]["OUTP"], (900, u11["pins"]["OUTP"][1]), u12["pins"]["INP"]], color=BLUE, label="CODEC_OUT_P", label_at=(950, u12["pins"]["INP"][1]-6)),
        wire([u11["pins"]["OUTN"], (900, u11["pins"]["OUTN"][1]), u12["pins"]["INN"]], color=BLUE, label="CODEC_OUT_N", label_at=(950, u12["pins"]["INN"][1]-6)),
        wire([u12["pins"]["OUTP"], (1370, u12["pins"]["OUTP"][1]), spk["pins"]["SPK+"]], color=BLUE, label="SPK_P", label_at=(1400, spk["pins"]["SPK+"][1]-6)),
        wire([u12["pins"]["OUTN"], (1370, u12["pins"]["OUTN"][1]), spk["pins"]["SPK−"]], color=BLUE, label="SPK_N", label_at=(1400, spk["pins"]["SPK−"][1]-6)),
        wire([u12["pins"]["PA_EN"], (900, u12["pins"]["PA_EN"][1]), (900, 590)], color=BLUE, label="PA_EN ← IO48", label_at=(900, 575), arrow=False),
        wire([(950, 690), (1075, 690)], color=BLUE, label="RGB_DATA ← IO3", label_at=(1010, 680)),
    ])
    # Coupling capacitors and PA pull-down.
    for x, ref, val, y in [(850, "C14", "1 µF", 315), (850, "C15", "1 µF", 385), (930, "R7/R8", "0 Ω", 315)]:
        c, a, z = capacitor(x, y, ref, val, vertical=False); b.append(c)
    r, a, z = resistor(820, 590, "R_PA_EN", "100 kΩ", vertical=False); b.append(r); b.append(gnd(z[0]+20, z[1]+30))
    r, a, z = resistor(900, 690, "R_DATA", "330 Ω", vertical=False); b.append(r)
    b.append(note(60, 560, 330, 235, "ES8311 去耦/配置", ["PVDD/DVDD：100 nF 各一", "AVDD：1 µF", "参考端：1 µF 各一", "I²S 22 pF 仅 DNP 料位", "SDA/SCL 上拉 4.7 kΩ → 3V3", "地址/CE-CDAT 与主从模式待测"], color=GREEN))
    b.append(note(60, 830, 1610, 80, "NS4150B / RGB", ["NS4150B：VCC 1 µF + 22 µF + 22 µF，BYPASS 1 µF，输入 100 nF + 100 kΩ；WS2812C VDD=AUDIO_5V，DI 串约 330 Ω并默认下拉。"], color=AMBER))
    return sheet(4, "ES8311 / NS4150B / 扬声器 / RGB", "I²C + I²S codec、差分耦合、BTL 功放和 5 V 状态灯", "".join(b), footer_note="音频参考值来自用户参考图的子电路并按 EWF 电源树改写；不复制旧 GPIO/5 V 4G 供电。")


def p05_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 70, "PVDF 保护优先级", ["LDT0-028K 的压电瞬态可能很高；1 MΩ 限流、低漏钳位和 10 MΩ 泄放先保证 ADC 安全，再调阈值/滞回。IO11 只唤醒，IO9 ADC 二次确认。"], color=RED))
    j = connector(70, 280, 190, 150, "J6/S1", "LDT0-028K", [("1", "PVDF+"), ("2", "PVDF−")], color=AMBER, side="right")
    b.append(j["svg"])
    r, a, z = resistor(360, 355, "R_PVDF_SER", "1 MΩ", vertical=False); b.append(r)
    b.append(wire([j["pins"]["PVDF+"], (315, j["pins"]["PVDF+"][1]), a], color=BLUE, label="PVDF_RAW", label_at=(315, 340), arrow=False))
    b.append(wire([z, (520, z[1])], color=BLUE, label="PVDF_COND", label_at=(470, 340), arrow=False))
    # Clamp diodes to rails.
    d1, a1, z1 = diode(580, 300, "D_CL+", "低漏钳位", vertical=True, color=RED); d2, a2, z2 = diode(650, 410, "D_CL−", "低漏钳位", vertical=True, color=RED); b.extend([d1, d2]); b.extend([wire([(520,355),(580,355),(580,345)], color=RED, arrow=False), wire([(520,355),(650,355),(650,365)], color=RED, arrow=False), vcc(580,240,"3V3"), gnd(650,455)])
    r3, a3, z3 = resistor(720, 355, "R_BIAS", "10 MΩ", vertical=False); b.append(r3); b.append(wire([(520,355),(675,355),a3], color=BLUE, arrow=False)); b.append(vcc(z3[0], z3[1]-30,"VBIAS≈1.65V"))
    op = opamp(900, 355, "U15", "TLV2369IDGKR", comparator=False); b.append(op["svg"])
    b.extend([wire([(765,355), op["in_plus"]], color=BLUE, label="PVDF_COND", label_at=(830,340), arrow=False), wire([op["out"], (1030,op["out"][1])], color=BLUE, label="PVDF_BUF", label_at=(1000,340), arrow=False), wire([op["out"], (800,op["out"][1]), (800,op["in_minus"][1]), op["in_minus"]], color=GREEN, label="电压跟随反馈", label_at=(800,op["in_minus"][1]-8), arrow=False), vcc(op["vcc"][0],op["vcc"][1],"3V3"), gnd(op["gnd"][0],op["gnd"][1])])
    # ADC branch and comparator branch.
    r4, a4, z4 = resistor(1120, 300, "R_ADC", "1 kΩ", vertical=False); c4, a5, z5 = capacitor(1235, 390, "C_ADC", "100 nF"); b.extend([r4,c4]); b.extend([wire([op["out"], (1075,op["out"][1]), a4], color=BLUE, arrow=False), wire([z4, (1180,300),(1180,390),a5], color=BLUE, arrow=False), gnd(z5[0],z5[1]), wire([(1280,390),(1430,390)], color=BLUE, label="PVDF_ADC → IO9", label_at=(1360,375), arrow=True)])
    comp = opamp(900, 620, "U16", "TLV7042 双窗比较器", comparator=True); b.append(comp["svg"]); b.extend([wire([op["out"],(820,op["out"][1]),(820,592),comp["in_plus"]], color=BLUE, label="PVDF_BUF", label_at=(820,575), arrow=False), wire([(1315,730),(1080,730),(1080,648),comp["in_minus"]], color=GREEN, label="VBIAS/阈值", label_at=(1110,716), arrow=False), wire([comp["out"],(1060,comp["out"][1]),(1430,620)], color=RED, label="PVDF_CMP_WAKE → IO11", label_at=(1260,605), arrow=True), vcc(comp["vcc"][0],comp["vcc"][1],"3V3"), gnd(comp["gnd"][0],comp["gnd"][1])])
    # VBIAS divider.
    r5,a5,z5=resistor(1260,730,"R_VBIAS_TOP","10 kΩ",vertical=True); r6,a6,z6=resistor(1370,730,"R_VBIAS_BOT","10 kΩ",vertical=True); b.extend([r5,r6,vcc(1260,655,"3V3"),gnd(1370,790),wire([z5,(1315,730),a6],color=BLUE,label="VBIAS≈1.65V",label_at=(1315,715),arrow=False)])
    b.extend([tp(285, 530, "TP_RAW", "PVDF_RAW"), tp(470, 530, "TP_ADC", "PVDF_ADC"), tp(655, 530, "TP_CMP", "PVDF_CMP_WAKE")])
    b.append(note(60, 800, 1610, 100, "必须样机验证", ["比较器具体封装/输出形式、钳位型号与漏电、阈值约 1.65 V ±150 mV、滞回、消抖和 1–20 次/秒输入都只是可调初值；不得把原始波形保存或上传。"], color=RED))
    return sheet(5, "PVDF 敲击模拟前端 / ADC / 唤醒", "LDT0-028K · 1 MΩ 限流 · 低漏钳位 · TLV2369 · TLV7042", "".join(b), footer_note="先验证 ADC 输入安全范围，再调整比较器阈值；IO11 为唤醒而非计数事实源。")


def p06_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 70, "4G 供电与控制", ["M100 VIN 只能接 VBAT_SW（3.3–4.2 V，高电流）；不要从 BQ_SYS 或 AUDIO_5V 供电。DTR/RST 采用开漏候选并保留极性替代焊盘。"], color=RED))
    j = connector(70, 185, 330, 610, "J4/U8", "M100EG-C2 / Air780EGP", [("VIN", "M100_VIN"), ("GND", "GND"), ("TXD", "M100_TXD"), ("RXD", "M100_RXD"), ("DTR", "M100_DTR"), ("RST", "M100_RST"), ("NET", "NET_STATUS_COMPAT"), ("GNSS", "GNSS_VCC_NC"), ("USB", "USB_TEST"), ("SIM/ANT", "M100_SIM/ANT")], color=AMBER, side="right"); b.append(j["svg"])
    # Pull-up + open-drain transistor stages.
    q1 = ic(600, 235, 250, 150, "Q1", "DTR 开漏候选", left=("G",), right=("D",), bottom=("S",), color=GREEN)
    q2 = ic(600, 485, 250, 150, "Q2", "RST 开漏候选", left=("G",), right=("D",), bottom=("S",), color=GREEN)
    b.extend([q1["svg"],q2["svg"]])
    r1,a,z=resistor(520,210,"R_DTR_PU","100 kΩ",vertical=True);r2,a2,z2=resistor(520,460,"R_RST_PU","100 kΩ",vertical=True);b.extend([r1,r2,vcc(520,150,"VBAT_SW"),vcc(520,400,"VBAT_SW"),wire([z,(650,210),q1["pins"]["D"]],color=BLUE,label="M100_DTR",label_at=(600,195),arrow=False),wire([z2,(650,460),q2["pins"]["D"]],color=BLUE,label="M100_RST",label_at=(600,445),arrow=False),wire([q1["pins"]["G"],(500,q1["pins"]["G"][1]),(500,300)],color=BLUE,label="IO10 DTR",label_at=(500,285),arrow=False),wire([q2["pins"]["G"],(500,q2["pins"]["G"][1]),(500,550)],color=BLUE,label="IO15 RST",label_at=(500,535),arrow=False),gnd(q1["pins"]["S"][0],q1["pins"]["S"][1]+20),gnd(q2["pins"]["S"][0],q2["pins"]["S"][1]+20)])
    # 载板引脚采用短引线 + 网络标号，避免把不同功能线画成交叉长线。
    for key, label_text, color, dashed in [
        ("M100_VIN", "VBAT_SW", AMBER, False),
        ("M100_TXD", "M100_RXD → IO44", BLUE, False),
        ("M100_RXD", "M100_TXD ← IO43", BLUE, False),
        ("M100_DTR", "M100_DTR ← IO10", BLUE, False),
        ("M100_RST", "M100_RST ← IO15", BLUE, False),
        ("NET_STATUS_COMPAT", "IO16 compat / TP / NC", RED, True),
        ("GNSS_VCC_NC", "GNSS_VCC_NC / 不接 IO8", RED, True),
    ]:
        px, py = j["pins"][key]
        b.append(wire([(px, py), (470, py)], color=color, dashed=dashed, arrow=False, label=label_text, label_at=(445, py - 6)))
    b.extend([wire([q1["pins"]["D"], (900, q1["pins"]["D"][1])], color=BLUE, label="M100_DTR", label_at=(860, q1["pins"]["D"][1]-6), arrow=False), wire([q2["pins"]["D"], (900, q2["pins"]["D"][1])], color=BLUE, label="M100_RST", label_at=(860, q2["pins"]["D"][1]-6), arrow=False), wire([q1["pins"]["G"], (470, q1["pins"]["G"][1])], color=BLUE, label="IO10", label_at=(450, q1["pins"]["G"][1]-6), arrow=False), wire([q2["pins"]["G"], (470, q2["pins"]["G"][1])], color=BLUE, label="IO15", label_at=(450, q2["pins"]["G"][1]-6), arrow=False)])
    b.append(note(980, 180, 690, 210, "载板前排逻辑（资料页顺序）", ["VIN · GND · RXD · TXD · DTR · RST · RI · 1PPS", "TXD/RXD 与 ESP32 交叉：IO44 接收、IO43 发送", "RI / 1PPS 仅测试点，不占普通 GPIO", "RST 先按低有效方式落图，保留反相替代焊盘"], color=BLUE))
    b.append(note(980, 450, 690, 180, "载板 USB / SIM / ANT", ["载板 USB（VB/DM/DP/GND）不并入主 USB，只留 TP", "SIM、天线匹配和 ESD 由载板版本/机械资料确认", "真实 header pin number、间距和方向必须实物核对"], color=RED))
    b.append(note(980, 690, 690, 135, "main_control 旧宏仅作交叉", ["旧 GPIO：DTR=7、RST=15、NET_STATUS=16、UART=17/18、GNSS_VCC=8。EWF 实际为 IO10/15/16/43/44；IO8 已给 BQ_OTG_EN。"], color=RED))
    b.extend([tp(430,900,"TP_VBAT","VBAT_SW"),tp(620,900,"TP_DTR","M100_DTR"),tp(800,900,"TP_RST","M100_RST"),tp(980,900,"TP_NET","NET_STATUS_COMPAT"),tp(1200,900,"TP_GNSS","GNSS_VCC_NC")])
    return sheet(6, "M100EG-C2 / Air780EGP 4G 载板", "VBAT_SW 供电 · UART 交叉 · DTR/RST 开漏候选 · 兼容位与测试点", "".join(b), footer_note="载板物理针脚与电平仍未闭合；本页逻辑交叉表不能替代载板版本图纸。")


def p07_standard() -> str:
    b: list[str] = []
    b.append(note(48, 92, 1700, 70, "I²C 地址与上拉", ["I2C0：SDA=IO1、SCL=IO2，统一 3V3 上拉 4.7 kΩ。首版地址 BQ=0x6A、CW2015=0x62、CST9217=0x5A、ES8311=0x18、QMI8658C=0x6B，必须上电扫描。"], color=BLUE))
    # Pullups and bus.
    r1,a,z=resistor(260,260,"R_SDA","4.7 kΩ",vertical=True);r2,a2,z2=resistor(360,260,"R_SCL","4.7 kΩ",vertical=True);b.extend([r1,r2,vcc(260,190,"3V3"),vcc(360,190,"3V3"),wire([z,(260,470)],color=BLUE,label="I2C_SDA / IO1",label_at=(260,455),arrow=False),wire([z2,(360,470)],color=BLUE,label="I2C_SCL / IO2",label_at=(360,455),arrow=False)])
    u13=ic(520,220,340,220,"U13","CW2015 · 地址 0x62 候选",left=("SDA","SCL"),right=("3V3","GND"),color=GREEN);u14=ic(1020,220,340,260,"U14","QMI8658C · SA0=高 · 0x6B",left=("SDA","SCL","SA0"),right=("INT1","INT2"),top=("3V3",),bottom=("GND",),color=GREEN);b.extend([u13["svg"],u14["svg"]])
    b.extend([wire([(260,470),(470,470),(470,u13["pins"]["SDA"][1]),u13["pins"]["SDA"]],color=BLUE,arrow=False),wire([(360,470),(450,470),(450,u13["pins"]["SCL"][1]),u13["pins"]["SCL"]],color=BLUE,arrow=False),wire([(260,470),(930,470),(930,u14["pins"]["SDA"][1]),u14["pins"]["SDA"]],color=BLUE,label="共享 SDA",label_at=(700,455),arrow=False),wire([(360,470),(900,470),(900,u14["pins"]["SCL"][1]),u14["pins"]["SCL"]],color=BLUE,label="共享 SCL",label_at=(700,490),arrow=False),wire([u14["pins"]["SA0"],(980,u14["pins"]["SA0"][1]),(980,170)],color=BLUE,label="SA0=高",label_at=(950,155),arrow=False),wire([u14["pins"]["INT1"],(1430,u14["pins"]["INT1"][1])],color=BLUE,label="IMU_INT1 → IO41",label_at=(1510,u14["pins"]["INT1"][1]-6),arrow=True),wire([u14["pins"]["INT2"],(1430,u14["pins"]["INT2"][1])],color=RED,label="NC / 不占 IO45",label_at=(1510,u14["pins"]["INT2"][1]-6),dashed=True,arrow=False)])
    b.append(note(520, 560, 840, 190, "同一总线上的其他器件", ["U1 BQ25895：0x6A（P01）", "U10 CST9217：0x5A（P03）", "U11 ES8311：0x18（P04）", "地址冲突时先改硬件基线/spine，再改固件；不要静默兼容多个地址"], color=BLUE))
    b.append(note(60, 560, 390, 190, "测试点", ["TP_I2C_SDA", "TP_I2C_SCL", "TP_IMU_INT1", "上电扫描日志必须保留", "QMI8658C MVP 默认不启用，仅首版贴装"] , color=GREEN))
    b.append(note(60, 810, 1610, 80, "封装提醒", ["CW2015/QMI8658C 的具体封装、SA0/INT 电平和去耦按实际采购批次核对；图中地址是首版合同，不是样机 ACK 结果。"], color=RED))
    return sheet(7, "共享 I²C / CW2015 / QMI8658C", "I2C0 总线、4.7 kΩ 上拉、地址表与 INT1/INT2 处理", "".join(b), footer_note="共享总线地址必须唯一；先扫描再冻结驱动配置。")


def standard_svgs() -> list[str]:
    return [p01_standard(), p02_standard(), p03_standard(), p04_standard(), p05_standard(), p06_standard(), p07_standard()]
