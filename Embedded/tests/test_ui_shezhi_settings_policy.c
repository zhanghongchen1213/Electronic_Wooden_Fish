#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ui_shezhi_settings_policy.h"

static void test_defaults_mid_15_50(void)
{
    ewf_ui_shezhi_settings_view_t view;
    ewf_ui_shezhi_settings_project(NULL, &view);
    assert(view.brightness_index == 1);
    assert(view.timeout_index == 1);
    assert(view.volume == 50U);
    assert(strcmp(view.volume_text, "50") == 0);
    assert(view.page1_row_count_ok);
    assert(view.sync_btn.component_state == EWF_UI_SHEZHI_SYNC_BTN_BASE);
    assert(strcmp(view.sync_btn.action_text, "立即同步") == 0);
}

static void test_brightness_and_timeout_indices(void)
{
    assert(ewf_ui_shezhi_brightness_index(0) == 0);
    assert(ewf_ui_shezhi_brightness_index(1) == 1);
    assert(ewf_ui_shezhi_brightness_index(2) == 2);
    assert(ewf_ui_shezhi_brightness_index(9) == 1);
    assert(ewf_ui_shezhi_timeout_index(5) == 0);
    assert(ewf_ui_shezhi_timeout_index(15) == 1);
    assert(ewf_ui_shezhi_timeout_index(30) == 2);
    assert(ewf_ui_shezhi_timeout_index(99) == 1);
}

static void test_volume_geometry_and_clamp(void)
{
    uint16_t fill = 0;
    int16_t knob = 0;
    ewf_ui_shezhi_volume_geometry(0, &fill, &knob);
    assert(fill == 0);
    ewf_ui_shezhi_volume_geometry(100, &fill, &knob);
    assert(fill == EWF_UI_SHEZHI_VOLUME_TRACK_W);

    ewf_ui_shezhi_settings_input_t in = {
        .volume = 200U,
        .brightness = 0,
        .timeout_s = 5U,
        .sync_status = 0,
    };
    ewf_ui_shezhi_settings_view_t view;
    ewf_ui_shezhi_settings_project(&in, &view);
    assert(view.volume == 100U);
    assert(strcmp(view.volume_text, "100") == 0);

    in.volume = 0;
    ewf_ui_shezhi_settings_project(&in, &view);
    assert(view.volume == 0U);
    assert(strcmp(view.volume_text, "0") == 0);
}

static void test_sync_btn_five_states(void)
{
    static const struct {
        uint8_t status;
        ewf_ui_shezhi_sync_btn_state_t state;
        const char *text;
    } cases[] = {
        {EWF_UI_SHEZHI_SYNC_STATUS_IDLE, EWF_UI_SHEZHI_SYNC_BTN_BASE, "立即同步"},
        {EWF_UI_SHEZHI_SYNC_STATUS_PENDING, EWF_UI_SHEZHI_SYNC_BTN_PENDING, "待同步"},
        {EWF_UI_SHEZHI_SYNC_STATUS_BUSY, EWF_UI_SHEZHI_SYNC_BTN_BUSY, "同步中"},
        {EWF_UI_SHEZHI_SYNC_STATUS_OK, EWF_UI_SHEZHI_SYNC_BTN_OK, "已同步"},
        {EWF_UI_SHEZHI_SYNC_STATUS_FAIL, EWF_UI_SHEZHI_SYNC_BTN_FAIL, "同步失败"},
        {99, EWF_UI_SHEZHI_SYNC_BTN_BASE, "立即同步"},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        ewf_ui_shezhi_sync_btn_view_t btn;
        ewf_ui_shezhi_sync_btn_from_status(cases[i].status, &btn);
        assert(btn.component_state == cases[i].state);
        assert(strcmp(btn.action_text, cases[i].text) == 0);
        assert(ewf_ui_shezhi_sync_action_allowed(btn.action_text));
    }
    assert(!ewf_ui_shezhi_sync_action_allowed("成功"));
    assert(!ewf_ui_shezhi_sync_action_allowed("失败了"));
    assert(!ewf_ui_shezhi_sync_action_allowed("medium"));
}

static void test_identity_safe_placeholder(void)
{
    ewf_ui_shezhi_settings_input_t in = {
        .volume = 50,
        .brightness = 1,
        .timeout_s = 15,
        .sync_status = 0,
        .firmware_version = "ewf-unconfigured",
        .device_id = "0.0.0-dev",
        .identity_configured = false,
    };
    ewf_ui_shezhi_settings_view_t view;
    ewf_ui_shezhi_settings_project(&in, &view);
    assert(view.identity_muted);
    assert(strcmp(view.firmware_text, EWF_UI_SHEZHI_IDENTITY_PLACEHOLDER) == 0);

    in.identity_configured = true;
    in.firmware_version = "1.2.3";
    in.device_id = "ewf-device-001";
    ewf_ui_shezhi_settings_project(&in, &view);
    assert(!view.identity_muted);
    assert(strcmp(view.firmware_text, "1.2.3") == 0);
    assert(strcmp(view.device_id_text, "ewf-device-001") == 0);
}

static void test_no_medium_in_policy_outputs(void)
{
    ewf_ui_shezhi_settings_input_t in = {
        .volume = 50,
        .brightness = 1,
        .timeout_s = 15,
        .sync_status = 0,
        .identity_configured = true,
        .firmware_version = "1.0.0",
        .device_id = "id-1",
    };
    ewf_ui_shezhi_settings_view_t view;
    ewf_ui_shezhi_settings_project(&in, &view);
    assert(strstr(view.sync_btn.action_text, "medium") == NULL);
    assert(strstr(view.firmware_text, "medium") == NULL);
    assert(strstr(view.device_id_text, "medium") == NULL);
}

int main(void)
{
    test_defaults_mid_15_50();
    test_brightness_and_timeout_indices();
    test_volume_geometry_and_clamp();
    test_sync_btn_five_states();
    test_identity_safe_placeholder();
    test_no_medium_in_policy_outputs();
    printf("PASS test_ui_shezhi_settings_policy\n");
    return 0;
}
