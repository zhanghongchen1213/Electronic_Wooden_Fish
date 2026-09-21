/**
 * @file     ui_display_flush_guard.c
 * @brief    实现 UI 显示颜色事务代次与超时守卫
 * @details  以颜色完成代次而非共享任务通知作为完成事实，并对单调等待时间执行有界判断。
 * @author   ZHC
 * @date     2026-07-28
 */

#include "ui_display_flush_guard.h"

#include <stddef.h>

void ui_display_flush_guard_init(ui_display_flush_guard_t *guard)
{
    if (guard == NULL)
    {
        return;
    }
    *guard = (ui_display_flush_guard_t){0};
}

void ui_display_flush_guard_begin(ui_display_flush_guard_t *guard,
                                  uint32_t submitted_generation,
                                  int64_t started_us)
{
    if (guard == NULL)
    {
        return;
    }
    guard->submitted_generation = submitted_generation;
    guard->started_us = started_us;
    guard->active = true;
}

bool ui_display_flush_guard_completed(
    const ui_display_flush_guard_t *guard,
    uint32_t current_generation)
{
    return guard != NULL && guard->active &&
           current_generation != guard->submitted_generation;
}

bool ui_display_flush_guard_timed_out(
    const ui_display_flush_guard_t *guard,
    int64_t now_us,
    uint32_t timeout_us)
{
    return guard != NULL && guard->active && timeout_us > 0U &&
           now_us >= guard->started_us &&
           (uint64_t)(now_us - guard->started_us) >= timeout_us;
}

void ui_display_flush_guard_rearm_timeout(
    ui_display_flush_guard_t *guard,
    int64_t now_us)
{
    if (guard == NULL || !guard->active)
    {
        return;
    }
    guard->started_us = now_us;
}

void ui_display_flush_guard_clear(ui_display_flush_guard_t *guard)
{
    ui_display_flush_guard_init(guard);
}
