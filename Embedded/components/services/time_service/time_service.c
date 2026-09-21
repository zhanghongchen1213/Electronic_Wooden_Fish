/**
 * @file     time_service.c
 * @brief    启动周期内统一 UTC 时间服务实现
 * @details  使用短临界原子写入和序列锁读取维护 CLOUD > GPS 的单调时间偏移。
 * @author   ZHC
 * @date     2026-07-31
 */

#include "time_service.h"

#include <limits.h>
#include <stdatomic.h>

#ifndef TIME_SERVICE_TEST
#include "freertos/FreeRTOS.h"
#endif

#ifdef TIME_SERVICE_TEST
/** 主机测试写入锁仅保护四个原子字段的一致发布。 */
static atomic_flag s_write_lock = ATOMIC_FLAG_INIT;
#else
/** ESP32-S3 双核短临界区，防止高优先级任务自旋造成优先级反转。 */
static portMUX_TYPE s_write_lock = portMUX_INITIALIZER_UNLOCKED;
#endif
/** 偶数表示稳定快照，奇数表示写入进行中。 */
static atomic_uint s_sequence;
/** 当前来源。 */
static atomic_int s_source;
/** 当前 UTC 与单调时间偏移。 */
static atomic_llong s_utc_offset_ms;
/** 最近接受样本的单调毫秒。 */
static atomic_ullong s_sample_monotonic_ms;

static void lock_writer(void);
static void unlock_writer(void);
static bool source_is_valid(time_service_source_t source);

void time_service_reset(void)
{
    lock_writer();
    (void)atomic_fetch_add_explicit(&s_sequence, 1U, memory_order_acq_rel);
    atomic_store_explicit(&s_source, TIME_SOURCE_NONE, memory_order_relaxed);
    atomic_store_explicit(&s_utc_offset_ms, 0, memory_order_relaxed);
    atomic_store_explicit(&s_sample_monotonic_ms, 0U, memory_order_relaxed);
    (void)atomic_fetch_add_explicit(&s_sequence, 1U, memory_order_release);
    unlock_writer();
}

esp_err_t time_service_submit_sample(time_service_source_t source,
                                     int64_t utc_ms,
                                     uint64_t monotonic_ms)
{
    if (!source_is_valid(source) || utc_ms < 0 ||
        utc_ms > TIME_SERVICE_UTC_MAX_MS ||
        monotonic_ms > (uint64_t)INT64_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const int64_t offset_ms = utc_ms - (int64_t)monotonic_ms;

    lock_writer();
    const time_service_source_t current_source =
        (time_service_source_t)atomic_load_explicit(&s_source,
                                                    memory_order_relaxed);
    const uint64_t current_sample =
        atomic_load_explicit(&s_sample_monotonic_ms, memory_order_relaxed);
    if (current_source > source ||
        (current_source == source && current_sample > monotonic_ms))
    {
        unlock_writer();
        return ESP_ERR_INVALID_STATE;
    }

    (void)atomic_fetch_add_explicit(&s_sequence, 1U, memory_order_acq_rel);
    atomic_store_explicit(&s_source, source, memory_order_relaxed);
    atomic_store_explicit(&s_utc_offset_ms, offset_ms, memory_order_relaxed);
    atomic_store_explicit(&s_sample_monotonic_ms,
                          monotonic_ms,
                          memory_order_relaxed);
    (void)atomic_fetch_add_explicit(&s_sequence, 1U, memory_order_release);
    unlock_writer();
    return ESP_OK;
}

esp_err_t time_service_get_snapshot(time_service_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (;;)
    {
        const unsigned before =
            atomic_load_explicit(&s_sequence, memory_order_acquire);
        if ((before & 1U) != 0U)
        {
            continue;
        }
        const time_service_source_t source =
            (time_service_source_t)atomic_load_explicit(&s_source,
                                                        memory_order_relaxed);
        const int64_t offset =
            atomic_load_explicit(&s_utc_offset_ms, memory_order_relaxed);
        const uint64_t sample =
            atomic_load_explicit(&s_sample_monotonic_ms, memory_order_relaxed);
        const unsigned after =
            atomic_load_explicit(&s_sequence, memory_order_acquire);
        if (before == after)
        {
            snapshot->valid = source != TIME_SOURCE_NONE;
            snapshot->source = source;
            snapshot->utc_offset_ms = offset;
            snapshot->sample_monotonic_ms = sample;
            return ESP_OK;
        }
    }
}

esp_err_t time_service_now(uint64_t monotonic_ms,
                           int64_t *utc_ms,
                           time_service_source_t *source)
{
    if (utc_ms == NULL || monotonic_ms > (uint64_t)INT64_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    time_service_snapshot_t snapshot = {0};
    const esp_err_t error = time_service_get_snapshot(&snapshot);
    if (error != ESP_OK)
    {
        return error;
    }
    if (!snapshot.valid)
    {
        *utc_ms = 0;
        if (source != NULL)
        {
            *source = TIME_SOURCE_NONE;
        }
        return ESP_ERR_NOT_FOUND;
    }
    const int64_t monotonic_signed = (int64_t)monotonic_ms;
    if ((snapshot.utc_offset_ms > 0 &&
         monotonic_signed > INT64_MAX - snapshot.utc_offset_ms) ||
        (snapshot.utc_offset_ms < 0 &&
         monotonic_signed < INT64_MIN - snapshot.utc_offset_ms))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    *utc_ms = monotonic_signed + snapshot.utc_offset_ms;
    if (*utc_ms < 0 || *utc_ms > TIME_SERVICE_UTC_MAX_MS)
    {
        return ESP_ERR_INVALID_SIZE;
    }
    if (source != NULL)
    {
        *source = snapshot.source;
    }
    return ESP_OK;
}

static void lock_writer(void)
{
#ifdef TIME_SERVICE_TEST
    while (atomic_flag_test_and_set_explicit(&s_write_lock,
                                             memory_order_acquire))
    {
    }
#else
    taskENTER_CRITICAL(&s_write_lock);
#endif
}

static void unlock_writer(void)
{
#ifdef TIME_SERVICE_TEST
    atomic_flag_clear_explicit(&s_write_lock, memory_order_release);
#else
    taskEXIT_CRITICAL(&s_write_lock);
#endif
}

static bool source_is_valid(time_service_source_t source)
{
    return source == TIME_SOURCE_GPS || source == TIME_SOURCE_CLOUD;
}
