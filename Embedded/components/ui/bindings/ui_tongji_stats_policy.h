/**
 * @file     ui_tongji_stats_policy.h
 * @brief    统计页今日/累计/进度/pending 纯逻辑（零 ESP-IDF）。
 * @details  裁决 A/C/D/E/F/G：不含七日/三十日/连续天数字段；累计投影 local_total；
 *           进度复用 ewf_ui_muyu_progress_percent；MUYU/TONGJI 今日同文案。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_TONGJI_STATS_POLICY_H
#define EWF_UI_TONGJI_STATS_POLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ui_tongji_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 统计页投影输入（只读事实，禁止 UI 自增）。 */
typedef struct {
    bool time_synchronized; /**< 可信时间门控。 */
    uint32_t today_count;   /**< 非权威今日桶展示值（失信时忽略）。 */
    uint32_t local_total;   /**< tap_local_total。 */
    uint32_t acked_total;   /**< tap_acked_total。 */
    uint32_t round_cursor;  /**< tap_round_cursor。 */
    uint32_t round_id;      /**< tap_round_id。 */
    uint32_t round_state;   /**< 0=in_progress，1=completed。 */
    bool pending_completion; /**< 末字完成锁定。 */
} ewf_ui_tongji_stats_input_t;

/** 统计页投影视图（禁止含七日/三十日/连续天数字段键）。 */
typedef struct {
    bool today_untrusted;           /**< true=今日区显「待校时」。 */
    char today_value[24];           /**< 今日大数或「待校时」。 */
    char total_value[24];           /**< 累计大数（可选千分位）。 */
    uint32_t progress_percent;      /**< 0–100。 */
    uint32_t arc_value;             /**< 环进度 0–100。 */
    char progress_value[32];        /**< 「{n} / 260 字」。 */
    char progress_percent_text[24]; /**< 「已完成 {p}%」。 */
    char round_label[16];           /**< 「本次诵读」。 */
    char round_index_text[32];      /**< 「第 N 次诵读」。 */
    char round_status_text[16];     /**< 「进行中」/「已完成」/空。 */
    bool pending_sync;              /**< local>acked 待同步标记。 */
    char pending_phrase[16];        /**< 「待同步」或「同步中」。 */
} ewf_ui_tongji_stats_view_t;

/**
 * @brief 格式化今日区文案（MUYU/TONGJI 共用）
 * @param time_synchronized 可信时间
 * @param today_count 今日桶数字
 * @param out 输出缓冲
 * @param out_len 容量
 */
void ewf_ui_today_value_format(bool time_synchronized,
                               uint32_t today_count,
                               char *out,
                               size_t out_len);

/**
 * @brief 由快照字段投影统计页三块文案与环进度
 * @param in 输入；NULL 无操作
 * @param out 输出；NULL 无操作
 */
void ewf_ui_tongji_stats_project(const ewf_ui_tongji_stats_input_t *in,
                                 ewf_ui_tongji_stats_view_t *out);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_TONGJI_STATS_POLICY_H */
