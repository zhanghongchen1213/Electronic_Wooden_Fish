/**
 * @file     semphr.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_SEMPHR_H
#define EWF_HOST_SEMPHR_H
#include "FreeRTOS.h"
typedef struct host_mutex *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t mutex, TickType_t timeout);
BaseType_t xSemaphoreGive(SemaphoreHandle_t mutex);
void vSemaphoreDelete(SemaphoreHandle_t mutex);
#endif
