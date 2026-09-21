/**
 * @file     ui_runtime_events.c
 * @brief    SquareLine 空回调到 typed intent 的纯转换层
 * @details  实现导出头文件声明的 64 个回调；不读取业务状态、不直接跳页，也不调用服务 owner。
 * @author   ZHC
 * @date     2026-07-22
 */

#include "ui_runtime_binding.h"

#include "ui.h"

static void emit(ui_runtime_intent_type_t type);
static void emit_value(ui_runtime_intent_type_t type, uint32_t value);

void ui_evt_main_shell_gesture(lv_event_t *e)
{
    /*
     * 横向分页和顶部下拉均由 binding 基于完整触摸序列处理；
     * 生成回调保留为空壳，避免 LVGL 瞬时 gesture 再提交第二套路由。
     */
    (void)e;
}

void ui_evt_home_normal_nav_home_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_HOME);
}

void ui_evt_home_normal_nav_control_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_CONTROL);
}

void ui_evt_home_normal_nav_alerts_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_ALERTS);
}

void ui_evt_home_normal_nav_mode_power_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_MODE_POWER);
}

void ui_evt_home_unbound_btn_choose_device(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_OPEN_BLE_CANDIDATES);
}

void ui_evt_home_disconnected_btn_reconnect_now(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_RECONNECT_BLE);
}

void ui_evt_control_connected_btn_gear_minus(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_GEAR_DECREMENT);
}

void ui_evt_control_connected_btn_gear_plus(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_GEAR_INCREMENT);
}

void ui_evt_control_connected_toggle_assist(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_TOGGLE_ASSIST);
}

void ui_evt_control_connected_nav_home_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_HOME);
}

void ui_evt_control_connected_nav_control_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_CONTROL);
}

void ui_evt_control_connected_nav_alerts_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_ALERTS);
}

void ui_evt_control_connected_nav_mode_power_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_MODE_POWER);
}

void ui_evt_mode_default_btn_mode_standard(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SET_MODE_STANDARD);
}

void ui_evt_mode_default_btn_mode_extreme(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SET_MODE_EXTREME);
}

void ui_evt_mode_default_btn_mode_sport(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SET_MODE_SPORT);
}

void ui_evt_mode_default_btn_mode_eco(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SET_MODE_ECO);
}

void ui_evt_mode_default_btn_exo_poweroff(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_EXO_POWEROFF);
}

void ui_evt_mode_default_nav_home_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_HOME);
}

void ui_evt_mode_default_nav_control_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_CONTROL);
}

void ui_evt_mode_default_nav_mode_power_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_MODE_POWER);
}

void ui_evt_mode_default_nav_alerts_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_ALERTS);
}

void ui_evt_alerts_empty_nav_home_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_HOME);
}

void ui_evt_alerts_empty_nav_control_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_CONTROL);
}

void ui_evt_alerts_empty_nav_alerts_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_ALERTS);
}

void ui_evt_alerts_empty_nav_mode_power_button(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SHELL_MODE_POWER);
}

void ui_evt_payment_required_btn_payment_verify(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_VERIFY_PAYMENT);
}

void ui_evt_payment_verify_failed_btn_payment_retry(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_VERIFY_PAYMENT);
}

void ui_evt_settings_gesture(lv_event_t *e)
{
    /*
     * 设置纵向滚动和底部返回由 binding 的释放事件统一仲裁；
     * 不再依赖识别瞬间坐标或 gesture velocity。
     */
    (void)e;
}

void ui_evt_cmp_setting_toggle_clicked(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == ui_ui_settings_row_haptics_instance)
    {
        emit(UI_INTENT_TOGGLE_HAPTICS);
    }
    else if (target == ui_ui_settings_row_click_audio_instance)
    {
        emit(UI_INTENT_TOGGLE_CLICK_AUDIO);
    }
    else if (target == ui_ui_settings_row_raise_wake_instance)
    {
        emit(UI_INTENT_TOGGLE_RAISE_WAKE);
    }
}

void ui_evt_settings_btn_brightness_low(lv_event_t *e)
{
    (void)e;
    emit_value(UI_INTENT_SET_BRIGHTNESS, UI_RUNTIME_BRIGHTNESS_LOW);
}

void ui_evt_settings_btn_brightness_medium(lv_event_t *e)
{
    (void)e;
    emit_value(UI_INTENT_SET_BRIGHTNESS, UI_RUNTIME_BRIGHTNESS_MEDIUM);
}

