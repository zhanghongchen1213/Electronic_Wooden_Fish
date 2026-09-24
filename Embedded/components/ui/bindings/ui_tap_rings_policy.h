/**
 * @file     ui_tap_rings_policy.h
 * @brief    三环 flash 触发/抑制/重启合并纯逻辑（零 ESP-IDF）。
 * @details  裁决 D/E：仅 physical_pvdf/device_touch；单次 flash 重启 coalesce。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_TAP_RINGS_POLICY_H
#define EWF_UI_TAP_RINGS_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_muyu_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EWF_UI_TAP_ORIGIN_NONE = 0,
    EWF_UI_TAP_ORIGIN_PHYSICAL_PVDF,
    EWF_UI_TAP_ORIGIN_DEVICE_TOUCH,
    EWF_UI_TAP_ORIGIN_AUTOMATIC_TAP,
} ewf_ui_tap_origin_t;

typedef struct {
    bool pending_completion;
    bool fault_locked;
    bool screen_on;
    bool active_page_muyu;
} ewf_ui_tap_rings_gate_t;

typedef struct {
    bool flashing;
    uint32_t flash_deadline_ms;
} ewf_ui_tap_rings_state_t;

/**
 * @brief 判定本次有效敲击是否应触发三环 flash
 */
bool ewf_ui_tap_rings_should_flash(ewf_ui_tap_origin_t origin,
                                   const ewf_ui_tap_rings_gate_t *gate);

/**
 * @brief 事件到达：立即进入 flash，并设置/重置 160ms 截止时刻
 * @return true 表示应启动或重置定时器
 */
bool ewf_ui_tap_rings_on_event(ewf_ui_tap_rings_state_t *state,
                               uint32_t now_ms,
                               ewf_ui_tap_origin_t origin,
                               const ewf_ui_tap_rings_gate_t *gate);

/**
 * @brief 定时到期：若已过 deadline 则回 idle
 * @return true 表示刚从 flash 回到 idle
 */
bool ewf_ui_tap_rings_on_tick(ewf_ui_tap_rings_state_t *state, uint32_t now_ms);

/**
 * @brief 木鱼触区是否允许提交 device_touch
 */
bool ewf_ui_muyu_touch_may_submit(const ewf_ui_tap_rings_gate_t *gate);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_TAP_RINGS_POLICY_H */
