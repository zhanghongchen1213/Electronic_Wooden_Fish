/**
 * @file     feedback_host.c
 * @brief    反馈服务主机集成测试的播放链/RGB/持久化替身。
 * @details  只替代 BSP、SPIFFS 与 NVS 介质边界：播放链替身镜像固件后端合同
 *           （音量 0 等价静音跳过播放序列；失败注入覆盖 codec 与功放域），
 *           记录调用序列供接线断言；音量替身以内存单副本模拟单事务提交。
 * @author   ZHC
 * @date     2026-09-23
 */

#include <stdbool.h>
#include <string.h>

#include "app_state.h"
#include "audio_volume_store.h"
#include "device_nav_service.h"
#include "device_settings_policy.h"
#include "esp_err.h"
#include "feedback_audio.h"
#include "feedback_policy.h"
#include "rgb_bsp.h"

/* ==================== 播放链替身 ==================== */

typedef enum
{
    HOST_AUDIO_CALL_NONE = 0,
    HOST_AUDIO_CALL_PLAY,
} host_audio_call_t;

static host_audio_call_t s_audio_calls[64];
static uint8_t s_audio_call_volumes[64];
static unsigned s_audio_call_count;
static bool s_audio_pa_off_called;
static bool s_audio_ready;
static bool s_fail_play_codec;
static bool s_fail_play_pa;
static watch_feedback_error_t s_typed_error;

void host_feedback_audio_reset(void)
{
    memset(s_audio_calls, 0, sizeof(s_audio_calls));
    memset(s_audio_call_volumes, 0, sizeof(s_audio_call_volumes));
    s_audio_call_count = 0U;
    s_audio_pa_off_called = false;
    s_audio_ready = false;
    s_fail_play_codec = false;
    s_fail_play_pa = false;
    s_typed_error = WATCH_FEEDBACK_ERROR_NONE;
}

void host_feedback_audio_fail_codec(bool fail) { s_fail_play_codec = fail; }
void host_feedback_audio_fail_pa(bool fail) { s_fail_play_pa = fail; }
unsigned host_feedback_audio_play_count(void) { return s_audio_call_count; }
uint8_t host_feedback_audio_played_volume(unsigned index)
{
    return index < s_audio_call_count ? s_audio_call_volumes[index] : 0U;
}
bool host_feedback_audio_pa_off_called(void) { return s_audio_pa_off_called; }

esp_err_t ewf_feedback_audio_backend_init(void)
{
    /* 镜像固件合同：资源/codec 就绪后 announce ready。 */
    s_audio_ready = true;
    return ESP_OK;
}

esp_err_t ewf_feedback_audio_backend_play(uint8_t volume_percent)
{
    if (!s_audio_ready)
    {
        s_typed_error = WATCH_FEEDBACK_ERROR_NOT_READY;
        return ESP_ERR_INVALID_STATE;
    }
    if (volume_percent == 0U)
    {
        /* 镜像固件合同：音量 0 等价静音，只安全关断功放，不进入播放序列。 */
        s_audio_pa_off_called = true;
        return ESP_OK;
    }
    if (s_audio_call_count < 64U)
    {
        s_audio_calls[s_audio_call_count] = HOST_AUDIO_CALL_PLAY;
        s_audio_call_volumes[s_audio_call_count] = volume_percent;
        ++s_audio_call_count;
    }
    if (s_fail_play_codec)
    {
        s_typed_error = WATCH_FEEDBACK_ERROR_CODEC_FAILED;
        return ESP_FAIL;
    }
    if (s_fail_play_pa)
    {
        s_typed_error = WATCH_FEEDBACK_ERROR_PA_FAILED;
        return ESP_FAIL;
    }
    s_typed_error = WATCH_FEEDBACK_ERROR_NONE;
    return ESP_OK;
}

esp_err_t ewf_feedback_audio_backend_ensure_pa_off(void)
{
    s_audio_pa_off_called = true;
    return ESP_OK;
}

esp_err_t ewf_feedback_audio_backend_last_error(void)
{
    return s_typed_error == WATCH_FEEDBACK_ERROR_NONE ? ESP_OK : ESP_FAIL;
}

watch_feedback_error_t ewf_feedback_audio_backend_typed_error(void)
{
    return s_typed_error;
}

/* ==================== RGB 替身 ==================== */

static unsigned s_rgb_flash_count;
static unsigned s_rgb_off_count;
static bool s_fail_rgb;

void host_rgb_reset(void)
{
    s_rgb_flash_count = 0U;
    s_rgb_off_count = 0U;
    s_fail_rgb = false;
}

void host_rgb_fail(bool fail) { s_fail_rgb = fail; }
unsigned host_rgb_flash_count(void) { return s_rgb_flash_count; }
unsigned host_rgb_off_count(void) { return s_rgb_off_count; }

esp_err_t rgb_bsp_init(void)
{
    return ESP_OK;
}

esp_err_t rgb_bsp_tap_flash(void)
{
    ++s_rgb_flash_count;
    return s_fail_rgb ? ESP_FAIL : ESP_OK;
}

esp_err_t rgb_bsp_off(void)
{
    ++s_rgb_off_count;
    return ESP_OK;
}

