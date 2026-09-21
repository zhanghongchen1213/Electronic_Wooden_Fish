/**
 * @file     cloud_payment_schedule.c
 * @brief    云支付有限自动复查的纯 deadline 策略实现
 * @details  使用绝对单调毫秒实现十五秒快速槽、三分钟慢速合并窗、七档网络退避与一次人工加速。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "cloud_payment_schedule.h"

#include <stddef.h>

/** 网络暂错后的固定退避毫秒表。 */
static const uint32_t s_payment_backoff_ms[CLOUD_PAYMENT_BACKOFF_STEP_COUNT] = {
    CLOUD_PAYMENT_BACKOFF_STEP_1_MS,
    CLOUD_PAYMENT_BACKOFF_STEP_2_MS,
    CLOUD_PAYMENT_BACKOFF_STEP_3_MS,
    CLOUD_PAYMENT_BACKOFF_STEP_4_MS,
    CLOUD_PAYMENT_BACKOFF_STEP_5_MS,
    CLOUD_PAYMENT_BACKOFF_STEP_6_MS,
    CLOUD_PAYMENT_BACKOFF_STEP_7_MS,
};

static uint64_t saturating_add_ms(uint64_t base, uint64_t delta);
static uint64_t normalized_cloud_window(uint64_t requested_window_ms,
                                        uint64_t minimum_ms);
static void enter_slow_mode(cloud_payment_schedule_t *schedule,
                            uint64_t next_cloud_window_ms,
                            uint64_t minimum_ms);
static void advance_fast_slot_to_now(cloud_payment_schedule_t *schedule,
                                     uint64_t now_ms);

void cloud_payment_schedule_reset(cloud_payment_schedule_t *schedule)
{
    if (schedule == NULL)
    {
        return;
    }
    *schedule = (cloud_payment_schedule_t){0};
    schedule->cadence = CLOUD_PAYMENT_MODE_IDLE;
}

void cloud_payment_schedule_start(cloud_payment_schedule_t *schedule,
                                  uint64_t now_ms)
{
    if (schedule == NULL)
    {
        return;
    }
    cloud_payment_schedule_reset(schedule);
    schedule->active = true;
    schedule->cadence = CLOUD_PAYMENT_MODE_INITIAL;
    schedule->next_deadline_ms = now_ms;
}

bool cloud_payment_schedule_take_due(
    cloud_payment_schedule_t *schedule,
    uint64_t now_ms,
    uint64_t next_cloud_window_ms)
{
    if (schedule == NULL || !schedule->active || schedule->in_flight)
    {
        return false;
    }

    if (schedule->cadence == CLOUD_PAYMENT_MODE_FAST)
    {
        const uint64_t fast_end = saturating_add_ms(
            schedule->first_unpaid_ms,
            CLOUD_PAYMENT_FAST_WINDOW_MS);
        if (now_ms >= fast_end)
        {
            enter_slow_mode(schedule,
                            next_cloud_window_ms,
                            fast_end);
        }
        else
        {
            advance_fast_slot_to_now(schedule, now_ms);
        }
    }

    const bool manual_attempt = schedule->manual_due;
    const uint64_t payment_due = manual_attempt
                                     ? now_ms
                                     : schedule->next_deadline_ms;
    const uint64_t effective_due =
        manual_attempt || payment_due >= schedule->modem_available_ms
            ? payment_due
            : schedule->modem_available_ms;
    if (now_ms < effective_due)
    {
        return false;
    }
    if (schedule->cadence == CLOUD_PAYMENT_MODE_FAST &&
        schedule->http_attempt_count >= CLOUD_PAYMENT_FAST_HTTP_MAX)
    {
        return false;
    }

    schedule->manual_due = false;
    schedule->in_flight = true;
    if (schedule->http_attempt_count < UINT16_MAX)
    {
        ++schedule->http_attempt_count;
    }

    if (schedule->cadence == CLOUD_PAYMENT_MODE_FAST)
    {
        if (schedule->next_fast_slot < CLOUD_PAYMENT_FAST_HTTP_MAX)
        {
            ++schedule->next_fast_slot;
        }
        if (schedule->next_fast_slot < CLOUD_PAYMENT_FAST_HTTP_MAX)
        {
            schedule->next_deadline_ms = saturating_add_ms(
                schedule->first_unpaid_ms,
                (uint64_t)schedule->next_fast_slot *
                    CLOUD_PAYMENT_FAST_INTERVAL_MS);
        }
        else
        {
            schedule->next_deadline_ms = saturating_add_ms(
                schedule->first_unpaid_ms,
                CLOUD_PAYMENT_FAST_WINDOW_MS);
        }
    }
    else if (schedule->cadence == CLOUD_PAYMENT_MODE_SLOW)
    {
        schedule->next_deadline_ms = normalized_cloud_window(
            next_cloud_window_ms,
            saturating_add_ms(now_ms, 1U));
    }
    return true;
}

