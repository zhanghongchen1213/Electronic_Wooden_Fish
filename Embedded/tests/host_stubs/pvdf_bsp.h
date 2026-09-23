/**
 * @file     pvdf_bsp.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_PVDF_BSP_H
#define EWF_HOST_PVDF_BSP_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#define EWF_PVDF_ADC_GPIO 9
#define EWF_PVDF_CMP_WAKE_GPIO 11
typedef struct { bool initialized; bool calibration_available; bool degraded; } pvdf_bsp_selfcheck_t;
esp_err_t pvdf_bsp_selfcheck(pvdf_bsp_selfcheck_t *snapshot);
const char *pvdf_bsp_error_code(void);
esp_err_t pvdf_bsp_wake_enable(void);
esp_err_t pvdf_bsp_wake_disable(void);
esp_err_t pvdf_bsp_isr_register(void (*callback)(void *), void *context);
esp_err_t pvdf_bsp_isr_unregister(void);
esp_err_t pvdf_bsp_read_mv(int32_t *millivolt);
#endif
