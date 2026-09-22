"""EWF Story 1.1 的无硬件合同测试（可在主机重复运行）。

覆盖两条门禁：
1. BSP 资源表与 PWR_INT/BOOT0 只读输入边界符合硬件事实源；
2. 生产代码（EWF 构建图内）不得残留旧 GPIO46 PWR、GPS/ML76KB、ML307R、vibration、
   QMI INT2 IO45、LEGBOT_CAP_* 默认启动、BLE/cloud 自动启动或软件关机 API。
"""

from pathlib import Path

ROOT = Path(__file__).parents[1]

BSP_RES_H = ROOT / "components/BSP/BSP_INCLUDE/bsp_resources.h"
BSP_RES_C = ROOT / "components/BSP/BSP_INCLUDE/bsp_resources.c"
KEY_C = ROOT / "components/BSP/KEY/key.c"
BOARD_C = ROOT / "components/BSP/BOARD/bsp_board.c"
POWER_C = ROOT / "components/services/power_service/power_service.c"
POWER_POLICY_C = ROOT / "components/services/power_service/power_boot_policy.c"
SELFTEST_C = ROOT / "components/services/selftest_service/selftest_service.c"
STATE_C = ROOT / "components/services/state_service/state_service.c"
SERVICES_CMAKE = ROOT / "components/services/CMakeLists.txt"
SERVICES_LEGACY = (
    "audio_service",
    "ble_service",
    "cloud_service",
    "gps_service",
    "modem_service",
    "voice_service",
    "time_service",
    "maintenance_service",
    "ui_service",
    "log_service",
)

# 允许出现 GPIO_NUM_46/45 与 LEGBOT_CAP_ 之外的保留脚字面量的文件。
RESERVED_PIN_ALLOWLIST = {
    Path("components/BSP/BSP_INCLUDE/bsp_resources.h"),
    Path("components/BSP/BSP_INCLUDE/bsp_resources.c"),
    Path("components/contract_checks/contract_checks.c"),
}

# EWF 生产代码集合：只有进入默认构建图与启动图的内容。
PRODUCTION_SOURCES = [
    Path("CMakeLists.txt"),
    Path("sdkconfig.defaults"),
    Path("main/CMakeLists.txt"),
    Path("main/app_main.c"),
    Path("main/Kconfig.projbuild"),
    *sorted(p.relative_to(ROOT) for p in (ROOT / "components/BSP").rglob("*") if p.suffix in {".c", ".h"}),
    *sorted(p.relative_to(ROOT) for p in (ROOT / "components/platform").rglob("*") if p.suffix in {".c", ".h"}),
    *sorted(p.relative_to(ROOT) for p in (ROOT / "components/app_state").rglob("*") if p.suffix in {".c", ".h"}),
    *sorted(p.relative_to(ROOT) for p in (ROOT / "components/contract_checks").rglob("*") if p.suffix in {".c", ".h"}),
    *sorted(p.relative_to(ROOT) for p in (ROOT / "components/control_gate").rglob("*") if p.suffix in {".c", ".h"}),
    *sorted(p.relative_to(ROOT) for p in (ROOT / "components/services").rglob("*") if p.suffix in {".c", ".h"}),
]

# 旧默认启动前提、旧板级所有者与软件关机 API 的禁止符号。
# 只匹配真实代码/资源所有权符号，不匹配说明“已移除这些链路”的审计注释。
FORBIDDEN_TOKENS = (
    "legbot_cap_",
    "scenic_area_management_debug",
    "ml307r_bsp",
    "ml307r_https",
    "l76kb_a58",
    "vibration_bsp",
    "legbot_bsp_gps_",
    "legbot_bsp_ml307r_",
    "legbot_bsp_vibration_",
    "charging_pause",
    "charge_state",
    "is_charging",
    "ui_service_",
    "ble_service_",
    "gps_service_",
    "modem_service_",
    "cloud_service_",
    "audio_service_",
    "voice_service_",
    "maintenance_service_",
    "time_service_",
    "radio_power_",
    "esp_deep_sleep_start",
    "esp_restart",
)


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def test_power_and_boot_pins_are_read_only_contract():
    resources = _read(BSP_RES_H)
    key = _read(KEY_C)
    assert "EWF_BSP_PWR_INT_GPIO GPIO_NUM_8" in resources
    assert "EWF_BSP_BOOT0_GPIO GPIO_NUM_0" in resources
    assert "EWF_BSP_PWR_INT_ACTIVE_LEVEL 0U" in resources
    assert "GPIO_INTR_ANYEDGE" in key
    assert "gpio_set_direction" not in key
    assert "GPIO_NUM_46" in resources and "GPIO_NUM_45" in resources


