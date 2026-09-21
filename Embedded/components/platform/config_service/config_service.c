/**
 * @file     config_service.c
 * @brief    设备身份与本地配置唯一所有者实现
 * @details  独占身份、cloud/cfg 与 ble NVS schema，发布无凭据快照并提供窄凭据复制边界。
 * @author   ZHC
 * @date     2026-07-13
 */

#include "config_service.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/semphr.h"
#include "nvs.h"

static const char *TAG = "PLAT_CFG";

/** 配置快照互斥等待预算。 */
#define CONFIG_SERVICE_LOCK_TIMEOUT_MS 50U
/** ESP32-S3 工厂 MAC 字节数。 */
#define CONFIG_SERVICE_FACTORY_MAC_LENGTH 6U
/** v1 合同 token，仅由配置 owner 持有。 */
#define CONFIG_SERVICE_V1_TOKEN "xianli_freedom_2189"
/** 配置快照互斥锁。 */
static SemaphoreHandle_t s_config_mutex;
/** 最近一次完整加载或提交的无凭据配置快照。 */
static config_service_snapshot_t s_snapshot;
/** 仅配置 owner 持有的云凭据。 */
static config_service_cloud_credentials_t s_credentials;
/** 云配置提交期间受 mutex 保护的模块级候选值，避免占用 main_task 大块栈。 */
static config_service_cloud_credentials_t s_candidate_credentials;
/** 配置 owner 是否已初始化。 */
static bool s_initialized;
/** 配置成功提交后的无凭据真值回调。 */
static config_service_change_callback_t s_change_callback;
/** 配置真值回调上下文。 */
static void *s_change_callback_context;
/** 尚未被上层 typed-state mailbox 接受的最新无凭据快照。 */
static config_service_snapshot_t s_pending_change_snapshot;
/** 是否存在待重投的配置真值。 */
static bool s_change_pending;
/** 启动阶段从 NVS 读入的云时间偏移 RAM 缓存。 */
static int64_t s_cloud_offset_ms;
/** 云时间偏移 key 是否存在且类型合法。 */
static bool s_cloud_offset_present;

static bool normalize_mac(const char input[CONFIG_SERVICE_MAC_CAPACITY],
                          char output[CONFIG_SERVICE_MAC_CAPACITY]);
static esp_err_t load_binding_from_nvs(config_service_snapshot_t *snapshot);
static esp_err_t load_schema_from_nvs(config_service_snapshot_t *snapshot);
static esp_err_t load_cloud_from_nvs(config_service_snapshot_t *snapshot,
                                     config_service_cloud_credentials_t *credentials);
static esp_err_t load_optional_string(nvs_handle_t handle,
                                      const char *key,
                                      char *destination,
                                      size_t capacity,
                                      bool *present,
                                      bool *invalid);
static bool base_url_is_valid(const char *value, size_t capacity);
static bool apn_is_valid(const char *value, size_t capacity);
static bool ca_ref_is_valid(const char *value, size_t capacity);
static esp_err_t validate_ca_pem(const char *value, size_t capacity);
static void classify_cloud(config_service_snapshot_t *snapshot,
                           config_service_cloud_credentials_t *credentials,
                           bool base_present,
                           bool base_invalid,
                           bool apn_present,
                           bool apn_invalid,
                           bool token_present,
                           bool token_invalid,
                           bool ca_ref_present,
                           bool ca_ref_invalid,
                           bool ca_pem_present,
                           bool ca_pem_invalid);
static void secure_zero(void *buffer, size_t length);
static esp_err_t set_or_erase_string(nvs_handle_t handle,
                                     const char *key,
                                     const char *value);
static esp_err_t write_cloud_candidate(
    nvs_handle_t handle,
    const config_service_cloud_credentials_t *credentials,
    uint16_t epoch);
static void load_cloud_offset_cache(void);

esp_err_t config_service_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }
    if (s_config_mutex == NULL)
    {
        s_config_mutex = xSemaphoreCreateMutex();
        if (s_config_mutex == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }

    config_service_snapshot_t loaded = {0};
    /* CA PEM 使凭据结构达到 2400 字节，启动阶段直接使用 owner 存储，避免压垮 main_task 栈。 */
    secure_zero(&s_credentials, sizeof(s_credentials));
    uint8_t factory_mac[CONFIG_SERVICE_FACTORY_MAC_LENGTH] = {0};
    esp_err_t err = esp_efuse_mac_get_default(factory_mac);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "工厂 MAC 读取失败，云配置保持不可用，错误=0x%x", (unsigned)err);
        return err;
    }
    load_cloud_offset_cache();
    const int written = snprintf(loaded.watch_id,
                                 sizeof(loaded.watch_id),
                                 "%02X:%02X:%02X:%02X:%02X:%02X",
                                 factory_mac[0], factory_mac[1], factory_mac[2],
                                 factory_mac[3], factory_mac[4], factory_mac[5]);
    if (written != (int)(CONFIG_SERVICE_WATCH_ID_CAPACITY - 1U))
    {
        return ESP_FAIL;
    }

    err = load_schema_from_nvs(&loaded);
    if (err == ESP_OK)
    {
        err = load_cloud_from_nvs(&loaded, &s_credentials);
    }
    if (err == ESP_OK)
    {
        err = load_binding_from_nvs(&loaded);
    }
    if (err != ESP_OK)
    {
        secure_zero(&s_credentials, sizeof(s_credentials));
        ESP_LOGE(TAG, "配置读取发生真实 I/O 失败，错误=0x%x", (unsigned)err);
        return err;
    }

    loaded.cloud_error = config_service_cloud_preflight(&loaded);
    loaded.cloud_configured = loaded.cloud_error == CLOUD_ERROR_NONE;
    loaded.config_revision = 1U;
    s_snapshot = loaded;
    s_initialized = true;

    if (loaded.status == CONFIG_SERVICE_BINDING_CORRUPT)
    {
        ESP_LOGE(TAG, "BLE 绑定配置格式损坏，已拒绝使用");
    }
    else if (loaded.has_bound_exoskeleton)
    {
        ESP_LOGI(TAG, "BLE 绑定配置已加载，MAC=%s", loaded.bound_exoskeleton_mac);
    }
    else
    {
        ESP_LOGI(TAG, "BLE 绑定配置不存在，当前未绑定");
    }
    ESP_LOGI(TAG,
             "设备身份与云配置已加载，watch_id=%s，云状态=%s",
             loaded.watch_id,
             config_service_cloud_error_code(loaded.cloud_error));
    return ESP_OK;
}

