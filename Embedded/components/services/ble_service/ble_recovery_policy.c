/**
 * @file     ble_recovery_policy.c
 * @brief    BLE 自动恢复、授权触发、扫描会话与自检暂停纯策略实现
 * @details  实现固定退避单次 deadline、链路授权触发、扫描模式、自检 discovery 暂停转换、饱和计数和不触碰 NimBLE 的回收守卫。
 * @author   ZHC
 * @date     2026-07-24
 */

#include "ble_recovery_policy.h"

#include <limits.h>
#include <stddef.h>

/** 亮屏固定退避序列，超过末档后保持最后一项。 */
static const uint32_t
    s_screen_on_backoff_ms[BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT] = {
    1000U,
    2000U,
    5000U,
    10000U,
    30000U,
};
_Static_assert(BLE_SCAN_SCREEN_ON_WINDOW_UNITS <=
                   BLE_SCAN_SCREEN_ON_INTERVAL_UNITS,
               "亮屏 BLE 扫描窗口不得超过扫描间隔");
_Static_assert(BLE_CONNECTION_ACTIVE_INTERVAL_MIN_UNITS >= 6U &&
                   BLE_CONNECTION_ACTIVE_INTERVAL_MIN_UNITS <=
                       BLE_CONNECTION_ACTIVE_INTERVAL_MAX_UNITS,
               "活跃 BLE 连接间隔必须满足 Core 规范范围");
_Static_assert(BLE_CONNECTION_STABLE_INTERVAL_MIN_UNITS >= 6U &&
                   BLE_CONNECTION_STABLE_INTERVAL_MIN_UNITS <=
                       BLE_CONNECTION_STABLE_INTERVAL_MAX_UNITS,
               "稳定 BLE 连接间隔必须满足 Core 规范范围");
_Static_assert(BLE_CONNECTION_ACTIVE_TIMEOUT_UNITS >= 10U &&
                   BLE_CONNECTION_ACTIVE_TIMEOUT_UNITS <= 3200U,
               "活跃 BLE 监督超时必须满足 Core 规范范围");
_Static_assert(BLE_CONNECTION_STABLE_TIMEOUT_UNITS >= 10U &&
                   BLE_CONNECTION_STABLE_TIMEOUT_UNITS <= 3200U,
               "稳定 BLE 监督超时必须满足 Core 规范范围");
_Static_assert(BLE_CONNECTION_ACTIVE_TIMEOUT_UNITS >
                   ((1U + BLE_CONNECTION_ACTIVE_LATENCY) *
                    BLE_CONNECTION_ACTIVE_INTERVAL_MAX_UNITS) /
                       4U,
               "活跃 BLE 监督超时必须覆盖连接间隔与延迟");
_Static_assert(BLE_CONNECTION_STABLE_TIMEOUT_UNITS >
                   ((1U + BLE_CONNECTION_STABLE_LATENCY) *
                    BLE_CONNECTION_STABLE_INTERVAL_MAX_UNITS) /
                       4U,
               "稳定 BLE 监督超时必须覆盖连接间隔与延迟");

uint32_t ble_recovery_backoff_ms(uint32_t attempt)
{
    return ble_recovery_backoff_ms_for_screen(attempt, true);
}

uint32_t ble_recovery_backoff_ms_for_screen(uint32_t attempt,
                                            bool screen_on)
{
    if (!screen_on)
    {
        return 0U;
    }
    const uint32_t index =
        attempt < BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT
            ? attempt
            : BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT - 1U;
    return s_screen_on_backoff_ms[index];
}

void ble_recovery_policy_reset(ble_recovery_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    *policy = (ble_recovery_policy_t){0};
}

void ble_recovery_policy_schedule(ble_recovery_policy_t *policy,
                                  uint32_t now_ms,
                                  uint32_t generation)
{
    ble_recovery_policy_schedule_for_screen(policy,
                                            now_ms,
                                            generation,
                                            true);
}

void ble_recovery_policy_schedule_for_screen(
    ble_recovery_policy_t *policy,
    uint32_t now_ms,
    uint32_t generation,
    bool screen_on)
{
    if (policy == NULL || generation == 0U)
    {
        return;
    }
    if (!screen_on)
    {
        ble_recovery_policy_cancel(policy);
        return;
    }
    const uint8_t backoff_count = BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT;
    const uint8_t scheduled_index =
        policy->next_backoff_index < backoff_count
            ? policy->next_backoff_index
            : (uint8_t)(backoff_count - 1U);
    const uint32_t delay_ms =
        ble_recovery_backoff_ms_for_screen(scheduled_index, screen_on);
    policy->scheduled_backoff_index = scheduled_index;
    policy->screen_on = screen_on;
    policy->deadline_ms = now_ms + delay_ms;
    policy->generation = generation;
    policy->pending = true;
    if (policy->next_backoff_index + 1U < backoff_count)
    {
        ++policy->next_backoff_index;
    }
}

bool ble_recovery_policy_retarget_screen(
    ble_recovery_policy_t *policy,
    uint32_t now_ms,
    bool screen_on)
{
    if (policy == NULL || !policy->pending)
    {
        return false;
    }
    if (!screen_on)
    {
        ble_recovery_policy_cancel(policy);
        return true;
    }
    if (policy->screen_on)
    {
        return false;
    }
    const uint8_t backoff_count = BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT;
    const uint8_t scheduled_index =
        policy->scheduled_backoff_index < backoff_count
            ? policy->scheduled_backoff_index
            : (uint8_t)(backoff_count - 1U);
    policy->scheduled_backoff_index = scheduled_index;
    policy->screen_on = screen_on;
    policy->deadline_ms =
        now_ms + ble_recovery_backoff_ms_for_screen(scheduled_index,
                                                     screen_on);
    return true;
}

