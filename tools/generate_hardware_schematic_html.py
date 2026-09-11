#!/usr/bin/env python3
"""生成电子木鱼离线硬件原理图复刻 HTML 与 SVG 图集。

输入只取 docs/hardware/电子木鱼-硬件网络清单.json；图形是给人工复刻的
可视化层，不宣称嘉立创 EDA ERC、PCB DRC 或样机验证通过。
"""

from __future__ import annotations

import html
import json
import re
from pathlib import Path
from typing import Iterable

from hardware_schematic_standard import defs as standard_defs
from hardware_schematic_standard import standard_svgs


ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = ROOT / "docs" / "hardware" / "电子木鱼-硬件网络清单.json"
OUT_DIR = ROOT / "docs" / "hardware" / "lceda"
PAGES_DIR = OUT_DIR / "pages"
HTML_PATH = OUT_DIR / "电子木鱼-完整原理图复刻.html"
SVG_PATH = OUT_DIR / "电子木鱼-完整原理图图集.svg"

COLORS = {
    "power": "#fbbf24",
    "signal": "#22d3ee",
    "peripheral": "#34d399",
    "connector": "#fb923c",
    "uncertain": "#fb7185",
    "neutral": "#94a3b8",
    "white": "#f8fafc",
    "grid": "#1e293b",
    "bg": "#0f172a",
}

STATUS_LABELS = {
    "confirmed": ("已确认", "confirmed"),
    "recommended_initial": ("推荐初值", "recommended"),
    "must_prototype_validate": ("必须样机验证", "prototype"),
    "pages_created_blank_manual_html": ("页树已建·手工复刻", "recommended"),
    "baseline_not_prototype_verified": ("基线未样机验证", "prototype"),
}


def esc(value: object) -> str:
    return html.escape(str(value), quote=True)


def status_label(status: str | None) -> tuple[str, str]:
    return STATUS_LABELS.get(status or "", (status or "未标注", "neutral"))


def badge_html(status: str | None) -> str:
    label, cls = status_label(status)
    return f'<span class="badge {cls}">{esc(label)}</span>'


def text_lines(x: float, y: float, lines: Iterable[str], *, size: int = 12,
               color: str = COLORS["white"], weight: str = "400",
               anchor: str = "start", line_gap: int | None = None) -> str:
    gap = line_gap or int(size * 1.35)
    lines = list(lines)
    if not lines:
        return ""
    spans = []
    for idx, line in enumerate(lines):
        dy = 0 if idx == 0 else gap
        spans.append(f'<tspan x="{x:g}" dy="{dy:g}">{esc(line)}</tspan>')
    return (f'<text x="{x:g}" y="{y:g}" fill="{color}" font-size="{size}px" '
            f'font-weight="{weight}" text-anchor="{anchor}">{"".join(spans)}</text>')


def wrap(value: str, width: int = 32) -> list[str]:
    """按字符宽度做稳定的中文/ASCII 混合换行。"""
    result: list[str] = []
    current = ""
    for char in str(value):
        if char == "\n":
            result.append(current)
            current = ""
            continue
        weight = 2 if ord(char) > 127 else 1
        current_weight = sum(2 if ord(c) > 127 else 1 for c in current)
        if current and current_weight + weight > width:
            result.append(current)
            current = ""
        current += char
    if current or not result:
        result.append(current)
    return result


def svg_defs() -> str:
    return f"""<defs>
      <pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse">
        <path d="M 40 0 L 0 0 0 40" fill="none" stroke="{COLORS['grid']}" stroke-width="0.6"/>
      </pattern>
      <marker id="arrow" markerWidth="10" markerHeight="7" refX="9" refY="3.5" orient="auto">
        <polygon points="0 0, 10 3.5, 0 7" fill="{COLORS['neutral']}"/>
      </marker>
      <marker id="arrow-power" markerWidth="10" markerHeight="7" refX="9" refY="3.5" orient="auto">
        <polygon points="0 0, 10 3.5, 0 7" fill="{COLORS['power']}"/>
      </marker>
      <marker id="arrow-signal" markerWidth="10" markerHeight="7" refX="9" refY="3.5" orient="auto">
        <polygon points="0 0, 10 3.5, 0 7" fill="{COLORS['signal']}"/>
      </marker>
      <filter id="shadow" x="-20%" y="-20%" width="140%" height="140%">
        <feDropShadow dx="0" dy="3" stdDeviation="3" flood-color="#000" flood-opacity="0.35"/>
      </filter>
    </defs>"""


