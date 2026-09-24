/**
 * @file     fault_gate_policy.h
 * @brief    fault_locked 生产裁决纯逻辑（零 ESP-IDF）
 * @details  音频/RGB/同步失败永不 lock（AD-12）。progress 介质连续失败达到阈值
 *           → lock；一次成功落盘 → unlock。预留 display 故障注入入口供 Epic 3。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_FAULT_GATE_POLICY_H
#define EWF_FAULT_GATE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 连续 persist 失败达到该次数后置 fault_locked。 */
#define EWF_FAULT_GATE_PERSIST_FAIL_THRESHOLD 3U

    /** 故障闸门内存态。 */
    typedef struct
    {
        uint32_t consecutive_persist_errors; /**< 连续落盘失败计数。 */
        bool locked;                         /**< 当前是否锁定输入。 */
        bool display_fault;                  /**< 显示链路注入故障（Epic 3 预留）。 */
    } ewf_fault_gate_state_t;

    /** 一次裁决结果。 */
    typedef struct
    {
        bool locked;          /**< 裁决后是否锁定。 */
        bool locked_changed;  /**< 相对输入状态是否翻转。 */
        bool unlock_on_flush; /**< 是否因成功落盘解锁。 */
    } ewf_fault_gate_decision_t;

    /**
     * @brief 复位故障闸门状态
     * @param state 状态实例
     */
    void ewf_fault_gate_reset(ewf_fault_gate_state_t *state);

    /**
     * @brief 处理一次 progress 落盘结果
     * @param state 状态实例
     * @param persist_ok true=落盘成功，false=失败
     * @return 裁决
     */
    ewf_fault_gate_decision_t ewf_fault_gate_on_persist_result(
        ewf_fault_gate_state_t *state,
        bool persist_ok);

    /**
     * @brief 注入/清除显示故障（Epic 3 预留；主机可测）
     * @param state 状态实例
     * @param display_fault true 置故障，false 清除
     * @return 裁决
     */
    ewf_fault_gate_decision_t ewf_fault_gate_on_display_fault(
        ewf_fault_gate_state_t *state,
        bool display_fault);

    /**
     * @brief 音频/RGB/同步通道失败是否应 lock（恒 false，合同钉死）
     * @return 恒为 false
     */
    bool ewf_fault_gate_feedback_or_sync_failure_locks(void);

#ifdef __cplusplus
}
#endif

#endif /* EWF_FAULT_GATE_POLICY_H */
