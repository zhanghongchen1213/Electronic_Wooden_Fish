/**
 * @file     sync_service.h
 * @brief    Air780EGP 活动窗口同步服务接口。
 * @details  消费立即同步与 backlog；经 mock/真机传输上报契约 §6 JSON；
 *           确认只经 progress_service_advance_acked_total；立即同步终态经
 *           device_nav_service_complete_sync。ASCII TAG=SVC_SYNC。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_SYNC_SERVICE_H
#define EWF_SYNC_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"
#include "sync_https_codec.h"
#include "sync_response_policy.h"
#include "sync_window_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 同步服务在统一服务表中的固定 ID。 */
#define LEGBOT_SYNC_SERVICE_ID LEGBOT_SERVICE_SYNC

typedef enum {
    EWF_SYNC_FACT_LOCAL_RECORDED = 0, /**< 本地已记录。 */
    EWF_SYNC_FACT_SYNCING,            /**< 同步中。 */
    EWF_SYNC_FACT_SYNCED,             /**< 已同步。 */
    EWF_SYNC_FACT_PENDING_SYNC,       /**< 待同步。 */
    EWF_SYNC_FACT_SYNC_FAILED,        /**< 同步失败。 */
    EWF_SYNC_FACT_CONFLICT,           /**< 可恢复冲突。 */
} ewf_sync_fact_t;

typedef enum {
    EWF_SYNC_NETWORK_CONNECTED = 0,
    EWF_SYNC_NETWORK_NO_SIGNAL,
    EWF_SYNC_NETWORK_DISABLED,
} ewf_sync_network_mode_t;

typedef struct {
    ewf_sync_fact_t sync_fact;             /**< 累计同步事实。 */
    ewf_sync_network_mode_t network_mode;  /**< 4G 信号事实。 */
    uint8_t backoff_stage;                 /**< 当前退避档。 */
    uint32_t last_business_code;           /**< 最近业务码。 */
    uint32_t window_count;                 /**< 本启动周期开窗次数。 */
    bool conflict;                         /**< 是否处于可恢复冲突。 */
    bool identity_ready;                   /**< 身份是否足以开窗。 */
} ewf_sync_service_snapshot_t;

/**
 * @brief 初始化同步服务契约资源
 */
esp_err_t sync_service_init_contracts(void);

/**
 * @brief 进入 prepared（校验身份；缺省 fail-closed 不开窗假成功）
 */
esp_err_t sync_service_prepare_run(void);

/** @brief 放弃 prepared。 */
void sync_service_cancel_prepared_run(void);

/**
 * @brief 释放契约资源
 */
esp_err_t sync_service_deinit_contracts(void);

/**
 * @brief 进入同步任务循环（轮询 nav pending / backlog）
 */
esp_err_t sync_service_run(void);

/**
 * @brief 请求停止
 */
esp_err_t sync_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 读取可测快照（网络/同步事实）
 */
esp_err_t sync_service_snapshot(ewf_sync_service_snapshot_t *snapshot);

/**
 * @brief 主机/测试：强制调度一次活动窗口评估
 * @param now_ms 单调毫秒
 */
esp_err_t sync_service_poll_once(uint64_t now_ms);

/**
 * @brief 主机/测试：注入身份字段（空串 fail-closed）
 */
esp_err_t sync_service_set_identity(const char *device_id,
                                    const char *firmware_version,
                                    const char *scripture_version,
                                    uint32_t audio_config_version);

/**
 * @brief 只读拷贝身份字段供设置页展示（不伪造已绑定）
 * @param device_id 输出缓冲；可为 NULL
 * @param device_id_len 容量
 * @param firmware_version 输出缓冲；可为 NULL
 * @param firmware_version_len 容量
 * @param identity_configured 输出：是否足以开窗；可为 NULL
 */
esp_err_t sync_service_copy_identity(char *device_id,
                                     size_t device_id_len,
                                     char *firmware_version,
                                     size_t firmware_version_len,
                                     bool *identity_configured);

/**
 * @brief 注入电量百分比（0–100）
 * @details 有有效样本后清除 hardware_pending 占位；调用方不得伪装充电态。
 */
esp_err_t sync_service_set_battery_percent(uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif /* EWF_SYNC_SERVICE_H */
