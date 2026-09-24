/*
 * @file  test_fault_gate_policy.c
 * @brief fault_locked 生产裁决主机测试（Story 2.7）
 */

#include <stdio.h>
#include <stdlib.h>

#include "fault_gate_policy.h"

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

static void test_feedback_sync_never_locks(void)
{
    CHECK_TRUE(!ewf_fault_gate_feedback_or_sync_failure_locks());
}

static void test_three_persist_failures_lock(void)
{
    ewf_fault_gate_state_t state;
    ewf_fault_gate_reset(&state);

    ewf_fault_gate_decision_t d =
        ewf_fault_gate_on_persist_result(&state, false);
    CHECK_TRUE(!d.locked);
    d = ewf_fault_gate_on_persist_result(&state, false);
    CHECK_TRUE(!d.locked);
    d = ewf_fault_gate_on_persist_result(&state, false);
    CHECK_TRUE(d.locked);
    CHECK_TRUE(d.locked_changed);
    CHECK_EQ_U32(state.consecutive_persist_errors,
                 EWF_FAULT_GATE_PERSIST_FAIL_THRESHOLD);
}

static void test_success_unlocks(void)
{
    ewf_fault_gate_state_t state;
    ewf_fault_gate_reset(&state);
    (void)ewf_fault_gate_on_persist_result(&state, false);
    (void)ewf_fault_gate_on_persist_result(&state, false);
    (void)ewf_fault_gate_on_persist_result(&state, false);
    CHECK_TRUE(state.locked);

    const ewf_fault_gate_decision_t d =
        ewf_fault_gate_on_persist_result(&state, true);
    CHECK_TRUE(!d.locked);
    CHECK_TRUE(d.locked_changed);
    CHECK_TRUE(d.unlock_on_flush);
    CHECK_EQ_U32(state.consecutive_persist_errors, 0U);
}

static void test_display_fault_inject(void)
{
    ewf_fault_gate_state_t state;
    ewf_fault_gate_reset(&state);

    ewf_fault_gate_decision_t d =
        ewf_fault_gate_on_display_fault(&state, true);
    CHECK_TRUE(d.locked);
    CHECK_TRUE(d.locked_changed);

    d = ewf_fault_gate_on_display_fault(&state, false);
    CHECK_TRUE(!d.locked);
    CHECK_TRUE(d.locked_changed);
}

static void test_display_fault_keeps_lock_after_flush(void)
{
    ewf_fault_gate_state_t state;
    ewf_fault_gate_reset(&state);
    (void)ewf_fault_gate_on_display_fault(&state, true);
    const ewf_fault_gate_decision_t d =
        ewf_fault_gate_on_persist_result(&state, true);
    CHECK_TRUE(d.locked);
    CHECK_TRUE(!d.unlock_on_flush);
}

int main(void)
{
    test_feedback_sync_never_locks();
    test_three_persist_failures_lock();
    test_success_unlocks();
    test_display_fault_inject();
    test_display_fault_keeps_lock_after_flush();

    if (g_failures != 0U)
    {
        (void)fprintf(stderr, "fault_gate_policy: %u failures\n", g_failures);
        return 1;
    }
    (void)puts("PASS fault_gate_policy");
    return 0;
}
