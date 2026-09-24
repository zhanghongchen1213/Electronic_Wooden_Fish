/**
 * @file     test_ui_muyu_done_policy.c
 * @brief    完成遮罩 policy 主机回归（Story 3.6）。
 * @author   ZHC
 * @date     2026-09-24
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ui_muyu_done_policy.h"

static void test_overlay_matrix(void)
{
    ewf_ui_muyu_done_input_t in = {0};
    ewf_ui_muyu_done_view_t out;

    ewf_ui_muyu_done_project(&in, &out);
    assert(!out.overlay_visible);

    in.pending_completion = true;
    in.round_cursor = 260U;
    ewf_ui_muyu_done_project(&in, &out);
    assert(out.overlay_visible);
    assert(out.restart_enabled);
    assert(out.exit_enabled);
    assert(strcmp(out.title, "本轮完成") == 0);
    assert(strcmp(out.summary, "心经进度 260 / 260 字") == 0);

    in.pending_completion = false;
    in.round_state = 1U;
    in.round_cursor = 260U;
    ewf_ui_muyu_done_project(&in, &out);
    assert(out.overlay_visible);
}

static void test_text_whitelist(void)
{
    assert(ewf_ui_muyu_done_text_allowed("本轮完成"));
    assert(ewf_ui_muyu_done_text_allowed("从头开始"));
    assert(ewf_ui_muyu_done_text_allowed("退出"));
    assert(ewf_ui_muyu_done_text_allowed("心经进度 0 / 260 字"));
    assert(ewf_ui_muyu_done_text_allowed("心经进度 260 / 260 字"));
    assert(!ewf_ui_muyu_done_text_allowed("TODO"));
    assert(!ewf_ui_muyu_done_text_allowed("draft"));
    assert(!ewf_ui_muyu_done_text_allowed("礼花"));
}

static void test_exit_keeps_overlay_under_decision_a(void)
{
    /* 裁决 A：overlay ≡ gate.completed；exit 清 pending 后仍靠 completed 显示遮罩。 */
    ewf_ui_muyu_done_input_t in = {
        .pending_completion = false,
        .round_state = 1U,
        .round_cursor = 260U,
    };
    ewf_ui_muyu_done_view_t out;
    ewf_ui_muyu_done_project(&in, &out);
    assert(out.overlay_visible);
    assert(out.restart_enabled);
    assert(out.exit_enabled);
}

int main(void)
{
    test_overlay_matrix();
    test_text_whitelist();
    test_exit_keeps_overlay_under_decision_a();
    printf("ui_muyu_done_policy: 全部通过\n");
    return 0;
}
