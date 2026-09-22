#!/usr/bin/env python3
"""canonical《心经》生成与校验工具。

用法：
  python3 docs/contracts/canonical/scripture_tool.py emit
  python3 docs/contracts/canonical/scripture_tool.py check

  emit   从 canonical/heart-sutra.txt 派生 manifest 与三端经文资源；确定性、幂等。
         已有 manifest 的 scripture_version 与 provenance 会被保留，供版本变更流程
         手工递增版本号后重建其余派生字段；上一版的发布事实写入 manifest 的
         previous 字段存档，供 check 钉住变更记录中上一版那一行。
  check  只读校验；任一端不一致时退出码非 0，且不修改任何文件。

纯 Python 3 标准库，无第三方依赖。
"""

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

TOOL_PATH = Path(__file__).resolve()
DEFAULT_ROOT = TOOL_PATH.parents[3]

CANONICAL_REL = Path("docs/contracts/canonical")
SOURCE_REL = CANONICAL_REL / "heart-sutra.txt"
MANIFEST_REL = CANONICAL_REL / "heart-sutra.manifest.json"
README_REL = CANONICAL_REL / "README.md"
HEADER_REL = CANONICAL_REL / "generated" / "ewf_scripture_canonical.h"
BACKEND_REL = Path("cloud/backend/src/main/resources/canonical/heart-sutra.json")
FRONTEND_REL = Path("cloud/frontend/src/canonical/heart-sutra.generated.ts")

SCHEMA_VERSION = 1
INITIAL_SCRIPTURE_VERSION = "HS-1.0.0"
VERSION_PATTERN = re.compile(r"^HS-\d+\.\d+\.\d+$")
# 上一版发布事实的存档字段：{scripture_version, source_sha256, counts, consumable_digest}。
# manifest 的 scripture_version 是版本变更流程中唯一人工递增的字段，一旦递增就不再表示
# 上一版，故该存档由 emit 写入、仅供 check 只读比对。
PREVIOUS_FIELD = "previous"
SOURCE_FILE_NAME = "heart-sutra.txt"
TITLE = "般若波罗蜜多心经"
GENERATED_BANNER = "GENERATED — 由 docs/contracts/canonical/scripture_tool.py 生成，禁止手改。"
GENERATE_COMMAND = "python3 docs/contracts/canonical/scripture_tool.py emit"

# 前锚点：取自冻结 UX 真源 `Embedded/lvgl-design/ewf-device-ui-export.html`
# 经文页前两行 13 槽行的顺序拼接；第 27 字符起必须接 `苦厄`。
ANCHOR_LENGTH = 26
FIRST_CONSUMABLE = "观"
ANCHOR_26 = "观自在菩萨，行深般若波罗蜜多时，照见五蕴皆空，度一切"
ANCHOR_TAIL = "苦厄"

DEFAULT_PROVENANCE = {
    "edition": "大正新脩大藏經 第8卷 No.251《般若波羅蜜多心經》（唐·三藏法師玄奘譯）",
    "source": "維基文庫轉錄《大正新修大藏經 第8卷·般若部四》第864頁（SSID-10510986）；"
    "https://zh.wikisource.org/zh-hans/般若波羅蜜多心經_(玄奘)",
    "retrieved": "2026-09-22",
}

CONSUMABLE_RANGES = ((0x3400, 0x4DBF), (0x4E00, 0x9FFF))


class ScriptureError(Exception):
    """源文本或产物不满足 canonical 契约。"""


def is_consumable(char):
    """可消费字符：BMP 的 CJK 统一表意文字与扩展 A。"""
    code_point = ord(char)
    return any(low <= code_point <= high for low, high in CONSUMABLE_RANGES)


def read_source(root):
    """读取 canonical 源文本，返回 (文本, 字节, 问题列表)。"""
    path = root / SOURCE_REL
    if not path.is_file():
        raise ScriptureError(f"缺少 canonical 源文本：{path}")
    raw = path.read_bytes()
    if raw.startswith(b"\xef\xbb\xbf"):
        raise ScriptureError(f"{SOURCE_REL} 含 UTF-8 BOM，必须为无 BOM")
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as error:
        raise ScriptureError(f"{SOURCE_REL} 不是合法 UTF-8：{error}") from error
    if "\r" in text:
        raise ScriptureError(f"{SOURCE_REL} 含 CR，行结束必须是 LF")
    if not text.endswith("\n") or text.endswith("\n\n"):
        raise ScriptureError(f"{SOURCE_REL} 必须以单个换行结束")
    body = text[:-1]
    if "\n" in body:
        raise ScriptureError(f"{SOURCE_REL} 正文不得包含换行（换行不是经文内容）")
    return body, body.encode("utf-8")


