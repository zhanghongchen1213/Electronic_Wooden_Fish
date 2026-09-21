/**
 * @file     ui_selftest_summary.c
 * @brief    十一项内部自检到七类 UI 结果的聚合实现
 * @details  失败优先于未完成，未完成优先于跳过，跳过优先于通过；不修改底层单项结果与重试身份。
 * @author   ZHC
 * @date     2026-07-24
 */

#include "ui_selftest_summary.h"

#include <string.h>

/** 每个内部项所属的最终 UI 类别。 */
static const ui_selftest_category_t s_item_categories[SELFTEST_ITEM_COUNT] = {
    [SELFTEST_ITEM_AMOLED] = UI_SELFTEST_CATEGORY_DISPLAY,
    [SELFTEST_ITEM_TOUCH] = UI_SELFTEST_CATEGORY_TOUCH,
    [SELFTEST_ITEM_I2C_SCAN] = UI_SELFTEST_CATEGORY_POWER,
    [SELFTEST_ITEM_CW2015] = UI_SELFTEST_CATEGORY_POWER,
    [SELFTEST_ITEM_QMI8658C] = UI_SELFTEST_CATEGORY_SENSOR,
    [SELFTEST_ITEM_AUDIO] = UI_SELFTEST_CATEGORY_AUDIO,
    [SELFTEST_ITEM_GPS_UART] = UI_SELFTEST_CATEGORY_GPS,
    [SELFTEST_ITEM_GPS_FIX] = UI_SELFTEST_CATEGORY_GPS,
    [SELFTEST_ITEM_ML307R] = UI_SELFTEST_CATEGORY_CELLULAR,
    [SELFTEST_ITEM_VIBRATION] = UI_SELFTEST_CATEGORY_SENSOR,
    [SELFTEST_ITEM_SPIFFS] = UI_SELFTEST_CATEGORY_AUDIO,
};

static uint8_t outcome_priority(selftest_outcome_t outcome);
static void merge_item(ui_selftest_category_result_t *category,
                       const selftest_item_result_t *item);

bool ui_selftest_summary_build(const watch_selftest_summary_t *source,
                               ui_selftest_summary_t *summary)
{
    if (source == NULL || summary == NULL)
    {
        return false;
    }
    memset(summary, 0, sizeof(*summary));
    for (size_t index = 0; index < UI_SELFTEST_CATEGORY_COUNT; ++index)
    {
        summary->categories[index].outcome = SELFTEST_OUTCOME_PASS;
    }

    for (size_t index = 0; index < SELFTEST_ITEM_COUNT; ++index)
    {
        const selftest_item_result_t *item = &source->selftest_results[index];
        if (item->item_id != (selftest_item_id_t)index ||
            item->outcome < SELFTEST_OUTCOME_NOT_RUN ||
            item->outcome > SELFTEST_OUTCOME_INCONCLUSIVE)
        {
            return false;
        }
        merge_item(&summary->categories[s_item_categories[index]], item);
    }

    for (size_t index = 0; index < UI_SELFTEST_CATEGORY_COUNT; ++index)
    {
        switch (summary->categories[index].outcome)
        {
        case SELFTEST_OUTCOME_PASS:
            ++summary->pass_count;
            ++summary->completed_count;
            break;
        case SELFTEST_OUTCOME_FAIL:
            ++summary->fail_count;
            ++summary->completed_count;
            break;
        case SELFTEST_OUTCOME_SKIP:
            ++summary->skip_count;
            ++summary->completed_count;
            break;
        case SELFTEST_OUTCOME_INCONCLUSIVE:
            ++summary->inconclusive_count;
            ++summary->completed_count;
            break;
        case SELFTEST_OUTCOME_NOT_RUN:
        default:
            break;
        }
    }
    return true;
}

const char *ui_selftest_category_name(ui_selftest_category_t category)
{
    static const char *const names[UI_SELFTEST_CATEGORY_COUNT] = {
        [UI_SELFTEST_CATEGORY_DISPLAY] = "屏幕显示",
        [UI_SELFTEST_CATEGORY_TOUCH] = "触摸",
        [UI_SELFTEST_CATEGORY_POWER] = "供电",
        [UI_SELFTEST_CATEGORY_SENSOR] = "传感",
        [UI_SELFTEST_CATEGORY_AUDIO] = "音频资源",
        [UI_SELFTEST_CATEGORY_GPS] = "GPS 定位",
        [UI_SELFTEST_CATEGORY_CELLULAR] = "4G 网络",
    };
    return category >= UI_SELFTEST_CATEGORY_DISPLAY &&
                   category < UI_SELFTEST_CATEGORY_COUNT
               ? names[category]
               : "未知";
}

ui_selftest_category_t ui_selftest_category_for_item(
    selftest_item_id_t item)
{
    return item >= SELFTEST_ITEM_AMOLED && item < SELFTEST_ITEM_COUNT
               ? s_item_categories[item]
               : UI_SELFTEST_CATEGORY_COUNT;
}

static uint8_t outcome_priority(selftest_outcome_t outcome)
{
    switch (outcome)
    {
    case SELFTEST_OUTCOME_FAIL:
        return 5U;
    case SELFTEST_OUTCOME_INCONCLUSIVE:
        return 4U;
    case SELFTEST_OUTCOME_NOT_RUN:
        return 3U;
    case SELFTEST_OUTCOME_SKIP:
        return 2U;
    case SELFTEST_OUTCOME_PASS:
        return 1U;
    default:
        return 0U;
    }
}

static void merge_item(ui_selftest_category_result_t *category,
                       const selftest_item_result_t *item)
{
    category->elapsed_ms += item->elapsed_ms;
    if (outcome_priority(item->outcome) >=
        outcome_priority(category->outcome))
    {
        category->outcome = item->outcome;
        category->reason = item->reason;
        category->source_item_id = item->item_id;
        (void)strncpy(category->detail_code, item->detail_code,
                      sizeof(category->detail_code) - 1U);
        category->detail_code[sizeof(category->detail_code) - 1U] = '\0';
    }
}
