/**
 * @file     modem_recovery_policy.h
 * @brief    ML307R 自动恢复指数退避纯策略接口
 * @details  提供无任务、无队列和无 AT 副作用的退避调度、成功重置与单次到期消费。
 * @author   ZHC
 * @date     2026-07-31
 */

#ifndef LEGBOT_MODEM_RECOVERY_POLICY_H
#define LEGBOT_MODEM_RECOVERY_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

/** ML307R 自动恢复指数退避档位总数。 */
#define MODEM_RECOVERY_BACKOFF_STEP_COUNT 7U
/** ML307R 自动恢复首档退避时间。 */
#define MODEM_RECOVERY_BACKOFF_FIRST_MS 30000U
/** ML307R 自动恢复封顶退避时间。 */
#define MODEM_RECOVERY_BACKOFF_MAX_MS 14400000U

typedef struct
{
    bool pending;               /**< 是否存在尚未消费的恢复截止时间。 */
    uint8_t next_delay_index;   /**< 下一次失败应采用的退避档位。 */
    uint32_t scheduled_delay_ms; /**< 当前已调度的退避毫秒数。 */
    TickType_t retry_at_ticks;  /**< 支持 TickType_t 回绕的绝对恢复截止。 */
} modem_recovery_policy_t;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 清空自动恢复退避状态并回到首档
     * @param policy 恢复退避策略状态
     */
    void modem_recovery_policy_reset(modem_recovery_policy_t *policy);

    /**
     * @brief 从本次失败时刻调度当前指数退避档位
     * @param policy 恢复退避策略状态
     * @param now_ticks 本次失败的单调 tick
     */
    void modem_recovery_policy_schedule(modem_recovery_policy_t *policy,
                                        TickType_t now_ticks);

    /**
     * @brief 取消尚未到期的恢复截止但保留下一退避档位
     * @param policy 恢复退避策略状态
     */
    void modem_recovery_policy_cancel(modem_recovery_policy_t *policy);

    /**
     * @brief 取得指定退避档位对应的毫秒数
     * @param index 退避档位，超出范围时返回封顶值
     * @return 退避毫秒数
     */
    uint32_t modem_recovery_policy_delay_ms(uint8_t index);

    /**
     * @brief 判断整机自检是否应暂停普通 4G 生命周期
     * @param selftest_active 整机自检或单项重试是否仍在进行
     * @param modem_selftest_pending modem owner 是否正在执行 4G 自检命令
     * @return true 暂停后台联网与恢复，false 允许普通生命周期或 4G 自检推进
     */
    bool modem_recovery_policy_background_suspended_by_selftest(
        bool selftest_active,
        bool modem_selftest_pending);

    /**
     * @brief 在冷却到期时单次取得自动恢复执行权
     * @param policy 恢复冷却策略状态
     * @param now_ticks 当前单调 tick
     * @return true 仅在首次到期消费时返回
     */
    bool modem_recovery_policy_take_due(modem_recovery_policy_t *policy,
                                        TickType_t now_ticks);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_MODEM_RECOVERY_POLICY_H */
