/**
 * @file     ble_service.h
 * @brief    BLE 服务公共接口
 * @details  声明 BLE 服务固定 ID、有界 typed 队列、控制 intent/result 与 NimBLE host 生命周期入口。
 * @author   ZHC
 * @date     2026-07-13
 */

#ifndef LEGBOT_BLE_SERVICE_H
#define LEGBOT_BLE_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"
#include "ble_status_frame.h"
#include "ble_recovery_policy.h"
#include "ble_unlock.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"

/** BLE 服务在统一服务表中的固定 ID。 */
#define LEGBOT_BLE_SERVICE_ID LEGBOT_SERVICE_BLE
/** BLE owner 状态机、候选快照与控制证据链的任务栈预算。 */
#define BLE_SERVICE_TASK_STACK_BYTES 8192U
/** BLE host callback 到 ble_task 的私有 typed 事件队列深度。 */
#define BLE_EVENT_QUEUE_DEPTH 32U
/** 未绑定候选扫描的有界时间窗口。缩短为 5 秒以快速显示候选。 */
#define BLE_BINDING_SCAN_WINDOW_MS 5000
/** BLE 默认扫描、建链与连接发射功率，单位 dBm。 */
#define BLE_TX_POWER_DEFAULT_DBM 0
/** BLE 弱链路下一次物理连接的临时发射功率，单位 dBm。 */
#define BLE_TX_POWER_BOOST_DBM 9
/** 最近扫描报告触发下一次物理连接临时升功率的 RSSI 阈值。 */
#define BLE_TX_POWER_WEAK_RSSI_DBM (-80)
/** 两次异常链路丢失触发临时升功率的滚动窗口。 */
#define BLE_TX_POWER_ABNORMAL_DISCONNECT_WINDOW_MS 600000U
/** 同一普通控制已经执行两次重试时对应的 attempt 序号。 */
#define BLE_TX_POWER_CONTROL_RETRY_TRIGGER_ATTEMPT 3U
/** BLE 证据与身份接口的规范 MAC 容量。 */
#define BLE_MAC_CAPACITY BLE_STATUS_MAC_CAPACITY
/** 目标外骨骼 Service 的 16-bit UUID 表达。 */
#define BLE_TARGET_SERVICE_UUID16 0x00FFU
/** 目标外骨骼 Characteristic 的 16-bit UUID 表达。 */
#define BLE_TARGET_CHARACTERISTIC_UUID16 0xFF01U
/** Client Characteristic Configuration Descriptor 的 UUID。 */
#define BLE_TARGET_CCCD_UUID16 0x2902U
/** Notify 开启的总尝试上限：首次加两次重试。 */
#define BLE_NOTIFY_ENABLE_MAX_ATTEMPTS 3U
/** 单条链路 GATT 准入的最长时间。 */
#define BLE_LINK_ADMISSION_TIMEOUT_MS 30000U
/** Host callback 单次复制的 Notify 负载上限。 */
#define BLE_NOTIFY_PAYLOAD_CAPACITY 69U
/** BLE 服务启动失败稳定错误码。 */
#define BLE_START_FAILED "BLE_START_FAILED"
/** NimBLE host reset 稳定错误码。 */
#define BLE_HOST_RESET "BLE_HOST_RESET"
/** BLE 扫描启动失败稳定错误码。 */
#define BLE_SCAN_FAILED "BLE_SCAN_FAILED"
/** BLE 连接失败稳定错误码。 */
#define BLE_CONNECT_FAILED "BLE_CONNECT_FAILED"
/** BLE host callback 事件队列溢出稳定错误码。 */
#define BLE_EVENT_QUEUE_OVERFLOW "BLE_EVENT_QUEUE_OVERFLOW"
/** 候选超过维护壳层 8 项上限的稳定错误码。 */
#define BLE_CANDIDATE_OVERFLOW "BLE_CANDIDATE_OVERFLOW"
/** 相同文本 MAC 同时使用不同地址类型，无法安全持久化身份。 */
#define BLE_CANDIDATE_IDENTITY_CONFLICT "BLE_CANDIDATE_IDENTITY_CONFLICT"
/** 候选快照已保存但 UI 刷新请求队列满。 */
#define BLE_UI_REFRESH_QUEUE_FULL "BLE_UI_REFRESH_QUEUE_FULL"
/** BLE 选择命令已过期或 MAC 不在同代次。 */
#define BLE_SELECTION_STALE "BLE_SELECTION_STALE"
/** BLE 选择命令在连接进行中被拒绝。 */
#define BLE_SELECTION_BUSY "BLE_SELECTION_BUSY"
/** BLE 选择命令连接发起失败。 */
#define BLE_SELECTION_ERROR "BLE_SELECTION_ERROR"
/** 绑定 NVS set 或 commit 失败稳定错误码。 */
#define BLE_BINDING_COMMIT_FAILED "BLE_BINDING_COMMIT_FAILED"
/** BLE 最近操作无错误的稳定码。 */
#define BLE_OK "BLE_OK"
/** BLE 链路已断开稳定码，不表示外骨骼已重新上锁。 */
#define BLE_DISCONNECTED "BLE_DISCONNECTED"
/** BLE 绑定证据顺序、handle、MAC 或值被拒绝。 */
#define BLE_BINDING_EVIDENCE_REJECTED "BLE_BINDING_EVIDENCE_REJECTED"
/** BLE 状态快照无法在有界时间内入队。 */
#define BLE_STATE_QUEUE_FULL "BLE_STATE_QUEUE_FULL"
/** BLE 绑定单 key erase 或 commit 失败。 */
#define BLE_BINDING_CLEAR_FAILED "BLE_BINDING_CLEAR_FAILED"
/** BLE 绑定配置存在但格式损坏，已拒绝使用。 */
#define BLE_BINDING_CONFIG_CORRUPT "BLE_BINDING_CONFIG_CORRUPT"
/** BLE 旧 peer 终止请求失败，物理链路事实仍保留。 */
#define BLE_DISCONNECT_FAILED "BLE_DISCONNECT_FAILED"
/** NimBLE host task 未在有界时间内完成删除。 */
#define BLE_HOST_STOP_TIMEOUT "BLE_HOST_STOP_TIMEOUT"
/** NimBLE host 拒绝启动停止流程。 */
#define BLE_HOST_STOP_FAILED "BLE_HOST_STOP_FAILED"
/** NimBLE port 在 host task 退出后仍回收失败。 */
#define BLE_HOST_DEINIT_FAILED "BLE_HOST_DEINIT_FAILED"
/** NimBLE host 未在有界时间内完成首次同步。 */
#define BLE_HOST_SYNC_TIMEOUT "BLE_HOST_SYNC_TIMEOUT"
/** 无线栈回收后无法重新取得 DORMANT retention 保留块。 */
#define BLE_DORMANT_RETENTION_FAILED "BLE_DORMANT_RETENTION_FAILED"
/** 目标 Service 缺失、重复或身份不唯一。 */
#define BLE_GATT_SERVICE_NOT_FOUND "BLE_GATT_SERVICE_NOT_FOUND"
/** 目标 Characteristic 缺失、重复或属性不兼容。 */
#define BLE_GATT_CHARACTERISTIC_INCOMPATIBLE "BLE_GATT_CHARACTERISTIC_INCOMPATIBLE"
/** 目标 Characteristic 的 CCCD 缺失或不唯一。 */
#define BLE_GATT_CCCD_NOT_FOUND "BLE_GATT_CCCD_NOT_FOUND"
/** GATT discovery procedure 启动或回调失败。 */
#define BLE_GATT_DISCOVERY_FAILED "BLE_GATT_DISCOVERY_FAILED"
/** ATT MTU exchange 启动或回调失败。 */
#define BLE_MTU_EXCHANGE_FAILED "BLE_MTU_EXCHANGE_FAILED"
/** CCCD 初始尝试和两次重试全部失败。 */
#define BLE_NOTIFY_ENABLE_FAILED "BLE_NOTIFY_ENABLE_FAILED"
/** Notify 原始字节复制、长度或 host 事件入口失败。 */
#define BLE_NOTIFY_INGRESS_FAILED "BLE_NOTIFY_INGRESS_FAILED"
/** 单条链路的 GATT 准入超过 30 秒。 */
#define BLE_LINK_ADMISSION_TIMEOUT "BLE_LINK_ADMISSION_TIMEOUT"
/** 状态 freshness 从 fresh 转为 stale。 */
#define BLE_STATUS_STALE "BLE_STATUS_STALE"
/** 完整 67 字节候选未通过协议字段校验。 */
#define BLE_PROTOCOL_FRAME_INVALID "BLE_PROTOCOL_FRAME_INVALID"
/** 首次解锁不可变 GATT callback context 池深度。 */
#define BLE_UNLOCK_REQUEST_QUEUE_DEPTH 2U
/** 首次解锁业务终态队列深度。 */
#define BLE_UNLOCK_RESULT_QUEUE_DEPTH 2U
/** 一次性解锁初始 GATT 调用失败。 */
#define BLE_UNLOCK_WRITE_START_FAILED "BLE_UNLOCK_WRITE_START_FAILED"
/** 一次性解锁 GATT callback 返回失败。 */
#define BLE_UNLOCK_WRITE_CALLBACK_FAILED "BLE_UNLOCK_WRITE_CALLBACK_FAILED"
/** 一次性解锁 GATT callback 未在有界时间内返回。 */
#define BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT "BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT"
/** 解锁写后 5 秒内没有下一帧有效状态。 */
#define BLE_UNLOCK_CONFIRM_TIMEOUT "BLE_UNLOCK_CONFIRM_TIMEOUT"
/** 解锁事务因连接身份或代次变化作废。 */
#define BLE_UNLOCK_LINK_CHANGED "BLE_UNLOCK_LINK_CHANGED"
/** 普通控制业务终态队列深度。 */
#define BLE_CONTROL_RESULT_QUEUE_DEPTH 4U
/** 结果背压后可容纳全部已接收普通控制 intent 的内部 FIFO 深度。 */
#define BLE_CONTROL_PENDING_RESULT_DEPTH (BLE_EVENT_QUEUE_DEPTH + 2U)
/** 普通控制初始写 callback 的最长等待时间。 */
#define BLE_CONTROL_WRITE_CALLBACK_TIMEOUT_MS 5000U
/** 普通控制产品快照读取失败。 */
#define BLE_CONTROL_STATE_SNAPSHOT_FAILED "BLE_CONTROL_STATE_SNAPSHOT_FAILED"
/** 普通控制 owner 复核事实变化，未调用 GATT。 */
#define BLE_CONTROL_REVALIDATION_FAILED "BLE_CONTROL_REVALIDATION_FAILED"
/** 普通控制因更高优先级外骨骼关机请求被取消。 */
#define BLE_CONTROL_PREEMPTED_BY_POWEROFF "BLE_CONTROL_PREEMPTED_BY_POWEROFF"
/** 普通控制状态 apply-ACK 未确认，未调用 GATT。 */
#define BLE_CONTROL_STATE_SYNC_FAILED "BLE_CONTROL_STATE_SYNC_FAILED"
/** 普通控制 NimBLE 初始调用失败。 */
#define BLE_CONTROL_WRITE_START_FAILED "BLE_CONTROL_WRITE_START_FAILED"
/** 普通控制 GATT callback 失败。 */
#define BLE_CONTROL_WRITE_CALLBACK_FAILED "BLE_CONTROL_WRITE_CALLBACK_FAILED"
/** 普通控制 GATT callback 丢失并触发有界隔离。 */
#define BLE_CONTROL_WRITE_CALLBACK_TIMEOUT "BLE_CONTROL_WRITE_CALLBACK_TIMEOUT"
/** 普通控制正在执行同身份下一 attempt。 */
#define BLE_CONTROL_RETRYING "BLE_CONTROL_RETRYING"
/** 普通控制三次 attempt 已耗尽并等待真实重连。 */
#define BLE_CONTROL_RETRY_EXHAUSTED "BLE_CONTROL_RETRY_EXHAUSTED"
/** 一次性关机写已接受，等待产品离线或在线证据。 */
#define BLE_POWEROFF_PENDING "BLE_POWEROFF_PENDING"
/** 一次性关机窗口内观察到离线或状态失鲜。 */
#define BLE_POWEROFF_ASSUMED_OFF "BLE_POWEROFF_ASSUMED_OFF"
/** 一次性关机 15 秒窗口内证据不足。 */
#define BLE_POWEROFF_TIMEOUT_UNKNOWN "BLE_POWEROFF_TIMEOUT_UNKNOWN"
/** 一次性关机后同一设备仍有新鲜在线状态。 */
#define BLE_POWEROFF_FAILED_RECONNECTED "BLE_POWEROFF_FAILED_RECONNECTED"