void ble_recovery_policy_cancel(ble_recovery_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    policy->pending = false;
    policy->scheduled_backoff_index = 0U;
    policy->screen_on = false;
    policy->deadline_ms = 0U;
    policy->generation = 0U;
}

bool ble_recovery_policy_take_due(ble_recovery_policy_t *policy,
                                  uint32_t now_ms,
                                  uint32_t generation)
{
    if (policy == NULL || !policy->pending || generation == 0U ||
        generation != policy->generation ||
        (int32_t)(now_ms - policy->deadline_ms) < 0)
    {
        return false;
    }
    ble_recovery_policy_cancel(policy);
    return true;
}

bool ble_authorization_should_schedule(
    bool link_control_ready,
    bool has_binding,
    bool binding_matches,
    uint32_t link_generation,
    uint32_t attempt_generation)
{
    return link_control_ready && has_binding && binding_matches &&
           link_generation != 0U &&
           attempt_generation != link_generation;
}

uint32_t ble_recovery_saturating_increment(uint32_t value)
{
    return value == UINT32_MAX ? UINT32_MAX : value + 1U;
}

ble_discovery_pause_transition_t ble_discovery_pause_transition(
    bool suspended,
    bool selftest_running)
{
    if (suspended == selftest_running)
    {
        return BLE_DISCOVERY_PAUSE_NO_CHANGE;
    }
    return selftest_running ? BLE_DISCOVERY_PAUSE_ENTER
                            : BLE_DISCOVERY_PAUSE_EXIT;
}

ble_discovery_mode_t ble_discovery_resolve_mode(
    bool has_binding,
    bool binding_session_active)
{
    if (has_binding)
    {
        return BLE_DISCOVERY_MODE_BOUND_RECONNECT;
    }
    return binding_session_active
               ? BLE_DISCOVERY_MODE_BINDING_SESSION
               : BLE_DISCOVERY_MODE_IDLE;
}

bool ble_discovery_mode_publishes_candidates(
    ble_discovery_mode_t mode)
{
    return mode == BLE_DISCOVERY_MODE_BINDING_SESSION;
}

ble_scan_power_policy_t ble_scan_power_policy_resolve(
    ble_discovery_mode_t mode,
    bool screen_on)
{
    if (mode == BLE_DISCOVERY_MODE_IDLE || !screen_on)
    {
        return (ble_scan_power_policy_t){0};
    }
    return (ble_scan_power_policy_t){
        .allowed = true,
        .interval_units = BLE_SCAN_SCREEN_ON_INTERVAL_UNITS,
        .window_units = BLE_SCAN_SCREEN_ON_WINDOW_UNITS,
        .duration_ms = BLE_SCAN_WINDOW_DURATION_MS,
    };
}

ble_connection_power_profile_t ble_connection_power_profile_resolve(
    bool screen_on,
    bool control_active)
{
    return screen_on || control_active
               ? BLE_CONNECTION_POWER_PROFILE_ACTIVE
               : BLE_CONNECTION_POWER_PROFILE_STABLE;
}

ble_connection_parameter_policy_t ble_connection_parameter_policy(
    ble_connection_power_profile_t profile)
{
    if (profile == BLE_CONNECTION_POWER_PROFILE_ACTIVE)
    {
        return (ble_connection_parameter_policy_t){
            .interval_min_units =
                BLE_CONNECTION_ACTIVE_INTERVAL_MIN_UNITS,
            .interval_max_units =
                BLE_CONNECTION_ACTIVE_INTERVAL_MAX_UNITS,
            .latency = BLE_CONNECTION_ACTIVE_LATENCY,
            .timeout_units = BLE_CONNECTION_ACTIVE_TIMEOUT_UNITS,
        };
    }
    if (profile == BLE_CONNECTION_POWER_PROFILE_STABLE)
    {
        return (ble_connection_parameter_policy_t){
            .interval_min_units =
                BLE_CONNECTION_STABLE_INTERVAL_MIN_UNITS,
            .interval_max_units =
                BLE_CONNECTION_STABLE_INTERVAL_MAX_UNITS,
            .latency = BLE_CONNECTION_STABLE_LATENCY,
            .timeout_units = BLE_CONNECTION_STABLE_TIMEOUT_UNITS,
        };
    }
    return (ble_connection_parameter_policy_t){0};
}

void ble_nimble_cleanup_guard_reset(ble_nimble_cleanup_guard_t *guard)
{
    if (guard == NULL)
    {
        return;
    }
    *guard = (ble_nimble_cleanup_guard_t){0};
}

bool ble_nimble_cleanup_begin_stop(ble_nimble_cleanup_guard_t *guard)
{
    if (guard == NULL || guard->terminal_error != 0 || guard->stop_issued)
    {
        return false;
    }
    guard->stop_issued = true;
    return true;
}

bool ble_nimble_cleanup_begin_deinit(ble_nimble_cleanup_guard_t *guard)
{
    if (guard == NULL || guard->terminal_error != 0 || !guard->stop_issued ||
        guard->deinit_attempted)
    {
        return false;
    }
    guard->deinit_attempted = true;
    return true;
}

void ble_nimble_cleanup_record_error(ble_nimble_cleanup_guard_t *guard,
                                     int32_t error)
{
    if (guard != NULL && error != 0 && guard->terminal_error == 0)
    {
        guard->terminal_error = error;
    }
}
