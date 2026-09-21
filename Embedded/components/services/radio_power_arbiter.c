/**
 * @file     radio_power_arbiter.c
 * @brief    GPS 与 LTE 射频活动互斥仲裁实现
 * @details  使用 C11 原子比较交换维护唯一 owner，不持有任务锁且不阻塞调用方。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "radio_power_arbiter.h"

#include <stdatomic.h>

/** 全局唯一射频活动 owner。 */
static atomic_int s_radio_power_owner = ATOMIC_VAR_INIT(RADIO_POWER_OWNER_NONE);

esp_err_t radio_power_arbiter_try_acquire(radio_power_owner_t owner)
{
    if (owner != RADIO_POWER_OWNER_GPS && owner != RADIO_POWER_OWNER_LTE)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int current = atomic_load(&s_radio_power_owner);
    for (;;)
    {
        if (current == (int)owner)
        {
            return ESP_OK;
        }
        if (current != (int)RADIO_POWER_OWNER_NONE)
        {
            return ESP_ERR_INVALID_STATE;
        }
        if (atomic_compare_exchange_weak(&s_radio_power_owner,
                                         &current,
                                         (int)owner))
        {
            return ESP_OK;
        }
    }
}

esp_err_t radio_power_arbiter_release(radio_power_owner_t owner)
{
    if (owner != RADIO_POWER_OWNER_GPS && owner != RADIO_POWER_OWNER_LTE)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int expected = (int)owner;
    return atomic_compare_exchange_strong(&s_radio_power_owner,
                                           &expected,
                                           (int)RADIO_POWER_OWNER_NONE)
               ? ESP_OK
               : ESP_ERR_INVALID_STATE;
}

radio_power_owner_t radio_power_arbiter_owner(void)
{
    return (radio_power_owner_t)atomic_load(&s_radio_power_owner);
}