def test_reserved_and_unowned_pins_are_not_firmware_driven():
    resources = _read(BSP_RES_H)
    assert "EWF_BSP_EN_RESET_N_GPIO GPIO_NUM_NC" in resources
    assert "EWF_BSP_PWR_STATE_LTC2954_EN_GPIO GPIO_NUM_NC" in resources
    assert "EWF_BSP_KILL_LTC2954_GPIO GPIO_NUM_NC" in resources
    assert "LEGBOT_BSP_QMI8658C_INT2_GPIO GPIO_NUM_NC" in resources
    assert "EWF_BSP_PVDF_ADC_GPIO GPIO_NUM_9" in resources
    assert "EWF_BSP_PVDF_CMP_WAKE_GPIO GPIO_NUM_11" in resources


def test_legacy_resources_are_not_started_by_board_bsp():
    board = _read(BOARD_C)
    assert "l76kb_a58_bsp.h" not in board
    assert "ml307r_bsp.h" not in board
    assert "vibration_bsp.h" not in board
    assert "旧 GPS/ML307R/振动未启动" in board


def test_no_software_poweroff_or_charging_gate_in_production():
    bsp_source = "\n".join(
        p.read_text(encoding="utf-8")
        for p in (ROOT / "components/BSP").rglob("*.c")
    )
    assert "esp_deep_sleep_start" not in bsp_source
    assert "esp_restart" not in bsp_source

    services = _read(POWER_C) + _read(SELFTEST_C) + _read(STATE_C)
    assert "esp_deep_sleep_start" not in services
    assert "esp_restart" not in services


def test_power_service_owns_the_pwr_boot_edge_boundary():
    power = _read(POWER_C)
    assert "key_register_pwr_isr_callback" in power
    assert "key_register_boot_isr_callback" in power
    assert "key_rearm_pwr_isr_for_next_level" in power
    assert "key_rearm_boot_isr_for_next_level" in power
    # ISR 只投递 ISR-safe 消息，不得在中断上下文打印或阻塞。
    assert "xQueueSendFromISR" in power
    assert "vTaskDelay" not in power.split("static void power_service_pwr_isr")[-1]


def test_services_component_only_holds_the_ewf_startup_graph():
    cmake = _read(SERVICES_CMAKE)
    # 只核对真正注册进构建图的路径，注释里的审计说明不算残留。
    registered = {
        token
        for line in cmake.splitlines()
        if not line.lstrip().startswith("#")
        for token in line.replace('"', " ").split()
    }
    assert "legbot_services.c" in registered
    assert "power_service/power_service.c" in registered
    assert "power_service/power_boot_policy.c" in registered
    assert "selftest_service/selftest_service.c" in registered
    assert "state_service/state_service.c" in registered
    assert "esp-sr" not in registered
    assert "lvgl" not in registered
    assert "ui" not in registered
    for legacy in SERVICES_LEGACY:
        assert legacy not in registered, f"{legacy} 仍在 services 构建图中"


def test_legacy_service_directories_are_removed():
    services_dir = ROOT / "components/services"
    for legacy in SERVICES_LEGACY:
        assert not (services_dir / legacy).exists(), f"{legacy} 必须已移出生产启动图"


def test_selftest_requires_evidence_and_forbids_unproven_hardware_claim():
    selftest = _read(SELFTEST_C)
    assert "SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE == 0U" in selftest
    assert "SELFTEST_EVIDENCE_HARDWARE_VERIFIED" in selftest
    assert "SELFTEST_EVIDENCE_HARDWARE_PENDING" in selftest
    assert "SELFTEST_REASON_HANDLER_MISSING" in selftest


def test_production_sources_are_free_of_legacy_defaults():
    offenders = []
    for relative in PRODUCTION_SOURCES:
        path = ROOT / relative
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8").lower()
        for token in FORBIDDEN_TOKENS:
            if token in text:
                offenders.append(f"{relative}: {token}")
        if relative not in RESERVED_PIN_ALLOWLIST:
            if "gpio_num_46" in text or "gpio_num_45" in text:
                offenders.append(f"{relative}: 占用了保留脚 GPIO_NUM_46/45")
    assert not offenders, "生产代码残留旧启动前提：" + "; ".join(offenders)


def test_main_entry_only_runs_the_ewf_stage_sequence():
    app_main = _read(ROOT / "main/app_main.c")
    for stage in (
        "LEGBOT_BSP_STAGE_LOG_NVS_CONFIG",
        "LEGBOT_BSP_STAGE_EVENT_STATE",
        "LEGBOT_BSP_STAGE_BOARD_PINS",
        "LEGBOT_BSP_STAGE_SHARED_BUSES",
        "LEGBOT_BSP_STAGE_SERVICES",
        "LEGBOT_BSP_STAGE_SELFTEST_HOOKS",
    ):
        assert stage in app_main
    assert "l76kb_a58_bsp" not in app_main
    assert "ml307r_bsp" not in app_main
    assert "cloud_provision" not in app_main
    assert "legbot_bsp_record_stage_result" in app_main


def test_power_boot_policy_is_a_pure_host_testable_module():
    policy = _read(POWER_POLICY_C)
    assert "#include" in policy
    assert "esp_log.h" not in policy
    assert "freertos/" not in policy
    assert "driver/gpio.h" not in policy