def box(x: float, y: float, w: float, h: float, title: str, subtitle: str = "",
        *, color: str = COLORS["signal"], status: str = "confirmed",
        lines: Iterable[str] = (), ref: str = "") -> str:
    label, _ = status_label(status)
    body = list(lines)
    parts = [
        f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" rx="10" fill="{COLORS["bg"]}" filter="url(#shadow)"/>',
        f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" rx="10" fill="{color}" fill-opacity="0.16" stroke="{color}" stroke-width="2"/>',
        f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="6" rx="3" fill="{color}"/>',
        text_lines(x + 14, y + 28, [title], size=16, weight="700"),
    ]
    if ref:
        parts.append(text_lines(x + w - 14, y + 27, [ref], size=11, color=color, weight="700", anchor="end"))
    if subtitle:
        parts.append(text_lines(x + 14, y + 48, wrap(subtitle, max(20, int(w / 7))), size=10, color="#cbd5e1"))
    parts.append(f'<rect x="{x+w-14-len(label)*7:g}" y="{y+h-25:g}" width="{max(54,len(label)*7+14):g}" height="17" rx="8" fill="{color}" fill-opacity="0.18"/>')
    parts.append(text_lines(x + w - 14, y + h - 13, [label], size=9, color=color, anchor="end"))
    if body:
        start_y = y + 70
        for idx, line in enumerate(body):
            if start_y + idx * 18 > y + h - 34:
                break
            parts.append(text_lines(x + 14, start_y + idx * 18, wrap(line, max(18, int((w-28)/7))), size=10, color="#dbeafe"))
    return "".join(parts)


def pill(x: float, y: float, value: str, *, color: str = COLORS["signal"], width: float | None = None) -> str:
    w = width or max(80, len(value) * 8 + 22)
    return (f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="24" rx="12" fill="{color}" fill-opacity="0.15" stroke="{color}" stroke-width="1"/>'
            + text_lines(x + w / 2, y + 16, [value], size=10, color=color, weight="600", anchor="middle"))


def wire(x1: float, y1: float, x2: float, y2: float, *, color: str = COLORS["neutral"], label: str = "", dashed: bool = False, arrow: bool = True) -> str:
    dash = ' stroke-dasharray="7 5"' if dashed else ""
    marker = f' marker-end="url(#{"arrow-power" if color == COLORS["power"] else "arrow-signal" if color == COLORS["signal"] else "arrow"})"' if arrow else ""
    result = f'<path d="M {x1:g} {y1:g} L {x2:g} {y2:g}" fill="none" stroke="{color}" stroke-width="2"{dash}{marker}/>'
    if label:
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2 - 7
        result += text_lines(mx, my, [label], size=10, color=color, weight="600", anchor="middle")
    return result


def bus(x: float, y: float, w: float, name: str, *, color: str = COLORS["signal"], note: str = "") -> str:
    parts = [f'<rect x="{x:g}" y="{y-12:g}" width="{w:g}" height="24" rx="12" fill="{color}" fill-opacity="0.10" stroke="{color}" stroke-width="1.5"/>',
             text_lines(x + 12, y + 4, [name], size=11, color=color, weight="700")]
    if note:
        parts.append(text_lines(x + w - 12, y + 4, [note], size=9, color="#cbd5e1", anchor="end"))
    return "".join(parts)


def note_panel(x: float, y: float, w: float, h: float, title: str, lines: Iterable[str], *, color: str = COLORS["uncertain"]) -> str:
    parts = [f'<rect x="{x:g}" y="{y:g}" width="{w:g}" height="{h:g}" rx="8" fill="{color}" fill-opacity="0.08" stroke="{color}" stroke-width="1" stroke-dasharray="6 4"/>',
             text_lines(x + 14, y + 23, [title], size=12, color=color, weight="700")]
    for i, line in enumerate(lines):
        parts.append(text_lines(x + 14, y + 44 + i * 17, wrap(line, max(20, int((w-28)/7))), size=9, color="#fecdd3"))
    return "".join(parts)


def svg_page(page_id: str, title: str, subtitle: str, body: str, *, page_no: int) -> str:
    header = text_lines(54, 48, [f"P{page_no:02d}  {title}"], size=22, weight="700")
    header += text_lines(54, 73, wrap(subtitle, 160), size=11, color="#cbd5e1")
    footer = text_lines(54, 918, ["电子木鱼硬件基线 · 离线人工复刻图 · 静态合同通过 ≠ ERC/样机通过"], size=10, color="#64748b")
    return f'''<svg class="sheet-svg" data-page="{esc(page_id)}" viewBox="0 0 1600 950" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="{esc(title)}">
      {svg_defs()}
      <rect width="1600" height="950" fill="{COLORS['bg']}"/>
      <rect width="1600" height="950" fill="url(#grid)"/>
      {header}
      {body}
      {footer}
    </svg>'''


def p01() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 66, "落图边界", [
        "VBAT_SW 只能来自 TPS22965 输出；BQ_SYS 只保留系统侧/测试点，禁止接 M100 VIN。",
        "USB-C → FSW7227 单路选择 → BQ D+/D− 或 ESP32 原生 USB；强制焊盘只能闭合一路。",
    ], color=COLORS["power"]))
    # Power flow wires first.
    b.extend([
        wire(270, 255, 365, 255, color=COLORS["power"], label="VBUS_5V"),
        wire(270, 340, 365, 340, color=COLORS["signal"], label="USB_DP/DM_C"),
        wire(270, 600, 365, 600, color=COLORS["power"], label="BAT_PROTECTED"),
        wire(665, 225, 760, 225, color=COLORS["power"], label="PMID_OTG_5V"),
        wire(665, 315, 760, 315, color=COLORS["neutral"], label="BQ_SYS", dashed=True),
        wire(1015, 225, 1080, 225, color=COLORS["power"], label="PWR_LATCH"),
        wire(1260, 225, 1410, 225, color=COLORS["power"], label="VBAT_SW"),
        wire(1260, 470, 1410, 470, color=COLORS["power"], label="3V3"),
        wire(1015, 515, 1080, 515, color=COLORS["power"], label="PMID_OTG_5V"),
        wire(1260, 515, 1410, 515, color=COLORS["power"], label="AUDIO_5V"),
        wire(515, 650, 515, 760, color=COLORS["signal"], label="USB_MUX_SEL/DSEL"),
    ])
    b.extend([
        box(60, 205, 210, 180, "USB-C 输入", "TYPEC-304-ACP16 · C165948 候选", color=COLORS["connector"], status="recommended_initial", ref="J1", lines=["VBUS → VBUS_5V", "D+ / D− → USB_DP/DM_C", "CC1/CC2 → 5.1 kΩ Rd → GND", "SBU = NC · SHIELD = USB_SHIELD"]),
        box(60, 520, 210, 160, "单节电池", "B3B-PH-K-S(LF)(SN) · C131339", color=COLORS["connector"], status="confirmed", ref="J2", lines=["1 BATT+ → BAT_PROTECTED", "2 BATT− → GND", "3 NTC → BAT_NTC", "约 3000 mAh，高倍率"]),
        box(365, 185, 300, 235, "BQ25895RTWR", "QFN-24-EP · C80200 · I²C 0x6A", color=COLORS["power"], status="confirmed", ref="U1", lines=["VBUS / BAT / SYS / PMID", "D+ / D− = USB_DP/DM_BQ", "OTG_EN = BQ_OTG_EN (IO8)", "TS：5.23 kΩ / 30.1 kΩ + 10 kΩ NTC", "RILIM≈180 Ω · ICHG≈1.5 A", "PMID≈60 µF · REGN 4.7 µF · BTST 47 nF"]),
        box(760, 180, 255, 150, "TPS3424A11C13ADRLR", "硬件 PWR 锁存 · LPT≈2 s 候选", color=COLORS["power"], status="recommended_initial", ref="U2", lines=["VIN = BAT_PROTECTED", "PB = PWR_BUTTON", "SPT≈50 ms · LPT=4.7 nF", "KILL/INT/RESET 留测试点"]),
        box(1080, 180, 180, 150, "TPS22965DSGR", "UDFN-8-EP · C122837", color=COLORS["power"], status="recommended_initial", ref="U3", lines=["IN = BAT_PROTECTED", "ON = PWR_LATCH", "OUT = VBAT_SW", "CIN 22 µF + 100 nF"]),
        box(1080, 400, 180, 150, "TLV62569DBVR", "SOT-23-5 · C141836", color=COLORS["power"], status="recommended_initial", ref="U4", lines=["IN = VBAT_SW", "L=2.2 µH · FB 453 k/100 k", "OUT = 3V3", "COUT = 2×22 µF"]),
        box(760, 445, 255, 140, "TPS22919DCKR", "SC-70-6 · C2149796", color=COLORS["power"], status="recommended_initial", ref="U5", lines=["VIN = PMID_OTG_5V", "ON = AUDIO_EN / PMID_VALID", "OUT = AUDIO_5V", "EN 下拉 100 kΩ"]),
        box(365, 540, 300, 205, "FSW7227YMS10G/TR", "MSOP-10 · C32713318", color=COLORS["signal"], status="confirmed", ref="U6", lines=["COM = USB_DP/DM_C", "BQ 端 = USB_DP/DM_BQ", "ESP 端 = USB_D+/USB_D−", "SEL/DSEL：默认 BQ，保留强制焊盘", "VDD = 3V3 · /EN 默认有效"]),
        note_panel(1320, 175, 220, 170, "必须样机验证", ["USB 5 V / 2 A 与 OVP", "PMID OTG 启动/限流", "PWR 长按与掉压", "MUX 隔离/无反灌"], color=COLORS["uncertain"]),
        note_panel(1320, 390, 220, 180, "电源规则", ["M100 VIN = VBAT_SW", "BQ_SYS 不供 4G", "3V3 只供逻辑", "AUDIO_5V 先稳定，再 PA_EN"], color=COLORS["power"]),
    ])
    b.append(text_lines(365, 795, ["推荐初值（均需按所购器件数据手册复核）：VBUS 1 µF + 100 nF · SYS≈20 µF · 2.2 µH · 输入/输出浪涌留示波器测试点"], size=10, color="#cbd5e1"))
    return svg_page("P01_POWER_USB", "电源、充电、USB MUX 与硬件锁存", "单 USB-C / BQ25895 NVDC+PMID / VBAT_SW / 3V3 / AUDIO_5V", "".join(b), page_no=1)