esp_err_t config_service_snapshot(config_service_snapshot_t *snapshot,
                                  TickType_t timeout_ticks)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *snapshot = s_snapshot;
    xSemaphoreGive(s_config_mutex);
    return ESP_OK;
}

esp_err_t config_service_set_change_callback(
    config_service_change_callback_t callback,
    void *context)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex,
                       pdMS_TO_TICKS(CONFIG_SERVICE_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    s_change_callback = callback;
    s_change_callback_context = callback == NULL ? NULL : context;
    xSemaphoreGive(s_config_mutex);
    if (callback != NULL)
    {
        const esp_err_t notify_error = config_service_retry_pending_change();
        if (notify_error != ESP_OK && notify_error != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGW(TAG, "配置真值回调已注册，待通知快照将在 state_task 中重试");
        }
    }
    return ESP_OK;
}

esp_err_t config_service_retry_pending_change(void)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex,
                       pdMS_TO_TICKS(CONFIG_SERVICE_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!s_change_pending)
    {
        xSemaphoreGive(s_config_mutex);
        return ESP_OK;
    }
    if (s_change_callback == NULL)
    {
        xSemaphoreGive(s_config_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    const config_service_snapshot_t pending = s_pending_change_snapshot;
    config_service_change_callback_t callback = s_change_callback;
    void *context = s_change_callback_context;
    xSemaphoreGive(s_config_mutex);

    const esp_err_t notify_error = callback(&pending, context);
    if (notify_error != ESP_OK)
    {
        return notify_error;
    }
    if (xSemaphoreTake(s_config_mutex,
                       pdMS_TO_TICKS(CONFIG_SERVICE_LOCK_TIMEOUT_MS)) == pdTRUE)
    {
        if (s_change_pending &&
            memcmp(&s_pending_change_snapshot, &pending, sizeof(pending)) == 0)
        {
            s_change_pending = false;
            memset(&s_pending_change_snapshot, 0, sizeof(s_pending_change_snapshot));
        }
        xSemaphoreGive(s_config_mutex);
    }
    return ESP_OK;
}

esp_err_t config_service_set_bound_exoskeleton_mac(
    const char mac[CONFIG_SERVICE_MAC_CAPACITY])
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    char normalized[CONFIG_SERVICE_MAC_CAPACITY] = {0};
    if (!normalize_mac(mac, normalized))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_config_mutex,
                       pdMS_TO_TICKS(CONFIG_SERVICE_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_BLE_NAMESPACE, NVS_READWRITE, &handle);
    const bool opened = err == ESP_OK;
    if (err == ESP_OK)
    {
        err = nvs_set_str(handle, CONFIG_SERVICE_BOUND_EXO_KEY, normalized);
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    if (opened)
    {
        nvs_close(handle);
    }
    if (err == ESP_OK)
    {
        s_snapshot.status = CONFIG_SERVICE_BINDING_LOADED;
        s_snapshot.has_bound_exoskeleton = true;
        memcpy(s_snapshot.bound_exoskeleton_mac,
               normalized,
               sizeof(s_snapshot.bound_exoskeleton_mac));
    }
    xSemaphoreGive(s_config_mutex);
    return err;
}

esp_err_t config_service_clear_bound_exoskeleton_mac(void)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex,
                       pdMS_TO_TICKS(CONFIG_SERVICE_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_BLE_NAMESPACE, NVS_READWRITE, &handle);
    const bool opened = err == ESP_OK;
    if (err == ESP_OK)
    {
        err = nvs_erase_key(handle, CONFIG_SERVICE_BOUND_EXO_KEY);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = ESP_OK;
        }
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    if (opened)
    {
        nvs_close(handle);
    }
    if (err == ESP_OK)
    {
        s_snapshot.status = CONFIG_SERVICE_BINDING_UNBOUND;
        s_snapshot.has_bound_exoskeleton = false;
        memset(s_snapshot.bound_exoskeleton_mac,
               0,
               sizeof(s_snapshot.bound_exoskeleton_mac));
    }
    xSemaphoreGive(s_config_mutex);
    return err;
}

