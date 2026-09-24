/**
 * @file     today_bucket_store.h
 * @brief    今日桶日键/计数持久化接口（与 progress 高水位事务分离）。
 * @details  固件侧 NVS 实现；主机测试由替身实现。失败只打中文日志，不阻断累计。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_TODAY_BUCKET_STORE_H
#define EWF_TODAY_BUCKET_STORE_H

#include "esp_err.h"
#include "today_bucket_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 加载今日桶；缺字段返回 EMPTY 语义（桶清零且 ESP_OK）
 * @param bucket 输出；NULL 返回 ESP_ERR_INVALID_ARG
 * @return ESP_OK 成功或空；其他值表示介质错误
 */
esp_err_t today_bucket_store_load(ewf_today_bucket_t *bucket);

/**
 * @brief 保存今日桶（日键 + 计数）
 * @param bucket 输入；NULL 返回 ESP_ERR_INVALID_ARG
 */
esp_err_t today_bucket_store_save(const ewf_today_bucket_t *bucket);

#ifdef __cplusplus
}
#endif

#endif /* EWF_TODAY_BUCKET_STORE_H */
