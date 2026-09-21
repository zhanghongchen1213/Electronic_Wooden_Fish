/**
 * @file     power_interaction_policy.c
 * @brief    日常屏幕与运行态 PWR 纯策略实现
 * @details  在无任务、队列和硬件依赖的 production C 中推进 idle、短按释放与去抖状态。
 * @author   ZHC
 * @date     2026-07-27
 */

#include "power_interaction_policy.h"

#include <limits.h>
#include <stddef.h>

static uint64_t monotonic_elapsed_ms(uint64_t now_ms, uint64_t started_ms);
static uint32_t deadline_remaining_ms(
    uint64_t now_ms,
    const power_event_deadline_t *deadline);

void power_battery_level_policy_init(
    power_battery_level_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    *policy = (power_battery_level_policy_t){
        .level = WATCH_POWER_LEVEL_NORMAL,
    };
}

watch_power_level_t power_battery_level_policy_advance(
    power_battery_level_policy_t *policy,
    bool valid,
    uint8_t percent)
{
    if (policy == NULL)
    {
        return WATCH_POWER_LEVEL_NORMAL;
    }
    policy->last_sample_valid = valid && percent <= 100U;
    if (!policy->last_sample_valid)
    {
        return policy->level;
    }

    if (!policy->has_valid_sample)
    {
        if (percent <= POWER_WATCH_CRITICAL_BATTERY_ENTER_PERCENT)
        {
            policy->level = WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY;
        }
        else if (percent <= POWER_WATCH_LOW_BATTERY_ENTER_PERCENT)
        {
            policy->level = WATCH_POWER_LEVEL_LOW_BATTERY_WARN;
        }
        else
        {
            policy->level = WATCH_POWER_LEVEL_NORMAL;
        }
    }
    else if (policy->level == WATCH_POWER_LEVEL_NORMAL)
    {
        if (percent <= POWER_WATCH_CRITICAL_BATTERY_ENTER_PERCENT)
        {
            policy->level = WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY;
        }
        else if (percent <= POWER_WATCH_LOW_BATTERY_ENTER_PERCENT)
        {
            policy->level = WATCH_POWER_LEVEL_LOW_BATTERY_WARN;
        }
    }
    else if (policy->level == WATCH_POWER_LEVEL_LOW_BATTERY_WARN)
    {
        if (percent <= POWER_WATCH_CRITICAL_BATTERY_ENTER_PERCENT)
        {
            policy->level = WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY;
        }
        else if (percent > POWER_WATCH_LOW_BATTERY_EXIT_PERCENT)
        {
            policy->level = WATCH_POWER_LEVEL_NORMAL;
        }
    }
    else if (percent > POWER_WATCH_LOW_BATTERY_EXIT_PERCENT)
    {
        policy->level = WATCH_POWER_LEVEL_NORMAL;
    }
    else if (percent > POWER_WATCH_CRITICAL_BATTERY_EXIT_PERCENT)
    {
        policy->level = WATCH_POWER_LEVEL_LOW_BATTERY_WARN;
    }

    policy->last_valid_percent = percent;
    policy->has_valid_sample = true;
    return policy->level;
}

uint16_t power_display_brightness_for(
    watch_power_level_t power_level,
    bool selftest_active)
{
    return power_display_brightness_for_user(
        power_level,
        selftest_active,
        POWER_DISPLAY_NORMAL_BRIGHTNESS);
}

uint16_t power_display_brightness_for_user(
    watch_power_level_t power_level,
    bool selftest_active,
    uint16_t user_brightness)
{
    if (selftest_active)
    {
        return POWER_DISPLAY_NORMAL_BRIGHTNESS;
    }
    if (user_brightness > POWER_DISPLAY_NORMAL_BRIGHTNESS)
    {
        user_brightness = POWER_DISPLAY_NORMAL_BRIGHTNESS;
    }
    return power_level == WATCH_POWER_LEVEL_NORMAL ||
                   power_level == WATCH_POWER_LEVEL_LOW_BATTERY_WARN
               ? user_brightness
               : user_brightness < POWER_DISPLAY_CRITICAL_BRIGHTNESS_MAX
                     ? user_brightness
                     : POWER_DISPLAY_CRITICAL_BRIGHTNESS_MAX;
}