esp_err_t config_service_set_cloud_credentials(
    const config_service_cloud_credentials_t *credentials)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (credentials == NULL ||
        strnlen(credentials->base_url, sizeof(credentials->base_url)) >=
            sizeof(credentials->base_url) ||
        strnlen(credentials->apn, sizeof(credentials->apn)) >= sizeof(credentials->apn) ||
        strnlen(credentials->token, sizeof(credentials->token)) >= sizeof(credentials->token) ||
        strnlen(credentials->ca_ref, sizeof(credentials->ca_ref)) >= sizeof(credentials->ca_ref) ||
        strnlen(credentials->ca_pem, sizeof(credentials->ca_pem)) >= sizeof(credentials->ca_pem) ||
        (credentials->base_url[0] != '\0' &&
         !base_url_is_valid(credentials->base_url, sizeof(credentials->base_url))) ||
        !apn_is_valid(credentials->apn, sizeof(credentials->apn)) ||
        (credentials->token[0] != '\0' &&
         strcmp(credentials->token, CONFIG_SERVICE_V1_TOKEN) != 0) ||
        (credentials->ca_ref[0] != '\0' && credentials->ca_pem[0] != '\0') ||
        (credentials->ca_ref[0] != '\0' &&
         !ca_ref_is_valid(credentials->ca_ref, sizeof(credentials->ca_ref))))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (credentials->ca_pem[0] != '\0')
    {
        const esp_err_t pem_error = validate_ca_pem(credentials->ca_pem,
                                                    sizeof(credentials->ca_pem));
        if (pem_error != ESP_OK)
        {
            return pem_error;
        }
    }

    if (xSemaphoreTake(s_config_mutex,
                       pdMS_TO_TICKS(CONFIG_SERVICE_LOCK_TIMEOUT_MS)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    secure_zero(&s_candidate_credentials, sizeof(s_candidate_credentials));
    s_candidate_credentials = *credentials;
    if (s_candidate_credentials.token[0] == '\0')
    {
        memcpy(s_candidate_credentials.token,
               CONFIG_SERVICE_V1_TOKEN,
               sizeof(CONFIG_SERVICE_V1_TOKEN));
    }

    nvs_handle_t cloud_handle = 0;
    nvs_handle_t config_handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_CLOUD_NAMESPACE, NVS_READWRITE, &cloud_handle);
    const bool cloud_opened = err == ESP_OK;
    if (err == ESP_OK)
    {
        err = nvs_open(CONFIG_SERVICE_CONFIG_NAMESPACE, NVS_READWRITE, &config_handle);
    }
    const bool config_opened = err == ESP_OK;
    const uint16_t next_epoch = (uint16_t)(s_snapshot.config_epoch + 1U);
    bool cloud_committed = false;
    if (err == ESP_OK)
    {
        if (next_epoch == 0U)
        {
            err = ESP_ERR_INVALID_STATE;
        }
        else
        {
            err = write_cloud_candidate(cloud_handle, credentials, next_epoch);
        }
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u16(config_handle,
                          CONFIG_SERVICE_SCHEMA_VERSION_KEY,
                          CONFIG_SERVICE_SCHEMA_VERSION);
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(cloud_handle);
        cloud_committed = err == ESP_OK;
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u16(config_handle,
                          CONFIG_SERVICE_CONFIG_EPOCH_KEY,
                          next_epoch);
    }
    if (err == ESP_OK)
    {
        /* cfg epoch 最后激活；此前失败时新 cloud 代次在重启后保持 fail-closed。 */
        err = nvs_commit(config_handle);
    }
    if (err != ESP_OK && cloud_committed)
    {
        /* 激活失败时尽力恢复上一份完整 cloud 值；回滚失败仍由 epoch mismatch 保持 fail-closed。 */
        esp_err_t rollback_error = write_cloud_candidate(cloud_handle,
                                                         &s_credentials,
                                                         s_snapshot.config_epoch);
        if (rollback_error == ESP_OK)
        {
            rollback_error = nvs_commit(cloud_handle);
        }
        if (rollback_error != ESP_OK)
        {
            ESP_LOGE(TAG, "云配置激活失败且旧值回滚失败，重启后将保持不可用");
        }
    }
    if (cloud_opened)
    {
        nvs_close(cloud_handle);
    }
    if (config_opened)
    {
        nvs_close(config_handle);
    }

    if (err == ESP_OK)
    {
        config_service_snapshot_t next_snapshot = s_snapshot;
        next_snapshot.schema_version = CONFIG_SERVICE_SCHEMA_VERSION;
        next_snapshot.config_epoch = (uint16_t)(s_snapshot.config_epoch + 1U);
        next_snapshot.schema_status = CONFIG_SERVICE_SCHEMA_VALID;
        if (next_snapshot.config_revision < UINT32_MAX)
        {
            ++next_snapshot.config_revision;
        }
        classify_cloud(&next_snapshot,
                       &s_candidate_credentials,
                       credentials->base_url[0] != '\0', false,
                       credentials->apn[0] != '\0', false,
                       credentials->token[0] != '\0', false,
                       credentials->ca_ref[0] != '\0', false,
                       credentials->ca_pem[0] != '\0', false);
        next_snapshot.cloud_error = config_service_cloud_preflight(&next_snapshot);
        next_snapshot.cloud_configured = next_snapshot.cloud_error == CLOUD_ERROR_NONE;
        s_credentials = s_candidate_credentials;
        s_snapshot = next_snapshot;
        s_pending_change_snapshot = next_snapshot;
        s_change_pending = true;
    }
    secure_zero(&s_candidate_credentials, sizeof(s_candidate_credentials));
    xSemaphoreGive(s_config_mutex);
    if (err == ESP_OK)
    {
        const esp_err_t notify_error = config_service_retry_pending_change();
        if (notify_error != ESP_OK && notify_error != ESP_ERR_INVALID_STATE)
        {
            ESP_LOGW(TAG, "云配置已提交，typed 状态通知暂未送达并已保留重试");
        }
    }
    return err;
}

cloud_config_error_t config_service_cloud_preflight(
    const config_service_snapshot_t *snapshot)
{
    if (snapshot == NULL || !normalize_mac(snapshot->watch_id, (char[CONFIG_SERVICE_MAC_CAPACITY]){0}))
    {
        return CLOUD_WATCH_ID_UNAVAILABLE;
    }
    if (snapshot->schema_status != CONFIG_SERVICE_SCHEMA_VALID ||
        snapshot->schema_version != CONFIG_SERVICE_SCHEMA_VERSION)
    {
        return CLOUD_SCHEMA_CORRUPT;
    }
    if (snapshot->cloud_base_url_status == CONFIG_SERVICE_VALUE_UNCONFIGURED)
    {
        return CLOUD_BASE_URL_UNCONFIGURED;
    }
    if (snapshot->cloud_base_url_status != CONFIG_SERVICE_VALUE_CONFIGURED)
    {
        return CLOUD_BASE_URL_INVALID;
    }
    if (snapshot->apn_mode == CONFIG_SERVICE_APN_INVALID)
    {
        return CLOUD_APN_INVALID;
    }
    if (snapshot->token_status == CONFIG_SERVICE_VALUE_UNCONFIGURED)
    {
        return CLOUD_TOKEN_UNCONFIGURED;
    }
    if (snapshot->token_status != CONFIG_SERVICE_VALUE_CONFIGURED)
    {
        return CLOUD_TOKEN_INVALID;
    }
    if (snapshot->certificate_status == CONFIG_SERVICE_VALUE_INVALID)
    {
        return CLOUD_CERT_INVALID;
    }
    return CLOUD_ERROR_NONE;
}

