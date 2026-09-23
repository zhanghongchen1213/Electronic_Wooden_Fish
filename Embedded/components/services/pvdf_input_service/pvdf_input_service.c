/**
 * @file     pvdf_input_service.c
 * @brief    PVDF 候选有效输入边界服务实现
 * @details  ISR 只取单调时间戳并投递 ISR-safe 消息；ADC 有界采样、typed 二次确认判定与候选
 *           事件发布全部在 pvdf_task 上下文完成。本服务只产出候选事件，不建有效敲击队列、
 *           不写任何正式累计、不保存或上报原始波形，也不读取 QMI8658A 或充电状态。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "pvdf_input_service.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "pvdf_bsp.h"
#include "tap_input_service.h"

static const char *TAG = "SVC_PVDF";

/** pvdf_task 私有 typed 消息类型：比较器唤醒事件。 */
#define EWF_PVDF_INPUT_SERVICE_MESSAGE_WAKE 1U

/** 候选事件发布到事件总线的有界等待。 */
#define EWF_PVDF_INPUT_SERVICE_PUBLISH_TIMEOUT_MS 10U

/** pvdf_task 私有 ISR-safe 消息载荷。 */
typedef struct
{
    uint32_t type;  /**< 消息类型。 */
    uint32_t ticks; /**< 唤醒边沿时刻的 ISR tick。 */
} pvdf_input_service_message_t;

/** 服务状态机。 */
typedef enum
{
    EWF_PVDF_SERVICE_IDLE = 0, /**< 尚未创建队列。 */
    EWF_PVDF_SERVICE_PREPARED, /**< 队列就绪，等待框架创建任务。 */
    EWF_PVDF_SERVICE_RUNNING,  /**< pvdf_task 正在运行。 */
} pvdf_input_service_phase_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_snapshot_mutex;
static pvdf_input_service_phase_t s_phase;
static bool s_isr_registered;

static ewf_pvdf_confirm_policy_t s_policy;
static pvdf_input_service_snapshot_t s_snapshot;
static uint32_t s_candidate_sequence;

static void pvdf_input_service_wake_isr(void *context);
static void publish_snapshot(const pvdf_input_service_snapshot_t *snapshot);
static void publish_terminal(const ewf_pvdf_event_t *event, uint32_t now_ms);
static bool handle_wake_message(const pvdf_input_service_message_t *message);
static bool drain_merged_wake_messages(void);
static uint32_t current_ms(void);

