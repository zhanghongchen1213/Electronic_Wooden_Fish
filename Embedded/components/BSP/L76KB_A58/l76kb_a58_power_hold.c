/**
 * @file     l76kb_a58_power_hold.c
 * @brief    L76KB-A58 GPS 上电安全待机实现
 * @details  仅配置 IO3 WAKE 为持续低电平，不初始化 GPS 生命周期、UART 或任务。
 * @author   ZHC
 * @date     2026-08-07
 */

#include "l76kb_a58_bsp.h"

#include "driver/gpio.h"

esp_err_t l76kb_a58_bsp_hold_standby(void)
{
    const legbot_bsp_resource_t *resource =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_GPS_L76KB_A58);
    if (resource == NULL ||
        resource->id != LEGBOT_BSP_RESOURCE_GPS_L76KB_A58 ||
        resource->gpio_aux0 != LEGBOT_BSP_GPS_WAKE_GPIO ||
        LEGBOT_BSP_SDMMC_ENABLED != 0)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* 先预装输出锁存低电平，再打开输出，避免模式切换产生高脉冲。 */
    esp_err_t error = gpio_set_level(resource->gpio_aux0, 0);
    if (error != ESP_OK)
    {
        return error;
    }
    const gpio_config_t wake_config = {
        .pin_bit_mask = 1ULL << (uint32_t)resource->gpio_aux0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    error = gpio_config(&wake_config);
    if (error != ESP_OK)
    {
        return error;
    }
    error = gpio_set_level(resource->gpio_aux0, 0);
    if (error != ESP_OK)
    {
        return error;
    }
    return gpio_sleep_sel_dis(resource->gpio_aux0);
}