const char *config_service_cloud_error_code(cloud_config_error_t error)
{
    static const char *const codes[] = {
        [CLOUD_ERROR_NONE] = "CLOUD_OK",
        [CLOUD_WATCH_ID_UNAVAILABLE] = "CLOUD_WATCH_ID_UNAVAILABLE",
        [CLOUD_SCHEMA_CORRUPT] = "CLOUD_SCHEMA_CORRUPT",
        [CLOUD_BASE_URL_UNCONFIGURED] = "CLOUD_BASE_URL_UNCONFIGURED",
        [CLOUD_BASE_URL_INVALID] = "CLOUD_BASE_URL_INVALID",
        [CLOUD_APN_INVALID] = "CLOUD_APN_INVALID",
        [CLOUD_TOKEN_UNCONFIGURED] = "CLOUD_TOKEN_UNCONFIGURED",
        [CLOUD_TOKEN_INVALID] = "CLOUD_TOKEN_INVALID",
        [CLOUD_CERT_UNCONFIGURED] = "CLOUD_CERT_UNCONFIGURED",
        [CLOUD_CERT_INVALID] = "CLOUD_CERT_INVALID",
    };
    return error >= CLOUD_ERROR_NONE && error <= CLOUD_CERT_INVALID
               ? codes[error]
               : "CLOUD_CONFIG_UNKNOWN";
}

esp_err_t config_service_cloud_credentials_copy(
    config_service_cloud_credentials_t *credentials,
    TickType_t timeout_ticks)
{
    if (credentials == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    secure_zero(credentials, sizeof(*credentials));
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (config_service_cloud_preflight(&s_snapshot) != CLOUD_ERROR_NONE)
    {
        xSemaphoreGive(s_config_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    *credentials = s_credentials;
    xSemaphoreGive(s_config_mutex);
    return ESP_OK;
}

esp_err_t config_service_cloud_offset_read(int64_t *offset_ms,
                                           TickType_t timeout_ticks)
{
    if (offset_ms == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *offset_ms = 0;
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *offset_ms = s_cloud_offset_ms;
    const esp_err_t err = s_cloud_offset_present
                              ? ESP_OK
                              : ESP_ERR_NVS_NOT_FOUND;
    xSemaphoreGive(s_config_mutex);
    return err;
}

esp_err_t config_service_cloud_offset_write(int64_t offset_ms,
                                            TickType_t timeout_ticks)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_TIME_NAMESPACE,
                             NVS_READWRITE,
                             &handle);
    if (err == ESP_OK)
    {
        err = nvs_set_i64(handle,
                          CONFIG_SERVICE_CLOUD_OFFSET_KEY,
                          offset_ms);
        if (err == ESP_OK)
        {
            err = nvs_commit(handle);
        }
        nvs_close(handle);
    }
    if (err == ESP_OK)
    {
        s_cloud_offset_ms = offset_ms;
        s_cloud_offset_present = true;
    }
    xSemaphoreGive(s_config_mutex);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "云时间偏移 NVS 事务失败，错误=0x%x", (unsigned)err);
    }
    return err;
}

esp_err_t config_service_ui_preferences_read(
    config_service_ui_preferences_t *preferences,
    TickType_t timeout_ticks)
{
    if (preferences == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    preferences->haptics_enabled =
        CONFIG_SERVICE_UI_DEFAULT_HAPTICS_ENABLED;
    preferences->click_audio_enabled =
        CONFIG_SERVICE_UI_DEFAULT_CLICK_AUDIO_ENABLED;
    preferences->raise_to_wake_enabled =
        CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED;
    preferences->brightness = CONFIG_SERVICE_UI_DEFAULT_BRIGHTNESS;
    preferences->screen_timeout = CONFIG_SERVICE_UI_DEFAULT_SCREEN_TIMEOUT;
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_UI_NAMESPACE,
                             NVS_READONLY,
                             &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = ESP_OK;
    }
    else if (err == ESP_OK)
    {
        uint8_t haptics =
            CONFIG_SERVICE_UI_DEFAULT_HAPTICS_ENABLED ? 1U : 0U;
        uint8_t click_audio =
            CONFIG_SERVICE_UI_DEFAULT_CLICK_AUDIO_ENABLED ? 1U : 0U;
        uint8_t raise_wake =
            CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED ? 1U : 0U;
        uint8_t brightness =
            (uint8_t)CONFIG_SERVICE_UI_DEFAULT_BRIGHTNESS;
        uint8_t screen_timeout =
            (uint8_t)CONFIG_SERVICE_UI_DEFAULT_SCREEN_TIMEOUT;
        esp_err_t haptics_error = nvs_get_u8(handle,
                                             CONFIG_SERVICE_UI_HAPTICS_KEY,
                                             &haptics);
        esp_err_t click_audio_error = nvs_get_u8(
            handle,
            CONFIG_SERVICE_UI_CLICK_AUDIO_KEY,
            &click_audio);
        esp_err_t raise_wake_error = nvs_get_u8(
            handle,
            CONFIG_SERVICE_UI_RAISE_WAKE_KEY,
            &raise_wake);
        esp_err_t brightness_error = nvs_get_u8(
            handle,
            CONFIG_SERVICE_UI_BRIGHTNESS_KEY,
            &brightness);
        esp_err_t screen_timeout_error = nvs_get_u8(
            handle,
            CONFIG_SERVICE_UI_SCREEN_TIMEOUT_KEY,
            &screen_timeout);
        if (haptics_error == ESP_ERR_NVS_NOT_FOUND)
        {
            haptics_error = ESP_OK;
        }
        if (click_audio_error == ESP_ERR_NVS_NOT_FOUND)
        {
            click_audio_error = ESP_OK;
        }
        if (raise_wake_error == ESP_ERR_NVS_NOT_FOUND)
        {
            raise_wake_error = ESP_OK;
        }
        if (brightness_error == ESP_ERR_NVS_NOT_FOUND)
        {
            brightness_error = ESP_OK;
        }
        if (screen_timeout_error == ESP_ERR_NVS_NOT_FOUND)
        {
            screen_timeout_error = ESP_OK;
        }
        if (haptics_error != ESP_OK)
        {
            err = haptics_error;
        }
        else if (click_audio_error != ESP_OK)
        {
            err = click_audio_error;
        }
        else if (raise_wake_error != ESP_OK)
        {
            err = raise_wake_error;
        }
        else if (brightness_error != ESP_OK)
        {
            err = brightness_error;
        }
        else if (screen_timeout_error != ESP_OK)
        {
            err = screen_timeout_error;
        }
        else if (haptics > 1U ||
                 click_audio > 1U ||
                 raise_wake > 1U ||
                 brightness >= (uint8_t)CONFIG_SERVICE_UI_BRIGHTNESS_COUNT ||
                 screen_timeout >= (uint8_t)CONFIG_SERVICE_UI_SCREEN_TIMEOUT_COUNT)
        {
            err = ESP_ERR_INVALID_STATE;
        }
        else
        {
            preferences->haptics_enabled = haptics != 0U;
            preferences->click_audio_enabled = click_audio != 0U;
            preferences->raise_to_wake_enabled = raise_wake != 0U;
            preferences->brightness =
                (config_service_ui_brightness_t)brightness;
            preferences->screen_timeout =
                (config_service_ui_screen_timeout_t)screen_timeout;
        }
        nvs_close(handle);
    }
    xSemaphoreGive(s_config_mutex);
    return err;
}

