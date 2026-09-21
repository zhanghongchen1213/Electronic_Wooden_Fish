/**
 * @file     bsp_board.c
 * @brief    板级初始化实现
 * @details  执行 ESPWatch-S3-4G 板级策略检查和按键 GPIO 初始化，并输出稳定 BSP 错误日志。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "bsp_board.h"

#include "esp_log.h"
#include "key.h"
#include "l76kb_a58_bsp.h"
#include "ml307r_bsp.h"
#include "ns4150_bsp.h"
#include "vibration_bsp.h"

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "BSP_BOARD";

static esp_err_t fail_result(legbot_board_init_result_t *result,
                             legbot_bsp_init_stage_t stage,
                             esp_err_t err);

esp_err_t legbot_board_init(legbot_board_init_result_t *result)
{
    if (result != NULL)
    {
        result->failed_stage = LEGBOT_BSP_STAGE_COUNT;
        result->cause = ESP_OK;
        result->degraded = false;
        result->audio_degraded = false;
        result->audio_cause = ESP_OK;
    }

    /* SDMMC 当前被板级策略禁用，防止与现有权威引脚规划冲突。 */
    if (LEGBOT_BSP_SDMMC_ENABLED != 0)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, ESP_ERR_INVALID_STATE);
    }

    /* 再次保持 IO3 低，确保后续服务启动前 GPS 始终处于安全待机。 */
    esp_err_t err = l76kb_a58_bsp_hold_standby();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }

#if LEGBOT_CAP_MODEM
    /*
     * IO10 必须在 UART2/AT Core 之前稳定为高，避免 ML307R 使能输入处于
     * 未定义状态。
     */
    err = ml307r_bsp_enable();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }
#else
    /* 禁用 4G 的固件主动固定 EN 为低，不能依赖复位默认值或悬空电平。 */
    err = ml307r_bsp_hold_disabled();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }
#endif

    err = key_init();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }

    err = vibration_bsp_lease_init();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }

    /*
     * LEDC 首次初始化会分配内部上下文与 newlib 锁，必须在音频 I2S DMA
     * 占用内部内存前完成。板级阶段成功后，运行期只启停并复用该配置。
     */
    err = vibration_bsp_init();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }
    err = vibration_bsp_stop();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }

    err = ns4150_bsp_init();
    if (err != ESP_OK)
    {
        if (result != NULL)
        {
            result->cause = err;
            result->degraded = true;
            result->audio_degraded = true;
            result->audio_cause = err;
        }
        ESP_LOGW(TAG, "NS4150 无法进入安全关断状态，音频能力降级，错误=0x%x",
                 (unsigned)err);
    }

#if SCENIC_AREA_MANAGEMENT_DEBUG
    if (L76KB_A58_WAKE_POLICY == L76KB_A58_WAKE_POLICY_RELEASED_INPUT)
    {
        ESP_LOGI(TAG,
                 "GPS 使用 UART1 RX=IO9/TX=IO8，WAKE=IO3 当前低电平待机，定位 lease 后高阻释放");
    }
    else
    {
        ESP_LOGI(TAG,
                 "GPS 使用 UART1 RX=IO9/TX=IO8，WAKE=IO3 已驱动为 ACTIVE_HIGH 模式");
    }
#endif
    if (!vibration_bsp_profile_verified())
    {
        ESP_LOGW(TAG,
                 "VIB_PWM 使用 IO11，但 PWM 电气 profile 未确认，驱动保持安全门禁");
    }
    else
    {
        ESP_LOGI(TAG,
                 "VIB_PWM 使用 IO11，已启用 %u Hz、duty=%u/%u、空闲低电平 profile",
                 VIBRATION_BSP_PWM_FREQUENCY_HZ,
                 VIBRATION_BSP_ACTIVE_DUTY,
                 VIBRATION_BSP_PWM_PERIOD_STEPS);
    }

#if LEGBOT_CAP_MODEM
    ESP_LOGI(TAG,
             "板级引脚已就绪，GPS IO3 已保持低电平，GPS 独占 UART1，ML307R IO10 已使能并独占 UART2，SDMMC 已禁用，PA 默认关断");
#else
    ESP_LOGI(TAG,
             "板级引脚已就绪，GPS IO3 与 ML307R IO10 已保持低电平，UART1/UART2 未初始化，SDMMC 已禁用，PA 默认关断");
#endif
    return ESP_OK;
}

static esp_err_t fail_result(legbot_board_init_result_t *result,
                             legbot_bsp_init_stage_t stage,
                             esp_err_t err)
{
    if (result != NULL)
    {
        result->failed_stage = stage;
        result->cause = err;
        result->degraded = true;
    }

    ESP_LOGE(TAG,
             "初始化阶段 %s 失败，稳定错误码=%s，ESP 错误=%s",
             legbot_bsp_stage_name(stage),
             legbot_bsp_error_code(stage, err),
             esp_err_to_name(err));
    return err;
}
