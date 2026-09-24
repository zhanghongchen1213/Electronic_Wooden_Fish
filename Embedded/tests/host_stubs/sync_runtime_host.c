/**
 * @file     sync_runtime_host.c
 * @brief    sync_service 主机接线替身：progress 确认与反馈音量。
 * @author   ZHC
 * @date     2026-09-24
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_state.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "progress_service.h"

static uint32_t s_acked_total;
static uint32_t s_advance_calls;
static uint8_t s_last_volume = 50U;
static bool s_host_persist_pending;
static bool s_host_persist_inflight;
static bool s_host_persist_status_fail;

void host_sync_runtime_reset(void)
{
    s_acked_total = 0U;
    s_advance_calls = 0U;
    s_last_volume = 50U;
    s_host_persist_pending = false;
    s_host_persist_inflight = false;
    s_host_persist_status_fail = false;
}

void host_sync_set_persist_status(bool pending, bool inflight)
{
    s_host_persist_pending = pending;
    s_host_persist_inflight = inflight;
    s_host_persist_status_fail = false;
}

uint32_t host_sync_runtime_acked_total(void)
{
    return s_acked_total;
}

uint32_t host_sync_runtime_advance_calls(void)
{
    return s_advance_calls;
}

uint8_t host_sync_runtime_last_volume(void)
{
    return s_last_volume;
}

esp_err_t progress_service_advance_acked_total(uint32_t new_acked_total,
                                               TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (new_acked_total < s_acked_total) {
        return ESP_ERR_INVALID_ARG;
    }
    s_acked_total = new_acked_total;
    ++s_advance_calls;

    watch_tap_progress_update_t update = {0};
    watch_state_snapshot_t snap = {0};
    if (watch_state_snapshot(&snap, 0) == ESP_OK) {
        update.update_sequence = snap.tap_progress_update_sequence + 1U;
        if (update.update_sequence == 0U) {
            update.update_sequence = 1U;
        }
        update.local_total = snap.tap_local_total;
        update.acked_total = new_acked_total;
        update.round_id = snap.tap_round_id == 0U ? 1U : snap.tap_round_id;
        update.round_cursor = snap.tap_round_cursor;
        update.round_state = snap.tap_round_state;
        update.pending_completion = snap.tap_pending_completion;
        update.backlog_count = snap.tap_local_total > new_acked_total
                                   ? snap.tap_local_total - new_acked_total
                                   : 0U;
        (void)watch_state_apply_tap_progress_update(&update, 0);
    }
    return ESP_OK;
}

esp_err_t progress_service_get_persist_status(bool *persist_pending,
                                              bool *persist_inflight)
{
    if (persist_pending == NULL || persist_inflight == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_host_persist_status_fail) {
        return ESP_ERR_INVALID_STATE;
    }
    *persist_pending = s_host_persist_pending;
    *persist_inflight = s_host_persist_inflight;
    return ESP_OK;
}

esp_err_t feedback_service_set_volume(uint8_t volume, TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (volume > 100U) {
        return ESP_ERR_INVALID_ARG;
    }
    s_last_volume = volume;
    return ESP_OK;
}
