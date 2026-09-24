/**
 * @file     ui_service.c
 * @brief    独占 ui_task 实现：壳层投影与熄亮屏协调。
 * @details  裁决 C：所有 lv_* 仅在本任务调用。导航事实只读消费 device_nav /
 *           watch_state，不重写导航状态机。CO5300 完整整帧时序仍 hardware_pending。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_service.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "co5300_bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "tap_input_service.h"
#include "device_nav_service.h"
#include "device_settings_policy.h"
#include "sync_https_codec.h"
#include "sync_service.h"
#include "ui.h"
#include "ui_jingwen_projection.h"
#include "ui_muyu_projection.h"
#include "ui_runtime_binding.h"
#include "ui_runtime_events.h"
#include "ui_shezhi_projection.h"
#include "ui_tap_rings_policy.h"
#include "ui_tongji_projection.h"
#include "progress_service.h"

static const char *TAG = "SVC_UI";

/** prepared 状态。 */
static atomic_bool s_prepared;
/** 停止请求。 */
static atomic_bool s_stop_requested;
/** 最近一次已应用的显示开关，避免重复 set_display。 */
static bool s_display_on;
/** LVGL 对象树是否已初始化。 */
static bool s_ui_inited;
/** 已观测的 PVDF/触摸 accepted 计数（三环 origin 用增量，避免 last_source 被 automatic 冲掉）。 */
static uint32_t s_prev_pvdf_accepted;
static uint32_t s_prev_touch_accepted;
static bool s_tap_accept_primed;

static void apply_display_locked(bool want_on);
static void project_once(void);
static ewf_ui_tap_origin_t flash_origin_from_tap_snapshot(const ewf_tap_input_snapshot_t *tap_snap);
static void on_woodfish_clicked(void);
static void on_shezhi_intent(ewf_ui_shezhi_event_t code, int32_t value);
static void on_muyu_done_intent(ewf_ui_muyu_done_event_t code);
static void fill_action_id(char *buf, size_t cap);

static ewf_ui_tap_origin_t flash_origin_from_tap_snapshot(const ewf_tap_input_snapshot_t *tap_snap)
{
    if (tap_snap == NULL) {
        return EWF_UI_TAP_ORIGIN_NONE;
    }
    const uint32_t pvdf =
        tap_snap->by_source[EWF_TAP_SOURCE_PHYSICAL_PVDF].accepted_count;
    const uint32_t touch =
        tap_snap->by_source[EWF_TAP_SOURCE_DEVICE_TOUCH].accepted_count;
    if (!s_tap_accept_primed) {
        s_prev_pvdf_accepted = pvdf;
        s_prev_touch_accepted = touch;
        s_tap_accept_primed = true;
        return EWF_UI_TAP_ORIGIN_NONE;
    }
    ewf_ui_tap_origin_t origin = EWF_UI_TAP_ORIGIN_NONE;
    /* 同窗多源时优先 device_touch（与木鱼热区故事口径一致），否则 PVDF。 */
    if (touch > s_prev_touch_accepted) {
        origin = EWF_UI_TAP_ORIGIN_DEVICE_TOUCH;
    } else if (pvdf > s_prev_pvdf_accepted) {
        origin = EWF_UI_TAP_ORIGIN_PHYSICAL_PVDF;
    }
    s_prev_pvdf_accepted = pvdf;
    s_prev_touch_accepted = touch;
    return origin;
}

static void on_woodfish_clicked(void)
{
    watch_state_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    if (watch_state_snapshot(&snapshot, 0) != ESP_OK) {
        ESP_LOGW(TAG, "木鱼点击：读取快照失败，不提交");
        return;
    }
    const ewf_ui_tap_rings_gate_t gate = {
        .pending_completion = snapshot.tap_completed ||
                              snapshot.tap_pending_completion ||
                              (snapshot.tap_round_state == 1U),
        .fault_locked = snapshot.tap_fault_locked,
        .screen_on = (snapshot.screen_state == WATCH_SCREEN_STATE_ON),
        .active_page_muyu = (snapshot.nav.active_page == WATCH_NAV_PAGE_MUYU),
    };
    if (!ewf_ui_muyu_touch_may_submit(&gate)) {
        ESP_LOGW(TAG, "木鱼点击：闸门拒绝（页/熄屏/完成/故障）");
        return;
    }

    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000LL);
    const ewf_tap_event_t event = {
        .source = EWF_TAP_SOURCE_DEVICE_TOUCH,
        .sequence = tap_input_service_next_sequence(),
        .at_ms = now_ms,
        .candidate_confirmed = true,
        .screen_on = true,
        .wood_fish_hit = true,
        .wake_only = false,
        .control_event = false,
    };
    ewf_tap_decision_t decision = {0};
    const esp_err_t err =
        tap_input_service_submit_device_touch(&event, 0, &decision);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "木鱼点击提交失败：%s", esp_err_to_name(err));
        return;
    }
    if (!decision.accepted) {
        ESP_LOGW(TAG, "木鱼点击被队列闸门拒绝：原因码=%d", (int)decision.reason);
    }
}