void cloud_payment_schedule_finish_attempt(
    cloud_payment_schedule_t *schedule,
    cloud_payment_outcome_t outcome,
    uint64_t now_ms,
    uint64_t next_cloud_window_ms)
{
    if (schedule == NULL || !schedule->active || !schedule->in_flight)
    {
        return;
    }
    schedule->in_flight = false;

    if (outcome == CLOUD_PAYMENT_OUTCOME_PAID)
    {
        schedule->active = false;
        schedule->cadence = CLOUD_PAYMENT_MODE_COMPLETE;
        return;
    }
    if (outcome == CLOUD_PAYMENT_OUTCOME_FATAL)
    {
        schedule->active = false;
        schedule->cadence = CLOUD_PAYMENT_MODE_BLOCKED;
        return;
    }
    if (outcome == CLOUD_PAYMENT_OUTCOME_CANCELLED)
    {
        cloud_payment_schedule_reset(schedule);
        return;
    }
    if (outcome == CLOUD_PAYMENT_OUTCOME_TRANSIENT)
    {
        const uint32_t delay_ms = cloud_payment_schedule_backoff_ms(
            schedule->backoff_index);
        schedule->modem_available_ms = saturating_add_ms(now_ms, delay_ms);
        if (schedule->backoff_index + 1U <
            CLOUD_PAYMENT_BACKOFF_STEP_COUNT)
        {
            ++schedule->backoff_index;
        }
        return;
    }

    schedule->backoff_index = 0U;
    schedule->modem_available_ms = 0U;
    if (!schedule->first_unpaid_valid)
    {
        schedule->first_unpaid_valid = true;
        schedule->first_unpaid_ms = now_ms;
        schedule->cadence = CLOUD_PAYMENT_MODE_FAST;
        schedule->next_fast_slot = 1U;
        schedule->next_deadline_ms = saturating_add_ms(
            now_ms,
            CLOUD_PAYMENT_FAST_INTERVAL_MS);
        return;
    }
    if (schedule->cadence == CLOUD_PAYMENT_MODE_SLOW)
    {
        schedule->next_deadline_ms = normalized_cloud_window(
            next_cloud_window_ms,
            saturating_add_ms(now_ms, 1U));
    }
}

bool cloud_payment_schedule_accelerate(
    cloud_payment_schedule_t *schedule,
    uint64_t now_ms)
{
    if (schedule == NULL || !schedule->active || schedule->in_flight ||
        schedule->manual_bypass_used ||
        schedule->cadence == CLOUD_PAYMENT_MODE_COMPLETE ||
        schedule->cadence == CLOUD_PAYMENT_MODE_BLOCKED)
    {
        return false;
    }
    schedule->manual_bypass_used = true;
    schedule->manual_due = true;
    schedule->next_deadline_ms = now_ms;
    return true;
}

cloud_payment_mode_t cloud_payment_schedule_mode(
    const cloud_payment_schedule_t *schedule,
    uint64_t now_ms)
{
    if (schedule == NULL)
    {
        return CLOUD_PAYMENT_MODE_IDLE;
    }
    if (schedule->active && !schedule->in_flight &&
        schedule->modem_available_ms > now_ms &&
        !schedule->manual_due)
    {
        return CLOUD_PAYMENT_MODE_BACKOFF;
    }
    return schedule->cadence;
}

uint32_t cloud_payment_schedule_backoff_ms(uint8_t index)
{
    return index < CLOUD_PAYMENT_BACKOFF_STEP_COUNT
               ? s_payment_backoff_ms[index]
               : CLOUD_PAYMENT_BACKOFF_STEP_7_MS;
}

static uint64_t saturating_add_ms(uint64_t base, uint64_t delta)
{
    return delta > UINT64_MAX - base ? UINT64_MAX : base + delta;
}

static uint64_t normalized_cloud_window(uint64_t requested_window_ms,
                                        uint64_t minimum_ms)
{
    uint64_t window = requested_window_ms;
    if (window == 0U)
    {
        window = minimum_ms;
    }
    if (window >= minimum_ms)
    {
        return window;
    }
    const uint64_t elapsed = minimum_ms - window;
    const uint64_t intervals =
        elapsed / CLOUD_PAYMENT_SLOW_INTERVAL_MS +
        (elapsed % CLOUD_PAYMENT_SLOW_INTERVAL_MS != 0U ? 1U : 0U);
    if (intervals > (UINT64_MAX - window) /
                        CLOUD_PAYMENT_SLOW_INTERVAL_MS)
    {
        return UINT64_MAX;
    }
    return window + intervals * CLOUD_PAYMENT_SLOW_INTERVAL_MS;
}

static void enter_slow_mode(cloud_payment_schedule_t *schedule,
                            uint64_t next_cloud_window_ms,
                            uint64_t minimum_ms)
{
    schedule->cadence = CLOUD_PAYMENT_MODE_SLOW;
    schedule->next_fast_slot = (uint8_t)CLOUD_PAYMENT_FAST_HTTP_MAX;
    schedule->next_deadline_ms = normalized_cloud_window(
        next_cloud_window_ms,
        minimum_ms);
}

static void advance_fast_slot_to_now(cloud_payment_schedule_t *schedule,
                                     uint64_t now_ms)
{
    if (schedule->next_fast_slot >= CLOUD_PAYMENT_FAST_HTTP_MAX ||
        now_ms < schedule->first_unpaid_ms)
    {
        return;
    }
    const uint64_t elapsed = now_ms - schedule->first_unpaid_ms;
    uint64_t latest_slot = elapsed / CLOUD_PAYMENT_FAST_INTERVAL_MS;
    if (latest_slot >= CLOUD_PAYMENT_FAST_HTTP_MAX)
    {
        latest_slot = CLOUD_PAYMENT_FAST_HTTP_MAX - 1U;
    }
    if (latest_slot >= schedule->next_fast_slot)
    {
        schedule->next_fast_slot = (uint8_t)latest_slot;
        schedule->next_deadline_ms = saturating_add_ms(
            schedule->first_unpaid_ms,
            latest_slot * CLOUD_PAYMENT_FAST_INTERVAL_MS);
    }
}