void power_interaction_policy_init(power_interaction_policy_t *policy,
                                   uint64_t now_ms,
                                   bool display_on)
{
    if (policy == NULL)
    {
        return;
    }
    *policy = (power_interaction_policy_t){
        .last_activity_ms = now_ms,
        .idle_timeout_ms = POWER_SERVICE_IDLE_TIMEOUT_MS,
        .display_on = display_on,
    };
}

bool power_interaction_policy_idle_timeout_valid(uint32_t timeout_ms)
{
    return timeout_ms == POWER_SERVICE_IDLE_TIMEOUT_5S_MS ||
           timeout_ms == POWER_SERVICE_IDLE_TIMEOUT_15S_MS ||
           timeout_ms == POWER_SERVICE_IDLE_TIMEOUT_30S_MS;
}

bool power_interaction_policy_set_idle_timeout_ms(
    power_interaction_policy_t *policy,
    uint32_t timeout_ms)
{
    if (policy == NULL ||
        !power_interaction_policy_idle_timeout_valid(timeout_ms))
    {
        return false;
    }
    policy->idle_timeout_ms = timeout_ms;
    return true;
}

void power_interaction_policy_set_ble_page_active(
    power_interaction_policy_t *policy,
    bool active,
    uint64_t now_ms)
{
    if (policy == NULL || policy->ble_page_idle_timeout_active == active)
    {
        return;
    }
    policy->ble_page_idle_timeout_active = active;
    /* 进入 BLE 页固定计时 30 秒，离页后重新给用户一个完整设置档位窗口。 */
    policy->last_activity_ms = now_ms;
}

void power_interaction_policy_reconcile_display(
    power_interaction_policy_t *policy,
    bool display_on)
{
    if (policy != NULL)
    {
        policy->display_on = display_on;
    }
}

void power_interaction_policy_note_display_failure(
    power_interaction_policy_t *policy)
{
    if (policy != NULL && policy->selftest_active && !policy->display_on)
    {
        policy->selftest_wake_suppressed = true;
    }
}