static void on_shezhi_intent(ewf_ui_shezhi_event_t code, int32_t value)
{
    /* 裁决 B/E/F：经 device_nav 统一 API；禁止 NVS；禁止 UI 伪造成功。 */
    switch (code) {
    case EWF_UI_SHEZHI_EVT_BRIGHTNESS_LOW:
    case EWF_UI_SHEZHI_EVT_BRIGHTNESS_MID:
    case EWF_UI_SHEZHI_EVT_BRIGHTNESS_HIGH: {
        ewf_brightness_t brightness = EWF_BRIGHTNESS_MID;
        if (code == EWF_UI_SHEZHI_EVT_BRIGHTNESS_LOW) {
            brightness = EWF_BRIGHTNESS_LOW;
        } else if (code == EWF_UI_SHEZHI_EVT_BRIGHTNESS_HIGH) {
            brightness = EWF_BRIGHTNESS_HIGH;
        }
        const esp_err_t err =
            device_nav_service_set_brightness(brightness, pdMS_TO_TICKS(200));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "亮度写入失败：%s", esp_err_to_name(err));
        }
        break;
    }
    case EWF_UI_SHEZHI_EVT_TIMEOUT_5:
    case EWF_UI_SHEZHI_EVT_TIMEOUT_15:
    case EWF_UI_SHEZHI_EVT_TIMEOUT_30: {
        uint32_t timeout_s = 15U;
        if (code == EWF_UI_SHEZHI_EVT_TIMEOUT_5) {
            timeout_s = 5U;
        } else if (code == EWF_UI_SHEZHI_EVT_TIMEOUT_30) {
            timeout_s = 30U;
        }
        if (!ewf_device_settings_timeout_valid(timeout_s)) {
            ESP_LOGW(TAG, "熄屏秒数非法已拒绝：%lu", (unsigned long)timeout_s);
            return;
        }
        const esp_err_t err =
            device_nav_service_set_timeout_s(timeout_s, pdMS_TO_TICKS(200));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "熄屏写入失败：%s", esp_err_to_name(err));
        }
        break;
    }
    case EWF_UI_SHEZHI_EVT_VOLUME_CHANGED: {
        if (value < 0 || value > 100) {
            ESP_LOGW(TAG, "音量越界已拒绝：%ld", (long)value);
            return;
        }
        const esp_err_t err =
            device_nav_service_set_volume((uint8_t)value, pdMS_TO_TICKS(200));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "音量写入失败：%s", esp_err_to_name(err));
        }
        break;
    }
    case EWF_UI_SHEZHI_EVT_SYNC_CLICK: {
        /* AC2：飞行中（PENDING/BUSY）不可再点成新请求，避免打回 PENDING。 */
        const ewf_sync_status_t sync_status = device_nav_service_sync_status();
        if (sync_status == EWF_SYNC_STATUS_PENDING ||
            sync_status == EWF_SYNC_STATUS_BUSY) {
            ESP_LOGI(TAG, "立即同步忽略：飞行中 status=%d", (int)sync_status);
            break;
        }
        const esp_err_t err =
            device_nav_service_request_sync(pdMS_TO_TICKS(200));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "立即同步请求失败：%s", esp_err_to_name(err));
        }
        break;
    }
    default:
        break;
    }
}

static void fill_action_id(char *buf, size_t cap)
{
    /* 裁决 E：每次新点击新 id；混入单调时钟，避免重启后 s_seq 归零与 NVS 旧键碰撞。 */
    static uint32_t s_seq;
    if (buf == NULL || cap < 24U) {
        return;
    }
    ++s_seq;
    const int64_t now_us = esp_timer_get_time();
    (void)snprintf(buf,
                   cap,
                   "ui-r-%lld-%lu",
                   (long long)now_us,
                   (unsigned long)s_seq);
}

