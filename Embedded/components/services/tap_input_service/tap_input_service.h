/**
 * @file     tap_input_service.h
 * @brief    统一有效敲击输入服务接口。
 * @details  三类来源共用一个有界 typed queue；本服务只发布 valid_tap，不拥有累计或游标。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_TAP_INPUT_SERVICE_H
#define EWF_TAP_INPUT_SERVICE_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "legbot_services.h"
#include "tap_input_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEGBOT_TAP_INPUT_SERVICE_ID LEGBOT_SERVICE_TAP_INPUT
#define EWF_TAP_EVENT_VALID 2U

typedef struct {
    uint32_t accepted_count;
    uint32_t rejected_count;
    uint32_t queue_full_count;
    ewf_tap_reason_t last_reason;
} ewf_tap_source_snapshot_t;

typedef struct {
    uint32_t accepted_count;
    uint32_t rejected_count;
    uint32_t queue_full_count;
    size_t queue_depth;
    ewf_tap_reason_t last_reason;
    ewf_tap_source_t last_source;
    ewf_tap_source_snapshot_t by_source[EWF_TAP_SOURCE_COUNT];
} ewf_tap_input_snapshot_t;

esp_err_t tap_input_service_init_contracts(void);
esp_err_t tap_input_service_prepare_run(void);
void tap_input_service_cancel_prepared_run(void);
esp_err_t tap_input_service_deinit_contracts(void);
esp_err_t tap_input_service_run(void);
esp_err_t tap_input_service_request_stop(TickType_t timeout_ticks);
esp_err_t tap_input_service_submit(const ewf_tap_event_t *event,
                                   TickType_t timeout_ticks,
                                   ewf_tap_decision_t *decision);
esp_err_t tap_input_service_submit_physical_pvdf(const ewf_tap_event_t *event,
                                                 TickType_t timeout_ticks,
                                                 ewf_tap_decision_t *decision);
esp_err_t tap_input_service_submit_device_touch(const ewf_tap_event_t *event,
                                                TickType_t timeout_ticks,
                                                ewf_tap_decision_t *decision);
esp_err_t tap_input_service_submit_automatic_tap(const ewf_tap_event_t *event,
                                                 TickType_t timeout_ticks,
                                                 ewf_tap_decision_t *decision);
esp_err_t tap_input_service_snapshot(ewf_tap_input_snapshot_t *snapshot);
QueueHandle_t tap_input_service_queue(void);

#ifdef __cplusplus
}
#endif

#endif
