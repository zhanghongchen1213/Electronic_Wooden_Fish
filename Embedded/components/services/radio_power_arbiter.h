/**
 * @file     radio_power_arbiter.h
 * @brief    GPS 与 LTE 射频活动互斥仲裁接口
 * @details  以单个原子 owner 阻止 L76K 搜星与 ML307R 射频会话并发，避免小电池出现叠加峰值。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef LEGBOT_RADIO_POWER_ARBITER_H
#define LEGBOT_RADIO_POWER_ARBITER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        RADIO_POWER_OWNER_NONE = 0, /**< 当前没有 GPS 或 LTE 射频活动。 */
        RADIO_POWER_OWNER_GPS,      /**< L76K 搜星会话持有射频预算。 */
        RADIO_POWER_OWNER_LTE,      /**< ML307R 从唤醒到确认 CFUN=4 持有射频预算。 */
    } radio_power_owner_t;

    /**
     * @brief 尝试取得射频互斥所有权
     * @param owner GPS 或 LTE owner；同一 owner 重复获取为幂等成功
     * @return ESP_OK 已取得，ESP_ERR_INVALID_STATE 被另一 owner 占用，
     *         ESP_ERR_INVALID_ARG 参数非法
     */
    esp_err_t radio_power_arbiter_try_acquire(radio_power_owner_t owner);

    /**
     * @brief 释放当前 owner 的射频互斥所有权
     * @param owner 必须与当前 owner 一致
     * @return ESP_OK 已释放，ESP_ERR_INVALID_STATE owner 不匹配，
     *         ESP_ERR_INVALID_ARG 参数非法
     */
    esp_err_t radio_power_arbiter_release(radio_power_owner_t owner);

    /**
     * @brief 读取当前射频互斥 owner
     * @return 当前 owner 的原子快照
     */
    radio_power_owner_t radio_power_arbiter_owner(void);

#ifdef __cplusplus
}
#endif

#endif
