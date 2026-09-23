/**
 * @file     rgb_bsp.c
 * @brief    RGB 状态灯（WS2812 类单线器件）BSP 实现
 * @details  以 espressif/led_strip v3.0.1（RMT 后端）为上游驱动，自有 BSP 封装
 *           稳定 API 与错误码映射（AD-8）。RGB_DATA=IO3 与 JTAG 源选择 eFuse
 *           （EFUSE_STRAP_JTAG_SEL）相关：初始化先输出确定低电平，保证烧录该
 *           eFuse 后 RGB_DATA 外围仍有确定的复位电平（硬件基线 §5.3.1）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "rgb_bsp.h"

#include <stddef.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

static const char *TAG = "BSP_RGB";

/** RMT 时钟分辨率 10 MHz：WS2812 类 800 kHz 时序的稳定量化基准。 */
#define RGB_BSP_RMT_RESOLUTION_HZ (10U * 1000U * 1000U)

/** led_strip 设备句柄；NULL 表示尚未初始化。 */
static led_strip_handle_t s_strip;
/** 初始化完成标志。 */
static bool s_ready;

esp_err_t rgb_bsp_init(void)
{
    if (s_ready)
    {
        return ESP_OK;
    }

    /* GPIO3 是 strap/JTAG 源选择相关脚：先建立确定低电平，不改变下载流程。 */
    const gpio_num_t data_gpio = (gpio_num_t)RGB_BSP_DATA_GPIO_ID;
    const gpio_config_t strap_level = {
        .pin_bit_mask = 1ULL << (unsigned)data_gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t error = gpio_config(&strap_level);
    if (error == ESP_OK)
    {
        error = gpio_set_level(data_gpio, 0);
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "RGB_DATA strap 确定电平建立失败，错误=%s", esp_err_to_name(error));
        return error;
    }

    const led_strip_config_t strip_config = {
        .strip_gpio_num = (int)data_gpio,
        .max_leds = (int)RGB_BSP_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
    };
    const led_strip_rmt_config_t rmt_config = {
        .resolution_hz = RGB_BSP_RMT_RESOLUTION_HZ,
    };
    error = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "led_strip RMT 设备创建失败，错误=%s", esp_err_to_name(error));
        return error;
    }
    /* 初始化后立即输出熄灭态：空闲灯效为确定的灭。 */
    error = led_strip_clear(s_strip);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "RGB 初始化熄灭失败，错误=%s", esp_err_to_name(error));
        led_strip_del(s_strip);
        s_strip = NULL;
        return error;
    }
    s_ready = true;
    ESP_LOGI(TAG, "RGB 状态灯已就绪：RGB_DATA=IO%d，颗数=%u，亮度=%u%%",
             (int)data_gpio, (unsigned)RGB_BSP_LED_COUNT,
             (unsigned)RGB_BSP_TAP_BRIGHTNESS_PERCENT);
    return ESP_OK;
}

esp_err_t rgb_bsp_tap_flash(void)
{
    if (!s_ready || s_strip == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    /* 表现类参数集中在此：琥珀品牌色 × 默认亮度 × 默认时长，待样机定标。 */
    const uint32_t red = (uint32_t)RGB_BSP_AMBER_R * RGB_BSP_TAP_BRIGHTNESS_PERCENT / 100U;
    const uint32_t green = (uint32_t)RGB_BSP_AMBER_G * RGB_BSP_TAP_BRIGHTNESS_PERCENT / 100U;
    const uint32_t blue = (uint32_t)RGB_BSP_AMBER_B * RGB_BSP_TAP_BRIGHTNESS_PERCENT / 100U;
    esp_err_t error = led_strip_set_pixel(s_strip, 0, red, green, blue);
    if (error == ESP_OK)
    {
        error = led_strip_refresh(s_strip);
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "RGB 设色失败，错误=%s", esp_err_to_name(error));
        return error;
    }
    vTaskDelay(pdMS_TO_TICKS(RGB_BSP_TAP_FLASH_MS));
    error = led_strip_clear(s_strip);
    if (error == ESP_OK)
    {
        error = led_strip_refresh(s_strip);
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "RGB 短闪熄灭失败，错误=%s", esp_err_to_name(error));
        return error;
    }
    return ESP_OK;
}

esp_err_t rgb_bsp_off(void)
{
    if (!s_ready || s_strip == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t error = led_strip_clear(s_strip);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "RGB 熄灭失败，错误=%s", esp_err_to_name(error));
        return error;
    }
    return led_strip_refresh(s_strip);
}

bool rgb_bsp_is_ready(void)
{
    return s_ready;
}

const char *rgb_bsp_error_code(esp_err_t err)
{
    switch (err)
    {
    case ESP_OK:
        return "DRV_RGB_OK";
    case ESP_ERR_INVALID_STATE:
        return "DRV_RGB_NOT_READY";
    case ESP_ERR_INVALID_ARG:
        return "DRV_RGB_INVALID_ARG";
    case ESP_ERR_NO_MEM:
        return "DRV_RGB_NO_MEM";
    default:
        return "DRV_RGB_FAILED";
    }
}
