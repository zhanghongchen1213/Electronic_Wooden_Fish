/**
 * @file     test_feedback_policy.c
 * @brief    统一反馈服务纯逻辑策略的主机回归。
 * @details  覆盖音量校验与持久化记录打包、限速合并决策（含 20 次/秒与 1 次/秒
 *           边界与回绕）、RGB 节流、故障降级状态机与故障日志限频；全部用例
 *           零 ESP-IDF 依赖，断言失败即非零退出。
 * @author   ZHC
 * @date     2026-09-23
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "feedback_policy.h"

static void test_volume_valid_boundaries(void)
{
    assert(ewf_feedback_volume_valid(0));
    assert(ewf_feedback_volume_valid(1));
    assert(ewf_feedback_volume_valid(50));
    assert(ewf_feedback_volume_valid(99));
    assert(ewf_feedback_volume_valid(100));
    assert(!ewf_feedback_volume_valid(101));
    assert(!ewf_feedback_volume_valid(255));
    printf("PASS volume_valid_boundaries\n");
}

static void test_volume_record_pack(void)
{
    ewf_feedback_volume_record_t record = {0};

    assert(ewf_feedback_volume_record_pack(0, &record) == EWF_FEEDBACK_VOLUME_OK);
    assert(record.schema_version == EWF_FEEDBACK_VOLUME_SCHEMA_VERSION);
    assert(record.volume == 0U);

    assert(ewf_feedback_volume_record_pack(100, &record) == EWF_FEEDBACK_VOLUME_OK);
    assert(record.volume == 100U);

    assert(ewf_feedback_volume_record_pack(101, &record) ==
           EWF_FEEDBACK_VOLUME_OUT_OF_RANGE);
    /* 越界拒绝后记录不被部分改写。 */
    assert(record.volume == 100U);
    assert(ewf_feedback_volume_record_pack(50, NULL) == EWF_FEEDBACK_VOLUME_NULL);
    printf("PASS volume_record_pack\n");
}

static void test_volume_record_validate(void)
{
    ewf_feedback_volume_record_t record = {
        .schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION,
        .volume = 77U,
    };
    assert(ewf_feedback_volume_record_validate(&record) == EWF_FEEDBACK_RECORD_OK);

    record.schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION + 1U;
    assert(ewf_feedback_volume_record_validate(&record) == EWF_FEEDBACK_RECORD_SCHEMA);

    record.schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION;
    record.volume = 201U;
    assert(ewf_feedback_volume_record_validate(&record) == EWF_FEEDBACK_RECORD_VOLUME);
    assert(ewf_feedback_volume_record_validate(NULL) == EWF_FEEDBACK_RECORD_NULL);
    printf("PASS volume_record_validate\n");
}

static void test_policy_init_zero(void)
{
    ewf_feedback_policy_state_t state = {0};
    state.audio_fault_active = true;
    ewf_feedback_policy_init(&state);
    assert(!state.has_audio && !state.has_rgb);
    assert(!state.audio_fault_active && !state.rgb_fault_active);
    assert(state.audio_fault_count == 0U && state.rgb_fault_count == 0U);
    printf("PASS policy_init_zero\n");
}

static void test_audio_decide_rhythm(void)
{
    ewf_feedback_policy_state_t state = {0};
    ewf_feedback_policy_init(&state);

    /* 首次事件立即播放；成功登记不触发故障日志（返回 false）。 */
    assert(ewf_feedback_audio_decide(&state, 0U) == EWF_FEEDBACK_AUDIO_PLAY);
    assert(!ewf_feedback_note_audio_result(&state, 0U, 0U, true));

    /* 1 秒 20 次（50 ms 间隔）：PLAY 决策推进基准，节奏合并为约每 100 ms 一声，
     * 不叠加成不可辨识噪声（AC 3），全部 20 次计数仍由 progress 保留。 */
    uint32_t play_count = 1U;
    for (uint32_t ms = 50U; ms <= 950U; ms += 50U)
    {
        if (ewf_feedback_audio_decide(&state, ms) == EWF_FEEDBACK_AUDIO_PLAY)
        {
            ++play_count;
            assert(!ewf_feedback_note_audio_result(&state, ms, ms, true));
        }
        else
        {
            assert(ms - state.last_audio_ms < EWF_FEEDBACK_AUDIO_MIN_INTERVAL_MS);
        }
    }
    /* 20 次/秒合并后约 10 次/秒输出：节奏可辨识且无堆叠。 */
    assert(play_count >= 9U && play_count <= 11U);

    /* 窗口边界：间隔恰为最小重触发间隔即恢复播放。 */
    state.has_audio = true;
    state.last_audio_ms = 80U;
    assert(ewf_feedback_audio_decide(&state, 159U) == EWF_FEEDBACK_AUDIO_MERGE_SKIP);
    assert(ewf_feedback_audio_decide(&state, 160U) == EWF_FEEDBACK_AUDIO_PLAY);

    /* 1 次/秒保持可辨识节奏。 */
    state.last_audio_ms = 1000U;
    assert(ewf_feedback_audio_decide(&state, 2000U) == EWF_FEEDBACK_AUDIO_PLAY);

    /* 单调时钟回绕：无符号差值语义。 */
    state.last_audio_ms = 0xFFFFFFFFU - 10U;
    assert(ewf_feedback_audio_decide(&state, 30U) == EWF_FEEDBACK_AUDIO_MERGE_SKIP);
    state.last_audio_ms = 0xFFFFFFFFU - 80U;
    assert(ewf_feedback_audio_decide(&state, 30U) == EWF_FEEDBACK_AUDIO_PLAY);
    printf("PASS audio_decide_rhythm\n");
}

