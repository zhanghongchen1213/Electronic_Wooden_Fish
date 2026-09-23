/**
 * @file     task.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_TASK_H
#define EWF_HOST_TASK_H
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
typedef enum { eSetValueWithOverwrite } eNotifyAction;
TaskHandle_t xTaskGetCurrentTaskHandle(void);
TickType_t xTaskGetTickCount(void);
TickType_t xTaskGetTickCountFromISR(void);
void vTaskDelay(TickType_t ticks);
BaseType_t xTaskNotifyGive(TaskHandle_t task);
BaseType_t xTaskNotify(TaskHandle_t task, uint32_t value, eNotifyAction action);
BaseType_t xTaskNotifyWait(uint32_t clear_entry, uint32_t clear_exit, uint32_t *value, TickType_t timeout);
uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t timeout);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task);
#endif
