/**
 * @file     ui_shezhi_settings_policy.h
 * @brief    设置页档位/音量几何/sync 五态纯逻辑（零 ESP-IDF）。
 * @details  裁决 B/C/D/E/F：UI 只投影与转发；设置 owner=device_settings；
 *           立即同步飞行态 owner=device_nav；禁止自写 NVS / 第二 sync FSM；
 *           IDLE→BASE「立即同步」；禁止 medium wire 与非词表文案。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_SHEZHI_SETTINGS_POLICY_H
#define EWF_UI_SHEZHI_SETTINGS_POLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ui_shezhi_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/** sync-btn 组件态（对齐 Pen BASE/BUSY/OK/PENDING/FAIL）。 */
typedef enum {
    EWF_UI_SHEZHI_SYNC_BTN_BASE = 0, /**< IDLE：可点「立即同步」。 */
    EWF_UI_SHEZHI_SYNC_BTN_BUSY,     /**< 「同步中」。 */
    EWF_UI_SHEZHI_SYNC_BTN_OK,       /**< 「已同步」。 */
    EWF_UI_SHEZHI_SYNC_BTN_PENDING,  /**< 「待同步」。 */
    EWF_UI_SHEZHI_SYNC_BTN_FAIL,     /**< 「同步失败」。 */
    EWF_UI_SHEZHI_SYNC_BTN_COUNT
} ewf_ui_shezhi_sync_btn_state_t;

/** 设置页投影输入（只读事实）。 */
typedef struct {
    uint8_t volume;              /**< 0–100。 */
    uint8_t brightness;          /**< 0=low/1=mid/2=high（与 watch_brightness 对齐）。 */
    uint32_t timeout_s;          /**< 5|15|30。 */
    uint8_t sync_status;         /**< watch_sync_status / ewf_sync_status 同序。 */
    const char *firmware_version; /**< 可为 NULL。 */
    const char *device_id;        /**< 可为 NULL。 */
    bool identity_configured;     /**< false 时显示安全占位。 */
} ewf_ui_shezhi_settings_input_t;

/** sync-btn 投影输出。 */
typedef struct {
    ewf_ui_shezhi_sync_btn_state_t component_state;
    char action_text[16]; /**< 词表短文案。 */
    uint32_t action_color_hex;
} ewf_ui_shezhi_sync_btn_view_t;

/** 设置页完整投影视图。 */
typedef struct {
    int brightness_index; /**< 0/1/2。 */
    int timeout_index;    /**< 0=5 / 1=15 / 2=30。 */
    uint8_t volume;       /**< 钳制后 0–100。 */
    uint16_t volume_fill_w; /**< 轨填充宽度。 */
    int16_t volume_knob_x;  /**< knob 相对行左。 */
    char volume_text[8];
    ewf_ui_shezhi_sync_btn_view_t sync_btn;
    char firmware_text[EWF_UI_SHEZHI_FIRMWARE_TEXT_CAP];
    char device_id_text[EWF_UI_SHEZHI_DEVICE_ID_TEXT_CAP];
    bool identity_muted; /**< true=占位/未配置，用 muted 色。 */
    bool page1_row_count_ok; /**< 恒为 4，供合同断言。 */
} ewf_ui_shezhi_settings_view_t;

/**
 * @brief 由立即同步状态映射 sync-btn 五态
 * @param sync_status IDLE/PENDING/BUSY/OK/FAIL（与 WATCH_SYNC_STATUS_* 同序）
 * @param out 输出；NULL 无操作
 */
void ewf_ui_shezhi_sync_btn_from_status(uint8_t sync_status,
                                        ewf_ui_shezhi_sync_btn_view_t *out);

/**
 * @brief 亮度枚举 → 胶囊索引；非法回退 mid=1
 */
int ewf_ui_shezhi_brightness_index(uint8_t brightness);

/**
 * @brief 熄屏秒数 → 胶囊索引；非法回退 15 秒=1
 */
int ewf_ui_shezhi_timeout_index(uint32_t timeout_s);

/**
 * @brief 音量轨几何（填充宽与 knob X）
 */
void ewf_ui_shezhi_volume_geometry(uint8_t volume,
                                   uint16_t *fill_w,
                                   int16_t *knob_x);

/**
 * @brief 投影设置页完整视图
 */
void ewf_ui_shezhi_settings_project(const ewf_ui_shezhi_settings_input_t *in,
                                    ewf_ui_shezhi_settings_view_t *out);

/**
 * @brief 判断文案是否属于 sync-btn 允许词表
 */
bool ewf_ui_shezhi_sync_action_allowed(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_SHEZHI_SETTINGS_POLICY_H */
