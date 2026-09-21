/**
 * @file     vibration_bsp.c
 * @brief    振动马达 LEDC/PWM BSP 实现
 * @details  将 IO11 静态租约、20 kHz 最大有效 duty、运行期停止复用和完整反初始化集中在 BSP。
 * @author   ZHC
 * @date     2026-07-23
 */

#include "vibration_bsp.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/semphr.h"

static const char *TAG = "BSP_VIB";

/** 振动 PWM 定时器所有权。 */
#define VIBRATION_BSP_LEDC_TIMER LEDC_TIMER_0
/** 振动 PWM 通道所有权。 */
#define VIBRATION_BSP_LEDC_CHANNEL LEDC_CHANNEL_0
/** 振动 PWM duty 分辨率。 */
#define VIBRATION_BSP_LEDC_RESOLUTION LEDC_TIMER_10_BIT

_Static_assert(VIBRATION_BSP_ACTIVE_DUTY <= VIBRATION_BSP_DUTY_MAX,
               "Vibration active duty must fit the ESP32-S3 10-bit LEDC counter");
_Static_assert(VIBRATION_BSP_IDLE_DUTY == 0U && VIBRATION_BSP_IDLE_LEVEL == 0U,
               "Vibration idle output must remain low");

/** LEDC 是否已由本 BSP 初始化。 */
static bool s_initialized;
/** 振动硬件互斥租约的静态存储。 */
static StaticSemaphore_t s_lease_storage;
/** 振动硬件互斥租约。 */
static SemaphoreHandle_t s_lease;

const legbot_bsp_resource_t *vibration_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_VIBRATION);
}

bool vibration_bsp_profile_verified(void)
{
    return VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED != 0;
}

esp_err_t vibration_bsp_lease_init(void)
{
    if (s_lease != NULL)
    {
        return ESP_OK;
    }
    s_lease = xSemaphoreCreateMutexStatic(&s_lease_storage);
    return s_lease != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t vibration_bsp_lease_acquire(TickType_t timeout_ticks)
{
    if (s_lease == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xSemaphoreTake(s_lease, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t vibration_bsp_lease_release(void)
{
    if (s_lease == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xSemaphoreGive(s_lease) == pdTRUE
               ? ESP_OK
               : ESP_ERR_INVALID_STATE;
}

esp_err_t vibration_bsp_init(void)
{
#if !VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (s_initialized)
    {
        return ESP_OK;
    }
    const ledc_timer_config_t timer_config = {
        .speed_mode = VIBRATION_BSP_LEDC_SPEED_MODE,
        .duty_resolution = VIBRATION_BSP_LEDC_RESOLUTION,
        .timer_num = VIBRATION_BSP_LEDC_TIMER,
        .freq_hz = VIBRATION_BSP_PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "振动 PWM 定时器初始化失败，错误=0x%x", (unsigned)err);
        return err;
    }
    const ledc_channel_config_t channel_config = {
        .gpio_num = LEGBOT_BSP_VIBRATION_GPIO,
        .speed_mode = VIBRATION_BSP_LEDC_SPEED_MODE,
        .channel = VIBRATION_BSP_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = VIBRATION_BSP_LEDC_TIMER,
        .duty = VIBRATION_BSP_IDLE_DUTY,
        .hpoint = 0,
        .flags = {
            .output_invert = VIBRATION_BSP_ACTIVE_LEVEL == 0U,
        },
    };
    err = ledc_channel_config(&channel_config);
    if (err == ESP_OK)
    {
        s_initialized = true;
        ESP_LOGI(TAG,
                 "振动 PWM 已就绪：IO11，频率=%u Hz，有效 duty=%u/%u，空闲低电平",
                 VIBRATION_BSP_PWM_FREQUENCY_HZ,
                 VIBRATION_BSP_ACTIVE_DUTY,
                 VIBRATION_BSP_PWM_PERIOD_STEPS);
    }
    else
    {
        ESP_LOGE(TAG, "振动 PWM 通道初始化失败，错误=0x%x", (unsigned)err);
    }
    return err;
#endif
}

esp_err_t vibration_bsp_start_pulse(uint32_t duration_ms)
{
#if !VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED
    (void)duration_ms;
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (duration_ms == 0U || duration_ms > VIBRATION_BSP_PULSE_MAX_MS)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = ledc_set_duty(VIBRATION_BSP_LEDC_SPEED_MODE,
                                  VIBRATION_BSP_LEDC_CHANNEL,
                                  VIBRATION_BSP_ACTIVE_DUTY);
    if (err != ESP_OK)
    {
        return err;
    }
    err = ledc_update_duty(VIBRATION_BSP_LEDC_SPEED_MODE,
                           VIBRATION_BSP_LEDC_CHANNEL);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "振动脉冲已启动，计划时长=%lu ms", (unsigned long)duration_ms);
    }
    return err;
#endif
}

esp_err_t vibration_bsp_stop(void)
{
#if !VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED
    return ESP_OK;
#else
    if (!s_initialized)
    {
        return ESP_OK;
    }
    esp_err_t err = ledc_stop(VIBRATION_BSP_LEDC_SPEED_MODE,
                              VIBRATION_BSP_LEDC_CHANNEL,
                              VIBRATION_BSP_IDLE_LEVEL);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "振动输出已停止并恢复空闲低电平");
    }
    return err;
#endif
}

esp_err_t vibration_bsp_deinit(void)
{
    esp_err_t err = vibration_bsp_stop();
    if (err != ESP_OK || !s_initialized)
    {
        return err;
    }
    err = ledc_timer_pause(VIBRATION_BSP_LEDC_SPEED_MODE,
                           VIBRATION_BSP_LEDC_TIMER);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "振动 PWM 定时器暂停失败，错误=0x%x", (unsigned)err);
        return err;
    }
    const ledc_timer_config_t timer_config = {
        .speed_mode = VIBRATION_BSP_LEDC_SPEED_MODE,
        .timer_num = VIBRATION_BSP_LEDC_TIMER,
        .deconfigure = true,
    };
    err = ledc_timer_config(&timer_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "振动 PWM 定时器反初始化失败，错误=0x%x", (unsigned)err);
        return err;
    }
    s_initialized = false;
    err = gpio_reset_pin(LEGBOT_BSP_VIBRATION_GPIO);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "振动 GPIO 释放失败，错误=0x%x", (unsigned)err);
        return err;
    }
    ESP_LOGI(TAG, "振动 PWM 定时器与 IO11 已完整释放");
    return ESP_OK;
}