typedef enum
{
    BLE_UNLOCK_RESULT_SUCCESS = 0,      /**< 云授权、写 callback 与下一帧闭环成功。 */
    BLE_UNLOCK_RESULT_ALREADY_UNLOCKED, /**< 已有会话证明，本次无写入。 */
    BLE_UNLOCK_RESULT_BUSY,             /**< 已有授权或解锁事务。 */
    BLE_UNLOCK_RESULT_BLOCKED,          /**< 链路、状态或配置准入失败。 */
    BLE_UNLOCK_RESULT_CLOUD_DENIED,     /**< 云端业务拒绝或云错误。 */
    BLE_UNLOCK_RESULT_WRITE_FAILED,     /**< GATT 初始调用或 callback 失败。 */
    BLE_UNLOCK_RESULT_CONFIRM_TIMEOUT,  /**< 下一帧状态证据超时。 */
    BLE_UNLOCK_RESULT_LINK_CHANGED,     /**< MAC、连接或链路代次变化。 */
    BLE_UNLOCK_RESULT_PAYMENT_CHECK_ACCEPTED, /**< 人工立即检查已被当前支付会话接受。 */
} ble_unlock_result_code_t;

typedef struct
{
    uint32_t intent_sequence;                         /**< 调用方首次解锁 intent 的非零关联序号；自动入口为 0。 */
    uint32_t request_id;                              /**< control_gate 请求 ID；准入前失败可为 0。 */
    char exoskeleton_mac[BLE_MAC_CAPACITY];           /**< 事务绑定规范 MAC。 */
    uint32_t link_generation;                         /**< 事务绑定链路代次。 */
    ble_unlock_result_code_t result;                  /**< 本地有界业务终态。 */
    char error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< CLOUD_/BLE_ 稳定码。 */
} ble_unlock_result_t;

