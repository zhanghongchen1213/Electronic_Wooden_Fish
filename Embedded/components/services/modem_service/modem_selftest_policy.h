/**
 * @file     modem_selftest_policy.h
 * @brief    ML307R 自检 AT 命令失败策略接口
 * @details  区分影响 AT/SIM/信号/PDP 硬性门槛的命令与可容错诊断、注册回退命令。
 * @author   ZHC
 * @date     2026-07-23
 */

#ifndef LEGBOT_MODEM_SELFTEST_POLICY_H
#define LEGBOT_MODEM_SELFTEST_POLICY_H

#include <stdbool.h>

#include "at_core.h"
#include "modem_service.h"

/** modem owner 必须在统一自检总预算前生成终态。 */
#define MODEM_SELFTEST_OWNER_TIMEOUT_MS 58000U
/** selftest_task 领取 modem 终态的截止预算。 */
#define MODEM_SELFTEST_RESULT_DEADLINE_MS 59000U
/** 固定 115200 下基础 AT 的最大尝试次数。 */
#define MODEM_SELFTEST_AT_MAX_ATTEMPTS 3U
/** 每次基础 AT 的绝对截止预算。 */
#define MODEM_SELFTEST_AT_TIMEOUT_MS 3000U
/** 基础 AT 超时后的重试间隔，覆盖 AT core 的旧响应重同步窗口。 */
#define MODEM_SELFTEST_AT_RETRY_INTERVAL_MS 1100U
/** 三次基础 AT 全静默路径的最长预算。 */
#define MODEM_SELFTEST_AT_WORST_CASE_MS                             \
    (MODEM_SELFTEST_AT_MAX_ATTEMPTS * MODEM_SELFTEST_AT_TIMEOUT_MS + \
     (MODEM_SELFTEST_AT_MAX_ATTEMPTS - 1U) *                         \
         MODEM_SELFTEST_AT_RETRY_INTERVAL_MS)
/** 波特率探测单次 AT 超时。 */
#define MODEM_SELFTEST_BAUD_PROBE_TIMEOUT_MS 500U

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        MODEM_SELFTEST_PHASE_IDLE = 0,          /**< 尚未执行自检。 */
        MODEM_SELFTEST_PHASE_CFUN_WAKE,         /**< 从低功耗退避恢复射频。 */
        MODEM_SELFTEST_PHASE_CFUN_VERIFY,       /**< 通过 CFUN? 核对全功能真值。 */
        MODEM_SELFTEST_PHASE_BAUD_PROBE,        /**< 动态波特率探测阶段。 */
        MODEM_SELFTEST_PHASE_BASIC_AT,          /**< 固定 115200 基础 AT 门槛。 */
        MODEM_SELFTEST_PHASE_SIM,               /**< 查询 SIM READY。 */
        MODEM_SELFTEST_PHASE_SIGNAL,            /**< 查询 CSQ 信号事实。 */
        MODEM_SELFTEST_PHASE_REGISTRATION_CEREG, /**< 查询 LTE 注册主事实。 */
        MODEM_SELFTEST_PHASE_REGISTRATION_CREG,  /**< 查询电路域注册回退事实。 */
        MODEM_SELFTEST_PHASE_REGISTRATION_CGREG, /**< 查询分组域注册回退事实。 */
        MODEM_SELFTEST_PHASE_PDP_QUERY,         /**< 查询当前 PDP/IP。 */
        MODEM_SELFTEST_PHASE_PDP_ACTIVATE,      /**< 单次尝试激活 PDP。 */
        MODEM_SELFTEST_PHASE_PDP_WAIT,          /**< 等待 PDP 地址 URC。 */
        MODEM_SELFTEST_PHASE_DONE,              /**< 已取得完整自检终态事实。 */
    } modem_selftest_phase_t;

    /**
     * @brief 获取独立自检阶段对应的固定 AT 命令
     * @param phase 自检阶段
     * @return 固定命令；无需发命令的阶段返回 NULL
     */
    const char *modem_selftest_phase_command(modem_selftest_phase_t phase);

    /**
     * @brief 判断 PDP 就绪 URC 是否允许直接完成当前自检阶段
     * @param phase 当前 owner 自检阶段
     * @return true 仅 PDP_WAIT 可直接完成，其他阶段只能更新事实快照
     */
    bool modem_selftest_phase_accepts_pdp_urc(
        modem_selftest_phase_t phase);

    /**
     * @brief 根据基础 AT 终态与累计 RX 证据生成失败原因
     * @param status AT 核心终态
     * @param rx_bytes 本轮基础 AT 尝试累计收到的字节数
     * @return 稳定 MODEM_ 原因字符串；MODEM_RESTARTED 返回 NULL
     */
    const char *modem_selftest_classify_at_failure(
        at_core_result_status_t status,
        uint32_t rx_bytes);

    /**
     * @brief 按当前硬性事实生成自检终态原因
     * @param snapshot modem owner 当前事实快照
     * @param deadline 是否已到 owner 截止时间
     * @param pdp_attempted 是否已主动尝试 PDP 激活
     * @param outcome 自检通过或失败输出
     * @return 稳定 MODEM_ 原因字符串
     */
    const char *modem_selftest_classify_snapshot(
        const modem_service_snapshot_t *snapshot,
        bool deadline,
        bool pdp_attempted,
        modem_selftest_outcome_t *outcome);

    /**
     * @brief 将自检终态归并回持续运行的 modem 网络快照
     * @param snapshot modem owner 当前事实快照
     * @param outcome 本次自检终态
     * @return true 表示保留已验证 PDP/IP 并进入 HTTPS_READY，false 表示清除网络事实并重新启动
     */
    bool modem_selftest_apply_terminal_state(
        modem_service_snapshot_t *snapshot,
        modem_selftest_outcome_t outcome);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_MODEM_SELFTEST_POLICY_H */
