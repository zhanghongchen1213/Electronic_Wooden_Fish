/*
 * @file  test_power_core_path_policy.c
 * @brief 低电/落盘优先核心路径与电量迟滞主机测试（Story 2.7）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "power_core_path_policy.h"

static unsigned g_failures;

#define CHECK_TRUE(condition)                                                  \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            (void)fprintf(stderr,                                              \
                          "FAIL %s:%d: %s\n",                                  \
                          __func__,                                            \
                          __LINE__,                                            \
                          #condition);                                         \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

#define CHECK_EQ_U32(actual, expected)                                         \
    do                                                                         \
    {                                                                          \
        const unsigned long actual_value = (unsigned long)(actual);             \
        const unsigned long expected_value = (unsigned long)(expected);         \
        if (actual_value != expected_value)                                    \
        {                                                                      \
            (void)fprintf(stderr,                                              \
                          "FAIL %s:%d: %s == %lu, expected %lu\n",             \
                          __func__,                                            \
                          __LINE__,                                            \
                          #actual,                                             \
                          actual_value,                                        \
                          expected_value);                                     \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

static void test_persist_inflight_defers_sync_and_feedback(void)
{
    const ewf_power_core_path_input_t input = {
        .power_level = 0U,
        .persist_pending = false,
        .persist_inflight = true,
        .sync_window_requested = true,
        .feedback_pending = true,
    };
    const ewf_power_core_path_decision_t d = ewf_power_core_path_decide(&input);
    CHECK_TRUE(d.allow_flush);
    CHECK_TRUE(!d.allow_feedback);
    CHECK_TRUE(!d.allow_sync);
    CHECK_TRUE(d.defer_noncritical);
    CHECK_EQ_U32(d.primary_action, EWF_POWER_CORE_ACTION_FLUSH_PROGRESS);
    CHECK_TRUE(!d.software_poweroff_intent);
}

static void test_persist_done_allows_paths(void)
{
    const ewf_power_core_path_input_t input = {
        .power_level = 0U,
        .persist_pending = false,
        .persist_inflight = false,
        .sync_window_requested = true,
        .feedback_pending = false,
    };
    const ewf_power_core_path_decision_t d = ewf_power_core_path_decide(&input);
    CHECK_TRUE(d.allow_flush);
    CHECK_TRUE(d.allow_feedback);
    CHECK_TRUE(d.allow_sync);
    CHECK_TRUE(!d.defer_noncritical);
    CHECK_EQ_U32(d.primary_action, EWF_POWER_CORE_ACTION_ALLOW_SYNC);
    CHECK_TRUE(!d.software_poweroff_intent);
}

static void test_critical_still_allows_flush(void)
{
    const ewf_power_core_path_input_t input = {
        .power_level = 2U, /* CRITICAL */
        .persist_pending = true,
        .persist_inflight = false,
        .sync_window_requested = true,
        .feedback_pending = true,
    };
    const ewf_power_core_path_decision_t d = ewf_power_core_path_decide(&input);
    CHECK_TRUE(d.allow_flush);
    CHECK_TRUE(d.suggest_brightness_cap);
    CHECK_EQ_U32(d.brightness_cap, EWF_POWER_CRITICAL_BRIGHTNESS_CAP);
    CHECK_TRUE(!d.allow_sync);
    CHECK_TRUE(!d.software_poweroff_intent);
}

static void test_warn_does_not_force_defer(void)
{
    const ewf_power_core_path_input_t input = {
        .power_level = 1U, /* WARN */
        .persist_pending = false,
        .persist_inflight = false,
        .sync_window_requested = true,
        .feedback_pending = true,
    };
    const ewf_power_core_path_decision_t d = ewf_power_core_path_decide(&input);
    CHECK_TRUE(d.allow_sync);
    CHECK_TRUE(d.allow_feedback);
    CHECK_TRUE(!d.suggest_brightness_cap);
}

static void test_never_software_poweroff(void)
{
    ewf_power_core_path_input_t input = {0};
    for (unsigned level = 0U; level < 3U; ++level)
    {
        input.power_level = (uint8_t)level;
        input.persist_pending = true;
        input.persist_inflight = true;
        input.sync_window_requested = true;
        input.feedback_pending = true;
        const ewf_power_core_path_decision_t d =
            ewf_power_core_path_decide(&input);
        CHECK_TRUE(!d.software_poweroff_intent);
        CHECK_TRUE(d.allow_flush);
    }
}

static void test_hysteresis_warn_critical(void)
{
    ewf_power_battery_level_state_t state;
    ewf_power_battery_level_reset(&state);
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 50U), 0U);
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 20U), 1U);
    /* 迟滞：21 仍 WARN，23 才回 NORMAL */
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 21U), 1U);
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 23U), 0U);
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 10U), 2U);
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 12U), 2U);
    CHECK_EQ_U32(ewf_power_battery_level_update(&state, 13U), 1U);
}

static void test_no_charging_symbols_in_header(void)
{
    /* 源码合同由 test_bsp_contract 扫描；此处钉死策略侧无充电门控出口。 */
    const ewf_power_core_path_decision_t d = ewf_power_core_path_decide(NULL);
    CHECK_TRUE(d.allow_flush);
    CHECK_TRUE(!d.software_poweroff_intent);
}

int main(void)
{
    test_persist_inflight_defers_sync_and_feedback();
    test_persist_done_allows_paths();
    test_critical_still_allows_flush();
    test_warn_does_not_force_defer();
    test_never_software_poweroff();
    test_hysteresis_warn_critical();
    test_no_charging_symbols_in_header();

    if (g_failures != 0U)
    {
        (void)fprintf(stderr, "power_core_path_policy: %u failures\n", g_failures);
        return 1;
    }
    (void)puts("PASS power_core_path_policy");
    return 0;
}
