/**
 * @file     audio_volume_store_nvs.c
 * @brief    音量持久化的 NVS 实现。
 * @details  音量记录落在单一 NVS namespace "audio"，以单次 nvs_commit() 为事务
 *           边界：v5.5.4 官方文档明确 nvs_set_* 在 nvs_commit 之前不会更新实际
 *           存储，掉电时未提交写入整体丢失、已提交内容保持一致，满足契约 §11.1
 *           device 作用域设置存储的首块基线（Story 2.4 必须收敛到同一真源）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "audio_volume_store.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "SVC_FEEDBACK";

/** 音量唯一 NVS namespace；与 progress/config 多 namespace 先例并列。 */
#define AUDIO_VOLUME_NVS_NAMESPACE "audio"
/** 音量 schema 版本 key（15 字符 key 上限内的稳定命名）。 */
#define AUDIO_VOLUME_KEY_SCHEMA_VERSION "schema_ver"
/** 音量值 key。 */
#define AUDIO_VOLUME_KEY_VOLUME "volume"

esp_err_t ewf_audio_volume_store_load(ewf_feedback_volume_record_t *record,
                                      ewf_audio_store_status_t *status)
{
    if (record == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *status = EWF_AUDIO_STORE_ERROR;
    memset(record, 0, sizeof(*record));

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(AUDIO_VOLUME_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND)
    {
        /* namespace 尚未建立：这是首次启动的合法默认状态。 */
        *status = EWF_AUDIO_STORE_EMPTY;
        return ESP_OK;
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量 namespace 打开失败，错误=%s", esp_err_to_name(error));
        return ESP_OK;
    }

    uint32_t schema_version = 0U;
    uint32_t volume = 0U;
    error = nvs_get_u32(handle, AUDIO_VOLUME_KEY_SCHEMA_VERSION, &schema_version);
    if (error == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(handle);
        *status = EWF_AUDIO_STORE_EMPTY;
        return ESP_OK;
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量 schema 版本读取失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_OK;
    }
    error = nvs_get_u32(handle, AUDIO_VOLUME_KEY_VOLUME, &volume);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量值读取失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_OK;
    }
    record->schema_version = schema_version;
    record->volume = (uint8_t)volume;
    nvs_close(handle);
    *status = EWF_AUDIO_STORE_OK;
    return ESP_OK;
}

esp_err_t ewf_audio_volume_store_save(const ewf_feedback_volume_record_t *record)
{
    if (record == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(AUDIO_VOLUME_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量 namespace 写入打开失败，错误=%s", esp_err_to_name(error));
        return error;
    }

    /* 同一 namespace 内全部 set 之后单次 commit；任一 set 失败即放弃本次提交。 */
    error = nvs_set_u32(handle, AUDIO_VOLUME_KEY_SCHEMA_VERSION, record->schema_version);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量 schema 写入失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_FAIL;
    }
    error = nvs_set_u32(handle, AUDIO_VOLUME_KEY_VOLUME, record->volume);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量值写入失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_FAIL;
    }
    error = nvs_commit(handle);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "音量提交失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return error;
    }
    nvs_close(handle);
    return ESP_OK;
}
