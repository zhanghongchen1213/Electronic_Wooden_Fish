/**
 * @file     today_bucket_store_nvs.c
 * @brief    今日桶 NVS 持久化（namespace today_bkt）。
 * @details  与 progress 高水位事务分仓，避免污染 AD-15 单事务组语义。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "today_bucket_store.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "TODAY_BKT";

/** NVS 命名空间。 */
#define TODAY_NVS_NAMESPACE "today_bkt"
/** 日键键名。 */
#define TODAY_KEY_DAY "day_key"
/** 计数字段键名。 */
#define TODAY_KEY_COUNT "count"

esp_err_t today_bucket_store_load(ewf_today_bucket_t *bucket)
{
    if (bucket == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    bucket->day_key = 0U;
    bucket->count = 0U;

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(TODAY_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "今日桶打开失败：%s", esp_err_to_name(error));
        return error;
    }

    uint32_t day_key = 0U;
    uint32_t count = 0U;
    (void)nvs_get_u32(handle, TODAY_KEY_DAY, &day_key);
    (void)nvs_get_u32(handle, TODAY_KEY_COUNT, &count);
    nvs_close(handle);

    bucket->day_key = day_key;
    bucket->count = count;
    return ESP_OK;
}

esp_err_t today_bucket_store_save(const ewf_today_bucket_t *bucket)
{
    if (bucket == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(TODAY_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "今日桶写打开失败：%s", esp_err_to_name(error));
        return error;
    }

    error = nvs_set_u32(handle, TODAY_KEY_DAY, bucket->day_key);
    if (error == ESP_OK) {
        error = nvs_set_u32(handle, TODAY_KEY_COUNT, bucket->count);
    }
    if (error == ESP_OK) {
        error = nvs_commit(handle);
    }
    nvs_close(handle);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "今日桶落盘失败：%s", esp_err_to_name(error));
    }
    return error;
}
