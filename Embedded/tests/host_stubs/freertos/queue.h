/**
 * @file     queue.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_QUEUE_H
#define EWF_HOST_QUEUE_H
#include "FreeRTOS.h"
typedef struct host_queue *QueueHandle_t;
QueueHandle_t xQueueCreate(UBaseType_t capacity, UBaseType_t item_size);
BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t timeout);
BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t timeout);
BaseType_t xQueueSendFromISR(QueueHandle_t queue, const void *item, BaseType_t *woken);
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t queue);
BaseType_t xQueueReset(QueueHandle_t queue);
void vQueueDelete(QueueHandle_t queue);
#endif
