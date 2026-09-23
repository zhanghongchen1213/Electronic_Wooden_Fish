/**
 * @file     event_bus.c
 * @brief    系统事件总线实现
 * @details  创建统一事件队列和事件组，支持任务上下文与 ISR 上下文发布事件。
 * @author   ZHC
 * @date     2026-07-09
 */

#include "event_bus.h"

/** 事件队列深度，覆盖早期服务骨架的轻量事件流。 */
#define EVENT_BUS_QUEUE_DEPTH 16

/** 扇出订阅槽容量；当前订阅方只有反馈服务这一可合并型纯消费者。 */
#define EVENT_BUS_SUBSCRIBER_MAX 2U

/** 系统事件队列句柄。 */
static QueueHandle_t s_event_queue;

/** 扇出订阅队列表；只登记订阅方自有队列句柄，不持有所有权。 */
static QueueHandle_t s_subscribers[EVENT_BUS_SUBSCRIBER_MAX];

/** 系统事件组句柄。 */
static EventGroupHandle_t s_event_group;

/** 事件总线初始化状态。 */
static bool s_initialized;

esp_err_t event_bus_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    s_event_queue = xQueueCreate(EVENT_BUS_QUEUE_DEPTH, sizeof(legbot_event_t));
    if (s_event_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    s_event_group = xEventGroupCreate();
    if (s_event_group == NULL)
    {
        /* 事件组创建失败时回收队列，避免半初始化状态泄漏。 */
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
        return ESP_ERR_NO_MEM;
    }

    s_initialized = true;
    return ESP_OK;
}

bool event_bus_is_initialized(void)
{
    return s_initialized;
}

QueueHandle_t event_bus_queue(void)
{
    return s_event_queue;
}

EventGroupHandle_t event_bus_group(void)
{
    return s_event_group;
}

esp_err_t event_bus_publish(const legbot_event_t *event, TickType_t timeout_ticks)
{
    if (!s_initialized || s_event_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const esp_err_t main_result =
        xQueueSend(s_event_queue, event, timeout_ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
    /* 扇出投递为非阻塞：订阅队列满时丢弃本次副本（可合并型），不影响主队列结论。 */
    for (size_t index = 0; index < EVENT_BUS_SUBSCRIBER_MAX; ++index)
    {
        if (s_subscribers[index] != NULL)
        {
            (void)xQueueSend(s_subscribers[index], event, 0);
        }
    }
    return main_result;
}

esp_err_t event_bus_subscribe(QueueHandle_t queue)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t index = 0; index < EVENT_BUS_SUBSCRIBER_MAX; ++index)
    {
        if (s_subscribers[index] == queue)
        {
            return ESP_OK;
        }
    }
    for (size_t index = 0; index < EVENT_BUS_SUBSCRIBER_MAX; ++index)
    {
        if (s_subscribers[index] == NULL)
        {
            s_subscribers[index] = queue;
            return ESP_OK;
        }
    }
    return ESP_ERR_NO_MEM;
}

esp_err_t event_bus_publish_from_isr(const legbot_event_t *event, BaseType_t *higher_priority_woken)
{
    if (!s_initialized || s_event_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    /* ISR 路径使用 FreeRTOS FromISR API，避免在中断上下文阻塞。 */
    return xQueueSendFromISR(s_event_queue, event, higher_priority_woken) == pdTRUE ? ESP_OK : ESP_FAIL;
}
