#!/usr/bin/env python3
"""EWF Story 1.1 主机测试入口。

一次运行两类门禁：
1. 源码合同扫描（tests/test_bsp_contract.py，纯 Python）；
2. PWR_INT/BOOT0 边沿语义（tests/test_power_boot_policy.c，用本机 C 编译器构建后运行）。

不依赖 ESP-IDF，也不执行任何烧录或串口动作。
"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
EMBEDDED_DIR = TESTS_DIR.parent
POLICY_DIR = EMBEDDED_DIR / "components/services/power_service"
TAP_POLICY_DIR = EMBEDDED_DIR / "components/services/tap_input_service"
PROGRESS_DIR = EMBEDDED_DIR / "components/services/progress_service"
FEEDBACK_DIR = EMBEDDED_DIR / "components/services/feedback_service"
CANONICAL_DIR = EMBEDDED_DIR.parent / "docs/contracts/canonical/generated"


def run_source_contracts() -> int:
    sys.path.insert(0, str(TESTS_DIR))
    import test_bsp_contract  # noqa: PLC0415

    failures = 0
    executed = 0
    for name in sorted(vars(test_bsp_contract)):
        if not name.startswith("test_"):
            continue
        function = getattr(test_bsp_contract, name)
        if not callable(function):
            continue
        executed += 1
        try:
            function()
        except AssertionError as error:
            failures += 1
            print(f"FAIL {name}: {error}")
        else:
            print(f"PASS {name}")
    print(f"source-contract: {executed - failures}/{executed} 通过")
    return failures


def run_edge_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL edge-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_power_boot_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(POLICY_DIR),
                str(TESTS_DIR / "test_power_boot_policy.c"),
                str(POLICY_DIR / "power_boot_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL edge-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        if run.returncode != 0:
            print("FAIL edge-policy: PWR/BOOT 边沿语义用例未全部通过")
            return 1
    return 0


def run_tap_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL tap-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_tap_input_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(TAP_POLICY_DIR),
                str(TESTS_DIR / "test_tap_input_policy.c"),
                str(TAP_POLICY_DIR / "tap_input_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL tap-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_tap_runtime_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL tap-runtime: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_tap_input_runtime"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(TAP_POLICY_DIR),
                "-I",
                str(TESTS_DIR / "host_stubs"),
                "-I",
                str(EMBEDDED_DIR / "components/app_state"),
                "-I",
                str(EMBEDDED_DIR / "components/services/state_service"),
                "-I",
                str(EMBEDDED_DIR / "components/services"),
                "-I",
                str(EMBEDDED_DIR / "components/services/pvdf_input_service"),
                "-I",
                str(EMBEDDED_DIR / "components/platform/event_bus"),
                str(TESTS_DIR / "test_tap_input_runtime.c"),
                str(TAP_POLICY_DIR / "tap_input_policy.c"),
                str(TESTS_DIR / "host_stubs/host_platform.c"),
                str(TESTS_DIR / "host_stubs/state_service_host.c"),
                str(TESTS_DIR / "host_stubs/pvdf_service_host.c"),
                str(TESTS_DIR / "host_stubs/cst9217_bsp_host.c"),
                str(EMBEDDED_DIR / "components/app_state/watch_state.c"),
                str(EMBEDDED_DIR / "components/services/tap_input_service/tap_input_service.c"),
                str(EMBEDDED_DIR / "components/services/pvdf_input_service/pvdf_confirm_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL tap-runtime: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_progress_transaction_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL progress-transaction: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_progress_transaction"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(PROGRESS_DIR),
                "-I",
                str(TAP_POLICY_DIR),
                "-I",
                str(CANONICAL_DIR),
                str(TESTS_DIR / "test_progress_transaction.c"),
                str(PROGRESS_DIR / "progress_transaction.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL progress-transaction: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_progress_runtime_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL progress-runtime: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_progress_runtime"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(PROGRESS_DIR),
                "-I",
                str(TAP_POLICY_DIR),
                "-I",
                str(TESTS_DIR / "host_stubs"),
                "-I",
                str(EMBEDDED_DIR / "components/app_state"),
                "-I",
                str(EMBEDDED_DIR / "components/services/state_service"),
                "-I",
                str(EMBEDDED_DIR / "components/services"),
                "-I",
                str(EMBEDDED_DIR / "components/platform/event_bus"),
                "-I",
                str(CANONICAL_DIR),
                str(TESTS_DIR / "test_progress_runtime.c"),
                str(PROGRESS_DIR / "progress_transaction.c"),
                str(PROGRESS_DIR / "progress_service.c"),
                str(TESTS_DIR / "host_stubs/progress_store_host.c"),
                str(TESTS_DIR / "host_stubs/host_platform.c"),
                str(TESTS_DIR / "host_stubs/state_service_host.c"),
                str(EMBEDDED_DIR / "components/app_state/watch_state.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL progress-runtime: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_feedback_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL feedback-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_feedback_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(FEEDBACK_DIR),
                str(TESTS_DIR / "test_feedback_policy.c"),
                str(FEEDBACK_DIR / "feedback_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL feedback-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_feedback_runtime_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL feedback-runtime: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_feedback_runtime"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(FEEDBACK_DIR),
                "-I",
                str(TAP_POLICY_DIR),
                "-I",
                str(TESTS_DIR / "host_stubs"),
                "-I",
                str(EMBEDDED_DIR / "components/app_state"),
                "-I",
                str(EMBEDDED_DIR / "components/services/state_service"),
                "-I",
                str(EMBEDDED_DIR / "components/services"),
                "-I",
                str(EMBEDDED_DIR / "components/platform/event_bus"),
                "-I",
                str(EMBEDDED_DIR / "components/BSP/RGB"),
                str(TESTS_DIR / "test_feedback_runtime.c"),
                str(FEEDBACK_DIR / "feedback_policy.c"),
                str(FEEDBACK_DIR / "feedback_service.c"),
                str(TESTS_DIR / "host_stubs/feedback_host.c"),
                str(TESTS_DIR / "host_stubs/host_platform.c"),
                str(TESTS_DIR / "host_stubs/state_service_host.c"),
                str(EMBEDDED_DIR / "components/app_state/watch_state.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL feedback-runtime: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def main() -> int:
    failures = run_source_contracts()
    failures += run_edge_policy_host_test()
    failures += run_tap_policy_host_test()
    failures += run_tap_runtime_host_test()
    failures += run_progress_transaction_host_test()
    failures += run_progress_runtime_host_test()
    failures += run_feedback_policy_host_test()
    failures += run_feedback_runtime_host_test()
    if failures != 0:
        print("host tests: 失败")
        return 1
    print("host tests: 全部通过")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
