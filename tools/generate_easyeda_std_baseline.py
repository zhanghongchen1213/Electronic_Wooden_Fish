#!/usr/bin/env python3
"""生成可导入嘉立创 EDA 专业版的标准版原理图 JSON。

这些文件用于把已经审核过的网络合同批量放入网页端空白图页。
它们不是 ERC/PCB/样机验证结果；所有需要实测的初值仍在硬件基线中标记。
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "hardware" / "lceda" / "import"


class Ids:
    def __init__(self) -> None:
        self.n = 0

    def take(self, prefix: str = "gge") -> str:
        self.n += 1
        return f"{prefix}{self.n:05d}"


ids = Ids()


def txt(x: float, y: float, value: str, *, color: str = "#0000FF", size: str = "9pt", bold: str = "normal") -> str:
    ident = ids.take()
    return f"T~L~{x}~{y}~0~{color}~Arial~{size}~{bold}~normal~~comment~{value}~1~start~{ident}"


def label(x: float, y: float, value: str) -> str:
    ident = ids.take()
    return f"N~{x}~{y}~0~#000080~{value}~{ident}~start~{x + 3}~{y}~Verdana~7pt"


def wire(points: Iterable[tuple[float, float]]) -> str:
    ident = ids.take()
    coords = " ".join(f"{x:g} {y:g}" for x, y in points)
    return f"W~{coords}~#008800~1~0~none~{ident}"


def no_connect(x: float, y: float) -> str:
    ident = ids.take()
    return f"O~{x}~{y}~{ident}~M{x-4},{y-4} L{x+4},{y+4} M{x+4},{y-4} L{x-4},{y+4}~#FF0000"


def pin_shape(
    x: float,
    y: float,
    side: str,
    number: int,
    name: str,
    body_w: float,
    body_h: float,
) -> tuple[str, tuple[float, float]]:
    """返回 LIB 内嵌 PIN 字符串及其外部端点。"""
    ident = ids.take("pin")
    if side == "left":
        px, py, rotation, toward = x - body_w / 2 - 20, y, 0, 20
        name_x, name_anchor = px + 4, "start"
        num_x, num_anchor = px + 4, "start"
        path = f"M {px:g} {py:g} h {toward:g}"
    elif side == "right":
        px, py, rotation, toward = x + body_w / 2 + 20, y, 180, -20
        name_x, name_anchor = px - 4, "end"
        num_x, num_anchor = px - 4, "end"
        path = f"M {px:g} {py:g} h {toward:g}"
    elif side == "top":
        px, py, rotation, toward = x, y - body_h / 2 - 20, 270, 20
        name_x, name_anchor = px + 4, "start"
        num_x, num_anchor = px + 4, "start"
        path = f"M {px:g} {py:g} v {toward:g}"
    else:
        px, py, rotation, toward = x, y + body_h / 2 + 20, 90, -20
        name_x, name_anchor = px + 4, "start"
        num_x, num_anchor = px + 4, "start"
        path = f"M {px:g} {py:g} v {toward:g}"
    # 该写法沿用 EasyEDA Std 文档中的 P~...^^... 结构，保留名称、编号和图形段。
    pin = (
        f"P~show~0~{number}~{px:g}~{py:g}~{rotation}~{ident}"
        f"^^{px:g}~{py:g}^^{path}~#880000"
        f"^^0~{name_x:g}~{py:g}~0~1~{name_anchor}~~"
        f"^^0~{num_x:g}~{py-8:g}~0~{number}~{num_anchor}~~^^~~"
    )
    return pin, (px, py)


def lib_component(
    x: float,
    y: float,
    reference: str,
    value: str,
    pins: list[tuple[str, str, str]],
    *,
    prefix: str = "U",
    package: str = "CUSTOM",
    symbol_name: str = "Generic_IC",
) -> tuple[str, list[tuple[float, float, str]]]:
    """生成一个带矩形和引脚的自包含 LIB 图元。

    pins 项为 (side, name/number, net)，number 使用字符串以允许 NC/电源脚。
    引脚按 side 平均分布，返回端点供外层导线和网络标签使用。
    """
    left = [p for p in pins if p[0] == "left"]
    right = [p for p in pins if p[0] == "right"]
    top = [p for p in pins if p[0] == "top"]
    bottom = [p for p in pins if p[0] == "bottom"]
    rows = max(len(left), len(right), 1)
    body_w = 150.0
    body_h = max(70.0, rows * 18.0 + 20.0)
    ident = ids.take("lib")
    fields = [
        f"LIB~{x:g}~{y:g}~package`{package}`nameAlias`Value`Value`{value}`spicePre`{prefix}`spiceSymbolName`{symbol_name}`~~0~{ident}",
        f"R~{x-body_w/2:g}~{y-body_h/2:g}~0~0~{body_w:g}~{body_h:g}~#880000~1~0~none~{ids.take()}",
        f"T~N~{x-body_w/2+5:g}~{y-body_h/2-9:g}~0~#000080~Arial~8pt~bold~normal~~comment~{value}~1~start~{ids.take()}",
        f"T~P~{x-body_w/2+5:g}~{y+body_h/2+14:g}~0~#000080~Arial~8pt~normal~normal~~comment~{reference}~1~start~{ids.take()}",
    ]
    endpoints: list[tuple[float, float, str]] = []

    def add_side(items: list[tuple[str, str, str]], side: str) -> None:
        for idx, (_, pin_name, net) in enumerate(items):
            if side in ("left", "right"):
                offset = (idx - (len(items) - 1) / 2) * 18.0
                px, py = x, y + offset
            else:
                offset = (idx - (len(items) - 1) / 2) * 18.0
                px, py = x + offset, y
            try:
                number = int(pin_name)
            except ValueError:
                number = idx + 1
            shape, endpoint = pin_shape(px, py, side, number, pin_name, body_w, body_h)
            fields.append(shape)
            endpoints.append((endpoint[0], endpoint[1], net))

    add_side(left, "left")
    add_side(right, "right")
    add_side(top, "top")
    add_side(bottom, "bottom")
    return "#@$".join(fields), endpoints


def place_component(
    shapes: list[str],
    x: float,
    y: float,
    reference: str,
    value: str,
    pins: list[tuple[str, str, str]],
    **kwargs: str,
) -> None:
    lib, endpoints = lib_component(x, y, reference, value, pins, **kwargs)
    shapes.append(lib)
    for ex, ey, net in endpoints:
        if net == "NC":
            shapes.append(no_connect(ex, ey))
        else:
            # 短引线让网络名与引脚有明确的电气接触点，避免只放文字造成悬空误读。
            if ex < x - 1:
                shapes.append(wire([(ex, ey), (ex - 20, ey)]))
                lx = ex - 20
            elif ex > x + 1:
                shapes.append(wire([(ex, ey), (ex + 20, ey)]))
                lx = ex + 20
            else:
                shapes.append(wire([(ex, ey), (ex, ey + (20 if ey > y else -20))]))
                lx = ex
            shapes.append(label(lx, ey, net))


def page(title: str, notes: list[str], components: callable) -> dict:
    shapes: list[str] = []
    shapes.append(txt(70, 55, title, color="#7A1FA2", size="14pt", bold="bold"))
    y = 78
    for note in notes:
        shapes.append(txt(70, y, note, color="#555555", size="8pt"))
        y += 18
    components(shapes)
    return {
        "head": "1~1.11.3~~TRAN`1u`1m`0`{AC`dec``0`0`{DC`0``0`{TF```",
        "canvas": "CA~1600~1000~#FFFFFF~yes~#CCCCCC~10~1600~1000~line~10~pixel~5",
        "shape": shapes,
        "BBox": {"x": 50, "y": 30, "width": 1450, "height": 900},
        "colors": {},
    }


def make_p01() -> dict:
    def build(s: list[str]) -> None:
        place_component(s, 180, 300, "J1", "TYPE-C-304-ACP16", [
            ("left", "1", "VBUS_5V"), ("left", "2", "USB_DP_C"), ("left", "3", "USB_DM_C"),
            ("left", "4", "USB_CC1"), ("left", "5", "USB_CC2"), ("right", "6", "GND"),
        ], prefix="J", package="USB-C-16P", symbol_name="Connector")
        place_component(s, 180, 560, "J2", "B3B-PH-K-S(LF)(SN)", [
            ("left", "1", "BAT_PROTECTED"), ("left", "2", "GND"), ("left", "3", "BATT_NTC"),
        ], prefix="J", package="JST-PH-3P", symbol_name="Connector")
        place_component(s, 480, 300, "U1", "BQ25895RTWR", [
            ("left", "1", "VBUS_5V"), ("left", "2", "BAT_PROTECTED"), ("left", "3", "BQ_SYS"),
            ("left", "4", "PMID_OTG_5V"), ("right", "5", "I2C_SDA"), ("right", "6", "I2C_SCL"),
            ("right", "7", "BQ_OTG_EN"), ("right", "8", "USB_DP_BQ"), ("right", "9", "USB_DM_BQ"),
            ("top", "10", "GND"), ("bottom", "11", "REGN"), ("bottom", "12", "BTST"),
        ], prefix="U", package="WQFN-24", symbol_name="BQ25895")
        place_component(s, 810, 250, "U2", "TPS3424A11C13ADRLR", [
            ("left", "1", "BAT_PROTECTED"), ("left", "2", "PWR_BUTTON"), ("right", "3", "PWR_LATCH"),
            ("right", "4", "PWR_INT_N"), ("bottom", "5", "GND"),
        ], prefix="U", package="VSON-8", symbol_name="Pushbutton_Controller")
        place_component(s, 1080, 250, "U3", "TPS22965DSGR", [
            ("left", "1", "BAT_PROTECTED"), ("left", "2", "PWR_LATCH"), ("right", "3", "VBAT_SW"),
            ("bottom", "4", "SYS_PRE"), ("bottom", "5", "GND"),
        ], prefix="U", package="WSON-8", symbol_name="Load_Switch")
        place_component(s, 1080, 560, "U4", "TLV62569DBVR", [
            ("left", "1", "VBAT_SW"), ("left", "2", "GND"), ("right", "3", "3V3"),
            ("right", "4", "3V3_FB"), ("bottom", "5", "3V3_EN"),
        ], prefix="U", package="SOT-23-5", symbol_name="Buck_Regulator")
        place_component(s, 810, 560, "U5", "TPS22919DCKR", [
            ("left", "1", "PMID_OTG_5V"), ("left", "2", "AUDIO_EN"), ("right", "3", "AUDIO_5V"),
            ("bottom", "4", "GND"),
        ], prefix="U", package="SC-70-6", symbol_name="Load_Switch")
        place_component(s, 480, 650, "U6", "FSW7227YMS10G/TR", [
            ("left", "1", "USB_DP_C"), ("left", "2", "USB_DM_C"), ("right", "3", "USB_DP_BQ"),
            ("right", "4", "USB_DM_BQ"), ("top", "5", "USB_DP_ESP"), ("top", "6", "USB_DM_ESP"),
            ("bottom", "7", "USB_MUX_SEL"), ("bottom", "8", "USB_MUX_EN_N"), ("bottom", "9", "VBUS_5V"),
            ("bottom", "10", "GND"),
        ], prefix="U", package="MSOP-10", symbol_name="USB2_Mux")
        for x, val in ((280, "CC1 5.1k Rd"), (340, "CC2 5.1k Rd"), (560, "PMID 60uF / REGN 4.7uF"), (700, "RILIM 180R / ICHG 1.5A"), (1210, "2.2uH / FB 453k+100k")):
            s.append(txt(x, 820, val, color="#00695C", size="8pt"))

    return page("P01  电源、充电、USB MUX 与硬件电源锁存", [
        "已确认：BQ25895 + TPS3424 + TPS22965；VBAT_SW 不得来自 BQ_SYS。",
        "推荐初值：RILIM≈180Ω、ICHG≈1.5A、PMID 60uF、USB CC=5.1kΩ Rd。",
        "样机验证：USB 异常电压、OTG 启动、上电顺序、4G 峰值掉压和强制选路互斥。",
    ], build)


def make_p02() -> dict:
    def build(s: list[str]) -> None:
        gpio = [
            ("left", "0", "BOOT0"), ("left", "1", "I2C_SDA"), ("left", "2", "I2C_SCL"),
            ("left", "3", "RGB_DATA"), ("left", "4", "LCD_RST"), ("left", "5", "LCD_SCLK"),
            ("left", "6", "LCD_D0"), ("left", "7", "LCD_D1"), ("left", "8", "BQ_OTG_EN"),
            ("left", "9", "PVDF_ADC"), ("left", "10", "M100_DTR"), ("left", "11", "PVDF_CMP_WAKE"),
            ("left", "12", "LCD_D2"), ("left", "13", "I2S_DOUT"), ("left", "14", "I2S_WS"),
            ("left", "15", "M100_RST"), ("right", "16", "M100_NET_STATUS_COMPAT"), ("right", "17", "I2S_DIN"),
            ("right", "18", "I2S_BCLK"), ("right", "19", "USB_D-"), ("right", "20", "USB_D+"),
            ("right", "21", "I2S_MCLK"), ("right", "38", "TP_RST"), ("right", "39", "TP_INT"),
            ("right", "40", "LCD_CS"), ("right", "41", "IMU_INT1"), ("right", "42", "LCD_D3"),
            ("right", "43", "M100_TXD"), ("right", "44", "M100_RXD"), ("right", "45", "NC"),
            ("right", "46", "PWR_STATE"), ("right", "47", "LCD_EN"), ("right", "48", "PA_EN"),
            ("top", "49", "3V3"), ("top", "50", "GND"),
        ]
        place_component(s, 760, 430, "U7", "ESP32-S3-WROOM-1-N16R8", gpio, prefix="U", package="MODULE-41P", symbol_name="ESP32_S3_WROOM")
        place_component(s, 300, 300, "SW1", "BOOT", [("left", "1", "BOOT0"), ("right", "2", "GND")], prefix="SW", package="TACT-SW", symbol_name="Switch")
        place_component(s, 300, 500, "SW2", "RESET", [("left", "1", "RESET_N"), ("right", "2", "GND")], prefix="SW", package="TACT-SW", symbol_name="Switch")
        s.append(txt(640, 790, "GPIO0/46 为启动绑带；IO35–37、IO45、IO22–34 不使用", color="#C62828", size="9pt"))
        s.append(txt(640, 815, "USB-Serial-JTAG：IO19=D−，IO20=D+；不启用 USB-OTG", color="#00695C", size="8pt"))

    return page("P02  ESP32-S3 主控、启动/复位与 USB-Serial-JTAG", [
        "GPIO 合同来自 ARCHITECTURE-SPINE；IO8= BQ_OTG_EN，IO16=兼容逻辑 NC。",
        "主控模块保留 PCB 天线、16MB Flash/8MB PSRAM；电源去耦按模块手册补齐。",
    ], build)


def make_p03() -> dict:
    def build(s: list[str]) -> None:
        fpc_nets = [
            "3V3", "I2C_SCL", "I2C_SDA", "TP_INT", "TP_RST", "GND", "3V3", "LCD_VBAT", "NC", "3V3", "GND",
            "LCD_RST", "GND", "LCD_D0", "GND", "LCD_SCLK", "GND", "LCD_D3", "NC", "LCD_D2", "LCD_CS", "LCD_D1", "LCD_EN", "LCD_TE",
        ]
        pins = []
        for idx, net in enumerate(fpc_nets, 1):
            side = "left" if idx <= 12 else "right"
            pins.append((side, str(idx), net))
        place_component(s, 420, 450, "J3", "FH34SRJ-24S-0.5SH(50)", pins, prefix="J", package="FPC-24-0.5", symbol_name="FPC_24P")
        place_component(s, 950, 320, "U9", "CO5300AF-51", [
            ("left", "1", "LCD_RST"), ("left", "2", "LCD_CS"), ("left", "3", "LCD_SCLK"), ("left", "4", "LCD_D0"),
            ("left", "5", "LCD_D1"), ("left", "6", "LCD_D2"), ("left", "7", "LCD_D3"), ("left", "8", "LCD_EN"),
            ("right", "9", "LCD_VBAT"), ("right", "10", "3V3"), ("right", "11", "GND"),
        ], prefix="U", package="CO5300", symbol_name="AMOLED_Controller")
        place_component(s, 950, 650, "U10", "CST9217", [
            ("left", "1", "I2C_SDA"), ("left", "2", "I2C_SCL"), ("left", "3", "TP_RST"), ("left", "4", "TP_INT"),
            ("right", "5", "3V3"), ("right", "6", "GND"),
        ], prefix="U", package="CST9217", symbol_name="Touch_Controller")
        s.append(txt(70, 830, "24-pin FPC：第 8 脚首版接 VBAT_SW；正反面、接触方向、OLED_EN 有效极性必须实物核对。", color="#C62828", size="8pt"))

    return page("P03  PY206-W38-V2 / CO5300 / CST9217 24-pin FPC", [
        "目标资料：docs/hardware/3593896291PY206-W38-V2(2).pdf，第 3–4 页针脚表。",
        "连接器首选 FH34SRJ-24S-0.5SH(50)，LCSC C324726；封装高度/双面接触待实物验证。",
    ], build)


def make_p04() -> dict:
    def build(s: list[str]) -> None:
        place_component(s, 380, 320, "U11", "ES8311", [
            ("left", "1", "I2C_SDA"), ("left", "2", "I2C_SCL"), ("left", "3", "I2S_MCLK"),
            ("left", "4", "I2S_BCLK"), ("left", "5", "I2S_WS"), ("left", "6", "I2S_DOUT"), ("left", "7", "I2S_DIN"),
            ("right", "8", "CODEC_3V3"), ("right", "9", "CODEC_OUTP"), ("right", "10", "CODEC_OUTN"), ("bottom", "11", "GND"),
        ], prefix="U", package="QFN-20", symbol_name="ES8311")
        place_component(s, 820, 320, "U12", "NS4150B", [
            ("left", "1", "CODEC_OUTP"), ("left", "2", "CODEC_OUTN"), ("left", "3", "PA_EN"),
            ("right", "4", "SPK_P"), ("right", "5", "SPK_N"), ("top", "6", "AUDIO_5V"), ("bottom", "7", "GND"),
        ], prefix="U", package="MSOP-8", symbol_name="NS4150B")
        place_component(s, 1190, 320, "J5", "SPEAKER_4R_3W", [("left", "1", "SPK_P"), ("left", "2", "SPK_N")], prefix="J", package="JST-PH-2P", symbol_name="Speaker")
        place_component(s, 820, 650, "LED1", "WS2812C-2020-V1", [("left", "1", "RGB_DATA"), ("top", "2", "AUDIO_5V"), ("bottom", "3", "GND"), ("right", "4", "NC")], prefix="LED", package="LED-2020-4P", symbol_name="WS2812C")
        for x, val in ((180, "ES8311：100nF/1uF 去耦，输出 1uF + 0R"), (520, "NS4150B：输入 100nF，VCC 1uF+22uF+22uF"), (930, "PA_EN 100kΩ 下拉；BTL 输出不接地")):
            s.append(txt(x, 820, val, color="#00695C", size="8pt"))
        s.append(txt(70, 850, "音频时序：AUDIO_5V 有效→等待≥30ms→PA_EN→等待约150ms→PCM；充电期间暂停。", color="#C62828", size="8pt"))

    return page("P04  ES8311 音频 Codec、NS4150B BTL 功放、扬声器与 WS2812C", [
        "CODEC_3V3 常供，经磁珠/0Ω 与主 3V3 隔离；PA_EN=IO48，默认关闭。",
        "扬声器冻结为 4Ω/3W；输出 SPK_P/SPK_N 不接地。LED 首版优先 3.3V 兼容输入。",
    ], build)


def make_p05() -> dict:
    def build(s: list[str]) -> None:
        place_component(s, 220, 430, "J6", "LDT0-028K", [("left", "1", "PVDF_RAW"), ("left", "2", "GND")], prefix="J", package="PVDF-2P", symbol_name="PVDF")
        place_component(s, 520, 430, "R_PVDF_SER", "1MΩ", [("left", "1", "PVDF_RAW"), ("right", "2", "PVDF_AC")], prefix="R", package="R-0603", symbol_name="Resistor")
        place_component(s, 820, 300, "U15", "TLV2369IDGKR", [
            ("left", "1", "PVDF_AC"), ("left", "2", "VBIAS"), ("right", "3", "PVDF_BUF"), ("top", "4", "3V3"), ("bottom", "5", "GND"),
        ], prefix="U", package="VSSOP-8", symbol_name="OpAmp")
        place_component(s, 820, 620, "U16", "TLV7042DDFR", [
            ("left", "1", "PVDF_BUF"), ("left", "2", "VBIAS_HI"), ("left", "3", "VBIAS_LO"),
            ("right", "4", "PVDF_CMP_WAKE"), ("top", "5", "3V3"), ("bottom", "6", "GND"),
        ], prefix="U", package="SOT-23-8", symbol_name="Comparator")
        place_component(s, 1160, 300, "R_PVDF_ADC", "1kΩ", [("left", "1", "PVDF_BUF"), ("right", "2", "PVDF_ADC")], prefix="R", package="R-0603", symbol_name="Resistor")
        place_component(s, 1160, 620, "R_PVDF_BIAS", "10MΩ", [("left", "1", "PVDF_AC"), ("right", "2", "VBIAS")], prefix="R", package="R-0603", symbol_name="Resistor")
        s.append(txt(70, 820, "VBIAS≈1.65V（100k/100k + TLV2369 跟随）；比较窗口 VBIAS±150mV，开漏线与 100k 上拉。", color="#00695C", size="8pt"))
        s.append(txt(70, 850, "钳位、阈值、滞回、消抖和 1M/10M 实际值均为可调初值，必须用目标木鱼壳体和示波器关闭。", color="#C62828", size="8pt"))

    return page("P05  LDT0-028K PVDF 限流、偏置、钳位、比较器与 ADC", [
        "IO9=ADC1_CH8 二次确认；IO11 仅低功耗唤醒，事件来源 physical_pvdf。",
        "低漏钳位不画成默认 TVS；按器件漏电和 PVDF 高压瞬态实测选型。",
    ], build)


def make_p06() -> dict:
    def build(s: list[str]) -> None:
        place_component(s, 520, 420, "U8", "M100EG-C2 / Air780EGP", [
            ("left", "1", "M100_VIN"), ("left", "2", "GND"), ("left", "3", "M100_RXD"), ("left", "4", "M100_TXD"),
            ("right", "5", "M100_DTR"), ("right", "6", "M100_RST"), ("right", "7", "M100_NET_STATUS_COMPAT"), ("right", "8", "M100_GNSS_VCC_NC"),
            ("top", "9", "M100_SIM"), ("top", "10", "M100_ANT"),
        ], prefix="U", package="M100-CARRIER-25mm", symbol_name="Air780EGP_Carrier")
        place_component(s, 960, 250, "J4", "M100_TEST_HEADER", [
            ("left", "1", "M100_VIN"), ("left", "2", "GND"), ("left", "3", "M100_TXD"), ("left", "4", "M100_RXD"),
            ("right", "5", "M100_DTR"), ("right", "6", "M100_RST"), ("right", "7", "M100_NET_STATUS_COMPAT"), ("right", "8", "M100_GNSS_VCC_NC"),
        ], prefix="J", package="HDR-2.54-8P", symbol_name="Test_Header")
        s.append(txt(70, 780, "main_control gps.h 仅作信号名称交叉：旧 GPIO7/15/16/17/18/8 不直接复制。", color="#C62828", size="8pt"))
        s.append(txt(70, 805, "EWF：UART TX/RX=IO43/44，DTR=IO10，RST=IO15；IO16 兼容 NC/TP；GNSS_VCC 释放，IO8= BQ_OTG_EN。", color="#00695C", size="8pt"))
        s.append(txt(70, 830, "M100 RST 按低有效开漏候选落图，极性必须首块样机测量；载板 USB 仅测试点，不并入主 USB。", color="#C62828", size="8pt"))

    return page("P06  Air780EGP / M100EG-C2 载板、UART、DTR/RST 与测试点", [
        "M100 VIN 只接 VBAT_SW（3.3–4.2V 电池轨），不能接 BQ_SYS 或 5V。",
        "RI/1PPS、载板 USB、NET_STATUS/GNSS_VCC 物理针脚按载板版本资料和实物确认。",
    ], build)


def make_p07() -> dict:
    def build(s: list[str]) -> None:
        place_component(s, 480, 360, "U13", "CW2015", [
            ("left", "1", "I2C_SDA"), ("left", "2", "I2C_SCL"), ("right", "3", "3V3"), ("bottom", "4", "GND"),
        ], prefix="U", package="DFN-8", symbol_name="Fuel_Gauge")
        place_component(s, 950, 360, "U14", "QMI8658C", [
            ("left", "1", "I2C_SDA"), ("left", "2", "I2C_SCL"), ("right", "3", "IMU_INT1"), ("right", "4", "NC"),
            ("top", "5", "3V3"), ("bottom", "6", "GND"),
        ], prefix="U", package="LGA-16", symbol_name="QMI8658C")
        place_component(s, 700, 650, "R_I2C_PU", "4.7kΩ x2", [("left", "1", "3V3"), ("right", "2", "I2C_SDA"), ("bottom", "3", "I2C_SCL")], prefix="R", package="0603_ARRAY", symbol_name="Pullup")
        s.append(label(240, 700, "I2C_SDA"))
        s.append(label(240, 730, "I2C_SCL"))
        s.append(txt(70, 805, "共享 7-bit 地址：BQ25895 0x6A、QMI8658C 0x6B（SA0 高）、CW2015 0x62、CST9217 0x5A、ES8311 0x18。", color="#00695C", size="8pt"))
        s.append(txt(70, 830, "QMI INT2 不接、不占 IO45；上电扫描若地址不符，先改基线/清单再改固件。", color="#C62828", size="8pt"))

    return page("P07  共享 I²C：CW2015、QMI8658C 与总线测试点", [
        "I2C0：SDA=IO1，SCL=IO2；统一上拉、单一仲裁，推荐初始 100kHz。",
        "QMI8658C 首版贴装但业务默认不启用，仅保留 INT1→IO41。",
    ], build)


PAGES = {
    "P01_POWER_USB": make_p01,
    "P02_MCU_USB": make_p02,
    "P03_DISPLAY_TOUCH": make_p03,
    "P04_AUDIO_RGB": make_p04,
    "P05_PVDF": make_p05,
    "P06_MODEM": make_p06,
    "P07_SHARED_SENSORS": make_p07,
}


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for page_id, factory in PAGES.items():
        path = OUT / f"{page_id}.json"
        path.write_text(json.dumps(factory(), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"generated {path} ({path.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