def p02() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 60, "主控规则", ["ESP32-S3-WROOM-1-N16R8 只使用合同中的可用 GPIO；IO0/IO46 为启动绑带，EN 独立复位，IO35–37/45 不作普通 GPIO。"], color=COLORS["signal"]))
    b.extend([
        wire(275, 245, 445, 245, color=COLORS["signal"], label="BOOT0"),
        wire(275, 330, 445, 330, color=COLORS["signal"], label="RESET_N"),
        wire(275, 415, 445, 415, color=COLORS["signal"], label="PWR_STATE"),
        wire(1035, 240, 1280, 240, color=COLORS["signal"], label="USB_D− / USB_D+"),
        wire(1035, 320, 1280, 320, color=COLORS["signal"], label="I2C_SDA / I2C_SCL"),
        wire(1035, 400, 1280, 400, color=COLORS["signal"], label="I2S_*"),
        wire(1035, 480, 1280, 480, color=COLORS["signal"], label="LCD_*"),
        wire(1035, 560, 1280, 560, color=COLORS["signal"], label="M100 UART/DTR/RST"),
    ])
    b.extend([
        box(70, 200, 205, 90, "BOOT 按键", "下载绑带", color=COLORS["connector"], status="recommended_initial", ref="SW1", lines=["BOOT0(IO0) ↔ GND", "下载时拉低"]),
        box(70, 305, 205, 90, "RESET 按键", "独立 EN", color=COLORS["connector"], status="recommended_initial", ref="SW2", lines=["RESET_N ↔ EN", "不占普通 GPIO"]),
        box(70, 410, 205, 90, "PWR 状态", "硬件锁存反馈", color=COLORS["power"], status="confirmed", ref="TP_PWR", lines=["PWR_STATE = IO46", "固件只读"]),
        box(445, 165, 590, 500, "ESP32-S3-WROOM-1-N16R8", "主控模组 · C2913202 · 原生 USB-Serial-JTAG", color=COLORS["signal"], status="confirmed", ref="U7", lines=["左侧：IO0 BOOT0 · IO1/2 I²C · IO3 RGB · IO4/5/6/7 QSPI", "左侧：IO8 BQ_OTG_EN · IO9 PVDF_ADC · IO10 DTR · IO11 PVDF_WAKE", "左侧：IO12 LCD_D2 · IO13/14/17/18/21 I²S · IO15 M100_RST", "右侧：IO16 NET_STATUS 兼容位/当前 NC · IO19/20 USB", "右侧：IO38/39 TP_RST/INT · IO40 LCD_CS · IO41 IMU_INT1", "右侧：IO42 LCD_D3 · IO43/44 4G UART · IO46 PWR · IO47 LCD_EN", "右侧：IO48 PA_EN；IO45 NC；IO35–37 保留", "EN = RESET_N；所有外连线均用全局网络标签"]),
        box(1280, 185, 250, 95, "USB 调试", "来自 P01 的 ESP 端", color=COLORS["signal"], ref="TP_USB", lines=["IO19 = USB_D−", "IO20 = USB_D+"]),
        box(1280, 300, 250, 95, "共享 I²C", "I2C0 · 4.7 kΩ 上拉", color=COLORS["signal"], ref="TP_I2C", lines=["IO1 = I2C_SDA", "IO2 = I2C_SCL"]),
        box(1280, 415, 250, 95, "音频总线", "P04", color=COLORS["peripheral"], ref="TP_I2S", lines=["IO13/14/17/18/21", "DOUT/WS/DIN/BCLK/MCLK"]),
        box(1280, 530, 250, 110, "显示/4G", "P03 + P06", color=COLORS["peripheral"], ref="TP_IO", lines=["QSPI：IO4/5/6/7/12/40/42/47", "UART：IO43/44 · DTR IO10 · RST IO15"]),
        note_panel(445, 695, 590, 100, "手工复刻顺序", ["先放 U7 与 EN/BOOT，再按 GPIO 合同逐条放全局标签；不要把模块内部引脚号当作 GPIO 合同，也不要将 IO8 接 GNSS_VCC。"], color=COLORS["signal"]),
    ])
    return svg_page("P02_MCU_USB", "ESP32-S3 主控、启动复位与 USB-Serial-JTAG", "按 GPIO 合同逐条放置全局标签；P01/P03/P04/P05/P06/P07 通过网络名连接", "".join(b), page_no=2)


def fpc_pin_rows() -> list[tuple[int, str, str]]:
    return [
        (1, "TP-VDD_3V", "3V3"), (2, "TP-SCL_3V", "I2C_SCL"), (3, "TP-SDA_3V", "I2C_SDA"),
        (4, "TP-INT_3V", "TP_INT"), (5, "TP-RST_3V", "TP_RST"), (6, "TP-GND", "GND"),
        (7, "VDD_3V", "3V3"), (8, "VBAT", "LCD_VBAT=VBAT_SW 初版"), (9, "MTP/NC", "NC"),
        (10, "VDD_3V", "3V3"), (11, "GND", "GND"), (12, "L_RES", "LCD_RST"),
        (13, "GND", "GND"), (14, "L_IO0", "LCD_D0"), (15, "GND", "GND"),
        (16, "L_SCLK", "LCD_SCLK"), (17, "GND", "GND"), (18, "L_IO3", "LCD_D3"),
        (19, "NC", "NC"), (20, "L_IO2", "LCD_D2"), (21, "CS", "LCD_CS"),
        (22, "L_IO1", "LCD_D1"), (23, "OLED_EN", "LCD_EN"), (24, "LTE/TE", "LCD_TE/TP"),
    ]


PIN_CONTRACTS: dict[str, list[tuple[str, str, str, str]]] = {
    "P01_POWER_USB": [
        ("J1", "VBUS", "VBUS_5V", "A4/B4/A9/B9；输入"),
        ("J1", "D+ / D−", "USB_DP_C / USB_DM_C", "A6/B6、A7/B7；USB2 差分"),
        ("J1", "CC1 / CC2", "USB_CC1 / USB_CC2", "各 5.1 kΩ Rd → GND"),
        ("J1", "SBU1 / SBU2", "NC", "不接"),
        ("J1", "SHIELD", "USB_SHIELD", "按 EMI/机壳方案处理"),
        ("J2", "1 / 2 / 3", "BAT_PROTECTED / GND / BAT_NTC", "BATT+ / BATT− / NTC"),
        ("U1", "VBUS / BAT / SYS / PMID", "VBUS_5V / BAT_PROTECTED / BQ_SYS / PMID_OTG_5V", "NVDC；SYS 不去 M100"),
        ("U1", "D+ / D−", "USB_DP_BQ / USB_DM_BQ", "仅接 FSW BQ 端"),
        ("U1", "OTG / SDA / SCL", "BQ_OTG_EN / I2C_SDA / I2C_SCL", "IO8 / IO1 / IO2"),
        ("U1", "TS / REGN / BTST / SW", "BAT_NTC / REGN / BTST / L_BQ", "TS 5.23k/30.1k + 10k NTC；值待测"),
        ("U2", "VIN / PB / SPT / LPT / KILL / INT", "BAT_PROTECTED / PWR_BUTTON / 候选 / 候选 / VDD / PWR_INT_N", "TPS3424A11C13ADRLR；按购入版本核脚"),
        ("U3", "IN / OUT / ON / CT / QOD", "BAT_PROTECTED / VBAT_SW / PWR_LATCH / 候选 / 受控放电", "TPS22965；ON 不得悬空"),
        ("U4", "IN / SW / FB / EN / GND", "VBAT_SW / L_BQ? / 3V3_FB / 3V3_EN / GND", "TLV62569；电感/反馈按数据手册"),
        ("U5", "VIN / OUT / ON / QOD / GND", "PMID_OTG_5V / AUDIO_5V / AUDIO_EN / 候选 / GND", "TPS22919；EN 下拉 100 kΩ"),
        ("U6", "COM / BQ / ESP DP-DM", "USB_DP/DM_C / USB_DP/DM_BQ / USB_D+/USB_D−", "FSW7227 单路选择"),
        ("U6", "SEL / DSEL / VDD / /EN", "USB_MUX_SEL / USB_MUX_DSEL / 3V3 / 有效", "默认 BQ；强制焊盘互斥"),
    ],
    "P04_AUDIO_RGB": [
        ("U11", "SDA / SCL", "I2C_SDA / I2C_SCL", "地址首版 0x18"),
        ("U11", "I²S DO / WS / DI / BCLK / MCLK", "I2S_DOUT / I2S_WS / I2S_DIN / I2S_BCLK / I2S_MCLK", "IO13/14/17/18/21"),
        ("U11", "PVDD / DVDD / AVDD", "3V3", "各 100 nF；AVDD 1 µF"),
        ("U11", "OUTP / OUTN", "CODEC_OUT_P / CODEC_OUT_N", "各 1 µF + 0 Ω"),
        ("U12", "VCC / EN", "AUDIO_5V / PA_EN", "EN 下拉 100 kΩ"),
        ("U12", "INP / INN", "CODEC_OUT_P / CODEC_OUT_N", "100 nF 耦合 + 100 kΩ 偏置"),
        ("U12", "OUTP / OUTN", "SPK_P / SPK_N", "BTL；扬声器端不接地"),
        ("LED1", "VDD / DI / GND", "AUDIO_5V / RGB_DATA / GND", "DI 串 330 Ω；默认下拉"),
    ],
    "P05_PVDF": [
        ("S1/J6", "PVDF+ / PVDF−", "PVDF_RAW / GND", "LDT0-028K；平面贴合"),
        ("R_PVDF_SER", "两端", "PVDF_RAW / PVDF_COND", "1 MΩ 候选"),
        ("D_CLAMP/R_BIAS", "钳位 / 泄放", "PVDF_COND / VBIAS 或 GND", "低漏型号与方向待测；10 MΩ 候选"),
        ("U15", "IN / OUT / VBIAS", "PVDF_COND / PVDF_BUF / 约 1.65 V", "TLV2369 缓冲候选"),
        ("R_ADC/C_ADC", "ADC 支路", "PVDF_BUF → PVDF_ADC", "1 kΩ + 100 nF → IO9"),
        ("U16", "IN+ / IN− / OUT", "窗口阈值 / VBIAS / PVDF_CMP_WAKE", "TLV7042 双窗候选；输出极性待测"),
    ],
    "P06_MODEM": [
        ("J4/U8", "VIN / GND", "M100_VIN=VBAT_SW / GND", "3.3–4.2 V；高电流轨"),
        ("J4/U8", "TXD / RXD", "M100_RXD / M100_TXD", "TXD→IO44；RXD←IO43"),
        ("J4/U8", "DTR / RST", "M100_DTR / M100_RST", "IO10/IO15；开漏/极性待测"),
        ("J4/U8", "NET_STATUS", "M100_NET_STATUS_COMPAT", "IO16；无载板脚则 TP/NC"),
        ("J4/U8", "GNSS_VCC", "M100_GNSS_VCC_NC", "NC/TP；不接 IO8"),
        ("J4/U8", "RI / 1PPS / USB", "TP/NC", "仅测试点；不占普通 GPIO"),
    ],
    "P07_SHARED_SENSORS": [
        ("U13", "SDA / SCL / VDD", "I2C_SDA / I2C_SCL / 3V3", "CW2015；0x62 候选"),
        ("U14", "SDA / SCL / SA0", "I2C_SDA / I2C_SCL / 3V3", "QMI8658C；SA0=高，0x6B"),
        ("U14", "INT1 / INT2", "IMU_INT1(IO41) / NC", "INT2 不接，不占 IO45"),
        ("R_I2C", "SDA / SCL 上拉", "3V3", "4.7 kΩ；总线共享"),
    ],
}


