/**
 * @file     audio_volume_store_nvs.c
 * @brief    音量存储兼容层：转发到统一 settings 存储（Story 2.4 收敛）。
 * @details  不再维护独立 audio namespace 写入入口；读路径仍可触发 settings
 *           加载（含一次性 audio 迁移）。新代码应调用 device_nav_service_set_volume。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "audio_volume_store.h"

#include "device_settings_store.h"
#include "esp_log.h"

static const char *TAG = "SVC_FEEDBACK";

esp_err_t ewf_audio_volume_store_load(ewf_feedback_volume_record_t *record,
                                      ewf_audio_store_status_t *status)
{
    if (record == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ewf_device_settings_record_t settings = {0};
    ewf_settings_store_status_t settings_status = EWF_SETTINGS_STORE_ERROR;
    const esp_err_t error =
        ewf_device_settings_store_load(&settings, &settings_status);
    if (error != ESP_OK)
    {
        return error;
    }
    if (settings_status == EWF_SETTINGS_STORE_EMPTY)
    {
        *status = EWF_AUDIO_STORE_EMPTY;
        record->schema_version = 0U;
        record->volume = settings.volume;
        return ESP_OK;
    }
    if (settings_status == EWF_SETTINGS_STORE_ERROR)
    {
        *status = EWF_AUDIO_STORE_ERROR;
        return ESP_OK;
    }
    record->schema_version = settings.schema_version;
    record->volume = settings.volume;
    *status = EWF_AUDIO_STORE_OK;
    return ESP_OK;
}

esp_err_t ewf_audio_volume_store_save(const ewf_feedback_volume_record_t *record)
{
    if (record == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    (void)record;
    ESP_LOGW(TAG,
             "audio_volume_store_save 已废弃：请改用 device_nav_service_set_volume");
    /* 禁止第二写入入口改写 settings，避免绕过导航内存态并冲掉其它字段。 */
    return ESP_ERR_NOT_SUPPORTED;
}
