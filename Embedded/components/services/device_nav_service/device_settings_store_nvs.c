/**
 * @file     device_settings_store_nvs.c
 * @brief    设备设置的 NVS 实现（含 audio 音量一次性迁移）。
 * @details  namespace "settings"；nvs_set_* 在 nvs_commit 前不落盘（ESP-IDF
 *           v5.5.4）。首启若 settings 空而 audio 有合法 volume，则迁移后写
 *           settings，废弃第二写入入口。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_settings_store.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "SVC_SETTINGS";

/** 设置唯一 NVS namespace（≤15）。 */
#define SETTINGS_NVS_NAMESPACE "settings"
#define SETTINGS_KEY_SCHEMA "schema_ver"
#define SETTINGS_KEY_VOLUME "volume"
#define SETTINGS_KEY_BRIGHTNESS "brightness"
#define SETTINGS_KEY_TIMEOUT "timeout"
#define SETTINGS_KEY_APPLIED_REV "applied_rev"

/** Story 2.3 遗留 audio namespace，仅作一次性迁移源。 */
#define LEGACY_AUDIO_NVS_NAMESPACE "audio"
#define LEGACY_AUDIO_KEY_SCHEMA "schema_ver"
#define LEGACY_AUDIO_KEY_VOLUME "volume"

static bool try_migrate_audio_volume(uint8_t *volume_out)
{
    nvs_handle_t handle = 0;
    esp_err_t error =
        nvs_open(LEGACY_AUDIO_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (error != ESP_OK) {
        return false;
    }
    uint32_t schema = 0U;
    uint32_t volume = 0U;
    error = nvs_get_u32(handle, LEGACY_AUDIO_KEY_SCHEMA, &schema);
    if (error != ESP_OK) {
        nvs_close(handle);
        return false;
    }
    error = nvs_get_u32(handle, LEGACY_AUDIO_KEY_VOLUME, &volume);
    nvs_close(handle);
    if (error != ESP_OK || volume > 100U) {
        return false;
    }
    *volume_out = (uint8_t)volume;
    ESP_LOGI(TAG, "已从 audio namespace 迁移音量=%u", (unsigned)*volume_out);
    return true;
}

esp_err_t ewf_device_settings_store_load(ewf_device_settings_record_t *record,
                                         ewf_settings_store_status_t *status)
{
    if (record == NULL || status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *status = EWF_SETTINGS_STORE_ERROR;
    memset(record, 0, sizeof(*record));

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        uint8_t migrated = 0U;
        ewf_device_settings_defaults(record);
        if (try_migrate_audio_volume(&migrated)) {
            record->volume = migrated;
        }
        *status = EWF_SETTINGS_STORE_EMPTY;
        return ESP_OK;
    }
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "设置 namespace 打开失败，错误=%s", esp_err_to_name(error));
        return ESP_OK;
    }

    uint32_t schema = 0U;
    uint32_t volume = 0U;
    uint32_t brightness = 0U;
    uint32_t timeout = 0U;
    uint32_t applied = 0U;
    error = nvs_get_u32(handle, SETTINGS_KEY_SCHEMA, &schema);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        ewf_device_settings_defaults(record);
        uint8_t migrated = 0U;
        if (try_migrate_audio_volume(&migrated)) {
            record->volume = migrated;
        }
        *status = EWF_SETTINGS_STORE_EMPTY;
        return ESP_OK;
    }
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "设置 schema 读取失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_OK;
    }
    if (nvs_get_u32(handle, SETTINGS_KEY_VOLUME, &volume) != ESP_OK ||
        nvs_get_u32(handle, SETTINGS_KEY_BRIGHTNESS, &brightness) != ESP_OK ||
        nvs_get_u32(handle, SETTINGS_KEY_TIMEOUT, &timeout) != ESP_OK ||
        nvs_get_u32(handle, SETTINGS_KEY_APPLIED_REV, &applied) != ESP_OK) {
        ESP_LOGE(TAG, "设置字段读取失败");
        nvs_close(handle);
        return ESP_OK;
    }
    nvs_close(handle);

    if (volume > 100U ||
        brightness >= (uint32_t)EWF_BRIGHTNESS_COUNT ||
        !ewf_device_settings_timeout_valid(timeout)) {
        ESP_LOGE(TAG, "设置介质值非法，拒绝恢复");
        *status = EWF_SETTINGS_STORE_ERROR;
        return ESP_OK;
    }
    record->schema_version = schema;
    record->volume = (uint8_t)volume;
    record->brightness = (ewf_brightness_t)brightness;
    record->timeout_s = timeout;
    record->applied_revision = applied;
    if (ewf_device_settings_validate(record) != EWF_SETTINGS_OK) {
        *status = EWF_SETTINGS_STORE_ERROR;
        return ESP_OK;
    }
    *status = EWF_SETTINGS_STORE_OK;
    return ESP_OK;
}

esp_err_t ewf_device_settings_store_save(
    const ewf_device_settings_record_t *record)
{
    if (record == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ewf_device_settings_validate(record) != EWF_SETTINGS_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle = 0;
    esp_err_t error =
        nvs_open(SETTINGS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "设置 namespace 写入打开失败，错误=%s",
                 esp_err_to_name(error));
        return error;
    }
    error = nvs_set_u32(handle, SETTINGS_KEY_SCHEMA, record->schema_version);
    if (error == ESP_OK) {
        error = nvs_set_u32(handle, SETTINGS_KEY_VOLUME, record->volume);
    }
    if (error == ESP_OK) {
        error = nvs_set_u32(handle, SETTINGS_KEY_BRIGHTNESS,
                            (uint32_t)record->brightness);
    }
    if (error == ESP_OK) {
        error = nvs_set_u32(handle, SETTINGS_KEY_TIMEOUT, record->timeout_s);
    }
    if (error == ESP_OK) {
        error = nvs_set_u32(handle, SETTINGS_KEY_APPLIED_REV,
                            record->applied_revision);
    }
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "设置字段写入失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_FAIL;
    }
    error = nvs_commit(handle);
    nvs_close(handle);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "设置提交失败，错误=%s", esp_err_to_name(error));
        return error;
    }
    return ESP_OK;
}