def p03() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 60, "显示模组边界", ["PY206-W38-V2 规格书中的 CO5300AF-51 / CST9217 / OCP21351 视为模组内集成；主板只放 J3 FPC 座，不重复放 OCP21351。第 8 脚 VBAT、接触面和 pin 1 方向必须实物复核。"], color=COLORS["uncertain"]))
    b.extend([
        box(65, 190, 310, 560, "FH34SRJ-24S-0.5SH(50)", "24P / 0.5 mm / 双面接触候选 · C324726", color=COLORS["connector"], status="recommended_initial", ref="J3", lines=["1–6：触摸供电/ I²C / INT / RST / GND", "7–11：显示供电与 GND", "12–18：QSPI + GND", "19–24：NC / QSPI / OLED_EN / TE", "逐脚网络见右侧表；插合前确认方向"]),
        box(500, 235, 300, 180, "CO5300AF-51", "PY206 内置显示控制器", color=COLORS["peripheral"], status="recommended_initial", ref="U9", lines=["410×502 AMOLED", "QSPI：RST/CS/SCLK/D0–D3", "OLED_EN = LCD_EN(IO47)", "OCP21351 已在模组内"]),
        box(500, 500, 300, 150, "CST9217", "PY206 内置触摸控制器 · I²C", color=COLORS["peripheral"], status="recommended_initial", ref="U10", lines=["SDA/SCL = IO1/IO2", "TP_RST = IO38", "TP_INT = IO39", "地址首版 0x5A"]),
        wire(375, 280, 500, 280, color=COLORS["signal"], label="LCD_*"),
        wire(375, 545, 500, 545, color=COLORS["signal"], label="I2C_SDA/SCL"),
        wire(800, 300, 1080, 300, color=COLORS["signal"], label="IO4/5/6/7/12/40/42/47"),
        wire(800, 560, 1080, 560, color=COLORS["signal"], label="IO1/2/38/39"),
        note_panel(1080, 190, 450, 170, "QSPI 对照", ["LCD_RST ← IO4", "LCD_SCLK ← IO5", "LCD_D0 ← IO6", "LCD_D1 ← IO7", "LCD_D2 ← IO12", "LCD_D3 ← IO42", "LCD_CS ← IO40 · LCD_EN ← IO47"], color=COLORS["signal"]),
        note_panel(1080, 390, 450, 120, "触摸对照", ["FPC 1/7/10 = 3V3 · 2 = I2C_SCL · 3 = I2C_SDA", "4 = TP_INT(IO39) · 5 = TP_RST(IO38) · 6/11/13/15/17 = GND"], color=COLORS["peripheral"]),
    ])
    # 24-pin compact table drawn into SVG.
    b.append(f'<rect x="1000" y="550" width="550" height="280" rx="8" fill="#111827" stroke="#475569"/>')
    b.append(text_lines(1020, 575, ["FPC 24-pin 逐针网络（与 PDF 第 3–4 页逐项核对）"], size=12, color=COLORS["white"], weight="700"))
    for i, (pin, source, net) in enumerate(fpc_pin_rows()):
        col, row = (0 if i < 12 else 1), i % 12
        x = 1020 + col * 265
        y = 598 + row * 18
        b.append(text_lines(x, y, [f"{pin:>2}  {source:<10} → {net}"], size=9, color="#dbeafe"))
    return svg_page("P03_DISPLAY_TOUCH", "PY206-W38-V2 显示与触摸 FPC", "J3 24-pin 逐脚网络；显示/触摸 IC 集成在模组内", "".join(b), page_no=3)


