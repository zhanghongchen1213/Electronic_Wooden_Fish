/**
 * @file     ns4150_bsp.c
 * @brief    NS4150 功放安全控制实现
 * @details  按 NS4150B CTRL 高开低关契约配置 PA_EN，并确保初始化、停止、错误和释放均回到低电平。
 * @author   ZHC
 * @date     2026-07-10
 */

#include "ns4150_bsp.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BSP_NS4150";

/** 功放 GPIO 是否已由本模块初始化。 */
static bool s_initialized;
/** 功放当前是否处于开启电平。 */
static bool s_pa_enabled;
/** GPIO 硬件是否已确认处于安全关断电平。 */
static bool s_safe_state_known;

const legbot_bsp_resource_t *ns4150_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_NS4150);
}

esp_err_t ns4150_bsp_init(void)
{
    if (s_initialized)
    {
        return ns4150_bsp_safe_off();
    }
    const legbot_bsp_resource_t *resource = ns4150_bsp_resource();
    if (resource == NULL || resource->gpio_primary == GPIO_NUM_NC)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* 先写入输出锁存器，再切换为输出，降低上电阶段短促误开启风险。 */
    esp_err_t err = gpio_set_level(resource->gpio_primary,
                                   LEGBOT_BSP_NS4150_PA_INACTIVE_LEVEL);
    if (err != ESP_OK)
    {
        s_safe_state_known = false;
        return err;
    }
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << resource->gpio_primary,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&config);
    if (err != ESP_OK)
    {
        s_safe_state_known = false;
        return err;
    }
    err = gpio_set_level(resource->gpio_primary,
                         LEGBOT_BSP_NS4150_PA_INACTIVE_LEVEL);
    if (err == ESP_OK)
    {
        s_initialized = true;
        s_pa_enabled = false;
        s_safe_state_known = true;
        ESP_LOGI(TAG, "NS4150 功放已初始化并保持安全关断电平");
    }
    else
    {
        s_safe_state_known = false;
    }
    return err;
}

esp_err_t ns4150_bsp_enable(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const legbot_bsp_resource_t *resource = ns4150_bsp_resource();
    esp_err_t err = gpio_set_level(resource->gpio_primary,
                                   LEGBOT_BSP_NS4150_PA_ACTIVE_LEVEL);
    if (err == ESP_OK)
    {
        s_pa_enabled = true;
    }
    s_safe_state_known = false;
    return err;
}

esp_err_t ns4150_bsp_safe_off(void)
{
    if (!s_initialized)
    {
        s_pa_enabled = false;
        return s_safe_state_known ? ESP_OK : ns4150_bsp_init();
    }
    const legbot_bsp_resource_t *resource = ns4150_bsp_resource();
    esp_err_t err = gpio_set_level(resource->gpio_primary,
                                   LEGBOT_BSP_NS4150_PA_INACTIVE_LEVEL);
    if (err == ESP_OK)
    {
        s_pa_enabled = false;
        s_safe_state_known = true;
    }
    else
    {
        s_safe_state_known = false;
    }
    return err;
}

esp_err_t ns4150_bsp_deinit(void)
{
    esp_err_t err = ns4150_bsp_safe_off();
    if (err == ESP_OK)
    {
        s_initialized = false;
    }
    return err;
}

bool ns4150_bsp_is_enabled(void)
{
    return s_initialized && s_pa_enabled;
}

const char *ns4150_bsp_error_code(esp_err_t err)
{
    if (err == ESP_OK)
    {
        return "DRV_NS4150_OK";
    }
    if (err == ESP_ERR_INVALID_STATE)
    {
        return "DRV_NS4150_NOT_READY";
    }
    return "DRV_NS4150_GPIO_FAILED";
}