typedef enum
{
    BLE_CONTROL_RESULT_NO_OP = 0,                   /**< 目标与设备状态协议等价，零写入成功。 */
    BLE_CONTROL_RESULT_BUSY,                        /**< 唯一授权或普通控制槽已被占用。 */
    BLE_CONTROL_RESULT_AUTH_REQUIRED,               /**< 当前没有有效解锁会话证明。 */
    BLE_CONTROL_RESULT_BLOCKED,                     /**< 产品准入或 owner 复核失败。 */
    BLE_CONTROL_RESULT_INVALID,                     /**< typed 类型或目标严格范围校验失败。 */
    BLE_CONTROL_RESULT_WRITE_REJECTED,              /**< 初始调用或 callback 失败。 */
    BLE_CONTROL_RESULT_SUBMITTED_UNCONFIRMED,       /**< 写 procedure 成功，尚未证明设备应用。 */
    BLE_CONTROL_RESULT_APPLIED,                     /**< 新鲜后续状态帧已证明普通控制目标生效。 */
    BLE_CONTROL_RESULT_RETRYING,                    /**< 前一 attempt 失败，下一 attempt 已排定。 */
    BLE_CONTROL_RESULT_RETRY_EXHAUSTED,             /**< 三次 attempt 失败并进入重连锁止。 */
    BLE_CONTROL_RESULT_POWEROFF_PENDING,            /**< 关机写已接受，等待 15 秒产品结果。 */
    BLE_CONTROL_RESULT_POWEROFF_ASSUMED_OFF,        /**< 已观察到离线证据，预计已关机。 */
    BLE_CONTROL_RESULT_POWEROFF_TIMEOUT_UNKNOWN,    /**< 关机结果未知且会话已清除。 */
    BLE_CONTROL_RESULT_POWEROFF_FAILED_RECONNECTED, /**< 同一设备仍在线，关机失败。 */
} ble_control_result_code_t;