esp_err_t pvdf_input_service_init_contracts(void)
{
    if (s_queue != NULL && s_snapshot_mutex != NULL)
    {
        return ESP_OK;
    }
    if (s_queue == NULL)
    {
        s_queue = xQueueCreate(EWF_PVDF_INPUT_SERVICE_QUEUE_DEPTH,
                               sizeof(pvdf_input_service_message_t));
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

esp_err_t pvdf_input_service_prepare_run(void)
{
    if (s_queue == NULL || s_snapshot_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_phase == EWF_PVDF_SERVICE_RUNNING)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_phase = EWF_PVDF_SERVICE_PREPARED;
    return ESP_OK;
}

void pvdf_input_service_cancel_prepared_run(void)
{
    if (s_phase == EWF_PVDF_SERVICE_PREPARED)
    {
        s_phase = EWF_PVDF_SERVICE_IDLE;
    }
}

esp_err_t pvdf_input_service_deinit_contracts(void)
{
    if (s_phase == EWF_PVDF_SERVICE_RUNNING)
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
    s_phase = EWF_PVDF_SERVICE_IDLE;
    return ESP_OK;
}

QueueHandle_t pvdf_input_service_queue(void)
{
    return s_queue;
}

esp_err_t pvdf_input_service_run(void)
{
    if (s_queue == NULL || s_phase != EWF_PVDF_SERVICE_PREPARED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_candidate_sequence = 0U;
    s_snapshot.last_kind = EWF_PVDF_EVENT_NONE;
    ewf_pvdf_confirm_policy_reset(&s_policy);

    pvdf_bsp_selfcheck_t bsp = {0};
    esp_err_t err = pvdf_bsp_selfcheck(&bsp);
    if (err != ESP_OK || !bsp.initialized)
    {
        ESP_LOGE(TAG,
                 "PVDF 链路不可用，pvdf_task 不进入采样循环，稳定错误码=%s",
                 pvdf_bsp_error_code());
        s_phase = EWF_PVDF_SERVICE_IDLE;
        return err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
    }

    /* 逐脚唤醒由 BSP 在板级引脚阶段已武装，此处只确保运行态上升沿中断就绪。 */
    err = pvdf_bsp_wake_enable();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF 比较器唤醒武装失败，错误=%s", esp_err_to_name(err));
        s_phase = EWF_PVDF_SERVICE_IDLE;
        return err;
    }
    err = pvdf_bsp_isr_register(pvdf_input_service_wake_isr, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF 比较器中断登记失败，错误=%s", esp_err_to_name(err));
        (void)pvdf_bsp_wake_disable();
        s_phase = EWF_PVDF_SERVICE_IDLE;
        return err;
    }
    s_isr_registered = true;
    s_phase = EWF_PVDF_SERVICE_RUNNING;

    /* 初始化完成即开启上电盲窗：屏蔽阈值未建立与 TLV7042 上电高阻造成的伪高电平。 */
    const uint32_t started_ms = current_ms();
    ewf_pvdf_confirm_policy_arm_runtime(&s_policy, started_ms);

    pvdf_input_service_snapshot_t initial = {0};
    initial.last_kind = EWF_PVDF_EVENT_NONE;
    initial.isr_registered = s_isr_registered;
    initial.calibration_available = bsp.calibration_available;
    initial.degraded = bsp.degraded;
    initial.blind_window_open = true;
    publish_snapshot(&initial);
    ESP_LOGI(TAG,
             "PVDF 候选输入边界已就绪：ADC=IO%d，比较器唤醒=IO%d 高电平有效，盲窗=%u ms，"
             "校准=%s；本服务只产出候选事件，不推进正式累计",
             (int)EWF_PVDF_ADC_GPIO,
             (int)EWF_PVDF_CMP_WAKE_GPIO,
             (unsigned)EWF_PVDF_SETTLING_BLIND_MS,
             bsp.calibration_available ? "可用" : "不可用（降级）");

    bool stop_seen = false;
    while (!stop_seen)
    {
        pvdf_input_service_message_t message = {0};
        if (xQueueReceive(s_queue, &message, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        if (message.type == LEGBOT_SERVICE_STOP_MESSAGE)
        {
            break;
        }
        stop_seen = handle_wake_message(&message);
    }

    (void)pvdf_bsp_isr_unregister();
    s_isr_registered = false;
    s_phase = EWF_PVDF_SERVICE_IDLE;
    ESP_LOGW(TAG, "pvdf_task 已收到 typed STOP 并退出，比较器中断与唤醒已关闭");
    return ESP_OK;
}

esp_err_t pvdf_input_service_request_stop(TickType_t timeout_ticks)
{
    /* 允许在任务尚未进入 run 循环前投递停止命令，避免启动竞态。 */
    if (s_queue == NULL ||
        (s_phase != EWF_PVDF_SERVICE_RUNNING &&
         s_phase != EWF_PVDF_SERVICE_PREPARED))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const pvdf_input_service_message_t message = {
        .type = LEGBOT_SERVICE_STOP_MESSAGE,
        .ticks = 0U,
    };
    return xQueueSend(s_queue, &message, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t pvdf_input_service_snapshot(pvdf_input_service_snapshot_t *snapshot)
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

esp_err_t pvdf_input_service_rearm_isr(void)
{
    return pvdf_bsp_wake_enable();
}

esp_err_t pvdf_input_service_selfcheck(pvdf_selfcheck_result_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));
    ewf_pvdf_confirm_policy_t scratch;
    ewf_pvdf_selfcheck_replay(&scratch, &out->counts);
    out->all_expectations_met = ewf_pvdf_selfcheck_expectations_met(&out->counts);
    return ESP_OK;
}

const char *pvdf_input_service_event_name(ewf_pvdf_event_kind_t kind)
{
    return ewf_pvdf_event_name(kind);
}

static void pvdf_input_service_wake_isr(void *context)
{
    (void)context;
    /* ISR 只取单调时间戳并投递 ISR-safe 消息：不读 ADC、不打印、不阻塞、不做产品决策。 */
    pvdf_input_service_message_t message = {
        .type = EWF_PVDF_INPUT_SERVICE_MESSAGE_WAKE,
        .ticks = (uint32_t)xTaskGetTickCountFromISR(),
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static bool handle_wake_message(const pvdf_input_service_message_t *message)
{
    const uint32_t wake_ms = (uint32_t)pdTICKS_TO_MS(message->ticks);
    ewf_pvdf_event_t event = {0};
    bool stop_seen = false;

    if (ewf_pvdf_confirm_policy_on_wake_event(&s_policy, wake_ms, &event))
    {
        publish_terminal(&event, wake_ms);
    }
    else if (s_policy.confirm_in_flight)
    {
        for (uint32_t sample = 0U; sample < EWF_PVDF_CONFIRM_MAX_SAMPLES; ++sample)
        {
            int32_t millivolt = 0;
            ewf_pvdf_sample_status_t status = EWF_PVDF_SAMPLE_FAILED;
            const esp_err_t err = pvdf_bsp_read_mv(&millivolt);
            if (err == ESP_OK)
            {
                status = EWF_PVDF_SAMPLE_VALID;
            }
            else if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_INVALID_RESPONSE)
            {
                /* 读数超时与触边界都判为越界，不重试、不放大、不硬扛。 */
                status = EWF_PVDF_SAMPLE_OUT_OF_RANGE;
            }
            else
            {
                status = EWF_PVDF_SAMPLE_FAILED;
            }

            const uint32_t at_ms = current_ms();
            if (ewf_pvdf_confirm_policy_on_sample(&s_policy,
                                                  millivolt,
                                                  status,
                                                  at_ms,
                                                  &event))
            {
                publish_terminal(&event, at_ms);
                break;
            }
            if ((sample + 1U) < EWF_PVDF_CONFIRM_MAX_SAMPLES)
            {
                vTaskDelay(pdMS_TO_TICKS(EWF_PVDF_CONFIRM_SAMPLE_INTERVAL_MS));
            }
        }

        /* 采样循环未自然结清时按窗口截止有界推进，保证确认窗口恒有终态。 */
        if (s_policy.confirm_in_flight)
        {
            const uint32_t at_ms = current_ms();
            if (ewf_pvdf_confirm_policy_on_deadline(&s_policy, at_ms, &event))
            {
                publish_terminal(&event, at_ms);
            }
        }
        stop_seen = drain_merged_wake_messages();
    }

    /* ISR 在边沿处已先停中断，本任务采样完成后必须重武装，否则该脚中断永久关闭。 */
    (void)pvdf_input_service_rearm_isr();
    return stop_seen;
}

static bool drain_merged_wake_messages(void)
{
    bool stop_seen = false;
    pvdf_input_service_message_t message = {0};
    while (xQueueReceive(s_queue, &message, 0) == pdTRUE)
    {
        if (message.type == LEGBOT_SERVICE_STOP_MESSAGE)
        {
            stop_seen = true;
        }
        /* 其余为确认窗口期间到达的可合并唤醒事件，按 AD-7 丢最新。 */
    }
    return stop_seen;
}

static void publish_terminal(const ewf_pvdf_event_t *event, uint32_t now_ms)
{
    pvdf_input_service_snapshot_t snapshot = s_snapshot;
    snapshot.wake_event_count = s_policy.wake_event_count;
    snapshot.candidate_count = s_policy.candidate_count;
    memcpy(snapshot.rejected_counts,
           s_policy.rejected_counts,
           sizeof(snapshot.rejected_counts));
    snapshot.last_kind = event->kind;
    snapshot.last_margin_millivolt = s_policy.last_margin_millivolt;
    snapshot.isr_registered = s_isr_registered;
    snapshot.blind_window_open =
        ewf_pvdf_confirm_policy_blind_window_open(&s_policy, now_ms);
    publish_snapshot(&snapshot);

    if (event->kind == EWF_PVDF_EVENT_CANDIDATE_TAP)
    {
        const ewf_tap_event_t tap = {
            .source = EWF_TAP_SOURCE_PHYSICAL_PVDF,
            .sequence = ++s_candidate_sequence,
            .at_ms = event->at_ms,
            .candidate_confirmed = true,
            .screen_on = true,
        };
        ewf_tap_decision_t decision = {0};
        const esp_err_t err = tap_input_service_submit_physical_pvdf(
            &tap,
            pdMS_TO_TICKS(EWF_PVDF_INPUT_SERVICE_PUBLISH_TIMEOUT_MS),
            &decision);
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "PVDF 候选未进入统一敲击队列：原因=%s，错误=%s",
                     ewf_tap_reason_name(decision.reason),
                     esp_err_to_name(err));
        }
        ESP_LOGI(TAG,
                 "PVDF 结论=%s，确认裕量=%ld mV，峰值=%ld mV，候选累计=%lu，来源=%s",
                 ewf_pvdf_event_name(event->kind),
                 (long)event->margin_millivolt,
                 (long)event->peak_millivolt,
                 (unsigned long)s_policy.candidate_count,
                 EWF_PVDF_EVENT_SOURCE_NAME);
        return;
    }

    /* 候选失败也交给统一 gate 留下 typed 拒绝证据，绝不进入正式有效事件。 */
    const ewf_tap_event_t rejected = {
        .source = EWF_TAP_SOURCE_PHYSICAL_PVDF,
        .sequence = ++s_candidate_sequence,
        .at_ms = event->at_ms,
        .candidate_confirmed = false,
        .screen_on = true,
    };
    ewf_tap_decision_t rejected_decision = {0};
    (void)tap_input_service_submit_physical_pvdf(
        &rejected,
        pdMS_TO_TICKS(EWF_PVDF_INPUT_SERVICE_PUBLISH_TIMEOUT_MS),
        &rejected_decision);

    ESP_LOGI(TAG,
             "PVDF 结论=%s，峰值=%ld mV，采样数=%lu，拒绝累计=%lu",
             ewf_pvdf_event_name(event->kind),
             (long)event->peak_millivolt,
             (unsigned long)event->sample_count,
             (unsigned long)s_policy.rejected_counts[event->kind]);
}

static void publish_snapshot(const pvdf_input_service_snapshot_t *snapshot)
{
    if (s_snapshot_mutex == NULL ||
        xSemaphoreTake(s_snapshot_mutex, 0) != pdTRUE)
    {
        return;
    }
    s_snapshot = *snapshot;
    xSemaphoreGive(s_snapshot_mutex);
}

static uint32_t current_ms(void)
{
    return (uint32_t)pdTICKS_TO_MS(xTaskGetTickCount());
}
