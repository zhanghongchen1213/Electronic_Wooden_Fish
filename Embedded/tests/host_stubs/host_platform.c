/**
 * @file     host_platform.c
 * @brief    主机集成测试的有界队列、互斥锁和硬件替身。
 * @details  单线程确定性调度；队列按值复制，重复解锁由断言捕获。
 * @author   ZHC
 * @date     2026-09-22
 */
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "state_service.h"
#include "event_bus.h"
#include "pvdf_bsp.h"

struct host_queue
{
    unsigned capacity;
    unsigned item_size;
    unsigned count;
    unsigned head;
    unsigned char *items;
};
struct host_mutex { bool locked; };
static QueueHandle_t s_state_queue;
static QueueHandle_t s_subscribers[2U];
static TickType_t s_ticks;
static esp_err_t s_publish_error;
static uint32_t s_published_count;
static uint32_t s_published_sequences[64];
static bool s_fail_next_send;

QueueHandle_t xQueueCreate(UBaseType_t capacity, UBaseType_t item_size)
{
    QueueHandle_t queue = calloc(1, sizeof(*queue));
    assert(queue != NULL);
    queue->items = calloc(capacity, item_size);
    assert(queue->items != NULL);
    queue->capacity = capacity;
    queue->item_size = item_size;
    return queue;
}
BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t timeout)
{
    (void)timeout;
    if (s_fail_next_send) { s_fail_next_send = false; return pdFALSE; }
    if (queue == NULL || queue->count == queue->capacity) { return pdFALSE; }
    memcpy(queue->items + ((queue->head + queue->count) % queue->capacity) * queue->item_size,
           item, queue->item_size);
    ++queue->count;
    return pdTRUE;
}
BaseType_t xQueueReceive(QueueHandle_t queue, void *item, TickType_t timeout)
{
    (void)timeout;
    if (queue == NULL || queue->count == 0U) { return pdFALSE; }
    memcpy(item, queue->items + queue->head * queue->item_size, queue->item_size);
    queue->head = (queue->head + 1U) % queue->capacity;
    --queue->count;
    return pdTRUE;
}
BaseType_t xQueueSendFromISR(QueueHandle_t queue, const void *item, BaseType_t *woken)
{
    *woken = pdFALSE;
    return xQueueSend(queue, item, 0);
}
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t queue) { return queue->count; }
BaseType_t xQueueReset(QueueHandle_t queue) { queue->count = 0U; queue->head = 0U; return pdTRUE; }
void vQueueDelete(QueueHandle_t queue) { free(queue->items); free(queue); }
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return calloc(1, sizeof(struct host_mutex)); }
BaseType_t xSemaphoreTake(SemaphoreHandle_t mutex, TickType_t timeout)
{
    (void)timeout;
    if (mutex->locked) { return pdFALSE; }
    mutex->locked = true;
    return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t mutex)
{
    assert(mutex->locked);
    mutex->locked = false;
    return pdTRUE;
}
void vSemaphoreDelete(SemaphoreHandle_t mutex) { assert(!mutex->locked); free(mutex); }
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return &s_ticks; }
TickType_t xTaskGetTickCount(void) { return s_ticks++; }
TickType_t xTaskGetTickCountFromISR(void) { return s_ticks; }
void vTaskDelay(TickType_t ticks) { s_ticks += ticks; }
BaseType_t xTaskNotifyGive(TaskHandle_t task) { (void)task; return pdTRUE; }
BaseType_t xTaskNotify(TaskHandle_t task, uint32_t value, eNotifyAction action)
{
    (void)task; (void)value; (void)action; return pdTRUE;
}
BaseType_t xTaskNotifyWait(uint32_t clear_entry, uint32_t clear_exit, uint32_t *value, TickType_t timeout)
{
    (void)clear_entry; (void)clear_exit; (void)value; (void)timeout; return pdFALSE;
}
uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t timeout) { (void)clear; (void)timeout; return 0U; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t task) { (void)task; return 4096U; }
QueueHandle_t legbot_state_service_queue(void) { return s_state_queue; }
const char *esp_err_to_name(esp_err_t error) { return error == ESP_OK ? "ESP_OK" : "HOST_ERROR"; }
esp_err_t config_service_retry_pending_change(void) { return ESP_OK; }
esp_err_t config_service_cloud_offset_write(int64_t offset, TickType_t timeout)
{
    (void)offset; (void)timeout; return ESP_OK;
}
esp_err_t pvdf_bsp_selfcheck(pvdf_bsp_selfcheck_t *snapshot)
{
    snapshot->initialized = true; return ESP_OK;
}
const char *pvdf_bsp_error_code(void) { return "HOST_PVDF"; }
esp_err_t pvdf_bsp_wake_enable(void) { return ESP_OK; }
esp_err_t pvdf_bsp_wake_disable(void) { return ESP_OK; }
esp_err_t pvdf_bsp_isr_register(void (*callback)(void *), void *context)
{
    (void)callback; (void)context; return ESP_OK;
}
esp_err_t pvdf_bsp_isr_unregister(void) { return ESP_OK; }
esp_err_t pvdf_bsp_read_mv(int32_t *millivolt) { *millivolt = 1000; return ESP_OK; }
esp_err_t event_bus_publish(const legbot_event_t *event, TickType_t timeout)
{
    (void)timeout;
    if (s_publish_error != ESP_OK) { return s_publish_error; }
    assert(s_published_count < 64U);
    s_published_sequences[s_published_count++] = event->value;
    /* 扇出镜像真实 event_bus：订阅队列非阻塞投递，队满丢最新（可合并型）。 */
    for (unsigned index = 0; index < 2U; ++index)
    {
        if (s_subscribers[index] != NULL)
        {
            (void)xQueueSend(s_subscribers[index], event, 0);
        }
    }
    return ESP_OK;
}