def derive(text):
    """从源文本派生步序列、消费序列、计数与摘要。"""
    if not text:
        raise ScriptureError("canonical 源文本为空")
    if not is_consumable(text[0]):
        raise ScriptureError("canonical 源文本必须以可消费汉字开头")

    steps = []
    offsets = []
    byte_offset = 0
    index = 0
    length = len(text)
    while index < length:
        end = index + 1
        while end < length and not is_consumable(text[end]):
            end += 1
        display = text[index:end]
        steps.append({"consumable": text[index], "display": display})
        offsets.append(byte_offset)
        byte_offset += len(display.encode("utf-8"))
        index = end

    consumable_sequence = "".join(step["consumable"] for step in steps)
    non_consumable_chars = sorted({char for char in text if not is_consumable(char)})

    return {
        "steps": steps,
        "offsets": offsets,
        "display_bytes": text.encode("utf-8"),
        "consumable_sequence": consumable_sequence,
        "consumable_digest": hashlib.sha256(consumable_sequence.encode("utf-8")).hexdigest(),
        "counts": {
            "total_chars": len(text),
            "consumable_han": len(steps),
            "non_consumable": len(text) - len(steps),
        },
        "non_consumable_chars": non_consumable_chars,
    }


def build_previous_view(disk_manifest, ledger, current_sha256):
    """构造写入新 manifest 的上一版存档。

    存档的派生字段（`source_sha256` / `counts` / `consumable_digest`）取自磁盘
    manifest——那是上一次发布 `emit` 的产物，未被手改时即上一版发布事实；版本号则取自
    追加式台账（README 变更记录）的最新一行，因为磁盘 manifest 的
    `scripture_version` 可能已在版本变更流程中被人工递增，指的不是上一版。

    磁盘 manifest 的 `source_sha256` 与本次派生的源文本摘要一致时，它已是**本版本**的
    产物（同一版本重跑，或版本号已递增但台账行尚未追加时的重复 `emit`），此时沿用磁盘上
    的存档：否则本版本的事实会被错记为上一版，覆盖掉真正的上一版存档。
    """
    if not disk_manifest:
        return None
    if disk_manifest.get("source_sha256") == current_sha256:
        return disk_manifest.get(PREVIOUS_FIELD)
    latest = ledger[-1] if ledger else None
    return {
        "scripture_version": latest[0] if latest else None,
        "source_sha256": disk_manifest.get("source_sha256"),
        "counts": dict(disk_manifest.get("counts") or {}),
        "consumable_digest": disk_manifest.get("consumable_digest"),
    }


def build_manifest(text, raw, previous, ledger):
    derived = derive(text)
    if text[:ANCHOR_LENGTH] != ANCHOR_26:
        raise ScriptureError(
            f"canonical 源文本前 {ANCHOR_LENGTH} 字符与冻结 UX 真源锚点不一致："
            f"实际 {text[:ANCHOR_LENGTH]!r}，期望 {ANCHOR_26!r}"
        )
    if text[ANCHOR_LENGTH : ANCHOR_LENGTH + len(ANCHOR_TAIL)] != ANCHOR_TAIL:
        raise ScriptureError(
            f"canonical 源文本第 {ANCHOR_LENGTH + 1} 字符起必须接 {ANCHOR_TAIL!r}，"
            f"实际 {text[ANCHOR_LENGTH : ANCHOR_LENGTH + len(ANCHOR_TAIL)]!r}"
        )

    version = previous.get("scripture_version") or INITIAL_SCRIPTURE_VERSION
    provenance = previous.get("provenance") or DEFAULT_PROVENANCE
    source_sha256 = hashlib.sha256(raw).hexdigest()
    return {
        "schema_version": SCHEMA_VERSION,
        "title": TITLE,
        "scripture_version": version,
        "source_file": SOURCE_FILE_NAME,
        "source_sha256": source_sha256,
        "consumable_digest": derived["consumable_digest"],
        "counts": {
            "total_chars": derived["counts"]["total_chars"],
            "consumable_han": derived["counts"]["consumable_han"],
            "non_consumable": derived["counts"]["non_consumable"],
        },
        "non_consumable_chars": derived["non_consumable_chars"],
        "first_consumable": FIRST_CONSUMABLE,
        "anchor_26": ANCHOR_26,
        "provenance": provenance,
        PREVIOUS_FIELD: build_previous_view(previous, ledger, source_sha256),
    }


