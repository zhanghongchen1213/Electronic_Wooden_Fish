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
DEVICE_NAV_DIR = EMBEDDED_DIR / "components/services/device_nav_service"
SYNC_DIR = EMBEDDED_DIR / "components/services/sync_service"
UI_BIND_DIR = EMBEDDED_DIR / "components/ui/bindings"
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


def run_auto_mode_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL auto-mode-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_power_auto_mode_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(POLICY_DIR),
                str(TESTS_DIR / "test_power_auto_mode_policy.c"),
                str(POLICY_DIR / "power_auto_mode_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL auto-mode-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        if run.returncode != 0:
            print("FAIL auto-mode-policy: 自动模式策略用例未全部通过")
            return 1
    return 0


def run_core_path_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL core-path-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_power_core_path_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(POLICY_DIR),
                str(TESTS_DIR / "test_power_core_path_policy.c"),
                str(POLICY_DIR / "power_core_path_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL core-path-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        if run.returncode != 0:
            print("FAIL core-path-policy: 核心路径策略用例未全部通过")
            return 1
    return 0


def run_fault_gate_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL fault-gate-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_fault_gate_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(PROGRESS_DIR),
                str(TESTS_DIR / "test_fault_gate_policy.c"),
                str(PROGRESS_DIR / "fault_gate_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL fault-gate-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        if run.returncode != 0:
            print("FAIL fault-gate-policy: 故障闸门策略用例未全部通过")
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
                "-I",
                str(DEVICE_NAV_DIR),
                "-I",
                str(UI_BIND_DIR),
                str(TESTS_DIR / "test_tap_input_runtime.c"),
                str(TAP_POLICY_DIR / "tap_input_policy.c"),
                str(DEVICE_NAV_DIR / "device_nav_policy.c"),
                str(UI_BIND_DIR / "ui_jingwen_gesture_policy.c"),
                str(TESTS_DIR / "host_stubs/host_platform.c"),
                str(TESTS_DIR / "host_stubs/state_service_host.c"),
                str(TESTS_DIR / "host_stubs/pvdf_service_host.c"),
                str(TESTS_DIR / "host_stubs/cst9217_bsp_host.c"),
                str(TESTS_DIR / "host_stubs/device_nav_host.c"),
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


def run_progress_round_action_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL progress-round-action: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_progress_round_action"
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
                str(TESTS_DIR / "test_progress_round_action.c"),
                str(PROGRESS_DIR / "progress_transaction.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL progress-round-action: 主机测试构建失败")
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
                str(PROGRESS_DIR / "fault_gate_policy.c"),
                str(PROGRESS_DIR / "today_bucket_policy.c"),
                str(PROGRESS_DIR / "progress_service.c"),
                str(TESTS_DIR / "host_stubs/progress_store_host.c"),
                str(TESTS_DIR / "host_stubs/today_bucket_store_host.c"),
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
                "-I",
                str(DEVICE_NAV_DIR),
                "-I",
                str(PROGRESS_DIR),
                "-I",
                str(POLICY_DIR),
                str(TESTS_DIR / "test_feedback_runtime.c"),
                str(FEEDBACK_DIR / "feedback_policy.c"),
                str(FEEDBACK_DIR / "feedback_service.c"),
                str(POLICY_DIR / "power_core_path_policy.c"),
                str(DEVICE_NAV_DIR / "device_settings_policy.c"),
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


def run_device_nav_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL device-nav-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_device_nav_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(DEVICE_NAV_DIR),
                str(TESTS_DIR / "test_device_nav_policy.c"),
                str(DEVICE_NAV_DIR / "device_nav_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL device-nav-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_device_settings_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL device-settings-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_device_settings_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(DEVICE_NAV_DIR),
                str(TESTS_DIR / "test_device_settings_policy.c"),
                str(DEVICE_NAV_DIR / "device_settings_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL device-settings-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_device_nav_runtime_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL device-nav-runtime: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_device_nav_runtime"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(DEVICE_NAV_DIR),
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
                str(TESTS_DIR / "test_device_nav_runtime.c"),
                str(DEVICE_NAV_DIR / "device_nav_policy.c"),
                str(DEVICE_NAV_DIR / "device_settings_policy.c"),
                str(DEVICE_NAV_DIR / "device_nav_service.c"),
                str(TESTS_DIR / "host_stubs/device_settings_store_host.c"),
                str(TESTS_DIR / "host_stubs/co5300_bsp_host.c"),
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
            print("FAIL device-nav-runtime: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_sync_window_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL sync-window-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_sync_window_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(SYNC_DIR),
                str(TESTS_DIR / "test_sync_window_policy.c"),
                str(SYNC_DIR / "sync_window_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL sync-window-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_sync_response_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL sync-response-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_sync_response_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(SYNC_DIR),
                str(TESTS_DIR / "test_sync_response_policy.c"),
                str(SYNC_DIR / "sync_response_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL sync-response-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_sync_https_codec_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL sync-https-codec: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_sync_https_codec"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(SYNC_DIR),
                str(TESTS_DIR / "test_sync_https_codec.c"),
                str(SYNC_DIR / "sync_https_codec.c"),
                str(SYNC_DIR / "sync_https_mock.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL sync-https-codec: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_sync_runtime_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL sync-runtime: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_sync_runtime"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-DEWF_SYNC_TRANSPORT_MOCK=1",
                "-DEWF_SYNC_TRANSPORT_AIR780=0",
                "-I",
                str(SYNC_DIR),
                "-I",
                str(DEVICE_NAV_DIR),
                "-I",
                str(PROGRESS_DIR),
                "-I",
                str(TAP_POLICY_DIR),
                "-I",
                str(FEEDBACK_DIR),
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
                "-I",
                str(POLICY_DIR),
                str(TESTS_DIR / "test_sync_runtime.c"),
                str(SYNC_DIR / "sync_window_policy.c"),
                str(SYNC_DIR / "sync_response_policy.c"),
                str(SYNC_DIR / "sync_https_codec.c"),
                str(SYNC_DIR / "sync_https_mock.c"),
                str(SYNC_DIR / "sync_service.c"),
                str(POLICY_DIR / "power_core_path_policy.c"),
                str(DEVICE_NAV_DIR / "device_nav_policy.c"),
                str(DEVICE_NAV_DIR / "device_settings_policy.c"),
                str(DEVICE_NAV_DIR / "device_nav_service.c"),
                str(TESTS_DIR / "host_stubs/device_settings_store_host.c"),
                str(TESTS_DIR / "host_stubs/co5300_bsp_host.c"),
                str(TESTS_DIR / "host_stubs/host_platform.c"),
                str(TESTS_DIR / "host_stubs/state_service_host.c"),
                str(TESTS_DIR / "host_stubs/sync_runtime_host.c"),
                str(EMBEDDED_DIR / "components/app_state/watch_state.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL sync-runtime: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode



def run_ui_shell_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL ui-shell-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1
    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_ui_shell_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(UI_BIND_DIR),
                str(TESTS_DIR / "test_ui_shell_policy.c"),
                str(UI_BIND_DIR / "ui_shell_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL ui-shell-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_ui_shell_contract_tests() -> int:
    sys.path.insert(0, str(TESTS_DIR))
    import test_ui_shell_contract  # noqa: PLC0415
    failures = 0
    executed = 0
    for name in sorted(vars(test_ui_shell_contract)):
        if not name.startswith("test_"):
            continue
        function = getattr(test_ui_shell_contract, name)
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
    print(f"ui-shell-contract: {executed - failures}/{executed} 通过")
    return failures

def run_ui_muyu_belt_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL ui-muyu-belt-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1
    canonical = EMBEDDED_DIR.parent / "docs/contracts/canonical/generated"
    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_ui_muyu_belt_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(UI_BIND_DIR),
                "-I",
                str(canonical),
                str(TESTS_DIR / "test_ui_muyu_belt_policy.c"),
                str(UI_BIND_DIR / "ui_muyu_belt_policy.c"),
                str(UI_BIND_DIR / "ui_scripture_display_expand.c"),
                str(UI_BIND_DIR / "ui_tap_rings_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL ui-muyu-belt-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_ui_jingwen_stream_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL ui-jingwen-stream-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1
    canonical = EMBEDDED_DIR.parent / "docs/contracts/canonical/generated"
    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_ui_jingwen_stream_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(UI_BIND_DIR),
                "-I",
                str(canonical),
                str(TESTS_DIR / "test_ui_jingwen_stream_policy.c"),
                str(UI_BIND_DIR / "ui_jingwen_stream_policy.c"),
                str(UI_BIND_DIR / "ui_jingwen_gesture_policy.c"),
                str(UI_BIND_DIR / "ui_scripture_display_expand.c"),
                str(UI_BIND_DIR / "ui_muyu_belt_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL ui-jingwen-stream-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_ui_tongji_stats_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL ui-tongji-stats-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1
    canonical = EMBEDDED_DIR.parent / "docs/contracts/canonical/generated"
    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_ui_tongji_stats_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(UI_BIND_DIR),
                "-I",
                str(PROGRESS_DIR),
                "-I",
                str(canonical),
                str(TESTS_DIR / "test_ui_tongji_stats_policy.c"),
                str(UI_BIND_DIR / "ui_tongji_stats_policy.c"),
                str(UI_BIND_DIR / "ui_muyu_belt_policy.c"),
                str(UI_BIND_DIR / "ui_scripture_display_expand.c"),
                str(PROGRESS_DIR / "today_bucket_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL ui-tongji-stats-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_ui_shezhi_settings_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL ui-shezhi-settings-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1
    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_ui_shezhi_settings_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(UI_BIND_DIR),
                str(TESTS_DIR / "test_ui_shezhi_settings_policy.c"),
                str(UI_BIND_DIR / "ui_shezhi_settings_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL ui-shezhi-settings-policy: 主机测试构建失败")
            print(build.stdout)
            print(build.stderr)
            return 1
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=False)
        sys.stdout.write(run.stdout)
        sys.stderr.write(run.stderr)
        return run.returncode


def run_ui_muyu_done_policy_host_test() -> int:
    compiler = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if compiler is None:
        print("FAIL ui-muyu-done-policy: 未找到 cc/gcc/clang，无法构建主机测试")
        return 1
    with tempfile.TemporaryDirectory() as tmpdir:
        binary = Path(tmpdir) / "test_ui_muyu_done_policy"
        build = subprocess.run(
            [
                compiler,
                "-std=c17",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I",
                str(UI_BIND_DIR),
                str(TESTS_DIR / "test_ui_muyu_done_policy.c"),
                str(UI_BIND_DIR / "ui_muyu_done_policy.c"),
                "-o",
                str(binary),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if build.returncode != 0:
            print("FAIL ui-muyu-done-policy: 主机测试构建失败")
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
    failures += run_auto_mode_policy_host_test()
    failures += run_core_path_policy_host_test()
    failures += run_fault_gate_policy_host_test()
    failures += run_tap_policy_host_test()
    failures += run_tap_runtime_host_test()
    failures += run_progress_transaction_host_test()
    failures += run_progress_round_action_host_test()
    failures += run_progress_runtime_host_test()
    failures += run_feedback_policy_host_test()
    failures += run_feedback_runtime_host_test()
    failures += run_device_nav_policy_host_test()
    failures += run_device_settings_policy_host_test()
    failures += run_device_nav_runtime_host_test()
    failures += run_sync_window_policy_host_test()
    failures += run_sync_response_policy_host_test()
    failures += run_sync_https_codec_host_test()
    failures += run_sync_runtime_host_test()
    failures += run_ui_shell_policy_host_test()
    failures += run_ui_shell_contract_tests()
    failures += run_ui_muyu_belt_policy_host_test()
    failures += run_ui_jingwen_stream_policy_host_test()
    failures += run_ui_tongji_stats_policy_host_test()
    failures += run_ui_shezhi_settings_policy_host_test()
    failures += run_ui_muyu_done_policy_host_test()
    if failures != 0:
        print("host tests: 失败")
        return 1
    print("host tests: 全部通过")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
