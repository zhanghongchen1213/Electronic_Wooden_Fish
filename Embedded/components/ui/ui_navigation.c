/**
 * @file     ui_navigation.c
 * @brief    最终腕表页面与跳转状态机实现
 * @details  根据当前事实解析主壳状态变体，并保证维护自检入口只指向设计中真实存在的页面。
 * @author   ZHC
 * @date     2026-07-15
 */

#include "ui_navigation.h"

#include <stddef.h>

/** 页面编号按 ui_page_id_t 顺序固定。 */
static const char *const s_design_ids[UI_PAGE_COUNT] = {
    "01-01", "01-02", "01-03", "02-01", "02-02", "03-01",
    "03-02", "04-01", "04-02", "04-03", "04-04", "05-01",
    "05-02", "06-01", "07-01", "07-02", "08-01", "08-02",
    "08-03", "08-04", "08-05", "08-06", "09-01",
};

ui_page_id_t ui_navigation_resolve_shell(
    ui_shell_slot_t slot,
    const ui_navigation_facts_t *facts)
{
    if (facts == NULL || slot < UI_SHELL_SLOT_HOME ||
        slot >= UI_SHELL_SLOT_COUNT)
    {
        return UI_PAGE_HOME_DISCONNECTED;
    }

    const bool physical_link_ready = facts->has_binding &&
                                     facts->ble_connected;
    switch (slot)
    {
    case UI_SHELL_SLOT_HOME:
        if (!facts->has_binding)
        {
            return UI_PAGE_HOME_UNBOUND;
        }
        return physical_link_ready ? UI_PAGE_HOME_READY
                                   : UI_PAGE_HOME_DISCONNECTED;
    case UI_SHELL_SLOT_GEAR:
        return physical_link_ready ? UI_PAGE_GEAR_READY
                                   : UI_PAGE_GEAR_UNAVAILABLE;
    case UI_SHELL_SLOT_MODE_POWER:
        return physical_link_ready ? UI_PAGE_MODE_POWER_READY
                                   : UI_PAGE_MODE_POWER_UNAVAILABLE;
    case UI_SHELL_SLOT_ALERTS:
        if (facts->payment_required)
        {
            return facts->payment_verification_attempted &&
                           facts->payment_verification_failed
                       ? UI_PAGE_ALERTS_VERIFY_FAILED
                       : UI_PAGE_ALERTS_PAYMENT_VERIFY;
        }
        return facts->reminder_count > 0U ? UI_PAGE_ALERTS_READ_ONLY
                                         : UI_PAGE_ALERTS_EMPTY;
    default:
        return UI_PAGE_HOME_DISCONNECTED;
    }
}

ui_page_id_t ui_navigation_resolve_selftest(ui_selftest_stage_t stage)
{
    switch (stage)
    {
    case UI_SELFTEST_STAGE_IDLE:
        return UI_PAGE_SELFTEST_IDLE;
    case UI_SELFTEST_STAGE_RUNNING:
        return UI_PAGE_SELFTEST_RUNNING;
    case UI_SELFTEST_STAGE_MANUAL:
        return UI_PAGE_SELFTEST_MANUAL;
    case UI_SELFTEST_STAGE_FINISHED:
        return UI_PAGE_SELFTEST_RESULT;
    default:
        return UI_PAGE_SELFTEST_IDLE;
    }
}

bool ui_navigation_is_shell_page(ui_page_id_t page)
{
    return page >= UI_PAGE_HOME_READY && page <= UI_PAGE_ALERTS_VERIFY_FAILED;
}

const char *ui_navigation_design_id(ui_page_id_t page)
{
    if (page < UI_PAGE_HOME_READY || page >= UI_PAGE_COUNT)
    {
        return "invalid";
    }
    return s_design_ids[page];
}