def render_backend(manifest, derived):
    payload = {
        "scriptureVersion": manifest["scripture_version"],
        "title": manifest["title"],
        "sourceSha256": manifest["source_sha256"],
        "consumableDigest": manifest["consumable_digest"],
        "counts": {
            "totalChars": manifest["counts"]["total_chars"],
            "consumableHan": manifest["counts"]["consumable_han"],
            "nonConsumable": manifest["counts"]["non_consumable"],
        },
        "nonConsumableChars": manifest["non_consumable_chars"],
        "firstConsumable": manifest["first_consumable"],
        "consumableSequence": derived["consumable_sequence"],
        "steps": derived["steps"],
    }
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def render_frontend(manifest, derived):
    lines = [
        f"// {GENERATED_BANNER}",
        f"// 生成命令：{GENERATE_COMMAND}",
        "",
        "/** 经文单步：一个可消费汉字与其紧随其后的连续非消费字符。 */",
        "export interface ScriptureStep {",
        "  /** 该步的可消费汉字。 */",
        "  readonly consumable: string;",
        "  /** 该步参与槽位显示的完整文本。 */",
        "  readonly display: string;",
        "}",
        "",
        f"export const SCRIPTURE_VERSION = '{manifest['scripture_version']}';",
        f"export const SCRIPTURE_TITLE = '{manifest['title']}';",
        f"export const SOURCE_SHA256 = '{manifest['source_sha256']}';",
        f"export const CONSUMABLE_DIGEST = '{manifest['consumable_digest']}';",
        "export const COUNTS = {",
        f"  totalChars: {manifest['counts']['total_chars']},",
        f"  consumableHan: {manifest['counts']['consumable_han']},",
        f"  nonConsumable: {manifest['counts']['non_consumable']},",
        "} as const;",
        "export const NON_CONSUMABLE_CHARS: readonly string[] = ["
        + ", ".join(f"'{char}'" for char in manifest["non_consumable_chars"])
        + "];",
        f"export const FIRST_CONSUMABLE = '{manifest['first_consumable']}';",
        f"export const CONSUMABLE_SEQUENCE = '{derived['consumable_sequence']}';",
        "export const STEPS: readonly ScriptureStep[] = [",
    ]
    for step in derived["steps"]:
        lines.append(f"  {{ consumable: '{step['consumable']}', display: '{step['display']}' }},")
    lines.append("];")
    return "\n".join(lines) + "\n"


def render_values(values, formatter, per_row=12):
    """把一列数值排成 C 初始化列表的若干行。"""
    return [
        "    " + ", ".join(formatter(value) for value in values[start : start + per_row]) + ","
        for start in range(0, len(values), per_row)
    ]


