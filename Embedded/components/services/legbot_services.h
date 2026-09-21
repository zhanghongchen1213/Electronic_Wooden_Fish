/**
 * @file     legbot_services.h
 * @brief    统一服务框架接口
 * @details  定义固定服务 ID、输入通道能力、服务描述符和服务生命周期入口。
 * @author   ZHC
 * @date     2026-07-09
 */

#ifndef LEGBOT_SERVICES_H
#define LEGBOT_SERVICES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 通用服务停止消息类型。 */
#define LEGBOT_SERVICE_STOP_MESSAGE 0xffffffffU

    typedef enum
    {
        LEGBOT_SERVICE_UI = 0,   /**< UI 服务。 */
        LEGBOT_SERVICE_BLE,      /**< BLE 协议服务。 */
        LEGBOT_SERVICE_MODEM,    /**< 蜂窝模组服务。 */
        LEGBOT_SERVICE_GPS,      /**< GPS 定位服务。 */
        LEGBOT_SERVICE_AUDIO,    /**< 音频服务。 */
        LEGBOT_SERVICE_POWER,    /**< 电源管理服务。 */
        LEGBOT_SERVICE_CLOUD,    /**< 云端通信服务。 */
        LEGBOT_SERVICE_STATE,    /**< 状态聚合服务。 */
        LEGBOT_SERVICE_LOG,      /**< 关键日志服务。 */
        LEGBOT_SERVICE_SELFTEST, /**< 自检服务。 */
        LEGBOT_SERVICE_VOICE,    /**< BOOT0 按键离线语音控制服务。 */
        LEGBOT_SERVICE_COUNT     /**< 服务数量。 */
    } legbot_service_id_t;

    typedef enum
    {
        LEGBOT_SERVICE_INPUT_QUEUE = 1U << 0,          /**< 服务拥有消息队列输入。 */
        LEGBOT_SERVICE_INPUT_EVENT_GROUP = 1U << 1,    /**< 服务监听事件组输入。 */
        LEGBOT_SERVICE_INPUT_TYPED_UPDATE = 1U << 2,   /**< 服务接收类型化状态更新。 */
        LEGBOT_SERVICE_INPUT_MUTEX_BOUNDARY = 1U << 3, /**< 服务涉及互斥资源边界。 */
    } legbot_service_input_mask_t;

    typedef struct
    {
        legbot_service_id_t id;      /**< 服务 ID。 */
        const char *task_name;       /**< FreeRTOS 任务名称。 */
        const char *owner_component; /**< 服务归属组件路径。 */
        uint32_t input_mask;         /**< 服务输入能力掩码。 */
        bool starts_by_default;      /**< 是否随系统默认启动。 */
        bool on_demand;              /**< 是否支持按需启动。 */
    } legbot_service_descriptor_t;

    typedef struct
    {
        uint32_t type;  /**< 服务消息类型。 */
        uint32_t value; /**< 服务消息附加值。 */
    } legbot_service_message_t;

    /**
     * @brief 获取指定服务的描述符
     * @param id 服务 ID
     * @return 成功返回服务描述符指针，服务 ID 非法时返回 NULL
     */
    const legbot_service_descriptor_t *legbot_service_descriptor(legbot_service_id_t id);

    /**
     * @brief 获取服务描述符数量
     * @return 服务数量
     */
    size_t legbot_service_descriptor_count(void);

    /**
     * @brief 获取指定服务的消息队列
     * @param id 服务 ID
     * @return 成功返回通用队列句柄，服务 ID 非法、为 state/audio/GPS/selftest/voice 私有通道或尚未初始化时返回 NULL
     */
    QueueHandle_t legbot_service_queue(legbot_service_id_t id);

    /**
     * @brief 获取 state_service 专用类型化队列
     * @details 仅供 state_service 实现消费；其他模块必须使用 state_service_publish_battery()。
     * @return state_service 队列句柄，尚未初始化时返回 NULL
     */
    QueueHandle_t legbot_state_service_queue(void);

    /**
     * @brief 获取服务共享事件组
     * @return 服务事件组句柄，尚未初始化时可能为 NULL
     */
    EventGroupHandle_t legbot_services_event_group(void);

    /**
     * @brief 初始化服务框架契约资源
     * @return ESP_OK 成功
     *         ESP_ERR_NO_MEM 队列或事件组创建失败
     *         其他 ESP-IDF 错误码表示子服务初始化失败
     */
    esp_err_t legbot_services_init_contracts(void);

    /**
     * @brief 启动所有默认服务
     * @return ESP_OK 成功
     *         ESP_ERR_NO_MEM 任务创建失败
     *         其他 ESP-IDF 错误码表示服务契约初始化失败
     */
    esp_err_t legbot_services_start_all(void);

    /**
     * @brief 请求停止所有已启动服务
     * @return ESP_OK 成功
     *         ESP_ERR_TIMEOUT 停止消息投递失败
     */
    esp_err_t legbot_services_stop_all(void);

    /**
     * @brief 按需启动自检服务
     * @return ESP_OK 成功
     *         ESP_ERR_NO_MEM 任务创建失败
     *         其他 ESP-IDF 错误码表示服务契约初始化失败
     */
    esp_err_t legbot_selftest_service_start(void);

    /**
     * @brief 使用显式 GPS FIX 环境上下文按需启动完整自检
     * @param indoor_confirmed true 表示操作员已确认室内环境
     * @return ESP_OK 成功，其他值表示任务、队列或参数错误
     */
    esp_err_t legbot_selftest_service_start_with_gps_context(
        bool indoor_confirmed);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_SERVICES_H */