esp_err_t config_service_ui_preferences_write(
    const config_service_ui_preferences_t *preferences,
    TickType_t timeout_ticks)
{
    if (preferences == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (preferences->brightness < CONFIG_SERVICE_UI_BRIGHTNESS_LOW ||
        preferences->brightness >= CONFIG_SERVICE_UI_BRIGHTNESS_COUNT ||
        preferences->screen_timeout < CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S ||
        preferences->screen_timeout >= CONFIG_SERVICE_UI_SCREEN_TIMEOUT_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_UI_NAMESPACE,
                             NVS_READWRITE,
                             &handle);
    if (err == ESP_OK)
    {
        err = nvs_set_u8(handle,
                         CONFIG_SERVICE_UI_HAPTICS_KEY,
                         preferences->haptics_enabled ? 1U : 0U);
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u8(handle,
                         CONFIG_SERVICE_UI_CLICK_AUDIO_KEY,
                         preferences->click_audio_enabled ? 1U : 0U);
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u8(handle,
                         CONFIG_SERVICE_UI_RAISE_WAKE_KEY,
                         preferences->raise_to_wake_enabled ? 1U : 0U);
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u8(handle,
                         CONFIG_SERVICE_UI_BRIGHTNESS_KEY,
                         (uint8_t)preferences->brightness);
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u8(handle,
                         CONFIG_SERVICE_UI_SCREEN_TIMEOUT_KEY,
                         (uint8_t)preferences->screen_timeout);
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    xSemaphoreGive(s_config_mutex);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UI 偏好 NVS 事务失败，错误=0x%x", (unsigned)err);
    }
    return err;
}

esp_err_t config_service_selftest_running_read(bool *running,
                                               TickType_t timeout_ticks)
{
    if (running == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *running = false;
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_DIAG_NAMESPACE,
                             NVS_READONLY,
                             &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = ESP_OK;
    }
    else if (err == ESP_OK)
    {
        uint8_t stored_running = 0U;
        err = nvs_get_u8(handle,
                         CONFIG_SERVICE_SELFTEST_RUNNING_KEY,
                         &stored_running);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            err = ESP_OK;
        }
        else if (err == ESP_OK && stored_running > 1U)
        {
            err = ESP_ERR_INVALID_STATE;
        }
        else if (err == ESP_OK)
        {
            *running = stored_running != 0U;
        }
        nvs_close(handle);
    }
    xSemaphoreGive(s_config_mutex);
    return err;
}

esp_err_t config_service_selftest_running_write(bool running,
                                                TickType_t timeout_ticks)
{
    if (!s_initialized || s_config_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_config_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_DIAG_NAMESPACE,
                             NVS_READWRITE,
                             &handle);
    if (err == ESP_OK)
    {
        err = nvs_set_u8(handle,
                         CONFIG_SERVICE_SELFTEST_RUNNING_KEY,
                         running ? 1U : 0U);
    }
    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }
    if (handle != 0)
    {
        nvs_close(handle);
    }
    xSemaphoreGive(s_config_mutex);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "整机自检运行标记 NVS 事务失败，running=%u，错误=0x%x",
                 running ? 1U : 0U,
                 (unsigned)err);
    }
    return err;
}

static bool normalize_mac(const char input[CONFIG_SERVICE_MAC_CAPACITY],
                          char output[CONFIG_SERVICE_MAC_CAPACITY])
{
    if (input == NULL || output == NULL ||
        strnlen(input, CONFIG_SERVICE_MAC_CAPACITY) != CONFIG_SERVICE_MAC_CAPACITY - 1U)
    {
        return false;
    }
    for (size_t index = 0; index < CONFIG_SERVICE_MAC_CAPACITY - 1U; ++index)
    {
        const bool separator = index == 2U || index == 5U || index == 8U ||
                               index == 11U || index == 14U;
        if (separator)
        {
            if (input[index] != ':')
            {
                return false;
            }
            output[index] = ':';
        }
        else
        {
            const unsigned char value = (unsigned char)input[index];
            if (!isxdigit(value))
            {
                return false;
            }
            output[index] = (char)toupper(value);
        }
    }
    output[CONFIG_SERVICE_MAC_CAPACITY - 1U] = '\0';
    return true;
}

static esp_err_t load_schema_from_nvs(config_service_snapshot_t *snapshot)
{
    snapshot->schema_version = CONFIG_SERVICE_SCHEMA_VERSION;
    snapshot->schema_status = CONFIG_SERVICE_SCHEMA_VALID;
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_CONFIG_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }
    uint16_t stored_version = 0U;
    err = nvs_get_u16(handle, CONFIG_SERVICE_SCHEMA_VERSION_KEY, &stored_version);
    if (err == ESP_ERR_NVS_TYPE_MISMATCH)
    {
        snapshot->schema_status = CONFIG_SERVICE_SCHEMA_CORRUPT;
    }
    else if (err == ESP_OK)
    {
        snapshot->schema_version = stored_version;
        if (stored_version != CONFIG_SERVICE_SCHEMA_VERSION)
        {
            snapshot->schema_status = CONFIG_SERVICE_SCHEMA_CORRUPT;
        }
    }
    else if (err != ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(handle);
        return err;
    }

    uint16_t stored_epoch = 0U;
    err = nvs_get_u16(handle, CONFIG_SERVICE_CONFIG_EPOCH_KEY, &stored_epoch);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_TYPE_MISMATCH)
    {
        snapshot->schema_status = CONFIG_SERVICE_SCHEMA_CORRUPT;
        return ESP_OK;
    }
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        return err;
    }
    snapshot->config_epoch = err == ESP_OK ? stored_epoch : 0U;
    return ESP_OK;
}

