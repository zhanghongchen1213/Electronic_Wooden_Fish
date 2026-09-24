/**
 * @file     ui_muyu_done_policy.h
 * @brief    木鱼完成遮罩 modal-done 纯逻辑（零 ESP-IDF）。
 * @details  裁决 A/G：overlay_visible 与 gate.completed 对齐
 *           （pending_completion || round_state==completed）；
 *           禁止顶层 OVERLAY Screen；无礼花；文案白名单闭包。
 *           UI 只投影 + 转发 intent；禁止自写 NVS。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_MUYU_DONE_POLICY_H
#define EWF_UI_MUYU_DONE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_muyu_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 完成遮罩投影输入（只读 gate/round 事实）。 */
typedef struct {
    bool pending_completion; /**< 本地末字 pending。 */
    uint32_t round_state;    /**< 0=in_progress / 1=completed。 */
    uint32_t round_cursor;   /**< 当前游标（摘要分母固定 260）。 */
} ewf_ui_muyu_done_input_t;

/** 完成遮罩投影视图。 */
typedef struct {
    bool overlay_visible; /**< 与完成锁定 gate 对齐。 */
    char title[EWF_UI_MUYU_DONE_TITLE_CAP];
    char summary[EWF_UI_MUYU_DONE_SUMMARY_CAP];
    bool restart_enabled;
    bool exit_enabled;
} ewf_ui_muyu_done_view_t;

/**
 * @brief 是否处于完成锁定（与 progress fill_gate.completed 同语义）
 */
bool ewf_ui_muyu_done_is_locked(const ewf_ui_muyu_done_input_t *in);

/**
 * @brief 投影 modal-done 视图
 */
void ewf_ui_muyu_done_project(const ewf_ui_muyu_done_input_t *in,
                              ewf_ui_muyu_done_view_t *out);

/**
 * @brief 判断文案是否属于完成遮罩允许词表（标题/按钮/摘要前缀）
 */
bool ewf_ui_muyu_done_text_allowed(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_MUYU_DONE_POLICY_H */