def p04() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 60, "音频时序", ["PMID_OTG_5V → TPS22919 → AUDIO_5V 稳定后才拉高 PA_EN(IO48)；充电、静音、故障时 PA_EN 保持低。扬声器采用 BTL，SPK_P/SPK_N 不接地。"], color=COLORS["power"]))
    b.extend([
        wire(290, 300, 470, 300, color=COLORS["signal"], label="I2S_* / I2C"),
        wire(770, 315, 960, 315, color=COLORS["signal"], label="DAC_OUT_P/N"),
        wire(1130, 315, 1300, 315, color=COLORS["signal"], label="SPK_P / SPK_N"),
        wire(290, 560, 470, 560, color=COLORS["power"], label="AUDIO_5V"),
        wire(770, 540, 960, 540, color=COLORS["signal"], label="PA_EN(IO48)"),
        wire(1130, 540, 1300, 540, color=COLORS["signal"], label="RGB_DATA(IO3)"),
        box(70, 220, 220, 180, "共享总线", "来自 P01/P02", color=COLORS["signal"], status="confirmed", ref="BUS", lines=["I²C：SDA=IO1 · SCL=IO2", "I²S：DO=IO13 · WS=IO14", "DI=IO17 · BCLK=IO18", "MCLK=IO21"]),
        box(470, 200, 300, 240, "ES8311", "QFN-20-EP · C962342 · 地址 0x18 候选", color=COLORS["peripheral"], status="recommended_initial", ref="U11", lines=["PVDD/DVDD：100 nF 各一", "AVDD/参考端：1 µF", "OUTP/OUTN：1 µF + 0 Ω", "I²S 22 pF 只留 DNP 料位", "codec 只接 3V3，不接 AUDIO_5V"]),
        box(960, 200, 300, 240, "NS4150B", "MSOP-8 · C189961 · Class-D", color=COLORS["peripheral"], status="confirmed", ref="U12", lines=["VCC = AUDIO_5V", "1 µF + 22 µF + 22 µF", "BYPASS = 1 µF", "差分输入：100 nF 耦合 + 100 kΩ 偏置", "PA_EN 下拉 = 100 kΩ", "输出 EMI：1 nF/0 Ω DNP"]),
        box(1300, 235, 220, 160, "扬声器", "4 Ω / 3 W · 单声道", color=COLORS["connector"], status="must_prototype_validate", ref="J5", lines=["1 SPK+ → SPK_P", "2 SPK− → SPK_N", "BTL 输出不接地", "腔体/线长待验证"]),
        box(470, 500, 300, 160, "AUDIO_5V", "受控功放电源", color=COLORS["power"], status="confirmed", ref="TP_A5", lines=["来源：P01 TPS22919", "默认关闭；先稳压再 PA_EN", "充电期间固件暂停音频"]),
        box(960, 500, 300, 160, "WS2812C-2020-V1", "5 V 版本 · C2976072 候选", color=COLORS["peripheral"], status="must_prototype_validate", ref="LED1", lines=["VDD = AUDIO_5V", "DI = IO3 / RGB_DATA", "串联约 330 Ω", "默认下拉；3.3 V 高电平需实测"]),
        note_panel(70, 500, 330, 160, "手工落图元件", ["C16/C17：100 nF", "C6/C7/C8：1 µF", "C14/C15：1 µF + R7/R8=0 Ω", "NS4150B 输入/旁路/输出按上框值放置"], color=COLORS["peripheral"]),
        note_panel(1300, 500, 220, 160, "必须样机验证", ["I²S 主从与地址", "功放启动等待", "爆音/温升", "RGB 输入高电平"], color=COLORS["uncertain"]),
    ])
    return svg_page("P04_AUDIO_RGB", "ES8311、NS4150B、扬声器与 RGB", "差分音频链路 + 受控 AUDIO_5V + 状态灯", "".join(b), page_no=4)


def p05() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 60, "PVDF 安全边界", ["LDT0-028K 会产生高压瞬态；1 MΩ 限流、低漏钳位、10 MΩ 泄放和 ADC RC 是首版保护候选。比较器阈值约 1.65 V ±150 mV、滞回和消抖全部必须样机验证。"], color=COLORS["uncertain"]))
    # Flow arrows.
    b.extend([
        wire(245, 355, 340, 355, color=COLORS["signal"], label="PVDF_RAW"),
        wire(500, 355, 595, 355, color=COLORS["signal"], label="PVDF_COND"),
        wire(755, 355, 850, 355, color=COLORS["signal"], label="VBIAS/BUF"),
        wire(1010, 355, 1120, 355, color=COLORS["signal"], label="ADC"),
        wire(1010, 480, 1120, 480, color=COLORS["signal"], label="CMP_WAKE"),
        wire(1260, 355, 1430, 355, color=COLORS["signal"], label="IO9 ADC1_CH8"),
        wire(1260, 480, 1430, 480, color=COLORS["signal"], label="IO11 WAKE"),
    ])
    b.extend([
        box(60, 275, 185, 160, "LDT0-028K", "平面贴合木鱼壳体", color=COLORS["connector"], status="confirmed", ref="S1/J6", lines=["PVDF+ → PVDF_RAW", "PVDF− → GND", "有效自由长度/应力释放待定"]),
        box(340, 275, 160, 160, "限流", "首版候选", color=COLORS["uncertain"], status="must_prototype_validate", ref="R1", lines=["R_SER = 1 MΩ", "1% / 0603", "不得省略"]),
        box(595, 275, 160, 160, "钳位/泄放", "低漏器件候选", color=COLORS["uncertain"], status="must_prototype_validate", ref="D/R", lines=["D_CLAMP：低漏", "R_BIAS = 10 MΩ", "型号待采购复核"]),
        box(850, 260, 160, 190, "TLV2369", "IDGKR · C2057867", color=COLORS["peripheral"], status="recommended_initial", ref="U15", lines=["缓冲/偏置", "VBIAS≈1.65 V", "RRIO/低漏候选"]),
        box(1120, 275, 140, 160, "ADC RC", "输入保护", color=COLORS["signal"], status="must_prototype_validate", ref="R2/C2", lines=["R_ADC = 1 kΩ", "C_ADC = 100 nF", "→ IO9"]),
        box(1120, 435, 140, 160, "TLV7042", "双窗比较器候选", color=COLORS["peripheral"], status="must_prototype_validate", ref="U16", lines=["阈值：1.65 V ±150 mV", "输出极性待测", "→ IO11 唤醒"]),
        box(1430, 275, 115, 160, "PVDF_ADC", "IO9", color=COLORS["signal"], ref="TP", lines=["ADC1_CH8", "仅二次确认"]),
        box(1430, 435, 115, 160, "PVDF_WAKE", "IO11", color=COLORS["signal"], ref="TP", lines=["低功耗唤醒", "开漏线与 100 kΩ 上拉"]),
        note_panel(60, 520, 945, 170, "测试点与验证", ["TP_PVDF_RAW：钳位前后波形；TP_PVDF_ADC：ADC 输入范围；TP_PVDF_CMP：比较器输出。测试 1–20 次/秒、环境振动、负摆幅和过压；不得保存/上传原始波形。"], color=COLORS["uncertain"]),
        note_panel(1040, 650, 505, 70, "不可冻结项", ["比较器具体封装、钳位型号、阈值/滞回/消抖、IO11 极性"], color=COLORS["uncertain"]),
    ])
    return svg_page("P05_PVDF", "PVDF 敲击模拟前端、ADC 与唤醒", "LDT0-028K → 1 MΩ → 低漏钳位/缓冲 → ADC + 双窗比较器", "".join(b), page_no=5)


def p06() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 60, "M100 载板边界", ["VIN 只接 VBAT_SW；M100 USB 仅留测试点，不并入主 USB。RST 先按 Air780 低有效方式接开漏级，首块样机必须测极性；NET_STATUS/GNSS_VCC 无载板脚时只留 TP/NC。"], color=COLORS["uncertain"]))
    b.extend([
        wire(335, 280, 500, 280, color=COLORS["power"], label="VBAT_SW → VIN"),
        wire(335, 360, 500, 360, color=COLORS["signal"], label="TXD/RXD 交叉"),
        wire(335, 440, 500, 440, color=COLORS["signal"], label="DTR / RST"),
        wire(335, 520, 500, 520, color=COLORS["uncertain"], label="NET_STATUS / GNSS NC", dashed=True),
        wire(950, 300, 1170, 300, color=COLORS["signal"], label="IO43/44 UART"),
        wire(950, 390, 1170, 390, color=COLORS["signal"], label="IO10 DTR · IO15 RST"),
        wire(950, 480, 1170, 480, color=COLORS["uncertain"], label="IO16 compat", dashed=True),
    ])
    b.extend([
        box(70, 205, 265, 390, "M100EG-C2", "Air780EGP 载板 · 25×25 mm", color=COLORS["connector"], status="confirmed", ref="U8/J4", lines=["前排：VIN · GND · RXD · TXD", "      DTR · RST · RI · 1PPS", "USB 组：VB · DM · DP · GND（测试点）", "SIM / ANT：载板自带或外接，需核版", "VIN 电池轨：3.3–4.2 V", "持续 >1 A · 峰值约 1.5–2 A"]),
        box(500, 205, 450, 390, "逻辑交叉", "ESP32 ↔ M100 网络合同", color=COLORS["signal"], status="confirmed", ref="NET", lines=["载板 TXD → ESP IO44（M100_RXD）", "载板 RXD ← ESP IO43（M100_TXD）", "DTR ← ESP IO10（开漏）", "RST ← ESP IO15（低有效候选/开漏）", "NET_STATUS → IO16 兼容位；无脚则 TP/NC", "GNSS_VCC 不接 IO8；IO8 已给 BQ_OTG_EN", "RI / 1PPS 仅测试点；不占普通 GPIO"]),
        box(1170, 205, 320, 150, "UART / AT", "来自 P02", color=COLORS["signal"], status="confirmed", ref="IO43/44", lines=["IO43 → M100_RXD", "IO44 ← M100_TXD", "单一 AT 事务所有权"]),
        box(1170, 380, 320, 150, "DTR / RESET", "极性与方向待实测", color=COLORS["signal"], status="must_prototype_validate", ref="IO10/15", lines=["DTR：休眠/唤醒", "RST：先按低有效", "均保留 TP"]),
        note_panel(1170, 565, 320, 125, "必须样机验证", ["载板真实 header pin 编号", "UART 电平/波特率", "DTR/RST 极性", "峰值掉压/天线/散热"], color=COLORS["uncertain"]),
        note_panel(70, 655, 880, 85, "main_control 交叉规则", ["gps.h 旧宏：DTR=IO7、RST=IO15、NET_STATUS=IO16、UART_TX=IO17、UART_RX=IO18、GNSS_VCC=IO8；仅用于识别信号名，EWF 实际采用 IO10/15/16/43/44，GNSS_VCC 释放。"], color=COLORS["uncertain"]),
    ])
    return svg_page("P06_MODEM", "M100EG-C2 / Air780EGP 载板与测试点", "VBAT_SW 高电流供电、UART 交叉、DTR/RST、兼容位与 NC 边界", "".join(b), page_no=6)


