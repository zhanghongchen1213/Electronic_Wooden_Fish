/*
 * @file  test_power_auto_mode_policy.c
 * @brief BOOT0 自动模式纯逻辑主机测试（Story 2.6）
 * @details 钉死 toggle、锚点+10×3000 间距、退出零 emit、完成强制停表。
 *          长按/抖动由 boot_policy 过滤；本测直接喂 RUNTIME_TAP 语义。
 */

#include <stdio.h>
#include <stdlib.h>

#include "power_auto_mode_policy.h"

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

static void test_toggle_enable_does_not_emit(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);

    const ewf_power_auto_mode_decision_t d =
        ewf_power_auto_mode_on_runtime_tap(&state, 10000U);
    CHECK_EQ_U32(d.action, EWF_POWER_AUTO_ACTION_ENABLE);
    CHECK_TRUE(d.enabled);
    CHECK_EQ_U32(d.next_due_ms, 13000U);
    CHECK_EQ_U32(d.emit_count, 0U);
    CHECK_EQ_U32(state.anchor_ms, 10000U);
    CHECK_TRUE(state.enabled);
}

static void test_ten_periods_exact_spacing(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 10000U);

    uint32_t previous_emit = 0U;
    for (unsigned i = 1U; i <= 10U; ++i)
    {
        const uint32_t due = 10000U + i * EWF_POWER_AUTO_MODE_PERIOD_MS;
        const ewf_power_auto_mode_decision_t d =
            ewf_power_auto_mode_on_period_due(&state, due, false);
        CHECK_EQ_U32(d.action, EWF_POWER_AUTO_ACTION_EMIT_TAP);
        CHECK_EQ_U32(d.emit_at_ms, due);
        CHECK_EQ_U32(d.emit_count, i);
        if (i > 1U)
        {
            CHECK_EQ_U32(d.emit_at_ms - previous_emit,
                         EWF_POWER_AUTO_MODE_PERIOD_MS);
        }
        previous_emit = d.emit_at_ms;
    }
    CHECK_EQ_U32(state.emit_count, 10U);
    CHECK_EQ_U32(state.next_due_ms, 10000U + 11U * EWF_POWER_AUTO_MODE_PERIOD_MS);
}

static void test_early_tick_is_noop(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 5000U);

    const ewf_power_auto_mode_decision_t d =
        ewf_power_auto_mode_on_period_due(&state, 7999U, false);
    CHECK_EQ_U32(d.action, EWF_POWER_AUTO_ACTION_NOOP);
    CHECK_EQ_U32(state.emit_count, 0U);
    CHECK_TRUE(state.enabled);
}

static void test_disable_stops_emit(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 1000U);
    (void)ewf_power_auto_mode_on_period_due(&state, 4000U, false);
    CHECK_EQ_U32(state.emit_count, 1U);

    const ewf_power_auto_mode_decision_t off =
        ewf_power_auto_mode_on_runtime_tap(&state, 4500U);
    CHECK_EQ_U32(off.action, EWF_POWER_AUTO_ACTION_DISABLE);
    CHECK_TRUE(!off.enabled);
    CHECK_EQ_U32(state.next_due_ms, 0U);
    CHECK_EQ_U32(state.anchor_ms, 0U);

    const ewf_power_auto_mode_decision_t later =
        ewf_power_auto_mode_on_period_due(&state, 7000U, false);
    CHECK_EQ_U32(later.action, EWF_POWER_AUTO_ACTION_NOOP);
    CHECK_EQ_U32(state.emit_count, 1U);
}

static void test_completed_forces_disable_without_emit(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 2000U);

    const ewf_power_auto_mode_decision_t d =
        ewf_power_auto_mode_on_period_due(&state, 5000U, true);
    CHECK_EQ_U32(d.action, EWF_POWER_AUTO_ACTION_DISABLE);
    CHECK_TRUE(!d.enabled);
    CHECK_EQ_U32(state.emit_count, 0U);
    CHECK_EQ_U32(state.next_due_ms, 0U);
}

static void test_force_disable_idempotent(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 100U);
    const ewf_power_auto_mode_decision_t first =
        ewf_power_auto_mode_force_disable(&state, 200U);
    CHECK_EQ_U32(first.action, EWF_POWER_AUTO_ACTION_DISABLE);
    const ewf_power_auto_mode_decision_t second =
        ewf_power_auto_mode_force_disable(&state, 300U);
    CHECK_EQ_U32(second.action, EWF_POWER_AUTO_ACTION_NOOP);
}

static void test_disabled_period_is_noop(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    const ewf_power_auto_mode_decision_t d =
        ewf_power_auto_mode_on_period_due(&state, 9999U, false);
    CHECK_EQ_U32(d.action, EWF_POWER_AUTO_ACTION_NOOP);
}

static void test_toggle_reenable_new_anchor(void)
{
    ewf_power_auto_mode_state_t state;
    ewf_power_auto_mode_reset(&state);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 1000U);
    (void)ewf_power_auto_mode_on_runtime_tap(&state, 2000U);
    const ewf_power_auto_mode_decision_t again =
        ewf_power_auto_mode_on_runtime_tap(&state, 9000U);
    CHECK_EQ_U32(again.action, EWF_POWER_AUTO_ACTION_ENABLE);
    CHECK_EQ_U32(state.anchor_ms, 9000U);
    CHECK_EQ_U32(state.next_due_ms, 12000U);
    /* emit_count 保留诊断累计，不因重新开启清零。 */
    CHECK_EQ_U32(state.emit_count, 0U);
}

int main(void)
{
    test_toggle_enable_does_not_emit();
    test_ten_periods_exact_spacing();
    test_early_tick_is_noop();
    test_disable_stops_emit();
    test_completed_forces_disable_without_emit();
    test_force_disable_idempotent();
    test_disabled_period_is_noop();
    test_toggle_reenable_new_anchor();

    if (g_failures != 0U)
    {
        (void)fprintf(stderr, "power_auto_mode_policy: %u 项失败\n", g_failures);
        return 1;
    }
    (void)puts("PASS power_auto_mode_policy");
    return 0;
}
