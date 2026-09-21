/**
 * @file     time_service.h
 * @brief    启动周期内统一 UTC 时间服务接口
 * @details  以单调时钟偏移保存 GPS 或云端时间，不创建任务且不持久化到 NVS。
 * @author   ZHC
 * @date     2026-07-31
 */

#ifndef LEGBOT_TIME_SERVICE_H
#define LEGBOT_TIME_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/** JavaScript 安全整数范围内允许的最大 UTC 毫秒。 */
#define TIME_SERVICE_UTC_MAX_MS INT64_C(9007199254740991)

typedef enum
{
    TIME_SOURCE_NONE = 0, /**< 尚无可信时间。 */
    TIME_SOURCE_GPS,      /**< GPS RMC 提供的临时时间。 */
    TIME_SOURCE_CLOUD,    /**< 云端提供的最高优先级时间。 */
} time_service_source_t;

typedef struct
{
    bool valid;                     /**< 当前是否存在可信 UTC 偏移。 */
    time_service_source_t source;   /**< 当前时间来源。 */
    int64_t utc_offset_ms;          /**< UTC 毫秒减去单调毫秒。 */
    uint64_t sample_monotonic_ms;   /**< 最近接受样本的单调毫秒。 */
} time_service_snapshot_t;

/** @brief 清除当前启动周期时间事实。 */
void time_service_reset(void);

/**
 * @brief 提交一条 GPS 或云端 UTC 样本
 * @param source GPS 或 CLOUD 来源
 * @param utc_ms 样本对应 UTC 毫秒
 * @param monotonic_ms 样本产生时的启动单调毫秒
 * @return ESP_OK 已接受
 *         ESP_ERR_INVALID_ARG 样本非法
 *         ESP_ERR_INVALID_STATE 被更高优先级来源或更新样本拒绝
 */
esp_err_t time_service_submit_sample(time_service_source_t source,
                                     int64_t utc_ms,
                                     uint64_t monotonic_ms);

/**
 * @brief 读取一致的时间来源和偏移快照
 * @param snapshot 快照输出
 * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 参数无效
 */
esp_err_t time_service_get_snapshot(time_service_snapshot_t *snapshot);

/**
 * @brief 按指定单调时间计算当前 UTC
 * @param monotonic_ms 当前启动单调毫秒
 * @param utc_ms UTC 毫秒输出
 * @param source 可选的来源输出
 * @return ESP_OK 当前时间有效，ESP_ERR_NOT_FOUND 尚未同步
 */
esp_err_t time_service_now(uint64_t monotonic_ms,
                           int64_t *utc_ms,
                           time_service_source_t *source);

#endif /* LEGBOT_TIME_SERVICE_H */