static esp_err_t load_cloud_from_nvs(config_service_snapshot_t *snapshot,
                                     config_service_cloud_credentials_t *credentials)
{
    bool base_present = false;
    bool base_invalid = false;
    bool apn_present = false;
    bool apn_invalid = false;
    bool token_present = false;
    bool token_invalid = false;
    bool ca_ref_present = false;
    bool ca_ref_invalid = false;
    bool ca_pem_present = false;
    bool ca_pem_invalid = false;

    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_CLOUD_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        if (snapshot->config_epoch != 0U)
        {
            snapshot->schema_status = CONFIG_SERVICE_SCHEMA_CORRUPT;
        }
        memcpy(credentials->token, CONFIG_SERVICE_V1_TOKEN, sizeof(CONFIG_SERVICE_V1_TOKEN));
        classify_cloud(snapshot, credentials, false, false, false, false,
                       false, false, false, false, false, false);
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }

    uint16_t cloud_epoch = 0U;
    err = nvs_get_u16(handle, CONFIG_SERVICE_CLOUD_EPOCH_KEY, &cloud_epoch);
    if (err == ESP_ERR_NVS_TYPE_MISMATCH)
    {
        snapshot->schema_status = CONFIG_SERVICE_SCHEMA_CORRUPT;
        err = ESP_OK;
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        cloud_epoch = 0U;
        err = ESP_OK;
    }
    const bool epoch_matches = err == ESP_OK && cloud_epoch == snapshot->config_epoch;
    if (err == ESP_OK && !epoch_matches)
    {
        snapshot->schema_status = CONFIG_SERVICE_SCHEMA_CORRUPT;
    }

    if (err == ESP_OK)
    {
        err = load_optional_string(handle, CONFIG_SERVICE_CLOUD_BASE_URL_KEY,
                                   credentials->base_url, sizeof(credentials->base_url),
                                   &base_present, &base_invalid);
    }
    if (err == ESP_OK)
    {
        err = load_optional_string(handle, CONFIG_SERVICE_CLOUD_APN_KEY,
                                   credentials->apn, sizeof(credentials->apn),
                                   &apn_present, &apn_invalid);
    }
    char token_override[CONFIG_SERVICE_TOKEN_CAPACITY] = {0};
    if (err == ESP_OK)
    {
        err = load_optional_string(handle, CONFIG_SERVICE_CLOUD_TOKEN_KEY,
                                   token_override, sizeof(token_override),
                                   &token_present, &token_invalid);
    }
    if (err == ESP_OK)
    {
        err = load_optional_string(handle, CONFIG_SERVICE_CLOUD_CA_REF_KEY,
                                   credentials->ca_ref, sizeof(credentials->ca_ref),
                                   &ca_ref_present, &ca_ref_invalid);
    }
    if (err == ESP_OK)
    {
        err = load_optional_string(handle, CONFIG_SERVICE_CLOUD_CA_PEM_KEY,
                                   credentials->ca_pem, sizeof(credentials->ca_pem),
                                   &ca_pem_present, &ca_pem_invalid);
    }
    if (err == ESP_OK && ca_pem_present && !ca_pem_invalid)
    {
        const esp_err_t pem_error = validate_ca_pem(credentials->ca_pem,
                                                    sizeof(credentials->ca_pem));
        if (pem_error == ESP_ERR_INVALID_ARG)
        {
            ca_pem_invalid = true;
        }
        else if (pem_error != ESP_OK)
        {
            err = pem_error;
        }
    }
    nvs_close(handle);
    if (err != ESP_OK)
    {
        secure_zero(token_override, sizeof(token_override));
        return err;
    }

    if (!token_present)
    {
        memcpy(credentials->token, CONFIG_SERVICE_V1_TOKEN, sizeof(CONFIG_SERVICE_V1_TOKEN));
    }
    else if (!token_invalid && strcmp(token_override, CONFIG_SERVICE_V1_TOKEN) == 0)
    {
        memcpy(credentials->token, CONFIG_SERVICE_V1_TOKEN, sizeof(CONFIG_SERVICE_V1_TOKEN));
    }
    else
    {
        token_invalid = true;
    }
    classify_cloud(snapshot, credentials,
                   base_present, base_invalid, apn_present, apn_invalid,
                   token_present, token_invalid, ca_ref_present, ca_ref_invalid,
                   ca_pem_present, ca_pem_invalid);
    if (!epoch_matches)
    {
        secure_zero(credentials, sizeof(*credentials));
    }
    secure_zero(token_override, sizeof(token_override));
    return ESP_OK;
}

static esp_err_t load_binding_from_nvs(config_service_snapshot_t *snapshot)
{
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_BLE_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        snapshot->status = CONFIG_SERVICE_BINDING_UNBOUND;
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }

    size_t required_size = 0U;
    err = nvs_get_str(handle, CONFIG_SERVICE_BOUND_EXO_KEY, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        snapshot->status = CONFIG_SERVICE_BINDING_UNBOUND;
        nvs_close(handle);
        return ESP_OK;
    }
    if (err == ESP_ERR_NVS_TYPE_MISMATCH || err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        snapshot->status = CONFIG_SERVICE_BINDING_CORRUPT;
        nvs_close(handle);
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        nvs_close(handle);
        return err;
    }
    if (required_size != CONFIG_SERVICE_MAC_CAPACITY)
    {
        snapshot->status = CONFIG_SERVICE_BINDING_CORRUPT;
        nvs_close(handle);
        return ESP_OK;
    }

    char stored[CONFIG_SERVICE_MAC_CAPACITY] = {0};
    size_t capacity = sizeof(stored);
    err = nvs_get_str(handle, CONFIG_SERVICE_BOUND_EXO_KEY, stored, &capacity);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_INVALID_LENGTH || err == ESP_ERR_NVS_TYPE_MISMATCH)
    {
        snapshot->status = CONFIG_SERVICE_BINDING_CORRUPT;
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }
    if (!normalize_mac(stored, snapshot->bound_exoskeleton_mac))
    {
        memset(snapshot->bound_exoskeleton_mac, 0, sizeof(snapshot->bound_exoskeleton_mac));
        snapshot->status = CONFIG_SERVICE_BINDING_CORRUPT;
        return ESP_OK;
    }
    snapshot->status = CONFIG_SERVICE_BINDING_LOADED;
    snapshot->has_bound_exoskeleton = true;
    return ESP_OK;
}

