#!/usr/bin/env python3
"""canonical《心经》门禁测试。

一次运行两类检查：
1. 正向门禁：真实产物上 `scripture_tool.py check` 必须通过，`emit` 必须幂等，
   生成头必须是合法 C 且载荷可链接；
2. 负例门禁：必须失败的场景在**临时目录副本**上执行，真实产物不被污染。

纯 Python 标准库 + 本机 C 编译器（与 `Embedded/tests/run_host_tests.py` 同一范式），
使用 `assert` + 失败计数 + 非 0 退出，不依赖 pytest。
"""

import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
CANONICAL_DIR = TESTS_DIR.parent
REPO_ROOT = CANONICAL_DIR.parents[2]
TOOL = CANONICAL_DIR / "scripture_tool.py"

CANONICAL_REL = "docs/contracts/canonical"
BACKEND_REL = "cloud/backend/src/main/resources/canonical/heart-sutra.json"
FRONTEND_REL = "cloud/frontend/src/canonical/heart-sutra.generated.ts"
HEADER_REL = "docs/contracts/canonical/generated/ewf_scripture_canonical.h"

SOURCE_NAME = "heart-sutra.txt"
MANIFEST_NAME = "heart-sutra.manifest.json"
README_NAME = "README.md"

INITIAL_VERSION = "HS-1.0.0"
NEXT_VERSION = "HS-1.0.1"
# 只换一个汉字：消费序列长度与计数都不变，但正文摘要必然改变。
SOURCE_EDIT = ("罣", "挂")

COPIED_FILES = (SOURCE_NAME, MANIFEST_NAME, README_NAME)
PRODUCT_RELS = (BACKEND_REL, FRONTEND_REL, HEADER_REL)


def run_tool(root, subcommand):
    """在指定仓库根上执行工具，返回 (退出码, 输出)。"""
    completed = subprocess.run(
        [sys.executable, str(TOOL), "--root", str(root), subcommand],
        capture_output=True,
        text=True,
        check=False,
    )
    return completed.returncode, (completed.stdout or "") + (completed.stderr or "")


def find_c_compiler():
    return shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")


def make_sandbox(tmpdir):
    """把真实产物复制成一份可篡改的临时仓库根。"""
    root = Path(tmpdir)
    (root / CANONICAL_REL).mkdir(parents=True)
    for name in COPIED_FILES:
        shutil.copy2(CANONICAL_DIR / name, root / CANONICAL_REL / name)
    shutil.copytree(CANONICAL_DIR / "generated", root / CANONICAL_REL / "generated")
    for rel in PRODUCT_RELS:
        destination = root / rel
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(REPO_ROOT / rel, destination)
    return root


def edit_text(path, transform):
    text = path.read_text(encoding="utf-8")
    path.write_text(transform(text), encoding="utf-8", newline="\n")


def edit_json(path, transform):
    data = json.loads(path.read_text(encoding="utf-8"))
    transform(data)
    path.write_text(
        json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n"
    )


def expect_failure(root, label, expect_tokens=()):
    code, output = run_tool(root, "check")
    assert code != 0, f"{label}: check 不应通过（退出码 {code}），输出：{output}"
    for token in expect_tokens:
        assert token in output, f"{label}: 失败信息未指出 {token}，实际输出：{output}"


def bump_version(root, version, products=False):
    """按版本变更流程第 2 步递增 manifest 的 `scripture_version`。

    `products=True` 时同步改写三端产物的版本号字面量：不运行 `emit` 的场景必须这样做，
    否则失败会来自「产物与 manifest 版本不一致」，而不是被测的那条规则。
    """
    edit_json(
        root / CANONICAL_REL / MANIFEST_NAME,
        lambda data: data.__setitem__("scripture_version", version),
    )
    if products:
        for rel in PRODUCT_RELS:
            edit_text(root / rel, lambda text: text.replace(INITIAL_VERSION, version))


def edit_source(root, old, new):
    """替换正文中的一个汉字或标点。"""
    source = root / CANONICAL_REL / SOURCE_NAME
    original = source.read_text(encoding="utf-8")
    edited = original.replace(old, new, 1)
    assert edited != original, f"未能在正文中构造改动：{old!r} → {new!r}"
    source.write_text(edited, encoding="utf-8", newline="\n")


