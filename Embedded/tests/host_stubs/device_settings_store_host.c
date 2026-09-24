/**
 * @file     device_settings_store_host.c
 * @brief    设备设置存储主机替身。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_settings_store.h"

#include <string.h>

static ewf_device_settings_record_t s_record;
static bool s_has;
static bool s_fail_save;
static bool s_fail_load;
static uint8_t s_legacy_audio_volume;
static bool s_has_legacy_audio;

void host_settings_store_reset(void)
{
    memset(&s_record, 0, sizeof(s_record));
    s_has = false;
    s_fail_save = false;
    s_fail_load = false;
    s_legacy_audio_volume = 0U;
    s_has_legacy_audio = false;
}

void host_settings_store_fail_save(bool fail) { s_fail_save = fail; }
void host_settings_store_fail_load(bool fail) { s_fail_load = fail; }
void host_settings_store_preload_legacy_audio(uint8_t volume)
{
    s_legacy_audio_volume = volume;
    s_has_legacy_audio = true;
}

bool host_settings_store_has(void) { return s_has; }
const ewf_device_settings_record_t *host_settings_store_record(void)
{
    return &s_record;
}

esp_err_t ewf_device_settings_store_load(ewf_device_settings_record_t *record,
                                         ewf_settings_store_status_t *status)
{
    if (record == NULL || status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_load) {
        *status = EWF_SETTINGS_STORE_ERROR;
        return ESP_OK;
    }
    if (!s_has) {
        ewf_device_settings_defaults(record);
        if (s_has_legacy_audio) {
            record->volume = s_legacy_audio_volume;
        }
        *status = EWF_SETTINGS_STORE_EMPTY;
        return ESP_OK;
    }
    *record = s_record;
    *status = EWF_SETTINGS_STORE_OK;
    return ESP_OK;
}

esp_err_t ewf_device_settings_store_save(
    const ewf_device_settings_record_t *record)
{
    if (record == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_save) {
        return ESP_FAIL;
    }
    s_record = *record;
    s_has = true;
    return ESP_OK;
}
