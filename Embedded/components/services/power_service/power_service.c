/**
 * @file     power_service.c
 * @brief    PWR_INT/BOOT0 板级输入边界服务实现
 * @details  ISR 只做时间戳、原子状态更新和 ISR-safe typed 消息投递；去抖、低电平时长和
 *           typed 结论交接全部在 power_task 上下文完成。服务不读取或推断充电状态，
 *           不驱动 LTC2954 KILL，也不调用任何软件关机 API。
 * @author   ZHC
 * @date     2026-07-09
 */

#include "power_service.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "key.h"
#include "tap_input_service.h"

static const char *TAG = "PWR_BOOT";

/** power_task 私有 typed 消息类型。 */
typedef enum
{
    POWER_SERVICE_MESSAGE_EDGE_PWR = 1U, /**< PWR_INT 原始边沿。 */
    POWER_SERVICE_MESSAGE_EDGE_BOOT = 2U /**< BOOT0 原始边沿。 */
} power_service_message_type_t;

/** power_task 私有 ISR-safe 消息载荷。 */
typedef struct
{
    uint32_t type;  /**< 消息类型。 */
    uint32_t level; /**< 边沿时刻的原始电平。 */
    uint32_t ticks; /**< 边沿时刻的 ISR tick。 */
} power_service_message_t;

/** 服务状态机。 */
typedef enum
{
    POWER_SERVICE_IDLE = 0,    /**< 尚未创建队列。 */
    POWER_SERVICE_PREPARED,    /**< 队列就绪，等待框架创建任务。 */
    POWER_SERVICE_RUNNING,     /**< power_task 正在运行。 */
} power_service_phase_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_snapshot_mutex;
static power_service_phase_t s_phase;
static bool s_isr_registered;

static ewf_power_boot_policy_t s_policy;
static power_service_snapshot_t s_snapshot;

static void power_service_pwr_isr(void *context);
static void power_service_boot_isr(void *context);
static void power_service_publish_snapshot(const power_service_snapshot_t *snapshot);
static const char *power_service_event_name(ewf_power_boot_event_kind_t kind);
static void power_service_publish_automatic_tap(
    const ewf_power_boot_event_t *event);

