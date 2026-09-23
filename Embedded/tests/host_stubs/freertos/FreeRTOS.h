/**
 * @file     FreeRTOS.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_FREERTOS_H
#define EWF_HOST_FREERTOS_H
#include <stddef.h>
#include <stdint.h>
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef unsigned UBaseType_t;
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define portMAX_DELAY UINT32_MAX
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define pdTICKS_TO_MS(ticks) (ticks)
#define portTICK_PERIOD_MS ((TickType_t)1)
#define taskYIELD() ((void)0)
#define portYIELD_FROM_ISR() ((void)0)
#endif
