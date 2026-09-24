/**
 * @file     test_feedback_runtime.c
 * @brief    统一反馈服务的主机集成回归。
 * @details  编译实际生产 C 源码（feedback_service、feedback_policy、state_service、
 *           watch_state），只替代播放链/RGB/持久化介质与 RTOS 边界（沿
 *           test_progress_runtime 的接线测试教训：只测纯逻辑发现不了运行时
 *           链路缺口）。验证事件消费接线、限速合并、静音、故障降级与事实发布、
 *           音量恢复序列与输入锁定不置位。
 * @author   ZHC
 * @date     2026-09-23
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "audio_volume_store.h"
#include "event_bus.h"
#include "feedback_policy.h"
#include "feedback_service.h"
#include "state_service.h"
#include "tap_input_service.h"

void host_platform_init(void);
void host_state_owner_start(void);
esp_err_t host_state_apply_one(void);
void host_feedback_audio_reset(void);
void host_feedback_audio_fail_codec(bool fail);
void host_feedback_audio_fail_pa(bool fail);
unsigned host_feedback_audio_play_count(void);
uint8_t host_feedback_audio_played_volume(unsigned index);
bool host_feedback_audio_pa_off_called(void);
void host_rgb_reset(void);
void host_rgb_fail(bool fail);
unsigned host_rgb_flash_count(void);
unsigned host_rgb_off_count(void);
void host_audio_store_reset(void);
void host_audio_store_fail_save(bool fail);
void host_audio_store_fail_load_media(bool fail);
uint32_t host_audio_store_save_count(void);
bool host_audio_store_has_stored(void);
const ewf_feedback_volume_record_t *host_audio_store_stored(void);
void host_audio_store_preload(uint8_t volume);
void host_nav_settings_reset(void);
void host_feedback_set_persist_status(bool pending, bool inflight);

static uint32_t s_event_sequence;

static void drain_state_updates(void)
{
    /* 每轮事件至多产生一条反馈事实更新，全部应用到不可变快照。 */
    while (host_state_apply_one() == ESP_OK)
    {
    }
}

static watch_feedback_update_t feedback_snapshot(void)
{
    watch_feedback_update_t snapshot = {0};
    assert(watch_state_feedback_snapshot(&snapshot, 0) == ESP_OK);
    return snapshot;
}

static void reset_service(void)
{
    host_platform_init();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_feedback_audio_reset();
    host_rgb_reset();
    host_audio_store_reset();
    host_nav_settings_reset();
    host_feedback_set_persist_status(false, false);
    s_event_sequence = 0U;
    assert(feedback_service_init_contracts() == ESP_OK);
    assert(feedback_service_prepare_run() == ESP_OK);
    drain_state_updates();
}

static void publish_valid_event(void)
{
    const legbot_event_t event = {
        .source = LEGBOT_EVENT_SOURCE_STATE,
        .type = LEGBOT_EVENT_TYPE_SIGNAL,
        .code = EWF_TAP_EVENT_VALID,
        .value = ++s_event_sequence,
    };
    assert(event_bus_publish(&event, 0) == ESP_OK);
}

static void pump_service(void)
{
    /* 服务消费至队列排空后停止请求生效退出（主机 stub 非阻塞）。 */
    assert(feedback_service_request_stop(0) == ESP_OK);
    assert(feedback_service_run() == ESP_OK);
    drain_state_updates();
}

static void ensure_prepared(void)
{
    /* run() 退出后服务框架语义上已回收任务；主机逐轮重建 prepared 状态。 */
    assert(feedback_service_init_contracts() == ESP_OK);
    assert(feedback_service_prepare_run() == ESP_OK);
    drain_state_updates();
}

static void tap_once(void)
{
    ensure_prepared();
    publish_valid_event();
    pump_service();
}

static bool read_fault_locked(void)
{
    bool ready = false;
    bool completed = false;
    bool fault_locked = false;
    bool queue_full = false;
    assert(state_service_read_tap_gate(&ready, &completed, &fault_locked,
                                       &queue_full) == ESP_OK);
    return fault_locked;
}

static void test_first_boot_default_volume(void)
{
    reset_service();
    const watch_feedback_update_t snapshot = feedback_snapshot();
    /* 首启缺字段按默认 50 初始化并落盘。 */
    assert(snapshot.volume == EWF_FEEDBACK_VOLUME_DEFAULT);
    assert(host_audio_store_has_stored());
    assert(host_audio_store_stored()->volume == EWF_FEEDBACK_VOLUME_DEFAULT);
    assert(host_audio_store_stored()->schema_version ==
           EWF_FEEDBACK_VOLUME_SCHEMA_VERSION);
    assert(!snapshot.volume_persist_error);
    printf("PASS first_boot_default_volume\n");
}

