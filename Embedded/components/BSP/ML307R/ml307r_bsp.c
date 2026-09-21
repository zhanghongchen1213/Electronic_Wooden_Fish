/**
 * @file     ml307r_bsp.c
 * @brief    ML307R 4G 模块 BSP 资源与使能门禁实现
 * @details  从权威 BSP 资源表取得 IO10，并按固件能力在启动早期固定为启用或关闭电平。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "ml307r_bsp.h"

#include "esp_log.h"
#include "esp_timer.h"

#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "BSP_ML307R";

/** ML307R EN 成功写高时的单调微秒时间，负值表示尚未使能。 */
static int64_t s_enabled_at_us = -1;

static esp_err_t configure_enable_level(uint32_t level);

const legbot_bsp_resource_t *ml307r_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_ML307R);
}

esp_err_t ml307r_bsp_enable(void)
{
#if !LEGBOT_CAP_MODEM
    return ESP_ERR_NOT_SUPPORTED;
#else
    const esp_err_t error = configure_enable_level(
        LEGBOT_BSP_ML307R_EN_ACTIVE_LEVEL);
    if (error != ESP_OK)
    {
        return error;
    }

    s_enabled_at_us = esp_timer_get_time();
    ESP_LOGI(TAG,
             "ML307R 已启用，IO=%d，电平=%u，UART 最小就绪等待=%u ms",
             (int)LEGBOT_BSP_ML307R_EN_GPIO,
             (unsigned)LEGBOT_BSP_ML307R_EN_ACTIVE_LEVEL,
             (unsigned)ML307R_BSP_UART_READY_DELAY_MS);
    return ESP_OK;
#endif
}

esp_err_t ml307r_bsp_hold_disabled(void)
{
#if LEGBOT_CAP_MODEM
    return ESP_ERR_NOT_SUPPORTED;
#else
    const esp_err_t error = configure_enable_level(
        LEGBOT_BSP_ML307R_EN_INACTIVE_LEVEL);
    if (error != ESP_OK)
    {
        return error;
    }

    s_enabled_at_us = -1;
    ESP_LOGI(TAG,
             "4G 禁用 profile 已保持 ML307R 输出低且 light sleep 不隔离，IO=%d，电平=%u",
             (int)LEGBOT_BSP_ML307R_EN_GPIO,
             (unsigned)LEGBOT_BSP_ML307R_EN_INACTIVE_LEVEL);
    return ESP_OK;
#endif
}

esp_err_t ml307r_bsp_uart_ready_wait_ms(uint32_t *wait_ms)
{
    if (wait_ms == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_enabled_at_us < 0)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const int64_t required_us =
        (int64_t)ML307R_BSP_UART_READY_DELAY_MS * 1000LL;
    const int64_t elapsed_us = esp_timer_get_time() - s_enabled_at_us;
    if (elapsed_us >= required_us)
    {
        *wait_ms = 0U;
        return ESP_OK;
    }

    const int64_t remaining_us = required_us - elapsed_us;
    *wait_ms = (uint32_t)((remaining_us + 999LL) / 1000LL);
    return ESP_OK;
}

static esp_err_t configure_enable_level(uint32_t level)
{
    const legbot_bsp_resource_t *resource = ml307r_bsp_resource();
    if (resource == NULL ||
        resource->gpio_aux0 != LEGBOT_BSP_ML307R_EN_GPIO)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << (uint32_t)LEGBOT_BSP_ML307R_EN_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    /* 先预装目标电平，再启用输出，避免接管 EN 时短暂驱出旧锁存电平。 */
    const esp_err_t preload_error = gpio_set_level(
        LEGBOT_BSP_ML307R_EN_GPIO,
        level);
    if (preload_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "ML307R EN 电平预装失败，IO=%d，目标电平=%u，错误=%s",
                 (int)LEGBOT_BSP_ML307R_EN_GPIO,
                 (unsigned)level,
                 esp_err_to_name(preload_error));
        return preload_error;
    }

    const esp_err_t config_error = gpio_config(&config);
    if (config_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "ML307R EN GPIO 配置失败，IO=%d，目标电平=%u，错误=%s",
                 (int)LEGBOT_BSP_ML307R_EN_GPIO,
                 (unsigned)level,
                 esp_err_to_name(config_error));
        return config_error;
    }

    const esp_err_t level_error = gpio_set_level(
        LEGBOT_BSP_ML307R_EN_GPIO,
        level);
    if (level_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "ML307R EN 电平写入失败，IO=%d，目标电平=%u，错误=%s",
                 (int)LEGBOT_BSP_ML307R_EN_GPIO,
                 (unsigned)level,
                 esp_err_to_name(level_error));
        return level_error;
    }

    /* 自动 light sleep 时保持当前 EN 输出，避免 v1 低电平或 v2 高电平变为悬空。 */
    const esp_err_t sleep_error =
        gpio_sleep_sel_dis(LEGBOT_BSP_ML307R_EN_GPIO);
    if (sleep_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "ML307R EN 无法保持 light sleep 输出配置，IO=%d，目标电平=%u，错误=%s",
                 (int)LEGBOT_BSP_ML307R_EN_GPIO,
                 (unsigned)level,
                 esp_err_to_name(sleep_error));
    }
    return sleep_error;
}
