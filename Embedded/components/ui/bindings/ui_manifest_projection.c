/**
 * @file     ui_manifest_projection.c
 * @brief    SquareLine manifest 静态状态投影实现
 * @details  由工具机械生成，严格执行对象基线与 canonical/variant 的完整覆盖操作。
 * @author   ZHC
 * @date     2026-07-22
 */

#include "ui_manifest_projection.h"

#include "ui.h"

bool ui_manifest_projection_apply(ui_manifest_state_t state)
{
    switch (state)
    {
        case UI_MANIFEST_STATE_HOME_NORMAL:
            if (ui_ui_home_disconnected_btn_reconnect_now == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_icon_disconnected == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_lbl_last_snapshot == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_reconnect_action_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_control_paused == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_disconnected == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_last_snapshot == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_bottom_facts_baseline == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_bottom_facts_divider == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_gear_motion_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_assist_state == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_current_gear == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_gear_unit == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_txt_assist_state == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_txt_time == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_gear == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_bind_action_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_btn_choose_device == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_icon_unbound == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_state_rule == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_txt_unbound == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_home_disconnected_btn_reconnect_now, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_icon_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_lbl_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_reconnect_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_control_paused, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_bottom_facts_baseline, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_bottom_facts_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_gear_motion_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_current_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_gear_unit, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_txt_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_txt_time, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_bind_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_btn_choose_device, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_icon_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_state_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_txt_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_home_normal_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_home_normal_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_home_normal_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_home_normal_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_txt_time, 44, 51);
            lv_obj_set_size(ui_ui_home_normal_txt_time, 300, 66);
            lv_obj_clear_flag(ui_ui_home_normal_txt_time, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_txt_time, "--:--");
            lv_obj_set_style_text_color(ui_ui_home_normal_txt_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_txt_time, &ui_font_mo700_62, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_txt_time, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_icon_assist_state, 16, 134);
            lv_obj_set_size(ui_ui_home_normal_icon_assist_state, 42, 42);
            lv_obj_clear_flag(ui_ui_home_normal_icon_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_icon_assist_state, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_icon_assist_state, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_icon_assist_state, &ui_font_lucide_42, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_txt_assist_state, 68, 136);
            lv_obj_set_size(ui_ui_home_normal_txt_assist_state, 260, 38);
            lv_obj_clear_flag(ui_ui_home_normal_txt_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_txt_assist_state, "助力开启");
            lv_obj_set_style_text_color(ui_ui_home_normal_txt_assist_state, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_txt_assist_state, &ui_font_ns700_31, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_txt_assist_state, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_gear_motion_band, 16, 220);
            lv_obj_set_size(ui_ui_home_normal_gear_motion_band, 378, 110);
            lv_obj_clear_flag(ui_ui_home_normal_gear_motion_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_gear_motion_band, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_gear_motion_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_gear_motion_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_bottom_facts_baseline, 16, 350);
            lv_obj_set_size(ui_ui_home_normal_bottom_facts_baseline, 378, 2);
            lv_obj_clear_flag(ui_ui_home_normal_bottom_facts_baseline, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_bottom_facts_baseline, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_bottom_facts_baseline, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_bottom_facts_baseline, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_val_gear, 278, 230);
            lv_obj_set_size(ui_ui_home_normal_val_gear, 76, 71);
            lv_obj_clear_flag(ui_ui_home_normal_val_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_val_gear, "8");
            lv_obj_set_style_text_color(ui_ui_home_normal_val_gear, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_val_gear, &ui_font_mo700_67, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_val_gear, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_val_steps, 246, 404);
            lv_obj_set_size(ui_ui_home_normal_val_steps, 136, 42);
            lv_obj_clear_flag(ui_ui_home_normal_val_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_val_steps, "3,842");
            lv_obj_set_style_text_color(ui_ui_home_normal_val_steps, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_val_steps, &ui_font_mo700_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_val_steps, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_val_exo_battery, 52, 404);
            lv_obj_set_size(ui_ui_home_normal_val_exo_battery, 130, 42);
            lv_obj_clear_flag(ui_ui_home_normal_val_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_val_exo_battery, "68%");
            lv_obj_set_style_text_color(ui_ui_home_normal_val_exo_battery, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_val_exo_battery, &ui_font_mo700_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_val_exo_battery, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot, 17, 18);
            lv_obj_set_size(ui_ui_home_normal_nav_dot, 10, 10);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_3, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_3, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_3, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_3, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_lbl_current_gear, 42, 260);
            lv_obj_set_size(ui_ui_home_normal_lbl_current_gear, 150, 32);
            lv_obj_clear_flag(ui_ui_home_normal_lbl_current_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_lbl_current_gear, "当前档位");
            lv_obj_set_style_text_color(ui_ui_home_normal_lbl_current_gear, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_lbl_current_gear, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_lbl_current_gear, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_lbl_gear_unit, 350, 265);
            lv_obj_set_size(ui_ui_home_normal_lbl_gear_unit, 36, 32);
            lv_obj_clear_flag(ui_ui_home_normal_lbl_gear_unit, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_lbl_gear_unit, "档");
            lv_obj_set_style_text_color(ui_ui_home_normal_lbl_gear_unit, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_lbl_gear_unit, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_lbl_gear_unit, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_icon_exo_battery, 16, 367);
            lv_obj_set_size(ui_ui_home_normal_icon_exo_battery, 28, 26);
            lv_obj_clear_flag(ui_ui_home_normal_icon_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_icon_exo_battery, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_icon_exo_battery, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_icon_exo_battery, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_lbl_exo_battery, 52, 365);
            lv_obj_set_size(ui_ui_home_normal_lbl_exo_battery, 140, 31);
            lv_obj_clear_flag(ui_ui_home_normal_lbl_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_lbl_exo_battery, "外骨骼");
            lv_obj_set_style_text_color(ui_ui_home_normal_lbl_exo_battery, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_lbl_exo_battery, &ui_font_ns600_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_lbl_exo_battery, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_bottom_facts_divider, 204, 362);
            lv_obj_set_size(ui_ui_home_normal_bottom_facts_divider, 2, 82);
            lv_obj_clear_flag(ui_ui_home_normal_bottom_facts_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_bottom_facts_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_bottom_facts_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_bottom_facts_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_icon_steps, 220, 367);
            lv_obj_set_size(ui_ui_home_normal_icon_steps, 28, 26);
            lv_obj_clear_flag(ui_ui_home_normal_icon_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_icon_steps, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_icon_steps, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_icon_steps, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_lbl_steps, 258, 365);
            lv_obj_set_size(ui_ui_home_normal_lbl_steps, 136, 31);
            lv_obj_clear_flag(ui_ui_home_normal_lbl_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_lbl_steps, "步数");
            lv_obj_set_style_text_color(ui_ui_home_normal_lbl_steps, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_lbl_steps, &ui_font_ns600_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_lbl_steps, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_HOME_UNBOUND:
            if (ui_ui_home_disconnected_btn_reconnect_now == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_icon_disconnected == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_lbl_last_snapshot == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_reconnect_action_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_control_paused == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_disconnected == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_last_snapshot == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_bottom_facts_baseline == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_bottom_facts_divider == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_gear_motion_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_assist_state == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_current_gear == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_gear_unit == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_txt_assist_state == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_txt_time == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_gear == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_bind_action_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_btn_choose_device == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_icon_unbound == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_state_rule == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_txt_unbound == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_btn_choose_device_bluetooth_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_btn_choose_device_label == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_btn_choose_device_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_home_disconnected_btn_reconnect_now, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_icon_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_lbl_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_reconnect_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_control_paused, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_bottom_facts_baseline, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_bottom_facts_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_gear_motion_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_current_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_gear_unit, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_txt_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_txt_time, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_bind_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_btn_choose_device, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_icon_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_state_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_txt_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_home_normal_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_ble_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_icon_unbound, 16, 128);
            lv_obj_set_size(ui_ui_home_unbound_icon_unbound, 56, 52);
            lv_obj_clear_flag(ui_ui_home_unbound_icon_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_unbound_icon_unbound, "");
            lv_obj_set_style_text_color(ui_ui_home_unbound_icon_unbound, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_unbound_icon_unbound, &ui_font_lucide_56, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_txt_unbound, 80, 123);
            lv_obj_set_size(ui_ui_home_unbound_txt_unbound, 314, 53);
            lv_obj_clear_flag(ui_ui_home_unbound_txt_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_unbound_txt_unbound, "未连接外骨骼");
            lv_obj_set_style_text_color(ui_ui_home_unbound_txt_unbound, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_unbound_txt_unbound, &ui_font_ns700_44, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_unbound_txt_unbound, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_state_rule, 16, 210);
            lv_obj_set_size(ui_ui_home_unbound_state_rule, 378, 8);
            lv_obj_clear_flag(ui_ui_home_unbound_state_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_unbound_state_rule, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_unbound_state_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_unbound_state_rule, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_bind_action_band, 16, 260);
            lv_obj_set_size(ui_ui_home_unbound_bind_action_band, 378, 100);
            lv_obj_clear_flag(ui_ui_home_unbound_bind_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_unbound_bind_action_band, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_unbound_bind_action_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_unbound_bind_action_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_btn_choose_device, 16, 260);
            lv_obj_set_size(ui_ui_home_unbound_btn_choose_device, 378, 100);
            lv_obj_clear_flag(ui_ui_home_unbound_btn_choose_device, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_unbound_btn_choose_device, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_unbound_btn_choose_device, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_unbound_btn_choose_device, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, 22, 31);
            lv_obj_set_size(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, 40, 38);
            lv_obj_clear_flag(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_unbound_btn_choose_device_bluetooth_icon, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_btn_choose_device_label, 76, 32);
            lv_obj_set_size(ui_ui_home_unbound_btn_choose_device_label, 232, 40);
            lv_obj_clear_flag(ui_ui_home_unbound_btn_choose_device_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_unbound_btn_choose_device_label, "连接外骨骼");
            lv_obj_set_style_text_color(ui_ui_home_unbound_btn_choose_device_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_unbound_btn_choose_device_label, &ui_font_ns700_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_unbound_btn_choose_device_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_btn_choose_device_arrow, 328, 35);
            lv_obj_set_size(ui_ui_home_unbound_btn_choose_device_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_home_unbound_btn_choose_device_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_unbound_btn_choose_device_arrow, "");
            lv_obj_set_style_text_color(ui_ui_home_unbound_btn_choose_device_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_unbound_btn_choose_device_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot, 17, 18);
            lv_obj_set_size(ui_ui_home_normal_nav_dot, 10, 10);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_3, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_3, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_3, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_3, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_home_normal_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_home_normal_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_home_normal_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_txt_time, 44, 55);
            lv_obj_set_size(ui_ui_home_normal_txt_time, 300, 66);
            lv_obj_clear_flag(ui_ui_home_normal_txt_time, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_txt_time, "10:28");
            lv_obj_set_style_text_color(ui_ui_home_normal_txt_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_txt_time, &ui_font_mo700_62, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_txt_time, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_HOME_DISCONNECTED:
            if (ui_ui_home_disconnected_btn_reconnect_now == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_icon_disconnected == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_lbl_last_snapshot == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_reconnect_action_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_control_paused == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_disconnected == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_txt_last_snapshot == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_bottom_facts_baseline == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_bottom_facts_divider == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_gear_motion_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_assist_state == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_icon_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_current_gear == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_gear_unit == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_lbl_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_txt_assist_state == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_txt_time == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_exo_battery == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_gear == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_val_steps == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_bind_action_band == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_btn_choose_device == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_icon_unbound == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_state_rule == NULL)
            {
                return false;
            }
            if (ui_ui_home_unbound_txt_unbound == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_btn_reconnect_now_icon == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_btn_reconnect_now_label == NULL)
            {
                return false;
            }
            if (ui_ui_home_disconnected_btn_reconnect_now_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_home_normal_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_home_disconnected_btn_reconnect_now, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_icon_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_lbl_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_reconnect_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_control_paused, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_disconnected_txt_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_bottom_facts_baseline, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_baseline, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_bottom_facts_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_bottom_facts_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_gear_motion_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_gear_motion_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_assist_state, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_icon_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_icon_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_current_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_current_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_gear_unit, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_gear_unit, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_lbl_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_lbl_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_txt_assist_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_txt_assist_state, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_txt_time, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_exo_battery, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_exo_battery, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_normal_val_steps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_normal_val_steps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_bind_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_bind_action_band, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_btn_choose_device, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_btn_choose_device, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_icon_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_icon_unbound, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_state_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_home_unbound_txt_unbound, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_home_unbound_txt_unbound, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_home_normal_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_home_normal_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_ble_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_home_normal_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_home_normal_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_home_normal_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_home_normal_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_normal_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_icon_disconnected, 16, 125);
            lv_obj_set_size(ui_ui_home_disconnected_icon_disconnected, 48, 46);
            lv_obj_clear_flag(ui_ui_home_disconnected_icon_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_icon_disconnected, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_icon_disconnected, "");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_icon_disconnected, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_icon_disconnected, &ui_font_lucide_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_txt_disconnected, 78, 118);
            lv_obj_set_size(ui_ui_home_disconnected_txt_disconnected, 310, 60);
            lv_obj_clear_flag(ui_ui_home_disconnected_txt_disconnected, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_disconnected, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_txt_disconnected, "连接中断");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_txt_disconnected, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_txt_disconnected, &ui_font_ns700_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_disconnected_txt_disconnected, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_txt_control_paused, 16, 196);
            lv_obj_set_size(ui_ui_home_disconnected_txt_control_paused, 378, 27);
            lv_obj_clear_flag(ui_ui_home_disconnected_txt_control_paused, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_control_paused, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_txt_control_paused, "控制已暂停");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_txt_control_paused, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_txt_control_paused, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_disconnected_txt_control_paused, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_unbound_state_rule, 16, 240);
            lv_obj_set_size(ui_ui_home_unbound_state_rule, 378, 8);
            lv_obj_clear_flag(ui_ui_home_unbound_state_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_unbound_state_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_unbound_state_rule, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_unbound_state_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_unbound_state_rule, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_lbl_last_snapshot, 16, 266);
            lv_obj_set_size(ui_ui_home_disconnected_lbl_last_snapshot, 378, 22);
            lv_obj_clear_flag(ui_ui_home_disconnected_lbl_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_lbl_last_snapshot, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_lbl_last_snapshot, "最近状态");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_lbl_last_snapshot, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_lbl_last_snapshot, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_disconnected_lbl_last_snapshot, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_txt_last_snapshot, 16, 292);
            lv_obj_set_size(ui_ui_home_disconnected_txt_last_snapshot, 378, 32);
            lv_obj_clear_flag(ui_ui_home_disconnected_txt_last_snapshot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_txt_last_snapshot, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_txt_last_snapshot, "8档 68% 3,842步");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_txt_last_snapshot, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_txt_last_snapshot, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_disconnected_txt_last_snapshot, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_reconnect_action_band, 16, 338);
            lv_obj_set_size(ui_ui_home_disconnected_reconnect_action_band, 378, 84);
            lv_obj_clear_flag(ui_ui_home_disconnected_reconnect_action_band, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_reconnect_action_band, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_disconnected_reconnect_action_band, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_disconnected_reconnect_action_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_disconnected_reconnect_action_band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_btn_reconnect_now, 16, 338);
            lv_obj_set_size(ui_ui_home_disconnected_btn_reconnect_now, 378, 84);
            lv_obj_clear_flag(ui_ui_home_disconnected_btn_reconnect_now, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_disconnected_btn_reconnect_now, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_disconnected_btn_reconnect_now, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_disconnected_btn_reconnect_now, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_btn_reconnect_now_icon, 24, 40);
            lv_obj_set_size(ui_ui_home_disconnected_btn_reconnect_now_icon, 28, 26);
            lv_obj_clear_flag(ui_ui_home_disconnected_btn_reconnect_now_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_btn_reconnect_now_icon, "");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_btn_reconnect_now_icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_btn_reconnect_now_icon, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_btn_reconnect_now_label, 64, 30);
            lv_obj_set_size(ui_ui_home_disconnected_btn_reconnect_now_label, 248, 32);
            lv_obj_clear_flag(ui_ui_home_disconnected_btn_reconnect_now_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_btn_reconnect_now_label, "立即重连");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_btn_reconnect_now_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_btn_reconnect_now_label, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_disconnected_btn_reconnect_now_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_disconnected_btn_reconnect_now_arrow, 332, 22);
            lv_obj_set_size(ui_ui_home_disconnected_btn_reconnect_now_arrow, 28, 26);
            lv_obj_clear_flag(ui_ui_home_disconnected_btn_reconnect_now_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_disconnected_btn_reconnect_now_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_disconnected_btn_reconnect_now_arrow, "");
            lv_obj_set_style_text_color(ui_ui_home_disconnected_btn_reconnect_now_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_disconnected_btn_reconnect_now_arrow, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot, 17, 18);
            lv_obj_set_size(ui_ui_home_normal_nav_dot, 10, 10);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_3, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_3, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_3, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_3, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_txt_time, 44, 55);
            lv_obj_set_size(ui_ui_home_normal_txt_time, 300, 66);
            lv_obj_clear_flag(ui_ui_home_normal_txt_time, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_txt_time, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_home_normal_txt_time, "10:28");
            lv_obj_set_style_text_color(ui_ui_home_normal_txt_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_home_normal_txt_time, &ui_font_mo700_62, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_home_normal_txt_time, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_home_normal_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_home_normal_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_home_normal_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_home_normal_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_home_normal_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_home_normal_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_home_normal_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_home_normal_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_home_normal_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_CONTROL_CONNECTED:
            if (ui_ui_control_connected_btn_gear_minus == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_btn_gear_plus == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_toggle_assist == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_val_actual_gear == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_btn_gear_minus_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_btn_gear_plus_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_toggle_assist_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_btn_gear_minus_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_btn_gear_plus_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_icon_assist == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_lbl_assist == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_switch_assist_track == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_switch_assist_thumb == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_control_connected_btn_gear_minus, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_minus, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_minus, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_btn_gear_plus, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_plus, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_plus, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_toggle_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_toggle_assist, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_toggle_assist, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_val_actual_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_val_actual_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_val_actual_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_disconnected_toggle_assist_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_toggle_assist_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_disconnected_toggle_assist_disabled, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_control_connected_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_control_connected_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_control_connected_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_control_connected_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_control_connected_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_control_connected_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_control_connected_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_control_connected_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_val_actual_gear, 105, 42);
            lv_obj_set_size(ui_ui_control_connected_val_actual_gear, 200, 167);
            lv_obj_clear_flag(ui_ui_control_connected_val_actual_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_val_actual_gear, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_val_actual_gear, "8");
            lv_obj_set_style_text_color(ui_ui_control_connected_val_actual_gear, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_val_actual_gear, &ui_font_mo700_160, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_control_connected_val_actual_gear, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_btn_gear_minus, 16, 198);
            lv_obj_set_size(ui_ui_control_connected_btn_gear_minus, 181, 116);
            lv_obj_clear_flag(ui_ui_control_connected_btn_gear_minus, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_minus, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_btn_gear_minus, lv_color_hex(0x1B1E18), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_btn_gear_minus, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_btn_gear_minus, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_control_connected_btn_gear_minus, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_control_connected_btn_gear_minus, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_btn_gear_minus_icon, 59, 36);
            lv_obj_set_size(ui_ui_control_connected_btn_gear_minus_icon, 64, 44);
            lv_obj_clear_flag(ui_ui_control_connected_btn_gear_minus_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_minus_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_btn_gear_minus_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_btn_gear_minus_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_btn_gear_minus_icon, &ui_font_lucide_64, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_btn_gear_plus, 213, 198);
            lv_obj_set_size(ui_ui_control_connected_btn_gear_plus, 181, 116);
            lv_obj_clear_flag(ui_ui_control_connected_btn_gear_plus, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_plus, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_btn_gear_plus, lv_color_hex(0x1B1E18), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_btn_gear_plus, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_btn_gear_plus, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_control_connected_btn_gear_plus, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_control_connected_btn_gear_plus, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_btn_gear_plus_icon, 59, 36);
            lv_obj_set_size(ui_ui_control_connected_btn_gear_plus_icon, 64, 44);
            lv_obj_clear_flag(ui_ui_control_connected_btn_gear_plus_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_plus_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_btn_gear_plus_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_btn_gear_plus_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_btn_gear_plus_icon, &ui_font_lucide_64, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_toggle_assist, 16, 330);
            lv_obj_set_size(ui_ui_control_connected_toggle_assist, 378, 96);
            lv_obj_clear_flag(ui_ui_control_connected_toggle_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_toggle_assist, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_toggle_assist, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_toggle_assist, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_toggle_assist, 48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_control_connected_toggle_assist, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_control_connected_toggle_assist, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_icon_assist, 24, 29);
            lv_obj_set_size(ui_ui_control_connected_icon_assist, 40, 38);
            lv_obj_clear_flag(ui_ui_control_connected_icon_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_icon_assist, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_icon_assist, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_icon_assist, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_icon_assist, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_lbl_assist, 80, 28);
            lv_obj_set_size(ui_ui_control_connected_lbl_assist, 120, 40);
            lv_obj_clear_flag(ui_ui_control_connected_lbl_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_lbl_assist, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_lbl_assist, "助力");
            lv_obj_set_style_text_color(ui_ui_control_connected_lbl_assist, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_lbl_assist, &ui_font_ns700_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_control_connected_lbl_assist, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_switch_assist_track, 230, 12);
            lv_obj_set_size(ui_ui_control_connected_switch_assist_track, 132, 72);
            lv_obj_clear_flag(ui_ui_control_connected_switch_assist_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_switch_assist_track, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_switch_assist_track, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_switch_assist_track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_switch_assist_track, 36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_switch_assist_thumb, 66, 6);
            lv_obj_set_size(ui_ui_control_connected_switch_assist_thumb, 60, 60);
            lv_obj_clear_flag(ui_ui_control_connected_switch_assist_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_switch_assist_thumb, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_switch_assist_thumb, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_switch_assist_thumb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_switch_assist_thumb, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_control_connected_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot_2, 17, 18);
            lv_obj_set_size(ui_ui_control_connected_nav_dot_2, 10, 10);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot_2, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot_2, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot_3, 19, 20);
            lv_obj_set_size(ui_ui_control_connected_nav_dot_3, 6, 6);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot_3, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot_3, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_control_connected_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_CONTROL_DISCONNECTED:
            if (ui_ui_control_connected_btn_gear_minus == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_btn_gear_plus == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_toggle_assist == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_val_actual_gear == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_btn_gear_minus_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_btn_gear_plus_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_toggle_assist_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_btn_gear_minus_icon_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_btn_gear_plus_icon_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_icon_assist == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_lbl_assist == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_switch_assist_track_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_disconnected_switch_assist_thumb_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_control_connected_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_control_connected_btn_gear_minus, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_minus, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_minus, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_btn_gear_plus, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_plus, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_btn_gear_plus, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_toggle_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_toggle_assist, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_toggle_assist, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_connected_val_actual_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_val_actual_gear, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_connected_val_actual_gear, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_control_disconnected_toggle_assist_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_toggle_assist_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_control_disconnected_toggle_assist_disabled, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_control_connected_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_control_connected_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_control_connected_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_control_connected_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_ble_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_control_connected_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_control_connected_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_control_connected_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_control_connected_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_control_connected_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_control_connected_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_val_actual_gear, 105, 42);
            lv_obj_set_size(ui_ui_control_connected_val_actual_gear, 200, 167);
            lv_obj_clear_flag(ui_ui_control_connected_val_actual_gear, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_val_actual_gear, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_connected_val_actual_gear, "8");
            lv_obj_set_style_text_color(ui_ui_control_connected_val_actual_gear, lv_color_hex(0x555C67), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_connected_val_actual_gear, &ui_font_mo700_160, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_control_connected_val_actual_gear, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_btn_gear_minus_disabled, 16, 198);
            lv_obj_set_size(ui_ui_control_disconnected_btn_gear_minus_disabled, 181, 116);
            lv_obj_clear_flag(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_btn_gear_minus_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_disconnected_btn_gear_minus_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_disconnected_btn_gear_minus_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_disconnected_btn_gear_minus_disabled, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_control_disconnected_btn_gear_minus_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_control_disconnected_btn_gear_minus_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, 59, 36);
            lv_obj_set_size(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, 64, 44);
            lv_obj_clear_flag(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, "");
            lv_obj_set_style_text_color(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_disconnected_btn_gear_minus_icon_disabled, &ui_font_lucide_64, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_btn_gear_plus_disabled, 213, 198);
            lv_obj_set_size(ui_ui_control_disconnected_btn_gear_plus_disabled, 181, 116);
            lv_obj_clear_flag(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_btn_gear_plus_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_disconnected_btn_gear_plus_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_disconnected_btn_gear_plus_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_disconnected_btn_gear_plus_disabled, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_control_disconnected_btn_gear_plus_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_control_disconnected_btn_gear_plus_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, 59, 36);
            lv_obj_set_size(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, 64, 44);
            lv_obj_clear_flag(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, "");
            lv_obj_set_style_text_color(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_disconnected_btn_gear_plus_icon_disabled, &ui_font_lucide_64, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_toggle_assist_disabled, 16, 330);
            lv_obj_set_size(ui_ui_control_disconnected_toggle_assist_disabled, 378, 96);
            lv_obj_clear_flag(ui_ui_control_disconnected_toggle_assist_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_toggle_assist_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_disconnected_toggle_assist_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_disconnected_toggle_assist_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_disconnected_toggle_assist_disabled, 48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_control_disconnected_toggle_assist_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_control_disconnected_toggle_assist_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_icon_assist, 24, 29);
            lv_obj_set_size(ui_ui_control_disconnected_icon_assist, 40, 38);
            lv_obj_clear_flag(ui_ui_control_disconnected_icon_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_icon_assist, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_disconnected_icon_assist, "");
            lv_obj_set_style_text_color(ui_ui_control_disconnected_icon_assist, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_disconnected_icon_assist, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_lbl_assist, 80, 28);
            lv_obj_set_size(ui_ui_control_disconnected_lbl_assist, 120, 40);
            lv_obj_clear_flag(ui_ui_control_disconnected_lbl_assist, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_disconnected_lbl_assist, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_control_disconnected_lbl_assist, "助力");
            lv_obj_set_style_text_color(ui_ui_control_disconnected_lbl_assist, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_control_disconnected_lbl_assist, &ui_font_ns700_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_control_disconnected_lbl_assist, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_switch_assist_track_disabled, 230, 12);
            lv_obj_set_size(ui_ui_control_disconnected_switch_assist_track_disabled, 132, 72);
            lv_obj_clear_flag(ui_ui_control_disconnected_switch_assist_track_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_switch_assist_track_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_disconnected_switch_assist_track_disabled, lv_color_hex(0x343A43), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_disconnected_switch_assist_track_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_disconnected_switch_assist_track_disabled, 36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_disconnected_switch_assist_thumb_disabled, 6, 6);
            lv_obj_set_size(ui_ui_control_disconnected_switch_assist_thumb_disabled, 60, 60);
            lv_obj_clear_flag(ui_ui_control_disconnected_switch_assist_thumb_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_control_disconnected_switch_assist_thumb_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_disconnected_switch_assist_thumb_disabled, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_disconnected_switch_assist_thumb_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_disconnected_switch_assist_thumb_disabled, 30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_control_connected_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot_2, 17, 18);
            lv_obj_set_size(ui_ui_control_connected_nav_dot_2, 10, 10);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot_2, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot_2, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot_3, 19, 20);
            lv_obj_set_size(ui_ui_control_connected_nav_dot_3, 6, 6);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot_3, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot_3, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_control_connected_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_control_connected_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_control_connected_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_control_connected_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_control_connected_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_control_connected_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_control_connected_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_control_connected_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_control_connected_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_MODE_DEFAULT:
            if (ui_ui_mode_default_btn_exo_poweroff == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_eco == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_extreme == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_sport == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_standard == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_lbl_current_mode == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_exo_poweroff_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_eco_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_extreme_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_sport_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_standard_selected_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_standard_label == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_icon_mode_standard == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_extreme_label == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_icon_mode_extreme == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_sport_label == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_icon_mode_sport == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_eco_label == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_icon_mode_eco == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_icon_exo_poweroff == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_lbl_exo_poweroff == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_mode_default_btn_exo_poweroff, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_exo_poweroff, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_exo_poweroff, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_eco, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_eco, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_eco, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_extreme, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_extreme, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_extreme, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_sport, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_sport, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_sport, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_standard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_standard, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_standard, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_lbl_current_mode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_lbl_current_mode, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_lbl_current_mode, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_mode_default_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_mode_default_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_mode_default_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_mode_default_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_mode_default_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_mode_default_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_mode_default_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_standard, 16, 540);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_standard, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_standard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_standard, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_btn_mode_standard, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_btn_mode_standard, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_btn_mode_standard, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_default_btn_mode_standard, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_default_btn_mode_standard, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_standard_label, 78, 33);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_standard_label, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_standard_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_standard_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_btn_mode_standard_label, "标准");
            lv_obj_set_style_text_color(ui_ui_mode_default_btn_mode_standard_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_btn_mode_standard_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_btn_mode_standard_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_icon_mode_standard, 22, 29);
            lv_obj_set_size(ui_ui_mode_default_icon_mode_standard, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_default_icon_mode_standard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_icon_mode_standard, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_icon_mode_standard, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_icon_mode_standard, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_icon_mode_standard, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_extreme, 213, 540);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_extreme, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_extreme, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_extreme, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_btn_mode_extreme, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_btn_mode_extreme, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_btn_mode_extreme, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_default_btn_mode_extreme, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_default_btn_mode_extreme, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_extreme_label, 78, 33);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_extreme_label, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_extreme_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_extreme_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_btn_mode_extreme_label, "极限");
            lv_obj_set_style_text_color(ui_ui_mode_default_btn_mode_extreme_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_btn_mode_extreme_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_btn_mode_extreme_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_icon_mode_extreme, 22, 29);
            lv_obj_set_size(ui_ui_mode_default_icon_mode_extreme, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_default_icon_mode_extreme, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_icon_mode_extreme, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_icon_mode_extreme, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_icon_mode_extreme, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_icon_mode_extreme, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_sport, 16, 644);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_sport, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_sport, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_sport, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_btn_mode_sport, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_btn_mode_sport, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_btn_mode_sport, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_default_btn_mode_sport, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_default_btn_mode_sport, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_sport_label, 78, 33);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_sport_label, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_sport_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_sport_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_btn_mode_sport_label, "健身");
            lv_obj_set_style_text_color(ui_ui_mode_default_btn_mode_sport_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_btn_mode_sport_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_btn_mode_sport_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_icon_mode_sport, 22, 29);
            lv_obj_set_size(ui_ui_mode_default_icon_mode_sport, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_default_icon_mode_sport, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_icon_mode_sport, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_icon_mode_sport, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_icon_mode_sport, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_icon_mode_sport, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_eco, 213, 644);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_eco, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_eco, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_eco, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_btn_mode_eco, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_btn_mode_eco, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_btn_mode_eco, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_default_btn_mode_eco, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_default_btn_mode_eco, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_mode_eco_label, 78, 33);
            lv_obj_set_size(ui_ui_mode_default_btn_mode_eco_label, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_default_btn_mode_eco_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_eco_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_btn_mode_eco_label, "下山");
            lv_obj_set_style_text_color(ui_ui_mode_default_btn_mode_eco_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_btn_mode_eco_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_btn_mode_eco_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_icon_mode_eco, 22, 29);
            lv_obj_set_size(ui_ui_mode_default_icon_mode_eco, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_default_icon_mode_eco, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_icon_mode_eco, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_icon_mode_eco, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_icon_mode_eco, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_icon_mode_eco, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_btn_exo_poweroff, 89, 135);
            lv_obj_set_size(ui_ui_mode_default_btn_exo_poweroff, 232, 232);
            lv_obj_clear_flag(ui_ui_mode_default_btn_exo_poweroff, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_exo_poweroff, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_btn_exo_poweroff, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_btn_exo_poweroff, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_btn_exo_poweroff, 116, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_default_btn_exo_poweroff, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_default_btn_exo_poweroff, lv_color_hex(0x6A1512), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_icon_exo_poweroff, 88, 42);
            lv_obj_set_size(ui_ui_mode_default_icon_exo_poweroff, 56, 52);
            lv_obj_clear_flag(ui_ui_mode_default_icon_exo_poweroff, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_icon_exo_poweroff, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_icon_exo_poweroff, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_icon_exo_poweroff, lv_color_hex(0x0A0A0A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_icon_exo_poweroff, &ui_font_lucide_56, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_lbl_exo_poweroff, 16, 126);
            lv_obj_set_size(ui_ui_mode_default_lbl_exo_poweroff, 200, 36);
            lv_obj_clear_flag(ui_ui_mode_default_lbl_exo_poweroff, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_lbl_exo_poweroff, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_lbl_exo_poweroff, "外骨骼关机");
            lv_obj_set_style_text_color(ui_ui_mode_default_lbl_exo_poweroff, lv_color_hex(0x0A0A0A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_lbl_exo_poweroff, &ui_font_ns700_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_lbl_exo_poweroff, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_mode_default_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_mode_default_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot_3, 17, 18);
            lv_obj_set_size(ui_ui_mode_default_nav_dot_3, 10, 10);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot_3, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_mode_default_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_lbl_current_mode, 500, 51);
            lv_obj_set_size(ui_ui_mode_default_lbl_current_mode, 120, 30);
            lv_obj_clear_flag(ui_ui_mode_default_lbl_current_mode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_lbl_current_mode, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_lbl_current_mode, "模式");
            lv_obj_set_style_text_color(ui_ui_mode_default_lbl_current_mode, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_lbl_current_mode, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_lbl_current_mode, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_MODE_DISCONNECTED:
            if (ui_ui_mode_default_btn_exo_poweroff == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_eco == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_extreme == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_sport == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_btn_mode_standard == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_lbl_current_mode == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_exo_poweroff_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_eco_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_extreme_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_sport_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_standard_selected_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_icon_mode_standard_selected_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_extreme_label_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_icon_mode_extreme_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_sport_label_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_icon_mode_sport_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_btn_mode_eco_label_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_icon_mode_eco_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_icon_exo_poweroff_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_disconnected_lbl_exo_poweroff_disabled == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_mode_default_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_mode_default_btn_exo_poweroff, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_exo_poweroff, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_exo_poweroff, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_eco, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_eco, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_eco, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_extreme, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_extreme, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_extreme, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_sport, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_sport, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_sport, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_btn_mode_standard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_standard, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_btn_mode_standard, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_lbl_current_mode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_lbl_current_mode, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_lbl_current_mode, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_default_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_mode_default_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_mode_default_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_mode_default_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_mode_default_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_ble_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_mode_default_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_mode_default_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_mode_default_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_mode_default_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_mode_default_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, 16, 540);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_disconnected_btn_mode_standard_selected_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, 78, 33);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, "标准");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, 22, 29);
            lv_obj_set_size(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, "");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_icon_mode_standard_selected_disabled, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_extreme_disabled, 213, 540);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_extreme_disabled, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_extreme_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_disconnected_btn_mode_extreme_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_disconnected_btn_mode_extreme_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_disconnected_btn_mode_extreme_disabled, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_disconnected_btn_mode_extreme_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_disconnected_btn_mode_extreme_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, 78, 33);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, "极限");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_icon_mode_extreme_disabled, 22, 29);
            lv_obj_set_size(ui_ui_mode_disconnected_icon_mode_extreme_disabled, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_disconnected_icon_mode_extreme_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_icon_mode_extreme_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_icon_mode_extreme_disabled, "");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_icon_mode_extreme_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_icon_mode_extreme_disabled, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_sport_disabled, 16, 644);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_sport_disabled, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_sport_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_disconnected_btn_mode_sport_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_disconnected_btn_mode_sport_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_disconnected_btn_mode_sport_disabled, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_disconnected_btn_mode_sport_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_disconnected_btn_mode_sport_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, 78, 33);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, "健身");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_disconnected_btn_mode_sport_label_disabled, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_icon_mode_sport_disabled, 22, 29);
            lv_obj_set_size(ui_ui_mode_disconnected_icon_mode_sport_disabled, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_disconnected_icon_mode_sport_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_icon_mode_sport_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_icon_mode_sport_disabled, "");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_icon_mode_sport_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_icon_mode_sport_disabled, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_eco_disabled, 213, 644);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_eco_disabled, 181, 96);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_eco_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_disconnected_btn_mode_eco_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_disconnected_btn_mode_eco_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_disconnected_btn_mode_eco_disabled, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_disconnected_btn_mode_eco_disabled, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_disconnected_btn_mode_eco_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, 78, 33);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, 85, 30);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, "下山");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_disconnected_btn_mode_eco_label_disabled, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_icon_mode_eco_disabled, 22, 29);
            lv_obj_set_size(ui_ui_mode_disconnected_icon_mode_eco_disabled, 40, 38);
            lv_obj_clear_flag(ui_ui_mode_disconnected_icon_mode_eco_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_icon_mode_eco_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_icon_mode_eco_disabled, "");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_icon_mode_eco_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_icon_mode_eco_disabled, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, 89, 135);
            lv_obj_set_size(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, 232, 232);
            lv_obj_clear_flag(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, lv_color_hex(0x111318), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, 116, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_mode_disconnected_btn_exo_poweroff_disabled, lv_color_hex(0x3C434D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, 88, 42);
            lv_obj_set_size(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, 56, 52);
            lv_obj_clear_flag(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, "");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, lv_color_hex(0x626B78), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_icon_exo_poweroff_disabled, &ui_font_lucide_56, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, 16, 126);
            lv_obj_set_size(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, 200, 36);
            lv_obj_clear_flag(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, "外骨骼关机");
            lv_obj_set_style_text_color(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, &ui_font_ns700_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_disconnected_lbl_exo_poweroff_disabled, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_mode_default_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_mode_default_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot_3, 17, 18);
            lv_obj_set_size(ui_ui_mode_default_nav_dot_3, 10, 10);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot_3, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_mode_default_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_mode_default_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_mode_default_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_mode_default_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_mode_default_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_mode_default_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_mode_default_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_mode_default_lbl_current_mode, 500, 51);
            lv_obj_set_size(ui_ui_mode_default_lbl_current_mode, 120, 30);
            lv_obj_clear_flag(ui_ui_mode_default_lbl_current_mode, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_mode_default_lbl_current_mode, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_mode_default_lbl_current_mode, "模式");
            lv_obj_set_style_text_color(ui_ui_mode_default_lbl_current_mode, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_mode_default_lbl_current_mode, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_mode_default_lbl_current_mode, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_ALERTS_EMPTY:
            if (ui_ui_alerts_empty_icon_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_txt_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_exo_battery_warning_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_temperature_danger_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_watch_critical_battery_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_top_alert_rule == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_txt_alert_title == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_btn_payment_verify == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_card == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_phone == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_payment_focus_rule == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_txt_payment_required == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_btn_payment_retry == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_icon_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_failure_reason == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_verify_failed_rule == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_alerts_empty_icon_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_txt_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_top_alert_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_txt_alert_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_btn_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_card, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_phone, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_payment_focus_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_txt_payment_required, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_btn_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_verify_failed_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_alerts_empty_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_icon_alerts_empty, 137, 113);
            lv_obj_set_size(ui_ui_alerts_empty_icon_alerts_empty, 136, 126);
            lv_obj_clear_flag(ui_ui_alerts_empty_icon_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_icon_alerts_empty, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_icon_alerts_empty, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_icon_alerts_empty, &ui_font_lucide_136, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_txt_alerts_empty, 16, 295);
            lv_obj_set_size(ui_ui_alerts_empty_txt_alerts_empty, 378, 55);
            lv_obj_clear_flag(ui_ui_alerts_empty_txt_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_txt_alerts_empty, "暂无提醒");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_txt_alerts_empty, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_txt_alerts_empty, &ui_font_ns700_45, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_alerts_empty_txt_alerts_empty, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_3, 17, 18);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_3, 10, 10);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_3, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_ALERTS_READONLY:
            if (ui_ui_alerts_empty_icon_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_txt_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_exo_battery_warning_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_temperature_danger_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_watch_critical_battery_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_top_alert_rule == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_txt_alert_title == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_btn_payment_verify == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_card == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_phone == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_payment_focus_rule == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_txt_payment_required == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_btn_payment_retry == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_icon_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_failure_reason == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_verify_failed_rule == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_alerts_empty_icon_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_txt_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_top_alert_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_txt_alert_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_btn_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_card, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_phone, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_payment_focus_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_txt_payment_required, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_btn_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_verify_failed_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_4g_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_gps_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_alerts_empty_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_readonly_txt_alert_title, 40, 53);
            lv_obj_set_size(ui_ui_alerts_readonly_txt_alert_title, 250, 38);
            lv_obj_clear_flag(ui_ui_alerts_readonly_txt_alert_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_readonly_txt_alert_title, "设备提醒");
            lv_obj_set_style_text_color(ui_ui_alerts_readonly_txt_alert_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_readonly_txt_alert_title, &ui_font_ns700_31, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_alerts_readonly_txt_alert_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_readonly_top_alert_rule, 16, 108);
            lv_obj_set_size(ui_ui_alerts_readonly_top_alert_rule, 378, 6);
            lv_obj_clear_flag(ui_ui_alerts_readonly_top_alert_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_readonly_top_alert_rule, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_readonly_top_alert_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_readonly_top_alert_rule, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, 16, 216);
            lv_obj_set_size(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, 378, 72);
            lv_obj_clear_flag(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, lv_color_hex(0x0D0F11), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 0, 8);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 6, 56);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), 18, 23);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), 28, 26);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), 62, 23);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), 210, 26);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), "外骨骼低电");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), &ui_font_ns600_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), 278, 25);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), 82, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), "20%");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_readonly_alert_temperature_danger_instance, 16, 132);
            lv_obj_set_size(ui_ui_alerts_readonly_alert_temperature_danger_instance, 378, 72);
            lv_obj_clear_flag(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_readonly_alert_temperature_danger_instance, lv_color_hex(0x0D0F11), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_readonly_alert_temperature_danger_instance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_readonly_alert_temperature_danger_instance, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_alerts_readonly_alert_temperature_danger_instance, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_alerts_readonly_alert_temperature_danger_instance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 0, 8);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 6, 56);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), 18, 23);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), 28, 26);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), 62, 23);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), 210, 26);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), "温度异常");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), &ui_font_ns600_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), 278, 25);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), 82, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), "过高");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_alerts_readonly_alert_temperature_danger_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, 16, 300);
            lv_obj_set_size(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, 378, 72);
            lv_obj_clear_flag(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, lv_color_hex(0x0D0F11), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 0, 8);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 6, 56);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT), 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), 18, 23);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), 28, 26);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON), &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), 62, 23);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), 210, 26);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), "手环严重低电");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), &ui_font_ns600_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), 278, 25);
            lv_obj_set_size(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), 82, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), "15%");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_3, 17, 18);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_3, 10, 10);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_3, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_PAYMENT_REQUIRED:
            if (ui_ui_alerts_empty_icon_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_txt_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_exo_battery_warning_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_temperature_danger_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_watch_critical_battery_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_top_alert_rule == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_txt_alert_title == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_btn_payment_verify == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_card == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_phone == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_payment_focus_rule == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_txt_payment_required == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_btn_payment_retry == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_icon_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_failure_reason == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_verify_failed_rule == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_verify == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_lbl_payment_verify == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_verify_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_alerts_empty_icon_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_txt_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_top_alert_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_txt_alert_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_btn_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_card, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_phone, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_payment_focus_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_txt_payment_required, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_btn_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_verify_failed_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_alerts_empty_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_icon_payment_phone, 129, 76);
            lv_obj_set_size(ui_ui_payment_required_icon_payment_phone, 152, 141);
            lv_obj_clear_flag(ui_ui_payment_required_icon_payment_phone, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_required_icon_payment_phone, "");
            lv_obj_set_style_text_color(ui_ui_payment_required_icon_payment_phone, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_required_icon_payment_phone, &ui_font_lucide_152, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_txt_payment_required, 16, 243);
            lv_obj_set_size(ui_ui_payment_required_txt_payment_required, 378, 51);
            lv_obj_clear_flag(ui_ui_payment_required_txt_payment_required, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_required_txt_payment_required, "请完成支付");
            lv_obj_set_style_text_color(ui_ui_payment_required_txt_payment_required, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_required_txt_payment_required, &ui_font_ns700_42, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_payment_required_txt_payment_required, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_payment_focus_rule, 125, 316);
            lv_obj_set_size(ui_ui_payment_required_payment_focus_rule, 160, 6);
            lv_obj_clear_flag(ui_ui_payment_required_payment_focus_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_payment_required_payment_focus_rule, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_payment_required_payment_focus_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_payment_required_payment_focus_rule, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_btn_payment_verify, 16, 350);
            lv_obj_set_size(ui_ui_payment_required_btn_payment_verify, 378, 80);
            lv_obj_clear_flag(ui_ui_payment_required_btn_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_payment_required_btn_payment_verify, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_payment_required_btn_payment_verify, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_payment_required_btn_payment_verify, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_icon_payment_verify, 24, 26);
            lv_obj_set_size(ui_ui_payment_required_icon_payment_verify, 30, 28);
            lv_obj_clear_flag(ui_ui_payment_required_icon_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_verify, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_required_icon_payment_verify, "");
            lv_obj_set_style_text_color(ui_ui_payment_required_icon_payment_verify, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_required_icon_payment_verify, &ui_font_lucide_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_lbl_payment_verify, 68, 24);
            lv_obj_set_size(ui_ui_payment_required_lbl_payment_verify, 248, 32);
            lv_obj_clear_flag(ui_ui_payment_required_lbl_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_lbl_payment_verify, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_required_lbl_payment_verify, "支付后验证");
            lv_obj_set_style_text_color(ui_ui_payment_required_lbl_payment_verify, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_required_lbl_payment_verify, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_payment_required_lbl_payment_verify, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_icon_payment_verify_arrow, 334, 26);
            lv_obj_set_size(ui_ui_payment_required_icon_payment_verify_arrow, 30, 28);
            lv_obj_clear_flag(ui_ui_payment_required_icon_payment_verify_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_verify_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_required_icon_payment_verify_arrow, "");
            lv_obj_set_style_text_color(ui_ui_payment_required_icon_payment_verify_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_required_icon_payment_verify_arrow, &ui_font_lucide_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_3, 17, 18);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_3, 10, 10);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_3, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_required_icon_payment_card, 169, 126);
            lv_obj_set_size(ui_ui_payment_required_icon_payment_card, 96, 65);
            lv_obj_clear_flag(ui_ui_payment_required_icon_payment_card, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_required_icon_payment_card, "");
            lv_obj_set_style_text_color(ui_ui_payment_required_icon_payment_card, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_required_icon_payment_card, &ui_font_lucide_96, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_PAYMENT_VERIFY_FAILED:
            if (ui_ui_alerts_empty_icon_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_alerts_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_control_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_home_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_mode_power_button == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_4g_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_ble_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_icon == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_rail_watch_battery_value == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_txt_alerts_empty == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_exo_battery_warning_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_temperature_danger_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_alert_watch_critical_battery_instance == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_top_alert_rule == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_readonly_txt_alert_title == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_btn_payment_verify == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_card == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_icon_payment_phone == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_payment_focus_rule == NULL)
            {
                return false;
            }
            if (ui_ui_payment_required_txt_payment_required == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_btn_payment_retry == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_icon_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_failure_reason == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_txt_payment_verify_failed == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_verify_failed_rule == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_icon_payment_retry == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_lbl_payment_retry == NULL)
            {
                return false;
            }
            if (ui_ui_payment_verify_failed_icon_payment_retry_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_2 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_3 == NULL)
            {
                return false;
            }
            if (ui_ui_alerts_empty_nav_dot_4 == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_alerts_empty_icon_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_icon_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_empty_txt_alerts_empty, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_empty_txt_alerts_empty, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_exo_battery_warning_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_temperature_danger_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_alert_watch_critical_battery_instance, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_top_alert_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_top_alert_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_alerts_readonly_txt_alert_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_alerts_readonly_txt_alert_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_btn_payment_verify, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_btn_payment_verify, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_card, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_card, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_icon_payment_phone, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_icon_payment_phone, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_payment_focus_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_payment_focus_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_required_txt_payment_required, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_required_txt_payment_required, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_btn_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_payment_verify_failed_verify_failed_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_4g_icon, 82, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_4g_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_4g_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_4g_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_4g_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_4g_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_4g_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_gps_icon, 116, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_gps_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_gps_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_gps_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_ble_icon, 150, 12);
            lv_obj_set_size(ui_ui_alerts_empty_rail_ble_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_ble_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_ble_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_ble_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_ble_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_ble_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_value, 254, 19);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_value, 54, 20);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_value, "76%");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_value, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_alerts_empty_rail_watch_battery_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_rail_watch_battery_icon, 316, 16);
            lv_obj_set_size(ui_ui_alerts_empty_rail_watch_battery_icon, 22, 22);
            lv_obj_clear_flag(ui_ui_alerts_empty_rail_watch_battery_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_rail_watch_battery_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_alerts_empty_rail_watch_battery_icon, "");
            lv_obj_set_style_text_color(ui_ui_alerts_empty_rail_watch_battery_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_alerts_empty_rail_watch_battery_icon, &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_icon_payment_verify_failed, 129, 76);
            lv_obj_set_size(ui_ui_payment_verify_failed_icon_payment_verify_failed, 152, 141);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_verify_failed, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_verify_failed_icon_payment_verify_failed, "");
            lv_obj_set_style_text_color(ui_ui_payment_verify_failed_icon_payment_verify_failed, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_verify_failed_icon_payment_verify_failed, &ui_font_lucide_152, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_txt_payment_verify_failed, 16, 233);
            lv_obj_set_size(ui_ui_payment_verify_failed_txt_payment_verify_failed, 378, 49);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_verify_failed_txt_payment_verify_failed, "验证失败");
            lv_obj_set_style_text_color(ui_ui_payment_verify_failed_txt_payment_verify_failed, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_verify_failed_txt_payment_verify_failed, &ui_font_ns700_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_payment_verify_failed_txt_payment_verify_failed, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_verify_failed_rule, 125, 326);
            lv_obj_set_size(ui_ui_payment_verify_failed_verify_failed_rule, 160, 6);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_verify_failed_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_verify_failed_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_payment_verify_failed_verify_failed_rule, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_payment_verify_failed_verify_failed_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_payment_verify_failed_verify_failed_rule, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_btn_payment_retry, 16, 350);
            lv_obj_set_size(ui_ui_payment_verify_failed_btn_payment_retry, 378, 80);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_btn_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_btn_payment_retry, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_payment_verify_failed_btn_payment_retry, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_payment_verify_failed_btn_payment_retry, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_payment_verify_failed_btn_payment_retry, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_icon_payment_retry, 24, 26);
            lv_obj_set_size(ui_ui_payment_verify_failed_icon_payment_retry, 30, 28);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_icon_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_retry, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_verify_failed_icon_payment_retry, "");
            lv_obj_set_style_text_color(ui_ui_payment_verify_failed_icon_payment_retry, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_verify_failed_icon_payment_retry, &ui_font_lucide_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_lbl_payment_retry, 68, 24);
            lv_obj_set_size(ui_ui_payment_verify_failed_lbl_payment_retry, 248, 32);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_lbl_payment_retry, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_lbl_payment_retry, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_verify_failed_lbl_payment_retry, "重新验证");
            lv_obj_set_style_text_color(ui_ui_payment_verify_failed_lbl_payment_retry, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_verify_failed_lbl_payment_retry, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_payment_verify_failed_lbl_payment_retry, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_icon_payment_retry_arrow, 334, 26);
            lv_obj_set_size(ui_ui_payment_verify_failed_icon_payment_retry_arrow, 30, 28);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_icon_payment_retry_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_icon_payment_retry_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_verify_failed_icon_payment_retry_arrow, "");
            lv_obj_set_style_text_color(ui_ui_payment_verify_failed_icon_payment_retry_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_verify_failed_icon_payment_retry_arrow, &ui_font_lucide_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_home_button, 105, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_home_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_home_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_home_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_home_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_home_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_control_button, 157, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_control_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_control_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_control_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_control_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_control_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_2, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_2, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_2, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_alerts_button, 261, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_alerts_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_alerts_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_alerts_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_alerts_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_alerts_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_3, 17, 18);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_3, 10, 10);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_3, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_mode_power_button, 209, 442);
            lv_obj_set_size(ui_ui_alerts_empty_nav_mode_power_button, 44, 44);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_mode_power_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_mode_power_button, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_mode_power_button, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_mode_power_button, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_alerts_empty_nav_dot_4, 19, 20);
            lv_obj_set_size(ui_ui_alerts_empty_nav_dot_4, 6, 6);
            lv_obj_clear_flag(ui_ui_alerts_empty_nav_dot_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_alerts_empty_nav_dot_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_alerts_empty_nav_dot_4, lv_color_hex(0x737B88), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_alerts_empty_nav_dot_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_alerts_empty_nav_dot_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_payment_verify_failed_txt_payment_failure_reason, 16, 290);
            lv_obj_set_size(ui_ui_payment_verify_failed_txt_payment_failure_reason, 378, 22);
            lv_obj_clear_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_payment_verify_failed_txt_payment_failure_reason, "验证失败，请重新验证");
            lv_obj_set_style_text_color(ui_ui_payment_verify_failed_txt_payment_failure_reason, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_payment_verify_failed_txt_payment_failure_reason, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_payment_verify_failed_txt_payment_failure_reason, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SETTINGS:
            if (ui_ui_settings_gesture_zone_settings_return == NULL)
            {
                return false;
            }
            if (ui_ui_settings_hint_pull_up_handle == NULL)
            {
                return false;
            }
            if (ui_ui_settings_settings_list_viewport == NULL)
            {
                return false;
            }
            if (ui_ui_settings_settings_page_thumb == NULL)
            {
                return false;
            }
            if (ui_ui_settings_settings_page_track == NULL)
            {
                return false;
            }
            if (ui_ui_settings_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_settings_settings_list_content == NULL)
            {
                return false;
            }
            if (ui_ui_settings_settings_list_page_1 == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_haptics_instance == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW) == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_click_audio_instance == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW) == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_raise_wake_instance == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW) == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_brightness == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_brightness_icon == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_brightness_label == NULL)
            {
                return false;
            }
            if (ui_ui_settings_segment_brightness == NULL)
            {
                return false;
            }
            if (ui_ui_settings_btn_brightness_low == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_brightness_low == NULL)
            {
                return false;
            }
            if (ui_ui_settings_btn_brightness_medium == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_brightness_medium == NULL)
            {
                return false;
            }
            if (ui_ui_settings_btn_brightness_high == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_brightness_high == NULL)
            {
                return false;
            }
            if (ui_ui_settings_settings_list_page_2 == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_screen_timeout == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_screen_timeout_icon == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_screen_timeout_label == NULL)
            {
                return false;
            }
            if (ui_ui_settings_segment_screen_timeout == NULL)
            {
                return false;
            }
            if (ui_ui_settings_btn_screen_timeout_5s == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_screen_timeout_5s == NULL)
            {
                return false;
            }
            if (ui_ui_settings_btn_screen_timeout_15s == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_screen_timeout_15s == NULL)
            {
                return false;
            }
            if (ui_ui_settings_btn_screen_timeout_30s == NULL)
            {
                return false;
            }
            if (ui_ui_settings_txt_screen_timeout_30s == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_device_info == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_device_info_icon == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_device_info_label == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_device_info_value == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_device_info_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_maintenance == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_maintenance_icon == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_maintenance_label == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_maintenance_value == NULL)
            {
                return false;
            }
            if (ui_ui_settings_row_maintenance_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_settings_gesture_zone_settings_return, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_gesture_zone_settings_return, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_gesture_zone_settings_return, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_settings_hint_pull_up_handle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_hint_pull_up_handle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_hint_pull_up_handle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_settings_settings_list_viewport, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_list_viewport, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_settings_list_viewport, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_settings_settings_page_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_page_thumb, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_settings_page_thumb, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_settings_settings_page_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_page_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_settings_page_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_settings_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_settings_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_settings_txt_page_title, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_settings_txt_page_title, 92, 27);
            lv_obj_set_size(ui_ui_settings_txt_page_title, 226, 27);
            lv_obj_clear_flag(ui_ui_settings_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_page_title, "设置");
            lv_obj_set_style_text_color(ui_ui_settings_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_settings_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_settings_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_hint_pull_up_handle, 181, 479);
            lv_obj_set_size(ui_ui_settings_hint_pull_up_handle, 48, 5);
            lv_obj_clear_flag(ui_ui_settings_hint_pull_up_handle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_hint_pull_up_handle, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_hint_pull_up_handle, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_hint_pull_up_handle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_hint_pull_up_handle, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_settings_list_viewport, 16, 72);
            lv_obj_set_size(ui_ui_settings_settings_list_viewport, 378, 344);
            lv_obj_clear_flag(ui_ui_settings_settings_list_viewport, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_list_viewport, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_settings_list_viewport, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_settings_list_viewport, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_settings_list_viewport, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_settings_list_content, 0, 0);
            lv_obj_set_size(ui_ui_settings_settings_list_content, 378, 688);
            lv_obj_clear_flag(ui_ui_settings_settings_list_content, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_list_content, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_settings_list_content, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_settings_list_content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_settings_list_content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_settings_list_page_1, 0, 0);
            lv_obj_set_size(ui_ui_settings_settings_list_page_1, 378, 344);
            lv_obj_clear_flag(ui_ui_settings_settings_list_page_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_list_page_1, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_settings_list_page_1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_settings_list_page_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_settings_list_page_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_haptics_instance, 0, 0);
            lv_obj_set_size(ui_ui_settings_row_haptics_instance, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_haptics_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_haptics_instance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_haptics_instance, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_haptics_instance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_haptics_instance, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_haptics_instance, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_haptics_instance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), 18, 24);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), 66, 27);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), 170, 27);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), "触觉反馈");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), 242, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), 90, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), "开启");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), 338, 25);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_haptics_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_click_audio_instance, 0, 88);
            lv_obj_set_size(ui_ui_settings_row_click_audio_instance, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_click_audio_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_click_audio_instance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_click_audio_instance, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_click_audio_instance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_click_audio_instance, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_click_audio_instance, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_click_audio_instance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), 18, 24);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), 66, 27);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), 170, 27);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), "点击音效");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), 242, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), 90, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), "开启");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), 338, 25);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_click_audio_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_raise_wake_instance, 0, 176);
            lv_obj_set_size(ui_ui_settings_row_raise_wake_instance, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_raise_wake_instance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_raise_wake_instance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_raise_wake_instance, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_raise_wake_instance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_raise_wake_instance, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_raise_wake_instance, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_raise_wake_instance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), 18, 24);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), 66, 27);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), 170, 27);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), "抬腕亮屏");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), 242, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), 90, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), "开启");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), 338, 25);
            lv_obj_set_size(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_settings_row_raise_wake_instance, UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_brightness, 0, 264);
            lv_obj_set_size(ui_ui_settings_row_brightness, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_brightness, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_brightness, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_brightness, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_brightness, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_brightness, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_brightness, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_brightness, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_brightness_icon, 20, 26);
            lv_obj_set_size(ui_ui_settings_row_brightness_icon, 30, 28);
            lv_obj_clear_flag(ui_ui_settings_row_brightness_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_brightness_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_brightness_icon, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_brightness_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_brightness_icon, &ui_font_lucide_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_brightness_label, 66, 27);
            lv_obj_set_size(ui_ui_settings_row_brightness_label, 126, 26);
            lv_obj_clear_flag(ui_ui_settings_row_brightness_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_brightness_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_brightness_label, "屏幕亮度");
            lv_obj_set_style_text_color(ui_ui_settings_row_brightness_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_brightness_label, &ui_font_ns600_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_row_brightness_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_segment_brightness, 198, 16);
            lv_obj_set_size(ui_ui_settings_segment_brightness, 164, 48);
            lv_obj_clear_flag(ui_ui_settings_segment_brightness, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_segment_brightness, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_segment_brightness, lv_color_hex(0x262A31), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_segment_brightness, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_segment_brightness, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_segment_brightness, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_segment_brightness, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_btn_brightness_low, 2, 2);
            lv_obj_set_size(ui_ui_settings_btn_brightness_low, 52, 44);
            lv_obj_clear_flag(ui_ui_settings_btn_brightness_low, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_btn_brightness_low, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_btn_brightness_low, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_btn_brightness_low, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_btn_brightness_low, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_txt_brightness_low, 0, 13);
            lv_obj_set_size(ui_ui_settings_txt_brightness_low, 52, 19);
            lv_obj_clear_flag(ui_ui_settings_txt_brightness_low, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_brightness_low, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_brightness_low, "低");
            lv_obj_set_style_text_color(ui_ui_settings_txt_brightness_low, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_brightness_low, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_brightness_low, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_btn_brightness_medium, 56, 2);
            lv_obj_set_size(ui_ui_settings_btn_brightness_medium, 52, 44);
            lv_obj_clear_flag(ui_ui_settings_btn_brightness_medium, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_btn_brightness_medium, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_btn_brightness_medium, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_btn_brightness_medium, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_btn_brightness_medium, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_txt_brightness_medium, 0, 13);
            lv_obj_set_size(ui_ui_settings_txt_brightness_medium, 52, 19);
            lv_obj_clear_flag(ui_ui_settings_txt_brightness_medium, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_brightness_medium, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_brightness_medium, "中");
            lv_obj_set_style_text_color(ui_ui_settings_txt_brightness_medium, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_brightness_medium, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_brightness_medium, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_btn_brightness_high, 110, 2);
            lv_obj_set_size(ui_ui_settings_btn_brightness_high, 52, 44);
            lv_obj_clear_flag(ui_ui_settings_btn_brightness_high, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_btn_brightness_high, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_btn_brightness_high, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_btn_brightness_high, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_btn_brightness_high, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_txt_brightness_high, 0, 13);
            lv_obj_set_size(ui_ui_settings_txt_brightness_high, 52, 19);
            lv_obj_clear_flag(ui_ui_settings_txt_brightness_high, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_brightness_high, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_brightness_high, "高");
            lv_obj_set_style_text_color(ui_ui_settings_txt_brightness_high, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_brightness_high, &ui_font_ns700_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_brightness_high, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_settings_list_page_2, 0, 344);
            lv_obj_set_size(ui_ui_settings_settings_list_page_2, 378, 264);
            lv_obj_clear_flag(ui_ui_settings_settings_list_page_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_list_page_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_settings_list_page_2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_settings_list_page_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_settings_list_page_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_screen_timeout, 0, 0);
            lv_obj_set_size(ui_ui_settings_row_screen_timeout, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_screen_timeout, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_screen_timeout, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_screen_timeout, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_screen_timeout, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_screen_timeout, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_screen_timeout, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_screen_timeout, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_screen_timeout_icon, 20, 26);
            lv_obj_set_size(ui_ui_settings_row_screen_timeout_icon, 30, 28);
            lv_obj_clear_flag(ui_ui_settings_row_screen_timeout_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_screen_timeout_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_screen_timeout_icon, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_screen_timeout_icon, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_screen_timeout_icon, &ui_font_lucide_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_screen_timeout_label, 66, 27);
            lv_obj_set_size(ui_ui_settings_row_screen_timeout_label, 126, 26);
            lv_obj_clear_flag(ui_ui_settings_row_screen_timeout_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_screen_timeout_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_screen_timeout_label, "自动熄屏");
            lv_obj_set_style_text_color(ui_ui_settings_row_screen_timeout_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_screen_timeout_label, &ui_font_ns600_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_row_screen_timeout_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_segment_screen_timeout, 198, 16);
            lv_obj_set_size(ui_ui_settings_segment_screen_timeout, 164, 48);
            lv_obj_clear_flag(ui_ui_settings_segment_screen_timeout, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_segment_screen_timeout, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_segment_screen_timeout, lv_color_hex(0x262A31), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_segment_screen_timeout, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_segment_screen_timeout, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_segment_screen_timeout, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_segment_screen_timeout, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_btn_screen_timeout_5s, 2, 2);
            lv_obj_set_size(ui_ui_settings_btn_screen_timeout_5s, 52, 44);
            lv_obj_clear_flag(ui_ui_settings_btn_screen_timeout_5s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_btn_screen_timeout_5s, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_btn_screen_timeout_5s, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_btn_screen_timeout_5s, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_btn_screen_timeout_5s, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_txt_screen_timeout_5s, 0, 14);
            lv_obj_set_size(ui_ui_settings_txt_screen_timeout_5s, 52, 17);
            lv_obj_clear_flag(ui_ui_settings_txt_screen_timeout_5s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_screen_timeout_5s, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_screen_timeout_5s, "5秒");
            lv_obj_set_style_text_color(ui_ui_settings_txt_screen_timeout_5s, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_screen_timeout_5s, &ui_font_ns700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_screen_timeout_5s, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_btn_screen_timeout_15s, 56, 2);
            lv_obj_set_size(ui_ui_settings_btn_screen_timeout_15s, 52, 44);
            lv_obj_clear_flag(ui_ui_settings_btn_screen_timeout_15s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_btn_screen_timeout_15s, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_btn_screen_timeout_15s, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_btn_screen_timeout_15s, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_btn_screen_timeout_15s, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_txt_screen_timeout_15s, 0, 14);
            lv_obj_set_size(ui_ui_settings_txt_screen_timeout_15s, 52, 17);
            lv_obj_clear_flag(ui_ui_settings_txt_screen_timeout_15s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_screen_timeout_15s, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_screen_timeout_15s, "15秒");
            lv_obj_set_style_text_color(ui_ui_settings_txt_screen_timeout_15s, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_screen_timeout_15s, &ui_font_ns600_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_screen_timeout_15s, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_btn_screen_timeout_30s, 110, 2);
            lv_obj_set_size(ui_ui_settings_btn_screen_timeout_30s, 52, 44);
            lv_obj_clear_flag(ui_ui_settings_btn_screen_timeout_30s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_btn_screen_timeout_30s, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_btn_screen_timeout_30s, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_btn_screen_timeout_30s, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_btn_screen_timeout_30s, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_txt_screen_timeout_30s, 0, 14);
            lv_obj_set_size(ui_ui_settings_txt_screen_timeout_30s, 52, 17);
            lv_obj_clear_flag(ui_ui_settings_txt_screen_timeout_30s, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_txt_screen_timeout_30s, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_txt_screen_timeout_30s, "30秒");
            lv_obj_set_style_text_color(ui_ui_settings_txt_screen_timeout_30s, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_txt_screen_timeout_30s, &ui_font_ns600_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_txt_screen_timeout_30s, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_device_info, 0, 88);
            lv_obj_set_size(ui_ui_settings_row_device_info, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_device_info, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_device_info, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_device_info, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_device_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_device_info, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_device_info, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_device_info, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_device_info_icon, 18, 24);
            lv_obj_set_size(ui_ui_settings_row_device_info_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_settings_row_device_info_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_device_info_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_device_info_icon, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_device_info_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_device_info_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_device_info_label, 66, 27);
            lv_obj_set_size(ui_ui_settings_row_device_info_label, 170, 27);
            lv_obj_clear_flag(ui_ui_settings_row_device_info_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_device_info_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_device_info_label, "设备信息");
            lv_obj_set_style_text_color(ui_ui_settings_row_device_info_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_device_info_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_row_device_info_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_device_info_value, 242, 29);
            lv_obj_set_size(ui_ui_settings_row_device_info_value, 90, 23);
            lv_obj_clear_flag(ui_ui_settings_row_device_info_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_device_info_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_device_info_value, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_device_info_value, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_device_info_value, &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_row_device_info_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_device_info_arrow, 338, 25);
            lv_obj_set_size(ui_ui_settings_row_device_info_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_settings_row_device_info_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_device_info_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_device_info_arrow, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_device_info_arrow, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_device_info_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_maintenance, 0, 176);
            lv_obj_set_size(ui_ui_settings_row_maintenance, 378, 80);
            lv_obj_clear_flag(ui_ui_settings_row_maintenance, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_maintenance, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_row_maintenance, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_row_maintenance, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_row_maintenance, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_settings_row_maintenance, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_settings_row_maintenance, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_maintenance_icon, 18, 24);
            lv_obj_set_size(ui_ui_settings_row_maintenance_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_settings_row_maintenance_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_maintenance_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_maintenance_icon, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_maintenance_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_maintenance_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_maintenance_label, 66, 27);
            lv_obj_set_size(ui_ui_settings_row_maintenance_label, 170, 27);
            lv_obj_clear_flag(ui_ui_settings_row_maintenance_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_maintenance_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_maintenance_label, "维护");
            lv_obj_set_style_text_color(ui_ui_settings_row_maintenance_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_maintenance_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_row_maintenance_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_maintenance_value, 242, 29);
            lv_obj_set_size(ui_ui_settings_row_maintenance_value, 90, 23);
            lv_obj_clear_flag(ui_ui_settings_row_maintenance_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_maintenance_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_maintenance_value, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_maintenance_value, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_maintenance_value, &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_settings_row_maintenance_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_row_maintenance_arrow, 338, 25);
            lv_obj_set_size(ui_ui_settings_row_maintenance_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_settings_row_maintenance_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_row_maintenance_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_settings_row_maintenance_arrow, "");
            lv_obj_set_style_text_color(ui_ui_settings_row_maintenance_arrow, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_settings_row_maintenance_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_settings_page_track, 399, 88);
            lv_obj_set_size(ui_ui_settings_settings_page_track, 3, 312);
            lv_obj_clear_flag(ui_ui_settings_settings_page_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_page_track, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_settings_page_track, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_settings_page_track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_settings_page_track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_settings_page_thumb, 399, 88);
            lv_obj_set_size(ui_ui_settings_settings_page_thumb, 3, 152);
            lv_obj_clear_flag(ui_ui_settings_settings_page_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_settings_page_thumb, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_settings_page_thumb, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_settings_page_thumb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_settings_page_thumb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_settings_gesture_zone_settings_return, 117, 446);
            lv_obj_set_size(ui_ui_settings_gesture_zone_settings_return, 176, 52);
            lv_obj_clear_flag(ui_ui_settings_gesture_zone_settings_return, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_settings_gesture_zone_settings_return, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_settings_gesture_zone_settings_return, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_settings_gesture_zone_settings_return, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_settings_gesture_zone_settings_return, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_DEVICE_INFO:
            if (ui_ui_device_info_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_bound_mac == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_firmware == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_leg_positions == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_watch_id == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_firmware_icon == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_firmware_label == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_firmware_value == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_watch_id_icon == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_watch_id_label == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_watch_id_value == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_bound_mac_icon == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_bound_mac_label == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_bound_mac_value == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_leg_positions_icon == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_left_leg_position_label == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_left_leg_position_value == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_leg_positions_divider == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_right_leg_position_label == NULL)
            {
                return false;
            }
            if (ui_ui_device_info_row_right_leg_position_value == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_device_info_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_device_info_row_bound_mac, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_bound_mac, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_row_bound_mac, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_device_info_row_firmware, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_firmware, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_row_firmware, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_device_info_row_leg_positions, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_leg_positions, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_row_leg_positions, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_device_info_row_watch_id, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_watch_id, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_row_watch_id, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_device_info_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_device_info_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_device_info_txt_page_title, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_device_info_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_device_info_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_device_info_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_device_info_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_device_info_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_device_info_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_device_info_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_device_info_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_txt_page_title, "设备信息");
            lv_obj_set_style_text_color(ui_ui_device_info_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_device_info_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_device_info_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_firmware, 16, 72);
            lv_obj_set_size(ui_ui_device_info_row_firmware, 378, 80);
            lv_obj_clear_flag(ui_ui_device_info_row_firmware, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_firmware, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_row_firmware, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_row_firmware, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_row_firmware, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_device_info_row_firmware, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_device_info_row_firmware, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_firmware_icon, 18, 24);
            lv_obj_set_size(ui_ui_device_info_row_firmware_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_device_info_row_firmware_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_firmware_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_firmware_icon, "");
            lv_obj_set_style_text_color(ui_ui_device_info_row_firmware_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_firmware_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_firmware_label, 66, 27);
            lv_obj_set_size(ui_ui_device_info_row_firmware_label, 160, 27);
            lv_obj_clear_flag(ui_ui_device_info_row_firmware_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_firmware_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_firmware_label, "手环版本");
            lv_obj_set_style_text_color(ui_ui_device_info_row_firmware_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_firmware_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_firmware_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_firmware_value, 238, 29);
            lv_obj_set_size(ui_ui_device_info_row_firmware_value, 124, 23);
            lv_obj_clear_flag(ui_ui_device_info_row_firmware_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_firmware_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_firmware_value, "1.0.0");
            lv_obj_set_style_text_color(ui_ui_device_info_row_firmware_value, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_firmware_value, &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_firmware_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_watch_id, 16, 160);
            lv_obj_set_size(ui_ui_device_info_row_watch_id, 378, 80);
            lv_obj_clear_flag(ui_ui_device_info_row_watch_id, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_watch_id, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_row_watch_id, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_row_watch_id, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_row_watch_id, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_device_info_row_watch_id, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_device_info_row_watch_id, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_watch_id_icon, 18, 24);
            lv_obj_set_size(ui_ui_device_info_row_watch_id_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_device_info_row_watch_id_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_watch_id_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_watch_id_icon, "");
            lv_obj_set_style_text_color(ui_ui_device_info_row_watch_id_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_watch_id_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_watch_id_label, 66, 27);
            lv_obj_set_size(ui_ui_device_info_row_watch_id_label, 160, 27);
            lv_obj_clear_flag(ui_ui_device_info_row_watch_id_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_watch_id_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_watch_id_label, "Watch ID");
            lv_obj_set_style_text_color(ui_ui_device_info_row_watch_id_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_watch_id_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_watch_id_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_watch_id_value, 220, 31);
            lv_obj_set_size(ui_ui_device_info_row_watch_id_value, 142, 18);
            lv_obj_clear_flag(ui_ui_device_info_row_watch_id_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_watch_id_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_watch_id_value, "A4:C8:A2:9B:02:5A");
            lv_obj_set_style_text_color(ui_ui_device_info_row_watch_id_value, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_watch_id_value, &ui_font_mo500_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_watch_id_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_bound_mac, 16, 248);
            lv_obj_set_size(ui_ui_device_info_row_bound_mac, 378, 80);
            lv_obj_clear_flag(ui_ui_device_info_row_bound_mac, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_bound_mac, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_row_bound_mac, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_row_bound_mac, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_row_bound_mac, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_device_info_row_bound_mac, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_device_info_row_bound_mac, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_bound_mac_icon, 18, 24);
            lv_obj_set_size(ui_ui_device_info_row_bound_mac_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_device_info_row_bound_mac_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_bound_mac_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_bound_mac_icon, "");
            lv_obj_set_style_text_color(ui_ui_device_info_row_bound_mac_icon, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_bound_mac_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_bound_mac_label, 66, 29);
            lv_obj_set_size(ui_ui_device_info_row_bound_mac_label, 130, 22);
            lv_obj_clear_flag(ui_ui_device_info_row_bound_mac_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_bound_mac_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_bound_mac_label, "绑定设备 MAC");
            lv_obj_set_style_text_color(ui_ui_device_info_row_bound_mac_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_bound_mac_label, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_bound_mac_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_bound_mac_value, 198, 31);
            lv_obj_set_size(ui_ui_device_info_row_bound_mac_value, 164, 18);
            lv_obj_clear_flag(ui_ui_device_info_row_bound_mac_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_bound_mac_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_bound_mac_value, "A4:C1:38:9B:77:21");
            lv_obj_set_style_text_color(ui_ui_device_info_row_bound_mac_value, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_bound_mac_value, &ui_font_mo500_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_bound_mac_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_leg_positions, 16, 336);
            lv_obj_set_size(ui_ui_device_info_row_leg_positions, 378, 80);
            lv_obj_clear_flag(ui_ui_device_info_row_leg_positions, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_leg_positions, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_row_leg_positions, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_row_leg_positions, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_row_leg_positions, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_device_info_row_leg_positions, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_device_info_row_leg_positions, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_leg_positions_icon, 18, 24);
            lv_obj_set_size(ui_ui_device_info_row_leg_positions_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_device_info_row_leg_positions_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_leg_positions_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_leg_positions_icon, "");
            lv_obj_set_style_text_color(ui_ui_device_info_row_leg_positions_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_leg_positions_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_left_leg_position_label, 66, 11);
            lv_obj_set_size(ui_ui_device_info_row_left_leg_position_label, 114, 22);
            lv_obj_clear_flag(ui_ui_device_info_row_left_leg_position_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_left_leg_position_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_left_leg_position_label, "左腿位置");
            lv_obj_set_style_text_color(ui_ui_device_info_row_left_leg_position_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_left_leg_position_label, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_left_leg_position_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_left_leg_position_value, 66, 40);
            lv_obj_set_size(ui_ui_device_info_row_left_leg_position_value, 114, 23);
            lv_obj_clear_flag(ui_ui_device_info_row_left_leg_position_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_left_leg_position_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_left_leg_position_value, "-2°");
            lv_obj_set_style_text_color(ui_ui_device_info_row_left_leg_position_value, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_left_leg_position_value, &ui_font_mo700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_left_leg_position_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_leg_positions_divider, 190, 16);
            lv_obj_set_size(ui_ui_device_info_row_leg_positions_divider, 2, 48);
            lv_obj_clear_flag(ui_ui_device_info_row_leg_positions_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_leg_positions_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_device_info_row_leg_positions_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_device_info_row_leg_positions_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_device_info_row_leg_positions_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_right_leg_position_label, 208, 11);
            lv_obj_set_size(ui_ui_device_info_row_right_leg_position_label, 136, 22);
            lv_obj_clear_flag(ui_ui_device_info_row_right_leg_position_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_right_leg_position_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_right_leg_position_label, "右腿位置");
            lv_obj_set_style_text_color(ui_ui_device_info_row_right_leg_position_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_right_leg_position_label, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_right_leg_position_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_device_info_row_right_leg_position_value, 208, 40);
            lv_obj_set_size(ui_ui_device_info_row_right_leg_position_value, 136, 23);
            lv_obj_clear_flag(ui_ui_device_info_row_right_leg_position_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_device_info_row_right_leg_position_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_device_info_row_right_leg_position_value, "20°");
            lv_obj_set_style_text_color(ui_ui_device_info_row_right_leg_position_value, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_device_info_row_right_leg_position_value, &ui_font_mo700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_device_info_row_right_leg_position_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_MAINTENANCE:
            if (ui_ui_maintenance_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_cellular == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_clear_binding == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_gps == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_cellular_icon == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_cellular_label == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_cellular_value == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_cellular_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_gps_icon == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_gps_label == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_gps_value == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_selftest_icon == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_selftest_label == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_selftest_value == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_selftest_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_clear_binding_icon == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_clear_binding_label == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_clear_binding_value == NULL)
            {
                return false;
            }
            if (ui_ui_maintenance_row_clear_binding_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_maintenance_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_maintenance_row_cellular, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_maintenance_row_clear_binding, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_maintenance_row_gps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_gps, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_row_gps, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_maintenance_row_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_maintenance_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_maintenance_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_maintenance_txt_page_title, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_maintenance_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_maintenance_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_maintenance_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_maintenance_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_maintenance_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_maintenance_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_maintenance_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_maintenance_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_maintenance_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_maintenance_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_txt_page_title, "维护");
            lv_obj_set_style_text_color(ui_ui_maintenance_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_maintenance_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_maintenance_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_maintenance_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_maintenance_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_maintenance_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_cellular, 16, 72);
            lv_obj_set_size(ui_ui_maintenance_row_cellular, 378, 80);
            lv_obj_clear_flag(ui_ui_maintenance_row_cellular, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_maintenance_row_cellular, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_maintenance_row_cellular, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_maintenance_row_cellular, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_maintenance_row_cellular, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_maintenance_row_cellular, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_cellular_icon, 18, 24);
            lv_obj_set_size(ui_ui_maintenance_row_cellular_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_maintenance_row_cellular_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_cellular_icon, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_cellular_icon, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_cellular_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_cellular_label, 66, 27);
            lv_obj_set_size(ui_ui_maintenance_row_cellular_label, 170, 27);
            lv_obj_clear_flag(ui_ui_maintenance_row_cellular_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_cellular_label, "4G 网络");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_cellular_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_cellular_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_cellular_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_cellular_value, 226, 29);
            lv_obj_set_size(ui_ui_maintenance_row_cellular_value, 104, 23);
            lv_obj_clear_flag(ui_ui_maintenance_row_cellular_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_cellular_value, "已联网");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_cellular_value, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_cellular_value, &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_cellular_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_cellular_arrow, 338, 25);
            lv_obj_set_size(ui_ui_maintenance_row_cellular_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_maintenance_row_cellular_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_cellular_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_cellular_arrow, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_cellular_arrow, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_cellular_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_gps, 16, 160);
            lv_obj_set_size(ui_ui_maintenance_row_gps, 378, 80);
            lv_obj_clear_flag(ui_ui_maintenance_row_gps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_gps, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_maintenance_row_gps, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_maintenance_row_gps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_maintenance_row_gps, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_maintenance_row_gps, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_maintenance_row_gps, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_gps_icon, 18, 24);
            lv_obj_set_size(ui_ui_maintenance_row_gps_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_maintenance_row_gps_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_gps_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_gps_icon, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_gps_icon, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_gps_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_gps_label, 66, 27);
            lv_obj_set_size(ui_ui_maintenance_row_gps_label, 170, 27);
            lv_obj_clear_flag(ui_ui_maintenance_row_gps_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_gps_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_gps_label, "GPS");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_gps_label, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_gps_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_gps_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_gps_value, 226, 29);
            lv_obj_set_size(ui_ui_maintenance_row_gps_value, 136, 23);
            lv_obj_clear_flag(ui_ui_maintenance_row_gps_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_gps_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_gps_value, "不可用");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_gps_value, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_gps_value, &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_gps_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_selftest, 16, 248);
            lv_obj_set_size(ui_ui_maintenance_row_selftest, 378, 80);
            lv_obj_clear_flag(ui_ui_maintenance_row_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_maintenance_row_selftest, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_maintenance_row_selftest, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_maintenance_row_selftest, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_maintenance_row_selftest, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_maintenance_row_selftest, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_selftest_icon, 18, 24);
            lv_obj_set_size(ui_ui_maintenance_row_selftest_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_maintenance_row_selftest_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_selftest_icon, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_selftest_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_selftest_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_selftest_label, 66, 27);
            lv_obj_set_size(ui_ui_maintenance_row_selftest_label, 170, 27);
            lv_obj_clear_flag(ui_ui_maintenance_row_selftest_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_selftest_label, "整机自检");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_selftest_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_selftest_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_selftest_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_selftest_value, 242, 27);
            lv_obj_set_size(ui_ui_maintenance_row_selftest_value, 90, 26);
            lv_obj_clear_flag(ui_ui_maintenance_row_selftest_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_selftest_value, "开始");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_selftest_value, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_selftest_value, &ui_font_ns600_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_selftest_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_selftest_arrow, 338, 25);
            lv_obj_set_size(ui_ui_maintenance_row_selftest_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_maintenance_row_selftest_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_selftest_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_selftest_arrow, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_selftest_arrow, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_selftest_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_clear_binding, 16, 336);
            lv_obj_set_size(ui_ui_maintenance_row_clear_binding, 378, 80);
            lv_obj_clear_flag(ui_ui_maintenance_row_clear_binding, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_maintenance_row_clear_binding, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_maintenance_row_clear_binding, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_maintenance_row_clear_binding, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_maintenance_row_clear_binding, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_maintenance_row_clear_binding, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_clear_binding_icon, 18, 24);
            lv_obj_set_size(ui_ui_maintenance_row_clear_binding_icon, 34, 32);
            lv_obj_clear_flag(ui_ui_maintenance_row_clear_binding_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_clear_binding_icon, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_clear_binding_icon, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_clear_binding_icon, &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_clear_binding_label, 66, 27);
            lv_obj_set_size(ui_ui_maintenance_row_clear_binding_label, 170, 27);
            lv_obj_clear_flag(ui_ui_maintenance_row_clear_binding_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_clear_binding_label, "清除绑定");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_clear_binding_label, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_clear_binding_label, &ui_font_ns600_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_clear_binding_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_clear_binding_value, 242, 27);
            lv_obj_set_size(ui_ui_maintenance_row_clear_binding_value, 90, 26);
            lv_obj_clear_flag(ui_ui_maintenance_row_clear_binding_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding_value, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_clear_binding_value, "危险操作");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_clear_binding_value, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_clear_binding_value, &ui_font_ns600_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_maintenance_row_clear_binding_value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_maintenance_row_clear_binding_arrow, 338, 25);
            lv_obj_set_size(ui_ui_maintenance_row_clear_binding_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_maintenance_row_clear_binding_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_maintenance_row_clear_binding_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_maintenance_row_clear_binding_arrow, "");
            lv_obj_set_style_text_color(ui_ui_maintenance_row_clear_binding_arrow, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_maintenance_row_clear_binding_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_BLE_EMPTY:
            if (ui_ui_ble_candidates_btn_rescan == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_1 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_2 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_3 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_4 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_rescan == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_icon_no_devices == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_txt_no_devices == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_icon_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_panel_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_txt_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_rescan_icon == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_rescan_label == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_rescan_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_ble_candidates_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_icon_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_txt_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_icon_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_panel_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_txt_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_ble_empty_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_ble_empty_btn_back, 56, 56);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_icon_back, 14, 15);
            lv_obj_set_size(ui_ui_ble_empty_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_ble_empty_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_ble_empty_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_txt_page_title, 104, 27);
            lv_obj_set_size(ui_ui_ble_empty_txt_page_title, 202, 27);
            lv_obj_clear_flag(ui_ui_ble_empty_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_txt_page_title, "附近外骨骼");
            lv_obj_set_style_text_color(ui_ui_ble_empty_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_empty_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_ble_empty_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_ble_empty_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_icon_no_devices, 145, 96);
            lv_obj_set_size(ui_ui_ble_empty_icon_no_devices, 120, 112);
            lv_obj_clear_flag(ui_ui_ble_empty_icon_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_icon_no_devices, "");
            lv_obj_set_style_text_color(ui_ui_ble_empty_icon_no_devices, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_icon_no_devices, &ui_font_lucide_120, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_txt_no_devices, 46, 235);
            lv_obj_set_size(ui_ui_ble_empty_txt_no_devices, 318, 40);
            lv_obj_clear_flag(ui_ui_ble_empty_txt_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_txt_no_devices, "未发现外骨骼");
            lv_obj_set_style_text_color(ui_ui_ble_empty_txt_no_devices, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_txt_no_devices, &ui_font_ns700_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_empty_txt_no_devices, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_btn_rescan, 16, 330);
            lv_obj_set_size(ui_ui_ble_empty_btn_rescan, 378, 96);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_btn_rescan, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_btn_rescan, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_btn_rescan, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_btn_rescan_icon, 22, 29);
            lv_obj_set_size(ui_ui_ble_empty_btn_rescan_icon, 40, 38);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_rescan_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_btn_rescan_icon, "");
            lv_obj_set_style_text_color(ui_ui_ble_empty_btn_rescan_icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_btn_rescan_icon, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_btn_rescan_label, 78, 30);
            lv_obj_set_size(ui_ui_ble_empty_btn_rescan_label, 220, 36);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_rescan_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_btn_rescan_label, "重新搜索");
            lv_obj_set_style_text_color(ui_ui_ble_empty_btn_rescan_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_btn_rescan_label, &ui_font_ns700_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_empty_btn_rescan_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_btn_rescan_arrow, 330, 33);
            lv_obj_set_size(ui_ui_ble_empty_btn_rescan_arrow, 32, 30);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_rescan_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_btn_rescan_arrow, "");
            lv_obj_set_style_text_color(ui_ui_ble_empty_btn_rescan_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_btn_rescan_arrow, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_BLE_CANDIDATES:
            if (ui_ui_ble_candidates_btn_rescan == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_1 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_2 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_3 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_4 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_rescan == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_icon_no_devices == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_txt_no_devices == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_icon_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_panel_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_txt_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_icon_back == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW) == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_btn_rescan_icon == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW) == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_ble_candidates_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_icon_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_txt_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_icon_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_panel_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_txt_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_ble_empty_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_ble_empty_btn_back, 56, 56);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_icon_back, 14, 15);
            lv_obj_set_size(ui_ui_ble_empty_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_ble_empty_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_ble_empty_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_ble_empty_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_ble_empty_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_candidates_device_candidate_1, 24, 76);
            lv_obj_set_size(ui_ui_ble_candidates_device_candidate_1, 362, 88);
            lv_obj_clear_flag(ui_ui_ble_candidates_device_candidate_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_candidates_device_candidate_1, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_candidates_device_candidate_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_candidates_device_candidate_1, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_ble_candidates_device_candidate_1, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_ble_candidates_device_candidate_1, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 18, 28);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 66, 17);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 240, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), "Pro-202607-00010");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), &ui_font_mo700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 66, 51);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 180, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), "信号强");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 322, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_1, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_candidates_device_candidate_2, 24, 168);
            lv_obj_set_size(ui_ui_ble_candidates_device_candidate_2, 362, 88);
            lv_obj_clear_flag(ui_ui_ble_candidates_device_candidate_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_candidates_device_candidate_2, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_candidates_device_candidate_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_candidates_device_candidate_2, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_ble_candidates_device_candidate_2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_ble_candidates_device_candidate_2, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 18, 28);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 66, 17);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 240, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), "Max-202607-00001");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), &ui_font_mo700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 66, 51);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 180, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), "信号良好");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 322, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_2, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_candidates_device_candidate_3, 24, 260);
            lv_obj_set_size(ui_ui_ble_candidates_device_candidate_3, 362, 88);
            lv_obj_clear_flag(ui_ui_ble_candidates_device_candidate_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_candidates_device_candidate_3, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_candidates_device_candidate_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_candidates_device_candidate_3, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_ble_candidates_device_candidate_3, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_ble_candidates_device_candidate_3, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 18, 28);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 66, 17);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 240, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), "Mini-202607-00010");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), &ui_font_mo700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 66, 51);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 180, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), "信号较弱");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 322, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_3, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_candidates_btn_rescan, 282, 16);
            lv_obj_set_size(ui_ui_ble_candidates_btn_rescan, 56, 56);
            lv_obj_clear_flag(ui_ui_ble_candidates_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_candidates_btn_rescan, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_candidates_btn_rescan, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_candidates_btn_rescan, 28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_candidates_btn_rescan_icon, 12, 13);
            lv_obj_set_size(ui_ui_ble_candidates_btn_rescan_icon, 32, 30);
            lv_obj_clear_flag(ui_ui_ble_candidates_btn_rescan_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_candidates_btn_rescan_icon, "");
            lv_obj_set_style_text_color(ui_ui_ble_candidates_btn_rescan_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_candidates_btn_rescan_icon, &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_txt_page_title, 136, 27);
            lv_obj_set_size(ui_ui_ble_empty_txt_page_title, 138, 27);
            lv_obj_clear_flag(ui_ui_ble_empty_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_txt_page_title, "附近外骨骼");
            lv_obj_set_style_text_color(ui_ui_ble_empty_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_empty_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_candidates_device_candidate_4, 24, 352);
            lv_obj_set_size(ui_ui_ble_candidates_device_candidate_4, 362, 88);
            lv_obj_clear_flag(ui_ui_ble_candidates_device_candidate_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_candidates_device_candidate_4, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_candidates_device_candidate_4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_candidates_device_candidate_4, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_ble_candidates_device_candidate_4, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_ble_candidates_device_candidate_4, lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 18, 28);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), 34, 32);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON), &ui_font_lucide_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 66, 17);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), 240, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), "Pro-202607-00008");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), &ui_font_mo700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 66, 51);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), 180, 23);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), "信号弱");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), &ui_font_ns500_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 322, 29);
            lv_obj_set_size(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), 32, 30);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), lv_color_hex(0x0A84FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_ble_candidates_device_candidate_4, UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW), &ui_font_lucide_32, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_IDLE:
            if (ui_ui_selftest_idle_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_rect_selftest_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_domains == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_scope == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_seconds == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_question_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_coordinate == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_action_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_countdown_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_question == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_countdown == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_icon_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_lbl_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_value == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_current_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_marker == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_percent == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_total == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_progress == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest_icon == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_idle_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_idle_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_idle_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_idle_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_page_title, "整机自检");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_idle_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_icon_selftest_idle, 40, 105);
            lv_obj_set_size(ui_ui_selftest_idle_icon_selftest_idle, 52, 38);
            lv_obj_clear_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_icon_selftest_idle, "");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_icon_selftest_idle, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_icon_selftest_idle, &ui_font_lucide_52, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_selftest_idle, 40, 164);
            lv_obj_set_size(ui_ui_selftest_idle_txt_selftest_idle, 338, 47);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_selftest_idle, "尚未运行自检");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_selftest_idle, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_selftest_idle, &ui_font_ns700_38, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_selftest_idle, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_selftest_scope, 40, 226);
            lv_obj_set_size(ui_ui_selftest_idle_txt_selftest_scope, 338, 23);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_selftest_scope, "8 类 · 12 项内部检查");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_selftest_scope, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_selftest_scope, &ui_font_ns500_19, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_selftest_scope, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_start_selftest, 24, 382);
            lv_obj_set_size(ui_ui_selftest_idle_btn_start_selftest, 362, 64);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_btn_start_selftest, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_btn_start_selftest, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_btn_start_selftest, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_start_selftest_icon, 20, 20);
            lv_obj_set_size(ui_ui_selftest_idle_btn_start_selftest_icon, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_start_selftest_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_btn_start_selftest_icon, "");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_btn_start_selftest_icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_btn_start_selftest_icon, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_start_selftest_label, 62, 19);
            lv_obj_set_size(ui_ui_selftest_idle_btn_start_selftest_label, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_start_selftest_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_btn_start_selftest_label, "开始自检");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_btn_start_selftest_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_btn_start_selftest_label, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_btn_start_selftest_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_start_selftest_arrow, 330, 20);
            lv_obj_set_size(ui_ui_selftest_idle_btn_start_selftest_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_start_selftest_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_btn_start_selftest_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_btn_start_selftest_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_btn_start_selftest_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_rect_selftest_status_spine, 16, 92);
            lv_obj_set_size(ui_ui_selftest_idle_rect_selftest_status_spine, 6, 258);
            lv_obj_clear_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_rect_selftest_status_spine, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_rect_selftest_status_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_rect_selftest_status_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_selftest_eyebrow, 108, 99);
            lv_obj_set_size(ui_ui_selftest_idle_txt_selftest_eyebrow, 270, 24);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_selftest_eyebrow, "SYSTEM CHECK");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_selftest_eyebrow, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_selftest_eyebrow, &ui_font_mo700_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_selftest_count, 40, 268);
            lv_obj_set_size(ui_ui_selftest_idle_txt_selftest_count, 120, 90);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_selftest_count, "08");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_selftest_count, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_selftest_count, &ui_font_mo700_76, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_selftest_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_selftest_count_label, 154, 286);
            lv_obj_set_size(ui_ui_selftest_idle_txt_selftest_count_label, 224, 30);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_selftest_count_label, "类硬件检查");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_selftest_count_label, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_selftest_count_label, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_selftest_count_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_selftest_domains, 154, 320);
            lv_obj_set_size(ui_ui_selftest_idle_txt_selftest_domains, 224, 26);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_selftest_domains, "显示 · 触摸 · 传感 · 通信");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_selftest_domains, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_selftest_domains, &ui_font_ns500_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_selftest_domains, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_PROGRESS:
            if (ui_ui_selftest_idle_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_rect_selftest_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_domains == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_scope == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_seconds == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_question_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_coordinate == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_action_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_countdown_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_question == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_countdown == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_icon_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_lbl_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_value == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_current_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_marker == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_percent == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_total == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_progress == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest_icon == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_idle_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_idle_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_idle_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_idle_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_page_title, "整机自检");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_idle_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_txt_progress_label, 44, 110);
            lv_obj_set_size(ui_ui_selftest_progress_txt_progress_label, 120, 22);
            lv_obj_clear_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_txt_progress_label, "已完成");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_txt_progress_label, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_txt_progress_label, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_txt_progress_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_val_progress, 40, 133);
            lv_obj_set_size(ui_ui_selftest_progress_val_progress, 150, 87);
            lv_obj_clear_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_val_progress, "03");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_val_progress, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_val_progress, &ui_font_mo700_82, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_val_progress, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_progress_track, 16, 92);
            lv_obj_set_size(ui_ui_selftest_progress_progress_track, 6, 242);
            lv_obj_clear_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_progress_track, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_progress_track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_progress_track, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_progress_value, 16, 92);
            lv_obj_set_size(ui_ui_selftest_progress_progress_value, 6, 91);
            lv_obj_clear_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_progress_value, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_progress_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_progress_value, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_icon_current_item, 44, 265);
            lv_obj_set_size(ui_ui_selftest_progress_icon_current_item, 36, 34);
            lv_obj_clear_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_icon_current_item, "");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_icon_current_item, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_icon_current_item, &ui_font_lucide_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_lbl_current_item, 92, 257);
            lv_obj_set_size(ui_ui_selftest_progress_lbl_current_item, 200, 21);
            lv_obj_clear_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_lbl_current_item, "当前项目");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_lbl_current_item, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_lbl_current_item, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_lbl_current_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_val_current_item, 92, 285);
            lv_obj_set_size(ui_ui_selftest_progress_val_current_item, 286, 36);
            lv_obj_clear_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_val_current_item, "GPS 定位");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_val_current_item, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_val_current_item, &ui_font_ns700_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_val_current_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest, 24, 382);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest, 362, 64);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_btn_leave_selftest, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_btn_leave_selftest, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_btn_leave_selftest, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_progress_btn_leave_selftest, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_progress_btn_leave_selftest, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest_icon, 20, 20);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest_icon, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_btn_leave_selftest_icon, "");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_btn_leave_selftest_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_btn_leave_selftest_icon, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest_label, 62, 19);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest_label, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_btn_leave_selftest_label, "返回维护");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_btn_leave_selftest_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_btn_leave_selftest_label, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_btn_leave_selftest_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest_arrow, 330, 20);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_btn_leave_selftest_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_btn_leave_selftest_arrow, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_btn_leave_selftest_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_txt_progress_eyebrow, 44, 82);
            lv_obj_set_size(ui_ui_selftest_progress_txt_progress_eyebrow, 250, 22);
            lv_obj_clear_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_txt_progress_eyebrow, "SYSTEM CHECK · RUNNING");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_txt_progress_eyebrow, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_txt_progress_eyebrow, &ui_font_mo700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_txt_progress_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_txt_progress_total, 178, 168);
            lv_obj_set_size(ui_ui_selftest_progress_txt_progress_total, 90, 40);
            lv_obj_clear_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_txt_progress_total, "/ 08");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_txt_progress_total, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_txt_progress_total, &ui_font_mo600_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_txt_progress_total, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_txt_progress_percent, 292, 172);
            lv_obj_set_size(ui_ui_selftest_progress_txt_progress_percent, 86, 34);
            lv_obj_clear_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_txt_progress_percent, "38%");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_txt_progress_percent, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_txt_progress_percent, &ui_font_mo700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_txt_progress_percent, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_rect_progress_current_divider, 44, 236);
            lv_obj_set_size(ui_ui_selftest_progress_rect_progress_current_divider, 334, 2);
            lv_obj_clear_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_rect_progress_current_divider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_rect_progress_current_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_rect_progress_current_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_rect_progress_marker, 12, 240);
            lv_obj_set_size(ui_ui_selftest_progress_rect_progress_marker, 14, 4);
            lv_obj_clear_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_rect_progress_marker, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_rect_progress_marker, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_rect_progress_marker, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_MANUAL:
            if (ui_ui_selftest_idle_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_rect_selftest_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_domains == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_scope == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_seconds == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_question_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_coordinate == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_action_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_countdown_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_question == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_countdown == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_icon_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_lbl_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_value == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_current_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_marker == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_percent == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_total == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_progress == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_icon_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_icon_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_pass == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_idle_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_page_title, "人工确认");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_idle_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_item, 44, 85);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_item, 260, 19);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_item, "人工确认 · 01 / 08");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_item, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_item, &ui_font_ns700_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_val_manual_item, 40, 114);
            lv_obj_set_size(ui_ui_selftest_manual_val_manual_item, 338, 51);
            lv_obj_clear_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_val_manual_item, "屏幕显示");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_val_manual_item, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_val_manual_item, &ui_font_ns700_42, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_val_manual_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_question, 40, 185);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_question, 338, 27);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_question, "画面是否完整、无闪烁？");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_question, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_question, &ui_font_ns700_23, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_question, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_val_manual_countdown, 40, 255);
            lv_obj_set_size(ui_ui_selftest_manual_val_manual_countdown, 130, 85);
            lv_obj_clear_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_val_manual_countdown, "18");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_val_manual_countdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_val_manual_countdown, &ui_font_mo700_80, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_val_manual_countdown, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_seconds, 164, 296);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_seconds, 60, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_seconds, "秒");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_seconds, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_seconds, &ui_font_ns600_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_seconds, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_btn_manual_fail, 24, 382);
            lv_obj_set_size(ui_ui_selftest_manual_btn_manual_fail, 142, 64);
            lv_obj_clear_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_btn_manual_fail, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_btn_manual_fail, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_btn_manual_fail, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_manual_btn_manual_fail, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_manual_btn_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_icon_manual_fail, 14, 20);
            lv_obj_set_size(ui_ui_selftest_manual_icon_manual_fail, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_manual_icon_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_icon_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_icon_manual_fail, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_icon_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_icon_manual_fail, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_fail, 44, 19);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_fail, 84, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_fail, "失败");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_fail, &ui_font_ns700_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_fail, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_btn_manual_pass, 174, 382);
            lv_obj_set_size(ui_ui_selftest_manual_btn_manual_pass, 212, 64);
            lv_obj_clear_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_btn_manual_pass, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_btn_manual_pass, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_btn_manual_pass, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_icon_manual_pass, 28, 20);
            lv_obj_set_size(ui_ui_selftest_manual_icon_manual_pass, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_manual_icon_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_icon_manual_pass, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_icon_manual_pass, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_icon_manual_pass, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_icon_manual_pass, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_pass, 62, 19);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_pass, 130, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_pass, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_pass, "通过");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_pass, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_pass, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_pass, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_rect_manual_status_spine, 16, 88);
            lv_obj_set_size(ui_ui_selftest_manual_rect_manual_status_spine, 6, 252);
            lv_obj_clear_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_rect_manual_status_spine, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_rect_manual_status_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_rect_manual_status_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_rect_manual_question_divider, 40, 232);
            lv_obj_set_size(ui_ui_selftest_manual_rect_manual_question_divider, 338, 2);
            lv_obj_clear_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_rect_manual_question_divider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_rect_manual_question_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_rect_manual_question_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_countdown_label, 40, 238);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_countdown_label, 230, 22);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_countdown_label, "超时将记为失败");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_countdown_label, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_countdown_label, &ui_font_ns500_15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_countdown_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_action_hint, 238, 276);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_action_hint, 140, 52);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_action_hint, "观察屏幕后\n选择结果");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_action_hint, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_action_hint, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_action_hint, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_RESULT:
            if (ui_ui_selftest_result_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_btn_rerun_all == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_btn_retry_failed == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_pass_rule == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_skip_rule == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_list == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_rule == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_scroll_thumb == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_scroll_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_failure_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_failure_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_pass_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_pass_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_skip_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_skip_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_summary == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_btn_retry_failed_icon == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_btn_retry_failed_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_display == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_touch == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_audio == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_gps == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_01 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_02 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_03 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_04 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_power == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_sensor == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_cellular == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_ble == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_05 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_06 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_07 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_icon_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_lbl_manual_fail == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_result_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_btn_rerun_all, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_rerun_all, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_btn_rerun_all, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_btn_retry_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_rect_result_pass_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_pass_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_pass_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_rect_result_skip_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_skip_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_skip_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_list, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_list, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_scroll_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_thumb, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_thumb, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_scroll_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_failure_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_failure_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_pass_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_pass_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_skip_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_skip_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_summary, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_summary, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_summary, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_result_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_result_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_result_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_result_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_result_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_result_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_page_title, "自检结果");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_result_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_summary, 16, 78);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_summary, 180, 19);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_summary, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_summary, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_summary, "本轮完成");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_summary, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_summary, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_summary, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_rule, 16, 184);
            lv_obj_set_size(ui_ui_selftest_result_result_rule, 46, 4);
            lv_obj_clear_flag(ui_ui_selftest_result_result_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_rule, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_rule, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_btn_retry_failed, 174, 390);
            lv_obj_set_size(ui_ui_selftest_result_btn_retry_failed, 212, 56);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_retry_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_btn_retry_failed, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_btn_retry_failed, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_btn_retry_failed, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_btn_retry_failed_icon, 18, 16);
            lv_obj_set_size(ui_ui_selftest_result_btn_retry_failed_icon, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_retry_failed_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_btn_retry_failed_icon, "");
            lv_obj_set_style_text_color(ui_ui_selftest_result_btn_retry_failed_icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_btn_retry_failed_icon, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_btn_retry_failed_label, 56, 17);
            lv_obj_set_size(ui_ui_selftest_result_btn_retry_failed_label, 144, 23);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_retry_failed_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_btn_retry_failed_label, "重试失败项");
            lv_obj_set_style_text_color(ui_ui_selftest_result_btn_retry_failed_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_btn_retry_failed_label, &ui_font_ns700_19, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_btn_retry_failed_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_failure_count, 16, 96);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_failure_count, 112, 78);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_failure_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_failure_count, "01");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_failure_count, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_failure_count, &ui_font_mo700_68, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_failure_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_failure_label, 124, 124);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_failure_label, 112, 34);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_failure_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_failure_label, "项失败");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_failure_label, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_failure_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_failure_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_pass_count, 252, 92);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_pass_count, 48, 34);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_pass_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_pass_count, "06");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_pass_count, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_pass_count, &ui_font_mo700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_pass_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_pass_label, 304, 98);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_pass_label, 72, 28);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_pass_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_pass_label, "通过");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_pass_label, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_pass_label, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_pass_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_skip_count, 252, 134);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_skip_count, 48, 34);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_skip_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_skip_count, "01");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_skip_count, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_skip_count, &ui_font_mo700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_skip_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_skip_label, 304, 140);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_skip_label, 72, 28);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_skip_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_skip_label, "跳过");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_skip_label, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_skip_label, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_skip_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_pass_rule, 64, 184);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_pass_rule, 282, 4);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_pass_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_pass_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_pass_rule, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_pass_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_pass_rule, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_skip_rule, 348, 184);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_skip_rule, 46, 4);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_skip_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_skip_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_skip_rule, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_skip_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_skip_rule, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_list, 16, 204);
            lv_obj_set_size(ui_ui_selftest_result_result_list, 378, 196);
            lv_obj_clear_flag(ui_ui_selftest_result_result_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_list, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_list, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_display, 0, 0);
            lv_obj_set_size(ui_ui_selftest_result_result_display, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_display, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_display, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_display, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_display, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_display, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "屏幕显示");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_touch, 0, 46);
            lv_obj_set_size(ui_ui_selftest_result_result_touch, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_touch, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_touch, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_touch, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_touch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_touch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "触摸");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_audio, 0, 184);
            lv_obj_set_size(ui_ui_selftest_result_result_audio, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_audio, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_audio, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_audio, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_audio, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_audio, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "音频资源");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "跳过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_gps, 0, 230);
            lv_obj_set_size(ui_ui_selftest_result_result_gps, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_gps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_gps, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_gps, lv_color_hex(0x2B1517), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_gps, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_gps, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "GPS 定位");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "失败");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_01, 38, 44);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_01, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_01, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_01, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_01, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_01, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_01, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_02, 38, 90);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_02, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_02, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_02, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_02, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_02, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_02, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_03, 38, 136);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_03, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_03, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_03, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_03, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_03, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_03, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_04, 38, 182);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_04, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_04, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_04, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_04, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_04, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_04, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_power, 0, 92);
            lv_obj_set_size(ui_ui_selftest_result_result_power, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_power, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_power, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_power, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "供电");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_sensor, 0, 138);
            lv_obj_set_size(ui_ui_selftest_result_result_sensor, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_sensor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_sensor, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_sensor, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_sensor, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_sensor, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "传感");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_cellular, 0, 276);
            lv_obj_set_size(ui_ui_selftest_result_result_cellular, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_cellular, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_cellular, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_cellular, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_cellular, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_cellular, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "4G 网络");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_ble, 0, 322);
            lv_obj_set_size(ui_ui_selftest_result_result_ble, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_ble, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_ble, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_ble, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_ble, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_ble, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "BLE");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_05, 38, 228);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_05, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_05, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_05, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_05, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_05, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_05, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_06, 38, 274);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_06, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_06, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_06, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_06, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_06, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_06, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_07, 38, 320);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_07, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_07, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_07, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_07, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_07, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_07, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_scroll_track, 400, 216);
            lv_obj_set_size(ui_ui_selftest_result_result_scroll_track, 3, 168);
            lv_obj_clear_flag(ui_ui_selftest_result_result_scroll_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_track, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_scroll_track, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_scroll_track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_scroll_track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_scroll_thumb, 400, 216);
            lv_obj_set_size(ui_ui_selftest_result_result_scroll_thumb, 3, 72);
            lv_obj_clear_flag(ui_ui_selftest_result_result_scroll_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_thumb, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_scroll_thumb, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_scroll_thumb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_scroll_thumb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_btn_rerun_all, 24, 390);
            lv_obj_set_size(ui_ui_selftest_result_btn_rerun_all, 134, 56);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_rerun_all, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_rerun_all, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_btn_rerun_all, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_btn_rerun_all, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_btn_rerun_all, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_result_btn_rerun_all, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_result_btn_rerun_all, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_icon_manual_fail, 12, 16);
            lv_obj_set_size(ui_ui_selftest_result_icon_manual_fail, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_result_icon_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_icon_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_icon_manual_fail, "");
            lv_obj_set_style_text_color(ui_ui_selftest_result_icon_manual_fail, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_icon_manual_fail, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_lbl_manual_fail, 38, 17);
            lv_obj_set_size(ui_ui_selftest_result_lbl_manual_fail, 96, 22);
            lv_obj_clear_flag(ui_ui_selftest_result_lbl_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_lbl_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_lbl_manual_fail, "重新自检");
            lv_obj_set_style_text_color(ui_ui_selftest_result_lbl_manual_fail, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_lbl_manual_fail, &ui_font_ns700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_lbl_manual_fail, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_GPS_CONTEXT:
            if (ui_ui_selftest_gps_context_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_btn_gps_indoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_btn_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_context == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_rect_gps_context_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_submit_loading_btn_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_lbl_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_outdoor_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_outdoor_arrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_indoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_lbl_gps_indoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_indoor_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_indoor_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_gps_indoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_icon_gps_context, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_gps_context_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_gps_context_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_page_title, "定位环境");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_gps_context_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_rect_gps_context_spine, 16, 92);
            lv_obj_set_size(ui_ui_selftest_gps_context_rect_gps_context_spine, 6, 190);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_rect_gps_context_spine, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_rect_gps_context_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_rect_gps_context_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, 40, 82);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, 300, 24);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, "GPS FIX · ENVIRONMENT");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, &ui_font_mo700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_context, 40, 117);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_context, 48, 46);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_context, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_context, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_context, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_context, &ui_font_lucide_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_title, 40, 177);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_title, 338, 43);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_title, "现在在哪里测试？");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_title, &ui_font_ns700_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_hint, 40, 228);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_hint, 338, 54);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_hint, "选择当前环境，GPS 定位结果才可解释");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_hint, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_hint, &ui_font_ns500_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_btn_gps_outdoor, 16, 294);
            lv_obj_set_size(ui_ui_selftest_gps_context_btn_gps_outdoor, 378, 72);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_btn_gps_outdoor, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_btn_gps_outdoor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_btn_gps_outdoor, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_outdoor, 18, 24);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_outdoor, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_outdoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_outdoor, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_outdoor, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_outdoor, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_lbl_gps_outdoor, 58, 11);
            lv_obj_set_size(ui_ui_selftest_gps_context_lbl_gps_outdoor, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_lbl_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_lbl_gps_outdoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_lbl_gps_outdoor, "室外 / 靠窗");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_lbl_gps_outdoor, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_lbl_gps_outdoor, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_lbl_gps_outdoor, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, 58, 42);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, 250, 19);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, "执行定位 · 最长 180 秒");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, 332, 24);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_btn_gps_indoor, 24, 374);
            lv_obj_set_size(ui_ui_selftest_gps_context_btn_gps_indoor, 362, 72);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_btn_gps_indoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_btn_gps_indoor, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_btn_gps_indoor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_btn_gps_indoor, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_gps_context_btn_gps_indoor, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_gps_context_btn_gps_indoor, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_indoor, 18, 24);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_indoor, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_indoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_indoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_indoor, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_indoor, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_indoor, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_lbl_gps_indoor, 58, 11);
            lv_obj_set_size(ui_ui_selftest_gps_context_lbl_gps_indoor, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_lbl_gps_indoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_lbl_gps_indoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_lbl_gps_indoor, "室内");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_lbl_gps_indoor, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_lbl_gps_indoor, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_lbl_gps_indoor, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_indoor_hint, 58, 42);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_indoor_hint, 250, 19);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_indoor_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_indoor_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_indoor_hint, "GPS 定位项记为跳过");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_indoor_hint, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_indoor_hint, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_indoor_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, 332, 24);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, lv_color_hex(0xFFD60A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_indoor_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_RETRY_DETAIL:
            if (ui_ui_selftest_retry_detail_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_btn_failed_next == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_btn_failed_prev == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_btn_retry_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_icon_retry_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_lbl_retry_code == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_lbl_retry_elapsed == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_lbl_retry_reason == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_rect_retry_detail_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_rect_retry_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_txt_retry_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_txt_retry_scope_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_val_retry_code == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_val_retry_elapsed == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_val_retry_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_val_retry_reason == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_icon_failed_prev == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_icon_failed_next == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_icon_retry_item_action == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_lbl_retry_item_action == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_retry_detail_icon_retry_item_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_retry_detail_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_btn_failed_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_failed_next, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_failed_next, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_btn_failed_prev, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_failed_prev, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_failed_prev, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_btn_retry_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_retry_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_retry_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_icon_retry_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_retry_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_retry_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_lbl_retry_code, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_code, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_code, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_lbl_retry_elapsed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_elapsed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_elapsed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_lbl_retry_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_reason, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_reason, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_rect_retry_detail_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_rect_retry_detail_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_rect_retry_detail_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_rect_retry_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_rect_retry_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_rect_retry_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_txt_retry_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_retry_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_retry_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_txt_retry_scope_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_retry_scope_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_retry_scope_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_val_retry_code, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_code, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_code, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_val_retry_elapsed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_elapsed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_elapsed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_val_retry_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_retry_detail_val_retry_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_reason, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_reason, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_retry_detail_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_retry_detail_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_retry_detail_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_txt_page_title, "失败项详情");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_retry_detail_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_rect_retry_spine, 16, 92);
            lv_obj_set_size(ui_ui_selftest_retry_detail_rect_retry_spine, 6, 258);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_rect_retry_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_rect_retry_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_rect_retry_spine, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_rect_retry_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_rect_retry_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_txt_retry_eyebrow, 40, 82);
            lv_obj_set_size(ui_ui_selftest_retry_detail_txt_retry_eyebrow, 210, 24);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_txt_retry_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_retry_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_txt_retry_eyebrow, "FAILED ITEM · 01 / 03");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_txt_retry_eyebrow, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_txt_retry_eyebrow, &ui_font_mo700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_txt_retry_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_btn_failed_prev, 292, 72);
            lv_obj_set_size(ui_ui_selftest_retry_detail_btn_failed_prev, 44, 44);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_btn_failed_prev, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_failed_prev, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_btn_failed_prev, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_btn_failed_prev, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_btn_failed_prev, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_retry_detail_btn_failed_prev, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_retry_detail_btn_failed_prev, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_icon_failed_prev, 10, 15);
            lv_obj_set_size(ui_ui_selftest_retry_detail_icon_failed_prev, 24, 14);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_icon_failed_prev, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_failed_prev, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_icon_failed_prev, "");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_icon_failed_prev, lv_color_hex(0x484D56), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_icon_failed_prev, &ui_font_lucide_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_btn_failed_next, 344, 72);
            lv_obj_set_size(ui_ui_selftest_retry_detail_btn_failed_next, 44, 44);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_btn_failed_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_failed_next, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_btn_failed_next, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_btn_failed_next, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_btn_failed_next, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_retry_detail_btn_failed_next, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_retry_detail_btn_failed_next, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_icon_failed_next, 10, 15);
            lv_obj_set_size(ui_ui_selftest_retry_detail_icon_failed_next, 24, 14);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_icon_failed_next, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_failed_next, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_icon_failed_next, "");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_icon_failed_next, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_icon_failed_next, &ui_font_lucide_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_icon_retry_item, 40, 123);
            lv_obj_set_size(ui_ui_selftest_retry_detail_icon_retry_item, 44, 42);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_icon_retry_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_retry_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_icon_retry_item, "");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_icon_retry_item, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_icon_retry_item, &ui_font_lucide_44, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_val_retry_item, 100, 122);
            lv_obj_set_size(ui_ui_selftest_retry_detail_val_retry_item, 278, 43);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_val_retry_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_val_retry_item, "GPS 定位");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_val_retry_item, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_val_retry_item, &ui_font_ns700_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_val_retry_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_lbl_retry_reason, 40, 189);
            lv_obj_set_size(ui_ui_selftest_retry_detail_lbl_retry_reason, 120, 21);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_lbl_retry_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_reason, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_lbl_retry_reason, "失败原因");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_lbl_retry_reason, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_lbl_retry_reason, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_lbl_retry_reason, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_val_retry_reason, 40, 215);
            lv_obj_set_size(ui_ui_selftest_retry_detail_val_retry_reason, 338, 32);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_val_retry_reason, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_reason, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_val_retry_reason, "定位超时");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_val_retry_reason, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_val_retry_reason, &ui_font_ns700_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_val_retry_reason, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_rect_retry_detail_divider, 40, 260);
            lv_obj_set_size(ui_ui_selftest_retry_detail_rect_retry_detail_divider, 338, 2);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_rect_retry_detail_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_rect_retry_detail_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_rect_retry_detail_divider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_rect_retry_detail_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_rect_retry_detail_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_lbl_retry_code, 40, 280);
            lv_obj_set_size(ui_ui_selftest_retry_detail_lbl_retry_code, 90, 21);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_lbl_retry_code, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_code, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_lbl_retry_code, "稳定码");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_lbl_retry_code, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_lbl_retry_code, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_lbl_retry_code, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_val_retry_code, 132, 282);
            lv_obj_set_size(ui_ui_selftest_retry_detail_val_retry_code, 246, 17);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_val_retry_code, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_code, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_val_retry_code, "GPS_FIX_TIMEOUT");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_val_retry_code, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_val_retry_code, &ui_font_mo700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_val_retry_code, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_lbl_retry_elapsed, 40, 320);
            lv_obj_set_size(ui_ui_selftest_retry_detail_lbl_retry_elapsed, 90, 21);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_lbl_retry_elapsed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_elapsed, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_lbl_retry_elapsed, "本次耗时");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_lbl_retry_elapsed, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_lbl_retry_elapsed, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_lbl_retry_elapsed, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_val_retry_elapsed, 250, 320);
            lv_obj_set_size(ui_ui_selftest_retry_detail_val_retry_elapsed, 128, 20);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_val_retry_elapsed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_val_retry_elapsed, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_val_retry_elapsed, "180 秒");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_val_retry_elapsed, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_val_retry_elapsed, &ui_font_mo700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_val_retry_elapsed, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_txt_retry_scope_hint, 40, 358);
            lv_obj_set_size(ui_ui_selftest_retry_detail_txt_retry_scope_hint, 338, 21);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_txt_retry_scope_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_txt_retry_scope_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_txt_retry_scope_hint, "仅覆盖本项，其他结果保留");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_txt_retry_scope_hint, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_txt_retry_scope_hint, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_txt_retry_scope_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_btn_retry_item, 24, 382);
            lv_obj_set_size(ui_ui_selftest_retry_detail_btn_retry_item, 362, 64);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_btn_retry_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_btn_retry_item, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_retry_detail_btn_retry_item, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_retry_detail_btn_retry_item, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_retry_detail_btn_retry_item, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_icon_retry_item_action, 18, 20);
            lv_obj_set_size(ui_ui_selftest_retry_detail_icon_retry_item_action, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_icon_retry_item_action, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_retry_item_action, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_icon_retry_item_action, "");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_icon_retry_item_action, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_icon_retry_item_action, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_lbl_retry_item_action, 58, 19);
            lv_obj_set_size(ui_ui_selftest_retry_detail_lbl_retry_item_action, 270, 26);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_lbl_retry_item_action, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_lbl_retry_item_action, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_lbl_retry_item_action, "设置环境并重试");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_lbl_retry_item_action, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_lbl_retry_item_action, &ui_font_ns700_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_retry_detail_lbl_retry_item_action, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_retry_detail_icon_retry_item_arrow, 332, 20);
            lv_obj_set_size(ui_ui_selftest_retry_detail_icon_retry_item_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_retry_detail_icon_retry_item_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_retry_detail_icon_retry_item_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_retry_detail_icon_retry_item_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_retry_detail_icon_retry_item_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_retry_detail_icon_retry_item_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_CLEAR_BINDING_CONFIRM:
            if (ui_ui_clear_binding_confirm_modal_panel == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_icon_clear_binding == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_txt_confirm_title == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_confirm_rule == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_txt_confirm_effect == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_txt_confirm_rebind == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_btn_confirm_clear_binding == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_btn_cancel == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_btn_cancel_icon == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_btn_cancel_label == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_clear_binding_confirm_txt_clear_binding_scope == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_clear_binding_confirm_modal_panel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_modal_panel, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_modal_panel, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_modal_panel, 0, 0);
            lv_obj_set_size(ui_ui_clear_binding_confirm_modal_panel, 410, 502);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_modal_panel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_modal_panel, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_clear_binding_confirm_modal_panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_clear_binding_confirm_modal_panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_clear_binding_confirm_modal_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_icon_clear_binding, 40, 80);
            lv_obj_set_size(ui_ui_clear_binding_confirm_icon_clear_binding, 54, 50);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_icon_clear_binding, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_icon_clear_binding, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_icon_clear_binding, "");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_icon_clear_binding, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_icon_clear_binding, &ui_font_lucide_54, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_txt_confirm_title, 40, 149);
            lv_obj_set_size(ui_ui_clear_binding_confirm_txt_confirm_title, 354, 51);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_txt_confirm_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_txt_confirm_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_txt_confirm_title, "清除绑定？");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_txt_confirm_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_txt_confirm_title, &ui_font_ns700_42, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_txt_confirm_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_confirm_rule, 40, 212);
            lv_obj_set_size(ui_ui_clear_binding_confirm_confirm_rule, 100, 6);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_confirm_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_confirm_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_clear_binding_confirm_confirm_rule, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_clear_binding_confirm_confirm_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_clear_binding_confirm_confirm_rule, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_txt_confirm_effect, 40, 253);
            lv_obj_set_size(ui_ui_clear_binding_confirm_txt_confirm_effect, 338, 30);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_txt_confirm_effect, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_txt_confirm_effect, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_txt_confirm_effect, "将清除绑定设备和控制资格。");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_txt_confirm_effect, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_txt_confirm_effect, &ui_font_ns600_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_txt_confirm_effect, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_txt_confirm_rebind, 40, 321);
            lv_obj_set_size(ui_ui_clear_binding_confirm_txt_confirm_rebind, 338, 26);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_txt_confirm_rebind, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_txt_confirm_rebind, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_txt_confirm_rebind, "下次使用必须重新授权。");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_txt_confirm_rebind, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_txt_confirm_rebind, &ui_font_ns500_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_txt_confirm_rebind, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, 185, 382);
            lv_obj_set_size(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, 201, 64);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_clear_binding_confirm_btn_confirm_clear_binding, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, 22, 18);
            lv_obj_set_size(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, 28, 26);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, "");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_icon, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, 62, 17);
            lv_obj_set_size(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, 135, 30);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, "清除");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_btn_cancel, 24, 382);
            lv_obj_set_size(ui_ui_clear_binding_confirm_btn_cancel, 145, 64);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_btn_cancel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_btn_cancel, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_clear_binding_confirm_btn_cancel, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_clear_binding_confirm_btn_cancel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_clear_binding_confirm_btn_cancel, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_clear_binding_confirm_btn_cancel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_clear_binding_confirm_btn_cancel, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_btn_cancel_icon, 14, 18);
            lv_obj_set_size(ui_ui_clear_binding_confirm_btn_cancel_icon, 28, 26);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_btn_cancel_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_btn_cancel_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_btn_cancel_icon, "");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_btn_cancel_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_btn_cancel_icon, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_btn_cancel_label, 46, 19);
            lv_obj_set_size(ui_ui_clear_binding_confirm_btn_cancel_label, 85, 27);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_btn_cancel_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_btn_cancel_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_btn_cancel_label, "取消");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_btn_cancel_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_btn_cancel_label, &ui_font_ns700_23, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_btn_cancel_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, 16, 72);
            lv_obj_set_size(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, 6, 292);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_clear_binding_confirm_rect_clear_binding_danger_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, 108, 82);
            lv_obj_set_size(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, 270, 24);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, "DANGER · BINDING");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, &ui_font_mo700_15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_txt_clear_binding_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_clear_binding_confirm_txt_clear_binding_scope, 108, 110);
            lv_obj_set_size(ui_ui_clear_binding_confirm_txt_clear_binding_scope, 270, 24);
            lv_obj_clear_flag(ui_ui_clear_binding_confirm_txt_clear_binding_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_clear_binding_confirm_txt_clear_binding_scope, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_clear_binding_confirm_txt_clear_binding_scope, "设备绑定与控制资格");
            lv_obj_set_style_text_color(ui_ui_clear_binding_confirm_txt_clear_binding_scope, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_clear_binding_confirm_txt_clear_binding_scope, &ui_font_ns500_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_clear_binding_confirm_txt_clear_binding_scope, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_MANUAL_TOUCH:
            if (ui_ui_selftest_idle_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_rect_selftest_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_domains == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_scope == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_seconds == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_question_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_coordinate == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_action_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_countdown_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_question == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_countdown == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_icon_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_lbl_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_value == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_current_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_marker == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_percent == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_total == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_progress == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_icon_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_icon_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target_icon == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target_state == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_fill == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_thumb == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_idle_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_page_title, "人工确认");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_idle_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_item, 44, 85);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_item, 260, 19);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_item, "人工确认 · 02 / 08");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_item, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_item, &ui_font_ns700_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_val_manual_item, 40, 116);
            lv_obj_set_size(ui_ui_selftest_manual_val_manual_item, 338, 47);
            lv_obj_clear_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_val_manual_item, "触摸");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_val_manual_item, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_val_manual_item, &ui_font_ns700_38, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_val_manual_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_question, 40, 175);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_question, 338, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_question, "点击目标，再将滑块拖到最右端");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_question, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_question, &ui_font_ns700_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_question, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_val_manual_countdown, 316, 86);
            lv_obj_set_size(ui_ui_selftest_manual_val_manual_countdown, 38, 17);
            lv_obj_clear_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_val_manual_countdown, "30");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_val_manual_countdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_val_manual_countdown, &ui_font_mo700_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_val_manual_countdown, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_seconds, 358, 86);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_seconds, 26, 17);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_seconds, "秒");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_seconds, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_seconds, &ui_font_ns600_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_seconds, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_btn_manual_fail, 24, 382);
            lv_obj_set_size(ui_ui_selftest_manual_btn_manual_fail, 142, 64);
            lv_obj_clear_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_btn_manual_fail, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_btn_manual_fail, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_btn_manual_fail, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_manual_btn_manual_fail, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_manual_btn_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_icon_manual_fail, 14, 20);
            lv_obj_set_size(ui_ui_selftest_manual_icon_manual_fail, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_manual_icon_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_icon_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_icon_manual_fail, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_icon_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_icon_manual_fail, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_fail, 44, 19);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_fail, 84, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_fail, "失败");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_fail, &ui_font_ns700_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_fail, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_btn_manual_pass, 174, 382);
            lv_obj_set_size(ui_ui_selftest_manual_btn_manual_pass, 212, 64);
            lv_obj_clear_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_btn_manual_pass, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_btn_manual_pass, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_btn_manual_pass, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_icon_manual_pass, 28, 20);
            lv_obj_set_size(ui_ui_selftest_manual_icon_manual_pass, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_manual_icon_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_icon_manual_pass, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_icon_manual_pass, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_icon_manual_pass, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_icon_manual_pass, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_pass, 62, 19);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_pass, 130, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_pass, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_pass, "通过");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_pass, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_pass, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_pass, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_rect_manual_status_spine, 16, 88);
            lv_obj_set_size(ui_ui_selftest_manual_rect_manual_status_spine, 6, 286);
            lv_obj_clear_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_rect_manual_status_spine, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_rect_manual_status_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_rect_manual_status_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_rect_manual_question_divider, 40, 206);
            lv_obj_set_size(ui_ui_selftest_manual_rect_manual_question_divider, 338, 2);
            lv_obj_clear_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_rect_manual_question_divider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_rect_manual_question_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_rect_manual_question_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_target, 40, 220);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_target, 338, 62);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_touch_touch_target, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_touch_touch_target, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_touch_touch_target, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_manual_touch_touch_target, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_manual_touch_touch_target, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_target_icon, 18, 18);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_target_icon, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_target_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_touch_touch_target_icon, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_touch_touch_target_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_touch_touch_target_icon, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_target_label, 60, 18);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_target_label, 170, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_target_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_touch_touch_target_label, "点击目标");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_touch_touch_target_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_touch_touch_target_label, &ui_font_ns700_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_touch_touch_target_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_target_state, 240, 22);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_target_state, 78, 19);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_target_state, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target_state, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_touch_touch_target_state, "待点击");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_touch_touch_target_state, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_touch_touch_target_state, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_touch_touch_target_state, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_coordinate, 40, 293);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_coordinate, 338, 19);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_touch_touch_coordinate, "点击坐标：-- , --");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_touch_touch_coordinate, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_touch_touch_coordinate, &ui_font_ns500_15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_touch_touch_coordinate, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_slider_label, 40, 319);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_slider_label, 338, 19);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_touch_touch_slider_label, "向右滑动至 100%");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_touch_touch_slider_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_touch_touch_slider_label, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_touch_touch_slider_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_slider, 40, 346);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_slider, 338, 28);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_touch_touch_slider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_touch_touch_slider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_touch_touch_slider, 14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_slider_fill, 4, 4);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_slider_fill, 30, 20);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_slider_fill, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_fill, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_touch_touch_slider_fill, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_touch_touch_slider_fill, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_touch_touch_slider_fill, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_touch_touch_slider_thumb, 5, 3);
            lv_obj_set_size(ui_ui_selftest_manual_touch_touch_slider_thumb, 22, 22);
            lv_obj_clear_flag(ui_ui_selftest_manual_touch_touch_slider_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_thumb, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_touch_touch_slider_thumb, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_touch_touch_slider_thumb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_touch_touch_slider_thumb, 11, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_BLE_SCANNING:
            if (ui_ui_ble_candidates_btn_rescan == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_1 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_2 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_3 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_candidates_device_candidate_4 == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_btn_rescan == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_icon_no_devices == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_txt_no_devices == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_icon_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_panel_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_txt_scanning == NULL)
            {
                return false;
            }
            if (ui_ui_ble_empty_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_scanning_icon == NULL)
            {
                return false;
            }
            if (ui_ui_ble_scanning_scanning_label == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_ble_candidates_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_btn_rescan, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_1, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_2, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_3, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_candidates_device_candidate_4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_candidates_device_candidate_4, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_btn_rescan, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_btn_rescan, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_icon_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_icon_no_devices, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_txt_no_devices, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_txt_no_devices, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_empty_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_icon_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_panel_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_ble_scanning_txt_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_ble_empty_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_ble_empty_btn_back, 56, 56);
            lv_obj_clear_flag(ui_ui_ble_empty_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_icon_back, 14, 15);
            lv_obj_set_size(ui_ui_ble_empty_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_ble_empty_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_ble_empty_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_txt_page_title, 104, 27);
            lv_obj_set_size(ui_ui_ble_empty_txt_page_title, 202, 27);
            lv_obj_clear_flag(ui_ui_ble_empty_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_empty_txt_page_title, "附近外骨骼");
            lv_obj_set_style_text_color(ui_ui_ble_empty_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_empty_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_empty_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_empty_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_ble_empty_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_ble_empty_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_empty_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_empty_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_empty_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_empty_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_scanning_icon_scanning, 145, 96);
            lv_obj_set_size(ui_ui_ble_scanning_icon_scanning, 120, 112);
            lv_obj_clear_flag(ui_ui_ble_scanning_icon_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_icon_scanning, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_scanning_icon_scanning, "");
            lv_obj_set_style_text_color(ui_ui_ble_scanning_icon_scanning, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_scanning_icon_scanning, &ui_font_lucide_120, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_scanning_txt_scanning, 46, 237);
            lv_obj_set_size(ui_ui_ble_scanning_txt_scanning, 318, 36);
            lv_obj_clear_flag(ui_ui_ble_scanning_txt_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_txt_scanning, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_scanning_txt_scanning, "正在搜索外骨骼");
            lv_obj_set_style_text_color(ui_ui_ble_scanning_txt_scanning, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_scanning_txt_scanning, &ui_font_ns700_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_scanning_txt_scanning, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_scanning_panel_scanning, 16, 330);
            lv_obj_set_size(ui_ui_ble_scanning_panel_scanning, 378, 96);
            lv_obj_clear_flag(ui_ui_ble_scanning_panel_scanning, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_panel_scanning, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_ble_scanning_panel_scanning, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_ble_scanning_panel_scanning, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_ble_scanning_panel_scanning, 32, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_ble_scanning_panel_scanning, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_ble_scanning_panel_scanning, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_scanning_scanning_icon, 22, 29);
            lv_obj_set_size(ui_ui_ble_scanning_scanning_icon, 40, 38);
            lv_obj_clear_flag(ui_ui_ble_scanning_scanning_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_scanning_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_scanning_scanning_icon, "");
            lv_obj_set_style_text_color(ui_ui_ble_scanning_scanning_icon, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_scanning_scanning_icon, &ui_font_lucide_40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_ble_scanning_scanning_label, 78, 31);
            lv_obj_set_size(ui_ui_ble_scanning_scanning_label, 220, 34);
            lv_obj_clear_flag(ui_ui_ble_scanning_scanning_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_ble_scanning_scanning_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_ble_scanning_scanning_label, "正在搜索…");
            lv_obj_set_style_text_color(ui_ui_ble_scanning_scanning_label, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_ble_scanning_scanning_label, &ui_font_ns700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_ble_scanning_scanning_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO:
            if (ui_ui_selftest_idle_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_rect_selftest_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_domains == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_scope == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_seconds == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_question_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_coordinate == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_action_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_countdown_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_question == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_countdown == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_icon_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_lbl_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_value == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_current_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_marker == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_percent == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_total == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_progress == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest_icon == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_idle_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_idle_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_idle_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_idle_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_idle_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_page_title, "单项重试");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_idle_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_icon_current_item, 44, 155);
            lv_obj_set_size(ui_ui_selftest_progress_icon_current_item, 36, 34);
            lv_obj_clear_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_icon_current_item, "");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_icon_current_item, lv_color_hex(0x64D2FF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_icon_current_item, &ui_font_lucide_36, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_lbl_current_item, 92, 149);
            lv_obj_set_size(ui_ui_selftest_progress_lbl_current_item, 200, 21);
            lv_obj_clear_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_lbl_current_item, "正在重试");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_lbl_current_item, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_lbl_current_item, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_lbl_current_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_val_current_item, 92, 181);
            lv_obj_set_size(ui_ui_selftest_progress_val_current_item, 286, 36);
            lv_obj_clear_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_val_current_item, "4G 网络");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_val_current_item, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_val_current_item, &ui_font_ns700_30, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_val_current_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest, 24, 382);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest, 362, 64);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_btn_leave_selftest, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_btn_leave_selftest, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_btn_leave_selftest, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_progress_btn_leave_selftest, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_progress_btn_leave_selftest, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest_icon, 20, 20);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest_icon, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest_icon, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_btn_leave_selftest_icon, "");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_btn_leave_selftest_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_btn_leave_selftest_icon, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest_label, 62, 19);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest_label, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_btn_leave_selftest_label, "返回维护");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_btn_leave_selftest_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_btn_leave_selftest_label, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_btn_leave_selftest_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_btn_leave_selftest_arrow, 330, 20);
            lv_obj_set_size(ui_ui_selftest_progress_btn_leave_selftest_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_progress_btn_leave_selftest_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_btn_leave_selftest_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_btn_leave_selftest_arrow, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_btn_leave_selftest_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_txt_progress_eyebrow, 44, 86);
            lv_obj_set_size(ui_ui_selftest_progress_txt_progress_eyebrow, 300, 22);
            lv_obj_clear_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_progress_txt_progress_eyebrow, "SINGLE RETRY · AUTO");
            lv_obj_set_style_text_color(ui_ui_selftest_progress_txt_progress_eyebrow, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_progress_txt_progress_eyebrow, &ui_font_mo700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_progress_txt_progress_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_rect_progress_current_divider, 44, 238);
            lv_obj_set_size(ui_ui_selftest_progress_rect_progress_current_divider, 334, 2);
            lv_obj_clear_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_rect_progress_current_divider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_rect_progress_current_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_rect_progress_current_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_progress_rect_progress_marker, 12, 238);
            lv_obj_set_size(ui_ui_selftest_progress_rect_progress_marker, 14, 4);
            lv_obj_clear_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_progress_rect_progress_marker, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_progress_rect_progress_marker, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_progress_rect_progress_marker, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_MANUAL_VIBRATION:
            if (ui_ui_selftest_idle_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_btn_start_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_icon_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_rect_selftest_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_count_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_domains == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_idle == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_idle_txt_selftest_scope == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_btn_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_seconds == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_question_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_rect_manual_status_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_coordinate == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_slider_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_touch_touch_target == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_action_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_countdown_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_txt_manual_question == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_countdown == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_val_manual_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_btn_leave_selftest == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_icon_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_lbl_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_progress_value == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_current_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_rect_progress_marker == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_percent == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_txt_progress_total == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_current_item == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_progress_val_progress == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_icon_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_icon_manual_pass == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_manual_lbl_manual_pass == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_idle_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_btn_start_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_btn_start_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_icon_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_icon_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_rect_selftest_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_rect_selftest_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_count_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_count_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_domains, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_domains, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_idle, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_idle, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_idle_txt_selftest_scope, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_selftest_scope, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_coordinate, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_coordinate, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_slider_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_touch_touch_target, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_btn_leave_selftest, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_btn_leave_selftest, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_icon_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_icon_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_lbl_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_lbl_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_progress_value, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_progress_value, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_current_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_current_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_rect_progress_marker, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_rect_progress_marker, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_percent, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_percent, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_txt_progress_total, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_txt_progress_total, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_current_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_current_item, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_progress_val_progress, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_progress_val_progress, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_idle_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_idle_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_idle_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_idle_txt_page_title, "人工确认");
            lv_obj_set_style_text_color(ui_ui_selftest_idle_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_idle_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_idle_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_idle_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_idle_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_idle_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_idle_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_idle_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_idle_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_idle_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_item, 44, 85);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_item, 260, 19);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_item, "人工确认 · 03 / 08");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_item, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_item, &ui_font_ns700_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_val_manual_item, 40, 114);
            lv_obj_set_size(ui_ui_selftest_manual_val_manual_item, 338, 51);
            lv_obj_clear_flag(ui_ui_selftest_manual_val_manual_item, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_item, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_val_manual_item, "振动反馈");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_val_manual_item, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_val_manual_item, &ui_font_ns700_42, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_val_manual_item, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_question, 40, 185);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_question, 338, 27);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_question, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_question, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_question, "是否感受到振动？");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_question, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_question, &ui_font_ns700_23, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_question, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_val_manual_countdown, 50, 244);
            lv_obj_set_size(ui_ui_selftest_manual_val_manual_countdown, 120, 93);
            lv_obj_clear_flag(ui_ui_selftest_manual_val_manual_countdown, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_val_manual_countdown, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_val_manual_countdown, "再");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_val_manual_countdown, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_val_manual_countdown, &ui_font_ns700_76, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_val_manual_countdown, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_seconds, 170, 258);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_seconds, 190, 72);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_seconds, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_seconds, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_seconds, "点击中部\n再次振动");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_seconds, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_seconds, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_seconds, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_btn_manual_fail, 24, 382);
            lv_obj_set_size(ui_ui_selftest_manual_btn_manual_fail, 142, 64);
            lv_obj_clear_flag(ui_ui_selftest_manual_btn_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_fail, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_btn_manual_fail, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_btn_manual_fail, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_btn_manual_fail, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_manual_btn_manual_fail, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_manual_btn_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_icon_manual_fail, 14, 20);
            lv_obj_set_size(ui_ui_selftest_manual_icon_manual_fail, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_manual_icon_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_icon_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_icon_manual_fail, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_icon_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_icon_manual_fail, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_fail, 44, 19);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_fail, 84, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_fail, "失败");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_fail, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_fail, &ui_font_ns700_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_fail, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_btn_manual_pass, 174, 382);
            lv_obj_set_size(ui_ui_selftest_manual_btn_manual_pass, 212, 64);
            lv_obj_clear_flag(ui_ui_selftest_manual_btn_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_btn_manual_pass, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_btn_manual_pass, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_btn_manual_pass, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_btn_manual_pass, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_icon_manual_pass, 28, 20);
            lv_obj_set_size(ui_ui_selftest_manual_icon_manual_pass, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_manual_icon_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_icon_manual_pass, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_icon_manual_pass, "");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_icon_manual_pass, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_icon_manual_pass, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_lbl_manual_pass, 62, 19);
            lv_obj_set_size(ui_ui_selftest_manual_lbl_manual_pass, 130, 26);
            lv_obj_clear_flag(ui_ui_selftest_manual_lbl_manual_pass, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_lbl_manual_pass, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_lbl_manual_pass, "通过");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_lbl_manual_pass, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_lbl_manual_pass, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_lbl_manual_pass, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_rect_manual_status_spine, 16, 88);
            lv_obj_set_size(ui_ui_selftest_manual_rect_manual_status_spine, 6, 252);
            lv_obj_clear_flag(ui_ui_selftest_manual_rect_manual_status_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_status_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_rect_manual_status_spine, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_rect_manual_status_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_rect_manual_status_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_rect_manual_question_divider, 40, 232);
            lv_obj_set_size(ui_ui_selftest_manual_rect_manual_question_divider, 338, 2);
            lv_obj_clear_flag(ui_ui_selftest_manual_rect_manual_question_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_rect_manual_question_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_manual_rect_manual_question_divider, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_manual_rect_manual_question_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_manual_rect_manual_question_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_countdown_label, 40, 238);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_countdown_label, 260, 22);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_countdown_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_countdown_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_countdown_label, "中心区域可重复触发振动");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_countdown_label, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_countdown_label, &ui_font_ns500_15, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_countdown_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_manual_txt_manual_action_hint, 238, 276);
            lv_obj_set_size(ui_ui_selftest_manual_txt_manual_action_hint, 140, 52);
            lv_obj_clear_flag(ui_ui_selftest_manual_txt_manual_action_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_manual_txt_manual_action_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_manual_txt_manual_action_hint, "点击中部\n再次播放");
            lv_obj_set_style_text_color(ui_ui_selftest_manual_txt_manual_action_hint, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_manual_txt_manual_action_hint, &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_manual_txt_manual_action_hint, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_RESULT_ALL_PASSED:
            if (ui_ui_selftest_result_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_btn_rerun_all == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_btn_retry_failed == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_pass_rule == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_skip_rule == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_list == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_rule == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_scroll_thumb == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_scroll_track == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_failure_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_failure_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_pass_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_pass_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_skip_count == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_skip_label == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_txt_result_summary == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_display == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_touch == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_audio == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_gps == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_01 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_02 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_03 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_04 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_power == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_sensor == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_cellular == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_result_ble == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL) == NULL)
            {
                return false;
            }
            if (ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE) == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_05 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_06 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_rect_result_separator_07 == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_icon_manual_fail == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_result_lbl_manual_fail == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_result_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_btn_rerun_all, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_rerun_all, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_btn_rerun_all, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_btn_retry_failed, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_btn_retry_failed, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_rect_result_pass_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_pass_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_pass_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_rect_result_skip_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_skip_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_skip_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_list, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_list, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_rule, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_rule, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_scroll_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_thumb, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_thumb, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_result_scroll_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_track, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_track, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_failure_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_failure_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_pass_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_pass_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_skip_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_count, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_count, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_skip_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_label, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_label, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_result_txt_result_summary, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_summary, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_summary, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_result_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_result_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_result_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_result_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_result_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_result_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_page_title, "自检结果");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_result_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_summary, 16, 78);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_summary, 180, 19);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_summary, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_summary, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_summary, "全部通过");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_summary, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_summary, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_summary, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_rule, 16, 184);
            lv_obj_set_size(ui_ui_selftest_result_result_rule, 46, 4);
            lv_obj_clear_flag(ui_ui_selftest_result_result_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_rule, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_rule, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_failure_count, 16, 96);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_failure_count, 112, 78);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_failure_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_failure_count, "00");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_failure_count, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_failure_count, &ui_font_mo700_68, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_failure_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_failure_label, 124, 124);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_failure_label, 112, 34);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_failure_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_failure_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_failure_label, "项失败");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_failure_label, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_failure_label, &ui_font_ns700_24, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_failure_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_pass_count, 252, 92);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_pass_count, 48, 34);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_pass_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_pass_count, "08");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_pass_count, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_pass_count, &ui_font_mo700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_pass_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_pass_label, 304, 98);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_pass_label, 72, 28);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_pass_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_pass_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_pass_label, "通过");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_pass_label, lv_color_hex(0xF5F7FA), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_pass_label, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_pass_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_skip_count, 252, 134);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_skip_count, 48, 34);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_skip_count, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_count, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_skip_count, "00");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_skip_count, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_skip_count, &ui_font_mo700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_skip_count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_txt_result_skip_label, 304, 140);
            lv_obj_set_size(ui_ui_selftest_result_txt_result_skip_label, 72, 28);
            lv_obj_clear_flag(ui_ui_selftest_result_txt_result_skip_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_txt_result_skip_label, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_txt_result_skip_label, "跳过");
            lv_obj_set_style_text_color(ui_ui_selftest_result_txt_result_skip_label, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_txt_result_skip_label, &ui_font_ns600_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_txt_result_skip_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_pass_rule, 64, 184);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_pass_rule, 282, 4);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_pass_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_pass_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_pass_rule, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_pass_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_pass_rule, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_skip_rule, 348, 184);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_skip_rule, 46, 4);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_skip_rule, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_skip_rule, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_skip_rule, lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_skip_rule, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_skip_rule, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_list, 16, 204);
            lv_obj_set_size(ui_ui_selftest_result_result_list, 378, 196);
            lv_obj_clear_flag(ui_ui_selftest_result_result_list, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_list, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_list, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_display, 0, 0);
            lv_obj_set_size(ui_ui_selftest_result_result_display, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_display, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_display, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_display, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_display, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_display, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "屏幕显示");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_display, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_touch, 0, 46);
            lv_obj_set_size(ui_ui_selftest_result_result_touch, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_touch, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_touch, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_touch, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_touch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_touch, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "触摸");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_touch, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_audio, 0, 184);
            lv_obj_set_size(ui_ui_selftest_result_result_audio, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_audio, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_audio, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_audio, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_audio, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_audio, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "音频资源");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_audio, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_gps, 0, 230);
            lv_obj_set_size(ui_ui_selftest_result_result_gps, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_gps, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_gps, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_gps, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_gps, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_gps, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "GPS 定位");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_gps, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_01, 38, 44);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_01, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_01, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_01, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_01, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_01, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_01, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_02, 38, 90);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_02, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_02, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_02, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_02, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_02, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_02, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_03, 38, 136);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_03, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_03, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_03, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_03, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_03, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_03, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_04, 38, 182);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_04, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_04, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_04, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_04, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_04, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_04, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_power, 0, 92);
            lv_obj_set_size(ui_ui_selftest_result_result_power, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_power, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_power, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_power, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_power, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "供电");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_power, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_sensor, 0, 138);
            lv_obj_set_size(ui_ui_selftest_result_result_sensor, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_sensor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_sensor, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_sensor, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_sensor, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_sensor, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "传感");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_sensor, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_cellular, 0, 276);
            lv_obj_set_size(ui_ui_selftest_result_result_cellular, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_cellular, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_cellular, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_cellular, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_cellular, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_cellular, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "4G 网络");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_cellular, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_ble, 0, 322);
            lv_obj_set_size(ui_ui_selftest_result_result_ble, 378, 44);
            lv_obj_clear_flag(ui_ui_selftest_result_result_ble, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_ble, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_ble, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_ble, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_ble, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 2, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), 22, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), "");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON), &ui_font_lucide_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 38, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), 220, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), "BLE");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), &ui_font_ns600_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_LABEL), LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 282, 11);
            lv_obj_set_size(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), 96, 22);
            lv_obj_clear_flag(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_STATE_DISABLED);
            lv_label_set_text(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), "通过");
            lv_obj_set_style_text_color(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), lv_color_hex(0x30D158), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), &ui_font_ns700_17, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_comp_get_child(ui_ui_selftest_result_result_ble, UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE), LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_05, 38, 228);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_05, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_05, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_05, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_05, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_05, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_05, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_06, 38, 274);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_06, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_06, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_06, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_06, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_06, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_06, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_rect_result_separator_07, 38, 320);
            lv_obj_set_size(ui_ui_selftest_result_rect_result_separator_07, 340, 2);
            lv_obj_clear_flag(ui_ui_selftest_result_rect_result_separator_07, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_rect_result_separator_07, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_rect_result_separator_07, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_rect_result_separator_07, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_rect_result_separator_07, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_scroll_track, 400, 216);
            lv_obj_set_size(ui_ui_selftest_result_result_scroll_track, 3, 168);
            lv_obj_clear_flag(ui_ui_selftest_result_result_scroll_track, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_track, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_scroll_track, lv_color_hex(0x24272D), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_scroll_track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_scroll_track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_result_scroll_thumb, 400, 216);
            lv_obj_set_size(ui_ui_selftest_result_result_scroll_thumb, 3, 72);
            lv_obj_clear_flag(ui_ui_selftest_result_result_scroll_thumb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_result_scroll_thumb, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_result_scroll_thumb, lv_color_hex(0x8B919A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_result_scroll_thumb, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_result_scroll_thumb, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_btn_rerun_all, 24, 390);
            lv_obj_set_size(ui_ui_selftest_result_btn_rerun_all, 362, 56);
            lv_obj_clear_flag(ui_ui_selftest_result_btn_rerun_all, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_btn_rerun_all, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_result_btn_rerun_all, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_result_btn_rerun_all, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_result_btn_rerun_all, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_result_btn_rerun_all, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_result_btn_rerun_all, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_icon_manual_fail, 54, 16);
            lv_obj_set_size(ui_ui_selftest_result_icon_manual_fail, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_result_icon_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_icon_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_icon_manual_fail, "");
            lv_obj_set_style_text_color(ui_ui_selftest_result_icon_manual_fail, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_icon_manual_fail, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_result_lbl_manual_fail, 90, 17);
            lv_obj_set_size(ui_ui_selftest_result_lbl_manual_fail, 230, 22);
            lv_obj_clear_flag(ui_ui_selftest_result_lbl_manual_fail, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_result_lbl_manual_fail, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_result_lbl_manual_fail, "重新整机自检");
            lv_obj_set_style_text_color(ui_ui_selftest_result_lbl_manual_fail, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_result_lbl_manual_fail, &ui_font_ns700_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_result_lbl_manual_fail, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_LOADING:
            if (ui_ui_selftest_gps_context_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_btn_gps_indoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_btn_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_context == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_rect_gps_context_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_submit_loading_btn_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_submit_loading_icon_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_gps_indoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_icon_gps_context, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_page_title, 159, 27);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_page_title, 92, 27);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_page_title, "定位环境");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_gps_context_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_rect_gps_context_spine, 16, 92);
            lv_obj_set_size(ui_ui_selftest_gps_context_rect_gps_context_spine, 6, 190);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_rect_gps_context_spine, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_rect_gps_context_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_rect_gps_context_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, 40, 82);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, 300, 24);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, "GPS CONTEXT · SUBMITTING");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, &ui_font_mo700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_context, 40, 117);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_context, 48, 46);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_context, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_context, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_context, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_context, &ui_font_lucide_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_title, 40, 177);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_title, 338, 43);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_title, "正在启动自检");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_title, &ui_font_ns700_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_hint, 40, 228);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_hint, 338, 54);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_hint, "环境选择已提交，请勿重复操作");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_hint, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_hint, &ui_font_ns500_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, 16, 294);
            lv_obj_set_size(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, 378, 72);
            lv_obj_clear_flag(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, lv_color_hex(0x16181C), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, 18, 24);
            lv_obj_set_size(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_submit_loading_icon_gps_outdoor, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, 58, 11);
            lv_obj_set_size(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, "正在启动");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, lv_color_hex(0xD7FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_submit_loading_lbl_gps_outdoor, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, 58, 42);
            lv_obj_set_size(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, 250, 19);
            lv_obj_clear_flag(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, "请稍候…");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_submit_loading_txt_gps_outdoor_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        case UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_FAILED:
            if (ui_ui_selftest_gps_context_btn_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_btn_gps_indoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_btn_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_context == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_rect_gps_context_spine == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_top_divider == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_eyebrow == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_context_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_page_title == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_submit_loading_btn_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_back == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_lbl_gps_outdoor == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_txt_gps_outdoor_hint == NULL)
            {
                return false;
            }
            if (ui_ui_selftest_gps_context_icon_gps_outdoor_arrow == NULL)
            {
                return false;
            }
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_gps_indoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_indoor, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_icon_gps_context, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_gps_context_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_context_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_CHECKED);
            lv_obj_add_flag(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_clear_state(ui_ui_selftest_gps_submit_loading_btn_gps_outdoor, LV_STATE_CHECKED);
            lv_obj_set_pos(ui_ui_selftest_gps_context_btn_back, 72, 16);
            lv_obj_set_size(ui_ui_selftest_gps_context_btn_back, 48, 48);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_btn_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_back, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_btn_back, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_btn_back, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_back, 10, 11);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_back, 28, 26);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_back, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_back, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_back, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_back, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_back, &ui_font_lucide_28, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_page_title, 100, 27);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_page_title, 210, 27);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_page_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_page_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_page_title, "定位环境");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_page_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_page_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_page_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_top_divider, 16, 66);
            lv_obj_set_size(ui_ui_selftest_gps_context_top_divider, 378, 2);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_top_divider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_top_divider, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_top_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_top_divider, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_top_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_rect_gps_context_spine, 16, 92);
            lv_obj_set_size(ui_ui_selftest_gps_context_rect_gps_context_spine, 6, 190);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_rect_gps_context_spine, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_rect_gps_context_spine, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_rect_gps_context_spine, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_rect_gps_context_spine, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, 40, 82);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, 300, 24);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, "GPS CONTEXT · FAILED");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, &ui_font_mo700_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_eyebrow, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_context, 40, 117);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_context, 48, 46);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_context, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_context, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_context, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_context, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_context, &ui_font_lucide_48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_title, 40, 177);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_title, 338, 43);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_title, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_title, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_title, "启动失败");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_title, &ui_font_ns700_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_context_hint, 40, 228);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_context_hint, 338, 54);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_context_hint, "环境选择未提交，请重新开始");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_context_hint, lv_color_hex(0x8E98A7), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_context_hint, &ui_font_ns500_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_context_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_btn_gps_outdoor, 16, 294);
            lv_obj_set_size(ui_ui_selftest_gps_context_btn_gps_outdoor, 378, 72);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_btn_gps_outdoor, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(ui_ui_selftest_gps_context_btn_gps_outdoor, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(ui_ui_selftest_gps_context_btn_gps_outdoor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(ui_ui_selftest_gps_context_btn_gps_outdoor, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(ui_ui_selftest_gps_context_btn_gps_outdoor, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(ui_ui_selftest_gps_context_btn_gps_outdoor, lv_color_hex(0xFF453A), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_outdoor, 18, 24);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_outdoor, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_outdoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_outdoor, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_outdoor, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_outdoor, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_lbl_gps_outdoor, 58, 11);
            lv_obj_set_size(ui_ui_selftest_gps_context_lbl_gps_outdoor, 250, 26);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_lbl_gps_outdoor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_lbl_gps_outdoor, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_lbl_gps_outdoor, "重新开始");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_lbl_gps_outdoor, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_lbl_gps_outdoor, &ui_font_ns700_21, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_lbl_gps_outdoor, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, 58, 42);
            lv_obj_set_size(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, 250, 19);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, "返回环境选择");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, lv_color_hex(0x2B1517), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(ui_ui_selftest_gps_context_txt_gps_outdoor_hint, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, 332, 24);
            lv_obj_set_size(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, 26, 25);
            lv_obj_clear_flag(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_state(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, LV_STATE_DISABLED);
            lv_label_set_text(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, "");
            lv_obj_set_style_text_color(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(ui_ui_selftest_gps_context_icon_gps_outdoor_arrow, &ui_font_lucide_26, LV_PART_MAIN | LV_STATE_DEFAULT);
            return true;
        default:
            return false;
    }
}

