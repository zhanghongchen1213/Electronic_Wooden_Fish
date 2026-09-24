/**
 * @file     today_bucket_policy.c
 * @brief    本地非权威今日桶日键/递增纯逻辑实现。
 * @details  日键换算用 civil_from_days（固定 UTC+8），不依赖 newlib TZ。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "today_bucket_policy.h"

#include <stddef.h>
static void civil_from_days(int64_t z, int *year, unsigned *month, unsigned *day)
{
    /*
     * Howard Hinnant civil_from_days：z = days since 1970-01-01。
     * 输出公历年月日；调用方再叠加上海日界偏移。
     */
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    int y = (int)yoe + (int)era * 400;
    const unsigned doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    const unsigned mp = (5U * doy + 2U) / 153U;
    unsigned d = doy - (153U * mp + 2U) / 5U + 1U;
    unsigned m = mp < 10U ? mp + 3U : mp - 9U;
    y += (m <= 2U) ? 1 : 0;
    *year = y;
    *month = m;
    *day = d;
}

uint32_t ewf_today_day_key_from_utc_ms(int64_t utc_ms)
{
    if (utc_ms < 0) {
        utc_ms = 0;
    }
    const int64_t local_ms = utc_ms + EWF_TODAY_SHANGHAI_OFFSET_MS;
    const int64_t days = local_ms / EWF_TODAY_DAY_MS;
    int year = 0;
    unsigned month = 0U;
    unsigned day = 0U;
    civil_from_days(days, &year, &month, &day);
    if (year < 1970 || year > 9999) {
        return 0U;
    }
    return ((uint32_t)year * 10000U) + (month * 100U) + day;
}

int64_t ewf_today_utc_ms_from_offset(int64_t monotonic_ms, int64_t time_offset_ms)
{
    return monotonic_ms + time_offset_ms;
}

bool ewf_today_bucket_align_day(ewf_today_bucket_t *bucket, uint32_t current_day_key)
{
    if (bucket == NULL || current_day_key == 0U) {
        return false;
    }
    if (bucket->day_key == current_day_key) {
        return false;
    }
    bucket->day_key = current_day_key;
    bucket->count = 0U;
    return true;
}

bool ewf_today_bucket_on_trusted_tap(ewf_today_bucket_t *bucket,
                                     uint32_t current_day_key)
{
    if (bucket == NULL || current_day_key == 0U) {
        return false;
    }
    bool changed = ewf_today_bucket_align_day(bucket, current_day_key);
    if (bucket->count < UINT32_MAX) {
        bucket->count += 1U;
        return true;
    }
    return changed;
}

uint32_t ewf_today_bucket_display_count(bool time_synchronized,
                                        const ewf_today_bucket_t *bucket,
                                        uint32_t current_day_key)
{
    if (!time_synchronized || bucket == NULL || current_day_key == 0U) {
        return 0U;
    }
    if (bucket->day_key != current_day_key) {
        return 0U;
    }
    return bucket->count;
}
