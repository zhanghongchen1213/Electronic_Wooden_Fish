/**
 * @file     bsp_board.c
 * @brief    板级初始化实现
 * @details  执行电子木鱼 EWF 板级策略检查和 PWR_INT/BOOT0 GPIO 初始化，并输出稳定 BSP 错误日志。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "bsp_board.h"

#include "esp_log.h"
#include "key.h"
#include "ns4150_bsp.h"
#include "pvdf_bsp.h"

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

    /* PWR_INT/BOOT0 仅配置为输入；IO46、IO45 保持未占用。 */
    esp_err_t err = key_init();
    if (err != ESP_OK)
    {
        return fail_result(result, LEGBOT_BSP_STAGE_BOARD_PINS, err);
    }

    /* PVDF 不占用共享 I2C/SPI/UART/I2S，归属板级引脚阶段：在此完成 IO9 ADC 与
       IO11 逐脚高电平唤醒的武装，保证早于 app_main 的全局 GPIO 唤醒源开放。 */
    err = pvdf_bsp_init();
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

    ESP_LOGI(TAG,
             "EWF 板级输入已就绪：PWR_INT=IO8、BOOT0=IO0；PVDF=ADC IO9/比较器 IO11 已纳管，"
             "IO46/IO45 仍保留，旧 GPS/ML307R/振动未启动");
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
