#!/usr/bin/env python3
"""为固定 ESP-IDF v5.5.4 生成带官方 light sleep 修复的 esp_pm 实现。"""

from __future__ import annotations

import argparse
import hashlib
import pathlib


EXPECTED_SOURCE_SHA256 = (
    "235a2c89abb161ba48f453e5c196860be099d2e015b9c1e3a7a59fdf1d3f8d2c"
)
OFFICIAL_REJECT_FIX_COMMIT = "47651df5677f4397a90e95db43cf36dfff2f2fbe"
OFFICIAL_OVERFLOW_FIX_COMMIT = "cf94981732ff02d638b383ae5b2a3542d89bd0c6"

_INCLUDE_ANCHOR = "#include <stdint.h>\n"
_INCLUDE_REPLACEMENT = "#include <stdint.h>\n#include <inttypes.h>\n"

_TOLERANCE_ANCHOR = "#define LIGHT_SLEEP_EARLY_WAKEUP_US 100\n"
_TOLERANCE_REPLACEMENT = """#define LIGHT_SLEEP_EARLY_WAKEUP_US 100

/* 官方回移保护允许的最大 light sleep 超时 tick 数。 */
#define LEGBOT_LIGHT_SLEEP_TICK_OVERFLOW_TOLERANCE 2U
"""

_OLD_SLEEP_BLOCK = """            int64_t sleep_start = esp_timer_get_time();
            if (esp_light_sleep_start() != ESP_OK){
#ifdef WITH_PROFILING
                s_light_sleep_reject_counts++;
            } else {
                s_light_sleep_counts++;
#endif
            }
            slept_us = esp_timer_get_time() - sleep_start;
            ESP_PM_TRACE_EXIT(SLEEP, core_id);

            uint32_t slept_ticks = slept_us / (portTICK_PERIOD_MS * 1000LL);
            if (slept_ticks > 0) {
                /* Adjust RTOS tick count based on the amount of time spent in sleep */
                vTaskStepTick(slept_ticks);

#ifdef CONFIG_FREERTOS_SYSTICK_USES_CCOUNT
                /* Trigger tick interrupt, since sleep time was longer
                 * than portTICK_PERIOD_MS. Note that setting INTSET does not
                 * work for timer interrupt, and changing CCOMPARE would clear
                 * the interrupt flag.
                 */
                esp_cpu_set_cycle_count(XTHAL_GET_CCOMPARE(XT_TIMER_INDEX) - 16);
                while (!(XTHAL_GET_INTERRUPT() & BIT(XT_TIMER_INTNUM))) {
                    ;
                }
#else
                portYIELD_WITHIN_API();
#endif
            }
            other_core_should_skip_light_sleep(core_id);
"""

_NEW_SLEEP_BLOCK = """            int64_t sleep_start = esp_timer_get_time();
            esp_err_t sleep_error = esp_light_sleep_start();
            slept_us = esp_timer_get_time() - sleep_start;
            ESP_PM_TRACE_EXIT(SLEEP, core_id);

#ifdef WITH_PROFILING
            if (sleep_error == ESP_OK) {
                s_light_sleep_counts++;
            } else {
                s_light_sleep_reject_counts++;
            }
#endif

            /* 被硬件拒绝的休眠未停止系统 tick，不得再次补偿。 */
            if (sleep_error == ESP_OK) {
                uint32_t slept_ticks = slept_us / (portTICK_PERIOD_MS * 1000LL);
                if ((slept_ticks > xExpectedIdleTime) &&
                    (slept_ticks <= (xExpectedIdleTime +
                                     LEGBOT_LIGHT_SLEEP_TICK_OVERFLOW_TOLERANCE))) {
                    slept_ticks = xExpectedIdleTime;
                }
                if (slept_ticks > xExpectedIdleTime) {
                    ESP_EARLY_LOGE(TAG,
                                   "light sleep 严重超时：预计=%" PRIu32 " tick，实际=%" PRIu32 " tick",
                                   (uint32_t)xExpectedIdleTime,
                                   slept_ticks);
                }
                if (slept_ticks > 0) {
                    /* Adjust RTOS tick count based on the amount of time spent in sleep */
                    vTaskStepTick(slept_ticks);

#ifdef CONFIG_FREERTOS_SYSTICK_USES_CCOUNT
                    /* Trigger tick interrupt, since sleep time was longer
                     * than portTICK_PERIOD_MS. Note that setting INTSET does not
                     * work for timer interrupt, and changing CCOMPARE would clear
                     * the interrupt flag.
                     */
                    esp_cpu_set_cycle_count(XTHAL_GET_CCOMPARE(XT_TIMER_INDEX) - 16);
                    while (!(XTHAL_GET_INTERRUPT() & BIT(XT_TIMER_INTNUM))) {
                        ;
                    }
#else
                    portYIELD_WITHIN_API();
#endif
                }
            }
            other_core_should_skip_light_sleep(core_id);
"""


def patch_source(source: str) -> str:
    """只对已经核对的三个锚点执行确定性替换。"""
    replacements = (
        (_INCLUDE_ANCHOR, _INCLUDE_REPLACEMENT),
        (_TOLERANCE_ANCHOR, _TOLERANCE_REPLACEMENT),
        (_OLD_SLEEP_BLOCK, _NEW_SLEEP_BLOCK),
    )
    patched = source
    for old, new in replacements:
        if patched.count(old) != 1:
            raise ValueError("ESP-IDF esp_pm 源码锚点数量异常，拒绝生成不确定补丁")
        patched = patched.replace(old, new, 1)
    return patched


def generate(source_path: pathlib.Path, output_path: pathlib.Path) -> None:
    """校验固定版本源码后生成构建目录副本，不修改外部 ESP-IDF。"""
    source_bytes = source_path.read_bytes()
    actual_hash = hashlib.sha256(source_bytes).hexdigest()
    if actual_hash != EXPECTED_SOURCE_SHA256:
        raise ValueError(
            "仅允许为已核对的 ESP-IDF v5.5.4 pm_impl.c 生成补丁："
            f"期望 SHA256={EXPECTED_SOURCE_SHA256}，实际={actual_hash}"
        )
    patched = patch_source(source_bytes.decode("utf-8"))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(patched, encoding="utf-8", newline="\n")


def main() -> None:
    """解析命令行并生成修复后的构建输入。"""
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    arguments = parser.parse_args()
    generate(arguments.source, arguments.output)


if __name__ == "__main__":
    main()