typedef struct
{
    watch_control_kind_t kind; /**< 普通控制类型。 */
    uint8_t target;            /**< 普通控制目标；不允许调用方依赖 clamp。 */
} ble_control_intent_t;

typedef struct
{
    uint32_t request_id;                              /**< control_gate 请求 ID；准入前终态可为 0。 */
    char exoskeleton_mac[BLE_MAC_CAPACITY];           /**< 已接受事务绑定规范 MAC。 */
    uint32_t link_generation;                         /**< 已接受事务绑定链路代次。 */
    watch_control_kind_t kind;                        /**< 结果对应控制类型。 */
    uint8_t target;                                   /**< 结果对应原始或已锁定目标。 */
    uint8_t attempt;                                  /**< 已接受事务当前或终态 attempt。 */
    ble_control_result_code_t result;                 /**< 本地有界普通控制终态。 */
    char error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< BLE_ 稳定码。 */
} ble_control_result_t;

/** BLE 公共证据接口复用的不可分割链路 token。 */
typedef ble_status_owner_token_t ble_link_token_t;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 为固定 ble_task 创建私有 typed 事件队列
     * @return ESP_OK 成功或已准备，ESP_ERR_NO_MEM 表示队列创建失败
     */
    esp_err_t ble_service_prepare_run(void);

    /**
     * @brief 取消尚未启动的 BLE 服务准备状态
     * @details 只供统一服务框架在任务创建失败或全局初始化回滚时调用。
     */
    void ble_service_cancel_prepared_run(void);

    /**
     * @brief 查询 BLE owner 是否已完成启动
     * @details 当前正常启动会在其他高内存服务落位前初始化无线栈并进入 READY；DORMANT 只保留给初始化失败恢复或完整服务关闭边界。
     * @return true 表示 owner 已完成启动，false 表示仍在启动、停止或尚未运行
     */
    bool ble_service_startup_ready(void);

    /**
     * @brief 查询 NimBLE host、控制器与 PHY 是否已同步并归 ble_task 所有
     * @return true 表示无线栈实际 READY，false 表示初始化失败恢复中、启动中或已停止
     */
    bool ble_service_runtime_ready(void);

    /**
     * @brief 向 BLE owner 锁存最新物理屏幕事实
     * @details 序号采用 power owner 的非零单调转换序号；队列满时最新事实仍由原子 mailbox 保留。
     * @param screen_on true 表示屏幕已实际点亮，false 表示屏幕已实际熄灭
     * @param transition_sequence power owner 的非零屏幕转换序号
     * @return ESP_OK 已锁存，ESP_ERR_INVALID_ARG 序号非法，ESP_ERR_INVALID_STATE 服务未准备
     */
    esp_err_t ble_service_notify_screen_state(
        bool screen_on,
        uint32_t transition_sequence);

    /**
     * @brief 锁存 BLE 服务停止意图并尝试唤醒事件循环
     * @details 独立原子 latch 保证私有队列满或运行时恢复期间也不会丢失停止意图。
     * @param timeout_ticks 尝试向私有队列投递唤醒事件的有界 tick 数
     * @return ESP_OK 停止意图已锁存，ESP_ERR_INVALID_STATE 未准备
     */
    esp_err_t ble_service_request_stop(TickType_t timeout_ticks);

    /**
     * @brief 通知 BLE owner 云租赁结果队列已有新终态
     * @details 只投递无负载唤醒事件，结果仍由 cloud_service 的有界队列按值领取。
     * @return ESP_OK 已唤醒或 owner 正在处理事件，其他值表示服务尚未运行
     */
    esp_err_t ble_service_notify_cloud_result_ready(void);

    /**
     * @brief 向唯一 ble_task 投递候选页扫描会话开始请求
     * @details 仅在未绑定时开启一次 5 秒候选扫描；重复进入不会创建第二个 owner 或循环扫描。
     * @param timeout_ticks 等待私有有界队列空间的 tick 数
     * @return ESP_OK 已入队，ESP_ERR_INVALID_STATE 服务未运行，ESP_ERR_TIMEOUT 队列满
     */
    esp_err_t ble_service_begin_binding_scan_session(
        TickType_t timeout_ticks);

    /**
     * @brief 向唯一 ble_task 投递候选页扫描会话结束请求
     * @details 作废迟到扫描代次并清空候选、反馈和未消费选择；不取消用户已发起的连接。
     * @param timeout_ticks 等待私有有界队列空间的 tick 数
     * @return ESP_OK 已入队，ESP_ERR_INVALID_STATE 服务未运行，ESP_ERR_TIMEOUT 队列满
     */
    esp_err_t ble_service_end_binding_scan_session(
        TickType_t timeout_ticks);

    /**
     * @brief 向唯一 ble_task 投递立即重连 typed 请求
     * @details 仅对已绑定、未连接且屏幕点亮的设备生效，并由 BLE owner 绕过当前退避直接启动扫描；熄屏时保持空闲。
     * @param timeout_ticks 等待私有有界队列空间的 tick 数
     * @return ESP_OK 已入队，ESP_ERR_INVALID_STATE 服务未运行，ESP_ERR_TIMEOUT 队列满
     */
    esp_err_t ble_service_request_reconnect_now(TickType_t timeout_ticks);

    /**
     * @brief 向唯一 ble_task 投递未绑定候选重扫 typed 请求
     * @details 由 BLE owner 取消当前候选扫描并立即启动新扫描代次，不创建第二个 owner。
     * @param timeout_ticks 等待私有有界队列空间的 tick 数
     * @return ESP_OK 已入队，ESP_ERR_INVALID_STATE 服务未运行，ESP_ERR_TIMEOUT 队列满
     */
    esp_err_t ble_service_request_rescan(TickType_t timeout_ticks);

    /**
     * @brief 为当前连接提交实际 ATT MTU 诊断证据
     * @details 该证据不参与绑定提交、Notify 准入或控制可用性判定。
     * @details 已提交绑定的重连证据会按当前 handle 与 MAC 识别并忽略，不重复执行绑定事务。
     * @param link_token 由当前 Notify ingress 返回的不可变链路 token
     * @param mtu 实际协商 MTU，本地 preferred 值不得代替
     * @param timeout_ticks 等待 BLE 私有队列空间的有界 tick 数
     * @return ESP_OK 已入队，其他值表示参数、状态或队列错误
     */
    esp_err_t ble_service_publish_mtu_evidence(
        const ble_link_token_t *link_token,
        uint16_t mtu,
        TickType_t timeout_ticks);

    /**
     * @brief 向唯一 ble_task 投递“首次需要控制”的 typed intent
     * @param intent_sequence 成功入队时返回非零调用关联序号
     * @param timeout_ticks 等待私有有界队列空间的 tick 数
     * @return ESP_OK 已入队，其他值表示服务状态或队列满
     */
    esp_err_t ble_service_request_first_unlock(uint32_t *intent_sequence,
                                               TickType_t timeout_ticks);

    /**
     * @brief 领取首次解锁完整业务终态
     * @param result 终态输出
     * @param timeout_ticks 有界等待 tick
     * @return ESP_OK 已领取，其他值表示参数、状态或超时
     */
    esp_err_t ble_service_receive_unlock_result(ble_unlock_result_t *result,
                                                TickType_t timeout_ticks);

    /**
     * @brief 向唯一 ble_task 投递普通控制 typed intent
     * @details 合法关机 intent 进入独立优先邮箱，不受共享事件队列容量限制；
     *          邮箱已有未消费关机时拒绝覆盖；BLE owner 会终结较早普通控制并隔离其迟到 callback。
     * @param intent 助力、统一档位、场景模式或一次性关机目标
     * @param timeout_ticks 普通控制等待私有有界队列空间的 tick 数；关机不等待队列
     * @return ESP_OK 已受理，其他值表示服务状态、关机邮箱忙、普通控制结果背压或队列满
     */
    esp_err_t ble_service_request_control(const ble_control_intent_t *intent,
                                          TickType_t timeout_ticks);

    /**
     * @brief 按 FIFO 领取普通控制本地终态
     * @param result 有界 typed 终态输出
     * @param timeout_ticks 有界等待 tick
     * @return ESP_OK 已领取，其他值表示参数、状态或超时
     */
    esp_err_t ble_service_receive_control_result(ble_control_result_t *result,
                                                 TickType_t timeout_ticks);

    /**
     * @brief 在固定 ble_task 中运行 NimBLE Central typed 事件循环
     * @return ESP_OK 正常停止，其他值表示启动或停止错误
     */
    esp_err_t ble_service_run(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BLE_SERVICE_H */
