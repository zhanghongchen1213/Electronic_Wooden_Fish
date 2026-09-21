/**
 * @file     cloud_provision.c
 * @brief    线上联调云配置初始化实现
 * @details  仅为空白配置写入无服务端身份校验的默认 HTTPS 参数，不改写已有配置。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "cloud_provision.h"

#include <stdbool.h>

#include "config_service.h"
#include "exoskeleton_scene_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "MAIN_CLOUD";

/** v1 手环共享设备 Token，仅写入配置且禁止输出到日志。 */
#define CLOUD_DEVICE_TOKEN "xianli_freedom_2189"
/** 读取配置 owner 快照的最长等待时间。 */
#define CLOUD_CONFIG_TIMEOUT_MS 50U

/** 默认联调配置仅包含 API 地址、auto APN、固定 Token，不包含 CA。 */
static const config_service_cloud_credentials_t s_default_credentials = {
    .base_url = CLOUD_DEFAULT_BASE_URL,
    .apn = CLOUD_DEFAULT_APN,
    .token = CLOUD_DEVICE_TOKEN,
    .ca_ref = "",
    .ca_pem = "",
};

static esp_err_t verify_cloud_ready(void);

esp_err_t cloud_provision_apply(void)
{
    config_service_snapshot_t snapshot = {0};
    esp_err_t err = config_service_snapshot(
        &snapshot,
        pdMS_TO_TICKS(CLOUD_CONFIG_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "读取云配置快照失败，错误=0x%x", (unsigned)err);
        return err;
    }

    if (snapshot.cloud_configured && snapshot.cloud_error == CLOUD_ERROR_NONE)
    {
        ESP_LOGI(TAG, "已有有效云配置，保持当前人工配置不变");
        if (snapshot.certificate_status == CONFIG_SERVICE_VALUE_UNCONFIGURED)
        {
            ESP_LOGW(TAG, "HTTPS 已加密，服务端身份未校验，仅用于线上联调");
        }
        return ESP_OK;
    }

    /* 仅受控 token 与 APN auto 存在时才属于可安全初始化的空白云配置。 */
    const bool cloud_configuration_is_blank =
        snapshot.schema_status == CONFIG_SERVICE_SCHEMA_VALID &&
        snapshot.cloud_base_url_status == CONFIG_SERVICE_VALUE_UNCONFIGURED &&
        snapshot.apn_mode == CONFIG_SERVICE_APN_AUTO &&
        snapshot.token_status == CONFIG_SERVICE_VALUE_CONFIGURED &&
        snapshot.certificate_status == CONFIG_SERVICE_VALUE_UNCONFIGURED;
    if (!cloud_configuration_is_blank)
    {
        ESP_LOGE(TAG,
                 "云配置并非完全空白，保持不可用且不覆盖，当前状态=%s",
                 config_service_cloud_error_code(snapshot.cloud_error));
        return ESP_OK;
    }

    err = config_service_set_cloud_credentials(&s_default_credentials);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "默认联调云配置写入失败，错误=0x%x", (unsigned)err);
        return err;
    }
    ESP_LOGI(TAG, "已写入默认联调云地址，APN=auto，不保存 CA 配置");
    return verify_cloud_ready();
}

static esp_err_t verify_cloud_ready(void)
{
    config_service_snapshot_t snapshot = {0};
    const esp_err_t err = config_service_snapshot(
        &snapshot,
        pdMS_TO_TICKS(CLOUD_CONFIG_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "云配置写入后复核失败，错误=0x%x", (unsigned)err);
        return err;
    }
    if (!snapshot.cloud_configured || snapshot.cloud_error != CLOUD_ERROR_NONE)
    {
        ESP_LOGE(TAG,
                 "云配置写入后预检未通过，当前状态=%s",
                 config_service_cloud_error_code(snapshot.cloud_error));
        return ESP_FAIL;
    }

    ESP_LOGW(TAG, "HTTPS 已加密，服务端身份未校验，仅用于线上联调");
    return ESP_OK;
}
