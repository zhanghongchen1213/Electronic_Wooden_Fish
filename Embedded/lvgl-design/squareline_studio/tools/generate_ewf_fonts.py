#!/usr/bin/env python3
"""EWF fonts-only 生成并同步到 generated/fonts（禁止手改 .c 位图）。"""
from __future__ import annotations
import argparse, hashlib, json, re, shutil, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]  # Embedded/
SL = ROOT / "lvgl-design/squareline_studio"
FONT_DIR = SL / "assets/Fonts"
FONT_CONV = Path("/Applications/SquareLine_Studio.app/Contents/MacOS/lvgl/lv_font_conv-osx")


def _ui_chars() -> set[str]:
    """UI 文案字符闭包（不含经文正文）。新增页面文案时同步补字。"""
    ui = set(
        "木鱼经文统计设置同步中待同步同步失败已同步电量低电故障未校时今日累计"
        "心经进度音量亮度熄屏立即同步关闭开启待校时第次诵读敲击本次已完成进行中"
        "字屏幕自动版本高低秒本轮完成从头开始退出充暂停"
    )
    punct = set("，。：%/-+·")
    ascii_dyn = set(chr(c) for c in range(0x20, 0x7F))
    return ui | punct | ascii_dyn


def symbols_full() -> str:
    """《心经》可消费汉字 + UI 字符集（字带/经文行/glyph 大字档）。"""
    canon = ROOT.parent / "docs/contracts/canonical/heart-sutra.txt"
    text = canon.read_text(encoding="utf-8")
    han = set(re.findall(r"[一-鿿]", text))
    return "".join(sorted(han | _ui_chars()))


def symbols_ui() -> str:
    """仅 UI 字符集（状态栏/标签/数值/设置等小字号档），控制 flash 体积。"""
    return "".join(sorted(_ui_chars()))


def instantiate_serif(vf: Path, weight: int, out: Path) -> None:
    subprocess.run(
        ["fonttools", "varLib.instancer", str(vf), f"wght={weight}", "-o", str(out)],
        check=True,
        capture_output=True,
        text=True,
    )


