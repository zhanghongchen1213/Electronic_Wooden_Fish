/**
 * @file     modem_service.h
 * @brief    ML307R 网络生命周期服务接口
 * @details  定义固定 modem_task owner 的有界状态、异步请求、自检和停止边界，不暴露 UART 或敏感凭据。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_MODEM_SERVICE_H
#define LEGBOT_MODEM_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"
#include "ml307r_https_transport.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 蜂窝模组服务在统一服务表中的固定 ID。 */
#define LEGBOT_MODEM_SERVICE_ID LEGBOT_SERVICE_MODEM
/** modem owner 单次循环最大 UART 等待。 */
#define MODEM_OWNER_POLL_MS 10U
/** ML307R 自检总预算。 */
#define MODEM_SELFTEST_TIMEOUT_MS 60000U
/** 自检波特率探测候选总数。 */
#define MODEM_BAUD_PROBE_CANDIDATE_COUNT 8U
/** 切换波特率后等待 UART 稳定的间隔。 */
#define MODEM_BAUD_PROBE_INTERVAL_MS 130U
/** IMEI/SN 有界诊断容量，包含字符串终止符。 */
#define MODEM_IDENTITY_CAPACITY 24U
/** 运营商有界诊断容量，包含字符串终止符。 */
#define MODEM_OPERATOR_CAPACITY 32U
/** PDP IP 有界诊断容量，兼容 IPv6 并包含终止符。 */
#define MODEM_IP_CAPACITY 48U
/** 稳定 MODEM_ 错误容量，包含字符串终止符。 */
#define MODEM_ERROR_CAPACITY 40U

    typedef enum
    {
        MODEM_LIFECYCLE_BOOTING = 0,    /**< 等待 MATREADY 或有界 AT 探测。 */
        MODEM_LIFECYCLE_SIM_CHECK,      /**< 查询 SIM、模组信息与信号。 */
        MODEM_LIFECYCLE_REGISTERING,    /**< 等待或查询网络驻网。 */
        MODEM_LIFECYCLE_PDP_READY,      /**< PDP/IP 已验证。 */
        MODEM_LIFECYCLE_HTTPS_READY,    /**< 原生 HTTPS transport 可接受请求。 */
        MODEM_LIFECYCLE_REQUEST_ACTIVE, /**< 单个 HTTPS 请求正在执行。 */
        MODEM_LIFECYCLE_RECOVERING,     /**< 有界退避或模组软恢复。 */
        MODEM_LIFECYCLE_BACKOFF,        /**< 射频已关闭并等待自动或显式唤醒。 */
    } modem_lifecycle_t;

    typedef enum
    {
        MODEM_CONNECT_TRIGGER_MANUAL = 0, /**< 维护页用户主动请求联网。 */
        MODEM_CONNECT_TRIGGER_PAYMENT,    /**< 支付验证要求立即建立网络。 */
        MODEM_CONNECT_TRIGGER_TELEMETRY,  /**< 三分钟 telemetry 窗口要求建立网络。 */
    } modem_connect_trigger_t;

    typedef enum
    {
        MODEM_NETWORK_LEASE_PAYMENT = 0, /**< 支付查询及当前已授权 BLE 链路在线期占用 RF。 */
        MODEM_NETWORK_LEASE_TELEMETRY,   /**< latest-only telemetry 窗口占用 RF。 */
        MODEM_NETWORK_LEASE_MANUAL,      /**< 维护页显式联网会话占用 RF。 */
    } modem_network_lease_t;

    typedef enum
    {
        MODEM_NETWORK_BOOTING = 0,    /**< 模组仍在启动。 */
        MODEM_NETWORK_SIM_MISSING,    /**< 未插 SIM。 */
        MODEM_NETWORK_SIM_NOT_READY,  /**< SIM 存在但未就绪。 */
        MODEM_NETWORK_NO_SIGNAL,      /**< 信号缺失或未知。 */
        MODEM_NETWORK_NOT_REGISTERED, /**< 尚未驻网。 */
        MODEM_NETWORK_PDP_FAILED,     /**< PDP/拨号失败。 */
        MODEM_NETWORK_ONLINE,         /**< PDP 与有效 IP 已确认。 */
        MODEM_NETWORK_REQUEST_FAILED, /**< 最近 HTTPS transport 失败。 */
        MODEM_NETWORK_REPORTING_OK,   /**< 最近 HTTPS 请求完成。 */
        MODEM_NETWORK_RECOVERING,     /**< 网络或模组正在恢复。 */
    } modem_network_status_t;

    typedef enum
    {
        MODEM_SIM_UNKNOWN = 0, /**< 尚未取得 SIM 事实。 */
        MODEM_SIM_MISSING,     /**< CPIN 响应表明无 SIM。 */
        MODEM_SIM_NOT_READY,   /**< SIM 未 READY。 */
        MODEM_SIM_READY,       /**< SIM 已 READY。 */
    } modem_sim_status_t;

    typedef enum
    {
        MODEM_SIGNAL_UNKNOWN = 0, /**< CSQ/CESQ 未取得或为 unknown。 */
        MODEM_SIGNAL_NONE,        /**< 3GPP 响应表示无信号。 */
        MODEM_SIGNAL_PRESENT,     /**< 原始信号值有效，不施加无证据弱信号阈值。 */
    } modem_signal_status_t;

    typedef enum
    {
        MODEM_REGISTRATION_UNKNOWN = 0, /**< 注册事实未知。 */
        MODEM_REGISTRATION_SEARCHING,   /**< 正在搜索或未驻网。 */
        MODEM_REGISTRATION_HOME,        /**< 已驻留本地网络。 */
        MODEM_REGISTRATION_ROAMING,     /**< 已漫游驻网。 */
        MODEM_REGISTRATION_DENIED,      /**< 注册被拒绝。 */
    } modem_registration_status_t;

    typedef enum
    {
        MODEM_REGISTRATION_SOURCE_NONE = 0, /**< 尚未取得注册域事实。 */
        MODEM_REGISTRATION_SOURCE_CGREG,    /**< 取得分组域 CGREG 回退事实。 */
        MODEM_REGISTRATION_SOURCE_CREG,     /**< 取得电路域 CREG 回退事实。 */
        MODEM_REGISTRATION_SOURCE_CEREG,    /**< 取得 LTE CEREG 主事实。 */
    } modem_registration_source_t;

    typedef enum
    {
        MODEM_PDP_DOWN = 0, /**< PDP 未激活或 IP 无效。 */
        MODEM_PDP_UP,       /**< PDP 已激活且地址有效。 */
    } modem_pdp_status_t;

    typedef struct
    {
        uint32_t sequence;                               /**< owner 单调状态序号。 */
        modem_lifecycle_t lifecycle;                     /**< 架构生命周期。 */
        modem_network_status_t network_status;           /**< 产品可见网络状态。 */
        modem_sim_status_t sim_status;                   /**< SIM 状态。 */
        modem_signal_status_t signal_status;             /**< 信号状态。 */
        modem_registration_status_t registration;        /**< 驻网状态。 */
        modem_registration_source_t registration_source; /**< 当前注册事实来源与优先级。 */
        modem_pdp_status_t pdp_status;                   /**< PDP/IP 状态。 */
        int16_t csq_rssi;                                /**< 原始 CSQ RSSI，unknown 使用 99。 */
        int16_t csq_ber;                                 /**< 原始 CSQ BER。 */
        int16_t cesq_rsrq;                               /**< 原始 CESQ RSRQ，unknown 使用 255。 */
        int16_t cesq_rsrp;                               /**< 原始 CESQ RSRP，unknown 使用 255。 */
        TickType_t updated_at_ticks;                     /**< 最近状态更新时间。 */
        TickType_t last_success_ticks;                   /**< 最近请求成功时间。 */
        TickType_t last_failure_ticks;                   /**< 最近失败时间。 */
        uint32_t recovery_count;                         /**< 恢复次数饱和计数。 */
        char imei[MODEM_IDENTITY_CAPACITY];              /**< 有界 IMEI 诊断，不作 watch_id。 */
        char serial_number[MODEM_IDENTITY_CAPACITY];     /**< 有界 SN 诊断。 */
        char operator_name[MODEM_OPERATOR_CAPACITY];     /**< 有界运营商诊断。 */
        char ip_address[MODEM_IP_CAPACITY];              /**< 已验证 IP 地址。 */
        char last_error[MODEM_ERROR_CAPACITY];           /**< 最近 MODEM_ 稳定错误。 */
    } modem_service_snapshot_t;

    typedef enum
    {
        MODEM_SELFTEST_PASS = 0, /**< AT、SIM、信号、驻网与 PDP 当前均通过。 */
        MODEM_SELFTEST_FAIL,     /**< owner 完成检查但存在明确失败。 */
        MODEM_SELFTEST_SKIP,     /**< modem task 未运行或前置不可用。 */
    } modem_selftest_outcome_t;

    typedef struct
    {
        uint32_t request_id;               /**< 自检请求 ID。 */
        modem_selftest_outcome_t outcome;  /**< pass/fail/skip。 */
        char reason[MODEM_ERROR_CAPACITY]; /**< 精确 MODEM_ 原因。 */
    } modem_selftest_result_t;

    /**
     * @brief 判断启动 AT 是否属于联网硬门槛
     * @param command 已终止的固定启动 AT 命令
     * @return true 失败时必须恢复，false 失败时允许继续取得联网硬事实
     */
    bool modem_service_init_command_is_connectivity_gate(
        const char *command);

    /**
     * @brief 初始化 modem 私有有界契约资源
     * @return ESP_OK 成功，其他值表示内存或 AT 核心状态错误
     */
    esp_err_t modem_service_prepare(void);

    /**
     * @brief 取消尚未启动的 prepared run
     */
    void modem_service_cancel_prepared_run(void);

    /**
     * @brief 请求 modem_task 立即停止并作废 active generation
     * @param timeout_ticks 等待 STOP 入队的有界 tick
     * @return ESP_OK 已入队，其他值表示状态或队列错误
     */
    esp_err_t modem_service_request_stop(TickType_t timeout_ticks);

    /**
     * @brief 请求 modem owner 立即开始或合并一次显式联网会话
     * @details 维护页触发自动建立 60 秒有界 MANUAL RF lease；支付与 telemetry 应优先使用显式 lease API。
     * @param trigger 维护页手动或支付业务触发来源
     * @return ESP_OK 请求已锁存，ESP_ERR_INVALID_ARG 来源非法，
     *         ESP_ERR_INVALID_STATE 服务未准备
     */
    esp_err_t modem_service_request_connect(
        modem_connect_trigger_t trigger);

    /**
     * @brief 获取一个按位合并的网络 RF lease 并触发所需联网
     * @param lease 支付、telemetry 或维护页 lease
     * @return ESP_OK 已获取，其他值表示参数或服务状态无效
     */
    esp_err_t modem_service_network_acquire(modem_network_lease_t lease);

    /**
     * @brief 释放一个网络 RF lease，最后一个 lease 释放后进入 CFUN=4
     * @details 同步撤销尚未消费的同源触发；若联网窗口已启动，owner 会有界取消在途 AT 后再确认无需求并关闭射频。
     * @param lease 已持有的 lease 类型
     * @return ESP_OK 已释放，其他值表示参数或服务状态无效
     */
    esp_err_t modem_service_network_release(modem_network_lease_t lease);

    /**
     * @brief 运行固定 modem_task owner 循环
     * @return ESP_OK 正常停止，其他值表示不可恢复的 owner 错误
     */
    esp_err_t modem_service_run(void);

    /**
     * @brief 将一条已解析 URC 应用到生命周期快照
     * @param snapshot owner 私有状态
     * @param line 已终止 URC 行
     * @param now_ticks 当前单调 tick
     * @return ESP_OK 已识别并应用，ESP_ERR_NOT_FOUND 表示未知 URC
     */
    esp_err_t modem_service_apply_urc(modem_service_snapshot_t *snapshot,
                                      const char *line,
                                      TickType_t now_ticks);

    /**
     * @brief 向 modem_task owner 提交单个异步 HTTPS POST
     * @param request 有界请求值
     * @param timeout_ticks 等待请求队列空间的有界 tick
     * @return ESP_OK 已入队，其他值表示参数、状态、同 endpoint 冲突或队列错误
     */
    esp_err_t modem_service_https_post_async(const ml307r_https_post_request_t *request,
                                             TickType_t timeout_ticks);

    /**
     * @brief 领取一个 HTTPS typed 终态
     * @param response 响应输出
     * @param timeout_ticks 有界等待 tick
     * @return ESP_OK 已领取，其他值表示参数、状态或超时
     */
    esp_err_t modem_service_https_receive(ml307r_https_response_t *response,
                                          TickType_t timeout_ticks);

    /**
     * @brief 请求取消指定 transport 请求，活动请求将立即进入有界清理
     * @param request_id 非零 transport 请求 ID
     * @return ESP_OK 已锁存，其他值表示参数或服务状态无效
     */
    esp_err_t modem_service_https_cancel(uint32_t request_id);

    /**
     * @brief 向 modem_task owner 发起 AT/SIM/信号/驻网/PDP typed 自检
     * @param request_id 调用方非零请求 ID
     * @return ESP_OK 已入队，ESP_ERR_NOT_FINISHED 表示 GPS 正占用 RF，
     *         其他值表示状态、忙或队列错误
     */
    esp_err_t modem_service_selftest_begin(uint32_t request_id);

    /**
     * @brief 取消指定的 modem owner 自检请求
     * @param request_id modem_service_selftest_begin() 使用的非零请求 ID
     * @return ESP_OK 已提交取消，ESP_ERR_NOT_FOUND 表示请求已结束或不存在
     *         ESP_ERR_INVALID_ARG 参数非法，ESP_ERR_INVALID_STATE 服务未运行
     */
    esp_err_t modem_service_selftest_cancel(uint32_t request_id);

    /**
     * @brief 有界领取 modem owner 自检终态
     * @param result 自检终态输出
     * @param timeout_ticks 单次等待 tick，调用方应小步轮询取消
     * @return ESP_OK 已取得，其他值表示参数、状态或本次等待超时
     */
    esp_err_t modem_service_selftest_receive(modem_selftest_result_t *result,
                                             TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_MODEM_SERVICE_H */
