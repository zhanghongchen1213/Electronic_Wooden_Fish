/**
 * @file     modem_recovery_policy.c
 * @brief    ML307R 自动恢复指数退避纯策略实现
 * @details  使用绝对 tick 和回绕安全比较实现七档退避、成功重置与单次到期消费。
 * @author   ZHC
 * @date     2026-07-31
 */

#include "modem_recovery_policy.h"

#include <stddef.h>
#include <stdint.h>

/** 失败后依次采用的指数退避毫秒表。 */
static const uint32_t s_backoff_delays_ms[MODEM_RECOVERY_BACKOFF_STEP_COUNT] = {
    30000U,
    60000U,
    120000U,
    300000U,
    3600000U,
    7200000U,
    14400000U,
};

static TickType_t backoff_ms_to_ticks(uint32_t delay_ms);

void modem_recovery_policy_reset(modem_recovery_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    *policy = (modem_recovery_policy_t){0};
}

void modem_recovery_policy_schedule(modem_recovery_policy_t *policy,
                                    TickType_t now_ticks)
{
    if (policy == NULL)
    {
        return;
    }
    const uint32_t delay_ms =
        modem_recovery_policy_delay_ms(policy->next_delay_index);
    policy->scheduled_delay_ms = delay_ms;
    policy->retry_at_ticks = now_ticks + backoff_ms_to_ticks(delay_ms);
    policy->pending = true;
    if (policy->next_delay_index + 1U <
        MODEM_RECOVERY_BACKOFF_STEP_COUNT)
    {
        ++policy->next_delay_index;
    }
}

void modem_recovery_policy_cancel(modem_recovery_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    policy->pending = false;
    policy->scheduled_delay_ms = 0U;
    policy->retry_at_ticks = 0U;
}

uint32_t modem_recovery_policy_delay_ms(uint8_t index)
{
    return index < MODEM_RECOVERY_BACKOFF_STEP_COUNT
               ? s_backoff_delays_ms[index]
               : MODEM_RECOVERY_BACKOFF_MAX_MS;
}

bool modem_recovery_policy_background_suspended_by_selftest(
    bool selftest_active,
    bool modem_selftest_pending)
{
    return selftest_active && !modem_selftest_pending;
}

bool modem_recovery_policy_take_due(modem_recovery_policy_t *policy,
                                    TickType_t now_ticks)
{
    if (policy == NULL || !policy->pending ||
        (int32_t)(now_ticks - policy->retry_at_ticks) < 0)
    {
        return false;
    }
    modem_recovery_policy_cancel(policy);
    return true;
}

static TickType_t backoff_ms_to_ticks(uint32_t delay_ms)
{
    const uint64_t ticks =
        (uint64_t)delay_ms * (uint64_t)configTICK_RATE_HZ / 1000ULL;
    return (TickType_t)ticks;
}