static esp_err_t load_optional_string(nvs_handle_t handle,
                                      const char *key,
                                      char *destination,
                                      size_t capacity,
                                      bool *present,
                                      bool *invalid)
{
    *present = false;
    *invalid = false;
    size_t required_size = 0U;
    esp_err_t err = nvs_get_str(handle, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return ESP_OK;
    }
    *present = true;
    if (err == ESP_ERR_NVS_TYPE_MISMATCH || err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        *invalid = true;
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }
    if (required_size == 0U || required_size > capacity)
    {
        *invalid = true;
        return ESP_OK;
    }
    size_t output_size = capacity;
    err = nvs_get_str(handle, key, destination, &output_size);
    if (err == ESP_ERR_NVS_TYPE_MISMATCH || err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        memset(destination, 0, capacity);
        *invalid = true;
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        return err;
    }
    if (output_size != required_size ||
        strnlen(destination, capacity) != required_size - 1U)
    {
        memset(destination, 0, capacity);
        *invalid = true;
    }
    return ESP_OK;
}

static bool base_url_is_valid(const char *value, size_t capacity)
{
    if (value == NULL || strnlen(value, capacity) >= capacity ||
        strncmp(value, "https://", 8U) != 0)
    {
        return false;
    }
    const char *host = value + 8U;
    if (*host == '\0' || *host == '/' || *host == ':' || *host == '@')
    {
        return false;
    }
    const char *authority_end = host;
    while (*authority_end != '\0' && *authority_end != '/' &&
           *authority_end != '?' && *authority_end != '#')
    {
        const unsigned char character = (unsigned char)*authority_end;
        if (*authority_end == '@' || isspace(character) || iscntrl(character))
        {
            return false;
        }
        ++authority_end;
    }
    bool host_character = false;
    const char *cursor = host;
    size_t label_length = 0U;
    for (; cursor < authority_end && *cursor != ':'; ++cursor)
    {
        const unsigned char character = (unsigned char)*cursor;
        if (*cursor == '.')
        {
            if (label_length == 0U || cursor[-1] == '-')
            {
                return false;
            }
            label_length = 0U;
            continue;
        }
        if (!(isalnum(character) || *cursor == '-') ||
            (label_length == 0U && *cursor == '-') || label_length >= 63U)
        {
            return false;
        }
        ++label_length;
        host_character = true;
    }
    if (!host_character || label_length == 0U || cursor[-1] == '-')
    {
        return false;
    }
    if (cursor < authority_end && *cursor == ':')
    {
        ++cursor;
        if (cursor == authority_end)
        {
            return false;
        }
        uint32_t port = 0U;
        for (; cursor < authority_end; ++cursor)
        {
            if (!isdigit((unsigned char)*cursor))
            {
                return false;
            }
            const uint32_t digit = (uint32_t)(*cursor - '0');
            if (port > 6553U || (port == 6553U && digit > 5U))
            {
                return false;
            }
            port = port * 10U + digit;
        }
        if (port == 0U)
        {
            return false;
        }
    }
    for (const char *remainder = authority_end; *remainder != '\0'; ++remainder)
    {
        if (isspace((unsigned char)*remainder) || iscntrl((unsigned char)*remainder))
        {
            return false;
        }
    }
    return cursor == authority_end;
}

static bool apn_is_valid(const char *value, size_t capacity)
{
    if (value == NULL)
    {
        return false;
    }
    const size_t length = strnlen(value, capacity);
    if (length == 0U)
    {
        return true;
    }
    if (length >= capacity)
    {
        return false;
    }
    for (size_t index = 0; index < length; ++index)
    {
        const unsigned char character = (unsigned char)value[index];
        if (!(isalnum(character) || character == '.' || character == '_' || character == '-'))
        {
            return false;
        }
    }
    return true;
}

static bool ca_ref_is_valid(const char *value, size_t capacity)
{
    if (value == NULL)
    {
        return false;
    }
    const size_t length = strnlen(value, capacity);
    if (length == 0U || length >= capacity)
    {
        return false;
    }
    for (size_t index = 0; index < length; ++index)
    {
        const unsigned char character = (unsigned char)value[index];
        if (!(isalnum(character) || character == '.' || character == '_' ||
              character == '-' || character == '/'))
        {
            return false;
        }
    }
    return true;
}

