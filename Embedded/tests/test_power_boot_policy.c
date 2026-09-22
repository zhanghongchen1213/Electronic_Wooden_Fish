/*
 * @file  test_power_boot_policy.c
 * @brief PWR_INT/BOOT0 边沿语义的主机可重复测试
 * @details 只依赖 power_boot_policy.c 的纯逻辑，使用 gcc/clang 即可在 macOS 与 CI 上重复运行。
 */

#include <stdio.h>
#include <stdlib.h>

#include "power_boot_policy.h"

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

/* 持续低电平的下降/上升边只生成一组按压区间。 */
static void test_pwr_sustained_low_generates_one_interval(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);

    ewf_power_boot_event_t event = {0};
    /* 下降沿只记录区间起点，不产生事件。 */
    CHECK_TRUE(!ewf_power_boot_policy_on_pwr_level(&policy, 0U, 1000U, &event));
    CHECK_TRUE(ewf_power_boot_policy_pwr_is_low(&policy));
    CHECK_EQ_U32(ewf_power_boot_policy_pwr_low_duration_ms(&policy, 1055U), 55U);
    /* 按住期间重复采样不重复产生事件。 */
    CHECK_TRUE(!ewf_power_boot_policy_on_pwr_level(&policy, 0U, 1060U, &event));

    CHECK_TRUE(ewf_power_boot_policy_on_pwr_level(&policy, 1U, 1055U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_PWR_INTERVAL);
    CHECK_EQ_U32(event.duration_ms, 55U);
    CHECK_EQ_U32(policy.pwr_interval_count, 1U);
    CHECK_TRUE(!ewf_power_boot_policy_pwr_is_low(&policy));
    CHECK_EQ_U32(ewf_power_boot_policy_pwr_low_duration_ms(&policy, 2000U), 0U);
}

/* 去抖门限内的抖动不生成按压区间。 */
static void test_pwr_glitch_below_debounce_is_ignored(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(!ewf_power_boot_policy_on_pwr_level(&policy, 0U, 2000U, &event));
    CHECK_TRUE(ewf_power_boot_policy_on_pwr_level(&policy, 1U, 2005U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_PWR_GLITCH_IGNORED);
    CHECK_EQ_U32(policy.pwr_interval_count, 0U);
}

/* 恰好达到去抖门限视为合法区间。 */
static void test_pwr_interval_at_debounce_boundary_is_valid(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(!ewf_power_boot_policy_on_pwr_level(&policy, 0U, 3000U, &event));
    CHECK_TRUE(ewf_power_boot_policy_on_pwr_level(
        &policy, 1U, 3000U + EWF_POWER_BOOT_DEBOUNCE_MS, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_PWR_INTERVAL);
    CHECK_EQ_U32(policy.pwr_interval_count, 1U);
}

/* 启动/下载采样窗口内的 BOOT0 电平变化不产生运行态事件。 */
static void test_boot0_startup_window_never_creates_runtime_tap(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(ewf_power_boot_policy_on_boot_level(&policy, 0U, 100U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED);
    CHECK_TRUE(ewf_power_boot_policy_on_boot_level(&policy, 1U, 5000U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED);
    CHECK_EQ_U32(policy.boot_tap_count, 0U);
}

/* BOOT0 保持低电平进入运行态后释放，仍不产生运行态自动事件。 */
static void test_boot0_held_from_startup_is_suppressed_after_arming(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_policy_arm_runtime(&policy, 0U, 0U);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(policy.boot_held_from_startup);
    CHECK_TRUE(ewf_power_boot_policy_on_boot_level(&policy, 1U, 9000U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED);
    CHECK_EQ_U32(policy.boot_tap_count, 0U);
}

/* 运行态 BOOT0 完整短按产生一个 typed 边界事件。 */
static void test_boot0_runtime_short_press_generates_one_tap(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_policy_arm_runtime(&policy, 1U, 0U);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(!ewf_power_boot_policy_on_boot_level(&policy, 0U, 10000U, &event));
    CHECK_TRUE(ewf_power_boot_policy_on_boot_level(&policy, 1U, 10120U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP);
    CHECK_EQ_U32(event.duration_ms, 120U);
    CHECK_EQ_U32(policy.boot_tap_count, 1U);
}

/* 运行态 BOOT0 长按不产生自动模式事件。 */
static void test_boot0_runtime_long_press_is_suppressed(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_policy_arm_runtime(&policy, 1U, 0U);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(!ewf_power_boot_policy_on_boot_level(&policy, 0U, 20000U, &event));
    CHECK_TRUE(ewf_power_boot_policy_on_boot_level(
        &policy, 1U, 20000U + EWF_POWER_BOOT_RUNTIME_TAP_MAX_MS, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_LONG_SUPPRESSED);
    CHECK_EQ_U32(policy.boot_tap_count, 0U);
}

/* 运行态 BOOT0 抖动不产生事件。 */
static void test_boot0_runtime_glitch_is_ignored(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_policy_arm_runtime(&policy, 1U, 0U);
    ewf_power_boot_event_t event = {0};

    CHECK_TRUE(!ewf_power_boot_policy_on_boot_level(&policy, 0U, 30000U, &event));
    CHECK_TRUE(ewf_power_boot_policy_on_boot_level(&policy, 1U, 30005U, &event));
    CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_GLITCH_IGNORED);
    CHECK_EQ_U32(policy.boot_tap_count, 0U);
}

/* 运行态重复短按各自生成一个事件，且计数单调递增。 */
static void test_boot0_sequence_counts_each_short_press(void)
{
    ewf_power_boot_policy_t policy;
    ewf_power_boot_policy_reset(&policy);
    ewf_power_boot_policy_arm_runtime(&policy, 1U, 0U);
    ewf_power_boot_event_t event = {0};

    for (unsigned index = 0; index < 3U; ++index)
    {
        const uint32_t base = 40000U + index * 1000U;
        (void)ewf_power_boot_policy_on_boot_level(&policy, 0U, base, &event);
        (void)ewf_power_boot_policy_on_boot_level(&policy, 1U, base + 100U, &event);
        CHECK_EQ_U32(event.kind, EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP);
    }
    CHECK_EQ_U32(policy.boot_tap_count, 3U);
}

int main(void)
{
    test_pwr_sustained_low_generates_one_interval();
    test_pwr_glitch_below_debounce_is_ignored();
    test_pwr_interval_at_debounce_boundary_is_valid();
    test_boot0_startup_window_never_creates_runtime_tap();
    test_boot0_held_from_startup_is_suppressed_after_arming();
    test_boot0_runtime_short_press_generates_one_tap();
    test_boot0_runtime_long_press_is_suppressed();
    test_boot0_runtime_glitch_is_ignored();
    test_boot0_sequence_counts_each_short_press();

    if (g_failures != 0U)
    {
        (void)fprintf(stderr, "power_boot_policy: %u 项断言失败\n", g_failures);
        return EXIT_FAILURE;
    }
    (void)printf("power_boot_policy: 9 个主机用例全部通过\n");
    return EXIT_SUCCESS;
}