void ui_evt_settings_btn_brightness_high(lv_event_t *e)
{
    (void)e;
    emit_value(UI_INTENT_SET_BRIGHTNESS, UI_RUNTIME_BRIGHTNESS_HIGH);
}

void ui_evt_settings_btn_screen_timeout_5s(lv_event_t *e)
{
    (void)e;
    emit_value(UI_INTENT_SET_SCREEN_TIMEOUT,
               UI_RUNTIME_SCREEN_TIMEOUT_5S);
}

void ui_evt_settings_btn_screen_timeout_15s(lv_event_t *e)
{
    (void)e;
    emit_value(UI_INTENT_SET_SCREEN_TIMEOUT,
               UI_RUNTIME_SCREEN_TIMEOUT_15S);
}

void ui_evt_settings_btn_screen_timeout_30s(lv_event_t *e)
{
    (void)e;
    emit_value(UI_INTENT_SET_SCREEN_TIMEOUT,
               UI_RUNTIME_SCREEN_TIMEOUT_30S);
}

void ui_evt_settings_row_device_info(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_OPEN_DEVICE_INFO);
}

void ui_evt_settings_row_maintenance(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_OPEN_MAINTENANCE);
}

void ui_evt_device_info_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_maintenance_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_maintenance_row_cellular(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_CONNECT_CELLULAR);
}

void ui_evt_maintenance_row_selftest(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_OPEN_SELFTEST);
}

void ui_evt_maintenance_row_clear_binding(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_OPEN_CLEAR_BINDING);
}

void ui_evt_clear_binding_confirm_btn_confirm_clear_binding(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_CONFIRM_CLEAR_BINDING);
}

void ui_evt_clear_binding_confirm_btn_cancel(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_CANCEL_CLEAR_BINDING);
}

void ui_evt_ble_empty_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_ble_empty_btn_rescan(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_RESCAN_BLE);
}

void ui_evt_cmp_ble_candidate_row_clicked(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == ui_ui_ble_candidates_device_candidate_1)
    {
        emit_value(UI_INTENT_SELECT_BLE_CANDIDATE, 0U);
    }
    else if (target == ui_ui_ble_candidates_device_candidate_2)
    {
        emit_value(UI_INTENT_SELECT_BLE_CANDIDATE, 1U);
    }
    else if (target == ui_ui_ble_candidates_device_candidate_3)
    {
        emit_value(UI_INTENT_SELECT_BLE_CANDIDATE, 2U);
    }
    else if (target == ui_ui_ble_candidates_device_candidate_4)
    {
        emit_value(UI_INTENT_SELECT_BLE_CANDIDATE, 3U);
    }
}

void ui_evt_ble_candidates_btn_rescan(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_RESCAN_BLE);
}

void ui_evt_selftest_idle_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_selftest_idle_btn_start_selftest(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_START_SELFTEST);
}

void ui_evt_selftest_progress_btn_leave_selftest(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_LEAVE_SELFTEST);
}

void ui_evt_selftest_manual_btn_manual_fail(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SELFTEST_MANUAL_FAIL);
}

void ui_evt_selftest_manual_btn_manual_pass(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_SELFTEST_MANUAL_PASS);
}

void ui_evt_selftest_result_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_selftest_result_btn_retry_failed(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_OPEN_RETRY_FAILED);
}

void ui_evt_selftest_result_btn_rerun_all(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_RERUN_SELFTEST);
}

void ui_evt_selftest_gps_context_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_selftest_gps_context_btn_gps_outdoor(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_GPS_OUTDOOR);
}

void ui_evt_selftest_gps_context_btn_gps_indoor(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_GPS_INDOOR);
}

void ui_evt_selftest_retry_detail_btn_back(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_BACK);
}

void ui_evt_selftest_retry_detail_btn_failed_prev(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_FAILED_PREVIOUS);
}

void ui_evt_selftest_retry_detail_btn_failed_next(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_FAILED_NEXT);
}

void ui_evt_selftest_retry_detail_btn_retry_item(lv_event_t *e)
{
    (void)e;
    emit(UI_INTENT_RETRY_FAILED_ITEM);
}

static void emit(ui_runtime_intent_type_t type)
{
    emit_value(type, 0U);
}

static void emit_value(ui_runtime_intent_type_t type, uint32_t value)
{
    ui_runtime_binding_emit_intent(type,
                                   value,
                                   UI_RUNTIME_INTENT_SOURCE_BUTTON);
}