static esp_err_t validate_ca_pem(const char *value, size_t capacity)
{
    if (value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t length = strnlen(value, capacity);
    /** PEM 起始边界行的固定字符数，不含换行。 */
    const size_t begin_line_length = 27U;
    /** PEM 结束边界行的固定字符数，不含换行。 */
    const size_t end_line_length = 25U;
    if (length <= begin_line_length + end_line_length + 2U ||
        length >= capacity)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* 仅检查 PEM 信封与 Base64 字符；实际证书解析由 ML307R 在可选 CA 模式完成。 */
    if (memcmp(value, "-----", 5U) != 0 ||
        memcmp(value + 5U, "BEGIN", 5U) != 0 ||
        value[10U] != ' ' ||
        memcmp(value + 11U, "CERTIFICATE", 11U) != 0 ||
        memcmp(value + 22U, "-----", 5U) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    size_t body_start = begin_line_length;
    if (value[body_start] == '\r')
    {
        ++body_start;
    }
    if (value[body_start] != '\n')
    {
        return ESP_ERR_INVALID_ARG;
    }
    ++body_start;

    size_t content_end = length;
    while (content_end > body_start &&
           (value[content_end - 1U] == '\r' ||
            value[content_end - 1U] == '\n'))
    {
        --content_end;
    }
    if (content_end <= body_start + end_line_length)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t end_start = content_end - end_line_length;
    if (end_start == 0U || value[end_start - 1U] != '\n' ||
        memcmp(value + end_start, "-----", 5U) != 0 ||
        memcmp(value + end_start + 5U, "END", 3U) != 0 ||
        value[end_start + 8U] != ' ' ||
        memcmp(value + end_start + 9U, "CERTIFICATE", 11U) != 0 ||
        memcmp(value + end_start + 20U, "-----", 5U) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    bool body_has_data = false;
    for (size_t index = body_start; index < end_start; ++index)
    {
        const unsigned char character = (unsigned char)value[index];
        if (isalnum(character) || character == '+' || character == '/' ||
            character == '=')
        {
            body_has_data = true;
        }
        else if (character != '\r' && character != '\n')
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    return body_has_data ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static void classify_cloud(config_service_snapshot_t *snapshot,
                           config_service_cloud_credentials_t *credentials,
                           bool base_present,
                           bool base_invalid,
                           bool apn_present,
                           bool apn_invalid,
                           bool token_present,
                           bool token_invalid,
                           bool ca_ref_present,
                           bool ca_ref_invalid,
                           bool ca_pem_present,
                           bool ca_pem_invalid)
{
    (void)token_present;
    if (!base_present)
    {
        snapshot->cloud_base_url_status = CONFIG_SERVICE_VALUE_UNCONFIGURED;
    }
    else if (base_invalid || !base_url_is_valid(credentials->base_url,
                                                sizeof(credentials->base_url)))
    {
        snapshot->cloud_base_url_status = CONFIG_SERVICE_VALUE_INVALID;
        memset(credentials->base_url, 0, sizeof(credentials->base_url));
    }
    else
    {
        snapshot->cloud_base_url_status = CONFIG_SERVICE_VALUE_CONFIGURED;
    }

    if (!apn_present || (!apn_invalid && credentials->apn[0] == '\0'))
    {
        snapshot->apn_mode = CONFIG_SERVICE_APN_AUTO;
    }
    else if (apn_invalid || !apn_is_valid(credentials->apn, sizeof(credentials->apn)))
    {
        snapshot->apn_mode = CONFIG_SERVICE_APN_INVALID;
        memset(credentials->apn, 0, sizeof(credentials->apn));
    }
    else
    {
        snapshot->apn_mode = CONFIG_SERVICE_APN_CONFIGURED;
    }

    snapshot->token_status = token_invalid
                                 ? CONFIG_SERVICE_VALUE_INVALID
                                 : (credentials->token[0] == '\0'
                                        ? CONFIG_SERVICE_VALUE_UNCONFIGURED
                                        : CONFIG_SERVICE_VALUE_CONFIGURED);
    if (snapshot->token_status != CONFIG_SERVICE_VALUE_CONFIGURED)
    {
        memset(credentials->token, 0, sizeof(credentials->token));
    }

    const bool both_ca_sources = ca_ref_present && ca_pem_present;
    if (!ca_ref_present && !ca_pem_present)
    {
        snapshot->certificate_status = CONFIG_SERVICE_VALUE_UNCONFIGURED;
    }
    else if (both_ca_sources || ca_ref_invalid || ca_pem_invalid ||
             (ca_ref_present && !ca_ref_is_valid(credentials->ca_ref,
                                                 sizeof(credentials->ca_ref))))
    {
        snapshot->certificate_status = CONFIG_SERVICE_VALUE_INVALID;
        memset(credentials->ca_ref, 0, sizeof(credentials->ca_ref));
        memset(credentials->ca_pem, 0, sizeof(credentials->ca_pem));
    }
    else
    {
        snapshot->certificate_status = CONFIG_SERVICE_VALUE_CONFIGURED;
    }
}

static void secure_zero(void *buffer, size_t length)
{
    volatile unsigned char *cursor = (volatile unsigned char *)buffer;
    while (length > 0U)
    {
        *cursor = 0U;
        ++cursor;
        --length;
    }
}

static esp_err_t set_or_erase_string(nvs_handle_t handle,
                                     const char *key,
                                     const char *value)
{
    if (value[0] != '\0')
    {
        return nvs_set_str(handle, key, value);
    }
    const esp_err_t err = nvs_erase_key(handle, key);
    return err == ESP_ERR_NVS_NOT_FOUND ? ESP_OK : err;
}

static esp_err_t write_cloud_candidate(
    nvs_handle_t handle,
    const config_service_cloud_credentials_t *credentials,
    uint16_t epoch)
{
    esp_err_t err = set_or_erase_string(handle,
                                        CONFIG_SERVICE_CLOUD_BASE_URL_KEY,
                                        credentials->base_url);
    if (err == ESP_OK)
    {
        err = set_or_erase_string(handle,
                                  CONFIG_SERVICE_CLOUD_APN_KEY,
                                  credentials->apn);
    }
    if (err == ESP_OK)
    {
        err = set_or_erase_string(handle,
                                  CONFIG_SERVICE_CLOUD_TOKEN_KEY,
                                  credentials->token);
    }
    if (err == ESP_OK)
    {
        err = set_or_erase_string(handle,
                                  CONFIG_SERVICE_CLOUD_CA_REF_KEY,
                                  credentials->ca_ref);
    }
    if (err == ESP_OK)
    {
        err = set_or_erase_string(handle,
                                  CONFIG_SERVICE_CLOUD_CA_PEM_KEY,
                                  credentials->ca_pem);
    }
    if (err == ESP_OK)
    {
        err = nvs_set_u16(handle, CONFIG_SERVICE_CLOUD_EPOCH_KEY, epoch);
    }
    return err;
}

static void load_cloud_offset_cache(void)
{
    s_cloud_offset_ms = 0;
    s_cloud_offset_present = false;
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(CONFIG_SERVICE_TIME_NAMESPACE,
                             NVS_READONLY,
                             &handle);
    if (err == ESP_OK)
    {
        err = nvs_get_i64(handle,
                          CONFIG_SERVICE_CLOUD_OFFSET_KEY,
                          &s_cloud_offset_ms);
        nvs_close(handle);
    }
    if (err == ESP_OK)
    {
        s_cloud_offset_present = true;
    }
    else if (err != ESP_ERR_NVS_NOT_FOUND)
    {
        s_cloud_offset_ms = 0;
        ESP_LOGW(TAG,
                 "云时间偏移启动缓存读取失败，忽略派生值，错误=0x%x",
                 (unsigned)err);
    }
}