static void test_valid_tap_drives_feedback(void)
{
    reset_service();
    tap_once();
    /* 有效敲击获得与同一事件关联的音频与 RGB 反馈。 */
    assert(host_feedback_audio_play_count() == 1U);
    assert(host_feedback_audio_played_volume(0U) == EWF_FEEDBACK_VOLUME_DEFAULT);
    assert(host_rgb_flash_count() == 1U);
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.last_event_sequence == 1U);
    assert(snapshot.feedback_count == 1U);
    assert(snapshot.merged_count == 0U);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_RGB].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    /* 事件序号进入 owner 单调快照，消费者只读不可变事实。 */
    assert(snapshot.update_sequence >= 1U);
    printf("PASS valid_tap_drives_feedback\n");
}

static void test_high_rate_merge_rhythm(void)
{
    reset_service();
    ensure_prepared();
    /* 1 秒 20 次高速连击：首个事件播放，其余合并成既有节奏（AD-12）。
     * 主机扇出为瞬时全量投递：反馈队列深度 8，事件 9-20 按 AD-7 可合并型
     * "队满丢最新"丢弃；真实固件消费远快于 50 ms 到达间隔，不会触顶。 */
    for (int index = 0; index < 20; ++index)
    {
        publish_valid_event();
    }
    pump_service();
    /* 全部 20 次输入只产出一次可辨识输出：不叠加成不可辨识噪声（AC 3）。 */
    assert(host_feedback_audio_play_count() == 1U);
    assert(host_rgb_flash_count() == 1U);
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.feedback_count == 1U);
    /* 消费到的 8 个事件中 7 个落在合并窗口内（间隔 < 80 ms 最小重触发间隔）。 */
    assert(snapshot.merged_count == 7U);
    assert(snapshot.last_event_sequence == 8U);
    printf("PASS high_rate_merge_rhythm\n");
}

static void test_volume_zero_mutes(void)
{
    reset_service();
    assert(feedback_service_set_volume(0U, 0) == ESP_OK);
    assert(host_audio_store_stored()->volume == 0U);
    tap_once();
    /* 音量 0 等价静音：播放链跳过输出，功放保持安全关断。 */
    assert(host_feedback_audio_play_count() == 0U);
    assert(host_feedback_audio_pa_off_called());
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.volume == 0U);
    assert(snapshot.feedback_count == 1U);
    printf("PASS volume_zero_mutes\n");
}

static void test_play_failure_degrades(void)
{
    reset_service();
    host_feedback_audio_fail_codec(true);
    tap_once();
    /* 播放失败降级：安全关断功放并跳过本次音频，故障事实按 typed 发布。 */
    assert(host_feedback_audio_pa_off_called());
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state ==
           WATCH_FEEDBACK_CHANNEL_FAILED);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].error ==
           WATCH_FEEDBACK_ERROR_CODEC_FAILED);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_RGB].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    /* 反馈故障绝不置位输入锁定（生产者属 Story 2.7）。 */
    assert(!read_fault_locked());

    /* 故障恢复后（下次成功播放）事实自动回到正常态。 */
    host_feedback_audio_fail_codec(false);
    tap_once();
    const watch_feedback_update_t recovered = feedback_snapshot();
    assert(recovered.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    assert(recovered.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].error ==
           WATCH_FEEDBACK_ERROR_NONE);
    assert(!read_fault_locked());
    printf("PASS play_failure_degrades\n");
}

static void test_pa_failure_typed(void)
{
    reset_service();
    host_feedback_audio_fail_pa(true);
    tap_once();
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state ==
           WATCH_FEEDBACK_CHANNEL_FAILED);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].error ==
           WATCH_FEEDBACK_ERROR_PA_FAILED);
    assert(host_feedback_audio_pa_off_called());
    printf("PASS pa_failure_typed\n");
}

static void test_rgb_failure_degrades(void)
{
    reset_service();
    host_rgb_fail(true);
    tap_once();
    /* RGB 失败降级：熄灭并跳过本次灯效，音频与核心链路不受影响。 */
    assert(host_rgb_off_count() >= 1U);
    assert(host_feedback_audio_play_count() == 1U);
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_RGB].state ==
           WATCH_FEEDBACK_CHANNEL_FAILED);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_RGB].error ==
           WATCH_FEEDBACK_ERROR_RGB_FAILED);
    assert(!read_fault_locked());

    host_rgb_fail(false);
    tap_once();
    const watch_feedback_update_t recovered = feedback_snapshot();
    assert(recovered.channels[WATCH_FEEDBACK_CHANNEL_RGB].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    printf("PASS rgb_failure_degrades\n");
}

static void test_volume_persist_and_reboot(void)
{
    reset_service();
    assert(feedback_service_set_volume(80U, 0) == ESP_OK);
    assert(host_audio_store_save_count() >= 2U); /* 首启初始写 + 本次设置。 */

    /* 越界写入被拒绝并保持原值。 */
    assert(feedback_service_set_volume(101U, 0) == ESP_ERR_INVALID_ARG);
    assert(host_audio_store_stored()->volume == 80U);
    /* 同值 no-op：不重复落盘。 */
    const uint32_t saves_before = host_audio_store_save_count();
    assert(feedback_service_set_volume(80U, 0) == ESP_OK);
    assert(host_audio_store_save_count() == saves_before);

    /* 模拟重启：重建 owner 内存并从持久化介质恢复。 */
    assert(feedback_service_prepare_run() == ESP_OK);
    drain_state_updates();
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.volume == 80U);
    printf("PASS volume_persist_and_reboot\n");
}

