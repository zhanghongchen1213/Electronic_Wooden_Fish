#include <assert.h>
#include <stdio.h>
#include "ui_shell_policy.h"

int main(void)
{
    assert(EWF_UI_SHELL_CANVAS_W == 410);
    assert(EWF_UI_SHELL_CANVAS_H == 502);
    assert(EWF_UI_SHELL_CORNER_RADIUS == 110);
    assert(EWF_UI_SHELL_MARGIN_PX == 16);
    assert(EWF_UI_SHELL_STATUSBAR_H == 24);
    assert(EWF_UI_SHELL_PAGER_CONTENT_W == 1230);
    assert(ewf_ui_shell_page_offset_x(EWF_UI_SHELL_PAGE_MUYU) == 0);
    assert(ewf_ui_shell_page_offset_x(EWF_UI_SHELL_PAGE_JINGWEN) == 410);
    assert(ewf_ui_shell_page_offset_x(EWF_UI_SHELL_PAGE_TONGJI) == 820);
    assert(ewf_ui_shell_settings_is_ondemand(true, 3));
    assert(ewf_ui_shell_settings_is_ondemand(false, 3));
    assert(!ewf_ui_shell_settings_is_ondemand(true, 4));
    assert(ewf_ui_shell_wifi_ble_gps_default_disabled());
    puts("PASS test_ui_shell_policy");
    return 0;
}