power_display_intent_t power_interaction_policy_advance(
    power_interaction_policy_t *policy,
    const power_interaction_input_t *input)
{
    if (policy == NULL || input == NULL || policy->stopped)
    {
        return POWER_DISPLAY_INTENT_NONE;
    }
    if (input->stop_requested)
    {
        policy->stopped = true;
        return POWER_DISPLAY_INTENT_STOP;
    }

    if (input->selftest_fact_valid &&
        input->selftest_active &&
        !policy->selftest_active)
    {
        policy->selftest_active = true;
        policy->selftest_entry_display_on = policy->display_on;
        policy->selftest_user_activity = false;
        policy->selftest_wake_suppressed = false;
        if (policy->pwr_valid && policy->pwr_pressed)
        {
            policy->pwr_release_suppressed = true;
        }
    }
    if (policy->selftest_active &&
        (input->local_activity ||
         (input->pwr_valid && input->pwr_pressed)))
    {
        policy->selftest_user_activity = true;
        policy->last_activity_ms = input->now_ms;
        if (input->pwr_valid && input->pwr_pressed)
        {
            policy->pwr_release_suppressed = true;
        }
    }
    if (input->selftest_fact_valid &&
        !input->selftest_active &&
        policy->selftest_active)
    {
        const bool restore_display =
            policy->selftest_user_activity
                ? true
                : policy->selftest_entry_display_on;
        policy->selftest_active = false;
        policy->selftest_user_activity = false;
        policy->selftest_wake_suppressed = false;
        if (restore_display)
        {
            /* 退出时建立新的完整 idle 窗口，避免长自检复用过期 deadline。 */
            policy->last_activity_ms = input->now_ms;
        }
        if (policy->display_on != restore_display)
        {
            policy->display_on = restore_display;
            return restore_display ? POWER_DISPLAY_INTENT_WAKE
                                   : POWER_DISPLAY_INTENT_SLEEP;
        }
    }

    if (policy->selftest_active && !policy->display_on &&
        !policy->selftest_wake_suppressed)
    {
        policy->display_on = true;
        return POWER_DISPLAY_INTENT_WAKE;
    }

    power_display_intent_t pwr_intent = POWER_DISPLAY_INTENT_NONE;
    if (!input->pwr_valid)
    {
        policy->pwr_valid = false;
        policy->pwr_pressed = false;
        policy->pwr_press_started_display_on = false;
        policy->pwr_release_suppressed = false;
    }
    else if (!policy->pwr_valid)
    {
        policy->pwr_valid = true;
        policy->pwr_pressed = input->pwr_pressed;
        if (input->pwr_pressed)
        {
            policy->last_activity_ms = input->now_ms;
            policy->pwr_press_started_display_on = policy->display_on;
            policy->pwr_release_suppressed = policy->selftest_active;
        }
    }
    else if (input->pwr_pressed != policy->pwr_pressed)
    {
        policy->pwr_pressed = input->pwr_pressed;
        policy->last_activity_ms = input->now_ms;
        if (input->pwr_pressed)
        {
            policy->pwr_press_started_display_on = policy->display_on;
            policy->pwr_release_suppressed = policy->selftest_active;
        }
        else
        {
            const bool press_started_display_on =
                policy->pwr_press_started_display_on;
            const bool release_suppressed =
                policy->pwr_release_suppressed;
            policy->pwr_press_started_display_on = false;
            policy->pwr_release_suppressed = false;
            policy->selftest_wake_suppressed = false;

            if (!release_suppressed && !press_started_display_on)
            {
                if (!policy->display_on)
                {
                    policy->display_on = true;
                    pwr_intent = POWER_DISPLAY_INTENT_WAKE;
                }
            }
            else if (!release_suppressed && !policy->display_on)
            {
                policy->display_on = true;
                pwr_intent = POWER_DISPLAY_INTENT_WAKE;
            }
            else if (!release_suppressed && !policy->selftest_active)
            {
                pwr_intent = POWER_DISPLAY_INTENT_NEXT_PAGE;
            }
        }
    }

    if (input->local_activity)
    {
        policy->selftest_wake_suppressed = false;
        policy->last_activity_ms = input->now_ms;
        if (!policy->display_on)
        {
            policy->display_on = true;
            return POWER_DISPLAY_INTENT_WAKE;
        }
    }
    if (pwr_intent != POWER_DISPLAY_INTENT_NONE)
    {
        return pwr_intent;
    }

    if (policy->pwr_valid && policy->pwr_pressed)
    {
        /* 持续按住只保持活动，不生成显示切换或任何软件关机动作。 */
        policy->last_activity_ms = input->now_ms;
    }

    if (input->wrist_raise_activity &&
        !policy->display_on &&
        !policy->selftest_active &&
        !(policy->pwr_valid && policy->pwr_pressed))
    {
        policy->last_activity_ms = input->now_ms;
        policy->display_on = true;
        return POWER_DISPLAY_INTENT_WAKE;
    }

    const uint32_t idle_timeout_ms =
        policy->ble_page_idle_timeout_active
            ? POWER_SERVICE_BLE_PAGE_IDLE_TIMEOUT_MS
            : policy->idle_timeout_ms;
    if (policy->display_on && !policy->selftest_active &&
        monotonic_elapsed_ms(input->now_ms, policy->last_activity_ms) >=
            idle_timeout_ms)
    {
        policy->display_on = false;
        return POWER_DISPLAY_INTENT_SLEEP;
    }
    return POWER_DISPLAY_INTENT_NONE;
}

void power_pwr_debounce_init(power_pwr_debounce_t *debounce,
                             bool initial_pressed,
                             uint32_t debounce_ms)
{
    if (debounce == NULL)
    {
        return;
    }
    *debounce = (power_pwr_debounce_t){
        .debounce_ms = debounce_ms,
        .stable_pressed = initial_pressed,
    };
}

