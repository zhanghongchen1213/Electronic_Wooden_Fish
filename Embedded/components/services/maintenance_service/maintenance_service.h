/**
 * @file     maintenance_service.h
 * @brief    最小维护与诊断服务接口
 * @details  定义维护快照、自检命令和 BLE 候选选择的有界值语义，作为当前 UI 与后续服务的稳定适配边界。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef LEGBOT_MAINTENANCE_SERVICE_H
#define LEGBOT_MAINTENANCE_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"
#include "config_service.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 单次 BLE 扫描会话允许发布的候选上限。 */
#define MAINTENANCE_BLE_CANDIDATE_MAX_COUNT 8U
/** 规范 MAC 文本容量，包含字符串终止符。 */
#define MAINTENANCE_BLE_MAC_CAPACITY 18U
/** BLE 广播名称容量，包含字符串终止符；空串表示设备未广播名称。 */
#define MAINTENANCE_BLE_NAME_CAPACITY 32U
/** NimBLE 广播报告用于表示 RSSI 不可用的保留值。 */
#define MAINTENANCE_BLE_RSSI_UNAVAILABLE INT8_MAX
/** BLE 选择 typed 命令队列深度。 */
#define MAINTENANCE_SELECTION_QUEUE_DEPTH 2U
/** BLE 确认清除 typed 命令队列深度。 */
#define MAINTENANCE_CLEAR_QUEUE_DEPTH 2U
/** 最近稳定错误码容量，包含字符串终止符。 */
#define MAINTENANCE_ERROR_CODE_CAPACITY SELFTEST_DETAIL_CODE_CAPACITY

    typedef enum
    {
        MAINTENANCE_CAPABILITY_NOT_IMPLEMENTED = 0, /**< 真实能力 owner 尚未接入。 */
        MAINTENANCE_CAPABILITY_UNAVAILABLE,         /**< owner 已接入但当前不可用。 */
        MAINTENANCE_CAPABILITY_AVAILABLE,           /**< owner 已接入且状态值可信。 */
    } maintenance_capability_status_t;

    typedef enum
    {
        MAINTENANCE_RESULT_OK = 0,           /**< 请求已完成或命令已入队。 */
        MAINTENANCE_RESULT_INVALID_ARGUMENT, /**< 参数或 MAC 格式非法。 */
        MAINTENANCE_RESULT_NOT_READY,        /**< 维护契约尚未初始化。 */
        MAINTENANCE_RESULT_BUSY,             /**< 资源、队列或自检当前忙。 */
        MAINTENANCE_RESULT_STALE,            /**< 候选代次过期或 MAC 已被替换。 */
        MAINTENANCE_RESULT_EMPTY,            /**< 有界等待内没有选择命令。 */
        MAINTENANCE_RESULT_ERROR,            /**< 下游返回其他显式错误。 */
    } maintenance_result_t;

    typedef struct
    {
        maintenance_capability_status_t status; /**< 能力接入与可用状态。 */
        bool observed_value;                    /**< 同一时刻 watch_state 中的原始事实值。 */
    } maintenance_capability_view_t;

    typedef struct
    {
        char mac[MAINTENANCE_BLE_MAC_CAPACITY];              /**< 规范大写完整 MAC 值。 */
        char advertised_name[MAINTENANCE_BLE_NAME_CAPACITY]; /**< 广播名称，缺失时为空串。 */
        int8_t rssi;                                         /**< 最近 RSSI（dBm）；127 表示不可用。 */
    } maintenance_ble_candidate_t;

    typedef struct
    {
        uint32_t generation;                                                         /**< BLE 扫描会话的单调代次。 */
        uint8_t count;                                                               /**< 当前有效候选数量。 */
        maintenance_ble_candidate_t candidates[MAINTENANCE_BLE_CANDIDATE_MAX_COUNT]; /**< 定长候选值列表。 */
    } maintenance_ble_candidate_list_t;

    typedef struct
    {
        uint32_t generation;                    /**< 选择所属候选代次。 */
        char mac[MAINTENANCE_BLE_MAC_CAPACITY]; /**< 按值复制的完整规范 MAC。 */
    } maintenance_ble_selection_command_t;

    typedef enum
    {
        MAINTENANCE_BLE_SELECTION_NONE = 0,   /**< 当前代次尚无 ble_task 最终处理结果。 */
        MAINTENANCE_BLE_SELECTION_CONNECTING, /**< 已从内部候选地址发起连接。 */
        MAINTENANCE_BLE_SELECTION_BUSY,       /**< 已有连接或绑定事务，拒绝重复选择。 */
        MAINTENANCE_BLE_SELECTION_STALE,      /**< generation 或 MAC 已过期。 */
        MAINTENANCE_BLE_SELECTION_ERROR,      /**< 候选连接或后续链路准入失败。 */
    } maintenance_ble_selection_result_t;

    typedef struct
    {
        uint32_t generation;                       /**< 处理结果所属候选代次。 */
        char mac[MAINTENANCE_BLE_MAC_CAPACITY];    /**< 处理结果所属规范 MAC。 */
        maintenance_ble_selection_result_t result; /**< ble_task 的最终处理结果。 */
    } maintenance_ble_selection_feedback_t;

    typedef struct
    {
        uint64_t version;                                            /**< 维护瞬态模型的单调版本。 */
        maintenance_ble_candidate_list_t ble_candidates;             /**< 当前候选会话按值快照。 */
        maintenance_ble_selection_feedback_t ble_selection_feedback; /**< ble_task 最终选择反馈。 */
    } maintenance_ble_ui_snapshot_t;

    typedef struct
    {
        bool confirmed; /**< 仅确认页最终操作可为 true 入队。 */
    } maintenance_ble_clear_command_t;

    typedef struct
    {
        uint64_t version;                                            /**< 维护瞬态模型的单调版本。 */
        maintenance_capability_view_t ble;                           /**< BLE 能力占位及 watch_state 事实。 */
        bool has_bound_exoskeleton;                                  /**< 是否存在已提交 BLE 绑定。 */
        char bound_exoskeleton_mac[MAINTENANCE_BLE_MAC_CAPACITY];    /**< 已提交规范绑定 MAC。 */
        watch_ble_transaction_t ble_transaction;                     /**< 当前 BLE 绑定事务状态。 */
        bool gatt_ready;                                             /**< GATT 结构准入是否完成。 */
        bool mtu_ready;                                              /**< 当前是否已记录实际 ATT MTU。 */
        uint16_t negotiated_mtu;                                     /**< 当前实际协商 ATT MTU。 */
        bool notify_ready;                                           /**< CCCD 是否已确认开启 Notify。 */
        bool device_state_available;                                 /**< 是否保留最近有效外骨骼只读状态。 */
        bool dataReady;                                              /**< 当前物理链路是否已收到有效状态帧。 */
        bool status_fresh;                                           /**< 最近状态是否仍处于 5 秒新鲜窗口。 */
        uint64_t last_status_rx_ms;                                  /**< 最近状态的本地单调接收毫秒。 */
        watch_exoskeleton_state_t exoskeleton_state;                 /**< 外骨骼产品状态只读摘要。 */
        bool link_control_ready;                                     /**< 链路控制准入证明是否有效。 */
        bool unlock_session_valid;                                   /**< 外骨骼上电会话解锁证明是否有效。 */
        watch_rental_phase_t rental_phase;                           /**< 首次租赁授权与解锁阶段。 */
        watch_control_block_reason_t control_block_reason;           /**< 控制 fail-closed 原因。 */
        watch_rental_result_t last_rental_result;                    /**< 最近租赁查询本地终态。 */
        bool pending_control;                                        /**< 是否存在唯一授权/解锁事务。 */
        char control_error_code[MAINTENANCE_ERROR_CODE_CAPACITY];    /**< CLOUD_/BLE_ 控制稳定码。 */
        watch_ble_diagnostics_t ble_diagnostics;                     /**< BLE 专用累计诊断按值快照。 */
        char ble_latest_error_code[MAINTENANCE_ERROR_CODE_CAPACITY]; /**< 不受全局优先级遮蔽的 BLE 最近错误。 */
        maintenance_capability_view_t gps;                           /**< GPS owner 可用性及 fixed 派生观测事实。 */
        maintenance_capability_view_t modem;                         /**< 4G 能力及 watch_state 事实。 */
        watch_modem_snapshot_t modem_diagnostics;                    /**< 4G 生命周期与脱敏诊断投影。 */
        maintenance_capability_view_t cloud_config;                  /**< 云配置真实可用性，不包含任何凭据。 */
        char watch_id[WATCH_STATE_WATCH_ID_CAPACITY];                /**< 工厂 base MAC 设备身份。 */
        watch_cloud_config_status_t cloud_status;                    /**< 云 configured/unconfigured/invalid 状态。 */
        watch_config_value_status_t base_url_status;                 /**< base URL 脱敏状态。 */
        watch_apn_mode_t apn_mode;                                   /**< APN auto/configured/invalid 状态。 */
        watch_config_value_status_t token_status;                    /**< token 脱敏状态。 */
        char token_mask[CONFIG_SERVICE_TOKEN_MASK_CAPACITY];         /**< token 固定脱敏文本。 */
        watch_config_value_status_t certificate_status;              /**< 证书策略脱敏状态。 */
        watch_config_schema_status_t config_schema_status;           /**< 配置 schema 状态。 */
        char cloud_error_code[MAINTENANCE_ERROR_CODE_CAPACITY];      /**< CLOUD_ 配置错误。 */
        char modem_error_code[MAINTENANCE_ERROR_CODE_CAPACITY];      /**< MODEM_ 最近错误。 */
        watch_telemetry_result_t last_telemetry_result;              /**< 最近 telemetry 有界终态。 */
        uint16_t last_telemetry_http_status;                         /**< 最近 telemetry HTTP 状态。 */
        uint64_t last_telemetry_attempt_monotonic_ms;                /**< 最近 telemetry 尝试的 boot 单调毫秒。 */
        int64_t last_telemetry_success_server_time_ms;               /**< 最近成功服务器 UTC 毫秒。 */
        int64_t cloud_time_offset_ms;                                /**< 当前 boot 云时间偏移。 */
        bool last_telemetry_success_server_time_valid;               /**< 最近成功服务器时间是否有效。 */
        bool cloud_time_synchronized;                                /**< 当前 boot 是否具有服务器时间证据。 */
        char telemetry_error_code[MAINTENANCE_ERROR_CODE_CAPACITY];  /**< 最近 telemetry CLOUD_ 稳定码。 */
        bool battery_valid;                                          /**< 最近电量读数是否有效。 */
        uint8_t battery_percent;                                     /**< 最近有效电量百分比。 */
        watch_audio_state_t audio_state;                             /**< 现有 typed 音频状态。 */
        watch_audio_error_t audio_error;                             /**< 现有 typed 音频错误。 */
        watch_selftest_summary_t selftest;                           /**< 现有最近自检摘要。 */
        bool has_latest_error;                                       /**< 是否存在可解释的 typed 稳定错误码。 */
        char latest_error_code[MAINTENANCE_ERROR_CODE_CAPACITY];     /**< 最近稳定错误码或 NONE。 */
        maintenance_ble_candidate_list_t ble_candidates;             /**< 当前候选会话按值快照。 */
        maintenance_ble_selection_feedback_t ble_selection_feedback; /**< ble_task 最终选择反馈。 */
    } maintenance_snapshot_t;

    /**
     * @brief 初始化维护 mutex、选择队列和瞬态候选状态
     * @details 不创建任务；重复调用幂等，半初始化失败会回收已创建资源。
     * @return ESP_OK 成功或已初始化，ESP_ERR_NO_MEM 资源创建失败
     */
    esp_err_t maintenance_service_init(void);

    /**
     * @brief 回收未由任务持有的维护契约资源
     * @return ESP_OK 已回收或原本未初始化
     */
    esp_err_t maintenance_service_deinit(void);

    /**
     * @brief 获取一次维护视图快照
     * @details 先复制维护瞬态状态并释放其 mutex，再独立读取一次 watch_state，避免嵌套锁。
     * Story 5.4 的完整维护 UI 应直接复用本快照，不复制第二套产品事实。
     * @param snapshot 输出的定长维护快照
     * @param timeout_ticks 每个 mutex 获取允许等待的有界 tick 数
     * @return ESP_OK 成功，其他值表示参数、初始化状态或 mutex 等待失败
     */
    esp_err_t maintenance_service_snapshot(maintenance_snapshot_t *snapshot,
                                           TickType_t timeout_ticks);

    /**
     * @brief 获取 UI 所需的 BLE 候选瞬态快照
     * @details 只读取维护 mutex 内的候选、反馈和版本，不重复读取 watch_state。
     * @param snapshot 输出的 BLE 候选瞬态快照
     * @param timeout_ticks 等待维护 mutex 的有界 tick 数
     * @return ESP_OK 成功，其他值表示参数、初始化状态或 mutex 等待失败
     */
    esp_err_t maintenance_service_ble_ui_snapshot(
        maintenance_ble_ui_snapshot_t *snapshot,
        TickType_t timeout_ticks);

    /**
     * @brief 通过唯一维护入口请求运行既有完整自检
     * @details Story 5.4 应继续复用此命令，不得由 UI 构造自检结果。
     * @return typed 维护结果，busy 与下游错误显式区分
     */
    maintenance_result_t maintenance_service_request_selftest(void);

    /**
     * @brief 通过唯一维护入口显式请求 4G 联网
     * @details 仅投递手动唤醒意图；真实联网、合并和退避仲裁均由 modem owner 完成。
     *          4G 能力关闭时固定返回 NOT_READY。
     * @return OK 已接受，NOT_READY 服务未初始化或固件已关闭 4G，其他值表示下游拒绝
     */
    maintenance_result_t maintenance_service_request_modem_connect(void);

    /**
     * @brief 通过唯一维护入口请求 GPS 搜星
     * @details 只向 GPS owner 零等待投递手动请求；不得直接操作 UART、WAKE 或接收机配置。
     * @return OK 已接受，BUSY 正在搜星或处于主动冷却，NOT_READY 服务未初始化
     */
    maintenance_result_t maintenance_service_request_gps_acquisition(void);

    /**
     * @brief 使用操作员确认的 GPS 环境请求运行完整自检
     * @param indoor_confirmed true 表示室内环境，GPS 定位项按前置条件记为 skip
     * @return typed 维护结果，busy 与下游错误显式区分
     */
    maintenance_result_t maintenance_service_request_selftest_with_gps_context(
        bool indoor_confirmed);

    /**
     * @brief 重试既有完整结果中的一个失败项
     * @param run_id 当前完整结果的非零运行 ID
     * @param item_id 当前结果中 outcome=FAIL 的项目 ID
     * @param indoor_confirmed GPS FIX 重试时的室内环境确认，其他项目忽略
     * @return typed 维护结果，stale/busy 与下游错误显式区分
     */
    maintenance_result_t maintenance_service_request_selftest_retry(
        uint32_t run_id,
        selftest_item_id_t item_id,
        bool indoor_confirmed);

    /**
     * @brief 发布一轮按值复制的 BLE 候选
     * @details 供 Story 2.1 的 ble_service 发布真实扫描结果；选择被接受前允许同代次刷新数量、名称与 RSSI。
     * @param generation 非零扫描代次；开始时可发布空快照，后续按同代次更新扫描结果
     * @param candidates 待校验并规范化的候选数组；空列表时必须为 NULL
     * @param candidate_count 候选数量，最大为 MAINTENANCE_BLE_CANDIDATE_MAX_COUNT
     * @param timeout_ticks 等待维护 mutex 的有界 tick 数
     * @return typed 维护结果
     */
    maintenance_result_t maintenance_service_publish_ble_candidates(
        uint32_t generation,
        const maintenance_ble_candidate_t candidates[],
        size_t candidate_count,
        TickType_t timeout_ticks);

    /**
     * @brief 原子关闭 BLE 候选会话并清空全部瞬态选择状态
     * @details 由 ble_task 在候选页离开、清绑或绑定提交后调用；同时清空候选、反馈、已接受代次和尚未消费的选择命令。
     * @param generation 非零的新代次，用于拒绝离页前的迟到命令和快照
     * @param timeout_ticks 等待维护 mutex 的有界 tick 数
     * @return OK、STALE、BUSY、INVALID_ARGUMENT 或 NOT_READY
     */
    maintenance_result_t maintenance_service_clear_ble_candidate_session(
        uint32_t generation,
        TickType_t timeout_ticks);

    /**
     * @brief 提交同代次、完整 MAC 的 BLE 候选选择
     * @details 成功仅表示 typed 命令已入队，不表示已连接或已绑定；Story 5.4 应复用此命令。
     * @param generation 页面渲染时取得的候选代次
     * @param mac 页面选择的完整 MAC 值
     * @param timeout_ticks 等待维护 mutex 的有界 tick 数
     * @return OK、BUSY、STALE、INVALID_ARGUMENT 或 NOT_READY
     */
    maintenance_result_t maintenance_service_select_ble_candidate(
        uint32_t generation,
        const char mac[MAINTENANCE_BLE_MAC_CAPACITY],
        TickType_t timeout_ticks);

    /**
     * @brief 接收下一条完整 BLE 选择命令
     * @details Story 2.1 的 ble_service 应直接消费该 typed 命令，不重建候选选择模型。
     * @param command 输出的代次与完整 MAC 值
     * @param timeout_ticks 等待选择命令的有界 tick 数
     * @return OK 已收到，EMPTY 超时或无命令，其他值表示参数或初始化状态错误
     */
    maintenance_result_t maintenance_service_receive_ble_selection(
        maintenance_ble_selection_command_t *command,
        TickType_t timeout_ticks);

    /**
     * @brief 发布 ble_task 对候选选择的最终处理结果
     * @details 所有结果必须匹配当前候选代次与 MAC；历史错误由 BLE 稳定诊断单独展示。
     * @param generation 结果所属候选代次
     * @param mac 结果所属规范完整 MAC
     * @param result CONNECTING、BUSY、STALE 或 ERROR
     * @param timeout_ticks 等待维护 mutex 的有界 tick 数
     * @return OK 已保存，STALE 代次已替换，其他值表示参数或资源错误
     */
    maintenance_result_t maintenance_service_publish_ble_selection_feedback(
        uint32_t generation,
        const char mac[MAINTENANCE_BLE_MAC_CAPACITY],
        maintenance_ble_selection_result_t result,
        TickType_t timeout_ticks);

    /**
     * @brief 从独立确认页投递 BLE 清除绑定命令
     * @param timeout_ticks 等待清除队列空间的有界 tick 数
     * @return OK 已入队，BUSY 队列满，NOT_READY 未初始化
     */
    maintenance_result_t maintenance_service_confirm_clear_ble_binding(
        TickType_t timeout_ticks);

    /**
     * @brief 接收下一条已确认 BLE 清除命令
     * @param command 确认值命令输出
     * @param timeout_ticks 等待命令的有界 tick 数
     * @return OK 已收到，EMPTY 无命令，其他值表示参数或状态错误
     */
    maintenance_result_t maintenance_service_receive_ble_clear(
        maintenance_ble_clear_command_t *command,
        TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_MAINTENANCE_SERVICE_H */
