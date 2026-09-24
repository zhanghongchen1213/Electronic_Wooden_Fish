/**
 * @file     ui_shell_policy.c
 * @brief    DEVICE-01 壳层纯逻辑。
 * @author   ZHC
 * @date     2026-09-24
 */
#include "ui_shell_policy.h"

int ewf_ui_shell_page_offset_x(ewf_ui_shell_page_t page)
{
    switch (page) {
    case EWF_UI_SHELL_PAGE_JINGWEN:
        return 410;
    case EWF_UI_SHELL_PAGE_TONGJI:
        return 820;
    case EWF_UI_SHELL_PAGE_MUYU:
    default:
        return 0;
    }
}

bool ewf_ui_shell_settings_is_ondemand(bool settings_open, int pager_page_count)
{
    /* 设置不是第四循环页；pager 只承载三主页。 */
    (void)settings_open;
    return pager_page_count == EWF_UI_SHELL_PAGE_COUNT;
}

bool ewf_ui_shell_wifi_ble_gps_default_disabled(void)
{
    return true;
}