def append_ledger_row(root, version, note):
    """按版本变更流程第 4 步向 README 变更记录表追加一行（数值取自当前 manifest）。"""
    manifest = json.loads((root / CANONICAL_REL / MANIFEST_NAME).read_text(encoding="utf-8"))
    row = (
        f"| {version} | 2026-09-22 | {manifest['counts']['total_chars']} | "
        f"{manifest['counts']['consumable_han']} | {manifest['source_sha256'][:12]} | {note} |\n"
    )
    readme = root / CANONICAL_REL / README_NAME
    readme.write_text(readme.read_text(encoding="utf-8") + row, encoding="utf-8", newline="\n")


def edit_ledger_row(root, version, column, value):
    """改写变更记录表中指定版本行的某个单元格。"""
    path = root / CANONICAL_REL / README_NAME
    lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
    for index, line in enumerate(lines):
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) >= 6 and cells[0] == version:
            cells[column] = value
            lines[index] = "| " + " | ".join(cells) + " |\n"
            path.write_text("".join(lines), encoding="utf-8", newline="\n")
            return
    raise AssertionError(f"变更记录中未找到版本 {version} 的行")


def make_published_sandbox(tmpdir):
    """构造一份「已按版本变更流程发布过两个版本」的沙箱副本。"""
    root = make_sandbox(tmpdir)
    edit_source(root, *SOURCE_EDIT)
    bump_version(root, NEXT_VERSION)
    code, output = run_tool(root, "emit")
    assert code == 0, f"构造已发布沙箱时 emit 应成功，实际退出码 {code}：{output}"
    append_ledger_row(root, NEXT_VERSION, "修正正文一个汉字，消费序列与计数不变")
    return root


def test_check_passes_on_real_products():
    code, output = run_tool(REPO_ROOT, "check")
    assert code == 0, f"真实产物 check 应通过，实际退出码 {code}：{output}"
    assert "check" in output or "通过" in output, f"check 应输出可读结论：{output}"


def test_emit_is_idempotent():
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        watched = [
            root / CANONICAL_REL / MANIFEST_NAME,
            *[root / rel for rel in PRODUCT_RELS],
        ]
        first = {path: path.read_bytes() for path in watched}
        code, output = run_tool(root, "emit")
        assert code == 0, f"sandbox emit 应成功，实际退出码 {code}：{output}"
        second = {path: path.read_bytes() for path in watched}
        assert first == second, "emit 不幂等：沙箱首轮 emit 改动了产物字节"

        code, output = run_tool(root, "emit")
        assert code == 0, f"二次 emit 应成功，实际退出码 {code}：{output}"
        third = {path: path.read_bytes() for path in watched}
        assert second == third, "emit 不幂等：连续两次 emit 产物字节不稳定"


def test_positive_readme_version_change_flow():
    """正例：README「版本变更流程」第 1-5 步必须端到端可执行。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)

        edit_source(root, *SOURCE_EDIT)              # 1. 修改正文
        bump_version(root, NEXT_VERSION)             # 2. 递增 scripture_version

        code, output = run_tool(root, "emit")        # 3. emit
        assert code == 0, f"版本变更流程第 3 步（emit）应成功，实际退出码 {code}：{output}"

        append_ledger_row(root, NEXT_VERSION, "修正正文一个汉字，消费序列与计数不变")  # 4.

        code, output = run_tool(root, "check")       # 5. check
        assert code == 0, f"版本变更流程第 5 步（check）应通过，实际退出码 {code}：{output}"

        manifest = json.loads((root / CANONICAL_REL / MANIFEST_NAME).read_text(encoding="utf-8"))
        assert manifest["scripture_version"] == NEXT_VERSION, manifest["scripture_version"]
        previous = manifest.get("previous")
        assert isinstance(previous, dict), f"emit 应写入上一版存档，实际 {previous!r}"
        assert previous["scripture_version"] == INITIAL_VERSION, previous


def test_positive_emit_repeatable_before_ledger_row():
    """正例：版本变更流程第 3、4 步之间重跑 `emit` 必须幂等，不得覆盖上一版存档。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)

        edit_source(root, *SOURCE_EDIT)              # 1. 修改正文
        bump_version(root, NEXT_VERSION)             # 2. 递增 scripture_version
        code, output = run_tool(root, "emit")        # 3. emit
        assert code == 0, f"首次 emit 应成功，实际退出码 {code}：{output}"

        manifest_path = root / CANONICAL_REL / MANIFEST_NAME
        first = manifest_path.read_bytes()
        code, output = run_tool(root, "emit")        # 3'. 追加台账行之前重跑 emit
        assert code == 0, f"重复 emit 应成功，实际退出码 {code}：{output}"
        assert manifest_path.read_bytes() == first, (
            "emit 不幂等：追加台账行之前重跑改写了 manifest，本版本的事实被错记为上一版"
            "（真正的上一版存档被覆盖）"
        )

        append_ledger_row(root, NEXT_VERSION, "修正正文一个汉字，消费序列与计数不变")  # 4.
        code, output = run_tool(root, "check")       # 5. check
        assert code == 0, f"重跑 emit 后 check 应通过，实际退出码 {code}：{output}"
        previous = json.loads(manifest_path.read_text(encoding="utf-8"))["previous"]
        assert isinstance(previous, dict) and previous["scripture_version"] == INITIAL_VERSION, (
            f"上一版存档必须是 {INITIAL_VERSION} 的发布事实，实际 {previous!r}"
        )