static void test_rgb_decide_throttle(void)
{
    ewf_feedback_policy_state_t state = {0};
    ewf_feedback_policy_init(&state);

    /* 首次事件允许更新并登记成功（推进节流基准）。 */
    assert(ewf_feedback_rgb_decide(&state, 0U) == EWF_FEEDBACK_RGB_UPDATE);
    assert(!ewf_feedback_note_rgb_result(&state, 0U, 0U, true));

    /* 高速期间 RGB 低打扰节流：UPDATE 推进基准，输出合并为约每 100 ms 一次，
     * 不逐事件全亮度闪烁堆叠（AC 3）。 */
    uint32_t update_count = 1U;
    for (uint32_t ms = 50U; ms <= 950U; ms += 50U)
    {
        if (ewf_feedback_rgb_decide(&state, ms) == EWF_FEEDBACK_RGB_UPDATE)
        {
            ++update_count;
            assert(!ewf_feedback_note_rgb_result(&state, ms, ms, true));
        }
        else
        {
            assert(ms - state.last_rgb_ms < EWF_FEEDBACK_RGB_MIN_INTERVAL_MS);
        }
    }
    assert(update_count >= 9U && update_count <= 11U);
    printf("PASS rgb_decide_throttle\n");
}

static void test_audio_fault_degradation(void)
{
    ewf_feedback_policy_state_t state = {0};
    ewf_feedback_policy_init(&state);

    /* 成功输出推进节奏基准并保持正常态。 */
    assert(!ewf_feedback_note_audio_result(&state, 100U, 100U, true));
    assert(state.has_audio && state.last_audio_ms == 100U);
    assert(!state.audio_fault_active);

    /* 首次失败立即允许发布（命令型事实不得静默丢失）。 */
    assert(ewf_feedback_note_audio_result(&state, 110U, 110U, false));
    assert(state.audio_fault_active);
    assert(state.audio_fault_count == 1U);

    /* 限频窗口内连续失败静默（低打扰，不反复刷错）。 */
    assert(!ewf_feedback_note_audio_result(&state, 111U, 120U, false));
    assert(!ewf_feedback_note_audio_result(&state, 1100U, 130U, false));
    assert(state.audio_fault_count == 3U);

    /* 超过限频间隔允许再次发布。 */
    assert(ewf_feedback_note_audio_result(&state, 110U + 2000U, 140U, false));

    /* 播放成功后故障态自动清除，事实回到正常态并重置限频。 */
    assert(!ewf_feedback_note_audio_result(&state, 5000U, 5000U, true));
    assert(!state.audio_fault_active);
    assert(!state.has_audio_fault_log);

    /* 恢复后再次失败立即发布。 */
    assert(ewf_feedback_note_audio_result(&state, 5001U, 5001U, false));
    printf("PASS audio_fault_degradation\n");
}

static void test_rgb_fault_degradation(void)
{
    ewf_feedback_policy_state_t state = {0};
    ewf_feedback_policy_init(&state);

    assert(!ewf_feedback_note_rgb_result(&state, 200U, 200U, true));
    assert(state.has_rgb && state.last_rgb_ms == 200U);
    assert(ewf_feedback_note_rgb_result(&state, 210U, 210U, false));
    assert(state.rgb_fault_active);
    assert(state.rgb_fault_count == 1U);
    assert(!ewf_feedback_note_rgb_result(&state, 215U, 220U, false));
    assert(ewf_feedback_note_rgb_result(&state, 210U + 2000U, 230U, false));
    assert(!ewf_feedback_note_rgb_result(&state, 6000U, 6000U, true));
    assert(!state.rgb_fault_active);
    printf("PASS rgb_fault_degradation\n");
}

int main(void)
{
    test_volume_valid_boundaries();
    test_volume_record_pack();
    test_volume_record_validate();
    test_policy_init_zero();
    test_audio_decide_rhythm();
    test_rgb_decide_throttle();
    test_audio_fault_degradation();
    test_rgb_fault_degradation();
    printf("feedback-policy: 全部通过\n");
    return 0;
}