# (name, source, px, weight, symbols) —— symbols: full=经文+UI，ui=仅 UI。
# 档位对照 ewf-device-ui-export.html（UI 对齐批次 2026-10）：
#   ns= Noto Sans SC，serif= Noto Serif SC；字重 500/600/700 对应设计 medium/semibold/bold。
OUTS: list[tuple[str, str, int, int | None, str]] = [
    # —— 遗留档（仍在引用）——
    ("ui_font_ns600_16", "NotoSansSC-600.ttf", 16, None, "full"),
    ("ui_font_ns700_22", "NotoSansSC-700.ttf", 22, None, "full"),
    ("ui_font_serif700_52", "NotoSerifSC-700.ttf", 52, 700, "full"),
    # —— 新增档 ——
    ("ui_font_ns500_11", "NotoSansSC-500.ttf", 11, None, "ui"),    # 充电横幅
    ("ui_font_ns500_12", "NotoSansSC-500.ttf", 12, None, "ui"),    # sync-label / round-index / progress label
    ("ui_font_ns500_13", "NotoSansSC-500.ttf", 13, None, "ui"),    # stat label/unit / reading labels / modal summary / battery value
    ("ui_font_ns600_14", "NotoSansSC-600.ttf", 14, None, "ui"),    # modal 按钮
    ("ui_font_ns500_15", "NotoSansSC-500.ttf", 15, None, "ui"),    # today/total-taps
    ("ui_font_ns600_18", "NotoSansSC-600.ttf", 18, None, "ui"),    # 音量值 / 版本·ID 值
    ("ui_font_ns600_20", "NotoSansSC-600.ttf", 20, None, "ui"),    # 设置行标题
    ("ui_font_ns600_22", "NotoSansSC-600.ttf", 22, None, "ui"),    # 统计/设置 Sans 标题
    ("ui_font_ns700_26", "NotoSansSC-700.ttf", 26, None, "ui"),    # stat value
    ("ui_font_serif600_22", "NotoSerifSC-600.ttf", 22, 600, "ui"),  # 心经页标题（UI 子集即可）
    ("ui_font_serif700_24", "NotoSerifSC-700.ttf", 24, 700, "ui"),  # modal 标题
    ("ui_font_serif500_24", "NotoSerifSC-500.ttf", 24, 500, "full"),  # 字带槽 / 经文行
    ("ui_font_serif700_48", "NotoSerifSC-700.ttf", 48, 700, "full"),  # MUYU glyph-current
]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--fonts-only", action="store_true")
    ap.add_argument("--sync-generated-fonts", type=Path, required=True)
    ap.add_argument("--serif-source", type=Path, default=Path("/tmp/NotoSerifSC-VF.ttf"))
    ap.add_argument("--font-conv", type=Path, default=FONT_CONV)
    ap.add_argument("--only", default="",
                    help="逗号分隔的档位名单，仅生成这些档（分批跑规避 lv_font_conv 连续崩溃）；空=全部")
    args = ap.parse_args()
    if not args.fonts_only:
        print("本工具仅支持 --fonts-only", file=sys.stderr)
        return 2
    if not args.font_conv.exists():
        print(f"缺少 lv_font_conv: {args.font_conv}", file=sys.stderr)
        return 1
    syms_full = symbols_full()
    syms_ui = symbols_ui()
    serif_vf = args.serif_source if args.serif_source.exists() else None

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        serif_fonts: dict[int, Path] = {}
        if serif_vf is not None:
            for weight in (400, 500, 600, 700):
                out = tmp_path / f"NotoSerifSC-{weight}.ttf"
                instantiate_serif(serif_vf, weight, out)
                serif_fonts[weight] = out
                shutil.copy2(out, FONT_DIR / out.name)

        resolved: list[tuple[str, Path, int, int | None, str]] = []
        for name, src_name, size, weight, sym in OUTS:
            src = serif_fonts.get(weight, FONT_DIR / src_name) if weight is not None else FONT_DIR / src_name
            if weight is not None and not src.exists():
                print(
                    f"缺少 Serif 源字体: {src}（禁止用 Sans 冒充经文字形）",
                    file=sys.stderr,
                )
                return 1
            if "Sans" in src.name and name.startswith("ui_font_serif"):
                print(
                    f"Serif 档回退到 Sans 被拒绝: {name} <- {src.name}",
                    file=sys.stderr,
                )
                return 1
            resolved.append((name, src, size, weight, sym))

        only = {s.strip() for s in args.only.split(",") if s.strip()}
        if only:
            unknown = only - {row[0] for row in resolved}
            if unknown:
                print(f"--only 含未知档位: {sorted(unknown)}", file=sys.stderr)
                return 2
            resolved = [row for row in resolved if row[0] in only]

        args.sync_generated_fonts.mkdir(parents=True, exist_ok=True)
        for name, src, size, weight, sym in resolved:
            syms = syms_full if sym == "full" else syms_ui
            out = args.sync_generated_fonts / f"{name}.c"
            cmd = [
                str(args.font_conv),
                "--font",
                str(src),
                "--size",
                str(size),
                "--format",
                "lvgl",
                "--bpp",
                "4",
                "--no-compress",
                "--lv-include",
                "lvgl.h",
                "-o",
                str(out),
                "--symbols",
                syms,
            ]
            print("RUN", name, "weight", weight, "symbols", sym)
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode != 0:
                print(r.stderr, file=sys.stderr)
                return r.returncode
            shutil.copy2(out, FONT_DIR / out.name)
            (FONT_DIR / f"{name}.fcfg").write_text(
                json.dumps(
                    {
                        "name": name,
                        "size": size,
                        "bpp": 4,
                        "source": src.name,
                        "weight": weight,
                        "symbols": sym,
                        "symbols_sha1": hashlib.sha1(syms.encode()).hexdigest(),
                        "note": "Generated by EWF fonts-only pipeline; do not hand-edit .c",
                    },
                    ensure_ascii=False,
                    indent=2,
                )
                + "\n",
                encoding="utf-8",
            )
    print("fonts-only sync OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