def p07() -> str:
    b = []
    b.append(note_panel(50, 105, 1500, 60, "共享总线规则", ["I2C0：SDA=IO1、SCL=IO2，统一 3V3 上拉；五个 7-bit 地址必须唯一。QMI8658C 首版 SA0 置高为 0x6B，INT2 不接。"], color=COLORS["signal"]))
    b.extend([
        bus(190, 300, 1180, "I2C_SDA / I2C_SCL", color=COLORS["signal"], note="4.7 kΩ → 3V3"),
        wire(360, 300, 360, 430, color=COLORS["signal"], arrow=False),
        wire(720, 300, 720, 430, color=COLORS["signal"], arrow=False),
        wire(1080, 300, 1080, 430, color=COLORS["signal"], arrow=False),
        wire(360, 520, 360, 610, color=COLORS["signal"], label="IMU_INT1"),
        box(190, 430, 340, 180, "CW2015", "电量计 · 地址 0x62 候选", color=COLORS["peripheral"], status="recommended_initial", ref="U13", lines=["SDA/SCL → 共享 I²C", "VDD = 3V3", "ALERT/低电状态按目标料号核对", "实板 ACK 必须扫描"]),
        box(550, 430, 340, 180, "QMI8658C", "六轴 IMU · SA0=高 · 0x6B", color=COLORS["peripheral"], status="recommended_initial", ref="U14", lines=["SDA/SCL → 共享 I²C", "INT1 → IO41 / IMU_INT1", "INT2 = NC（不占 IO45）", "MVP 默认不启用，保留扩展"]),
        box(910, 430, 340, 180, "地址/测试点", "P01/P03/P04 同一总线", color=COLORS["signal"], ref="TP", lines=["BQ25895 = 0x6A", "CST9217 = 0x5A", "ES8311 = 0x18", "TP_I2C_SDA · TP_I2C_SCL"]),
        note_panel(190, 660, 1060, 80, "扫描验收", ["上电运行 I²C 扫描；若地址、SA0 或触摸模组实际值不同，先更新硬件基线与 spine，再改固件驱动，不在驱动中静默兼容多个地址。"], color=COLORS["uncertain"]),
        note_panel(1300, 430, 220, 180, "器件状态", ["CW2015：首版贴装", "QMI8658C：首版贴装", "INT2：NC", "地址：样机确认"], color=COLORS["peripheral"]),
    ])
    return svg_page("P07_SHARED_SENSORS", "共享 I²C、CW2015 与 QMI8658C", "电量计 / IMU / 地址表 / 总线测试点", "".join(b), page_no=7)


def html_table(headers: list[str], rows: Iterable[Iterable[object]], *, cls: str = "") -> str:
    head = "".join(f"<th>{esc(h)}</th>" for h in headers)
    body_rows = []
    for row in rows:
        cells = []
        for value in row:
            if isinstance(value, tuple) and len(value) == 2 and value[0] == "status":
                cells.append(f"<td>{badge_html(str(value[1]))}</td>")
            else:
                cells.append(f"<td>{esc(value)}</td>")
        body_rows.append("<tr>" + "".join(cells) + "</tr>")
    return f'<div class="table-wrap"><table class="{esc(cls)}"><thead><tr>{head}</tr></thead><tbody>{"".join(body_rows)}</tbody></table></div>'


def page_card(page: dict, svg: str, manifest: dict) -> str:
    pid = page["id"]
    page_no = int(re.search(r"P(\d+)", pid).group(1))
    rows = []
    for net in page["nets"]:
        n = next((item for item in manifest["nets"] if item.get("name") == net), None)
        rows.append((net, (n or {}).get("class", ""), ", ".join((n or {}).get("endpoints", [])), ("status", (n or {}).get("status", "recommended_initial"))))
    if pid == "P03_DISPLAY_TOUCH":
        detail = html_table(["FPC pin", "原厂符号", "EWF 网络"], fpc_pin_rows(), cls="compact")
    elif pid == "P02_MCU_USB":
        detail = html_table(["功能", "网络", "GPIO", "方向/约束"], ((g.get("signal"), g.get("net"), g.get("gpio"), "; ".join(g.get("constraints", []))) for g in manifest["gpio"]), cls="compact")
    elif pid == "P06_MODEM":
        cr = manifest["carrier_pin_cross_reference"]
        detail = html_table(["载板信号", "旧 GPIO（仅交叉）", "EWF GPIO", "网络", "测试点/处理"], ((r.get("carrier_signal", ""), r.get("legacy_source_gpio") or "—", r.get("ewf_gpio") or "—", r.get("ewf_net", ""), f"{r.get('test_point', '—')}；{r.get('note', '')}") for r in cr.get("rows", [])), cls="compact")
    else:
        detail = html_table(["网络", "类别", "端点", "状态"], rows, cls="compact")
    contracts = PIN_CONTRACTS.get(pid, [])
    if contracts:
        detail += "<h4>关键器件脚位契约（功能名；封装号按数据手册核对）</h4>" + html_table(["位号", "功能脚", "网络", "备注"], contracts, cls="compact")
    return f'''<section class="sheet-section searchable" id="{esc(pid)}" data-title="{esc(page['title'])}">
      <div class="sheet-heading"><div><span class="sheet-id">P{page_no:02d}</span><h2>{esc(page['title'])}</h2></div><span class="sheet-state">标准符号原理图 · 人工复刻主视图 · <a class="open-page" href="pages/{esc(pid)}.svg" target="_blank" rel="noopener">单页放大</a></span></div>
      <div class="svg-frame">{svg}</div>
      <details open><summary>该页网络/针脚明细（点击展开）</summary>{detail}</details>
    </section>'''


