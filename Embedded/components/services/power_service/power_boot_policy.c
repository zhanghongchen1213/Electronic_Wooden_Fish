/**
 * @file     power_boot_policy.c
 * @brief    PWR_INT/BOOT0 边沿与低电平时长判定实现
 * @details  纯逻辑实现：只处理原始电平、去抖门限和区间分类，不访问 GPIO、不打印、不阻塞。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "power_boot_policy.h"

#include <stddef.h>

static bool level_is_active(uint8_t level)
{
    return level == EWF_POWER_BOOT_ACTIVE_LEVEL;
}

static void publish_event(ewf_power_boot_event_t *event,
                          ewf_power_boot_event_kind_t kind,
                          uint32_t duration_ms,
                          uint32_t now_ms)
{
    if (event == NULL)
    {
        return;
    }
    event->kind = kind;
    event->duration_ms = duration_ms;
    event->at_ms = now_ms;
}

void ewf_power_boot_policy_reset(ewf_power_boot_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    policy->pwr_interval_open = false;
    policy->pwr_started_at_ms = 0U;
    policy->boot_interval_open = false;
    policy->boot_started_at_ms = 0U;
    policy->boot_runtime_armed = false;
    policy->boot_held_from_startup = false;
    policy->pwr_interval_count = 0U;
    policy->boot_tap_count = 0U;
}

void ewf_power_boot_policy_arm_runtime(ewf_power_boot_policy_t *policy,
                                       uint8_t boot_level,
                                       uint32_t now_ms)
{
    if (policy == NULL)
    {
        return;
    }
    policy->boot_runtime_armed = true;
    /* 进入运行态时 BOOT0 仍为低，说明本次是下载或启动绑带窗口，不得产生运行态事件。 */
    policy->boot_held_from_startup = level_is_active(boot_level);
    /* 该区间起点早于运行态，仍需在释放边沿被显式抑制并结清。 */
    policy->boot_interval_open = policy->boot_held_from_startup;
    policy->boot_started_at_ms = now_ms;
}

bool ewf_power_boot_policy_on_pwr_level(ewf_power_boot_policy_t *policy,
                                        uint8_t level,
                                        uint32_t now_ms,
                                        ewf_power_boot_event_t *event)
{
    if (policy == NULL)
    {
        return false;
    }

    if (level_is_active(level))
    {
        if (policy->pwr_interval_open)
        {
            return false;
        }
        /* 按住期间持续为低，只在下降沿记录起点，不在电平期间重复产生事件。 */
        policy->pwr_interval_open = true;
        policy->pwr_started_at_ms = now_ms;
        return false;
    }

    if (!policy->pwr_interval_open)
    {
        return false;
    }
    policy->pwr_interval_open = false;
    const uint32_t duration_ms = now_ms - policy->pwr_started_at_ms;
    if (duration_ms < EWF_POWER_BOOT_DEBOUNCE_MS)
    {
        publish_event(event, EWF_POWER_BOOT_EVENT_PWR_GLITCH_IGNORED, duration_ms, now_ms);
        return true;
    }
    ++policy->pwr_interval_count;
    publish_event(event, EWF_POWER_BOOT_EVENT_PWR_INTERVAL, duration_ms, now_ms);
    return true;
}

bool ewf_power_boot_policy_on_boot_level(ewf_power_boot_policy_t *policy,
                                         uint8_t level,
                                         uint32_t now_ms,
                                         ewf_power_boot_event_t *event)
{
    if (policy == NULL)
    {
        return false;
    }

    if (!policy->boot_runtime_armed)
    {
        /* 启动/下载采样窗口内的任何电平变化都不属于运行态产品事件。 */
        if (level_is_active(level))
        {
            policy->boot_interval_open = true;
            policy->boot_started_at_ms = now_ms;
        }
        else
        {
            policy->boot_interval_open = false;
        }
        publish_event(event,
                      EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED,
                      0U,
                      now_ms);
        return true;
    }

    if (level_is_active(level))
    {
        if (policy->boot_interval_open)
        {
            return false;
        }
        policy->boot_interval_open = true;
        policy->boot_started_at_ms = now_ms;
        return false;
    }

    if (!policy->boot_interval_open)
    {
        return false;
    }
    policy->boot_interval_open = false;
    const uint32_t duration_ms = now_ms - policy->boot_started_at_ms;

    if (policy->boot_held_from_startup)
    {
        /* 该区间起点早于运行态，属于启动/下载保持，不是运行态短按。 */
        policy->boot_held_from_startup = false;
        publish_event(event,
                      EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED,
                      duration_ms,
                      now_ms);
        return true;
    }
    if (duration_ms < EWF_POWER_BOOT_DEBOUNCE_MS)
    {
        publish_event(event, EWF_POWER_BOOT_EVENT_BOOT0_GLITCH_IGNORED, duration_ms, now_ms);
        return true;
    }
    if (duration_ms >= EWF_POWER_BOOT_RUNTIME_TAP_MAX_MS)
    {
        /* 长按、抖动与启动采样都不产生运行态自动模式事件。 */
        publish_event(event,
                      EWF_POWER_BOOT_EVENT_BOOT0_LONG_SUPPRESSED,
                      duration_ms,
                      now_ms);
        return true;
    }
    ++policy->boot_tap_count;
    publish_event(event, EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP, duration_ms, now_ms);
    return true;
}

uint32_t ewf_power_boot_policy_pwr_low_duration_ms(
    const ewf_power_boot_policy_t *policy,
    uint32_t now_ms)
{
    if (policy == NULL || !policy->pwr_interval_open)
    {
        return 0U;
    }
    return now_ms - policy->pwr_started_at_ms;
}

bool ewf_power_boot_policy_pwr_is_low(const ewf_power_boot_policy_t *policy)
{
    return policy != NULL && policy->pwr_interval_open;
}