esp_err_t event_bus_subscribe(QueueHandle_t queue)
{
    if (queue == NULL) { return ESP_ERR_INVALID_ARG; }
    for (unsigned index = 0; index < 2U; ++index)
    {
        if (s_subscribers[index] == queue) { return ESP_OK; }
    }
    for (unsigned index = 0; index < 2U; ++index)
    {
        if (s_subscribers[index] == NULL) { s_subscribers[index] = queue; return ESP_OK; }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t event_bus_unsubscribe(QueueHandle_t queue)
{
    if (queue == NULL) { return ESP_ERR_INVALID_ARG; }
    for (unsigned index = 0; index < 2U; ++index)
    {
        if (s_subscribers[index] == queue)
        {
            s_subscribers[index] = NULL;
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static QueueHandle_t s_event_bus_queue;
bool event_bus_is_initialized(void) { return true; }
QueueHandle_t event_bus_queue(void)
{
    if (s_event_bus_queue == NULL)
    {
        s_event_bus_queue = xQueueCreate(16U, sizeof(legbot_event_t));
    }
    return s_event_bus_queue;
}
void host_platform_init(void)
{
    if (s_state_queue == NULL) { s_state_queue = xQueueCreate(8U, sizeof(state_service_update_t)); }
    xQueueReset(s_state_queue);
    if (s_event_bus_queue == NULL) { event_bus_queue(); }
    xQueueReset(s_event_bus_queue);
    memset(s_subscribers, 0, sizeof(s_subscribers));
    s_publish_error = ESP_OK;
    s_published_count = 0U;
}
void host_publish_error(esp_err_t error) { s_publish_error = error; }
void host_fail_next_send(void) { s_fail_next_send = true; }
uint32_t host_published_count(void) { return s_published_count; }
uint32_t host_published_sequence(uint32_t index)
{
    assert(index < s_published_count);
    return s_published_sequences[index];
}

int64_t esp_timer_get_time(void)
{
    /* 主机：用 tick 近似微秒，供今日桶日键换算。 */
    return (int64_t)s_ticks * 1000LL;
}
