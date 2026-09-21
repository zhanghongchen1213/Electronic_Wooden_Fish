/**
 * @file     event_bus.h
 * @brief    系统事件总线接口
 * @details  定义跨服务事件来源、事件类型和发布接口，统一承载 ISR、服务和诊断事件。
 * @author   ZHC
 * @date     2026-07-09
 */

#ifndef LEGBOT_EVENT_BUS_H
#define LEGBOT_EVENT_BUS_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        LEGBOT_EVENT_SOURCE_ISR = 0,  /**< 中断上下文事件来源。 */
        LEGBOT_EVENT_SOURCE_UI,       /**< UI 服务事件来源。 */
        LEGBOT_EVENT_SOURCE_BLE,      /**< BLE 服务事件来源。 */
        LEGBOT_EVENT_SOURCE_MODEM,    /**< 蜂窝模组服务事件来源。 */
        LEGBOT_EVENT_SOURCE_GPS,      /**< GPS 服务事件来源。 */
        LEGBOT_EVENT_SOURCE_AUDIO,    /**< 音频服务事件来源。 */
        LEGBOT_EVENT_SOURCE_POWER,    /**< 电源服务事件来源。 */
        LEGBOT_EVENT_SOURCE_CLOUD,    /**< 云端服务事件来源。 */
        LEGBOT_EVENT_SOURCE_STATE,    /**< 状态服务事件来源。 */
        LEGBOT_EVENT_SOURCE_LOG,      /**< 日志服务事件来源。 */
        LEGBOT_EVENT_SOURCE_SELFTEST, /**< 自检服务事件来源。 */
    } legbot_event_source_t;

    typedef enum
    {
        LEGBOT_EVENT_TYPE_SIGNAL = 0,   /**< 普通信号事件。 */
        LEGBOT_EVENT_TYPE_STATE_UPDATE, /**< 状态更新事件。 */
        LEGBOT_EVENT_TYPE_UI_REQUEST,   /**< UI 请求事件。 */
        LEGBOT_EVENT_TYPE_DIAGNOSTIC,   /**< 诊断事件。 */
    } legbot_event_type_t;

    typedef struct
    {
        legbot_event_source_t source; /**< 事件来源模块。 */
        legbot_event_type_t type;     /**< 事件类型。 */
        uint32_t code;                /**< 事件代码，由来源模块定义。 */
        uint32_t value;               /**< 事件附加值。 */
    } legbot_event_t;

    /**
     * @brief 初始化事件队列和事件组
     * @return ESP_OK 成功
     *         ESP_ERR_NO_MEM 队列或事件组创建失败
     */
    esp_err_t event_bus_init(void);

    /**
     * @brief 查询事件总线是否已初始化
     * @return true 已初始化
     *         false 未初始化
     */
    bool event_bus_is_initialized(void);

    /**
     * @brief 获取事件总线队列句柄
     * @return 事件队列句柄，未初始化时可能为 NULL
     */
    QueueHandle_t event_bus_queue(void);

    /**
     * @brief 获取事件总线事件组句柄
     * @return 事件组句柄，未初始化时可能为 NULL
     */
    EventGroupHandle_t event_bus_group(void);

    /**
     * @brief 从任务上下文发布事件
     * @param event 待发布事件
     * @param timeout_ticks 等待队列写入的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 事件总线尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         ESP_ERR_TIMEOUT 队列写入超时
     */
    esp_err_t event_bus_publish(const legbot_event_t *event, TickType_t timeout_ticks);

    /**
     * @brief 从 ISR 上下文发布事件
     * @param event 待发布事件
     * @param higher_priority_woken FreeRTOS ISR 唤醒标志输出
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 事件总线尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         ESP_FAIL 队列写入失败
     */
    esp_err_t event_bus_publish_from_isr(const legbot_event_t *event, BaseType_t *higher_priority_woken);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_EVENT_BUS_H */
