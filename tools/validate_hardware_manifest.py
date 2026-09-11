#!/usr/bin/env python3
"""校验电子木鱼硬件网络清单。

脚本只做静态合同检查，不代表嘉立创 EDA ERC、PCB DRC 或目标板样机通过。
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


DEFAULT_MANIFEST = Path("docs/hardware/电子木鱼-硬件网络清单.json")


def _as_list(value: Any, path: str, errors: list[str]) -> list[Any]:
    if not isinstance(value, list):
        errors.append(f"{path} 必须是数组")
        return []
    return value


def _as_dict(value: Any, path: str, errors: list[str]) -> dict[str, Any]:
    if not isinstance(value, dict):
        errors.append(f"{path} 必须是对象")
        return {}
    return value


def _check_gpio(data: dict[str, Any], errors: list[str], passes: list[str]) -> None:
    validation = _as_dict(data.get("validation"), "validation", errors)
    allowed = set(validation.get("allowed_gpio_numbers", []))
    reserved = set(validation.get("reserved_gpio_numbers", []))
    assignments = _as_list(data.get("gpio"), "gpio", errors)

    seen_numbers: dict[int, str] = {}
    seen_signals: set[str] = set()
    for index, item in enumerate(assignments):
        path = f"gpio[{index}]"
        row = _as_dict(item, path, errors)
        signal = row.get("signal")
        gpio = row.get("gpio")
        number = row.get("gpio_number")
        if not isinstance(signal, str) or not signal:
            errors.append(f"{path}.signal 缺失")
        elif signal in seen_signals:
            errors.append(f"GPIO signal 重复: {signal}")
        else:
            seen_signals.add(signal)
        if number is None:
            if gpio != "EN":
                errors.append(f"{path}: 无编号 GPIO 只能是 EN，实际 {gpio!r}")
            continue
        if not isinstance(number, int):
            errors.append(f"{path}.gpio_number 必须是整数或 null")
            continue
        if number not in allowed:
            errors.append(f"{path}: IO{number} 不在 WROOM 合法 GPIO 集合")
        if number in reserved:
            errors.append(f"{path}: IO{number} 是保留脚，不得分配")
        if gpio != f"IO{number}":
            errors.append(f"{path}: gpio={gpio!r} 与 gpio_number={number} 不一致")
        if number in seen_numbers:
            errors.append(
                f"GPIO 编号重复: IO{number}（{seen_numbers[number]} 与 {signal}）"
            )
        else:
            seen_numbers[number] = str(signal)

    required = validation.get("required_gpio_signals", [])
    if not isinstance(required, list):
        errors.append("validation.required_gpio_signals 必须是数组")
    else:
        missing = sorted(set(required) - seen_signals)
        if missing:
            errors.append("缺少必需 GPIO signal: " + ", ".join(missing))
    if not errors:
        passes.append(f"GPIO 合法性/重复检查通过（{len(seen_numbers)} 个编号，EN 独立复位）")


def _parse_address(value: Any) -> int | None:
    if isinstance(value, int):
        return value
    if isinstance(value, str) and re.fullmatch(r"0x[0-9A-Fa-f]{1,2}", value):
        return int(value, 16)
    return None


def _check_i2c(data: dict[str, Any], errors: list[str], passes: list[str]) -> None:
    bus = _as_dict(data.get("i2c"), "i2c", errors)
    for key in ("bus_name", "sda_gpio", "scl_gpio", "devices"):
        if key not in bus:
            errors.append(f"i2c 缺少字段: {key}")
    devices = _as_list(bus.get("devices"), "i2c.devices", errors)
    seen: dict[int, str] = {}
    for index, item in enumerate(devices):
        path = f"i2c.devices[{index}]"
        row = _as_dict(item, path, errors)
        part = row.get("part", path)
        address = _parse_address(row.get("address_7bit"))
        if address is None or not 0x03 <= address <= 0x77:
            errors.append(f"{path}: 7-bit 地址无效: {row.get('address_7bit')!r}")
            continue
        if row.get("address_int") != address:
            errors.append(f"{path}: address_int 与 address_7bit 不一致")
        if address in seen:
            errors.append(f"I²C 地址冲突: 0x{address:02X}（{seen[address]} 与 {part}）")
        else:
            seen[address] = str(part)
    if not errors:
        passes.append(f"I²C 地址检查通过（{len(seen)} 个地址，无冲突）")


def _check_required_fields(data: dict[str, Any], errors: list[str], passes: list[str]) -> None:
    validation = _as_dict(data.get("validation"), "validation", errors)
    rails = _as_list(data.get("power_rails"), "power_rails", errors)
    rail_names = {r.get("name") for r in rails if isinstance(r, dict)}
    required_rails = validation.get("required_power_rails", [])
    missing_rails = sorted(set(required_rails) - rail_names)
    if missing_rails:
        errors.append("缺少必需电源轨: " + ", ".join(missing_rails))

    connectors = _as_list(data.get("connectors"), "connectors", errors)
    connector_refs = {c.get("reference") for c in connectors if isinstance(c, dict)}
    missing_connectors = sorted(set(validation.get("required_connectors", [])) - connector_refs)
    if missing_connectors:
        errors.append("缺少关键连接器: " + ", ".join(missing_connectors))
    for index, connector in enumerate(connectors):
        if not isinstance(connector, dict):
            continue
        if connector.get("critical") and not connector.get("pins"):
            errors.append(f"connectors[{index}] {connector.get('reference')} 缺少 pins")

    nets = _as_list(data.get("nets"), "nets", errors)
    net_names = {n.get("name") for n in nets if isinstance(n, dict)}
    missing_nets = sorted(set(validation.get("required_nets", [])) - net_names)
    if missing_nets:
        errors.append("缺少关键网络: " + ", ".join(missing_nets))

    pages = _as_list(data.get("schematic_pages"), "schematic_pages", errors)
    page_ids = {p.get("id") for p in pages if isinstance(p, dict)}
    missing_pages = sorted(set(validation.get("required_schematic_pages", [])) - page_ids)
    if missing_pages:
        errors.append("缺少原理图层次页: " + ", ".join(missing_pages))
    for index, page in enumerate(pages):
        if not isinstance(page, dict):
            continue
        for field in ("title", "components", "nets", "test_points", "eda_status"):
            if field not in page:
                errors.append(f"schematic_pages[{index}] 缺少字段: {field}")

    if not errors:
        passes.append(
            f"必需字段检查通过（{len(rail_names)} 电源轨、{len(connector_refs)} 连接器、{len(net_names)} 网络、{len(page_ids)} 层次页）"
        )


def _check_constraints(data: dict[str, Any], errors: list[str], passes: list[str]) -> None:
    bom = _as_list(data.get("bom"), "bom", errors)
    refs: set[str] = set()
    forbidden = {str(x).upper() for x in data.get("constraints", {}).get("forbidden_components", [])}
    for index, item in enumerate(bom):
        if not isinstance(item, dict):
            errors.append(f"bom[{index}] 必须是对象")
            continue
        ref = item.get("reference")
        if not isinstance(ref, str) or not ref:
            errors.append(f"bom[{index}] 缺少 reference")
        elif ref in refs:
            errors.append(f"BOM reference 重复: {ref}")
        else:
            refs.add(ref)
        part = item.get("part")
        if isinstance(part, str) and part.upper() in forbidden:
            errors.append(f"BOM 使用禁止器件: {part}")

    net_by_name = {
        n.get("name"): n for n in data.get("nets", []) if isinstance(n, dict) and n.get("name")
    }
    m100 = net_by_name.get("M100_VIN", {})
    endpoints = [str(x) for x in m100.get("endpoints", [])]
    if any("BQ_SYS" in endpoint for endpoint in endpoints):
        errors.append("M100_VIN 端点包含 BQ_SYS，违反 4G 供电边界")
    if any("BQ_SYS" in str(x) for x in m100.get("forbidden_sources", [])) is False:
        errors.append("M100_VIN 未声明 BQ_SYS 为禁止来源")
    io8 = next((g for g in data.get("gpio", []) if isinstance(g, dict) and g.get("gpio") == "IO8"), None)
    if not io8 or io8.get("signal") != "BQ_OTG_EN":
        errors.append("IO8 必须唯一映射为 BQ_OTG_EN")
    gnss = net_by_name.get("M100_GNSS_VCC_NC", {})
    if any("IO8" in str(x) for x in gnss.get("endpoints", [])):
        errors.append("M100_GNSS_VCC_NC 不得连接 IO8")
    if not errors:
        passes.append("电源/USB/IO8 边界检查通过（BQ_SYS 未供给 M100，GNSS_VCC 未接 IO8）")


def _check_fpc(data: dict[str, Any], errors: list[str], passes: list[str]) -> None:
    connector = next(
        (c for c in data.get("connectors", []) if isinstance(c, dict) and c.get("reference") == "J3"),
        None,
    )
    if connector is None:
        errors.append("J3 FPC 连接器不存在")
        return
    pins = connector.get("pins", [])
    numbers = [p.get("pin") for p in pins if isinstance(p, dict)]
    if numbers != list(range(1, 25)):
        errors.append("J3 FPC 必须按 1..24 顺序列出 24 个针脚")
    for pin in ("LCD_RST", "LCD_CS", "LCD_SCLK", "LCD_D0", "LCD_D1", "LCD_D2", "LCD_D3", "LCD_EN", "TP_RST", "TP_INT"):
        if not any(isinstance(p, dict) and p.get("net") == pin for p in pins):
            errors.append(f"J3 FPC 缺少网络: {pin}")
    if not errors:
        passes.append("24-pin FPC 针脚闭合检查通过")


def validate(path: Path) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    passes: list[str] = []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return [f"找不到清单文件: {path}"], []
    except json.JSONDecodeError as exc:
        return [f"JSON 解析失败: line {exc.lineno} column {exc.colno}: {exc.msg}"], []
    if not isinstance(data, dict):
        return ["清单顶层必须是 JSON 对象"], []
    for key in ("schema_version", "manifest_type", "gpio", "i2c", "power_rails", "connectors", "nets", "bom", "open_items"):
        if key not in data:
            errors.append(f"顶层缺少字段: {key}")
    _check_gpio(data, errors, passes)
    _check_i2c(data, errors, passes)
    _check_required_fields(data, errors, passes)
    _check_constraints(data, errors, passes)
    _check_fpc(data, errors, passes)
    return errors, passes


def main() -> int:
    parser = argparse.ArgumentParser(description="校验 EWF 硬件网络清单")
    parser.add_argument("manifest", nargs="?", type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args()
    errors, passes = validate(args.manifest)
    for message in passes:
        print(f"PASS: {message}")
    if errors:
        for message in errors:
            print(f"FAIL: {message}", file=sys.stderr)
        print(f"硬件清单校验失败：{len(errors)} 个错误", file=sys.stderr)
        return 1
    print("硬件清单校验通过：未发现静态合同错误。")
    print("注意：这不等价于嘉立创 EDA ERC 或样机验证通过。")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