# 生成头探针：同时引用两个载荷，使「只有 extern 声明、没有定义」在链接期即失败。
HEADER_PROBE = """\
#include "ewf_scripture_canonical.h"

int main(void)
{
    unsigned int last_step = EWF_SCRIPTURE_STEP_COUNT - 1;

    if (EWF_SCRIPTURE_DISPLAY_BYTES != (int)sizeof(EWF_SCRIPTURE_DISPLAY_UTF8)) {
        return 1;
    }
    if (EWF_SCRIPTURE_STEP_OFFSETS[last_step] >= EWF_SCRIPTURE_DISPLAY_BYTES) {
        return 2;
    }
    return 0;
}
"""


def test_generated_header_is_valid_c():
    """生成头必须是合法 C 且自洽：语法检查 + 载荷定义可链接。"""
    compiler = find_c_compiler()
    assert compiler is not None, "未找到 cc/gcc/clang，无法验证生成头是合法 C"
    header = REPO_ROOT / HEADER_REL

    syntax = subprocess.run(
        [compiler, "-fsyntax-only", "-x", "c", str(header)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert syntax.returncode == 0, (
        f"生成头不是合法 C（{HEADER_REL}）：\n{syntax.stdout}{syntax.stderr}"
    )

    with tempfile.TemporaryDirectory() as tmpdir:
        probe = Path(tmpdir) / "probe.c"
        probe.write_text(HEADER_PROBE, encoding="utf-8", newline="\n")
        binary = Path(tmpdir) / "probe"
        build = subprocess.run(
            [compiler, "-Wall", "-Wextra", "-I", str(header.parent), str(probe), "-o", str(binary)],
            capture_output=True,
            text=True,
            check=False,
        )
        assert build.returncode == 0, (
            "生成头的载荷必须头内定义并可用"
            f"（EWF_SCRIPTURE_DISPLAY_UTF8 / EWF_SCRIPTURE_STEP_OFFSETS）：\n"
            f"{build.stdout}{build.stderr}"
        )
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        assert run.returncode == 0, f"生成头自洽探针失败（退出码 {run.returncode}）"


# 冻结 UX 真源 `Embedded/lvgl-design/ewf-device-ui-export.html` 的经文页前两行各 13 字符，
# 第三行以 `苦厄舍利` 起。这些期望值独立于本工具派生，用于固定标点计槽语义。
UX_SLOT_LINE_1 = "观自在菩萨，行深般若波罗蜜"
UX_SLOT_LINE_2 = "多时，照见五蕴皆空，度一切"
UX_LINE_3_HEAD = "苦厄舍利"
UX_STEP_DISPLAYS = ((4, "萨，"), (13, "时，"), (19, "空，"))


def test_step_ownership_matches_frozen_ux_slots():
    """步归属与标点计槽由冻结 UX 槽行独立固定，而不是与工具自身派生比对。"""
    payload = json.loads((REPO_ROOT / BACKEND_REL).read_text(encoding="utf-8"))
    displays = [step["display"] for step in payload["steps"]]

    rendered = "".join(displays)
    assert rendered[:13] == UX_SLOT_LINE_1, f"第 1 条 13 槽行不符：{rendered[:13]!r}"
    assert rendered[13:26] == UX_SLOT_LINE_2, f"第 2 条 13 槽行不符：{rendered[13:26]!r}"
    assert rendered[26:30] == UX_LINE_3_HEAD, f"第 3 行起始不符：{rendered[26:30]!r}"

    for index, expected in UX_STEP_DISPLAYS:
        assert displays[index] == expected, (
            f"标点必须随紧邻的前一个汉字：第 {index + 1} 步 display 期望 {expected!r}，"
            f"实际 {displays[index]!r}"
        )


def test_negative_1_source_text_drift():
    """负例 1：改源文本一个汉字、不更新 manifest。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        source = root / CANONICAL_REL / SOURCE_NAME
        edit_text(source, lambda t: t.replace("度一切苦厄", "度一切苦危", 1))
        expect_failure(root, "负例 1", ("source_sha256",))


def test_negative_2_version_mismatch():
    """负例 2：manifest 版本改成三端产物不含的版本。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / CANONICAL_REL / MANIFEST_NAME,
            lambda data: data.__setitem__("scripture_version", "HS-9.9.9"),
        )
        expect_failure(root, "负例 2", ("scripture_version",))


def test_negative_3_punctuation_step_ownership():
    """负例 3：把标点移到另一个汉字之后，改变步归属。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        source = root / CANONICAL_REL / SOURCE_NAME
        original = source.read_text(encoding="utf-8")
        moved = original.replace("观自在菩萨，", "观，自在菩萨", 1)
        assert moved != original, "负例 3 未能构造标点移位文本"
        source.write_text(moved, encoding="utf-8", newline="\n")

        # 只刷新 manifest 的源摘要，不动三端产物与 anchor：本场景的失败信息还包含三端
        # source_sha256 与 anchor_26 不一致。用例断言的 `step`/`步` 文本只会由步归属
        # 比对产出，故该断言固定的是步归属。
        digest = hashlib.sha256(moved.rstrip("\n").encode("utf-8")).hexdigest()
        edit_json(
            root / CANONICAL_REL / MANIFEST_NAME,
            lambda data: data.__setitem__("source_sha256", digest),
        )
        expect_failure(root, "负例 3", ("step", "步"))


def test_negative_4_undeclared_non_consumable():
    """负例 4：源文本插入未声明的非消费字符。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        source = root / CANONICAL_REL / SOURCE_NAME
        for injected in ("A", "《"):
            restore = source.read_text(encoding="utf-8")
            source.write_text(
                restore.replace("度一切苦厄", f"度一切苦厄{injected}", 1),
                encoding="utf-8",
                newline="\n",
            )
            expect_failure(root, f"负例 4（{injected}）", ("non_consumable_chars",))
            source.write_text(restore, encoding="utf-8", newline="\n")


def test_negative_5_missing_changelog_row():
    """负例 5：删除当前版本的变更记录行。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        readme = root / CANONICAL_REL / README_NAME
        original = readme.read_text(encoding="utf-8")
        kept = [line for line in original.splitlines(keepends=True) if "| HS-" not in line]
        assert len(kept) != len(original.splitlines(keepends=True)), "负例 5 未找到变更记录行"
        readme.write_text("".join(kept), encoding="utf-8", newline="\n")
        expect_failure(root, "负例 5", ("变更记录", "README"))


def test_negative_6_anchor_tampered():
    """负例 6：篡改 anchor_26 或源文本前 26 字。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_json(
            root / CANONICAL_REL / MANIFEST_NAME,
            lambda data: data.__setitem__("anchor_26", "自在菩萨行深般若波罗蜜多时照见五蕴皆空度一切苦"),
        )
        expect_failure(root, "负例 6（manifest anchor）", ("anchor_26",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        source = root / CANONICAL_REL / SOURCE_NAME
        edit_text(source, lambda t: t.replace("观自在菩萨，", "观自在菩萨。", 1))
        expect_failure(root, "负例 6（源文本前 26 字）", ("anchor_26",))


def test_negative_7_product_sequence_tampered():
    """负例 7：手工改动任一三端产物的消费序列。"""
    for rel in PRODUCT_RELS:
        with tempfile.TemporaryDirectory() as tmpdir:
            root = make_sandbox(tmpdir)
            edit_text(root / rel, lambda t: t.replace("照见五蕴皆空", "照见五蕴皆无", 1))
            expect_failure(root, f"负例 7（{rel}）", (rel.split("/")[-1],))


def test_negative_8_counts_forged_to_viewport_slot():
    """负例 8：把 counts.consumable_han 手工写成 7/13/17。"""
    for forged in (7, 13, 17):
        with tempfile.TemporaryDirectory() as tmpdir:
            root = make_sandbox(tmpdir)
            edit_json(
                root / CANONICAL_REL / MANIFEST_NAME,
                lambda data, value=forged: data["counts"].__setitem__("consumable_han", value),
            )
            expect_failure(root, f"负例 8（{forged}）", ("consumable_han",))


def test_negative_9_header_offset_payload_missing():
    """负例 9：生成头丢失每步字节偏移表，不得静默跳过该端比对。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        header = root / HEADER_REL
        original = header.read_text(encoding="utf-8")
        stripped = re.sub(
            r"static const uint16_t EWF_SCRIPTURE_STEP_OFFSETS\[[^\]]*\] = \{\n"
            r"(?:\s*[\d, ]+,\n)+\};\n",
            "",
            original,
        )
        assert stripped != original, "负例 9 未能构造缺失偏移表的头"
        header.write_text(stripped, encoding="utf-8", newline="\n")
        expect_failure(root, "负例 9", ("EWF_SCRIPTURE_STEP_OFFSETS",))


def test_negative_11_header_display_payload_missing_or_tampered():
    """负例 11：生成头丢失或篡改正文字节载荷，不得静默跳过比对。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        header = root / HEADER_REL
        original = header.read_text(encoding="utf-8")
        stripped, removed = re.subn(
            r"static const char EWF_SCRIPTURE_DISPLAY_UTF8\[[^\]]*\] = \{.*?\};\n",
            "",
            original,
            flags=re.DOTALL,
        )
        assert removed == 1, "负例 11 未能构造缺失正文字节载荷的头"
        header.write_text(stripped, encoding="utf-8", newline="\n")
        expect_failure(root, "负例 11（缺失载荷）", ("EWF_SCRIPTURE_DISPLAY_UTF8",))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        header = root / HEADER_REL
        original = header.read_text(encoding="utf-8")
        tampered = original.replace("0xE8, 0xA7, 0x82", "0xE8, 0xA7, 0x83", 1)
        assert tampered != original, "负例 11 未能构造被篡改的正文字节"
        header.write_text(tampered, encoding="utf-8", newline="\n")
        expect_failure(root, "负例 11（篡改字节）", ("EWF_SCRIPTURE_DISPLAY_UTF8",))


def test_negative_12_source_changed_without_version_bump():
    """负例 12：改正文却不递增 scripture_version —— 同一版本号不得指向两份正文。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        source = root / CANONICAL_REL / SOURCE_NAME
        original = source.read_text(encoding="utf-8")
        edited = original.replace("无眼界，乃至无意识界", "无眼界乃，至无意识界", 1)
        assert edited != original, "负例 12 未能构造正文改动"
        source.write_text(edited, encoding="utf-8", newline="\n")

        # 该改动只位移标点，消费序列与计数都不变，故失败必须来自正文摘要/版本绑定。
        expect_failure(root, "负例 12（check）", ("source_sha256",))
        code, output = run_tool(root, "emit")
        assert code != 0, (
            f"负例 12（emit）: 正文变化而未递增版本号时 emit 不应重建产物（退出码 {code}）：{output}"
        )
        assert "scripture_version" in output, (
            f"负例 12（emit）: 失败信息未指出 scripture_version：{output}"
        )


def test_negative_13_version_bump_without_content_change():
    """负例 13：正文不动、只递增版本号（追加同摘要记录行或改写既有行）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        bump_version(root, NEXT_VERSION, products=True)
        readme = root / CANONICAL_REL / README_NAME
        appended = (
            readme.read_text(encoding="utf-8")
            + f"\n| {NEXT_VERSION} | 2026-09-22 | 303 | 260 | a265c93dbc60 | 仅递增版本号，正文未变 |\n"
        )
        readme.write_text(appended, encoding="utf-8", newline="\n")
        expect_failure(root, "负例 13（追加同摘要记录行）", ("scripture_version", "变更记录"))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        bump_version(root, NEXT_VERSION, products=True)
        readme = root / CANONICAL_REL / README_NAME
        edit_text(readme, lambda text: text.replace("| HS-1.0.0 |", f"| {NEXT_VERSION} |", 1))
        expect_failure(root, "负例 13（改写既有记录行）", ("变更记录",))


def test_negative_14_version_bump_without_content_change_blocks_emit():
    """负例 14：正文不动、只递增版本号 —— emit 必须拒绝重建（版本变更流程第 2 步的反向违规）。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        bump_version(root, NEXT_VERSION)
        code, output = run_tool(root, "emit")
        assert code != 0, (
            f"负例 14: 正文未变而递增版本号时 emit 不应放行（退出码 {code}）：{output}"
        )
        assert "scripture_version" in output, f"负例 14: 失败信息未指出 scripture_version：{output}"


def test_negative_15_manifest_source_sha256_forged():
    """负例 15：改正文并手改 manifest 的 `source_sha256` 使其自洽，仍不得绕过版本绑定。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        edit_source(root, *SOURCE_EDIT)
        digest = hashlib.sha256(
            (root / CANONICAL_REL / SOURCE_NAME).read_text(encoding="utf-8").rstrip("\n").encode("utf-8")
        ).hexdigest()
        edit_json(
            root / CANONICAL_REL / MANIFEST_NAME,
            lambda data: data.__setitem__("source_sha256", digest),
        )
        code, output = run_tool(root, "emit")
        assert code != 0, (
            f"负例 15: 手改 manifest 的派生字段不得绕过版本绑定（退出码 {code}）：{output}"
        )
        assert "scripture_version" in output, f"负例 15: 失败信息未指出 scripture_version：{output}"


def test_negative_16_history_row_values_are_pinned():
    """负例 16：改写上一版历史行的数值单元格，必须被 manifest 的上一版存档钉住。"""
    for column, forged in ((4, "ffffffffffff"), (2, "999"), (3, "7")):
        with tempfile.TemporaryDirectory() as tmpdir:
            root = make_published_sandbox(tmpdir)
            edit_ledger_row(root, INITIAL_VERSION, column, forged)
            expect_failure(root, f"负例 16（第 {column} 列改写为 {forged}）", ("历史行",))


def test_negative_17_history_row_deleted_or_reordered():
    """负例 17：追加式台账不得删除既有行，也不得重排。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_published_sandbox(tmpdir)
        path = root / CANONICAL_REL / README_NAME
        kept = [
            line
            for line in path.read_text(encoding="utf-8").splitlines(keepends=True)
            if f"| {INITIAL_VERSION} |" not in line
        ]
        path.write_text("".join(kept), encoding="utf-8", newline="\n")
        expect_failure(root, "负例 17（删除上一版行）", (INITIAL_VERSION,))

    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_published_sandbox(tmpdir)
        path = root / CANONICAL_REL / README_NAME
        lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
        rows = [index for index, line in enumerate(lines) if line.startswith("| HS-")]
        assert len(rows) == 2, f"负例 17 期望两条记录行，实际 {len(rows)}"
        first, second = rows
        lines[first], lines[second] = lines[second], lines[first]
        path.write_text("".join(lines), encoding="utf-8", newline="\n")
        expect_failure(root, "负例 17（重排记录行）", ("递增",))


def test_negative_18_previous_archive_missing():
    """负例 18：绕过 emit 手工递增版本号并追加记录行 —— 历史行没有存档，必须失败。"""
    with tempfile.TemporaryDirectory() as tmpdir:
        root = make_sandbox(tmpdir)
        bump_version(root, NEXT_VERSION, products=True)
        readme = root / CANONICAL_REL / README_NAME
        text = readme.read_text(encoding="utf-8").replace("| a265c93dbc60 |", "| ffffffffffff |", 1)
        readme.write_text(
            text + f"| {NEXT_VERSION} | 2026-09-22 | 303 | 260 | a265c93dbc60 | 仅递增版本号，正文未变 |\n",
            encoding="utf-8",
            newline="\n",
        )
        expect_failure(root, "负例 18", ("历史行",))


def test_negative_10_backend_field_missing():
    """负例 10：后端产物缺少必需字段，不得静默跳过该字段比对。"""
    for field in ("title", "sourceSha256", "firstConsumable", "nonConsumableChars"):
        with tempfile.TemporaryDirectory() as tmpdir:
            root = make_sandbox(tmpdir)
            edit_json(root / BACKEND_REL, lambda data, key=field: data.pop(key))
            expect_failure(root, f"负例 10（{field}）", (field,))


def main():
    tests = [value for name, value in sorted(globals().items()) if name.startswith("test_")]
    failures = 0
    for test in tests:
        try:
            test()
        except AssertionError as error:
            failures += 1
            print(f"FAIL {test.__name__}: {error}")
        else:
            print(f"PASS {test.__name__}")
    print(f"canonical: {len(tests) - failures}/{len(tests)} 通过")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
