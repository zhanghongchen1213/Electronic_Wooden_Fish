/**
 * @file     sync_response_policy.h
 * @brief    HTTPS 同步响应收敛纯逻辑。
 * @details  成功推进 acked；同值/更低 no-op；20003 冲突；20004 不阻断确认；
 *           20005 停止推进。零 ESP-IDF 依赖。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_SYNC_RESPONSE_POLICY_H
#define EWF_SYNC_RESPONSE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 契约业务码：成功。 */
#define EWF_SYNC_CODE_OK 0
/** 设备重置冲突（可恢复）。 */
#define EWF_SYNC_CODE_DEVICE_RESET_CONFLICT 20003
/** 完成未确认：不阻断确认但仍拒绝新输入。 */
#define EWF_SYNC_CODE_PENDING_COMPLETION 20004
/** 经文版本不一致：停止推进。 */
#define EWF_SYNC_CODE_SCRIPTURE_MISMATCH 20005

typedef enum {
    EWF_SYNC_RESPONSE_ADVANCE_ACKED = 0, /**< 推进 acked_total。 */
    EWF_SYNC_RESPONSE_NOOP,              /**< 同值/更低幂等 no-op。 */
    EWF_SYNC_RESPONSE_CONFLICT,          /**< 可恢复冲突态。 */
    EWF_SYNC_RESPONSE_ACK_WITH_GATE,     /**< 可确认但保持输入门禁。 */
    EWF_SYNC_RESPONSE_STOP_PROGRESS,     /**< 停止推进（版本不一致）。 */
    EWF_SYNC_RESPONSE_TRANSPORT_FAIL,    /**< 传输/解析失败，不得伪装已同步。 */
    EWF_SYNC_RESPONSE_REJECT,            /**< 其它业务拒绝。 */
} ewf_sync_response_action_t;

typedef struct {
    int32_t code;                 /**< 信封业务码。 */
    uint32_t request_acked_total; /**< 请求携带的 acked_total。 */
    uint32_t response_acked_total;/**< 响应 acked_total。 */
    uint32_t command_revision;    /**< 响应命令修订。 */
    uint32_t applied_revision;    /**< 本地已应用修订。 */
    bool transport_ok;            /**< 传输层是否成功拿到信封。 */
} ewf_sync_response_input_t;

typedef struct {
    ewf_sync_response_action_t action; /**< 收敛动作。 */
    uint32_t new_acked_total;          /**< 若 ADVANCE 则目标值。 */
    bool apply_pending_command;        /**< 是否应应用待应用命令。 */
    bool mark_synced;                  /**< 是否可标记已同步（不得伪装）。 */
    bool mark_pending_sync;            /**< 是否进入待同步。 */
    bool mark_sync_failed;             /**< 是否进入同步失败。 */
    bool mark_conflict;                /**< 是否进入可恢复冲突。 */
} ewf_sync_response_decision_t;

/**
 * @brief 收敛一次同步响应
 * @param input 响应事实
 * @return 决策
 */
ewf_sync_response_decision_t ewf_sync_response_policy_apply(
    const ewf_sync_response_input_t *input);

#ifdef __cplusplus
}
#endif

#endif /* EWF_SYNC_RESPONSE_POLICY_H */