bool rgb_bsp_is_ready(void)
{
    return true;
}

const char *rgb_bsp_error_code(esp_err_t err)
{
    (void)err;
    return "HOST_RGB";
}

/* ==================== 音量持久化替身 ==================== */

static ewf_feedback_volume_record_t s_stored;
static bool s_has_stored;
static uint32_t s_save_count;
static bool s_fail_save;
static bool s_fail_load_media;

void host_audio_store_reset(void)
{
    memset(&s_stored, 0, sizeof(s_stored));
    s_has_stored = false;
    s_save_count = 0U;
    s_fail_save = false;
    s_fail_load_media = false;
}

void host_audio_store_fail_save(bool fail) { s_fail_save = fail; }
void host_audio_store_fail_load_media(bool fail) { s_fail_load_media = fail; }
uint32_t host_audio_store_save_count(void) { return s_save_count; }
bool host_audio_store_has_stored(void) { return s_has_stored; }
const ewf_feedback_volume_record_t *host_audio_store_stored(void)
{
    return &s_stored;
}

void host_audio_store_preload(uint8_t volume)
{
    s_stored.schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION;
    s_stored.volume = volume;
    s_has_stored = true;
}

esp_err_t ewf_audio_volume_store_load(ewf_feedback_volume_record_t *record,
                                      ewf_audio_store_status_t *status)
{
    if (record == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_load_media)
    {
        *status = EWF_AUDIO_STORE_ERROR;
        return ESP_OK;
    }
    if (!s_has_stored)
    {
        memset(record, 0, sizeof(*record));
        *status = EWF_AUDIO_STORE_EMPTY;
        return ESP_OK;
    }
    *record = s_stored;
    *status = EWF_AUDIO_STORE_OK;
    return ESP_OK;
}

esp_err_t ewf_audio_volume_store_save(const ewf_feedback_volume_record_t *record)
{
    if (record == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_save)
    {
        return ESP_FAIL;
    }
    s_stored = *record;
    s_has_stored = true;
    ++s_save_count;
    return ESP_OK;
}

/* ==================== 统一设置/导航音量替身（Story 2.4） ==================== */

static ewf_device_settings_record_t s_nav_settings;
static bool s_nav_settings_loaded;

static void ensure_nav_settings(void)
{
    if (s_nav_settings_loaded)
    {
        return;
    }
    ewf_device_settings_defaults(&s_nav_settings);
    if (s_has_stored)
    {
        s_nav_settings.volume = s_stored.volume;
        s_nav_settings.schema_version = s_stored.schema_version == 0U
                                            ? EWF_DEVICE_SETTINGS_SCHEMA_VERSION
                                            : s_stored.schema_version;
    }
    else if (!s_fail_load_media)
    {
        /* 镜像首启：默认音量落盘一次，供 host_audio_store 断言观测。 */
        ewf_feedback_volume_record_t initial = {
            .schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION,
            .volume = EWF_FEEDBACK_VOLUME_DEFAULT,
        };
        (void)ewf_audio_volume_store_save(&initial);
        s_nav_settings.volume = EWF_FEEDBACK_VOLUME_DEFAULT;
    }
    s_nav_settings_loaded = true;
}

esp_err_t device_nav_service_get_settings(ewf_device_settings_record_t *record)
{
    if (record == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_load_media)
    {
        return ESP_FAIL;
    }
    ensure_nav_settings();
    *record = s_nav_settings;
    return ESP_OK;
}

esp_err_t device_nav_service_set_volume(uint8_t volume, TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (!ewf_device_settings_volume_valid(volume))
    {
        return ESP_ERR_INVALID_ARG;
    }
    ensure_nav_settings();
    if (volume == s_nav_settings.volume && s_has_stored)
    {
        return ESP_OK;
    }
    ewf_feedback_volume_record_t record = {
        .schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION,
        .volume = volume,
    };
    const esp_err_t save_error = ewf_audio_volume_store_save(&record);
    if (save_error != ESP_OK)
    {
        return save_error;
    }
    s_nav_settings.volume = volume;
    return ESP_OK;
}

void host_nav_settings_reset(void)
{
    memset(&s_nav_settings, 0, sizeof(s_nav_settings));
    s_nav_settings_loaded = false;
}

/* feedback_service 查询 core_path 时需要落盘事实；运行时默认可注入。 */
static bool s_host_persist_pending;
static bool s_host_persist_inflight;
static bool s_host_persist_status_fail;

void host_feedback_set_persist_status(bool pending, bool inflight)
{
    s_host_persist_pending = pending;
    s_host_persist_inflight = inflight;
    s_host_persist_status_fail = false;
}

void host_feedback_fail_persist_status(bool fail)
{
    s_host_persist_status_fail = fail;
}

esp_err_t progress_service_get_persist_status(bool *persist_pending,
                                              bool *persist_inflight)
{
    if (persist_pending == NULL || persist_inflight == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_host_persist_status_fail)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *persist_pending = s_host_persist_pending;
    *persist_inflight = s_host_persist_inflight;
    return ESP_OK;
}

