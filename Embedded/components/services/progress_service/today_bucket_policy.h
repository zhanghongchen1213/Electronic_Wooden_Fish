/**
 * @file     today_bucket_policy.h
 * @brief    本地非权威「今日」桶纯逻辑（零 ESP-IDF）。
 * @details  裁决 B：可信时间后按 Asia/Shanghai 日键维护展示计数；敲击 +1；
 *           跨日清零；不存每敲绝对时间；未校时不写入。UI 只投影，禁止自增。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_TODAY_BUCKET_POLICY_H
#define EWF_TODAY_BUCKET_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Asia/Shanghai 相对 UTC 固定偏移（无夏令时）。 */
#define EWF_TODAY_SHANGHAI_OFFSET_MS (8LL * 60LL * 60LL * 1000LL)

/** 一日毫秒数。 */
#define EWF_TODAY_DAY_MS (24LL * 60LL * 60LL * 1000LL)

/** 今日桶内存态（日键 + 计数）。 */
typedef struct {
    uint32_t day_key; /**< YYYYMMDD（上海日界）。 */
    uint32_t count;   /**< 当日非权威敲击展示计数。 */
} ewf_today_bucket_t;

/**
 * @brief 由 UTC 毫秒计算 Asia/Shanghai 日键 YYYYMMDD
 * @param utc_ms Unix 纪元起 UTC 毫秒；负值按 0 处理
 * @return 日键；utc_ms 不可用时返回 0
 */
uint32_t ewf_today_day_key_from_utc_ms(int64_t utc_ms);

/**
 * @brief 由单调毫秒 + 可信时间偏移合成 UTC 毫秒
 * @param monotonic_ms 启动后单调毫秒（int64，避免 uint32 ~49 天溢出）
 * @param time_offset_ms 快照 time_offset_ms（UTC = monotonic + offset）
 */
int64_t ewf_today_utc_ms_from_offset(int64_t monotonic_ms, int64_t time_offset_ms);

/**
 * @brief 对齐日键：跨日则清零再挂到 current_day_key
 * @param bucket 桶；NULL 无操作
 * @param current_day_key 当前上海日键（0 表示不可用，不改桶）
 * @return true=发生了跨日清零或首次绑定日键
 */
bool ewf_today_bucket_align_day(ewf_today_bucket_t *bucket, uint32_t current_day_key);

/**
 * @brief 可信时间下接受一次有效敲击：对齐日键后 count+1
 * @param bucket 桶；NULL 返回 false
 * @param current_day_key 当前上海日键；0 时不写入
 * @return true=桶已变更应持久化；false=未写入
 */
bool ewf_today_bucket_on_trusted_tap(ewf_today_bucket_t *bucket,
                                     uint32_t current_day_key);

/**
 * @brief 失信后展示裁决：不得用陈旧日数字冒充今日
 * @param time_synchronized 是否可信
 * @param bucket 已加载桶（可为未对齐）
 * @param current_day_key 当前日键（失信时忽略）
 * @return 可投影的今日计数；失信或日键不匹配返回 0
 */
uint32_t ewf_today_bucket_display_count(bool time_synchronized,
                                        const ewf_today_bucket_t *bucket,
                                        uint32_t current_day_key);

#ifdef __cplusplus
}
#endif

#endif /* EWF_TODAY_BUCKET_POLICY_H */