def build_html(manifest: dict, svgs: list[str]) -> str:
    pages = manifest["schematic_pages"]
    page_sections = "".join(page_card(p, s, manifest) for p, s in zip(pages, svgs))
    gpio_rows = ((g.get("signal"), g.get("net"), g.get("gpio"), g.get("direction"), ("status", g.get("verification_status"))) for g in manifest["gpio"])
    i2c_rows = ((d.get("part"), d.get("address_7bit"), d.get("role", ""), ("status", d.get("status", "recommended_initial"))) for d in manifest["i2c"]["devices"])
    rail_rows = ((r.get("name"), r.get("nominal_voltage"), r.get("source"), ", ".join(r.get("consumers", [])), ("status", r.get("status"))) for r in manifest["power_rails"])
    bom_rows = ((b.get("reference"), b.get("part") or "待选", b.get("lcsc") or "待核", b.get("role", ""), ("status", b.get("status", "recommended_initial"))) for b in manifest["bom"])
    net_rows = ((n.get("name"), n.get("class", ""), ", ".join(n.get("endpoints", [])), ("status", n.get("status", "recommended_initial"))) for n in manifest["nets"])
    open_rows = ((o.get("id"), o.get("topic"), o.get("verification"), ("status", o.get("status"))) for o in manifest["open_items"])
    source_rows = [(str(s), "本地事实源", str(s)) for s in manifest.get("sources", [])]
    return f'''<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>电子木鱼 · 完整硬件原理图复刻</title>
  <style>
    :root {{ color-scheme: dark; --bg:#0b1220; --panel:#111c2e; --panel2:#0f172a; --line:#26364f; --text:#e5eefb; --muted:#93a4bd; --cyan:#22d3ee; --green:#34d399; --amber:#fbbf24; --rose:#fb7185; --orange:#fb923c; }}
    * {{ box-sizing:border-box; }}
    html {{ scroll-behavior:smooth; }}
    body {{ margin:0; background:var(--bg); color:var(--text); font:14px/1.55 -apple-system,BlinkMacSystemFont,"PingFang SC","Noto Sans SC","Helvetica Neue",Arial,sans-serif; }}
    a {{ color:var(--cyan); }}
    .app {{ display:grid; grid-template-columns:280px minmax(0,1fr); min-height:100vh; }}
    aside {{ position:sticky; top:0; height:100vh; overflow:auto; padding:24px 18px; background:#0a1323; border-right:1px solid var(--line); }}
    .brand {{ font-size:20px; font-weight:800; letter-spacing:.02em; }}
    .subtitle {{ margin:6px 0 20px; color:var(--muted); font-size:12px; }}
    .status-box {{ padding:12px; border:1px solid #7f1d1d; border-radius:10px; background:rgba(127,29,29,.22); color:#fecdd3; font-size:12px; }}
    nav {{ display:grid; gap:6px; margin-top:22px; }}
    nav button, .toolbar button {{ width:100%; border:1px solid var(--line); border-radius:8px; padding:8px 10px; background:#101c30; color:var(--text); text-align:left; cursor:pointer; }}
    nav button:hover, nav button.active, .toolbar button:hover {{ border-color:var(--cyan); background:#123047; }}
    .legend {{ margin-top:22px; display:grid; gap:6px; color:var(--muted); font-size:12px; }}
    main {{ min-width:0; padding:28px clamp(18px,3vw,52px) 70px; }}
    .hero {{ max-width:1200px; }}
    h1 {{ margin:0 0 8px; font-size:clamp(26px,4vw,42px); line-height:1.15; }}
    h2 {{ margin:0; font-size:22px; }}
    h3 {{ margin:28px 0 10px; font-size:18px; }}
    .lead {{ margin:0; color:#cbd5e1; max-width:960px; }}
    .toolbar {{ display:flex; flex-wrap:wrap; gap:10px; align-items:center; margin:24px 0; padding:12px; border:1px solid var(--line); border-radius:12px; background:var(--panel); }}
    .toolbar input {{ flex:1 1 260px; min-width:180px; border:1px solid var(--line); border-radius:8px; padding:10px 12px; background:#0b1424; color:var(--text); }}
    .toolbar button {{ width:auto; text-align:center; }}
    .overview, .data-section {{ max-width:1250px; margin:24px 0; padding:18px; border:1px solid var(--line); border-radius:14px; background:linear-gradient(145deg,#111d30,#0e1727); }}
    .overview-grid {{ display:grid; grid-template-columns:repeat(auto-fit,minmax(190px,1fr)); gap:12px; margin-top:14px; }}
    .metric {{ padding:12px; border:1px solid var(--line); border-radius:10px; background:#0b1424; }}
    .metric strong {{ display:block; font-size:23px; color:var(--cyan); }}
    .metric span {{ color:var(--muted); font-size:12px; }}
    .tree {{ margin:14px 0 0; padding:14px 18px; border-left:3px solid var(--cyan); background:#0b1424; font-family:ui-monospace,SFMono-Regular,Menlo,monospace; white-space:pre-wrap; color:#cbd5e1; }}
    .sheet-section {{ max-width:1250px; margin:34px 0 48px; scroll-margin-top:24px; }}
    .sheet-heading {{ display:flex; justify-content:space-between; gap:20px; align-items:end; margin-bottom:12px; }}
    .sheet-heading > div {{ display:flex; gap:12px; align-items:center; min-width:0; }}
    .sheet-id {{ color:var(--cyan); font:700 18px ui-monospace,SFMono-Regular,Menlo,monospace; }}
    .sheet-state {{ color:var(--amber); font-size:12px; white-space:nowrap; }}
    .svg-frame {{ overflow:auto; border:1px solid var(--line); border-radius:12px; background:#0f172a; box-shadow:0 10px 35px rgba(0,0,0,.2); }}
    .sheet-svg {{ display:block; width:1800px; max-width:none; height:auto; }}
    details {{ margin-top:12px; border:1px solid var(--line); border-radius:10px; background:var(--panel2); }}
    summary {{ padding:10px 12px; cursor:pointer; color:#cbd5e1; }}
    .table-wrap {{ overflow:auto; max-height:520px; }}
    table {{ width:100%; border-collapse:collapse; font-size:12px; }}
    th, td {{ padding:8px 10px; border-bottom:1px solid #20304a; text-align:left; vertical-align:top; }}
    th {{ position:sticky; top:0; z-index:1; background:#17253b; color:#cbd5e1; }}
    td {{ color:#dbeafe; }}
    tr:hover td {{ background:#12243a; }}
    .compact td, .compact th {{ padding:6px 8px; }}
    .badge {{ display:inline-flex; align-items:center; padding:2px 7px; border-radius:99px; border:1px solid currentColor; font-size:11px; white-space:nowrap; }}
    .badge.confirmed {{ color:var(--green); background:rgba(52,211,153,.1); }}
    .badge.recommended {{ color:var(--amber); background:rgba(251,191,36,.1); }}
    .badge.prototype {{ color:var(--rose); background:rgba(251,113,133,.1); }}
    .badge.neutral {{ color:var(--muted); }}
    .callout {{ margin:14px 0; padding:12px 14px; border-left:3px solid var(--amber); background:rgba(251,191,36,.08); color:#fde68a; }}
    .source-list {{ display:grid; gap:8px; }}
    .source-list code {{ color:#cbd5e1; }}
    .hidden {{ display:none !important; }}
    footer {{ max-width:1250px; margin-top:40px; color:#64748b; font-size:12px; }}
    @media (max-width:900px) {{ .app {{ display:block; }} aside {{ position:static; height:auto; border-right:0; border-bottom:1px solid var(--line); }} nav {{ grid-template-columns:repeat(4,minmax(0,1fr)); }} nav button {{ font-size:11px; text-align:center; padding:7px 4px; }} }}
    @media print {{ :root {{ color-scheme:light; }} body {{ background:#fff; color:#111827; }} aside, .toolbar {{ display:none; }} .app {{ display:block; }} main {{ padding:0; }} .overview, .data-section, .sheet-section {{ break-inside:avoid; border:1px solid #cbd5e1; background:#fff; }} .sheet-section {{ page-break-before:always; }} .svg-frame {{ border:1px solid #94a3b8; }} .sheet-svg {{ width:100%; max-width:100%; }} details {{ break-inside:avoid; }} details[open] {{ display:block; }} table {{ color:#111827; }} th {{ background:#e5e7eb; color:#111827; }} td {{ color:#111827; border-color:#d1d5db; }} .sheet-state {{ color:#92400e; }} }}
  </style>
</head>
<body>
<div class="app">
  <aside>
    <div class="brand">电子木鱼</div>
    <div class="subtitle">完整硬件原理图 · 人工复刻版</div>
    <div class="status-box">当前状态：文档基线已整理；嘉立创页树已建但未放置元件；本页不替代 ERC、PCB DRC 或样机验收。</div>
    <nav id="sheet-nav">
      <button data-target="overview">总览 / 电源树</button>
      {''.join(f'<button data-target="{esc(p["id"])}">P{idx+1:02d} · {esc(p["title"].split(" / ")[0])}</button>' for idx,p in enumerate(pages))}
      <button data-target="data">网络 / BOM / 开放项</button>
    </nav>
    <div class="legend"><div>{badge_html('confirmed')} 接口或边界已冻结</div><div>{badge_html('recommended_initial')} 可落图初值</div><div>{badge_html('must_prototype_validate')} 必须实测后才能关闭</div></div>
  </aside>
  <main>
    <div class="hero" id="overview">
      <h1>电子木鱼 · 完整硬件原理图复刻</h1>
      <p class="lead">这是一份单文件、离线可打开的人工落图参考。每个页面首先给出采用电阻/电容/电感/二极管/运放/比较器/连接器与 IC 功能脚的标准符号 SVG 原理图，再给出网络名、逐针表、推荐初值和验证边界；手工在嘉立创 EDA 复刻时，先放置真实库元件，再按图接线并用表格逐项复核。</p>
      <div class="callout">重要：红色/粉色标记不是“故障”，而是明确告诉你该项仍需器件数据手册、实物插合、示波器、电子负载或样机测试。静态合同通过 ≠ ERC/样机通过；静态清单通过也不等价于电气安全或可制造性通过。</div>
    </div>
    <div class="toolbar"><input id="search" type="search" placeholder="搜索位号、网络名、GPIO、LCSC、开放项…"><button id="expand">展开所有明细</button><button id="collapse">收起所有明细</button><button id="print">打印 / 导出 PDF</button></div>
    <section class="overview searchable" data-title="电源树 总览">
      <h2>全局电源树与页树</h2>
      <div class="overview-grid"><div class="metric"><strong>7</strong><span>原理图页面 P01–P07</span></div><div class="metric"><strong>32</strong><span>编号 GPIO 合同 + EN</span></div><div class="metric"><strong>5</strong><span>I²C 地址（首版）</span></div><div class="metric"><strong>52</strong><span>机器网络条目</span></div><div class="metric"><strong>24</strong><span>显示 FPC 针脚</span></div><div class="metric"><strong>11</strong><span>必须样机验证开放项</span></div></div>
      <div class="tree">VBUS_5V  J1 USB-C → BQ25895 VBUS / FSW VBUS 感知
BAT_PROTECTED  J2 BATT+ → BQ BAT + TPS3424 VIN
PMID_OTG_5V  BQ PMID（OTG）→ TPS22919 VIN
VBAT_SW  TPS3424 锁存 → TPS22965 OUT → M100 VIN + TLV62569 IN + FPC pin 8（首版）
3V3  TLV62569 OUT → ESP32 / I²C / 显示逻辑 / PVDF / codec（磁珠或 0 Ω 隔离）
AUDIO_5V  TPS22919 OUT → NS4150B + WS2812C
GND  全板公共回流；BTL SPK_P/SPK_N 不接地</div>
    </section>
    {page_sections}
    <section class="data-section searchable" id="data" data-title="网络 BOM 开放项">
      <h2>机器清单对照</h2>
      <p class="lead">下表由 <code>电子木鱼-硬件网络清单.json</code> 生成；如果手工复刻时发现页面标签与此处不一致，以架构 spine、JSON 和本页明确的冻结合同为准，先记录差异再改图。</p>
      <h3>GPIO 合同</h3>{html_table(["功能", "网络", "GPIO", "方向", "状态"], gpio_rows, cls="compact")}
      <h3>I²C 地址</h3>{html_table(["器件", "7-bit 地址", "职责", "状态"], i2c_rows, cls="compact")}
      <h3>电源轨</h3>{html_table(["网络", "标称电压", "来源", "消费者", "状态"], rail_rows, cls="compact")}
      <h3>首版 BOM</h3>{html_table(["位号", "MPN", "LCSC", "角色", "状态"], bom_rows, cls="compact")}
      <h3>完整网络名</h3>{html_table(["网络", "类别", "端点", "状态"], net_rows, cls="compact")}
      <h3>开放项 / 样机验收</h3>{html_table(["编号", "开放项", "验证方法", "状态"], open_rows, cls="compact")}
      <h3>来源与追溯</h3><div class="source-list">{''.join(f'<div>· <strong>{esc(n)}</strong>（{esc(k)}）：<code>{esc(u)}</code></div>' for n,k,u in source_rows)}<div>· 显示规格书：<a href="../3593896291PY206-W38-V2(2).pdf">3593896291PY206-W38-V2(2).pdf</a></div><div>· 用户参考图（只读）：<a href="../SCH_遥控板_双TypeC_4G稳压_2026-09-07.pdf">SCH_遥控板_双TypeC_4G稳压_2026-09-07.pdf</a></div><div>· M100 载板资料页：<a href="https://yinerda.yuque.com/yt1fh6/4gdtu/yyah6wuq5r72q939">YED M100EG-C2</a></div></div>
    </section>
    <footer>生成时间：2026-09-10 · 源文件：<code>tools/generate_hardware_schematic_html.py</code> · 机器校验：<code>python3 tools/validate_hardware_manifest.py</code>。请在手工落图时保留所有“必须样机验证”备注。</footer>
  </main>
</div>
<script>
(() => {{
  const nav = document.querySelectorAll('#sheet-nav button');
  nav.forEach(btn => btn.addEventListener('click', () => {{ document.getElementById(btn.dataset.target)?.scrollIntoView({{behavior:'smooth', block:'start'}}); nav.forEach(x => x.classList.toggle('active', x===btn)); }}));
  const search = document.getElementById('search');
  search.addEventListener('input', () => {{ const q=search.value.trim().toLowerCase(); document.querySelectorAll('.searchable').forEach(el => el.classList.toggle('hidden', !!q && !el.innerText.toLowerCase().includes(q))); }});
  document.getElementById('expand').addEventListener('click', () => document.querySelectorAll('details').forEach(d => d.open=true));
  document.getElementById('collapse').addEventListener('click', () => document.querySelectorAll('details').forEach(d => d.open=false));
  document.getElementById('print').addEventListener('click', () => window.print());
}})();
</script>
</body>
</html>'''