bool power_pwr_debounce_advance(power_pwr_debounce_t *debounce,
                                bool raw_pressed,
                                uint64_t now_ms,
                                bool *changed)
{
    if (debounce == NULL || changed == NULL)
    {
        return false;
    }
    *changed = false;
    if (raw_pressed == debounce->stable_pressed)
    {
        debounce->candidate_active = false;
        return debounce->stable_pressed;
    }
    if (!debounce->candidate_active ||
        debounce->candidate_pressed != raw_pressed)
    {
        debounce->candidate_pressed = raw_pressed;
        debounce->candidate_since_ms = now_ms;
        debounce->candidate_active = true;
        return debounce->stable_pressed;
    }
    if (monotonic_elapsed_ms(now_ms, debounce->candidate_since_ms) <
        debounce->debounce_ms)
    {
        return debounce->stable_pressed;
    }

    debounce->stable_pressed = debounce->candidate_pressed;
    debounce->candidate_active = false;
    *changed = true;
    return debounce->stable_pressed;
}

void power_display_intent_mailbox_offer(
    power_display_intent_mailbox_t *mailbox,
    power_display_intent_t intent)
{
    if (mailbox != NULL &&
        (intent == POWER_DISPLAY_INTENT_WAKE ||
         intent == POWER_DISPLAY_INTENT_SLEEP ||
         intent == POWER_DISPLAY_INTENT_NEXT_PAGE))
    {
        if (intent == POWER_DISPLAY_INTENT_NEXT_PAGE &&
            mailbox->pending == POWER_DISPLAY_INTENT_NEXT_PAGE)
        {
            if (mailbox->pending_count < UINT32_MAX)
            {
                ++mailbox->pending_count;
            }
        }
        else
        {
            mailbox->pending = intent;
            mailbox->pending_count = 1U;
        }
    }
}

power_display_intent_t power_display_intent_mailbox_peek(
    const power_display_intent_mailbox_t *mailbox)
{
    return mailbox != NULL && mailbox->pending_count > 0U
               ? mailbox->pending
               : POWER_DISPLAY_INTENT_NONE;
}

void power_display_intent_mailbox_complete(
    power_display_intent_mailbox_t *mailbox)
{
    if (mailbox != NULL)
    {
        if (mailbox->pending == POWER_DISPLAY_INTENT_NEXT_PAGE &&
            mailbox->pending_count > 1U)
        {
            --mailbox->pending_count;
        }
        else
        {
            mailbox->pending = POWER_DISPLAY_INTENT_NONE;
            mailbox->pending_count = 0U;
        }
    }
}

uint32_t power_event_deadlines_wait_ms(
    uint64_t now_ms,
    const power_event_deadlines_t *deadlines)
{
    if (deadlines == NULL || deadlines->immediate_work)
    {
        return 0U;
    }
    const power_event_deadline_t *const candidates[] = {
        &deadlines->idle,
        &deadlines->pwr_debounce,
        &deadlines->wrist_raise,
        &deadlines->battery,
        &deadlines->pending_delivery,
    };
    uint32_t wait_ms = POWER_EVENT_WAIT_FOREVER_MS;
    for (size_t index = 0U;
         index < sizeof(candidates) / sizeof(candidates[0]);
         ++index)
    {
        if (!candidates[index]->active)
        {
            continue;
        }
        const uint32_t remaining =
            deadline_remaining_ms(now_ms, candidates[index]);
        if (remaining < wait_ms)
        {
            wait_ms = remaining;
        }
    }
    return wait_ms;
}

static uint64_t monotonic_elapsed_ms(uint64_t now_ms, uint64_t started_ms)
{
    const uint64_t elapsed = now_ms - started_ms;
    /*
     * 无符号减法自然覆盖真实回绕；大于 INT64_MAX 的差值视为时钟倒退，
     * 防止校时或错误 fixture 让 idle 或去抖立即越过截止线。
     */
    return elapsed <= (uint64_t)INT64_MAX ? elapsed : 0U;
}

static uint32_t deadline_remaining_ms(
    uint64_t now_ms,
    const power_event_deadline_t *deadline)
{
    if (deadline->deadline_ms <= now_ms)
    {
        return 0U;
    }
    const uint64_t remaining = deadline->deadline_ms - now_ms;
    return remaining >= POWER_EVENT_WAIT_FOREVER_MS
               ? POWER_EVENT_WAIT_FOREVER_MS - 1U
               : (uint32_t)remaining;
}
