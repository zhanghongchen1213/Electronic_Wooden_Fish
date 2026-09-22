/**
 * @file     bsp_board.h
 * @brief    板级初始化入口
 * @details  定义电子木鱼 EWF 板级初始化结果结构和统一初始化函数。
 * @author   ZHC
 * @date     2026-07-09
 */

#ifndef LEGBOT_BSP_BOARD_H
#define LEGBOT_BSP_BOARD_H

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        legbot_bsp_init_stage_t failed_stage; /**< 首个失败的初始化阶段。 */
        esp_err_t cause;                      /**< ESP-IDF 原始错误码。 */
        bool degraded;                        /**< 是否进入降级状态。 */
        bool audio_degraded;                  /**< 音频子资源是否降级但允许核心启动继续。 */
        esp_err_t audio_cause;                /**< 音频子资源的原始失败原因。 */
    } legbot_board_init_result_t;

    /**
     * @brief 初始化板级引脚和基础硬件策略
     * @param result 可选输出初始化结果，传入 NULL 时忽略详细结果
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 板级策略冲突
     *         其他 ESP-IDF 错误码表示底层硬件初始化失败
     */
    esp_err_t legbot_board_init(legbot_board_init_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BSP_BOARD_H */