def render_header(manifest, derived):
    """生成自洽（header-only）的设备头：两个载荷都头内定义，接入方只需 include。"""
    display_bytes = derived["display_bytes"]
    byte_rows = render_values(display_bytes, lambda value: f"0x{value:02X}")
    offset_rows = render_values(derived["offsets"], str)

    lines = [
        f"/* {GENERATED_BANNER} */",
        f"/* 生成命令：{GENERATE_COMMAND} */",
        "",
        "#ifndef EWF_SCRIPTURE_CANONICAL_H",
        "#define EWF_SCRIPTURE_CANONICAL_H",
        "",
        "#include <stdint.h>",
        "",
        f'#define EWF_SCRIPTURE_VERSION              "{manifest["scripture_version"]}"',
        f"#define EWF_SCRIPTURE_CONSUMABLE_COUNT     {manifest['counts']['consumable_han']}",
        f"#define EWF_SCRIPTURE_STEP_COUNT           {manifest['counts']['consumable_han']}",
        f"#define EWF_SCRIPTURE_TOTAL_CHARS          {manifest['counts']['total_chars']}",
        f"#define EWF_SCRIPTURE_NON_CONSUMABLE_COUNT {manifest['counts']['non_consumable']}",
        f'#define EWF_SCRIPTURE_SOURCE_SHA256        "{manifest["source_sha256"]}"',
        f'#define EWF_SCRIPTURE_CONSUMABLE_DIGEST    "{manifest["consumable_digest"]}"',
        "",
        "/* 源文本 UTF-8 字节（不含文件末尾换行）；元素个数 == EWF_SCRIPTURE_DISPLAY_BYTES。 */",
        f"#define EWF_SCRIPTURE_DISPLAY_BYTES        {len(display_bytes)}",
        "static const char EWF_SCRIPTURE_DISPLAY_UTF8[EWF_SCRIPTURE_DISPLAY_BYTES] = {",
        *byte_rows,
        "};",
        "",
        "/* 每步在 EWF_SCRIPTURE_DISPLAY_UTF8 中的字节偏移；元素个数 == EWF_SCRIPTURE_STEP_COUNT。 */",
        "static const uint16_t EWF_SCRIPTURE_STEP_OFFSETS[EWF_SCRIPTURE_STEP_COUNT] = {",
        *offset_rows,
        "};",
        "",
        "/* 消费序列（可消费汉字按序拼接），用于校验三端消费同一版经文。 */",
        f'#define EWF_SCRIPTURE_CONSUMABLE_SEQUENCE  "{derived["consumable_sequence"]}"',
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def write_text(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(content)


def load_previous_manifest(root):
    path = root / MANIFEST_REL
    if not path.is_file():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise ScriptureError(f"{MANIFEST_REL} 不是合法 JSON：{error}") from error


def load_ledger(root):
    """读取 README 版本变更记录表的数据行（按追加顺序）。"""
    path = root / README_REL
    if not path.is_file():
        return []
    return parse_version_rows(path.read_text(encoding="utf-8"))


def find_row(rows, version):
    """从台账数据行中取版本号对应的那一行。"""
    for cells in rows:
        if cells[0] == version:
            return cells
    return None


def row_mismatches(cells, sha12, total_chars, consumable_han):
    """返回台账行中与给定事实不符的单元格描述（空列表表示逐值一致）。"""
    expected = (
        (2, "字符总数", str(total_chars)),
        (3, "可消费汉字数", str(consumable_han)),
        (4, "源摘要前12位", sha12),
    )
    return [
        f"{label} 记录行={cells[index]!r}，实际={value!r}"
        for index, label, value in expected
        if cells[index] != value
    ]


def check_version_binding(previous, manifest, ledger):
    """正文变化必须伴随 `scripture_version` 递增（AD-4 / AC 2、AC 3）。

    「上一版」的事实只取自追加式台账（README 变更记录）与本次从源文本派生的值；
    磁盘 manifest 的派生字段是 emit 的产物、可被手工改写，不充当自身校验的依据：

    - 当前版本号已登记在台账中：本次是重跑，正文必须仍是该版本登记的那一份，否则
      同一版本号会指向两份正文——「改正文而不递增版本号」在此被拒；
    - 当前版本号尚未登记：本次是新发布的首次 emit，正文必须相对台账最新一行确有
      变化，否则就是「不改正文只递增版本号」，同样拒绝重建。
    """
    if not previous:
        return
    if not ledger:
        raise ScriptureError(
            f"{MANIFEST_REL} 已存在，但 {README_REL} 的版本变更记录没有任何数据行，"
            f"无法裁定正文与 scripture_version 的绑定"
        )

    current = manifest["scripture_version"]
    sha12 = manifest["source_sha256"][:12]
    counts = manifest["counts"]

    registered = find_row(ledger, current)
    if registered is not None:
        mismatches = row_mismatches(
            registered, sha12, counts["total_chars"], counts["consumable_han"]
        )
        if mismatches:
            raise ScriptureError(
                f"无法重建 canonical 产物：{SOURCE_REL} 已变化"
                f"（source_sha256 前 12 位 {sha12}），但 {README_REL} 版本 {current} 的"
                f"变更记录行登记的是另一份正文（{'；'.join(mismatches)}）。"
                f"同一版本号不得指向两份正文，任何影响展示文本的改动都必须递增 "
                f"scripture_version"
            )
        return

    latest = ledger[-1]
    if latest[4] == sha12:
        raise ScriptureError(
            f"无法重建 canonical 产物：{SOURCE_REL} 未变化"
            f"（source_sha256 前 12 位仍为 {sha12}），"
            f"但 scripture_version 已递增为 {current!r}"
            f"（变更记录中最新一版为 {latest[0]!r}）。"
            f"版本变更流程要求版本号递增必须伴随正文或摘要变化，"
            f"不改正文而只递增版本号会把同一份正文登记在两个版本号下"
        )


def emit(root):
    text, raw = read_source(root)
    previous = load_previous_manifest(root)
    ledger = load_ledger(root)
    manifest = build_manifest(text, raw, previous, ledger)
    check_version_binding(previous, manifest, ledger)
    derived = derive(text)

    write_text(root / MANIFEST_REL, json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    write_text(root / BACKEND_REL, render_backend(manifest, derived))
    write_text(root / FRONTEND_REL, render_frontend(manifest, derived))
    write_text(root / HEADER_REL, render_header(manifest, derived))
    print(f"canonical 生成完成：scripture_version={manifest['scripture_version']}，"
          f"可消费汉字 {manifest['counts']['consumable_han']} 个，"
          f"非消费字符 {' '.join(manifest['non_consumable_chars'])}。")
    return 0


def find_version_entry(readme_text, version):
    """从 README 版本变更记录表取出指定版本行。"""
    for line in readme_text.splitlines():
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) >= 6 and cells[0] == version:
            return cells
    return None


def parse_version_rows(readme_text):
    """取出 README 版本变更记录表的所有数据行（表头与分隔行不匹配版本号格式）。"""
    rows = []
    for line in readme_text.splitlines():
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) >= 6 and VERSION_PATTERN.match(cells[0]):
            rows.append(cells)
    return rows


def version_key(version):
    """把 `HS-<major>.<minor>.<patch>` 转成可比较的元组。"""
    return tuple(int(part) for part in version.split("-", 1)[1].split("."))


def check_version_history(errors, root, manifest):
    """校验版本变更记录的追加纪律与「版本号 ↔ 正文」绑定（AC 3）。

    覆盖的必需失败路径：同一份正文不得登记在两个版本号下；变更记录只能追加
    （版本号不得重复、必须严格递增、既有行不得删除或重排），初始版本行必须保留；
    紧邻当前版本之前的那一行必须与 manifest 的 `previous` 存档逐值一致。
    """
    path = root / README_REL
    if not path.is_file():
        return
    rows = parse_version_rows(path.read_text(encoding="utf-8"))

    versions = [cells[0] for cells in rows]
    duplicates = sorted({version for version in versions if versions.count(version) > 1})
    if duplicates:
        errors.append(
            f"{README_REL} 版本变更记录出现重复版本行 {'、'.join(duplicates)}"
            f"（同一版本只能有一行记录）"
        )

    by_fingerprint = {}
    for cells in rows:
        by_fingerprint.setdefault((cells[2], cells[3], cells[4]), []).append(cells[0])
    for fingerprint, owners in sorted(by_fingerprint.items()):
        if len(owners) > 1:
            errors.append(
                f"{README_REL} 版本变更记录中 {'、'.join(sorted(owners))} 指向同一份正文"
                f"（字符总数/可消费汉字数/source_sha256 前 12 位完全相同：{'/'.join(fingerprint)}）："
                f"同一正文不得对应两个不同的 scripture_version，"
                f"版本号递增必须伴随正文或摘要变化（版本变更流程第 2、6 条）"
            )

    if INITIAL_SCRIPTURE_VERSION not in versions:
        errors.append(
            f"{README_REL} 版本变更记录缺少初始版本 {INITIAL_SCRIPTURE_VERSION} 的历史行"
            f"（变更记录只能追加，不得改写或删除既有历史行）"
        )

    for before, after in zip(versions, versions[1:]):
        if version_key(before) > version_key(after):
            errors.append(
                f"{README_REL} 版本变更记录的顺序不是递增的：{before} 出现在 {after} 之前"
                f"（变更记录只能追加，不得重排既有历史行）"
            )

    # 用 manifest 的上一版存档钉住紧邻当前版本之前的那一行；更早的历史行没有可比对的
    # 真源（本目录只保留上一版存档），其静态约束只有上面的重复/递增/指纹检查。
    current = manifest.get("scripture_version")
    position = versions.index(current) if current in versions else None
    history = rows[:position] if position is not None else rows
    if not history:
        return
    latest = history[-1]
    previous = manifest.get(PREVIOUS_FIELD)
    if not isinstance(previous, dict):
        errors.append(
            f"{README_REL} 版本变更记录中版本 {latest[0]} 的历史行没有对应的上一版存档"
            f"（{MANIFEST_REL} 的 {PREVIOUS_FIELD} 为 {previous!r}）——"
            f"该行无法被静态校验，变更记录要求历史行必须由 emit 的存档覆盖"
        )
        return
    previous_counts = previous.get("counts") or {}
    expected = (
        (0, "版本号", previous.get("scripture_version")),
        (2, "字符总数", str(previous_counts.get("total_chars"))),
        (3, "可消费汉字数", str(previous_counts.get("consumable_han"))),
        (4, "源摘要前12位", (previous.get("source_sha256") or "")[:12]),
    )
    mismatches = [
        f"{label} 记录行={latest[column]!r}，上一版存档={value!r}"
        for column, label, value in expected
        if latest[column] != value
    ]
    if mismatches:
        errors.append(
            f"{README_REL} 版本 {latest[0]} 的历史行与 {MANIFEST_REL} 的 {PREVIOUS_FIELD} "
            f"存档不一致（{'；'.join(mismatches)}）：变更记录只能追加，既有历史行不得改写"
        )


def check_manifest_against_source(errors, manifest, derived, text, raw):
    if manifest.get("schema_version") != SCHEMA_VERSION:
        errors.append(
            f"{MANIFEST_REL} 字段 schema_version 期望 {SCHEMA_VERSION}，"
            f"实际 {manifest.get('schema_version')!r}"
        )
    if manifest.get("source_file") != SOURCE_FILE_NAME:
        errors.append(
            f"{MANIFEST_REL} 字段 source_file 期望 {SOURCE_FILE_NAME!r}，"
            f"实际 {manifest.get('source_file')!r}"
        )

    expected_sha = hashlib.sha256(raw).hexdigest()
    if manifest.get("source_sha256") != expected_sha:
        errors.append(
            f"{MANIFEST_REL} 字段 source_sha256 与 {SOURCE_REL} 不一致："
            f"manifest={manifest.get('source_sha256')}，源文本={expected_sha}"
        )
    if manifest.get("consumable_digest") != derived["consumable_digest"]:
        errors.append(
            f"{MANIFEST_REL} 字段 consumable_digest 与 {SOURCE_REL} 派生的消费序列不一致："
            f"manifest={manifest.get('consumable_digest')}，源文本={derived['consumable_digest']}"
        )

    counts = manifest.get("counts") or {}
    for key, expected in derived["counts"].items():
        if counts.get(key) != expected:
            errors.append(
                f"{MANIFEST_REL} 字段 counts.{key} 与 {SOURCE_REL} 不一致："
                f"manifest={counts.get(key)!r}，源文本派生={expected}"
            )

    declared = manifest.get("non_consumable_chars")
    if not isinstance(declared, list):
        errors.append(f"{MANIFEST_REL} 字段 non_consumable_chars 必须是数组，实际 {declared!r}")
    elif sorted(declared) != derived["non_consumable_chars"]:
        undeclared = [
            char for char in derived["non_consumable_chars"] if char not in declared
        ]
        extra = [char for char in declared if char not in derived["non_consumable_chars"]]
        errors.append(
            f"{MANIFEST_REL} 字段 non_consumable_chars 与 {SOURCE_REL} 的字符分类不封闭："
            f"源文本出现但未声明={undeclared}，声明但源文本未出现={extra}"
        )

    if manifest.get("first_consumable") != FIRST_CONSUMABLE:
        errors.append(
            f"{MANIFEST_REL} 字段 first_consumable 期望 {FIRST_CONSUMABLE!r}，"
            f"实际 {manifest.get('first_consumable')!r}"
        )
    if manifest.get("anchor_26") != ANCHOR_26:
        errors.append(
            f"{MANIFEST_REL} 字段 anchor_26 与冻结 UX 真源锚点不一致："
            f"实际 {manifest.get('anchor_26')!r}，期望 {ANCHOR_26!r}"
        )
    if text[:ANCHOR_LENGTH] != ANCHOR_26:
        errors.append(
            f"{SOURCE_REL} 前 {ANCHOR_LENGTH} 字符与 anchor_26 不一致："
            f"实际 {text[:ANCHOR_LENGTH]!r}，期望 {ANCHOR_26!r}"
        )
    if text[ANCHOR_LENGTH : ANCHOR_LENGTH + len(ANCHOR_TAIL)] != ANCHOR_TAIL:
        errors.append(
            f"{SOURCE_REL} 第 {ANCHOR_LENGTH + 1} 字符起必须接 {ANCHOR_TAIL!r}，"
            f"实际 {text[ANCHOR_LENGTH : ANCHOR_LENGTH + len(ANCHOR_TAIL)]!r}"
        )


def check_readme(errors, root, manifest, derived):
    path = root / README_REL
    if not path.is_file():
        errors.append(f"缺少版本变更记录文件：{README_REL}")
        return
    text = path.read_text(encoding="utf-8")
    version = manifest.get("scripture_version")
    entry = find_version_entry(text, version)
    if entry is None:
        errors.append(
            f"{README_REL} 变更记录缺少当前版本 {version} 的记录行（版本变更流程未完成）"
        )
        return
    for mismatch in row_mismatches(
        entry,
        (manifest.get("source_sha256") or "")[:12],
        derived["counts"]["total_chars"],
        derived["counts"]["consumable_han"],
    ):
        errors.append(
            f"{README_REL} 版本 {version} 变更记录行与 manifest 不一致：{mismatch}"
        )
    for column, label in ((1, "日期"), (5, "影响说明")):
        if not entry[column]:
            errors.append(
                f"{README_REL} 版本 {version} 变更记录行的{label}为空"
                f"（版本变更流程要求变更包含可追溯的影响说明）"
            )


def check_product(errors, root, rel, manifest, derived):
    """校验三端产物与 manifest 一致。"""
    path = root / rel
    if not path.is_file():
        errors.append(f"缺少 {rel}（三端经文资源未生成）")
        return
    text = path.read_text(encoding="utf-8")

    # 各端产物承载的字段不同；未承载的字段保持 None 并跳过比对。
    title = source_sha = non_consumable_chars = first_consumable = None
    step_count = offsets = display_bytes = declared_bytes = None

    if rel == BACKEND_REL:
        try:
            payload = json.loads(text)
        except json.JSONDecodeError as error:
            errors.append(f"{rel} 不是合法 JSON：{error}")
            return
        version = payload.get("scriptureVersion")
        title = payload.get("title")
        source_sha = payload.get("sourceSha256")
        digest = payload.get("consumableDigest")
        sequence = payload.get("consumableSequence")
        counts = payload.get("counts") or {}
        consumable_count = counts.get("consumableHan")
        total_chars = counts.get("totalChars")
        non_consumable = counts.get("nonConsumable")
        non_consumable_chars = payload.get("nonConsumableChars")
        first_consumable = payload.get("firstConsumable")
        steps = payload.get("steps")
        if steps is None:
            errors.append(f"{rel} 未暴露 steps（每步消费归属），步归属无法校验")
        for field, value in (
            ("title", title),
            ("sourceSha256", source_sha),
            ("firstConsumable", first_consumable),
            ("nonConsumableChars", non_consumable_chars),
        ):
            if value is None:
                errors.append(f"{rel} 未暴露 {field}（无法与 {MANIFEST_REL} 的对应字段比对）")
    elif rel == FRONTEND_REL:
        version = match_group(text, rel, r"export const SCRIPTURE_VERSION = '([^']*)'", errors)
        title = match_group(text, rel, r"export const SCRIPTURE_TITLE = '([^']*)'", errors)
        source_sha = match_group(text, rel, r"export const SOURCE_SHA256 = '([^']*)'", errors)
        digest = match_group(text, rel, r"export const CONSUMABLE_DIGEST = '([^']*)'", errors)
        sequence = match_group(text, rel, r"export const CONSUMABLE_SEQUENCE = '([^']*)'", errors)
        consumable_count = match_int(text, rel, r"consumableHan: (\d+)", errors)
        total_chars = match_int(text, rel, r"totalChars: (\d+)", errors)
        non_consumable = match_int(text, rel, r"nonConsumable: (\d+)", errors)
        declared_chars = match_group(
            text, rel, r"export const NON_CONSUMABLE_CHARS[^=]*= \[([^\]]*)\]", errors
        )
        non_consumable_chars = (
            re.findall(r"'([^']*)'", declared_chars) if declared_chars is not None else None
        )
        first_consumable = match_group(
            text, rel, r"export const FIRST_CONSUMABLE = '([^']*)'", errors
        )
        steps = [
            {"consumable": consumable, "display": display}
            for consumable, display in re.findall(
                r"\{ consumable: '([^']*)', display: '([^']*)' \}", text
            )
        ]
    else:
        version = match_group(text, rel, r'#define EWF_SCRIPTURE_VERSION\s+"([^"]*)"', errors)
        digest = match_group(text, rel, r'#define EWF_SCRIPTURE_CONSUMABLE_DIGEST\s+"([^"]*)"', errors)
        sequence = match_group(
            text, rel, r'#define EWF_SCRIPTURE_CONSUMABLE_SEQUENCE\s+"([^"]*)"', errors
        )
        consumable_count = match_int(
            text, rel, r"#define EWF_SCRIPTURE_CONSUMABLE_COUNT\s+(\d+)", errors
        )
        total_chars = match_int(text, rel, r"#define EWF_SCRIPTURE_TOTAL_CHARS\s+(\d+)", errors)
        non_consumable = match_int(
            text, rel, r"#define EWF_SCRIPTURE_NON_CONSUMABLE_COUNT\s+(\d+)", errors
        )
        source_sha = match_group(
            text, rel, r'#define EWF_SCRIPTURE_SOURCE_SHA256\s+"([^"]*)"', errors
        )
        step_count = match_int(text, rel, r"#define EWF_SCRIPTURE_STEP_COUNT\s+(\d+)", errors)
        declared_bytes = match_int(
            text, rel, r"#define EWF_SCRIPTURE_DISPLAY_BYTES\s+(\d+)", errors
        )
        display_bytes = match_byte_array(text, rel, "EWF_SCRIPTURE_DISPLAY_UTF8", errors)
        offsets = match_int_array(text, rel, "EWF_SCRIPTURE_STEP_OFFSETS", errors)
        steps = None

    if version != manifest.get("scripture_version"):
        errors.append(
            f"{rel} 的 scripture_version 与 {MANIFEST_REL} 不一致："
            f"产物={version!r}，manifest={manifest.get('scripture_version')!r}"
        )
    if total_chars != derived["counts"]["total_chars"]:
        errors.append(
            f"{rel} 的字符总数与 {MANIFEST_REL} 不一致："
            f"产物={total_chars!r}，manifest={derived['counts']['total_chars']}"
        )
    if consumable_count != derived["counts"]["consumable_han"]:
        errors.append(
            f"{rel} 的可消费汉字数与 {MANIFEST_REL} 不一致："
            f"产物={consumable_count!r}，manifest={derived['counts']['consumable_han']}"
        )
    if non_consumable != derived["counts"]["non_consumable"]:
        errors.append(
            f"{rel} 的非消费字符数与 {MANIFEST_REL} 不一致："
            f"产物={non_consumable!r}，manifest={derived['counts']['non_consumable']}"
        )

    if not isinstance(sequence, str) or not sequence:
        errors.append(f"{rel} 未暴露消费序列（consumableSequence / CONSUMABLE_SEQUENCE）")
    else:
        recomputed = hashlib.sha256(sequence.encode("utf-8")).hexdigest()
        if recomputed != derived["consumable_digest"]:
            errors.append(
                f"{rel} 的消费序列摘要与 {MANIFEST_REL} 不一致："
                f"产物序列摘要={recomputed}，manifest={derived['consumable_digest']}"
            )
    if digest != manifest.get("consumable_digest"):
        errors.append(
            f"{rel} 的 consumable_digest 字段与 {MANIFEST_REL} 不一致："
            f"产物={digest!r}，manifest={manifest.get('consumable_digest')!r}"
        )
    if title is not None and title != manifest.get("title"):
        errors.append(
            f"{rel} 的经题与 {MANIFEST_REL} 不一致："
            f"产物={title!r}，manifest={manifest.get('title')!r}"
        )
    if source_sha is not None and source_sha != manifest.get("source_sha256"):
        errors.append(
            f"{rel} 的 source_sha256 与 {MANIFEST_REL} 不一致："
            f"产物={source_sha!r}，manifest={manifest.get('source_sha256')!r}"
        )
    if first_consumable is not None and first_consumable != manifest.get("first_consumable"):
        errors.append(
            f"{rel} 的首个可消费汉字与 {MANIFEST_REL} 不一致："
            f"产物={first_consumable!r}，manifest={manifest.get('first_consumable')!r}"
        )
    if non_consumable_chars is not None and sorted(non_consumable_chars) != derived[
        "non_consumable_chars"
    ]:
        errors.append(
            f"{rel} 的非消费字符集合与 {MANIFEST_REL} 的字符分类不一致："
            f"产物={sorted(non_consumable_chars)}，源文本派生={derived['non_consumable_chars']}"
        )
    if step_count is not None and step_count != len(derived["steps"]):
        errors.append(
            f"{rel} 的 EWF_SCRIPTURE_STEP_COUNT 与 {MANIFEST_REL} 派生的步数不一致："
            f"产物={step_count}，源文本派生={len(derived['steps'])}"
        )
    if declared_bytes is not None and declared_bytes != len(derived["display_bytes"]):
        errors.append(
            f"{rel} 的 EWF_SCRIPTURE_DISPLAY_BYTES 与 {SOURCE_REL} 的 UTF-8 字节数不一致："
            f"产物={declared_bytes}，源文本派生={len(derived['display_bytes'])}"
        )
    if display_bytes is not None and display_bytes != list(derived["display_bytes"]):
        errors.append(
            f"{rel} 的 EWF_SCRIPTURE_DISPLAY_UTF8 载荷与 {SOURCE_REL} 字节不一致"
            f"（不是同一份正文）：产物 {len(display_bytes)} 个字节，"
            f"源文本派生 {len(derived['display_bytes'])} 个字节"
        )
    if offsets is not None and offsets != derived["offsets"]:
        errors.append(
            f"{rel} 的 EWF_SCRIPTURE_STEP_OFFSETS 与 {MANIFEST_REL} 派生的步起点不一致："
            f"产物 {len(offsets)} 个值，源文本派生 {len(derived['offsets'])} 个值"
        )

    if steps is not None:
        if steps != derived["steps"]:
            errors.append(
                f"{rel} 的 step 序列与 {MANIFEST_REL} 派生的步归属不一致："
                f"产物 {len(steps)} 步，源文本派生 {len(derived['steps'])} 步"
                f"（标点随紧邻前一个汉字，不得跨汉字归属）"
            )


def match_group(text, rel, pattern, errors):
    found = re.search(pattern, text)
    if not found:
        errors.append(f"{rel} 缺少字段（未匹配 {pattern}）")
        return None
    return found.group(1)


def match_int(text, rel, pattern, errors):
    found = re.search(pattern, text)
    if not found:
        errors.append(f"{rel} 缺少字段（未匹配 {pattern}）")
        return None
    return int(found.group(1))


def find_array_body(text, rel, name, errors):
    """取 `name[尺寸] = { ... };` 的载荷；缺失即报错，不静默跳过该载荷的比对。"""
    found = re.search(re.escape(name) + r"\s*\[[^\]]*\]\s*=\s*\{(.*?)\};", text, re.DOTALL)
    if found is None:
        errors.append(
            f"{rel} 未暴露 {name} 的逐值定义（该符号无载荷），"
            f"无法与 {MANIFEST_REL} 的派生值比对"
        )
        return None
    return found.group(1)


def match_int_array(text, rel, name, errors):
    body = find_array_body(text, rel, name, errors)
    return None if body is None else [int(value) for value in re.findall(r"\d+", body)]


def match_byte_array(text, rel, name, errors):
    body = find_array_body(text, rel, name, errors)
    return None if body is None else [int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", body)]


def check(root):
    errors = []
    try:
        text, raw = read_source(root)
        derived = derive(text)
    except ScriptureError as error:
        print("canonical 校验失败：")
        print(f"  - {error}")
        return 1

    try:
        manifest = load_previous_manifest(root)
    except ScriptureError as error:
        print("canonical 校验失败：")
        print(f"  - {error}")
        return 1
    if not manifest:
        print("canonical 校验失败：")
        print(f"  - 缺少 {MANIFEST_REL}（尚未执行 emit）")
        return 1

    check_manifest_against_source(errors, manifest, derived, text, raw)
    check_readme(errors, root, manifest, derived)
    check_version_history(errors, root, manifest)
    for rel in (BACKEND_REL, FRONTEND_REL, HEADER_REL):
        check_product(errors, root, rel, manifest, derived)

    if errors:
        print("canonical 校验失败：")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(
        f"canonical 校验通过：scripture_version={manifest['scripture_version']}，"
        f"源文本、manifest、变更记录与三端产物一致"
        f"（{manifest['counts']['consumable_han']} 个可消费汉字）。"
    )
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description="canonical《心经》生成与校验工具")
    parser.add_argument("--root", default=str(DEFAULT_ROOT), help="仓库根目录")
    parser.add_argument("subcommand", choices=("emit", "check"))
    args = parser.parse_args(argv)
    root = Path(args.root).resolve()
    try:
        if args.subcommand == "emit":
            return emit(root)
        return check(root)
    except ScriptureError as error:
        print(f"canonical {args.subcommand} 失败：{error}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
