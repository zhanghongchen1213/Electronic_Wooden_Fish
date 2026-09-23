/**
 * @file     tap_input_service.c
 * @brief    统一有效敲击 typed queue 服务实现。
 * @details  投递时执行统一 gate，任务逐个消费并发布 valid_tap 事件；不修改正式累计。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "tap_input_service.h"

#include <string.h>

#include "esp_log.h"
#include "event_bus.h"
#include "legbot_services.h"
#include "freertos/semphr.h"
#include "state_service.h"

static const char *TAG = "SVC_TAP";

#define EWF_TAP_INPUT_QUEUE_DEPTH EWF_TAP_INPUT_QUEUE_CAPACITY
#define EWF_TAP_VALID_EVENT_PUBLISH_TIMEOUT_MS 10U

typedef struct {
    ewf_tap_event_t event;
} tap_input_message_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_mutex;
static ewf_tap_input_snapshot_t s_snapshot;
static uint32_t s_last_sequence;
static bool s_has_sequence;
static bool s_prepared;
static bool s_running;

static esp_err_t submit_from_source(ewf_tap_source_t source,
                                    const ewf_tap_event_t *event,
                                    TickType_t timeout_ticks,
                                    ewf_tap_decision_t *decision);
static void record_rejection(const ewf_tap_event_t *event,
                             ewf_tap_reason_t reason,
                             size_t queue_depth);
static void record_acceptance(ewf_tap_source_t source);
static bool load_gate_state(ewf_tap_gate_state_t *state);

esp_err_t tap_input_service_init_contracts(void)
{
    if (s_queue != NULL && s_mutex != NULL) {
        return ESP_OK;
    }
    s_queue = xQueueCreate(EWF_TAP_INPUT_QUEUE_DEPTH, sizeof(tap_input_message_t));
    s_mutex = xSemaphoreCreateMutex();
    if (s_queue == NULL || s_mutex == NULL) {
        if (s_queue != NULL) { vQueueDelete(s_queue); s_queue = NULL; }
        if (s_mutex != NULL) { vSemaphoreDelete(s_mutex); s_mutex = NULL; }
        return ESP_ERR_NO_MEM;
    }
    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.last_reason = EWF_TAP_REASON_NONE;
    s_last_sequence = 0U;
    s_has_sequence = false;
    return ESP_OK;
}

esp_err_t tap_input_service_prepare_run(void)
{
    if (s_queue == NULL || s_mutex == NULL || s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    s_prepared = true;
    return ESP_OK;
}

void tap_input_service_cancel_prepared_run(void)
{
    if (!s_running) { s_prepared = false; }
}

esp_err_t tap_input_service_deinit_contracts(void)
{
    if (s_running) { return ESP_ERR_INVALID_STATE; }
    if (s_queue != NULL) { vQueueDelete(s_queue); s_queue = NULL; }
    if (s_mutex != NULL) { vSemaphoreDelete(s_mutex); s_mutex = NULL; }
    s_last_sequence = 0U;
    s_has_sequence = false;
    s_prepared = false;
    return ESP_OK;
}

QueueHandle_t tap_input_service_queue(void) { return s_queue; }

esp_err_t tap_input_service_submit(const ewf_tap_event_t *event,
                                   TickType_t timeout_ticks,
                                   ewf_tap_decision_t *out_decision)
{
    if (event == NULL || s_queue == NULL || s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    ewf_tap_gate_state_t state = {0};
    const bool state_available = load_gate_state(&state);
    if (!state_available) {
        state.service_ready = false;
    }
    state.queue_full = uxQueueMessagesWaiting(s_queue) >= EWF_TAP_INPUT_QUEUE_DEPTH;
    ewf_tap_decision_t decision = ewf_tap_gate(event, &state);
    if (!s_prepared) {
        decision.accepted = false;
        decision.valid_tap = false;
        decision.reason = EWF_TAP_REASON_SERVICE_NOT_READY;
    }
    if (s_has_sequence && event->sequence <= s_last_sequence) {
        decision.accepted = false;
        decision.valid_tap = false;
        decision.reason = EWF_TAP_REASON_DUPLICATE_SEQUENCE;
    }
    if (!decision.accepted) {
        const size_t queue_depth = uxQueueMessagesWaiting(s_queue);
        record_rejection(event, decision.reason, queue_depth);
        xSemaphoreGive(s_mutex);
        if (out_decision != NULL) { *out_decision = decision; }
        ESP_LOGW(TAG, "敲击输入拒绝：来源=%s，原因=%s，队列深度=%u",
                 ewf_tap_source_name(event->source), ewf_tap_reason_name(decision.reason),
                 (unsigned)queue_depth);
        return decision.reason == EWF_TAP_REASON_QUEUE_FULL ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_ARG;
    }
    const tap_input_message_t message = {.event = *event};
    if (xQueueSend(s_queue, &message, timeout_ticks) != pdTRUE) {
        record_rejection(event, EWF_TAP_REASON_QUEUE_FULL,
                         uxQueueMessagesWaiting(s_queue));
        if (out_decision != NULL) { out_decision->accepted = false; out_decision->valid_tap = false; out_decision->reason = EWF_TAP_REASON_QUEUE_FULL; }
        xSemaphoreGive(s_mutex);
        return ESP_ERR_TIMEOUT;
    }
    s_last_sequence = event->sequence;
    s_has_sequence = true;
    if (out_decision != NULL) { *out_decision = decision; }
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static esp_err_t submit_from_source(ewf_tap_source_t source,
                                    const ewf_tap_event_t *event,
                                    TickType_t timeout_ticks,
                                    ewf_tap_decision_t *decision)
{
    if (event == NULL || event->source != source) {
        if (decision != NULL) {
            decision->accepted = false;
            decision->valid_tap = false;
            decision->reason = EWF_TAP_REASON_INVALID_SOURCE;
        }
        return ESP_ERR_INVALID_ARG;
    }
    return tap_input_service_submit(event, timeout_ticks, decision);
}

esp_err_t tap_input_service_submit_physical_pvdf(const ewf_tap_event_t *event,
                                                 TickType_t timeout_ticks,
                                                 ewf_tap_decision_t *decision)
{
    return submit_from_source(EWF_TAP_SOURCE_PHYSICAL_PVDF, event,
                              timeout_ticks, decision);
}

esp_err_t tap_input_service_submit_device_touch(const ewf_tap_event_t *event,
                                                TickType_t timeout_ticks,
                                                ewf_tap_decision_t *decision)
{
    return submit_from_source(EWF_TAP_SOURCE_DEVICE_TOUCH, event,
                              timeout_ticks, decision);
}

esp_err_t tap_input_service_submit_automatic_tap(const ewf_tap_event_t *event,
                                                 TickType_t timeout_ticks,
                                                 ewf_tap_decision_t *decision)
{
    return submit_from_source(EWF_TAP_SOURCE_AUTOMATIC_TAP, event,
                              timeout_ticks, decision);
}

esp_err_t tap_input_service_run(void)
{
    if (s_queue == NULL || !s_prepared) { return ESP_ERR_INVALID_STATE; }
    s_running = true;
    tap_input_message_t message = {0};
    while (xQueueReceive(s_queue, &message, portMAX_DELAY) == pdTRUE) {
        if (message.event.control_event && message.event.sequence == UINT32_MAX) { break; }
        ewf_tap_gate_state_t state = {0};
        const bool state_available = load_gate_state(&state);
        if (!state_available) {
            state.service_ready = false;
        }
        const ewf_tap_decision_t gate = ewf_tap_gate(&message.event, &state);
        if (!gate.accepted) {
            if (s_mutex != NULL && xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                record_rejection(&message.event, gate.reason,
                                 uxQueueMessagesWaiting(s_queue));
                xSemaphoreGive(s_mutex);
            }
            ESP_LOGW(TAG, "消费前 gate 拒绝：来源=%s，原因=%s",
                     ewf_tap_source_name(message.event.source),
                     ewf_tap_reason_name(gate.reason));
            continue;
        }
        const legbot_event_t valid = {
            .source = LEGBOT_EVENT_SOURCE_STATE,
            .type = LEGBOT_EVENT_TYPE_SIGNAL,
            .code = EWF_TAP_EVENT_VALID,
            .value = message.event.sequence,
        };
        const esp_err_t error = event_bus_publish(
            &valid, pdMS_TO_TICKS(EWF_TAP_VALID_EVENT_PUBLISH_TIMEOUT_MS));
        if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
            if (error == ESP_OK) {
                ++s_snapshot.accepted_count;
                s_snapshot.last_reason = EWF_TAP_REASON_ACCEPTED;
                record_acceptance(message.event.source);
            } else {
                const ewf_tap_reason_t reason =
                    error == ESP_ERR_TIMEOUT ? EWF_TAP_REASON_QUEUE_FULL
                                             : EWF_TAP_REASON_SERVICE_NOT_READY;
                record_rejection(&message.event,
                                 reason,
                                 uxQueueMessagesWaiting(s_queue));
                ESP_LOGW(TAG, "有效敲击事件发布失败：序号=%lu，错误=%s",
                         (unsigned long)message.event.sequence,
                         esp_err_to_name(error));
            }
            s_snapshot.last_source = message.event.source;
            s_snapshot.queue_depth = uxQueueMessagesWaiting(s_queue);
            xSemaphoreGive(s_mutex);
        }
    }
    s_running = false;
    s_prepared = false;
    return ESP_OK;
}

esp_err_t tap_input_service_request_stop(TickType_t timeout_ticks)
{
    if (s_queue == NULL || !s_prepared) { return ESP_ERR_INVALID_STATE; }
    const tap_input_message_t stop = {.event = {.sequence = UINT32_MAX, .control_event = true}};
    return xQueueSend(s_queue, &stop, timeout_ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t tap_input_service_snapshot(ewf_tap_input_snapshot_t *snapshot)
{
    if (snapshot == NULL || s_mutex == NULL) { return ESP_ERR_INVALID_ARG; }
    if (xSemaphoreTake(s_mutex, 0) != pdTRUE) { return ESP_ERR_TIMEOUT; }
    *snapshot = s_snapshot;
    snapshot->queue_depth = uxQueueMessagesWaiting(s_queue);
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static bool load_gate_state(ewf_tap_gate_state_t *state)
{
    if (state == NULL) {
        return false;
    }
    bool service_ready = false;
    bool completed = false;
    bool fault_locked = false;
    const esp_err_t error = state_service_read_tap_gate(&service_ready,
                                                        &completed,
                                                        &fault_locked);
    state->service_ready = error == ESP_OK && service_ready;
    state->completed = completed;
    state->fault_locked = fault_locked;
    state->queue_full = false;
    return error == ESP_OK;
}

static void record_rejection(const ewf_tap_event_t *event,
                             ewf_tap_reason_t reason,
                             size_t queue_depth)
{
    ++s_snapshot.rejected_count;
    if (reason == EWF_TAP_REASON_QUEUE_FULL) {
        ++s_snapshot.queue_full_count;
    }
    s_snapshot.last_reason = reason;
    s_snapshot.last_source = event != NULL ? event->source : EWF_TAP_SOURCE_COUNT;
    s_snapshot.queue_depth = queue_depth;
    if (event != NULL && (unsigned)event->source < (unsigned)EWF_TAP_SOURCE_COUNT) {
        ewf_tap_source_snapshot_t *source = &s_snapshot.by_source[event->source];
        ++source->rejected_count;
        if (reason == EWF_TAP_REASON_QUEUE_FULL) {
            ++source->queue_full_count;
        }
        source->last_reason = reason;
    }
}

static void record_acceptance(ewf_tap_source_t source)
{
    if ((unsigned)source < (unsigned)EWF_TAP_SOURCE_COUNT) {
        ++s_snapshot.by_source[source].accepted_count;
        s_snapshot.by_source[source].last_reason = EWF_TAP_REASON_ACCEPTED;
    }
}