esp_err_t power_service_init_contracts(void)
{
    if (s_queue != NULL && s_snapshot_mutex != NULL)
    {
        return ESP_OK;
    }
    if (s_queue == NULL)
    {
        s_queue = xQueueCreate(POWER_SERVICE_QUEUE_DEPTH,
                               sizeof(power_service_message_t));
        if (s_queue == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_snapshot_mutex == NULL)
    {
        s_snapshot_mutex = xSemaphoreCreateMutex();
        if (s_snapshot_mutex == NULL)
        {
            vQueueDelete(s_queue);
            s_queue = NULL;
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

esp_err_t power_service_prepare_run(void)
{
    if (s_queue == NULL || s_snapshot_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_phase == POWER_SERVICE_RUNNING)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_phase = POWER_SERVICE_PREPARED;
    return ESP_OK;
}

void power_service_cancel_prepared_run(void)
{
    if (s_phase == POWER_SERVICE_PREPARED)
    {
        s_phase = POWER_SERVICE_IDLE;
    }
}

esp_err_t power_service_deinit_contracts(void)
{
    if (s_phase == POWER_SERVICE_RUNNING)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_queue != NULL)
    {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }
    if (s_snapshot_mutex != NULL)
    {
        vSemaphoreDelete(s_snapshot_mutex);
        s_snapshot_mutex = NULL;
    }
    s_phase = POWER_SERVICE_IDLE;
    return ESP_OK;
}

QueueHandle_t power_service_queue(void)
{
    return s_queue;
}

esp_err_t power_service_run(void)
{
    if (s_queue == NULL || s_phase != POWER_SERVICE_PREPARED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.last_event = EWF_POWER_BOOT_EVENT_NONE;
    ewf_power_boot_policy_reset(&s_policy);

    esp_err_t err = key_register_pwr_isr_callback(power_service_pwr_isr, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PWR_INT 边沿中断登记失败，错误=%s", esp_err_to_name(err));
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }
    err = key_register_boot_isr_callback(power_service_boot_isr, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BOOT0 边沿中断登记失败，错误=%s", esp_err_to_name(err));
        (void)key_unregister_pwr_isr_callback();
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }
    s_isr_registered = true;
    s_phase = POWER_SERVICE_RUNNING;

    /* 结束启动采样窗口：BOOT0 若仍为低，本次属于下载/启动绑带，不产生运行态事件。 */
    legbot_key_state_t state = {0};
    err = key_read_state(&state);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "读取 PWR/BOOT 原始电平失败，错误=%s", esp_err_to_name(err));
        (void)key_unregister_boot_isr_callback();
        (void)key_unregister_pwr_isr_callback();
        s_isr_registered = false;
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }
    ewf_power_boot_policy_arm_runtime(&s_policy, state.boot_level, 0U);
    ESP_LOGI(TAG,
             "PWR_INT/BOOT0 只读输入边界已就绪：PWR_INT=IO8 持续低有效、BOOT0=IO0 任意边沿；"
             "启动采样窗口已结束，主控不检测充电状态，也不驱动 KILL");

    /* 先固化一份初始快照，供自检在没有任何边沿时也能观测到输入边界。 */
    power_service_snapshot_t initial = {0};
    initial.pwr_level = state.pwr_level;
    initial.boot_level = state.boot_level;
    initial.pwr_low = ewf_power_boot_policy_pwr_is_low(&s_policy);
    initial.pwr_low_duration_ms = 0U;
    initial.pwr_interval_count = s_policy.pwr_interval_count;
    initial.boot_tap_count = s_policy.boot_tap_count;
    initial.last_event = EWF_POWER_BOOT_EVENT_NONE;
    initial.last_event_duration_ms = 0U;
    initial.boot0_runtime_armed = s_policy.boot_runtime_armed;
    initial.isr_registered = s_isr_registered;
    power_service_publish_snapshot(&initial);

    for (;;)
    {
        power_service_message_t message = {0};
        if (xQueueReceive(s_queue, &message, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        if (message.type == LEGBOT_SERVICE_STOP_MESSAGE)
        {
            break;
        }

        legbot_key_state_t current = {0};
        if (key_read_state(&current) != ESP_OK)
        {
            ESP_LOGW(TAG, "边沿到达后读取原始电平失败，跳过本次交接");
            continue;
        }
        const uint32_t now_ms = (uint32_t)pdTICKS_TO_MS(message.ticks);

        power_service_snapshot_t snapshot = s_snapshot;
        snapshot.pwr_level = current.pwr_level;
        snapshot.boot_level = current.boot_level;

        if (message.type == POWER_SERVICE_MESSAGE_EDGE_PWR)
        {
            ewf_power_boot_event_t event = {0};
            if (ewf_power_boot_policy_on_pwr_level(&s_policy,
                                                   (uint8_t)message.level,
                                                   now_ms,
                                                   &event))
            {
                snapshot.last_event = event.kind;
                snapshot.last_event_duration_ms = event.duration_ms;
                ESP_LOGI(TAG,
                         "PWR_INT 结论=%s，低电平时长=%u ms，累计按压区间=%u",
                         power_service_event_name(event.kind),
                         (unsigned)event.duration_ms,
                         (unsigned)s_policy.pwr_interval_count);
            }
            (void)key_rearm_pwr_isr_for_next_level();
        }
        else if (message.type == POWER_SERVICE_MESSAGE_EDGE_BOOT)
        {
            ewf_power_boot_event_t event = {0};
            if (ewf_power_boot_policy_on_boot_level(&s_policy,
                                                    (uint8_t)message.level,
                                                    now_ms,
                                                    &event))
            {
                snapshot.last_event = event.kind;
                snapshot.last_event_duration_ms = event.duration_ms;
                ESP_LOGI(TAG,
                         "BOOT0 结论=%s，区间时长=%u ms，累计运行态短按=%u",
                         power_service_event_name(event.kind),
                         (unsigned)event.duration_ms,
                         (unsigned)s_policy.boot_tap_count);
                if (event.kind == EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP)
                {
                    power_service_publish_automatic_tap(&event);
                }
            }
            (void)key_rearm_boot_isr_for_next_level();
        }

        snapshot.pwr_low = ewf_power_boot_policy_pwr_is_low(&s_policy);
        snapshot.pwr_low_duration_ms =
            ewf_power_boot_policy_pwr_low_duration_ms(&s_policy, now_ms);
        snapshot.pwr_interval_count = s_policy.pwr_interval_count;
        snapshot.boot_tap_count = s_policy.boot_tap_count;
        snapshot.boot0_runtime_armed = s_policy.boot_runtime_armed;
        snapshot.isr_registered = s_isr_registered;
        power_service_publish_snapshot(&snapshot);
    }

    (void)key_unregister_boot_isr_callback();
    (void)key_unregister_pwr_isr_callback();
    s_isr_registered = false;
    s_phase = POWER_SERVICE_IDLE;
    ESP_LOGW(TAG, "power_task 已收到 typed STOP 并退出，边沿中断已解注册");
    return ESP_OK;
}

esp_err_t power_service_request_stop(TickType_t timeout_ticks)
{
    /* 允许在任务尚未进入 run 循环前投递停止命令，避免启动竞态。 */
    if (s_queue == NULL ||
        (s_phase != POWER_SERVICE_RUNNING && s_phase != POWER_SERVICE_PREPARED))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const power_service_message_t message = {
        .type = LEGBOT_SERVICE_STOP_MESSAGE,
        .level = 0U,
        .ticks = 0U,
    };
    return xQueueSend(s_queue, &message, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t power_service_snapshot(power_service_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_snapshot_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_snapshot_mutex, 0) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *snapshot = s_snapshot;
    xSemaphoreGive(s_snapshot_mutex);
    return ESP_OK;
}

static void power_service_publish_snapshot(const power_service_snapshot_t *snapshot)
{
    if (s_snapshot_mutex == NULL ||
        xSemaphoreTake(s_snapshot_mutex, 0) != pdTRUE)
    {
        return;
    }
    s_snapshot = *snapshot;
    xSemaphoreGive(s_snapshot_mutex);
}

static const char *power_service_event_name(ewf_power_boot_event_kind_t kind)
{
    switch (kind)
    {
    case EWF_POWER_BOOT_EVENT_PWR_INTERVAL:
        return "PWR_INTERVAL";
    case EWF_POWER_BOOT_EVENT_PWR_GLITCH_IGNORED:
        return "PWR_GLITCH_IGNORED";
    case EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP:
        return "BOOT0_RUNTIME_TAP";
    case EWF_POWER_BOOT_EVENT_BOOT0_LONG_SUPPRESSED:
        return "BOOT0_LONG_SUPPRESSED";
    case EWF_POWER_BOOT_EVENT_BOOT0_GLITCH_IGNORED:
        return "BOOT0_GLITCH_IGNORED";
    case EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED:
        return "BOOT0_STARTUP_SUPPRESSED";
    default:
        return "NONE";
    }
}

static void power_service_publish_automatic_tap(
    const ewf_power_boot_event_t *event)
{
    if (event == NULL)
    {
        return;
    }
    const ewf_tap_event_t tap = {
        .source = EWF_TAP_SOURCE_AUTOMATIC_TAP,
        .sequence = tap_input_service_next_sequence(),
        .at_ms = event->at_ms,
        .candidate_confirmed = true,
        .screen_on = true,
        .wood_fish_hit = true,
    };
    ewf_tap_decision_t decision = {0};
    const esp_err_t error = tap_input_service_submit_automatic_tap(
        &tap, pdMS_TO_TICKS(10U), &decision);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BOOT0 自动敲击未进入统一队列：原因=%s，错误=%s",
                 ewf_tap_reason_name(decision.reason),
                 esp_err_to_name(error));
    }
}

static void power_service_pwr_isr(void *context)
{
    (void)context;
    power_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_EDGE_PWR,
        .level = (uint32_t)gpio_get_level(LEGBOT_KEY_PWR_GPIO),
        .ticks = (uint32_t)xTaskGetTickCountFromISR(),
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static void power_service_boot_isr(void *context)
{
    (void)context;
    power_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_EDGE_BOOT,
        .level = (uint32_t)gpio_get_level(LEGBOT_KEY_BOOT_GPIO),
        .ticks = (uint32_t)xTaskGetTickCountFromISR(),
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}