const char *ui_manifest_projection_key(ui_manifest_state_t state)
{
    static const char *const keys[UI_MANIFEST_STATE_COUNT] = {
        [UI_MANIFEST_STATE_HOME_NORMAL] = "home_normal",
        [UI_MANIFEST_STATE_HOME_UNBOUND] = "home_unbound",
        [UI_MANIFEST_STATE_HOME_DISCONNECTED] = "home_disconnected",
        [UI_MANIFEST_STATE_CONTROL_CONNECTED] = "control_connected",
        [UI_MANIFEST_STATE_CONTROL_DISCONNECTED] = "control_disconnected",
        [UI_MANIFEST_STATE_MODE_DEFAULT] = "mode_default",
        [UI_MANIFEST_STATE_MODE_DISCONNECTED] = "mode_disconnected",
        [UI_MANIFEST_STATE_ALERTS_EMPTY] = "alerts_empty",
        [UI_MANIFEST_STATE_ALERTS_READONLY] = "alerts_readonly",
        [UI_MANIFEST_STATE_PAYMENT_REQUIRED] = "payment_required",
        [UI_MANIFEST_STATE_PAYMENT_VERIFY_FAILED] = "payment_verify_failed",
        [UI_MANIFEST_STATE_SETTINGS] = "settings",
        [UI_MANIFEST_STATE_DEVICE_INFO] = "device_info",
        [UI_MANIFEST_STATE_MAINTENANCE] = "maintenance",
        [UI_MANIFEST_STATE_BLE_EMPTY] = "ble_empty",
        [UI_MANIFEST_STATE_BLE_CANDIDATES] = "ble_candidates",
        [UI_MANIFEST_STATE_SELFTEST_IDLE] = "selftest_idle",
        [UI_MANIFEST_STATE_SELFTEST_PROGRESS] = "selftest_progress",
        [UI_MANIFEST_STATE_SELFTEST_MANUAL] = "selftest_manual",
        [UI_MANIFEST_STATE_SELFTEST_RESULT] = "selftest_result",
        [UI_MANIFEST_STATE_SELFTEST_GPS_CONTEXT] = "selftest_gps_context",
        [UI_MANIFEST_STATE_SELFTEST_RETRY_DETAIL] = "selftest_retry_detail",
        [UI_MANIFEST_STATE_CLEAR_BINDING_CONFIRM] = "clear_binding_confirm",
        [UI_MANIFEST_STATE_SELFTEST_MANUAL_TOUCH] = "selftest_manual_touch",
        [UI_MANIFEST_STATE_BLE_SCANNING] = "ble_scanning",
        [UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO] = "selftest_retry_auto",
        [UI_MANIFEST_STATE_SELFTEST_MANUAL_VIBRATION] = "selftest_manual_vibration",
        [UI_MANIFEST_STATE_SELFTEST_RESULT_ALL_PASSED] = "selftest_result_all_passed",
        [UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_LOADING] = "selftest_gps_submit_loading",
        [UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_FAILED] = "selftest_gps_submit_failed",
    };
    return state >= 0 && state < UI_MANIFEST_STATE_COUNT ? keys[state] : "invalid";
}
