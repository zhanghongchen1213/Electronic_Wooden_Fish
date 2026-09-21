/**
 * @file     control_gate.h
 * @brief    首次解锁与普通控制的唯一 typed 准入接口
 * @details  定义无长期任务的单 in-flight、身份锁定、no-op 与初始写入证据契约。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef LEGBOT_CONTROL_GATE_H
#define LEGBOT_CONTROL_GATE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** BLE 解锁写入成功后等待下一帧有效状态的固定窗口。 */
#define CONTROL_GATE_UNLOCK_CONFIRM_TIMEOUT_MS 5000U
/** 普通控制写 callback 成功后等待产品状态回显的固定窗口。 */
#define CONTROL_ACK_TIMEOUT_MS 3000U
/** 单个逻辑普通控制允许的一次初始写入加两次重试。 */
#define CONTROL_MAX_ATTEMPTS WATCH_CONTROL_MAX_ATTEMPTS
/** 一次性关机写 callback 成功后等待产品终态的固定窗口。 */
#define POWEROFF_RESULT_TIMEOUT_MS 15000U
/** control_gate 使用的规范外骨骼 MAC 容量。 */
#define CONTROL_GATE_MAC_CAPACITY WATCH_BLE_MAC_CAPACITY

    typedef enum
    {
        CONTROL_GATE_ACCEPTED = 0,     /**< 首次授权事务已接受。 */
        CONTROL_GATE_ALREADY_UNLOCKED, /**< 已有会话证明，本次为 no-op。 */
        CONTROL_GATE_BUSY,             /**< 已有查询、写入或确认事务。 */
        CONTROL_GATE_BLOCKED,          /**< 当前快照不满足 fail-closed 准入。 */
    } control_gate_request_status_t;

    typedef enum
    {
        CONTROL_GATE_CONTROL_ACCEPTED = 0,  /**< 普通控制已锁定身份并允许 owner 写入。 */
        CONTROL_GATE_CONTROL_NO_OP,         /**< 目标与当前协议状态等价，无需写入。 */
        CONTROL_GATE_CONTROL_BUSY,          /**< 首次解锁或普通控制已占用唯一槽。 */
        CONTROL_GATE_CONTROL_AUTH_REQUIRED, /**< 当前没有 RAM-only 解锁会话证明。 */
        CONTROL_GATE_CONTROL_BLOCKED,       /**< 产品或链路事实不满足准入。 */
        CONTROL_GATE_CONTROL_INVALID,       /**< typed 类型或目标值越界。 */
        CONTROL_GATE_CONTROL_LOCKED_OUT,    /**< 普通控制三次失败，等待真实 BLE 重连。 */
    } control_gate_control_request_status_t;

    typedef enum
    {
        CONTROL_GATE_CONTROL_ACTION_NONE = 0,  /**< 不产生 BLE owner 动作。 */
        CONTROL_GATE_CONTROL_ACTION_BLE_WRITE, /**< 产生一次完整普通控制包写入动作。 */
    } control_gate_control_action_type_t;

    typedef struct
    {
        watch_control_kind_t kind; /**< 普通控制类型。 */
        uint8_t target;            /**< 严格校验后的目标值。 */
    } control_gate_control_intent_t;

    typedef struct
    {
        control_gate_control_action_type_t type;         /**< 下一普通控制 typed 动作。 */
        uint32_t request_id;                             /**< 启动周期内非零请求 ID。 */
        char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY]; /**< 事务绑定规范 MAC。 */
        uint32_t link_generation;                        /**< 事务绑定非零物理链路代次。 */
        watch_control_kind_t kind;                       /**< 锁定的普通控制类型。 */
        uint8_t target;                                  /**< 锁定的普通控制目标。 */
        uint8_t attempt;                                 /**< 同一逻辑控制的当前 attempt，范围 1-3。 */
    } control_gate_control_action_t;

    typedef enum
    {
        CONTROL_GATE_ACTION_NONE = 0,     /**< 不向外部服务产生动作。 */
        CONTROL_GATE_ACTION_RENTAL_QUERY, /**< 向 cloud_task 投递租赁查询。 */
        CONTROL_GATE_ACTION_BLE_UNLOCK,   /**< 向 ble_task 内部写入一次解锁包。 */
    } control_gate_action_type_t;

    typedef struct
    {
        control_gate_action_type_t type;                 /**< 下一 typed 动作。 */
        uint32_t request_id;                             /**< 当前启动周期非零请求 ID。 */
        char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY]; /**< 事务绑定规范 MAC。 */
        uint32_t link_generation;                        /**< 事务绑定物理链路代次。 */
    } control_gate_action_t;

    typedef struct
    {
        uint32_t request_id;                              /**< cloud 终态对应请求 ID。 */
        char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY];  /**< cloud 终态绑定规范 MAC。 */
        uint32_t link_generation;                         /**< cloud 终态绑定物理链路代次。 */
        watch_rental_result_t result;                     /**< 有界本地租赁终态。 */
        uint16_t http_status;                             /**< HTTP 状态，transport 前失败为 0。 */
        char error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< CLOUD_ 稳定码。 */
    } control_gate_rental_result_t;

    typedef struct
    {
        uint32_t next_request_id;                        /**< 当前启动周期请求 ID 生成器。 */
        uint32_t request_id;                             /**< 活动事务请求 ID。 */
        char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY]; /**< 活动事务规范 MAC。 */
        uint32_t link_generation;                        /**< 活动事务链路代次。 */
        watch_rental_phase_t phase;                      /**< reducer 私有授权阶段。 */
        watch_rental_result_t last_result;               /**< 最近租赁终态。 */
        uint32_t control_update_sequence;                /**< watch_state typed 更新序号。 */
        uint32_t status_sequence_at_write;               /**< GATT callback 成功时的有效帧序号。 */
        uint64_t confirm_started_ms;                     /**< 写 callback 成功的单调毫秒。 */
        bool pending;                                    /**< 是否存在唯一活动事务。 */
        watch_control_kind_t control_kind;               /**< 当前普通控制类型。 */
        uint8_t control_target;                          /**< 当前普通控制目标。 */
        watch_control_phase_t control_phase;             /**< 当前普通控制操作阶段。 */
        watch_control_result_t control_last_result;      /**< 最近普通控制有界终态。 */
        bool control_pending;                            /**< 普通控制是否占用唯一控制槽。 */
        uint32_t control_status_sequence_at_write;       /**< 普通控制 callback 成功时的有效状态序号。 */
        uint8_t control_attempt;                         /**< 同一逻辑普通控制当前 attempt，范围 0-3。 */
        uint64_t control_ack_deadline_ms;                /**< 普通控制产品 ACK 截止毫秒。 */
        bool control_locked_out;                         /**< 三次失败后的零写入重连锁止。 */
        uint32_t lockout_link_generation;                /**< 触发锁止的原物理链路代次。 */
        bool lockout_disconnect_seen;                    /**< 锁止后是否已观察到真实断开。 */
        watch_poweroff_result_t poweroff_result;         /**< 一次性关机独立产品终态。 */
        uint64_t poweroff_deadline_ms;                   /**< 一次性关机产品结果截止毫秒。 */
        bool session_invalidated;                        /**< owner 已明确判定当前物理链路授权失效，滞后快照不得重新放行。 */
    } control_gate_t;

    /**
     * @brief 初始化 RAM-only 首次授权 reducer
     * @param gate reducer 实例
     */
    void control_gate_init(control_gate_t *gate);

    /**
     * @brief 使用单份不可变 watch_state 快照请求本会话首次解锁
     * @param gate reducer 实例
     * @param snapshot 调用方只读取一次得到的不可变产品快照
     * @param link_generation 当前非零 BLE 链路代次
     * @param action 下一 typed 动作输出
     * @param update 产品状态更新输出
     * @return accepted、already-unlocked、busy 或 blocked
     */
    control_gate_request_status_t control_gate_request_first_unlock(
        control_gate_t *gate,
        const watch_state_snapshot_t *snapshot,
        uint32_t link_generation,
        control_gate_action_t *action,
        watch_control_update_t *update);

    /**
     * @brief 使用单份不可变产品快照请求已解锁会话内普通控制
     * @param gate 唯一控制 reducer 实例
     * @param snapshot 调用方只读取一次得到的不可变产品快照
     * @param link_generation 当前 BLE owner 非零链路代次
     * @param intent 严格 typed 控制类型与目标
     * @param action 准入成功时的完整 owner 动作
     * @param update no-op 或 accepted 的产品控制域更新
     * @return accepted、no-op、busy、auth-required、blocked 或 invalid
     */
    control_gate_control_request_status_t control_gate_request_control(
        control_gate_t *gate,
        const watch_state_snapshot_t *snapshot,
        uint32_t link_generation,
        const control_gate_control_intent_t *intent,
        control_gate_control_action_t *action,
        watch_control_update_t *update);

    /**
     * @brief 应用普通控制初始有响应写入的即时终态
     * @param gate 唯一控制 reducer 实例
     * @param request_id callback 绑定请求 ID
     * @param exoskeleton_mac callback 绑定规范 MAC
     * @param link_generation callback 绑定链路代次
     * @param kind callback 绑定控制类型
     * @param target callback 绑定目标值
     * @param success 初始调用或 callback 是否成功
     * @param status_sequence callback 时最近有效状态序号
     * @param update WRITE_REJECTED 或 SUBMITTED_UNCONFIRMED 产品更新
     * @return ESP_OK 已应用，ESP_ERR_NOT_FOUND 表示迟到、重复或身份不匹配
     */
    esp_err_t control_gate_apply_control_write_result(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        watch_control_kind_t kind,
        uint8_t target,
        bool success,
        uint32_t status_sequence,
        watch_control_update_t *update);

    /**
     * @brief 按当前单调时刻应用普通控制或一次性关机写 procedure 终态
     * @param gate 唯一控制 reducer 实例
     * @param request_id callback 绑定请求 ID
     * @param exoskeleton_mac callback 绑定规范 MAC
     * @param link_generation callback 绑定链路代次
     * @param kind callback 绑定控制类型
     * @param target callback 绑定目标值
     * @param success 初始调用或 callback 是否成功
     * @param status_sequence callback 时最近有效状态序号
     * @param now_ms callback 完成的当前启动周期单调毫秒
     * @param action 失败可重试时输出同身份下一 attempt 动作
     * @param update retry、ACK 等待、关机 pending 或终态产品更新
     * @return ESP_OK 已应用，ESP_ERR_NOT_FOUND 表示迟到、重复或身份不匹配
     */
    esp_err_t control_gate_apply_control_write_result_at(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        watch_control_kind_t kind,
        uint8_t target,
        bool success,
        uint32_t status_sequence,
        uint64_t now_ms,
        control_gate_control_action_t *action,
        watch_control_update_t *update);

    /**
     * @brief 推进普通控制 3000 毫秒产品 ACK 截止并生成下一 attempt
     * @param gate 唯一控制 reducer 实例
     * @param now_ms 当前启动周期单调毫秒
     * @param action 到期且仍可重试时输出同身份下一 attempt
     * @param update 到期时输出 retry 或三次耗尽锁止更新
     * @return ESP_OK 尚未到期或无活动 ACK，ESP_ERR_TIMEOUT 已处理本 attempt 超时
     */
    esp_err_t control_gate_check_control_timeout(
        control_gate_t *gate,
        uint64_t now_ms,
        control_gate_control_action_t *action,
        watch_control_update_t *update);

    /**
     * @brief 记录锁止后同一绑定身份的真实 BLE 断开证据
     * @param gate 唯一控制 reducer 实例
     * @param exoskeleton_mac 断开链路的规范 MAC
     * @param link_generation 断开前非零物理链路代次
     * @return true 首次接受匹配断开证据，false 无锁止、重复或身份不匹配
     */
    bool control_gate_note_disconnect(
        control_gate_t *gate,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation);

    /**
     * @brief 在断开后的新链路重新授权完成且完全就绪时解除普通控制锁止
     * @param gate 唯一控制 reducer 实例
     * @param snapshot 新链路 MTU/Notify/新鲜状态已就绪的不可变产品快照
     * @param link_generation 新的非零物理链路代次
     * @param update 清除锁止且采用新链路解锁证明的产品更新
     * @return true 已解除，false 证据不足、代次未变化或原本无锁止
     */
    bool control_gate_release_lockout_after_reconnect(
        control_gate_t *gate,
        const watch_state_snapshot_t *snapshot,
        uint32_t link_generation,
        watch_control_update_t *update);

    /**
     * @brief 以同一事务的断链或状态失鲜证据完成一次性关机
     * @param gate 唯一控制 reducer 实例
     * @param request_id 关机请求 ID
     * @param exoskeleton_mac 关机事务绑定规范 MAC
     * @param link_generation 关机事务绑定物理链路代次
     * @param now_ms 离线证据的当前启动周期单调毫秒
     * @param update assumed_off 且清除会话的产品终态更新
     * @return ESP_OK 已完成，ESP_ERR_NOT_FOUND 表示迟到、过期或身份不匹配
     */
    esp_err_t control_gate_apply_poweroff_offline_evidence(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        uint64_t now_ms,
        watch_control_update_t *update);

    /**
     * @brief 以同一 MAC 新鲜在线状态完成关机失败分类
     * @param gate 唯一控制 reducer 实例
     * @param request_id 关机请求 ID
     * @param exoskeleton_mac 当前在线设备规范 MAC
     * @param observed_link_generation 当前在线物理链路代次
     * @param status_sequence 当前有效状态序号
     * @param status_fresh 当前在线状态是否新鲜
     * @param continuity_proven 是否可证明同一进程、无关机或重启证据的连续会话
     * @param now_ms 在线证据的当前启动周期单调毫秒
     * @param update failed_reconnected 产品终态更新
     * @return ESP_OK 已完成，ESP_ERR_NOT_FOUND 表示迟到、旧帧或证据不合格
     */
    esp_err_t control_gate_apply_poweroff_online_evidence(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t observed_link_generation,
        uint32_t status_sequence,
        bool status_fresh,
        bool continuity_proven,
        uint64_t now_ms,
        watch_control_update_t *update);

    /**
     * @brief 推进一次性关机 15000 毫秒产品结果截止
     * @param gate 唯一控制 reducer 实例
     * @param now_ms 当前启动周期单调毫秒
     * @param update 到期时 timeout_unknown 且清除会话的产品终态更新
     * @return ESP_OK 尚未到期或无活动关机，ESP_ERR_TIMEOUT 已固化未知终态
     */
    esp_err_t control_gate_check_poweroff_timeout(
        control_gate_t *gate,
        uint64_t now_ms,
        watch_control_update_t *update);

    /**
     * @brief 用写 callback 之后的新鲜同身份状态帧确认普通控制目标
     * @param gate 唯一控制 reducer 实例
     * @param request_id 状态证据绑定请求 ID
     * @param exoskeleton_mac 状态证据绑定规范 MAC
     * @param link_generation 状态证据绑定链路代次
     * @param kind 状态证据绑定控制类型
     * @param target 状态证据绑定控制目标
     * @param status_sequence 当前有效状态序号
     * @param actual_state 当前合法状态帧解析出的设备实际字段
     * @param status_fresh 当前状态是否仍新鲜
     * @param update APPLIED 产品控制更新
     * @return ESP_OK 已确认，ESP_ERR_NOT_FOUND 表示证据未命中或事务已结束
     */
    esp_err_t control_gate_apply_control_status_evidence(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        watch_control_kind_t kind,
        uint8_t target,
        uint32_t status_sequence,
        const watch_exoskeleton_state_t *actual_state,
        bool status_fresh,
        watch_control_update_t *update);

    /**
     * @brief 在副本上准备普通控制 APPLIED 终态，原 gate 保持 pending 直到状态已确认应用
     * @param gate 当前普通控制 reducer
     * @param request_id 状态证据绑定请求 ID
     * @param exoskeleton_mac 状态证据绑定规范 MAC
     * @param link_generation 状态证据绑定链路代次
     * @param kind 状态证据对应控制类型
     * @param target 状态证据对应目标
     * @param status_sequence 当前有效状态序号
     * @param actual_state 当前完整设备实际状态
     * @param status_fresh 当前状态是否仍新鲜
     * @param prepared_gate 仅在返回 ESP_OK 时写入可提交 reducer 副本
     * @param update 仅在返回 ESP_OK 时写入待确认应用的 APPLIED 产品更新
     * @return ESP_OK 已准备，ESP_ERR_NOT_FOUND 证据不匹配，ESP_ERR_INVALID_ARG 参数无效
     */
    esp_err_t control_gate_prepare_control_status_evidence(
        const control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        watch_control_kind_t kind,
        uint8_t target,
        uint32_t status_sequence,
        const watch_exoskeleton_state_t *actual_state,
        bool status_fresh,
        control_gate_t *prepared_gate,
        watch_control_update_t *update);

    /**
     * @brief 应用 cloud_task 返回的有界租赁终态
     * @param gate reducer 实例
     * @param result 请求 ID、MAC、链路代次和本地映射终态
     * @param action paid 时产生唯一 BLE 解锁动作
     * @param update 产品状态更新输出
     * @return ESP_OK 已应用，ESP_ERR_NOT_FOUND 表示迟到或不匹配终态
     */
    esp_err_t control_gate_apply_rental_result(
        control_gate_t *gate,
        const control_gate_rental_result_t *result,
        control_gate_action_t *action,
        watch_control_update_t *update);

    /**
     * @brief 应用一次性 BLE 写 callback 终态
     * @param gate reducer 实例
     * @param request_id callback 请求 ID
     * @param exoskeleton_mac callback 绑定 MAC
     * @param link_generation callback 链路代次
     * @param success callback status 是否成功
     * @param status_sequence callback 时最近有效状态序号
     * @param now_ms callback 单调毫秒
     * @param update 产品状态更新输出
     * @return ESP_OK 已应用，ESP_ERR_NOT_FOUND 表示迟到或重复 callback
     */
    esp_err_t control_gate_apply_write_result(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        bool success,
        uint32_t status_sequence,
        uint64_t now_ms,
        watch_control_update_t *update);

    /**
     * @brief 使用写 callback 之后的下一帧有效状态完成解锁闭环
     * @param gate reducer 实例
     * @param request_id 状态证据请求 ID
     * @param exoskeleton_mac 状态证据绑定 MAC
     * @param link_generation 状态证据链路代次
     * @param status_sequence 当前完整有效状态序号
     * @param now_ms 当前单调毫秒
     * @param update 成功或超时产品状态更新输出
     * @return ESP_OK 成功解锁，ESP_ERR_NOT_FOUND 表示旧帧/错事务，ESP_ERR_TIMEOUT 表示超时
     */
    esp_err_t control_gate_apply_status_evidence(
        control_gate_t *gate,
        uint32_t request_id,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        uint32_t status_sequence,
        uint64_t now_ms,
        watch_control_update_t *update);

    /**
     * @brief 在 ble_task 周期中检查 5 秒确认截止
     * @param gate reducer 实例
     * @param now_ms 当前单调毫秒
     * @param update 超时时的产品状态更新输出
     * @return ESP_OK 无超时，ESP_ERR_TIMEOUT 已固化超时终态
     */
    esp_err_t control_gate_check_timeout(control_gate_t *gate,
                                         uint64_t now_ms,
                                         watch_control_update_t *update);

    /**
     * @brief 将自动授权入口的本地终态失败绑定到当前链路
     * @param gate reducer 实例
     * @param exoskeleton_mac 当前绑定的规范外骨骼 MAC
     * @param link_generation 当前非零 BLE 链路代次
     * @param result 非 PAID 的本地租赁失败终态
     * @param error_code CLOUD_ 或 BLE_ 稳定码
     * @param update 当前链路 BLOCKED 产品状态更新输出
     * @return ESP_OK 已生成终态，ESP_ERR_INVALID_ARG 参数非法，ESP_ERR_INVALID_STATE 当前已有事务或会话已解锁
     */
    esp_err_t control_gate_fail_authorization_entry(
        control_gate_t *gate,
        const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
        uint32_t link_generation,
        watch_rental_result_t result,
        const char *error_code,
        watch_control_update_t *update);

    /**
     * @brief 因断连、身份或链路代次变化有界取消 pending
     * @param gate reducer 实例
     * @param update 取消后的 fail-closed 产品状态更新输出
     * @return true 取消了活动事务，false 原本无 pending
     */
    bool control_gate_abort_pending(control_gate_t *gate,
                                    watch_control_update_t *update);

    /**
     * @brief 原子中止当前事务并撤销跨物理链路授权证明
     * @param gate reducer 实例
     * @param update 中止后的待授权产品状态更新输出
     * @return true 中止了活动事务，false 原本无 pending
     */
    bool control_gate_abort_and_invalidate_session(
        control_gate_t *gate,
        watch_control_update_t *update);

    /**
     * @brief 清除当前 BLE 物理链路的 RAM-only 授权证明
     * @param gate reducer 实例
     * @param update 回到待授权状态的产品更新输出
     */
    void control_gate_invalidate_session(control_gate_t *gate,
                                         watch_control_update_t *update);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CONTROL_GATE_H */