static void test_volume_save_failure_keeps_memory(void)
{
    reset_service();
    host_audio_store_fail_save(true);
    /* 先持久化成功再提交内存：介质错误时保持原内存值（fail-closed）。 */
    assert(feedback_service_set_volume(30U, 0) == ESP_FAIL);
    drain_state_updates();
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.volume == EWF_FEEDBACK_VOLUME_DEFAULT);
    assert(snapshot.volume_persist_error);
    host_audio_store_fail_save(false);
    assert(feedback_service_set_volume(30U, 0) == ESP_OK);
    drain_state_updates();
    const watch_feedback_update_t recovered = feedback_snapshot();
    assert(recovered.volume == 30U);
    assert(!recovered.volume_persist_error);
    printf("PASS volume_save_failure_keeps_memory\n");
}

static void test_volume_media_error_fail_closed(void)
{
    host_platform_init();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_feedback_audio_reset();
    host_rgb_reset();
    host_audio_store_reset();
    host_nav_settings_reset();
    host_audio_store_fail_load_media(true);
    assert(feedback_service_init_contracts() == ESP_OK);
    assert(feedback_service_prepare_run() == ESP_OK);
    drain_state_updates();
    /* 介质错误 fail-closed：报错 + 保持内存默认值 + 发布持久化失败事实。 */
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.volume == EWF_FEEDBACK_VOLUME_DEFAULT);
    assert(snapshot.volume_persist_error);
    printf("PASS volume_media_error_fail_closed\n");
}

static void test_sources_get_identical_feedback(void)
{
    reset_service();
    /* NFR8 设备侧体现：事件流只携带统一序号（三类来源在 tap_input_service 归一），
     * 反馈服务完全不读来源字段——连续三个有效敲击事件获得仅由节奏策略区分的
     * 一致反馈通道行为，没有任何来源差异化分支。 */
    ensure_prepared();
    for (int index = 0; index < 3; ++index)
    {
        publish_valid_event();
    }
    pump_service();
    /* 三事件落在同一合并窗口：一次输出、两次合并，通道保持正常态。 */
    assert(host_feedback_audio_play_count() == 1U);
    assert(host_rgb_flash_count() == 1U);
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.feedback_count == 1U);
    assert(snapshot.merged_count == 2U);
    assert(snapshot.last_event_sequence == 3U);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_RGB].state ==
           WATCH_FEEDBACK_CHANNEL_OK);
    printf("PASS sources_get_identical_feedback\n");
}

static void test_progress_path_not_blocked_by_feedback_failure(void)
{
    reset_service();
    /* 播放链与 RGB 全部失败时，反馈服务仍完成事件消费并发布事实，
     * 不阻塞 progress_service 的持久化路径（反馈服务不接触 progress 任何状态）。 */
    host_feedback_audio_fail_codec(true);
    host_rgb_fail(true);
    tap_once();
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state ==
           WATCH_FEEDBACK_CHANNEL_FAILED);
    assert(snapshot.channels[WATCH_FEEDBACK_CHANNEL_RGB].state ==
           WATCH_FEEDBACK_CHANNEL_FAILED);
    assert(snapshot.last_event_sequence == 1U);
    assert(!read_fault_locked());
    printf("PASS progress_path_not_blocked_by_feedback_failure\n");
}

static void test_persist_busy_defers_feedback_without_rollback(void)
{
    reset_service();
    host_feedback_set_persist_status(true, false);
    tap_once();
    /* 落盘冲突：跳过音频/RGB，事件仍消费并发布事实，绝不回滚计数合同。 */
    assert(host_feedback_audio_play_count() == 0U);
    assert(host_rgb_flash_count() == 0U);
    const watch_feedback_update_t snapshot = feedback_snapshot();
    assert(snapshot.last_event_sequence == 1U);
    assert(snapshot.merged_count >= 1U);
    assert(!read_fault_locked());
    printf("PASS persist_busy_defers_feedback_without_rollback\n");
}

int main(void)
{
    test_first_boot_default_volume();
    test_valid_tap_drives_feedback();
    test_high_rate_merge_rhythm();
    test_volume_zero_mutes();
    test_play_failure_degrades();
    test_pa_failure_typed();
    test_rgb_failure_degrades();
    test_volume_persist_and_reboot();
    test_volume_save_failure_keeps_memory();
    test_volume_media_error_fail_closed();
    test_sources_get_identical_feedback();
    test_progress_path_not_blocked_by_feedback_failure();
    test_persist_busy_defers_feedback_without_rollback();
    printf("feedback-runtime: 全部通过\n");
    return 0;
}
