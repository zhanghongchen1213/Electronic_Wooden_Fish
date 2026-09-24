/**
 * @file     sync_window_policy.h
 * @brief    活动窗口开窗/合并/跳过/关窗与退避档纯逻辑。
 * @details  零 ESP-IDF 依赖；触发源为 backlog / 立即同步 / 待应用命令回传。
 *           退避表 5s/15s/30s/60s 单调封顶，连续 AT 超时 ≥3 才进恢复。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_SYNC_WINDOW_POLICY_H
#define EWF_SYNC_WINDOW_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 退避档数量。 */
#define EWF_SYNC_BACKOFF_STAGE_COUNT 4U
/** 连续 AT 超时达到该值才进入恢复退避。 */
#define EWF_SYNC_AT_TIMEOUT_RECOVERY_THRESHOLD 3U

/** 退避毫秒表：5s / 15s / 30s / 60s。 */
extern const uint32_t EWF_SYNC_BACKOFF_MS[EWF_SYNC_BACKOFF_STAGE_COUNT];

typedef enum {
    EWF_SYNC_TRIGGER_NONE = 0,           /**< 无触发。 */
    EWF_SYNC_TRIGGER_BACKLOG,            /**< local_total > acked_total。 */
    EWF_SYNC_TRIGGER_IMMEDIATE_SYNC,     /**< device_nav 立即同步。 */
    EWF_SYNC_TRIGGER_PENDING_COMMAND,    /**< command_revision > applied_revision。 */
} ewf_sync_trigger_t;

typedef enum {
    EWF_SYNC_WINDOW_SKIP = 0,  /**< 跳过本次开窗。 */
    EWF_SYNC_WINDOW_OPEN,      /**< 新开活动窗口。 */
    EWF_SYNC_WINDOW_MERGE,     /**< 合并进当前/下一窗口。 */
    EWF_SYNC_WINDOW_CLOSE,     /**< 关闭活动窗口。 */
} ewf_sync_window_action_t;

typedef struct {
    bool window_open;             /**< 当前是否处于活动窗口。 */
    uint8_t backoff_stage;        /**< 当前退避档 0..3。 */
    uint8_t consecutive_at_timeouts; /**< 连续 AT 超时计数。 */
    uint64_t next_eligible_ms;    /**< 下次允许开窗的单调毫秒。 */
    bool transport_busy;          /**< 传输是否占用中。 */
} ewf_sync_window_state_t;

typedef struct {
    ewf_sync_trigger_t trigger;   /**< 触发源。 */
    uint32_t local_total;         /**< 本地高水位。 */
    uint32_t acked_total;         /**< 已确认高水位。 */
    uint32_t command_revision;    /**< 响应侧最新命令修订。 */
    uint32_t applied_revision;    /**< 本地已应用修订。 */
    bool immediate_pending;       /**< 是否存在未完成的立即同步。 */
    bool transport_ok;            /**< 最近一次传输是否成功。 */
    bool transport_timeout;       /**< 最近一次是否 AT/传输超时。 */
    bool transport_failed;        /**< 最近一次非超时传输失败。 */
    bool force_close;             /**< 强制关窗（窗口结束）。 */
    uint64_t now_ms;              /**< 当前单调毫秒。 */
} ewf_sync_window_input_t;

typedef struct {
    ewf_sync_window_action_t action; /**< 开窗决策。 */
    uint8_t next_backoff_stage;      /**< 下一退避档。 */
    uint32_t backoff_delay_ms;       /**< 下一档延迟毫秒。 */
    bool schedule_retry;             /**< 是否需要补传调度。 */
    bool enter_recovery;             /**< 是否进入恢复路径。 */
    ewf_sync_trigger_t accepted_trigger; /**< 被接受的触发源。 */
} ewf_sync_window_decision_t;

/**
 * @brief 初始化窗口状态
 * @param state 状态输出
 */
void ewf_sync_window_state_init(ewf_sync_window_state_t *state);

/**
 * @brief 应用一次开窗/退避决策
 * @param state 可变状态
 * @param input 输入事实
 * @return 决策
 */
ewf_sync_window_decision_t ewf_sync_window_policy_apply(
    ewf_sync_window_state_t *state,
    const ewf_sync_window_input_t *input);

/**
 * @brief 读取指定退避档的延迟毫秒（越界封顶末档）
 * @param stage 档位
 * @return 毫秒
 */
uint32_t ewf_sync_backoff_delay_ms(uint8_t stage);

#ifdef __cplusplus
}
#endif

#endif /* EWF_SYNC_WINDOW_POLICY_H */