def main() -> None:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    PAGES_DIR.mkdir(parents=True, exist_ok=True)
    # 标准符号页是 HTML/SVG 的主视图；旧的块图函数保留作生成器内部参考，
    # 不再作为交付主图，避免把“网络关系图”误读成电路原理图。
    svgs = standard_svgs()
    for page, svg in zip(manifest["schematic_pages"], svgs):
        (PAGES_DIR / f"{page['id']}.svg").write_text(svg, encoding="utf-8")
    html_text = build_html(manifest, svgs)
    HTML_PATH.write_text(html_text, encoding="utf-8")
    # 技能要求的单一 SVG 图集：七页纵向拼接，便于浏览器/矢量软件检查。
    atlas = ["<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1800 7420\" role=\"img\" aria-label=\"电子木鱼完整硬件原理图图集\">", standard_defs()]
    for idx, svg in enumerate(svgs):
        body = re.search(r"<svg[^>]*>(.*)</svg>", svg, flags=re.S).group(1)
        # 去除每页背景/defs 的重复定义，保留页面内容并纵向编排。
        body = re.sub(r"<defs>.*?</defs>", "", body, flags=re.S)
        atlas.append(f'<g transform="translate(0 {idx*1060})">{body}</g>')
    atlas.append("</svg>")
    SVG_PATH.write_text("".join(atlas), encoding="utf-8")
    print(f"写入 {HTML_PATH} ({HTML_PATH.stat().st_size} bytes)")
    print(f"写入 {SVG_PATH} ({SVG_PATH.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
