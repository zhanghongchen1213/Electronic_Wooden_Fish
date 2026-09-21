/**
 * @file     gps_acquisition_policy.c
 * @brief    GPS 自动搜星、退避与失锁纯策略实现
 * @details  集中维护三分钟搜星硬上限、10/20/40/60 分钟退避及两分钟运动宽限判断。
 * @author   ZHC
 * @date     2026-07-31
 */

#include "gps_acquisition_policy.h"

/** GPS 自动退避等待毫秒表。 */
static const uint32_t s_backoff_delays_ms
    [GPS_ACQUISITION_BACKOFF_STAGE_COUNT] = {
        10U * 60U * 1000U,
        20U * 60U * 1000U,
        40U * 60U * 1000U,
        60U * 60U * 1000U,
    };

uint32_t gps_acquisition_policy_session_budget_ms(bool initial_completed,
                                                  bool boot_trigger)
{
    (void)initial_completed;
    (void)boot_trigger;
    return GPS_ACQUISITION_RETRY_BUDGET_MS;
}

uint32_t gps_acquisition_policy_backoff_delay_ms(uint8_t stage)
{
    const uint8_t bounded_stage =
        stage < GPS_ACQUISITION_BACKOFF_STAGE_COUNT
            ? stage
            : GPS_ACQUISITION_BACKOFF_STAGE_COUNT - 1U;
    return s_backoff_delays_ms[bounded_stage];
}

uint8_t gps_acquisition_policy_next_backoff_stage(uint8_t stage)
{
    return stage + 1U < GPS_ACQUISITION_BACKOFF_STAGE_COUNT
               ? stage + 1U
               : GPS_ACQUISITION_BACKOFF_STAGE_COUNT - 1U;
}

bool gps_acquisition_policy_screen_opportunity_allowed(
    uint8_t stage,
    bool opportunity_used,
    uint64_t now_ms,
    uint64_t cooldown_deadline_ms)
{
    return stage > 0U && !opportunity_used &&
           now_ms >= cooldown_deadline_ms;
}

bool gps_acquisition_policy_motion_active(uint64_t last_motion_ms,
                                          uint64_t now_ms)
{
    return last_motion_ms != 0U && now_ms >= last_motion_ms &&
           now_ms - last_motion_ms < GPS_ACQUISITION_MOTION_GRACE_MS;
}