static void on_muyu_done_intent(ewf_ui_muyu_done_event_t code)
{
    /* 裁决 C：UI 只转发；owner=progress_service；禁止自写 NVS。 */
    ewf_progress_round_action_t action = EWF_PROGRESS_ROUND_ACTION_RESTART;
    if (code == EWF_UI_MUYU_DONE_EVT_EXIT) {
        action = EWF_PROGRESS_ROUND_ACTION_EXIT;
    } else if (code != EWF_UI_MUYU_DONE_EVT_RESTART) {
        return;
    }

    char action_id[EWF_PROGRESS_ACTION_ID_CAPACITY];
    fill_action_id(action_id, sizeof(action_id));
    const esp_err_t err = progress_service_request_round_action(
        action, action_id, pdMS_TO_TICKS(200));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "跨轮动作失败：code=%d，错误=%s", (int)code, esp_err_to_name(err));
    }
}

esp_err_t ui_service_init_contracts(void)
{
    atomic_store(&s_prepared, false);
    atomic_store(&s_stop_requested, false);
    s_display_on = true;
    s_ui_inited = false;
    return ESP_OK;
}

esp_err_t ui_service_prepare_run(void)
{
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_prepared, true);
    return ESP_OK;
}

void ui_service_cancel_prepared_run(void)
{
    atomic_store(&s_prepared, false);
}

esp_err_t ui_service_deinit_contracts(void)
{
    atomic_store(&s_prepared, false);
    atomic_store(&s_stop_requested, false);
    return ESP_OK;
}

bool ui_service_should_display_on(const watch_state_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return true;
    }
    return snapshot->screen_state == WATCH_SCREEN_STATE_ON;
}

static void apply_display_locked(bool want_on)
{
    if (want_on == s_display_on) {
        return;
    }
    /*
     * hardware_pending：完整「先提交帧再 DISPON」时序未闭合；
     * 本 Story 仅接到 co5300_bsp_set_display，与 2.4 deferred 同窗。
     */
    esp_err_t err = co5300_bsp_set_display(want_on);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "显示开关失败：on=%d，错误=%s", (int)want_on, esp_err_to_name(err));
        return;
    }
    s_display_on = want_on;
}

static void project_once(void)
{
    watch_state_snapshot_t snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    esp_err_t err = watch_state_snapshot(&snapshot, pdMS_TO_TICKS(20));
    if (err != ESP_OK) {
        return;
    }

    ewf_ui_shell_model_t model;
    ui_runtime_binding_build_model(&snapshot, &model);
    (void)ui_runtime_binding_apply(&model);

    ewf_ui_tap_origin_t origin = EWF_UI_TAP_ORIGIN_NONE;
    ewf_tap_input_snapshot_t tap_snap;
    if (tap_input_service_snapshot(&tap_snap) == ESP_OK) {
        origin = flash_origin_from_tap_snapshot(&tap_snap);
    }
    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000LL);
    ui_muyu_projection_apply(&snapshot, origin, now_ms);
    ui_jingwen_projection_apply(&snapshot);
    ui_tongji_projection_apply(&snapshot);

    if (snapshot.nav.settings_open) {
        char device_id[EWF_SYNC_DEVICE_ID_CAPACITY];
        char firmware_version[EWF_SYNC_FIRMWARE_VERSION_CAPACITY];
        bool identity_configured = false;
        device_id[0] = '\0';
        firmware_version[0] = '\0';
        if (sync_service_copy_identity(device_id, sizeof(device_id),
                                       firmware_version, sizeof(firmware_version),
                                       &identity_configured) != ESP_OK) {
            identity_configured = false;
        }
        ui_shezhi_projection_apply(&snapshot, firmware_version, device_id,
                                   identity_configured);
    }

    apply_display_locked(ui_service_should_display_on(&snapshot));
}

esp_err_t ui_service_run(void)
{
    if (!atomic_load(&s_prepared)) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!s_ui_inited) {
        ui_init();
        (void)ui_runtime_binding_init();
        (void)ui_muyu_projection_init(on_woodfish_clicked);
        (void)ui_jingwen_projection_init();
        (void)ui_tongji_projection_init();
        (void)ui_shezhi_projection_init();
        ui_runtime_events_set_shezhi_handler(on_shezhi_intent);
        ui_runtime_events_set_muyu_done_handler(on_muyu_done_intent);
        s_ui_inited = true;
        ESP_LOGI(TAG, "ui_task 已初始化 DEVICE-01 壳层、木鱼、经文、统计、设置与完成遮罩投影");
    }

    while (!atomic_load(&s_stop_requested)) {
        project_once();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(EWF_UI_POLL_PERIOD_MS));
    }

    if (s_ui_inited) {
        ui_destroy();
        s_ui_inited = false;
    }
    atomic_store(&s_prepared, false);
    return ESP_OK;
}

esp_err_t ui_service_request_stop(TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    atomic_store(&s_stop_requested, true);
    return ESP_OK;
}
