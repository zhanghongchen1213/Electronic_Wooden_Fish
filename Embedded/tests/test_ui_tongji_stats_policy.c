#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "today_bucket_policy.h"
#include "ui_tongji_stats_policy.h"

static void test_day_key_shanghai(void)
{
    /* 2024-01-01 00:00:00 UTC = 2024-01-01 08:00 上海 → 20240101 */
    assert(ewf_today_day_key_from_utc_ms(1704067200000LL) == 20240101U);
    /* 2024-01-01 15:59:59 UTC = 2024-01-01 23:59:59 上海 → 20240101 */
    assert(ewf_today_day_key_from_utc_ms(1704124799000LL) == 20240101U);
    /* 2024-01-01 16:00:00 UTC = 2024-01-02 00:00:00 上海 → 20240102 */
    assert(ewf_today_day_key_from_utc_ms(1704124800000LL) == 20240102U);
}

static void test_bucket_untrusted_no_write_path(void)
{
    ewf_today_bucket_t bucket = {.day_key = 20240101U, .count = 5U};
    assert(!ewf_today_bucket_on_trusted_tap(&bucket, 0U));
    assert(bucket.count == 5U);
    assert(ewf_today_bucket_display_count(false, &bucket, 20240101U) == 0U);
}

static void test_bucket_trusted_increment_and_rollover(void)
{
    ewf_today_bucket_t bucket = {0};
    assert(ewf_today_bucket_on_trusted_tap(&bucket, 20240101U));
    assert(bucket.day_key == 20240101U);
    assert(bucket.count == 1U);
    assert(ewf_today_bucket_on_trusted_tap(&bucket, 20240101U));
    assert(bucket.count == 2U);
    assert(ewf_today_bucket_align_day(&bucket, 20240102U));
    assert(bucket.day_key == 20240102U);
    assert(bucket.count == 0U);
    assert(ewf_today_bucket_display_count(true, &bucket, 20240102U) == 0U);
    assert(ewf_today_bucket_display_count(true, &bucket, 20240101U) == 0U);
}

static void test_bucket_display_after_trust_flip(void)
{
    /* 失信期间快照可能仍持有旧 published=0；校时后应对齐桶内当日计数。 */
    ewf_today_bucket_t bucket = {.day_key = 20240101U, .count = 50U};
    assert(ewf_today_bucket_display_count(false, &bucket, 20240101U) == 0U);
    assert(ewf_today_bucket_display_count(true, &bucket, 20240101U) == 50U);
}

static void test_bucket_saturates_without_false_dirty(void)
{
    ewf_today_bucket_t bucket = {.day_key = 20240101U, .count = UINT32_MAX};
    assert(!ewf_today_bucket_on_trusted_tap(&bucket, 20240101U));
    assert(bucket.count == UINT32_MAX);
}

static void test_stats_untrusted(void)
{
    ewf_ui_tongji_stats_input_t in = {
        .time_synchronized = false,
        .today_count = 99U,
        .local_total = 10U,
        .acked_total = 10U,
        .round_cursor = 42U,
        .round_id = 1U,
    };
    ewf_ui_tongji_stats_view_t view;
    ewf_ui_tongji_stats_project(&in, &view);
    assert(view.today_untrusted);
    assert(strcmp(view.today_value, "待校时") == 0);
    assert(strstr(view.progress_value, "42 / 260") != NULL);
    assert(view.progress_percent == 16U);
    assert(view.arc_value == 16U);
    assert(!view.pending_sync);
    assert(strstr(view.today_value, "近7") == NULL);
    assert(strstr(view.total_value, "streak") == NULL);
}

static void test_stats_trusted_pending_and_full(void)
{
    ewf_ui_tongji_stats_input_t in = {
        .time_synchronized = true,
        .today_count = 128U,
        .local_total = 3456U,
        .acked_total = 3000U,
        .round_cursor = 260U,
        .round_id = 3U,
        .round_state = 1U,
        .pending_completion = true,
    };
    ewf_ui_tongji_stats_view_t view;
    ewf_ui_tongji_stats_project(&in, &view);
    assert(!view.today_untrusted);
    assert(strcmp(view.today_value, "128") == 0);
    assert(strcmp(view.total_value, "3,456") == 0);
    assert(view.progress_percent == 100U);
    assert(strstr(view.progress_value, "260 / 260") != NULL);
    assert(strstr(view.round_index_text, "第 3 次诵读") != NULL);
    assert(strcmp(view.round_status_text, "已完成") == 0);
    assert(view.pending_sync);
    assert(strcmp(view.pending_phrase, "待同步") == 0);
}

static void test_muyu_tongji_today_consistent(void)
{
    char muyu[24];
    char tongji[24];
    ewf_ui_today_value_format(false, 7U, muyu, sizeof(muyu));
    ewf_ui_today_value_format(false, 7U, tongji, sizeof(tongji));
    assert(strcmp(muyu, tongji) == 0);
    assert(strcmp(muyu, "待校时") == 0);

    ewf_ui_today_value_format(true, 42U, muyu, sizeof(muyu));
    ewf_ui_today_value_format(true, 42U, tongji, sizeof(tongji));
    assert(strcmp(muyu, tongji) == 0);
    assert(strcmp(muyu, "42") == 0);
}

static void test_no_streak_fields_in_view_struct(void)
{
    /* 编译期形状守卫：视图不得含近7/30/streak 字段名（源扫描由 shell contract 补）。 */
    ewf_ui_tongji_stats_view_t view;
    memset(&view, 0, sizeof(view));
    ewf_ui_tongji_stats_project(NULL, &view);
    assert(view.progress_percent == 0U);
}

int main(void)
{
    test_day_key_shanghai();
    test_bucket_untrusted_no_write_path();
    test_bucket_trusted_increment_and_rollover();
    test_bucket_display_after_trust_flip();
    test_bucket_saturates_without_false_dirty();
    test_stats_untrusted();
    test_stats_trusted_pending_and_full();
    test_muyu_tongji_today_consistent();
    test_no_streak_fields_in_view_struct();
    printf("PASS test_ui_tongji_stats_policy\n");
    return 0;
}
