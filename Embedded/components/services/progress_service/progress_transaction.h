/**
 * @file     progress_transaction.h
 * @brief    本地高水位/轮次单事务组的纯逻辑策略。
 * @details  定义契约 §11 单事务分组约束的设备侧事务组、积压判定、单调确认收敛与
 *           末字完成置位。本模块不依赖 ESP-IDF，可在主机测试中独立验证；
 *           持久化介质与事件消费由 progress_store/progress_service 承接。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_PROGRESS_TRANSACTION_H
#define EWF_PROGRESS_TRANSACTION_H

#include <stdbool.h>
#include <stdint.h>

#include "tap_input_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 事务组 schema 版本；删除或改义字段必须递增并一次性重写（契约 §11）。 */
#define EWF_PROGRESS_SCHEMA_VERSION 1U
/** 离线积压上限：local_total - acked_total 达到该值即拒绝新输入（契约 §12）。 */
#define EWF_PROGRESS_BACKLOG_LIMIT 1000U
/** scripture_version 字段容量，包含字符串终止符。 */
#define EWF_PROGRESS_SCRIPTURE_VERSION_CAPACITY 16U
/** action_id 字段容量，包含字符串终止符；本 Story 只保留落盘位（契约 §9）。 */
#define EWF_PROGRESS_ACTION_ID_CAPACITY 64U

/** 轮次状态取值域（契约 §2：仅两值，completed 只由完成确认翻转）。 */
typedef enum {
    EWF_PROGRESS_ROUND_STATE_IN_PROGRESS = 0, /**< 轮次进行中。 */
    EWF_PROGRESS_ROUND_STATE_COMPLETED = 1,   /**< 轮次已被完成确认翻转。 */
} ewf_progress_round_state_t;

/** 事务组校验结果；INVALID 版本属配置错误，调用方必须 fail-closed。 */
typedef enum {
    EWF_PROGRESS_VALIDATE_OK = 0,        /**< 事务组完整且与期望版本一致。 */
    EWF_PROGRESS_VALIDATE_NULL,          /**< 参数为 NULL。 */
    EWF_PROGRESS_VALIDATE_SCHEMA,        /**< schema 版本不被当前固件解释。 */
    EWF_PROGRESS_VALIDATE_FIELD,         /**< 字段越界或互相不一致。 */
    EWF_PROGRESS_VALIDATE_VERSION,       /**< scripture_version 与期望不一致。 */
} ewf_progress_validate_result_t;

/** acked_total 单调收敛结果（AD-3：只接受更高确认值）。 */
typedef enum {
    EWF_PROGRESS_ACK_ACCEPTED = 0, /**< 新值更高，已接受并推进。 */
    EWF_PROGRESS_ACK_NOOP,         /**< 新值与当前相同，无需变更。 */
    EWF_PROGRESS_ACK_REJECTED,     /**< 新值低于当前值，拒绝倒退提交。 */
} ewf_progress_ack_result_t;

/**
 * @brief 本地高水位/轮次单事务组
 * @details 字段集合对齐契约 §11 progress 单事务组与 §11.1 device 作用域：
 *          写集不含任何自动模式标志（AC 2，自动模式只在内存）。
 */
typedef struct {
    uint32_t schema_version;    /**< 事务组 schema 版本。 */
    uint32_t local_total;       /**< 本地累计高水位。 */
    uint32_t acked_total;       /**< 已被云端确认的累计高水位。 */
    uint32_t round_id;          /**< 当前轮次 ID，首启为 1。 */
    uint32_t round_cursor;      /**< 当前轮次已消费的可消费汉字游标。 */
    ewf_progress_round_state_t round_state; /**< 轮次状态闭集。 */
    bool pending_completion;    /**< 本地末字完成锁定置位，完成确认由后续 Story 清除。 */
    char scripture_version[EWF_PROGRESS_SCRIPTURE_VERSION_CAPACITY]; /**< canonical 登记版本。 */
    char action_id[EWF_PROGRESS_ACTION_ID_CAPACITY]; /**< 篇章动作族幂等去重键落盘位。 */
} ewf_progress_transaction_t;

/**
 * @brief 按首次启动零值初始化事务组
 * @param tx 事务组输出
 * @param scripture_version canonical 登记版本字符串
 */
void ewf_progress_transaction_init(ewf_progress_transaction_t *tx,
                                   const char *scripture_version);

/**
 * @brief 校验事务组完整性与版本一致性
 * @param tx 待校验事务组
 * @param expected_scripture_version canonical 登记期望版本
 * @param consumable_count canonical 可消费汉字总数
 * @return 校验结果；非 OK 时调用方必须停止推进并进入可诊断状态
 */
ewf_progress_validate_result_t ewf_progress_transaction_validate(
    const ewf_progress_transaction_t *tx,
    const char *expected_scripture_version,
    uint32_t consumable_count);

/**
 * @brief 计算离线积压差值
 * @param tx 事务组
 * @return local_total - acked_total
 */
uint32_t ewf_progress_backlog_count(const ewf_progress_transaction_t *tx);

/**
 * @brief 判断离线积压是否达到拒绝阈值
 * @param tx 事务组
 * @return true 差值达到 1000（达到即拒绝，契约 §12）
 */
bool ewf_progress_backlog_is_full(const ewf_progress_transaction_t *tx);

/**
 * @brief 应用一次有效敲击并推进事务组
 * @param tx 事务组
 * @param consumable_count canonical 可消费汉字总数
 * @return true 本次敲击置位了 pending_completion（本地末字完成）
 */
bool ewf_progress_apply_valid_tap(ewf_progress_transaction_t *tx,
                                  uint32_t consumable_count);

/**
 * @brief 带单调校验地推进 acked_total
 * @param tx 事务组
 * @param new_acked_total 确认结果携带的新确认值
 * @return 接受/同值 no-op/倒退拒绝
 */
ewf_progress_ack_result_t ewf_progress_apply_acked_total(
    ewf_progress_transaction_t *tx,
    uint32_t new_acked_total);

/**
 * @brief 由事务组投影统一敲击 gate 事实
 * @details completed 来自 pending_completion 投影，queue_full 来自积压判定；
 *          fault_locked 不属于本 owner，保持 false（故障反馈属 Story 2.3/2.7）。
 * @param tx 事务组
 * @param gate gate 事实输出
 */
void ewf_progress_fill_gate(const ewf_progress_transaction_t *tx,
                            ewf_tap_gate_state_t *gate);

#ifdef __cplusplus
}
#endif

#endif /* EWF_PROGRESS_TRANSACTION_H */
