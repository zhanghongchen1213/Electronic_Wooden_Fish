#!/usr/bin/env python3
"""生成小程序 MINI-06 的纸面材质平铺图（可复现、零第三方依赖）。

材质只作低对比底纹，不承载任何信息：alpha 通道恒为纯噪声/纤维，
不接受任何按数据变化的入参，峰值 alpha 受 ceiling 约束（见 ALPHA_CEILING）。

用法：
    python3 lvgl-design/make_material_tiles.py            # 生成
    python3 lvgl-design/make_material_tiles.py --verify   # 只校验已生成资产的统计量
"""

from __future__ import annotations

import argparse
import math
import random
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "miniapp-design" / "assets" / "paper-fiber-128.png"

SIZE = 128
SEED = 20260911
ALPHA_CEILING = 16  # 8-bit alpha 上限（≈6.3%），材质须比既有 divider 更淡
INK = (44, 40, 35)  # 取自 REGISTERED 墨色族，仅作底纹不应产生色偏


def _value_noise(n: int, seed: int) -> list[list[float]]:
    rng = random.Random(seed)
    return [[rng.random() for _ in range(n)] for _ in range(n)]


def _bilerp(grid: list[list[float]], n: int, x: float, y: float) -> float:
    """双线性采样，索引按 n 取模回绕 —— 保证成品可无缝平铺。"""
    x0, y0 = int(x) % n, int(y) % n
    x1, y1 = (x0 + 1) % n, (y0 + 1) % n
    fx, fy = x - int(x), y - int(y)
    a, b = grid[y0][x0], grid[y0][x1]
    c, d = grid[y1][x0], grid[y1][x1]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def build_alpha_map(size: int) -> list[list[int]]:
    coarse = _value_noise(8, SEED)
    fine = _value_noise(32, SEED + 1)
    rows: list[list[int]] = []
    for y in range(size):
        row: list[int] = []
        for x in range(size):
            v = 0.55 * _bilerp(coarse, 8, x * 8 / size, y * 8 / size)
            v += 0.45 * _bilerp(fine, 32, x * 32 / size, y * 32 / size)
            streak = 0.5 + 0.5 * math.sin(2 * math.pi * y / size * 3.0)
            v = v * 0.72 + streak * 0.28
            alpha = int(round(max(0.0, (v - 0.25)) / 0.75 * ALPHA_CEILING))
            row.append(max(0, min(ALPHA_CEILING, alpha)))
        rows.append(row)
    return rows


def _chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def encode_png(alpha: list[list[int]]) -> bytes:
    size = len(alpha)
    raw = bytearray()
    for row in alpha:
        raw.append(0)  # filter type 0
        for a in row:
            raw.extend((*INK, a))
    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + _chunk(b"IEND", b"")
    )


def stats(alpha: list[list[int]]) -> dict[str, float]:
    flat = [a for row in alpha for a in row]
    return {
        "peak": max(flat),
        "mean": sum(flat) / len(flat),
        "p99": sorted(flat)[int(len(flat) * 0.99)],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true", help="只校验已生成资产")
    args = parser.parse_args()

    alpha = build_alpha_map(SIZE)
    st = stats(alpha)

    if args.verify:
        if not OUT.exists():
            print(f"缺少资产: {OUT}", file=sys.stderr)
            return 1
        print(f"{OUT.name}: peak {st['peak']} (ceiling {ALPHA_CEILING}), mean {st['mean']:.2f}, p99 {st['p99']}")
        ok = st["peak"] <= ALPHA_CEILING
        print("material layer within ceiling" if ok else "material layer EXCEEDS ceiling")
        return 0 if ok else 1

    OUT.parent.mkdir(parents=True, exist_ok=True)
    png = encode_png(alpha)
    OUT.write_bytes(png)
    print(f"wrote {OUT}  {len(png)} B")
    print(f"  peak alpha {st['peak']} / ceiling {ALPHA_CEILING}, mean {st['mean']:.2f}, p99 {st['p99']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
