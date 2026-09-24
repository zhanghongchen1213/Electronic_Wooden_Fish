#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ui_jingwen_gesture_policy.h"
#include "ui_jingwen_stream_policy.h"

static void expect_slot(const ewf_ui_jingwen_stream_view_t *view,
                        uint32_t row,
                        uint32_t col,
                        const char *text)
{
    assert(row < view->row_count);
    assert(col < view->row_filled[row]);
    assert(strcmp(view->rows[row][col], text) == 0);
}

static void test_empty_stream(void)
{
    ewf_ui_jingwen_stream_view_t view;
    ewf_ui_jingwen_stream_project(0U, 0U, &view);
    assert(view.row_count == 0U);
    assert(view.display_char_count == 0U);
    assert(view.glyph_row == -1);
    assert(view.glyph_col == -1);
    assert(view.progress_percent == 0U);
    assert(strstr(view.progress_text, "0 / 260") != NULL);
    assert(view.round_index_text[0] == '\0');
}

static void test_one_char(void)
{
    ewf_ui_jingwen_stream_view_t view;
    ewf_ui_jingwen_stream_project(1U, 3U, &view);
    assert(view.display_char_count == 1U);
    assert(view.row_count == 1U);
    assert(view.row_filled[0] == 1U);
    expect_slot(&view, 0U, 0U, "观");
    assert(view.glyph_row == 0);
    assert(view.glyph_col == 0);
    assert(strstr(view.round_index_text, "第 3 次诵读") != NULL);
}

static void test_full_row_and_wrap(void)
{
    /* 前 13 个可消费字中含标点步，显示字符数可能 >13；取刚好填满一行的游标需探测。 */
    ewf_ui_jingwen_stream_view_t thirteen;
    uint32_t cursor_for_13 = 0U;
    for (uint32_t c = 1U; c <= 20U; ++c) {
        ewf_ui_jingwen_stream_project(c, 0U, &thirteen);
        if (thirteen.display_char_count == 13U) {
            cursor_for_13 = c;
            break;
        }
    }
    assert(cursor_for_13 > 0U);
    ewf_ui_jingwen_stream_project(cursor_for_13, 0U, &thirteen);
    assert(thirteen.row_count == 1U);
    assert(thirteen.row_filled[0] == 13U);
    assert(thirteen.glyph_row == 0);
    assert(thirteen.glyph_col == 12);

    ewf_ui_jingwen_stream_view_t fourteen;
    uint32_t cursor_for_14 = 0U;
    for (uint32_t c = cursor_for_13; c <= 30U; ++c) {
        ewf_ui_jingwen_stream_project(c, 0U, &fourteen);
        if (fourteen.display_char_count == 14U) {
            cursor_for_14 = c;
            break;
        }
    }
    assert(cursor_for_14 > cursor_for_13);
    ewf_ui_jingwen_stream_project(cursor_for_14, 0U, &fourteen);
    assert(fourteen.row_count == 2U);
    assert(fourteen.row_filled[0] == 13U);
    assert(fourteen.row_filled[1] == 1U);
    assert(fourteen.glyph_row == 1);
    assert(fourteen.glyph_col == 0);
    /* 前缀稳定：第 14 字换行后首行与满行时一致。 */
    for (uint32_t col = 0U; col < 13U; ++col) {
        assert(strcmp(thirteen.rows[0][col], fourteen.rows[0][col]) == 0);
    }
}

static void test_punctuation_two_slots(void)
{
    ewf_ui_jingwen_stream_view_t view;
    ewf_ui_jingwen_stream_project(5U, 0U, &view);
    assert(view.display_char_count == 6U);
    assert(view.row_count == 1U);
    expect_slot(&view, 0U, 4U, "萨");
    expect_slot(&view, 0U, 5U, "，");
    assert(view.glyph_row == 0);
    assert(view.glyph_col == 5);
}

static void test_prefix_stable_append(void)
{
    ewf_ui_jingwen_stream_view_t a;
    ewf_ui_jingwen_stream_view_t b;
    ewf_ui_jingwen_stream_project(10U, 0U, &a);
    ewf_ui_jingwen_stream_project(12U, 0U, &b);
    assert(b.display_char_count >= a.display_char_count);
    /* 前缀字节级稳定：较小游标的全部显示字符是较大游标的前缀。 */
    uint32_t idx = 0U;
    for (uint32_t row = 0U; row < a.row_count; ++row) {
        for (uint32_t col = 0U; col < a.row_filled[row]; ++col) {
            uint32_t brow = idx / 13U;
            uint32_t bcol = idx % 13U;
            assert(strcmp(a.rows[row][col], b.rows[brow][bcol]) == 0);
            ++idx;
        }
    }
}

static void test_full_cursor_no_future(void)
{
    ewf_ui_jingwen_stream_view_t view;
    ewf_ui_jingwen_stream_project(260U, 1U, &view);
    assert(view.display_char_count == 303U);
    assert(view.progress_percent == 100U);
    assert(strstr(view.progress_text, "260 / 260") != NULL);
    assert(view.glyph_row >= 0);
    assert(view.glyph_col >= 0);
    /* 行宽恒为 13（末行可不满）。 */
    for (uint32_t row = 0U; row + 1U < view.row_count; ++row) {
        assert(view.row_filled[row] == 13U);
    }
    assert(view.row_filled[view.row_count - 1U] <= 13U);
    /* 满流：ceil(303/13)=24 行。 */
    assert(view.row_count == 24U);
}

static void test_anchor_policy(void)
{
    assert(ewf_ui_jingwen_should_anchor_tail(true, true));
    assert(ewf_ui_jingwen_should_anchor_tail(false, true));
    assert(!ewf_ui_jingwen_should_anchor_tail(true, false));
    assert(!ewf_ui_jingwen_should_anchor_tail(false, false));
}

static void test_gesture_history_blocks_settings(void)
{
    /* history 内下拖：抑制垂直导航。 */
    assert(ewf_ui_jingwen_suppress_vertical_nav(true, false, 100, 200, true));
    /* history 外（标题带）下拖：不抑制 → 可开设置。 */
    assert(!ewf_ui_jingwen_suppress_vertical_nav(true, false, 100, 70, true));
    /* 非经文页：不抑制。 */
    assert(!ewf_ui_jingwen_suppress_vertical_nav(false, false, 100, 200, true));
    /* 水平滑动：不抑制。 */
    assert(!ewf_ui_jingwen_suppress_vertical_nav(true, false, 100, 200, false));
    /* 设置已打开：不抑制（允许上滑关闭）。 */
    assert(!ewf_ui_jingwen_suppress_vertical_nav(true, true, 100, 200, true));
    assert(ewf_ui_jingwen_point_in_history(24, 102));
    assert(!ewf_ui_jingwen_point_in_history(23, 102));
}

static void test_stable_same_cursor(void)
{
    ewf_ui_jingwen_stream_view_t a;
    ewf_ui_jingwen_stream_view_t b;
    ewf_ui_jingwen_stream_project(42U, 2U, &a);
    ewf_ui_jingwen_stream_project(42U, 2U, &b);
    assert(memcmp(&a, &b, sizeof(a)) == 0);
}

int main(void)
{
    test_empty_stream();
    test_one_char();
    test_full_row_and_wrap();
    test_punctuation_two_slots();
    test_prefix_stable_append();
    test_full_cursor_no_future();
    test_anchor_policy();
    test_gesture_history_blocks_settings();
    test_stable_same_cursor();
    puts("PASS test_ui_jingwen_stream_policy");
    return 0;
}
