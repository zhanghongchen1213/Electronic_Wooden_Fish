/**
 * @file     ui_service.c
 * @brief    单一 UI 所有权服务实现
 * @details  在 ui_task 中独占 LVGL display、input、SquareLine 对象树、状态 binding、请求队列与 timer handler。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "ui_service.h"

#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

#include "audio_service.h"
#include "ble_service.h"
#include "bsp_resources.h"
#include "co5300_bsp.h"
#include "config_service.h"
#include "control_ui_binding.h"
#include "cst9217_bsp.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "lvgl.h"
#include "maintenance_service.h"
#include "power_service.h"
#include "sdkconfig.h"
#include "selftest_service.h"
#include "ui_display_flush_guard.h"
#include "ui_payment_correlation.h"
#include "ui_runtime_binding.h"
#include "ui_performance_metrics.h"
#include "ui_selftest_summary.h"
#include "ui_touch_session.h"

#if CONFIG_LV_COLOR_DEPTH != 16
#error "ui_service requires LVGL RGB565 (CONFIG_LV_COLOR_DEPTH=16)."
#endif

#if !CONFIG_LV_COLOR_16_SWAP
#error "CO5300 requires CONFIG_LV_COLOR_16_SWAP=y."
#endif

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_GPS_CONTINUOUS_LOCATION)
#error "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION must be defined."
#endif
#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "SVC_UI";

/** 显示传输完成等待片段。 */
#define UI_FLUSH_WAIT_MS 10
/** 一次显示 DMA flush 的最大同步等待时间。 */
#define UI_FULL_REDRAW_FLUSH_TIMEOUT_MS 500U
/** 共享 I2C 触摸读点的短等待预算。 */
#define UI_TOUCH_READ_I2C_TIMEOUT_MS 2
/** 触摸初始化与硬件恢复的 I2C 等待预算。 */
#define UI_TOUCH_SETUP_I2C_TIMEOUT_MS 10
/** 唯一触摸唤醒因 I2C 忙而延后的基础重试毫秒。 */
#define UI_TOUCH_REARM_RETRY_BASE_MS 20U
/** 触摸唤醒重试的省电退避上限毫秒。 */
#define UI_TOUCH_REARM_RETRY_MAX_MS 1000U
/** 区分点击与滑动的最小触摸行程。 */
#define UI_TOUCH_GESTURE_LIMIT_PX 24
/** 用户事件发生后读取最新状态快照的有界等待预算。 */
#define UI_EVENT_SNAPSHOT_TIMEOUT_MS 20U
/** LVGL 局部双缓冲像素数。 */
#define UI_DRAW_BUFFER_PIXELS (CO5300_BSP_WIDTH * CO5300_BSP_DRAW_BUFFER_LINES)
/** 常驻主壳最终固定的软件圆角半径。 */
#define UI_SHELL_ROUND_RADIUS_PX 110
/** 快路等待单块 QSPI DMA 完成的最大时长。 */
#define UI_FAST_CHUNK_TIMEOUT_MS 10U
/** 普通 LVGL flush 等待匹配颜色事务完成的最大时长。 */
#define UI_LVGL_FLUSH_TIMEOUT_US 100000U
/** 控制字段乐观投影固定锁定时长。 */
#define UI_CONTROL_PROJECTION_LOCK_MS 5000U
typedef struct
{
    bool active;         /**< 当前字段是否处于乐观投影窗口。 */
    uint8_t target;      /**< 窗口内显示的 typed 目标值。 */
    uint64_t expires_ms; /**< 固定五秒窗口的单调截止毫秒。 */
} ui_control_projection_lock_t;

/** UI 服务请求队列句柄。 */
static QueueHandle_t s_ui_queue;
/** 唯一 LVGL 所有者任务句柄。 */
static TaskHandle_t s_ui_task;
/** LVGL display draw buffer 描述。 */
static lv_disp_draw_buf_t s_draw_buffer;
/** LVGL display driver 描述。 */
static lv_disp_drv_t s_display_driver;
/** LVGL input driver 描述。 */
static lv_indev_drv_t s_input_driver;
/** LVGL display 句柄。 */
static lv_disp_t *s_display;
/** LVGL input device 句柄。 */
static lv_indev_t *s_input;
/** 内部 DMA RGB565 绘制缓冲区 1。 */
static lv_color_t *s_buffer1;
/** 内部 DMA RGB565 绘制缓冲区 2。 */
static lv_color_t *s_buffer2;
/** UI 服务硬件准备状态。 */
static bool s_prepared;
/** 触摸读点连续失败计数，用于触发 CST9217 硬件复位恢复。 */
static uint32_t s_touch_error_count;
/** ISR 锁存的触摸数据就绪边沿，任务读点完成前不得因引脚回高而丢失。 */
static atomic_bool s_touch_interrupt_pending;
/** ISR 已停用触摸中断且任务级读点需要重试。 */
static bool s_touch_rearm_retry_pending;
/** 下一次任务级触摸读点重试的绝对单调毫秒。 */
static uint64_t s_touch_rearm_retry_at_ms;
/** 当前触摸读点退避档位。 */
static uint8_t s_touch_rearm_retry_stage;
/** UI 循环运行状态。 */
static bool s_running;
/** UI 已向 BLE owner 请求候选页扫描会话。 */
static bool s_ble_binding_scan_session_requested;
/** UI 已向 power owner 同步 BLE 候选页固定 30 秒熄屏状态。 */
static bool s_ble_page_idle_timeout_active;
/** ui_task 已进入可消费请求的 LVGL 所有权循环。 */
static atomic_bool s_ui_running;
/** 是否存在尚未归还 LVGL 的显示 flush。 */
static bool s_flush_pending;
/** ISR 每完成一个颜色事务递增的单调代次。 */
static atomic_uint s_display_color_completion_generation;
/** 普通 LVGL flush 的提交代次与有界等待守卫。 */
static ui_display_flush_guard_t s_lvgl_flush_guard;
/** 当前等待完成的 LVGL driver。 */
static lv_disp_drv_t *s_pending_driver;
/** 当前等待完成的 flush 是否为本轮刷新最后一块。 */
static bool s_pending_flush_last;
/** 当前完整帧首块 flush 是否已经开始。 */
static bool s_frame_flush_active;
/** 当前完整帧任一块提交时常驻 pager 是否处于运动。 */
static bool s_frame_shell_motion_seen;
/** 当前完整帧任一块提交时设置列表是否处于运动。 */
static bool s_frame_settings_motion_seen;
/** 当前完整帧任一块提交时自检结果列表是否处于运动。 */
static bool s_frame_selftest_result_motion_seen;
/** 当前完整帧是否仍可用于性能统计。 */
static bool s_frame_flush_valid;
/** 当前完整帧首块提交的单调微秒。 */
static uint64_t s_frame_flush_started_us;
/** 当前完整帧累计实际传输像素。 */
static uint32_t s_frame_flush_pixels;
/** 常驻主壳横滑性能统计器。 */
static ui_performance_metrics_t s_shell_performance_metrics;
/** 设置七栏纵滑性能统计器。 */
static ui_performance_metrics_t s_settings_performance_metrics;
/** 自检结果纵滑性能统计器。 */
static ui_performance_metrics_t s_selftest_result_performance_metrics;
/** 已汇入周期门禁的主壳快照准备失败累计值。 */
static uint32_t s_shell_prepare_failures_observed;
/** 已汇入周期门禁的设置快照准备失败累计值。 */
static uint32_t s_settings_prepare_failures_observed;
/** 已汇入周期门禁的自检结果快照准备失败累计值。 */
static uint32_t s_selftest_result_prepare_failures_observed;
/** 常驻主壳屏幕固定圆角每一行的黑边像素数。 */
static uint16_t s_shell_round_insets[CO5300_BSP_HEIGHT];
/** 常驻主壳硬件快路错误是否已经记录，避免热路径重复打印。 */
static bool s_shell_fast_failure_logged;
/** 设置纵滑硬件快路错误是否已经记录，避免热路径重复打印。 */
static bool s_settings_fast_failure_logged;
/** 自检结果纵滑硬件快路错误是否已经记录。 */
static bool s_selftest_result_fast_failure_logged;
/** 本轮同步整屏刷新是否出现立即提交错误。 */
static bool s_full_redraw_flush_error;
/** ui_task 已确认的显示开关事实。 */
static bool s_display_on;
/** CO5300 已供电并允许在 DISPOFF 状态接收隐藏首帧。 */
static bool s_display_flush_enabled;
/** 当前亮度缓存是否对应一次已确认的硬件写入。 */
static bool s_brightness_valid;
/** 最近一次已确认的原始亮度值。 */
static uint16_t s_brightness_value;
/** 首个唤醒触摸序列消费门。 */
static ui_wake_touch_gate_t s_wake_touch_gate;
/** state_task 合并写入、ui_task 原子取走的最新自检刷新 run ID。 */
static atomic_uint s_pending_selftest_refresh_run_id;
/** state_task 合并写入、ui_task 最终消费的模型刷新标志。 */
static atomic_bool s_pending_model_refresh;
/** 最近一次成功渲染的完整模型，供纯本地常驻页导航复用。 */
static ui_runtime_model_t s_last_rendered_model;
/** 最近一次成功渲染模型是否有效。 */
static bool s_last_rendered_model_valid;
/** 设置页触觉反馈的当前 UI 偏好。 */
static bool s_haptics_enabled =
    CONFIG_SERVICE_UI_DEFAULT_HAPTICS_ENABLED;
/** 设置页点击音效的当前 UI 偏好。 */
static bool s_click_audio_enabled =
    CONFIG_SERVICE_UI_DEFAULT_CLICK_AUDIO_ENABLED;
/** 设置页抬腕亮屏的当前 UI 偏好。 */
static bool s_raise_to_wake_enabled =
    CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED;
/** 设置页屏幕亮度的当前持久化档位。 */
static config_service_ui_brightness_t s_brightness_level =
    CONFIG_SERVICE_UI_DEFAULT_BRIGHTNESS;
/** 设置页自动熄屏的当前持久化档位。 */
static config_service_ui_screen_timeout_t s_screen_timeout =
    CONFIG_SERVICE_UI_DEFAULT_SCREEN_TIMEOUT;
/** 当前 boot 是否已读取并消费上次整机自检 dirty marker。 */
static bool s_selftest_marker_consumed_this_boot;
/** 当前 boot 是否需要提示上次整机自检未完成。 */
static bool s_previous_selftest_incomplete;
/** 整机自检运行期间是否已进入熄屏抑制状态。 */
static bool s_selftest_display_forced_on;
/** 自检亮屏的唯一硬件重试失败后是否等待新的用户唤醒事件。 */
static bool s_selftest_display_retry_suppressed;
/** GPS 环境选择提交后的瞬态显示状态。 */
static ui_runtime_gps_submit_state_t s_gps_submit_state;
/** 清除绑定命令已从 UI 成功提交的瞬态标志。 */
static bool s_clear_binding_submitted;
/** 清除绑定提交后的首次本地刷新尚未完成，避免把旧 ERROR 误判为本次终态。 */
static bool s_clear_binding_submission_refresh_pending;
/** 最近一次 UI 发起的清除绑定请求已由 owner 判定失败。 */
static bool s_clear_binding_failed;
/** 用户是否在本轮支付阻断中主动触发过验证。 */
static bool s_payment_verification_attempted;
/** 支付验证命令是否等待业务 owner 终态。 */
static bool s_payment_verification_pending;
/** 支付验证提交时由 BLE 入口返回的非零 intent 关联序号。 */
static uint32_t s_payment_intent_sequence;
/** 支付验证瞬态状态所属的 BLE 物理链路代次。 */
static uint32_t s_payment_link_generation;
/** 人工自检回复是否等待 selftest owner 推进。 */
static bool s_manual_reply_submitted;
/** 人工自检回复所属运行 ID。 */
static uint32_t s_manual_reply_run_id;
/** 人工自检回复所属项目。 */
static selftest_item_id_t s_manual_reply_item = SELFTEST_ITEM_COUNT;
/** 当前人工自检提示开始显示的单调毫秒。 */
static uint64_t s_manual_prompt_started_ms;
/** 当前人工自检提示所属运行 ID。 */
static uint32_t s_manual_prompt_run_id;
/** 当前人工自检提示所属项目。 */
static selftest_item_id_t s_manual_prompt_item = SELFTEST_ITEM_COUNT;
/** 人工自检倒计时最近已请求刷新的秒值。 */
static uint32_t s_manual_countdown_second = UINT32_MAX;
/** 档位字段乐观投影。 */
static ui_control_projection_lock_t s_gear_projection;
/** 助力字段乐观投影。 */
static ui_control_projection_lock_t s_assist_projection;
/** 模式字段乐观投影。 */
static ui_control_projection_lock_t s_mode_projection;
/** 当前已投影的跨界控制请求，防止同一 pending 事务在五秒后重复上锁。 */
static uint32_t s_boundary_projection_request_id;
/** 最近一次已记录的外骨骼型号是否有效。 */
static bool s_logged_exoskeleton_model_valid;
/** 最近一次已记录的外骨骼型号。 */
static watch_exoskeleton_model_t s_logged_exoskeleton_model;
/** 最近一次已记录的外骨骼原始版本。 */
static char s_logged_exoskeleton_version[WATCH_EXOSKELETON_VERSION_CAPACITY];
/** 最近一次已记录的控制可用性是否有效。 */
static bool s_logged_control_availability_valid;
/** 最近一次已记录的控制阻断原因。 */
static control_ui_block_reason_t s_logged_control_block_reason;
/** 最近一次已记录的支付阻断状态。 */
static bool s_logged_payment_required;
/** 最近一次已记录的授权显示状态。 */
static ui_authorization_display_t s_logged_authorization_display;
/** 最近一次已记录的支付验证按钮状态。 */
static bool s_logged_payment_action_enabled;
/** 最近一次已记录的 4G 维护状态是否有效。 */
static bool s_logged_cellular_state_valid;
/** 最近一次已记录的 modem 生命周期。 */
static watch_modem_lifecycle_t s_logged_cellular_lifecycle;
/** 最近一次已记录的 4G 联网状态。 */
static bool s_logged_cellular_connected;
/** 最近一次已记录的 4G 手动连接按钮状态。 */
static bool s_logged_cellular_action_enabled;
/** 最近一次已记录的 4G 连接中状态。 */
static bool s_logged_cellular_connecting;
/** 下一次可信分钟边界刷新时刻；UINT64_MAX 表示当前无可信时间。 */
static uint64_t s_next_clock_refresh_ms = UINT64_MAX;

static bool display_flush_done_from_isr(void *user_ctx);
static void display_rounder(lv_disp_drv_t *driver, lv_area_t *area);
static void display_flush(lv_disp_drv_t *driver,
                          const lv_area_t *area,
                          lv_color_t *color_map);
static void display_wait(lv_disp_drv_t *driver);
static void touch_read(lv_indev_drv_t *driver, lv_indev_data_t *data);
static void schedule_touch_rearm_retry(void);
static void clear_touch_rearm_retry(void);
static void report_power_runtime_result(watch_power_error_t error);
static esp_err_t initialize_lvgl(void);
static void complete_pending_flush(TickType_t wait_ticks);
static bool render_shell_fast_frame(void);
static bool render_settings_fast_frame(void);
static bool render_selftest_result_fast_frame(void);
static void record_shell_fast_path_miss(uint64_t now_us);
static void record_settings_fast_path_miss(uint64_t now_us);
static void record_selftest_result_fast_path_miss(uint64_t now_us);
static void abort_hardware_fast_path(bool dma_state_unknown);
static bool wait_for_fast_completion(uint32_t generation);
static void compose_shell_fast_chunk(
    lv_color_t *destination,
    const ui_runtime_shell_fast_frame_t *frame,
    uint16_t first_y,
    uint16_t line_count);
static void compose_settings_fast_chunk(
    lv_color_t *destination,
    const ui_runtime_settings_fast_frame_t *frame,
    uint16_t first_y,
    uint16_t line_count);
static void compose_selftest_result_fast_chunk(
    lv_color_t *destination,
    const ui_runtime_selftest_result_fast_frame_t *frame,
    uint16_t first_y,
    uint16_t line_count);
static void initialize_shell_round_insets(void);
static bool wait_for_pending_flush(uint32_t timeout_ms);
static bool refresh_full_screen_once(void);
static bool refresh_full_screen_with_recovery(void);
static void consume_full_redraw_request(void);
static void complete_flush_after_display_reset(void);
static void sync_performance_motion_and_report(uint64_t now_us);
static void log_shell_performance_report(
    const ui_performance_report_t *report);
static void log_settings_performance_report(
    const ui_performance_report_t *report);
static void log_selftest_result_performance_report(
    const ui_performance_report_t *report);
static bool is_manual_selftest_item(selftest_item_id_t item_id);
static void handle_request(const ui_service_request_t *request);
static esp_err_t build_runtime_model(ui_runtime_model_t *model,
                                     TickType_t snapshot_timeout);
static bool render_runtime_model(const ui_runtime_model_t *model);
static bool refresh_runtime_model(void);
static void sync_ble_binding_scan_session(void);
static esp_err_t apply_runtime_brightness(
    watch_power_level_t power_level,
    bool selftest_active,
    bool force);
static uint16_t brightness_raw_for_level(
    config_service_ui_brightness_t level);
static uint32_t timeout_ms_for_preference(
    config_service_ui_screen_timeout_t timeout);
static config_service_ui_preferences_t current_ui_preferences(void);
static void handle_ui_intent(const ui_runtime_intent_t *intent, void *context);
static bool is_local_shell_navigation_intent(
    ui_runtime_intent_type_t type);
static void drain_ble_results(void);
static void request_ui_feedback(bool haptics_enabled,
                                bool click_audio_enabled);
static esp_err_t submit_control_intent(const ui_runtime_intent_t *intent,
                                       const ui_runtime_model_t *model);
static esp_err_t submit_selftest_reply(const ui_runtime_intent_t *intent,
                                       const ui_runtime_model_t *model);
static void apply_control_projection(ui_runtime_model_t *model,
                                     bool ble_connected,
                                     uint64_t now_ms);
static void lock_control_projection(const ui_runtime_intent_t *intent,
                                    const ui_runtime_model_t *model,
                                    uint64_t now_ms);
static bool control_projection_refresh_due(uint64_t now_ms);
static void clear_control_projections(void);
static void clear_control_projection_for_kind(watch_control_kind_t kind);
static bool control_result_is_terminal(ble_control_result_code_t result);
static bool is_control_intent(ui_runtime_intent_type_t type);
static const char *ui_intent_name(
    ui_runtime_intent_type_t type,
    const ui_runtime_model_t *model);
static const char *control_block_reason_name(
    control_ui_block_reason_t reason);
static const char *exoskeleton_model_name(watch_exoskeleton_model_t model);
static const char *control_rejection_reason(
    const ui_runtime_intent_t *intent,
    const ui_runtime_model_t *model);
static void log_control_click(const ui_runtime_intent_t *intent,
                              const ui_runtime_model_t *model);
static void log_runtime_model_transitions(
    const watch_state_snapshot_t *watch,
    const ui_runtime_model_t *model);
static bool manual_countdown_refresh_due(uint64_t now_ms);
static bool clock_refresh_due(uint64_t now_ms);
static void append_alert(ui_runtime_model_t *model,
                         ui_runtime_alert_type_t type,
                         bool danger,
                         const char *label,
                         const char *value);
static bool is_printable_ascii_text(const char *text);
static void copy_text(char *destination, size_t capacity, const char *source);
static const char *selftest_item_text(selftest_item_id_t item);
static const char *selftest_prompt(selftest_item_id_t item);
static void touch_interrupt_from_isr(void *user_ctx);
static void wake_ui_owner(void);
static esp_err_t reveal_display_after_full_redraw(void);
static esp_err_t suspend_display_with_single_retry(void);
static esp_err_t resume_display_with_single_retry(void);
static esp_err_t reset_display_touch(bool recover_timed_out_flush);
static void cleanup_prepare_failure(void);

void ui_wake_touch_gate_init(ui_wake_touch_gate_t *gate,
                             bool display_on)
{
    if (gate == NULL)
    {
        return;
    }
    *gate = (ui_wake_touch_gate_t){
        .display_on = display_on,
    };
}

void ui_wake_touch_gate_set_display(ui_wake_touch_gate_t *gate,
                                    bool display_on)
{
    if (gate != NULL)
    {
        gate->display_on = display_on;
    }
}

bool ui_wake_touch_gate_filter(ui_wake_touch_gate_t *gate,
                               bool raw_pressed,
                               bool *wake_requested)
{
    if (gate == NULL || wake_requested == NULL)
    {
        return false;
    }
    *wake_requested = false;
    if (!gate->display_on)
    {
        if (raw_pressed && !gate->consume_until_release)
        {
            gate->consume_until_release = true;
            *wake_requested = true;
        }
        return false;
    }
    if (gate->consume_until_release)
    {
        if (!raw_pressed)
        {
            gate->consume_until_release = false;
        }
        return false;
    }
    return raw_pressed;
}

bool ui_service_should_request_feedback(bool button_source,
                                        bool intent_accepted)
{
    return button_source && intent_accepted;
}

bool ui_service_cellular_action_available(
    bool network_connected,
    bool selftest_running,
    watch_modem_lifecycle_t lifecycle)
{
#if !LEGBOT_CAP_MODEM
    (void)network_connected;
    (void)selftest_running;
    (void)lifecycle;
    return false;
#else
    return !network_connected &&
           !selftest_running &&
           (lifecycle == WATCH_MODEM_LIFECYCLE_RECOVERING ||
            lifecycle == WATCH_MODEM_LIFECYCLE_BACKOFF);
#endif
}

esp_err_t ui_service_init(void)
{
    if (s_prepared)
    {
        return ESP_OK;
    }
    if (!i2c_manager_is_initialized())
    {
        return ESP_ERR_INVALID_STATE;
    }

    s_ui_queue = xQueueCreate(UI_SERVICE_QUEUE_DEPTH, sizeof(ui_service_request_t));
    if (s_ui_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    atomic_store(&s_pending_model_refresh, false);
    atomic_store(&s_pending_selftest_refresh_run_id, 0U);
    atomic_store(&s_touch_interrupt_pending, false);
    memset(&s_last_rendered_model, 0, sizeof(s_last_rendered_model));
    s_last_rendered_model_valid = false;
    ui_performance_metrics_init(&s_shell_performance_metrics);
    ui_performance_metrics_init(&s_settings_performance_metrics);
    ui_performance_metrics_init(&s_selftest_result_performance_metrics);
    s_shell_prepare_failures_observed = 0U;
    s_settings_prepare_failures_observed = 0U;
    s_selftest_result_prepare_failures_observed = 0U;
    s_pending_flush_last = false;
    s_frame_flush_active = false;
    s_frame_shell_motion_seen = false;
    s_frame_settings_motion_seen = false;
    s_frame_selftest_result_motion_seen = false;
    s_frame_flush_valid = false;
    s_frame_flush_started_us = 0U;
    s_frame_flush_pixels = 0U;
    s_gps_submit_state = UI_RUNTIME_GPS_SUBMIT_IDLE;
    s_clear_binding_submitted = false;
    s_clear_binding_submission_refresh_pending = false;
    s_clear_binding_failed = false;
    s_payment_verification_attempted = false;
    s_payment_verification_pending = false;
    s_payment_intent_sequence = 0U;
    s_payment_link_generation = 0U;
    s_manual_reply_submitted = false;
    s_manual_reply_run_id = 0U;
    s_manual_reply_item = SELFTEST_ITEM_COUNT;
    s_manual_prompt_started_ms = 0U;
    s_manual_prompt_run_id = 0U;
    s_manual_prompt_item = SELFTEST_ITEM_COUNT;
    s_manual_countdown_second = UINT32_MAX;
    s_next_clock_refresh_ms = UINT64_MAX;
    s_selftest_display_forced_on = false;
    s_selftest_display_retry_suppressed = false;
    s_ble_binding_scan_session_requested = false;
    s_ble_page_idle_timeout_active = false;
    s_logged_exoskeleton_model_valid = false;
    s_logged_exoskeleton_model = WATCH_EXOSKELETON_MODEL_UNKNOWN;
    memset(s_logged_exoskeleton_version,
           0,
           sizeof(s_logged_exoskeleton_version));
    s_logged_control_availability_valid = false;
    s_logged_control_block_reason = CONTROL_UI_BLOCK_SNAPSHOT_FAILED;
    s_logged_payment_required = false;
    s_logged_authorization_display = UI_AUTHORIZATION_DISPLAY_NONE;
    s_logged_payment_action_enabled = false;
    s_logged_cellular_state_valid = false;
    s_logged_cellular_lifecycle = WATCH_MODEM_LIFECYCLE_BOOTING;
    s_logged_cellular_connected = false;
    s_logged_cellular_action_enabled = false;
    s_logged_cellular_connecting = false;
    clear_control_projections();
    s_display_on = false;
    s_display_flush_enabled = true;
    s_brightness_valid = false;
    s_brightness_value = 0U;
    clear_touch_rearm_retry();
    ui_wake_touch_gate_init(&s_wake_touch_gate, false);
    ui_touch_session_reset();

    config_service_ui_preferences_t preferences = {0};
    const esp_err_t preferences_error = config_service_ui_preferences_read(
        &preferences,
        pdMS_TO_TICKS(20));
    if (preferences_error == ESP_OK)
    {
        s_haptics_enabled = preferences.haptics_enabled;
        s_click_audio_enabled = preferences.click_audio_enabled;
        s_raise_to_wake_enabled = preferences.raise_to_wake_enabled;
        s_brightness_level = preferences.brightness;
        s_screen_timeout = preferences.screen_timeout;
    }
    else
    {
        s_haptics_enabled = CONFIG_SERVICE_UI_DEFAULT_HAPTICS_ENABLED;
        s_click_audio_enabled =
            CONFIG_SERVICE_UI_DEFAULT_CLICK_AUDIO_ENABLED;
        s_raise_to_wake_enabled =
            CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED;
        s_brightness_level = CONFIG_SERVICE_UI_DEFAULT_BRIGHTNESS;
        s_screen_timeout = CONFIG_SERVICE_UI_DEFAULT_SCREEN_TIMEOUT;
        ESP_LOGW(TAG,
                 "读取 UI 偏好失败，使用振动开启、声音与抬腕关闭、中亮度和 5 秒默认值，错误=0x%x",
                 (unsigned)preferences_error);
    }
    if (!s_selftest_marker_consumed_this_boot)
    {
        bool previous_running = false;
        const esp_err_t marker_error = config_service_selftest_running_read(
            &previous_running,
            pdMS_TO_TICKS(20));
        if (marker_error == ESP_OK)
        {
            s_previous_selftest_incomplete = previous_running;
            s_selftest_marker_consumed_this_boot = true;
            if (previous_running)
            {
                const esp_err_t clear_error =
                    config_service_selftest_running_write(
                        false,
                        pdMS_TO_TICKS(20));
                if (clear_error != ESP_OK)
                {
                    ESP_LOGW(TAG,
                             "上次整机自检未完成标记本 boot 清理失败，错误=0x%x",
                             (unsigned)clear_error);
                }
            }
        }
        else
        {
            ESP_LOGW(TAG,
                     "读取上次整机自检运行标记失败，错误=0x%x",
                     (unsigned)marker_error);
        }
    }

    /* esp_lcd 不会为颜色事务设置 PSRAM 直通 DMA 标志，因此绘制缓冲必须直接位于内部 DMA 内存。 */
    const size_t draw_buffer_bytes = UI_DRAW_BUFFER_PIXELS * sizeof(lv_color_t);
    const uint32_t buffer_caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL;
    s_buffer1 = heap_caps_malloc(draw_buffer_bytes, buffer_caps);
    s_buffer2 = heap_caps_malloc(draw_buffer_bytes, buffer_caps);
    if (s_buffer1 == NULL || s_buffer2 == NULL)
    {
        ESP_LOGE(TAG,
                 "LVGL 内部 DMA 双缓冲分配失败：单块=%lu 字节，DMA 空闲=%lu 字节，最大连续块=%lu 字节",
                 (unsigned long)draw_buffer_bytes,
                 (unsigned long)heap_caps_get_free_size(buffer_caps),
                 (unsigned long)heap_caps_get_largest_free_block(buffer_caps));
        cleanup_prepare_failure();
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG,
             "LVGL 内部 DMA 双缓冲已就绪：单块=%lu 字节，行数=%d，DMA 剩余=%lu 字节",
             (unsigned long)draw_buffer_bytes,
             CO5300_BSP_DRAW_BUFFER_LINES,
             (unsigned long)heap_caps_get_free_size(buffer_caps));

    const co5300_bsp_config_t display_config = {
        .flush_done_cb = display_flush_done_from_isr,
        .user_ctx = NULL,
    };
    esp_err_t err = co5300_bsp_init(&display_config);
    if (err != ESP_OK)
    {
        cleanup_prepare_failure();
        return err;
    }
    err = apply_runtime_brightness(WATCH_POWER_LEVEL_NORMAL,
                                   false,
                                   true);
    if (err != ESP_OK)
    {
        cleanup_prepare_failure();
        return err;
    }

    err = i2c_manager_acquire(
        pdMS_TO_TICKS(UI_TOUCH_SETUP_I2C_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        cleanup_prepare_failure();
        return err;
    }
    err = cst9217_bsp_init(i2c_manager_bus_handle());
    i2c_manager_release();
    if (err != ESP_OK)
    {
        cleanup_prepare_failure();
        return err;
    }

    s_prepared = true;
    ESP_LOGI(TAG, "UI 硬件底座已就绪，LVGL 注册将由 ui_task 独占执行");
    return ESP_OK;
}

QueueHandle_t ui_service_queue(void)
{
    return s_ui_queue;
}

esp_err_t ui_service_post_request(const ui_service_request_t *request,
                                  TickType_t timeout_ticks)
{
    if (s_ui_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (request == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (request->type < UI_SERVICE_REQUEST_MODEL_UPDATE ||
        request->type > UI_SERVICE_REQUEST_STOP ||
        (request->type == UI_SERVICE_REQUEST_SELFTEST_ITEM &&
         (request->run_id == 0U || !is_manual_selftest_item(request->item_id))) ||
        (request->type == UI_SERVICE_REQUEST_SELFTEST_REFRESH &&
         request->run_id == 0U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (request->type == UI_SERVICE_REQUEST_MODEL_UPDATE)
    {
        /* 模型刷新只携带“读取最新快照”语义，队列满时也不丢最终真值。 */
        atomic_store(&s_pending_model_refresh, true);
        wake_ui_owner();
        return ESP_OK;
    }
    /* 其他服务只能通过队列请求 UI 行为，避免直接跨线程调用 LVGL。 */
    if (xQueueSend(s_ui_queue, request, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    wake_ui_owner();
    return ESP_OK;
}

esp_err_t ui_service_request_selftest_item(uint32_t run_id,
                                           selftest_item_id_t item_id,
                                           TickType_t timeout_ticks)
{
    if (run_id == 0U || !is_manual_selftest_item(item_id))
    {
        return ESP_ERR_INVALID_ARG;
    }
    const ui_service_request_t request = {
        .type = UI_SERVICE_REQUEST_SELFTEST_ITEM,
        .run_id = run_id,
        .item_id = item_id,
    };
    return ui_service_post_request(&request, timeout_ticks);
}

esp_err_t ui_service_refresh_selftest_results(uint32_t run_id,
                                              TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (run_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ui_queue == NULL || !atomic_load(&s_ui_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    /* 阶段一仍合并刷新信号，正式页面接入后由同一 owner 消费最新快照。 */
    atomic_store(&s_pending_selftest_refresh_run_id, run_id);
    wake_ui_owner();
    return ESP_OK;
}

void ui_service_run(void)
{
    if (!s_prepared)
    {
        ESP_LOGE(TAG, "UI 硬件底座未初始化，拒绝进入 ui_task 主循环");
        return;
    }
    s_ui_task = xTaskGetCurrentTaskHandle();
    const esp_err_t interrupt_error =
        cst9217_bsp_register_interrupt_callback(
            touch_interrupt_from_isr,
            NULL);
    if (interrupt_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "CST9217 中断唤醒接入失败，拒绝进入 UI 低功耗循环，错误=0x%x",
                 (unsigned)interrupt_error);
        s_ui_task = NULL;
        return;
    }
    const esp_err_t err = initialize_lvgl();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "LVGL 底座初始化失败，错误=0x%x", (unsigned)err);
        (void)cst9217_bsp_register_interrupt_callback(NULL, NULL);
        s_ui_task = NULL;
        return;
    }

    ui_runtime_binding_init(handle_ui_intent, NULL);
    ui_runtime_binding_set_display_on(false);
    if (!refresh_runtime_model())
    {
        ESP_LOGW(TAG, "首帧产品快照读取失败，保留 SquareLine 初始页面等待刷新");
    }
    const esp_err_t reveal_error = reveal_display_after_full_redraw();
    if (reveal_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "首帧完整提交前保持 AMOLED 熄灭，显示启动失败，错误=0x%x",
                 (unsigned)reveal_error);
        ui_runtime_binding_deinit();
        (void)cst9217_bsp_register_interrupt_callback(NULL, NULL);
        s_ui_task = NULL;
        return;
    }
    s_running = true;
    atomic_store(&s_ui_running, true);
    int64_t last_tick_us = esp_timer_get_time();

    while (s_running)
    {
        const int64_t now_us = esp_timer_get_time();
        const uint32_t elapsed_ms =
            (uint32_t)((now_us - last_tick_us) / 1000);
        if (elapsed_ms > 0U)
        {
            lv_tick_inc(elapsed_ms);
            last_tick_us += (int64_t)elapsed_ms * 1000;
        }

        sync_performance_motion_and_report((uint64_t)now_us);
        complete_pending_flush(0);
        const int64_t render_started_us = esp_timer_get_time();
        uint32_t wait_ms = lv_timer_handler();
        const bool fast_frame_rendered =
            render_shell_fast_frame() ||
            render_settings_fast_frame() ||
            render_selftest_result_fast_frame();
        const int64_t render_finished_us = esp_timer_get_time();
        complete_pending_flush(0);
        const uint64_t render_us =
            render_finished_us >= render_started_us
                ? (uint64_t)(render_finished_us - render_started_us)
                : 0U;
        ui_performance_metrics_record_render(
            &s_shell_performance_metrics,
            render_us > UINT32_MAX ? UINT32_MAX : (uint32_t)render_us);
        ui_performance_metrics_record_render(
            &s_settings_performance_metrics,
            render_us > UINT32_MAX ? UINT32_MAX : (uint32_t)render_us);
        ui_performance_metrics_record_render(
            &s_selftest_result_performance_metrics,
            render_us > UINT32_MAX ? UINT32_MAX : (uint32_t)render_us);
        sync_performance_motion_and_report(
            (uint64_t)render_finished_us);
        TickType_t wait_ticks = 0U;
        const bool touch_interrupt_pending = atomic_load_explicit(
            &s_touch_interrupt_pending,
            memory_order_acquire);
        if (touch_interrupt_pending)
        {
            /* ISR 已停用 IO39；最多等待一个 tick 让 LVGL 立即消费锁存边沿。 */
            wait_ticks = 1U;
        }
        else if (!s_display_on && !fast_frame_rendered)
        {
            if (!s_touch_rearm_retry_pending)
            {
                wait_ticks = portMAX_DELAY;
            }
            else
            {
                const uint64_t now_ms = (uint64_t)now_us / 1000U;
                const uint64_t remaining_ms =
                    s_touch_rearm_retry_at_ms > now_ms
                        ? s_touch_rearm_retry_at_ms - now_ms
                        : 1U;
                const uint32_t bounded_wait_ms =
                    remaining_ms > UINT32_MAX
                        ? UINT32_MAX
                        : (uint32_t)remaining_ms;
                wait_ticks = pdMS_TO_TICKS(bounded_wait_ms);
                if (wait_ticks == 0U)
                {
                    wait_ticks = 1U;
                }
            }
        }
        else if (fast_frame_rendered)
        {
            wait_ms = 0U;
            wait_ticks = 0U;
        }
        else
        {
            if (wait_ms == 0U ||
                wait_ms > UI_SERVICE_MAX_LOOP_DELAY_MS)
            {
                wait_ms = UI_SERVICE_MAX_LOOP_DELAY_MS;
            }
            wait_ticks = pdMS_TO_TICKS(wait_ms);
        }

        (void)ulTaskNotifyTake(pdTRUE, wait_ticks);
        ui_service_request_t request = {0};
        while (xQueueReceive(s_ui_queue, &request, 0U) == pdTRUE)
        {
            handle_request(&request);
            if (!s_running)
            {
                break;
            }
        }
        drain_ble_results();

        const bool interaction_motion_active =
            ui_runtime_binding_motion_active();
        const bool model_refresh =
            !interaction_motion_active &&
            atomic_exchange(&s_pending_model_refresh, false);
        const uint32_t selftest_refresh_run_id =
            interaction_motion_active
                ? 0U
                : atomic_exchange(
                      &s_pending_selftest_refresh_run_id, 0U);
        const uint64_t refresh_now_ms =
            (uint64_t)esp_timer_get_time() / 1000U;
        const bool projection_refresh =
            !interaction_motion_active &&
            control_projection_refresh_due(refresh_now_ms);
        const bool countdown_refresh =
            !interaction_motion_active &&
            manual_countdown_refresh_due(refresh_now_ms);
        const bool clock_refresh =
            !interaction_motion_active &&
            clock_refresh_due(refresh_now_ms);
        if ((model_refresh || selftest_refresh_run_id != 0U ||
             projection_refresh || countdown_refresh || clock_refresh) &&
            !refresh_runtime_model())
        {
            ESP_LOGW(TAG, "UI 模型刷新快照失败，保留当前页面等待下一次请求");
            atomic_store(&s_pending_model_refresh, true);
            if (selftest_refresh_run_id != 0U)
            {
                atomic_store(&s_pending_selftest_refresh_run_id,
                             selftest_refresh_run_id);
            }
        }
        consume_full_redraw_request();
        (void)ui_runtime_binding_warm_shell_snapshot();
        (void)ui_runtime_binding_warm_settings_snapshot();
        (void)ui_runtime_binding_warm_selftest_result_snapshot();
    }

    while (s_flush_pending)
    {
        complete_pending_flush(pdMS_TO_TICKS(UI_FLUSH_WAIT_MS));
    }
    if (s_ble_binding_scan_session_requested)
    {
        (void)ble_service_end_binding_scan_session(
            pdMS_TO_TICKS(20));
        s_ble_binding_scan_session_requested = false;
    }
    ui_runtime_binding_deinit();
    atomic_store(&s_ui_running, false);
    const esp_err_t interrupt_release_error =
        cst9217_bsp_register_interrupt_callback(NULL, NULL);
    if (interrupt_release_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "UI 退出时 CST9217 中断回调解注册失败，错误=0x%x",
                 (unsigned)interrupt_release_error);
    }
    ESP_LOGI(TAG, "ui_task 已退出 LVGL 所有权循环");
    s_ui_task = NULL;
}

static bool display_flush_done_from_isr(void *user_ctx)
{
    (void)user_ctx;
    atomic_fetch_add_explicit(
        &s_display_color_completion_generation,
        1U,
        memory_order_relaxed);
    if (s_ui_task == NULL)
    {
        return false;
    }
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_ui_task, &higher_priority_task_woken);
    return higher_priority_task_woken == pdTRUE;
}

static void touch_interrupt_from_isr(void *user_ctx)
{
    (void)user_ctx;
    atomic_store_explicit(&s_touch_interrupt_pending,
                          true,
                          memory_order_release);
    if (s_ui_task == NULL)
    {
        return;
    }
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_ui_task, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void wake_ui_owner(void)
{
    if (s_ui_task != NULL)
    {
        xTaskNotifyGive(s_ui_task);
    }
}

static void display_rounder(lv_disp_drv_t *driver, lv_area_t *area)
{
    (void)driver;
    /* RGB565 每次传输固定为偶数像素，使 DMA 长度保持 4 字节对齐且无需临时缓冲。 */
    if ((area->x1 & 1) != 0)
    {
        --area->x1;
    }
    if ((area->x2 & 1) == 0)
    {
        ++area->x2;
    }
    if (area->x1 < 0)
    {
        area->x1 = 0;
    }
    if (area->x2 >= CO5300_BSP_WIDTH)
    {
        area->x2 = CO5300_BSP_WIDTH - 1;
    }
}

static void display_flush(lv_disp_drv_t *driver,
                          const lv_area_t *area,
                          lv_color_t *color_map)
{
    if (!s_display_flush_enabled)
    {
        lv_disp_flush_ready(driver);
        return;
    }
    const uint64_t now_us = (uint64_t)esp_timer_get_time();
    ui_performance_metrics_set_motion(
        &s_shell_performance_metrics,
        ui_runtime_binding_shell_motion_active(),
        now_us);
    ui_performance_metrics_set_motion(
        &s_settings_performance_metrics,
        ui_runtime_binding_settings_motion_active(),
        now_us);
    ui_performance_metrics_set_motion(
        &s_selftest_result_performance_metrics,
        ui_runtime_binding_selftest_result_motion_active(),
        now_us);
    if (!s_frame_flush_active)
    {
        s_frame_flush_active = true;
        s_frame_flush_valid = true;
        s_frame_shell_motion_seen = false;
        s_frame_settings_motion_seen = false;
        s_frame_selftest_result_motion_seen = false;
        s_frame_flush_started_us = now_us;
        s_frame_flush_pixels = 0U;
    }
    const uint32_t width =
        (uint32_t)(area->x2 - area->x1 + 1);
    const uint32_t height =
        (uint32_t)(area->y2 - area->y1 + 1);
    const uint64_t pixels =
        (uint64_t)s_frame_flush_pixels + (uint64_t)width * height;
    s_frame_flush_pixels =
        pixels > UINT32_MAX ? UINT32_MAX : (uint32_t)pixels;
    s_frame_shell_motion_seen =
        s_frame_shell_motion_seen ||
        ui_runtime_binding_shell_motion_active();
    s_frame_settings_motion_seen =
        s_frame_settings_motion_seen ||
        ui_runtime_binding_settings_motion_active();
    s_frame_selftest_result_motion_seen =
        s_frame_selftest_result_motion_seen ||
        ui_runtime_binding_selftest_result_motion_active();
    const uint32_t submitted_generation = atomic_load_explicit(
        &s_display_color_completion_generation,
        memory_order_relaxed);
    ui_display_flush_guard_begin(
        &s_lvgl_flush_guard,
        submitted_generation,
        (int64_t)now_us);
    s_pending_flush_last = lv_disp_flush_is_last(driver);
    s_pending_driver = driver;
    s_flush_pending = true;
    const esp_err_t err = co5300_bsp_draw_bitmap(area->x1,
                                                 area->y1,
                                                 area->x2 + 1,
                                                 area->y2 + 1,
                                                 color_map);
    if (err != ESP_OK)
    {
        s_flush_pending = false;
        s_pending_driver = NULL;
        ui_display_flush_guard_clear(&s_lvgl_flush_guard);
        s_frame_flush_valid = false;
        s_full_redraw_flush_error = true;
        ESP_LOGE(TAG, "显示 flush 失败，错误=0x%x", (unsigned)err);
        lv_disp_flush_ready(driver);
        if (s_pending_flush_last)
        {
            s_frame_flush_active = false;
            s_frame_shell_motion_seen = false;
            s_frame_settings_motion_seen = false;
            s_frame_selftest_result_motion_seen = false;
            s_frame_flush_pixels = 0U;
        }
        s_pending_flush_last = false;
    }
}

static void display_wait(lv_disp_drv_t *driver)
{
    (void)driver;
    complete_pending_flush(pdMS_TO_TICKS(UI_FLUSH_WAIT_MS));
    const int64_t now_us = esp_timer_get_time();
    if (!s_flush_pending ||
        !ui_display_flush_guard_timed_out(
            &s_lvgl_flush_guard,
            now_us,
            UI_LVGL_FLUSH_TIMEOUT_US))
    {
        return;
    }

    const uint32_t current_generation = atomic_load_explicit(
        &s_display_color_completion_generation,
        memory_order_relaxed);
    ESP_LOGE(TAG,
             "LVGL flush 等待匹配颜色事务超时：提交代次=%lu，当前代次=%lu，执行显示恢复",
             (unsigned long)s_lvgl_flush_guard.submitted_generation,
             (unsigned long)current_generation);
    s_frame_flush_valid = false;
    s_full_redraw_flush_error = true;
    const esp_err_t err = reset_display_touch(true);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "LVGL flush 超时后的显示恢复失败，错误=0x%x",
                 (unsigned)err);
        if (s_flush_pending)
        {
            ui_display_flush_guard_rearm_timeout(
                &s_lvgl_flush_guard,
                esp_timer_get_time());
        }
    }
}

static void touch_read(lv_indev_drv_t *driver, lv_indev_data_t *data)
{
    (void)driver;
    data->state = LV_INDEV_STATE_RELEASED;

    cst9217_bsp_point_t point = {0};
    ui_touch_sample_t sample = UI_TOUCH_SAMPLE_UNKNOWN;
    const int interrupt_level =
        gpio_get_level(LEGBOT_BSP_CST9217_INT_GPIO);
    ui_touch_snapshot_t current_touch = {0};
    const bool touch_active =
        ui_touch_session_snapshot(&current_touch) && current_touch.active;
    const bool data_ready = interrupt_level == 0;
    const bool interrupt_pending = atomic_exchange_explicit(
        &s_touch_interrupt_pending,
        false,
        memory_order_acq_rel);
    const bool read_requested =
        data_ready || touch_active || s_touch_rearm_retry_pending ||
        interrupt_pending;
    esp_err_t acquire_error = ESP_OK;
    esp_err_t read_error = ESP_OK;
    if (read_requested)
    {
        acquire_error = i2c_manager_acquire(
            pdMS_TO_TICKS(UI_TOUCH_READ_I2C_TIMEOUT_MS));
        read_error = acquire_error;
        if (acquire_error == ESP_OK)
        {
            read_error = cst9217_bsp_read_point(&point);
            i2c_manager_release();
        }
    }

    if (acquire_error != ESP_OK)
    {
        schedule_touch_rearm_retry();
        ESP_LOGD(TAG,
                 "触摸读点暂缓：共享 I2C 正忙，INT=%d，错误=0x%x",
                 interrupt_level,
                 (unsigned)acquire_error);
    }
    else if (!read_requested)
    {
        /* 无电平、边沿、活动序列或重试事实时不发起无效 I2C 读取。 */
    }
    else if (read_error == ESP_ERR_INVALID_RESPONSE && !data_ready)
    {
        clear_touch_rearm_retry();
        ESP_LOGD(TAG, "触摸暂无新数据：INT=%d", interrupt_level);
    }
    else if (read_error != ESP_OK)
    {
        schedule_touch_rearm_retry();
        if (s_touch_error_count < 50U)
        {
            ++s_touch_error_count;
        }
        if (s_touch_error_count == 1U)
        {
            report_power_runtime_result(WATCH_POWER_ERROR_TOUCH_FAILED);
            ESP_LOGE(TAG,
                     "CST9217 读点失败，INT=%d，错误=0x%x",
                     interrupt_level,
                     (unsigned)read_error);
        }
        if (s_touch_error_count >= 50U)
        {
            ESP_LOGW(TAG, "CST9217 连续读取失败 50 次，触发硬件复位恢复");
            esp_err_t reset_error = i2c_manager_acquire(
                pdMS_TO_TICKS(UI_TOUCH_SETUP_I2C_TIMEOUT_MS));
            if (reset_error == ESP_OK)
            {
                reset_error = cst9217_bsp_reset();
                i2c_manager_release();
            }
            if (reset_error == ESP_OK)
            {
                s_touch_error_count = 0U;
            }
            else
            {
                ESP_LOGE(TAG,
                         "CST9217 硬件复位恢复失败，错误=0x%x",
                         (unsigned)reset_error);
            }
        }
    }
    else
    {
        clear_touch_rearm_retry();
        if (s_touch_error_count != 0U)
        {
            ESP_LOGI(TAG,
                     "CST9217 读点恢复，连续错误=%lu",
                     (unsigned long)s_touch_error_count);
            report_power_runtime_result(WATCH_POWER_ERROR_NONE);
        }
        s_touch_error_count = 0U;
        sample = point.pressed ? UI_TOUCH_SAMPLE_PRESSED
                               : UI_TOUCH_SAMPLE_RELEASED;
    }

    const uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000U;
    const ui_touch_output_t output = ui_touch_session_feed(
        sample,
        (int16_t)point.x,
        (int16_t)point.y,
        now_ms);
    ESP_LOGD(TAG,
             "触摸原始帧：INT=%d，采样=%u，x=%u，y=%u，转发按压=%u",
             interrupt_level,
             (unsigned)sample,
             point.x,
             point.y,
             output.pressed ? 1U : 0U);

    bool wake_requested = false;
    const bool forward_pressed = ui_wake_touch_gate_filter(
        &s_wake_touch_gate,
        output.pressed,
        &wake_requested);
    if (output.sequence_started)
    {
        const esp_err_t activity_error =
            power_service_notify_local_activity(0);
        if (activity_error != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "本地用户活动通知 power_task 失败，错误=0x%x",
                     (unsigned)activity_error);
        }
    }
    if (wake_requested)
    {
        ESP_LOGI(TAG, "熄屏首触已消费并请求唤醒，本序列持续向 LVGL 报告释放");
    }
    if (forward_pressed)
    {
        data->point.x = output.x;
        data->point.y = output.y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    if (output.sequence_finished)
    {
        ui_touch_snapshot_t snapshot = {0};
        if (ui_touch_session_snapshot(&snapshot))
        {
            ESP_LOGD(
                TAG,
                "触摸序列结束：序号=%lu，起点=(%d,%d)，终点=(%d,%d)，"
                "位移=(%u,%u)，耗时=%lums，有效帧=%lu，未知帧=%lu，结果=%s",
                (unsigned long)snapshot.sequence_id,
                snapshot.start_x,
                snapshot.start_y,
                snapshot.end_x,
                snapshot.end_y,
                snapshot.max_delta_x,
                snapshot.max_delta_y,
                (unsigned long)snapshot.duration_ms,
                (unsigned long)snapshot.valid_sample_count,
                (unsigned long)snapshot.unknown_sample_count,
                snapshot.cancelled
                    ? "取消"
                    : (snapshot.dragged ? "拖动" : "短点"));
        }
    }
}

static void schedule_touch_rearm_retry(void)
{
    if (s_touch_rearm_retry_stage < 6U)
    {
        ++s_touch_rearm_retry_stage;
    }
    uint32_t delay_ms = UI_TOUCH_REARM_RETRY_BASE_MS;
    for (uint8_t stage = 1U;
         stage < s_touch_rearm_retry_stage &&
         delay_ms < UI_TOUCH_REARM_RETRY_MAX_MS;
         ++stage)
    {
        delay_ms = delay_ms > UI_TOUCH_REARM_RETRY_MAX_MS / 2U
                       ? UI_TOUCH_REARM_RETRY_MAX_MS
                       : delay_ms * 2U;
    }
    s_touch_rearm_retry_pending = true;
    s_touch_rearm_retry_at_ms =
        (uint64_t)esp_timer_get_time() / 1000U + delay_ms;
}

static void clear_touch_rearm_retry(void)
{
    s_touch_rearm_retry_pending = false;
    s_touch_rearm_retry_at_ms = 0U;
    s_touch_rearm_retry_stage = 0U;
}

static esp_err_t initialize_lvgl(void)
{
    lv_init();
    initialize_shell_round_insets();
    s_shell_fast_failure_logged = false;
    s_settings_fast_failure_logged = false;
    s_selftest_result_fast_failure_logged = false;
    atomic_store_explicit(
        &s_display_color_completion_generation,
        0U,
        memory_order_relaxed);
    ui_display_flush_guard_init(&s_lvgl_flush_guard);
    lv_disp_draw_buf_init(&s_draw_buffer,
                          s_buffer1,
                          s_buffer2,
                          UI_DRAW_BUFFER_PIXELS);
    lv_disp_drv_init(&s_display_driver);
    s_display_driver.hor_res = CO5300_BSP_WIDTH;
    s_display_driver.ver_res = CO5300_BSP_HEIGHT;
    s_display_driver.draw_buf = &s_draw_buffer;
    s_display_driver.rounder_cb = display_rounder;
    s_display_driver.flush_cb = display_flush;
    s_display_driver.wait_cb = display_wait;
    s_display = lv_disp_drv_register(&s_display_driver);
    if (s_display == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    lv_indev_drv_init(&s_input_driver);
    s_input_driver.type = LV_INDEV_TYPE_POINTER;
    s_input_driver.disp = s_display;
    s_input_driver.read_cb = touch_read;
    s_input_driver.gesture_limit = UI_TOUCH_GESTURE_LIMIT_PX;
    s_input = lv_indev_drv_register(&s_input_driver);
    if (s_input == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "LVGL display、input 与 timer handler 已由 ui_task 独占注册");
    return ESP_OK;
}

static void complete_pending_flush(TickType_t wait_ticks)
{
    if (!s_flush_pending || s_pending_driver == NULL)
    {
        return;
    }
    /*
     * 通知只用于唤醒，不能作为事务完成事实。快路颜色回调可能在双核
     * 下留下旧通知；只有提交后颜色代次真正变化才归还本笔 LVGL 缓冲。
     */
    (void)ulTaskNotifyTake(pdTRUE, wait_ticks);
    const uint32_t current_generation = atomic_load_explicit(
        &s_display_color_completion_generation,
        memory_order_relaxed);
    if (!ui_display_flush_guard_completed(
            &s_lvgl_flush_guard,
            current_generation))
    {
        return;
    }
    lv_disp_drv_t *driver = s_pending_driver;
    const bool final_flush = s_pending_flush_last;
    const uint64_t completed_us = (uint64_t)esp_timer_get_time();
    s_flush_pending = false;
    s_pending_driver = NULL;
    s_pending_flush_last = false;
    ui_display_flush_guard_clear(&s_lvgl_flush_guard);
    if (final_flush)
    {
        const uint64_t flush_us =
            completed_us >= s_frame_flush_started_us
                ? completed_us - s_frame_flush_started_us
                : 0U;
        if (s_frame_flush_active && s_frame_flush_valid &&
            s_frame_shell_motion_seen)
        {
            ui_performance_metrics_record_frame(
                &s_shell_performance_metrics,
                completed_us,
                flush_us > UINT32_MAX ? UINT32_MAX
                                      : (uint32_t)flush_us,
                s_frame_flush_pixels);
        }
        if (s_frame_flush_active && s_frame_flush_valid &&
            s_frame_settings_motion_seen)
        {
            ui_performance_metrics_record_frame(
                &s_settings_performance_metrics,
                completed_us,
                flush_us > UINT32_MAX ? UINT32_MAX
                                      : (uint32_t)flush_us,
                s_frame_flush_pixels);
        }
        if (s_frame_flush_active && s_frame_flush_valid &&
            s_frame_selftest_result_motion_seen)
        {
            ui_performance_metrics_record_frame(
                &s_selftest_result_performance_metrics,
                completed_us,
                flush_us > UINT32_MAX ? UINT32_MAX
                                      : (uint32_t)flush_us,
                s_frame_flush_pixels);
        }
        s_frame_flush_active = false;
        s_frame_shell_motion_seen = false;
        s_frame_settings_motion_seen = false;
        s_frame_selftest_result_motion_seen = false;
        s_frame_flush_valid = false;
        s_frame_flush_started_us = 0U;
        s_frame_flush_pixels = 0U;
    }
    lv_disp_flush_ready(driver);
}

static bool render_shell_fast_frame(void)
{
    const uint64_t started_us = (uint64_t)esp_timer_get_time();
    ui_runtime_shell_fast_frame_t frame = {0};
    if (!s_display_on || s_flush_pending ||
        !ui_runtime_binding_shell_fast_frame(&frame))
    {
        record_shell_fast_path_miss(started_us);
        return false;
    }

    ui_performance_metrics_set_motion(
        &s_shell_performance_metrics, true, started_us);
    (void)ulTaskNotifyTake(pdTRUE, 0);

    uint16_t first_y = 0U;
    uint16_t chunk_index = 0U;
    uint32_t pending_generation = 0U;
    while (first_y < CO5300_BSP_HEIGHT)
    {
        const uint16_t remaining_lines =
            (uint16_t)(CO5300_BSP_HEIGHT - first_y);
        const uint16_t line_count =
            remaining_lines > CO5300_BSP_DRAW_BUFFER_LINES
                ? CO5300_BSP_DRAW_BUFFER_LINES
                : remaining_lines;
        lv_color_t *destination =
            (chunk_index & 1U) == 0U ? s_buffer1 : s_buffer2;
        compose_shell_fast_chunk(destination,
                                 &frame,
                                 first_y,
                                 line_count);

        /*
         * 在上一块 DMA 发送期间先从 PSRAM 合成下一块；提交新窗口前
         * 再等待上一块完成，既形成流水线也不让参数事务重入驱动。
         */
        if (chunk_index > 0U &&
            !wait_for_fast_completion(pending_generation))
        {
            if (!s_shell_fast_failure_logged)
            {
                ESP_LOGE(TAG,
                         "常驻横滑硬件快路等待 DMA 超时：首行=%u",
                         (unsigned)first_y);
                s_shell_fast_failure_logged = true;
            }
            record_shell_fast_path_miss(
                (uint64_t)esp_timer_get_time());
            abort_hardware_fast_path(true);
            return false;
        }

        pending_generation = atomic_load_explicit(
            &s_display_color_completion_generation,
            memory_order_relaxed);
        const esp_err_t err = co5300_bsp_draw_bitmap(
            0,
            first_y,
            CO5300_BSP_WIDTH,
            first_y + line_count,
            destination);
        if (err != ESP_OK)
        {
            if (!s_shell_fast_failure_logged)
            {
                ESP_LOGE(TAG,
                         "常驻横滑硬件快路提交失败：错误=0x%x",
                         (unsigned)err);
                s_shell_fast_failure_logged = true;
            }
            record_shell_fast_path_miss(
                (uint64_t)esp_timer_get_time());
            abort_hardware_fast_path(false);
            return false;
        }
        first_y = (uint16_t)(first_y + line_count);
        ++chunk_index;
    }

    if (!wait_for_fast_completion(pending_generation))
    {
        if (!s_shell_fast_failure_logged)
        {
            ESP_LOGE(TAG,
                     "常驻横滑硬件快路等待末块 DMA 超时");
            s_shell_fast_failure_logged = true;
        }
        record_shell_fast_path_miss(
            (uint64_t)esp_timer_get_time());
        abort_hardware_fast_path(true);
        return false;
    }

    const uint64_t completed_us = (uint64_t)esp_timer_get_time();
    const uint64_t flush_us =
        completed_us >= started_us ? completed_us - started_us : 0U;
    ui_performance_metrics_record_frame(
        &s_shell_performance_metrics,
        completed_us,
        flush_us > UINT32_MAX ? UINT32_MAX : (uint32_t)flush_us,
        CO5300_BSP_WIDTH * CO5300_BSP_HEIGHT);
    return true;
}

static bool render_settings_fast_frame(void)
{
    const uint64_t started_us = (uint64_t)esp_timer_get_time();
    ui_runtime_settings_fast_frame_t frame = {0};
    if (!s_display_on || s_flush_pending ||
        !ui_runtime_binding_settings_fast_frame(&frame))
    {
        record_settings_fast_path_miss(started_us);
        return false;
    }

    ui_performance_metrics_set_motion(
        &s_settings_performance_metrics, true, started_us);
    (void)ulTaskNotifyTake(pdTRUE, 0);

    uint16_t first_y = 0U;
    uint16_t chunk_index = 0U;
    uint32_t pending_generation = 0U;
    while (first_y < CO5300_BSP_HEIGHT)
    {
        const uint16_t remaining_lines =
            (uint16_t)(CO5300_BSP_HEIGHT - first_y);
        const uint16_t line_count =
            remaining_lines > CO5300_BSP_DRAW_BUFFER_LINES
                ? CO5300_BSP_DRAW_BUFFER_LINES
                : remaining_lines;
        lv_color_t *destination =
            (chunk_index & 1U) == 0U ? s_buffer1 : s_buffer2;
        compose_settings_fast_chunk(destination,
                                    &frame,
                                    first_y,
                                    line_count);

        if (chunk_index > 0U &&
            !wait_for_fast_completion(pending_generation))
        {
            if (!s_settings_fast_failure_logged)
            {
                ESP_LOGE(TAG,
                         "设置纵滑硬件快路等待 DMA 超时：首行=%u",
                         (unsigned)first_y);
                s_settings_fast_failure_logged = true;
            }
            record_settings_fast_path_miss(
                (uint64_t)esp_timer_get_time());
            abort_hardware_fast_path(true);
            return false;
        }

        pending_generation = atomic_load_explicit(
            &s_display_color_completion_generation,
            memory_order_relaxed);
        const esp_err_t err = co5300_bsp_draw_bitmap(
            0,
            first_y,
            CO5300_BSP_WIDTH,
            first_y + line_count,
            destination);
        if (err != ESP_OK)
        {
            if (!s_settings_fast_failure_logged)
            {
                ESP_LOGE(TAG,
                         "设置纵滑硬件快路提交失败：错误=0x%x",
                         (unsigned)err);
                s_settings_fast_failure_logged = true;
            }
            record_settings_fast_path_miss(
                (uint64_t)esp_timer_get_time());
            abort_hardware_fast_path(false);
            return false;
        }
        first_y = (uint16_t)(first_y + line_count);
        ++chunk_index;
    }

    if (!wait_for_fast_completion(pending_generation))
    {
        if (!s_settings_fast_failure_logged)
        {
            ESP_LOGE(TAG,
                     "设置纵滑硬件快路等待末块 DMA 超时");
            s_settings_fast_failure_logged = true;
        }
        record_settings_fast_path_miss(
            (uint64_t)esp_timer_get_time());
        abort_hardware_fast_path(true);
        return false;
    }

    const uint64_t completed_us = (uint64_t)esp_timer_get_time();
    const uint64_t flush_us =
        completed_us >= started_us ? completed_us - started_us : 0U;
    ui_performance_metrics_record_frame(
        &s_settings_performance_metrics,
        completed_us,
        flush_us > UINT32_MAX ? UINT32_MAX : (uint32_t)flush_us,
        CO5300_BSP_WIDTH * CO5300_BSP_HEIGHT);
    return true;
}

static bool render_selftest_result_fast_frame(void)
{
    const uint64_t started_us = (uint64_t)esp_timer_get_time();
    ui_runtime_selftest_result_fast_frame_t frame = {0};
    if (!s_display_on || s_flush_pending ||
        !ui_runtime_binding_selftest_result_fast_frame(&frame))
    {
        record_selftest_result_fast_path_miss(started_us);
        return false;
    }

    ui_performance_metrics_set_motion(
        &s_selftest_result_performance_metrics, true, started_us);
    (void)ulTaskNotifyTake(pdTRUE, 0);

    uint16_t first_y = 0U;
    uint16_t chunk_index = 0U;
    uint32_t pending_generation = 0U;
    while (first_y < CO5300_BSP_HEIGHT)
    {
        const uint16_t remaining_lines =
            (uint16_t)(CO5300_BSP_HEIGHT - first_y);
        const uint16_t line_count =
            remaining_lines > CO5300_BSP_DRAW_BUFFER_LINES
                ? CO5300_BSP_DRAW_BUFFER_LINES
                : remaining_lines;
        lv_color_t *destination =
            (chunk_index & 1U) == 0U ? s_buffer1 : s_buffer2;
        compose_selftest_result_fast_chunk(destination,
                                           &frame,
                                           first_y,
                                           line_count);

        if (chunk_index > 0U &&
            !wait_for_fast_completion(pending_generation))
        {
            if (!s_selftest_result_fast_failure_logged)
            {
                ESP_LOGE(TAG,
                         "自检结果纵滑硬件快路等待 DMA 超时：首行=%u",
                         (unsigned)first_y);
                s_selftest_result_fast_failure_logged = true;
            }
            record_selftest_result_fast_path_miss(
                (uint64_t)esp_timer_get_time());
            abort_hardware_fast_path(true);
            return false;
        }

        pending_generation = atomic_load_explicit(
            &s_display_color_completion_generation,
            memory_order_relaxed);
        const esp_err_t err = co5300_bsp_draw_bitmap(
            0,
            first_y,
            CO5300_BSP_WIDTH,
            first_y + line_count,
            destination);
        if (err != ESP_OK)
        {
            if (!s_selftest_result_fast_failure_logged)
            {
                ESP_LOGE(TAG,
                         "自检结果纵滑硬件快路提交失败：错误=0x%x",
                         (unsigned)err);
                s_selftest_result_fast_failure_logged = true;
            }
            record_selftest_result_fast_path_miss(
                (uint64_t)esp_timer_get_time());
            abort_hardware_fast_path(false);
            return false;
        }
        first_y = (uint16_t)(first_y + line_count);
        ++chunk_index;
    }

    if (!wait_for_fast_completion(pending_generation))
    {
        if (!s_selftest_result_fast_failure_logged)
        {
            ESP_LOGE(TAG,
                     "自检结果纵滑硬件快路等待末块 DMA 超时");
            s_selftest_result_fast_failure_logged = true;
        }
        record_selftest_result_fast_path_miss(
            (uint64_t)esp_timer_get_time());
        abort_hardware_fast_path(true);
        return false;
    }

    const uint64_t completed_us = (uint64_t)esp_timer_get_time();
    const uint64_t flush_us =
        completed_us >= started_us ? completed_us - started_us : 0U;
    ui_performance_metrics_record_frame(
        &s_selftest_result_performance_metrics,
        completed_us,
        flush_us > UINT32_MAX ? UINT32_MAX : (uint32_t)flush_us,
        CO5300_BSP_WIDTH * CO5300_BSP_HEIGHT);
    return true;
}

static void record_shell_fast_path_miss(uint64_t now_us)
{
    if (!ui_runtime_binding_shell_motion_active())
    {
        return;
    }
    ui_performance_metrics_set_motion(
        &s_shell_performance_metrics, true, now_us);
    ui_performance_metrics_record_fast_path_miss(
        &s_shell_performance_metrics);
}

static void record_settings_fast_path_miss(uint64_t now_us)
{
    if (!ui_runtime_binding_settings_motion_active())
    {
        return;
    }
    ui_performance_metrics_set_motion(
        &s_settings_performance_metrics, true, now_us);
    ui_performance_metrics_record_fast_path_miss(
        &s_settings_performance_metrics);
}

static void record_selftest_result_fast_path_miss(uint64_t now_us)
{
    if (!ui_runtime_binding_selftest_result_motion_active())
    {
        return;
    }
    ui_performance_metrics_set_motion(
        &s_selftest_result_performance_metrics, true, now_us);
    ui_performance_metrics_record_fast_path_miss(
        &s_selftest_result_performance_metrics);
}

static void abort_hardware_fast_path(bool dma_state_unknown)
{
    ui_runtime_binding_abort_fast_path();
    if (dma_state_unknown && reset_display_touch(true) != ESP_OK)
    {
        ESP_LOGE(TAG, "硬件快路异常后的显示恢复失败，保留整屏重绘请求");
        ui_runtime_binding_request_full_redraw();
    }
}

static bool wait_for_fast_completion(uint32_t generation)
{
    const int64_t deadline_us =
        esp_timer_get_time() +
        (int64_t)UI_FAST_CHUNK_TIMEOUT_MS * 1000;
    while (atomic_load_explicit(
               &s_display_color_completion_generation,
               memory_order_relaxed) == generation)
    {
        const int64_t now_us = esp_timer_get_time();
        if (now_us >= deadline_us)
        {
            return false;
        }
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));
    }
    /*
     * 本地有界 panel IO 会在下一笔参数事务前有限回收上一笔颜色事务；
     * 通知只负责唤醒，代次匹配后清掉本笔通知，避免跨路径污染。
     */
    (void)ulTaskNotifyTake(pdTRUE, 0);
    return true;
}

static void compose_shell_fast_chunk(
    lv_color_t *destination,
    const ui_runtime_shell_fast_frame_t *frame,
    uint16_t first_y,
    uint16_t line_count)
{
    if (destination == NULL || frame == NULL ||
        frame->current_pixels == NULL ||
        frame->left_pixels == NULL ||
        frame->right_pixels == NULL)
    {
        return;
    }
    const lv_color_t *current =
        (const lv_color_t *)frame->current_pixels;
    const lv_color_t *left =
        (const lv_color_t *)frame->left_pixels;
    const lv_color_t *right =
        (const lv_color_t *)frame->right_pixels;
    const int32_t offset_px = frame->offset_px;

    for (uint16_t line = 0U; line < line_count; ++line)
    {
        const uint16_t y = (uint16_t)(first_y + line);
        lv_color_t *output =
            destination + (size_t)line * CO5300_BSP_WIDTH;
        const size_t source_offset =
            (size_t)y * CO5300_BSP_WIDTH;
        if (offset_px >= 0)
        {
            const size_t right_width = (size_t)offset_px;
            const size_t current_width =
                CO5300_BSP_WIDTH - right_width;
            memcpy(output,
                   current + source_offset + right_width,
                   current_width * sizeof(lv_color_t));
            memcpy(output + current_width,
                   right + source_offset,
                   right_width * sizeof(lv_color_t));
        }
        else
        {
            const size_t left_width = (size_t)(-offset_px);
            const size_t current_width =
                CO5300_BSP_WIDTH - left_width;
            memcpy(output,
                   left + source_offset + current_width,
                   left_width * sizeof(lv_color_t));
            memcpy(output + left_width,
                   current + source_offset,
                   current_width * sizeof(lv_color_t));
        }

        const size_t inset = s_shell_round_insets[y];
        if (inset > 0U)
        {
            memset(output, 0, inset * sizeof(lv_color_t));
            memset(output + CO5300_BSP_WIDTH - inset,
                   0,
                   inset * sizeof(lv_color_t));
        }
    }
}

static void compose_settings_fast_chunk(
    lv_color_t *destination,
    const ui_runtime_settings_fast_frame_t *frame,
    uint16_t first_y,
    uint16_t line_count)
{
    if (destination == NULL || frame == NULL ||
        frame->background_pixels == NULL ||
        frame->content_pixels == NULL)
    {
        return;
    }
    const lv_color_t *background =
        (const lv_color_t *)frame->background_pixels;
    const lv_color_t *content =
        (const lv_color_t *)frame->content_pixels;
    const lv_color_t thumb_color = {
        .full = frame->thumb_color,
    };

    for (uint16_t line = 0U; line < line_count; ++line)
    {
        const uint16_t y = (uint16_t)(first_y + line);
        lv_color_t *output =
            destination + (size_t)line * CO5300_BSP_WIDTH;
        const lv_color_t *background_line =
            background + (size_t)y * CO5300_BSP_WIDTH;
        if (y >= frame->viewport_y &&
            y < frame->viewport_y + frame->viewport_height)
        {
            const uint16_t content_y = (uint16_t)(
                frame->scroll_y + y - frame->viewport_y);
            if (content_y < frame->content_height)
            {
                const uint16_t right_x = (uint16_t)(
                    frame->viewport_x + frame->viewport_width);
                memcpy(output,
                       background_line,
                       frame->viewport_x * sizeof(lv_color_t));
                memcpy(output + frame->viewport_x,
                       content +
                           (size_t)content_y * frame->content_width,
                       frame->viewport_width * sizeof(lv_color_t));
                memcpy(output + right_x,
                       background_line + right_x,
                       (CO5300_BSP_WIDTH - right_x) *
                           sizeof(lv_color_t));
            }
        }
        else
        {
            memcpy(output,
                   background_line,
                   CO5300_BSP_WIDTH * sizeof(lv_color_t));
        }

        if (y >= frame->thumb_y &&
            y < frame->thumb_y + frame->thumb_height)
        {
            const uint16_t thumb_row =
                (uint16_t)(y - frame->thumb_y);
            uint16_t first_x = frame->thumb_x;
            uint16_t last_x =
                (uint16_t)(frame->thumb_x +
                           frame->thumb_width);
            if ((thumb_row == 0U ||
                 thumb_row + 1U == frame->thumb_height) &&
                frame->thumb_width > 2U)
            {
                ++first_x;
                --last_x;
            }
            for (uint16_t x = first_x; x < last_x; ++x)
            {
                output[x] = thumb_color;
            }
        }
    }
}

static void compose_selftest_result_fast_chunk(
    lv_color_t *destination,
    const ui_runtime_selftest_result_fast_frame_t *frame,
    uint16_t first_y,
    uint16_t line_count)
{
    if (destination == NULL || frame == NULL ||
        frame->background_pixels == NULL ||
        frame->content_pixels == NULL)
    {
        return;
    }
    const lv_color_t *background =
        (const lv_color_t *)frame->background_pixels;
    const lv_color_t *content =
        (const lv_color_t *)frame->content_pixels;
    const lv_color_t thumb_color = {
        .full = frame->thumb_color,
    };

    for (uint16_t line = 0U; line < line_count; ++line)
    {
        const uint16_t y = (uint16_t)(first_y + line);
        lv_color_t *output =
            destination + (size_t)line * CO5300_BSP_WIDTH;
        const lv_color_t *background_line =
            background + (size_t)y * CO5300_BSP_WIDTH;
        if (y >= frame->viewport_y &&
            y < frame->viewport_y + frame->viewport_height)
        {
            const uint16_t content_y = (uint16_t)(
                frame->scroll_y + y - frame->viewport_y);
            if (content_y < frame->content_height)
            {
                const uint16_t right_x = (uint16_t)(
                    frame->viewport_x + frame->viewport_width);
                memcpy(output,
                       background_line,
                       frame->viewport_x * sizeof(lv_color_t));
                memcpy(output + frame->viewport_x,
                       content +
                           (size_t)content_y * frame->content_width,
                       frame->viewport_width * sizeof(lv_color_t));
                memcpy(output + right_x,
                       background_line + right_x,
                       (CO5300_BSP_WIDTH - right_x) *
                           sizeof(lv_color_t));
            }
        }
        else
        {
            memcpy(output,
                   background_line,
                   CO5300_BSP_WIDTH * sizeof(lv_color_t));
        }

        if (y >= frame->thumb_y &&
            y < frame->thumb_y + frame->thumb_height)
        {
            const uint16_t thumb_row =
                (uint16_t)(y - frame->thumb_y);
            uint16_t first_x = frame->thumb_x;
            uint16_t last_x =
                (uint16_t)(frame->thumb_x +
                           frame->thumb_width);
            if ((thumb_row == 0U ||
                 thumb_row + 1U == frame->thumb_height) &&
                frame->thumb_width > 2U)
            {
                ++first_x;
                --last_x;
            }
            for (uint16_t x = first_x; x < last_x; ++x)
            {
                output[x] = thumb_color;
            }
        }
    }
}

static void initialize_shell_round_insets(void)
{
    const int32_t doubled_radius =
        UI_SHELL_ROUND_RADIUS_PX * 2;
    const int32_t radius_squared =
        doubled_radius * doubled_radius;
    for (uint16_t y = 0U; y < CO5300_BSP_HEIGHT; ++y)
    {
        int32_t doubled_dy = 0;
        if (y < UI_SHELL_ROUND_RADIUS_PX)
        {
            doubled_dy =
                doubled_radius - ((int32_t)y * 2 + 1);
        }
        else if (y >=
                 CO5300_BSP_HEIGHT -
                     UI_SHELL_ROUND_RADIUS_PX)
        {
            doubled_dy =
                ((int32_t)y * 2 + 1) -
                2 * (CO5300_BSP_HEIGHT -
                     UI_SHELL_ROUND_RADIUS_PX);
        }
        else
        {
            s_shell_round_insets[y] = 0U;
            continue;
        }

        uint16_t inset = 0U;
        while (inset < UI_SHELL_ROUND_RADIUS_PX)
        {
            const int32_t doubled_dx =
                doubled_radius - ((int32_t)inset * 2 + 1);
            if (doubled_dx * doubled_dx +
                    doubled_dy * doubled_dy <=
                radius_squared)
            {
                break;
            }
            ++inset;
        }
        s_shell_round_insets[y] = inset;
    }
}

static bool wait_for_pending_flush(uint32_t timeout_ms)
{
    const int64_t deadline_us =
        esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (s_flush_pending)
    {
        complete_pending_flush(pdMS_TO_TICKS(UI_FLUSH_WAIT_MS));
        if (s_flush_pending && esp_timer_get_time() >= deadline_us)
        {
            return false;
        }
    }
    return true;
}

static bool refresh_full_screen_once(void)
{
    lv_obj_t *screen = lv_scr_act();
    if (screen == NULL || s_display == NULL)
    {
        return false;
    }
    s_full_redraw_flush_error = false;
    lv_obj_invalidate(screen);
    lv_refr_now(s_display);
    return wait_for_pending_flush(UI_FULL_REDRAW_FLUSH_TIMEOUT_MS) &&
           !s_full_redraw_flush_error;
}

static bool refresh_full_screen_with_recovery(void)
{
    if (!wait_for_pending_flush(UI_FULL_REDRAW_FLUSH_TIMEOUT_MS))
    {
        ESP_LOGE(TAG, "整屏重绘等待旧 DMA flush 超时，执行显示恢复");
        if (reset_display_touch(true) != ESP_OK)
        {
            ESP_LOGE(TAG, "显示恢复失败，整屏重绘未完成");
            return false;
        }
    }

    ESP_LOGD(TAG, "执行一次性同步整屏重绘");
    if (refresh_full_screen_once())
    {
        return true;
    }

    const bool final_flush_timed_out = s_flush_pending;
    if (final_flush_timed_out)
    {
        ESP_LOGE(TAG, "整屏重绘等待最终 DMA flush 超时，执行显示恢复并重试");
    }
    else
    {
        ESP_LOGE(TAG, "整屏重绘提交失败，执行显示恢复并重试");
    }
    if (reset_display_touch(final_flush_timed_out) != ESP_OK)
    {
        ESP_LOGE(TAG, "显示恢复失败，整屏重绘未完成");
        return false;
    }
    if (refresh_full_screen_once())
    {
        return true;
    }

    const bool retry_flush_timed_out = s_flush_pending;
    if (retry_flush_timed_out)
    {
        ESP_LOGE(TAG, "显示恢复后的整屏重绘仍等待 DMA flush 超时");
    }
    else
    {
        ESP_LOGE(TAG, "显示恢复后的整屏重绘仍提交失败");
    }
    if (reset_display_touch(retry_flush_timed_out) != ESP_OK)
    {
        ESP_LOGE(TAG, "第二次显示恢复失败，整屏重绘未完成");
    }
    return false;
}

static void consume_full_redraw_request(void)
{
    if (!ui_runtime_binding_take_full_redraw_request())
    {
        return;
    }
    if (!refresh_full_screen_with_recovery())
    {
        ui_runtime_binding_request_full_redraw();
    }
}

static void complete_flush_after_display_reset(void)
{
    lv_disp_drv_t *driver = s_pending_driver;
    s_flush_pending = false;
    s_pending_driver = NULL;
    s_pending_flush_last = false;
    ui_display_flush_guard_clear(&s_lvgl_flush_guard);
    s_frame_flush_active = false;
    s_frame_shell_motion_seen = false;
    s_frame_settings_motion_seen = false;
    s_frame_selftest_result_motion_seen = false;
    s_frame_flush_valid = false;
    s_frame_flush_started_us = 0U;
    s_frame_flush_pixels = 0U;
    if (driver != NULL)
    {
        lv_disp_flush_ready(driver);
    }
    (void)ulTaskNotifyTake(pdTRUE, 0);
}

static void sync_performance_motion_and_report(uint64_t now_us)
{
    const bool shell_motion_active =
        ui_runtime_binding_shell_motion_active();
    ui_performance_metrics_set_motion(
        &s_shell_performance_metrics,
        shell_motion_active,
        now_us);
    ui_runtime_shell_snapshot_stats_t shell_snapshot_stats = {0};
    ui_runtime_binding_shell_snapshot_stats(&shell_snapshot_stats);
    if (shell_snapshot_stats.prepare_failures !=
        s_shell_prepare_failures_observed)
    {
        if (!shell_motion_active)
        {
            ui_performance_metrics_set_motion(
                &s_shell_performance_metrics, true, now_us);
        }
        ui_performance_metrics_record_fast_path_miss(
            &s_shell_performance_metrics);
        if (!shell_motion_active)
        {
            ui_performance_metrics_set_motion(
                &s_shell_performance_metrics, false, now_us);
        }
        s_shell_prepare_failures_observed =
            shell_snapshot_stats.prepare_failures;
    }
    ui_performance_report_t shell_report = {0};
    if (ui_performance_metrics_take_report(
            &s_shell_performance_metrics, now_us, &shell_report))
    {
        log_shell_performance_report(&shell_report);
    }
    const bool settings_motion_active =
        ui_runtime_binding_settings_motion_active();
    ui_performance_metrics_set_motion(
        &s_settings_performance_metrics,
        settings_motion_active,
        now_us);
    ui_runtime_settings_snapshot_stats_t snapshot_stats = {0};
    ui_runtime_binding_settings_snapshot_stats(&snapshot_stats);
    if (snapshot_stats.prepare_failures !=
        s_settings_prepare_failures_observed)
    {
        /*
         * SCROLL_BEGIN 与 SCROLL_END 可能在同一轮 LVGL handler 内完成。
         * 即使服务循环未观察到运动态，也必须把准备失败汇入本周期门禁。
         */
        if (!settings_motion_active)
        {
            ui_performance_metrics_set_motion(
                &s_settings_performance_metrics, true, now_us);
        }
        ui_performance_metrics_record_fast_path_miss(
            &s_settings_performance_metrics);
        if (!settings_motion_active)
        {
            ui_performance_metrics_set_motion(
                &s_settings_performance_metrics, false, now_us);
        }
        s_settings_prepare_failures_observed =
            snapshot_stats.prepare_failures;
    }
    ui_performance_report_t settings_report = {0};
    if (ui_performance_metrics_take_report(
            &s_settings_performance_metrics, now_us, &settings_report))
    {
        log_settings_performance_report(&settings_report);
    }

    const bool selftest_result_motion_active =
        ui_runtime_binding_selftest_result_motion_active();
    ui_performance_metrics_set_motion(
        &s_selftest_result_performance_metrics,
        selftest_result_motion_active,
        now_us);
    ui_runtime_selftest_result_snapshot_stats_t
        selftest_result_snapshot_stats = {0};
    ui_runtime_binding_selftest_result_snapshot_stats(
        &selftest_result_snapshot_stats);
    if (selftest_result_snapshot_stats.prepare_failures !=
        s_selftest_result_prepare_failures_observed)
    {
        if (!selftest_result_motion_active)
        {
            ui_performance_metrics_set_motion(
                &s_selftest_result_performance_metrics, true, now_us);
        }
        ui_performance_metrics_record_fast_path_miss(
            &s_selftest_result_performance_metrics);
        if (!selftest_result_motion_active)
        {
            ui_performance_metrics_set_motion(
                &s_selftest_result_performance_metrics, false, now_us);
        }
        s_selftest_result_prepare_failures_observed =
            selftest_result_snapshot_stats.prepare_failures;
    }
    ui_performance_report_t selftest_result_report = {0};
    if (ui_performance_metrics_take_report(
            &s_selftest_result_performance_metrics,
            now_us,
            &selftest_result_report))
    {
        log_selftest_result_performance_report(
            &selftest_result_report);
    }
}

static void log_shell_performance_report(
    const ui_performance_report_t *report)
{
    if (report == NULL)
    {
        return;
    }
    const uint64_t milli_fps =
        report->active_duration_us == 0U
            ? 0U
            : (uint64_t)report->complete_frames * 1000000000ULL /
                  report->active_duration_us;
    ui_runtime_shell_snapshot_stats_t snapshot_stats = {0};
    ui_runtime_binding_shell_snapshot_stats(&snapshot_stats);
    ESP_LOGI(
        TAG,
        "常驻横滑性能汇总：有效=%llums，完整帧=%lu，平均=%llu.%03llu FPS，"
        "一秒窗口最低=%lu，P99间隔=%lu.%01lums，最大间隔=%lu.%01lums，"
        "渲染平均/最大=%lu/%luus，flush平均/最大=%lu/%luus，"
        "像素=%llu，起滑切换最近/最大=%lu/%lums，"
        "预热单页最大=%lums，本周期快路缺失=%lu，"
        "快照起滑未就绪(启动累计)=%lu，"
        "视觉无变化刷新(启动累计)=%lu，"
        "精确失效页(启动累计)=%lu，当前所需待预热页=%u，"
        "路径=常驻快照直传，结果=%s",
        (unsigned long long)(report->active_duration_us / 1000U),
        (unsigned long)report->complete_frames,
        (unsigned long long)(milli_fps / 1000U),
        (unsigned long long)(milli_fps % 1000U),
        (unsigned long)report->min_window_frames,
        (unsigned long)(report->p99_interval_us / 1000U),
        (unsigned long)((report->p99_interval_us % 1000U) / 100U),
        (unsigned long)(report->max_interval_us / 1000U),
        (unsigned long)((report->max_interval_us % 1000U) / 100U),
        (unsigned long)report->average_render_us,
        (unsigned long)report->max_render_us,
        (unsigned long)report->average_flush_us,
        (unsigned long)report->max_flush_us,
        (unsigned long long)report->transferred_pixels,
        (unsigned long)snapshot_stats.last_prepare_ms,
        (unsigned long)snapshot_stats.max_prepare_ms,
        (unsigned long)snapshot_stats.max_warm_page_ms,
        (unsigned long)report->fast_path_misses,
        (unsigned long)snapshot_stats.prepare_failures,
        (unsigned long)snapshot_stats.ignored_refresh_count,
        (unsigned long)snapshot_stats.invalidated_page_count,
        (unsigned)snapshot_stats.dirty_page_count,
        report->passed && snapshot_stats.available &&
                snapshot_stats.max_prepare_ms <= 100U
            ? "通过"
            : "未通过");
}

static void log_settings_performance_report(
    const ui_performance_report_t *report)
{
    if (report == NULL)
    {
        return;
    }
    const uint64_t milli_fps =
        report->active_duration_us == 0U
            ? 0U
            : (uint64_t)report->complete_frames * 1000000000ULL /
                  report->active_duration_us;
    ui_runtime_settings_snapshot_stats_t snapshot_stats = {0};
    ui_runtime_binding_settings_snapshot_stats(&snapshot_stats);
    ESP_LOGI(
        TAG,
        "设置纵滑性能汇总：有效=%llums，完整帧=%lu，平均=%llu.%03llu FPS，"
        "一秒窗口最低=%lu，P99间隔=%lu.%01lums，最大间隔=%lu.%01lums，"
        "渲染平均/最大=%lu/%luus，flush平均/最大=%lu/%luus，"
        "像素=%llu，起滑切换最近/最大=%lu/%lums，"
        "预热单次最大=%lums，周期快路缺失=%lu，"
        "起滑未就绪累计=%lu，内容重建/切换=%lu/%lu，"
        "视觉代次前台/目标=%lu/%lu，结果=%s",
        (unsigned long long)(report->active_duration_us / 1000U),
        (unsigned long)report->complete_frames,
        (unsigned long long)(milli_fps / 1000U),
        (unsigned long long)(milli_fps % 1000U),
        (unsigned long)report->min_window_frames,
        (unsigned long)(report->p99_interval_us / 1000U),
        (unsigned long)((report->p99_interval_us % 1000U) / 100U),
        (unsigned long)(report->max_interval_us / 1000U),
        (unsigned long)((report->max_interval_us % 1000U) / 100U),
        (unsigned long)report->average_render_us,
        (unsigned long)report->max_render_us,
        (unsigned long)report->average_flush_us,
        (unsigned long)report->max_flush_us,
        (unsigned long long)report->transferred_pixels,
        (unsigned long)snapshot_stats.last_prepare_ms,
        (unsigned long)snapshot_stats.max_prepare_ms,
        (unsigned long)snapshot_stats.max_warm_ms,
        (unsigned long)report->fast_path_misses,
        (unsigned long)snapshot_stats.prepare_failures,
        (unsigned long)snapshot_stats.content_rebuild_count,
        (unsigned long)snapshot_stats.content_swap_count,
        (unsigned long)snapshot_stats.front_generation,
        (unsigned long)snapshot_stats.target_generation,
        report->passed && snapshot_stats.available &&
                snapshot_stats.max_prepare_ms <= 100U
            ? "通过"
            : "未通过");
}

static void log_selftest_result_performance_report(
    const ui_performance_report_t *report)
{
    if (report == NULL)
    {
        return;
    }
    const uint64_t milli_fps =
        report->active_duration_us == 0U
            ? 0U
            : (uint64_t)report->complete_frames * 1000000000ULL /
                  report->active_duration_us;
    ui_runtime_selftest_result_snapshot_stats_t snapshot_stats = {0};
    ui_runtime_binding_selftest_result_snapshot_stats(&snapshot_stats);
    ESP_LOGI(
        TAG,
        "自检结果纵滑性能汇总：有效=%llums，完整帧=%lu，"
        "平均=%llu.%03llu FPS，一秒窗口最低=%lu，"
        "P99间隔=%lu.%01lums，最大间隔=%lu.%01lums，"
        "渲染平均/最大=%lu/%luus，flush平均/最大=%lu/%luus，"
        "像素=%llu，起滑切换最近/最大=%lu/%lums，"
        "预热单次最大=%lums，周期快路缺失=%lu，"
        "起滑未就绪累计=%lu，内容重建/切换=%lu/%lu，"
        "视觉代次前台/目标=%lu/%lu，结果=%s",
        (unsigned long long)(report->active_duration_us / 1000U),
        (unsigned long)report->complete_frames,
        (unsigned long long)(milli_fps / 1000U),
        (unsigned long long)(milli_fps % 1000U),
        (unsigned long)report->min_window_frames,
        (unsigned long)(report->p99_interval_us / 1000U),
        (unsigned long)((report->p99_interval_us % 1000U) / 100U),
        (unsigned long)(report->max_interval_us / 1000U),
        (unsigned long)((report->max_interval_us % 1000U) / 100U),
        (unsigned long)report->average_render_us,
        (unsigned long)report->max_render_us,
        (unsigned long)report->average_flush_us,
        (unsigned long)report->max_flush_us,
        (unsigned long long)report->transferred_pixels,
        (unsigned long)snapshot_stats.last_prepare_ms,
        (unsigned long)snapshot_stats.max_prepare_ms,
        (unsigned long)snapshot_stats.max_warm_ms,
        (unsigned long)report->fast_path_misses,
        (unsigned long)snapshot_stats.prepare_failures,
        (unsigned long)snapshot_stats.content_rebuild_count,
        (unsigned long)snapshot_stats.content_swap_count,
        (unsigned long)snapshot_stats.front_generation,
        (unsigned long)snapshot_stats.target_generation,
        report->passed && snapshot_stats.available &&
                snapshot_stats.max_prepare_ms <= 100U
            ? "通过"
            : "未通过");
}

static bool is_manual_selftest_item(selftest_item_id_t item_id)
{
    return item_id == SELFTEST_ITEM_AMOLED || item_id == SELFTEST_ITEM_TOUCH ||
           item_id == SELFTEST_ITEM_AUDIO ||
           item_id == SELFTEST_ITEM_VIBRATION;
}

static void handle_request(const ui_service_request_t *request)
{
    esp_err_t err = ESP_OK;
    switch (request->type)
    {
    case UI_SERVICE_REQUEST_MODEL_UPDATE:
    case UI_SERVICE_REQUEST_MAINTENANCE_CANDIDATES:
        if (ui_runtime_binding_motion_active())
        {
            atomic_store(&s_pending_model_refresh, true);
        }
        else if (!refresh_runtime_model())
        {
            err = ESP_ERR_INVALID_STATE;
            atomic_store(&s_pending_model_refresh, true);
        }
        break;
    case UI_SERVICE_REQUEST_WAKE:
    {
        watch_power_snapshot_t power = {0};
        const bool display_was_off = !s_display_on;
        err = watch_state_power_snapshot(&power, 0);
        if (err == ESP_OK && display_was_off)
        {
            s_selftest_display_retry_suppressed = false;
            err = resume_display_with_single_retry();
            if (err == ESP_OK)
            {
                s_display_flush_enabled = true;
                s_brightness_valid = false;
            }
            else if (power.selftest_active)
            {
                s_selftest_display_retry_suppressed = true;
            }
        }
        if (err == ESP_OK)
        {
            err = apply_runtime_brightness(power.power_level,
                                           power.selftest_active,
                                           true);
            if (err != ESP_OK && power.selftest_active)
            {
                s_selftest_display_retry_suppressed = true;
            }
        }
        if (err == ESP_OK && !refresh_runtime_model())
        {
            err = ESP_ERR_INVALID_STATE;
        }
        if (err == ESP_OK && display_was_off && !s_display_on)
        {
            err = reveal_display_after_full_redraw();
        }
        report_power_runtime_result(
            err == ESP_OK ? WATCH_POWER_ERROR_NONE
                          : WATCH_POWER_ERROR_DISPLAY_FAILED);
        break;
    }
    case UI_SERVICE_REQUEST_SLEEP:
    {
        watch_state_snapshot_t watch = {0};
        err = watch_state_snapshot(&watch, 0);
        if (err == ESP_OK &&
            (watch.selftest.state == SELFTEST_RUN_RUNNING ||
             watch.selftest.retry_active))
        {
            const bool display_was_off = !s_display_on;
            err = resume_display_with_single_retry();
            if (err == ESP_OK)
            {
                if (display_was_off)
                {
                    s_display_flush_enabled = true;
                    s_brightness_valid = false;
                }
                err = apply_runtime_brightness(watch.power_level,
                                               true,
                                               display_was_off);
                if (err == ESP_OK && display_was_off)
                {
                    err = reveal_display_after_full_redraw();
                }
                s_selftest_display_forced_on = err == ESP_OK;
                s_selftest_display_retry_suppressed = err != ESP_OK;
            }
            else
            {
                s_selftest_display_retry_suppressed = true;
            }
        }
        else if (err == ESP_OK)
        {
            s_selftest_display_forced_on = false;
            s_selftest_display_retry_suppressed = false;
            if (!wait_for_pending_flush(UI_FULL_REDRAW_FLUSH_TIMEOUT_MS))
            {
                err = ESP_ERR_TIMEOUT;
            }
            else
            {
                err = suspend_display_with_single_retry();
            }
            if (err == ESP_OK)
            {
                s_display_on = false;
                s_display_flush_enabled = false;
                s_brightness_valid = false;
                ui_wake_touch_gate_set_display(&s_wake_touch_gate, false);
                ui_runtime_binding_set_display_on(false);
            }
        }
        report_power_runtime_result(
            err == ESP_OK ? WATCH_POWER_ERROR_NONE
                          : WATCH_POWER_ERROR_DISPLAY_FAILED);
        break;
    }
    case UI_SERVICE_REQUEST_NEXT_RESIDENT_PAGE:
    {
        watch_power_snapshot_t power = {0};
        const esp_err_t snapshot_error = watch_state_power_snapshot(
            &power,
            pdMS_TO_TICKS(UI_EVENT_SNAPSHOT_TIMEOUT_MS));
        if (snapshot_error != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "PWR 常驻页切换无法读取最新自检事实，已安全忽略，错误=0x%x",
                     (unsigned)snapshot_error);
        }
        else if (s_display_on && !power.selftest_active)
        {
            const ui_runtime_intent_t intent = {
                .type = UI_INTENT_SHELL_NEXT,
                .source = UI_RUNTIME_INTENT_SOURCE_POWER_KEY,
            };
            handle_ui_intent(&intent, NULL);
        }
        break;
    }
    case UI_SERVICE_REQUEST_DISPLAY_RESET:
        err = reset_display_touch(false);
        break;
    case UI_SERVICE_REQUEST_SELFTEST_ITEM:
        if (request->run_id == 0U || !is_manual_selftest_item(request->item_id))
        {
            err = ESP_ERR_INVALID_ARG;
        }
        else
        {
            ui_runtime_model_t model = {0};
            if (build_runtime_model(&model, 0) != ESP_OK ||
                !model.selftest_running ||
                model.selftest_run_id != request->run_id ||
                model.selftest_item != request->item_id)
            {
                err = ESP_ERR_INVALID_STATE;
            }
            else
            {
                s_manual_prompt_started_ms =
                    (uint64_t)esp_timer_get_time() / 1000U;
                s_manual_prompt_run_id = request->run_id;
                s_manual_prompt_item = request->item_id;
                s_manual_countdown_second = UINT32_MAX;
                if (request->item_id == SELFTEST_ITEM_AMOLED &&
                    !ui_runtime_binding_start_amoled_test())
                {
                    err = ESP_ERR_INVALID_STATE;
                }
            }
            if (err == ESP_OK && !refresh_runtime_model())
            {
                err = ESP_ERR_INVALID_STATE;
            }
        }
        break;
    case UI_SERVICE_REQUEST_SELFTEST_REFRESH:
        if (request->run_id == 0U)
        {
            err = ESP_ERR_INVALID_ARG;
        }
        else if (!refresh_runtime_model())
        {
            err = ESP_ERR_INVALID_STATE;
        }
        break;
    case UI_SERVICE_REQUEST_STOP:
        s_running = false;
        break;
    default:
        err = ESP_ERR_INVALID_ARG;
        break;
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "处理 UI 请求 %d 失败，错误=0x%x",
                 request->type,
                 (unsigned)err);
    }
}

static esp_err_t build_runtime_model(ui_runtime_model_t *model,
                                     TickType_t snapshot_timeout)
{
    if (model == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(model, 0, sizeof(*model));

    const TickType_t snapshot_started_tick = xTaskGetTickCount();
    watch_state_snapshot_t watch = {0};
    esp_err_t err = watch_state_snapshot(&watch, snapshot_timeout);
    if (err != ESP_OK)
    {
        return err;
    }
    TickType_t maintenance_timeout = snapshot_timeout;
    if (snapshot_timeout != portMAX_DELAY)
    {
        const TickType_t elapsed_ticks =
            xTaskGetTickCount() - snapshot_started_tick;
        maintenance_timeout =
            elapsed_ticks >= snapshot_timeout
                ? 0U
                : snapshot_timeout - elapsed_ticks;
    }
    maintenance_ble_ui_snapshot_t maintenance = {0};
    err = maintenance_service_ble_ui_snapshot(
        &maintenance, maintenance_timeout);
    if (err != ESP_OK)
    {
        return err;
    }
#if LEGBOT_CAP_MODEM
    const bool cellular_connected =
        watch_modem_network_connected(&watch.modem);
#else
    const bool cellular_connected = false;
#endif
    model->snapshot_valid = true;
    model->binding_present = watch.has_bound_exoskeleton;
    model->ble_connected = watch.ble_connected;
    model->unlock_session_valid = watch.unlock_session_valid;
    model->cellular_connected = cellular_connected;
    model->gps_fixed =
#if LEGBOT_CAP_GPS_TIME
        watch.gps.status == WATCH_GPS_STATUS_FIXED;
#else
        false;
#endif
    model->selftest_running = watch.selftest.state == SELFTEST_RUN_RUNNING;
#if LEGBOT_CAP_GPS_TIME
    model->gps_action_enabled =
        watch.gps.manual_request_available &&
        !model->selftest_running;
#else
    model->gps_action_enabled = false;
#endif
    model->cellular_action_enabled =
        ui_service_cellular_action_available(
            cellular_connected,
            model->selftest_running,
            watch.modem.lifecycle);
#if LEGBOT_CAP_MODEM
    model->cellular_connecting =
        !cellular_connected &&
        !model->selftest_running &&
        !model->cellular_action_enabled;
#else
    model->cellular_connecting = false;
#endif
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    control_ui_model_t control = {0};
    err = control_ui_model_from_snapshot(&watch, true, now_ms, &control);
    if (err != ESP_OK)
    {
        return err;
    }

#if SCENIC_AREA_MANAGEMENT_DEBUG
    if (s_payment_link_generation != watch.ble_link_generation)
    {
        s_payment_link_generation = watch.ble_link_generation;
        s_payment_verification_attempted = false;
        s_payment_verification_pending = false;
        s_payment_intent_sequence = 0U;
    }
    const bool payment_verification_succeeded =
        watch.rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
        watch.unlock_session_valid;
    const bool current_control_generation =
        watch.control_link_generation != 0U &&
        watch.control_link_generation == watch.ble_link_generation;
    const bool authorization_needed =
        model->binding_present && !watch.unlock_session_valid;
    const bool authorization_transport_ready =
        watch.ble_connected && watch.gatt_ready && watch.notify_ready &&
        watch.device_state_available && watch.dataReady &&
        control.actual_available && watch.status_fresh &&
        watch.last_status_rx_ms != 0U &&
        now_ms >= watch.last_status_rx_ms &&
        now_ms - watch.last_status_rx_ms < STATUS_STALE_MS &&
        watch.link_control_ready;
    if (!model->binding_present || payment_verification_succeeded)
    {
        s_payment_verification_attempted = false;
        s_payment_verification_pending = false;
        s_payment_intent_sequence = 0U;
    }
    ui_payment_projection_t payment = {0};
    ui_payment_projection_build(authorization_needed && watch.ble_connected,
                                current_control_generation,
                                authorization_transport_ready,
                                s_payment_verification_attempted,
                                s_payment_verification_pending,
                                watch.rental_phase,
                                watch.last_rental_result,
                                &payment);
    model->authorization_display = payment.display;
    model->authorization_pending = payment.authorization_pending;
    model->payment_required = payment.payment_required;
    model->payment_attempted = payment.payment_attempted;
    model->payment_pending = payment.payment_pending;
    model->payment_failed = payment.payment_failed;
    model->payment_action_enabled = payment.payment_action_enabled;
#else
    s_payment_verification_attempted = false;
    s_payment_verification_pending = false;
    s_payment_intent_sequence = 0U;
    model->authorization_pending = false;
    model->authorization_display = UI_AUTHORIZATION_DISPLAY_NONE;
    model->payment_required = false;
    model->payment_attempted = false;
    model->payment_pending = false;
    model->payment_failed = false;
    model->payment_action_enabled = false;
#endif
    model->control_block_reason = control.block_reason;
    model->poweroff_committed = control.poweroff_committed;
    model->controls_enabled =
        control.controls_enabled &&
        !model->selftest_running &&
        !model->payment_required;
    model->power_action_enabled = model->controls_enabled;
    model->haptics_enabled = s_haptics_enabled;
    model->click_audio_enabled = s_click_audio_enabled;
    model->raise_to_wake_enabled = s_raise_to_wake_enabled;
    model->brightness_level =
        (ui_runtime_brightness_t)s_brightness_level;
    model->screen_timeout_seconds =
        s_screen_timeout == CONFIG_SERVICE_UI_SCREEN_TIMEOUT_15S
            ? UI_RUNTIME_SCREEN_TIMEOUT_15S
            : s_screen_timeout == CONFIG_SERVICE_UI_SCREEN_TIMEOUT_30S
                  ? UI_RUNTIME_SCREEN_TIMEOUT_30S
                  : UI_RUNTIME_SCREEN_TIMEOUT_5S;
    model->power_level = watch.power_level;
    model->watch_battery =
        watch.battery_has_valid_sample ? watch.battery_percent : 0U;
    model->scene_config = watch.exoskeleton_model;
    model->scene_config = watch.exoskeleton_state.scene_config;
    model->selftest_results = watch.selftest;
    model->selftest_run_id = watch.selftest.run_id;
    model->selftest_item = watch.selftest.current_item;
    model->retry_running = watch.selftest.retry_active;
    model->leg_positions_available =
        watch.ble_connected &&
        watch.device_state_available &&
        watch.dataReady &&
        watch.status_fresh &&
        watch.last_status_rx_ms != 0U &&
        now_ms >= watch.last_status_rx_ms &&
        now_ms - watch.last_status_rx_ms < STATUS_STALE_MS;
    if (model->leg_positions_available)
    {
        model->left_leg_position =
            watch.exoskeleton_state.left_motor_position;
        model->right_leg_position =
            watch.exoskeleton_state.right_motor_position;
    }
    if (s_manual_reply_submitted &&
        (!model->selftest_running ||
         model->selftest_run_id != s_manual_reply_run_id ||
         model->selftest_item != s_manual_reply_item))
    {
        s_manual_reply_submitted = false;
        s_manual_reply_run_id = 0U;
        s_manual_reply_item = SELFTEST_ITEM_COUNT;
    }
    model->manual_reply_pending = s_manual_reply_submitted;
    ui_selftest_summary_t selftest_summary = {0};
    if (!ui_selftest_summary_build(&watch.selftest, &selftest_summary))
    {
        return ESP_ERR_INVALID_STATE;
    }
    model->selftest_completed_categories = selftest_summary.completed_count;
    model->selftest_progress =
        (uint8_t)(((uint32_t)selftest_summary.completed_count * 100U) /
                  UI_SELFTEST_CATEGORY_COUNT);
    copy_text(model->selftest_item_text,
              sizeof(model->selftest_item_text),
              selftest_item_text(model->selftest_item));
    copy_text(model->manual_prompt,
              sizeof(model->manual_prompt),
              selftest_prompt(model->selftest_item));
    if (!model->selftest_running ||
        model->selftest_run_id != s_manual_prompt_run_id ||
        model->selftest_item != s_manual_prompt_item)
    {
        s_manual_prompt_started_ms = 0U;
        s_manual_prompt_run_id = 0U;
        s_manual_prompt_item = SELFTEST_ITEM_COUNT;
        s_manual_countdown_second = UINT32_MAX;
    }
    uint32_t remaining_seconds = 30U;
    if (s_manual_prompt_started_ms != 0U &&
        now_ms >= s_manual_prompt_started_ms)
    {
        const uint64_t elapsed_ms = now_ms - s_manual_prompt_started_ms;
        remaining_seconds = elapsed_ms >= 30000U
                                ? 0U
                                : (uint32_t)((30000U - elapsed_ms + 999U) /
                                             1000U);
    }
    (void)snprintf(model->manual_countdown,
                   sizeof(model->manual_countdown),
                   "%lu",
                   (unsigned long)remaining_seconds);

    model->control_update_sequence = watch.control_update_sequence;
    if (control.actual_available)
    {
        model->assist_enabled = control.actual_motor_enable != 0U;
        model->gear = control.actual_gear;
        model->drive_mode = control.actual_scene_mode;
        model->exo_battery = watch.exoskeleton_state.battery_level <= 100U
                                 ? watch.exoskeleton_state.battery_level
                                 : 0U;
        if (isfinite(watch.exoskeleton_state.steps) &&
            watch.exoskeleton_state.steps >= 0.0F &&
            watch.exoskeleton_state.steps <= 4294967295.0F)
        {
            model->steps = (uint32_t)watch.exoskeleton_state.steps;
        }
        (void)snprintf(model->last_snapshot,
                       sizeof(model->last_snapshot),
                       "%u档 %u%% %lu步",
                       model->gear,
                       model->exo_battery,
                       (unsigned long)model->steps);
    }
    else
    {
        copy_text(model->last_snapshot,
                  sizeof(model->last_snapshot),
                  "暂无有效状态");
    }
    if (model->poweroff_committed)
    {
        clear_control_projections();
    }
    else
    {
        if (watch.pending_control &&
            watch.control_kind == WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY &&
            watch.control_request_id != 0U &&
            watch.control_request_id != s_boundary_projection_request_id &&
            (watch.control_target == 10U || watch.control_target == 11U))
        {
            /* 触摸与语音统一以已受理事务真值建立最长五秒双字段投影。 */
            s_boundary_projection_request_id = watch.control_request_id;
            s_gear_projection = (ui_control_projection_lock_t){
                .active = true,
                .target = watch.control_target,
                .expires_ms = now_ms + UI_CONTROL_PROJECTION_LOCK_MS,
            };
            s_mode_projection = (ui_control_projection_lock_t){
                .active = true,
                .target = watch.control_target == 11U ? 3U : 1U,
                .expires_ms = now_ms + UI_CONTROL_PROJECTION_LOCK_MS,
            };
        }
        apply_control_projection(model, watch.ble_connected, now_ms);
    }

#if LEGBOT_CAP_GPS_TIME
    bool clock_valid = false;
    int64_t utc_ms = 0;
    if (watch.time_synchronized && now_ms <= (uint64_t)INT64_MAX)
    {
        const int64_t monotonic_ms = (int64_t)now_ms;
        if (!((watch.time_offset_ms > 0 &&
               monotonic_ms > INT64_MAX - watch.time_offset_ms) ||
              (watch.time_offset_ms < 0 &&
               monotonic_ms < INT64_MIN - watch.time_offset_ms)))
        {
            utc_ms = monotonic_ms + watch.time_offset_ms;
            clock_valid = utc_ms >= 0 && utc_ms <= INT64_MAX - 28800000LL;
        }
    }
    if (clock_valid)
    {
        const time_t local_seconds =
            (time_t)((utc_ms + 28800000LL) / 1000LL);
        struct tm local_time = {0};
        if (gmtime_r(&local_seconds, &local_time) != NULL)
        {
            (void)snprintf(model->time,
                           sizeof(model->time),
                           "%02d:%02d",
                           local_time.tm_hour,
                           local_time.tm_min);
            const uint64_t until_next_minute =
                60000U - (uint64_t)(utc_ms % 60000LL);
            s_next_clock_refresh_ms =
                now_ms <= UINT64_MAX - until_next_minute
                    ? now_ms + until_next_minute
                    : UINT64_MAX;
        }
        else
        {
            clock_valid = false;
        }
    }
    if (!clock_valid)
    {
        copy_text(model->time, sizeof(model->time), "--:--");
        s_next_clock_refresh_ms = UINT64_MAX;
    }
#else
    copy_text(model->time, sizeof(model->time), "--:--");
    s_next_clock_refresh_ms = UINT64_MAX;
#endif

    copy_text(model->device_model,
              sizeof(model->device_model),
              "Legbot Watch");
    const esp_app_desc_t *app_description = esp_app_get_description();
    copy_text(model->firmware_version,
              sizeof(model->firmware_version),
              app_description->version[0] != '\0'
                  ? app_description->version
                  : "--");
    copy_text(model->serial_number,
              sizeof(model->serial_number),
              watch.watch_id[0] != '\0' ? watch.watch_id : "--");
    copy_text(model->binding_id,
              sizeof(model->binding_id),
              !model->binding_present
                  ? "未绑定"
                  : watch.bound_exoskeleton_mac[0] != '\0'
                        ? watch.bound_exoskeleton_mac
                        : "--");
#if LEGBOT_CAP_MODEM
    const char *cellular_status =
        cellular_connected
            ? "已联网"
            : model->selftest_running
                  ? "自检中"
                  : model->cellular_action_enabled
                        ? "点击连接"
                        : "连接中";
#else
    const char *cellular_status = "不可用";
#endif
    copy_text(model->cellular_status,
              sizeof(model->cellular_status),
              cellular_status);
#if LEGBOT_CAP_GPS_TIME
    const char *gps_status = "点击搜星";
    bool gps_success = false;
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    if (watch.gps.status == WATCH_GPS_STATUS_FIXED)
    {
        gps_status = "已定位";
        gps_success = true;
    }
#else
    if (watch.time_synchronized)
    {
        gps_status = "已授时";
        gps_success = true;
    }
#endif
    if (watch.gps.acquisition_state == WATCH_GPS_ACQUISITION_SEARCHING)
    {
        gps_status = "搜星中";
    }
    else if (watch.gps.acquisition_state == WATCH_GPS_ACQUISITION_UNAVAILABLE)
    {
        gps_status = "不可用";
    }
    else if (!gps_success &&
             !model->gps_action_enabled &&
             watch.gps.manual_cooldown_deadline_ms > now_ms)
    {
        gps_status = "冷却中";
    }
#else
    const char *gps_status = "不可用";
#endif
    copy_text(model->gps_status,
              sizeof(model->gps_status),
              gps_status);
    if (watch.selftest.state != SELFTEST_RUN_IDLE)
    {
        s_previous_selftest_incomplete = false;
    }
    const char *selftest_status = "尚未运行";
    if (watch.selftest.state == SELFTEST_RUN_RUNNING)
    {
        selftest_status =
            is_manual_selftest_item(watch.selftest.current_item)
                ? "待确认"
                : "运行中";
    }
    else if (watch.selftest.state == SELFTEST_RUN_FINISHED)
    {
        selftest_status =
            watch.selftest.fail_count == 0U &&
                    watch.selftest.inconclusive_count > 0U
                ? "需要复测"
                : "查看结果";
    }
    else if (s_previous_selftest_incomplete)
    {
        selftest_status = "上次未完成";
    }
    copy_text(model->selftest_status,
              sizeof(model->selftest_status),
              selftest_status);
    if (model->selftest_running)
    {
        s_gps_submit_state = UI_RUNTIME_GPS_SUBMIT_IDLE;
    }
    model->gps_submit_state = s_gps_submit_state;
    model->clear_binding_pending =
        s_clear_binding_submitted ||
        watch.ble_transaction == WATCH_BLE_TRANSACTION_CLEARING;
    if (!watch.has_bound_exoskeleton)
    {
        s_clear_binding_submitted = false;
        s_clear_binding_submission_refresh_pending = false;
        s_clear_binding_failed = false;
        model->clear_binding_pending = false;
    }
    else if (s_clear_binding_submission_refresh_pending)
    {
        /* 提交后的同步刷新仍可能读到 owner 接单前的旧 ERROR。 */
        s_clear_binding_submission_refresh_pending = false;
    }
    else if (s_clear_binding_submitted &&
             watch.ble_transaction == WATCH_BLE_TRANSACTION_ERROR)
    {
        s_clear_binding_submitted = false;
        s_clear_binding_failed = true;
        model->clear_binding_pending = false;
    }
    model->clear_binding_failed = s_clear_binding_failed;
    log_runtime_model_transitions(&watch, model);

    if (control.actual_current && watch.exoskeleton_state.temp_alarm != 0U)
    {
        append_alert(model,
                     UI_RUNTIME_ALERT_TEMPERATURE,
                     true,
                     "温度异常",
                     "过高");
    }
    if (watch.exoskeleton_low_battery &&
        watch.device_state_available &&
        watch.status_fresh &&
        control.actual_current)
    {
        char alert_value[12] = {0};
        (void)snprintf(alert_value,
                       sizeof(alert_value),
                       "%u%%",
                       model->exo_battery);
        append_alert(model,
                     UI_RUNTIME_ALERT_EXO_BATTERY,
                     false,
                     "外骨骼低电",
                     alert_value);
    }
    if (watch.battery_has_valid_sample &&
        watch.power_level ==
            WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY)
    {
        char alert_value[12] = {0};
        (void)snprintf(alert_value,
                       sizeof(alert_value),
                       "%u%%",
                       model->watch_battery);
        append_alert(model,
                     UI_RUNTIME_ALERT_WATCH_SEVERE,
                     true,
                     "手环严重低电",
                     alert_value);
    }
    else if (watch.battery_has_valid_sample &&
             watch.power_level == WATCH_POWER_LEVEL_LOW_BATTERY_WARN)
    {
        char alert_value[12] = {0};
        (void)snprintf(alert_value,
                       sizeof(alert_value),
                       "%u%%",
                       model->watch_battery);
        append_alert(model,
                     UI_RUNTIME_ALERT_WATCH_BATTERY,
                     false,
                     "手环低电",
                     alert_value);
    }
    if (!watch.audio_resource_available ||
        (watch.audio_error != WATCH_AUDIO_ERROR_NONE &&
         watch.audio_error != WATCH_AUDIO_ERROR_NOT_READY &&
         watch.audio_error != WATCH_AUDIO_ERROR_BUSY))
    {
        append_alert(model,
                     UI_RUNTIME_ALERT_AUDIO,
                     false,
                     "音频资源异常",
                     "请检查");
    }
#if SCENIC_AREA_MANAGEMENT_DEBUG
    if (!cellular_connected)
    {
        append_alert(model,
                     UI_RUNTIME_ALERT_CELLULAR,
                     false,
                     "4G 未就绪",
                     "未联网");
    }
    if (watch.gps.status != WATCH_GPS_STATUS_FIXED)
    {
        append_alert(model,
                     UI_RUNTIME_ALERT_GPS,
                     false,
                     "GPS 未定位",
                     watch.gps.status == WATCH_GPS_STATUS_SEARCHING
                         ? "搜索中"
                         : "未定位");
    }
#endif

    model->ble_scan_state =
        watch.ble_transaction == WATCH_BLE_TRANSACTION_SCANNING
            ? UI_RUNTIME_BLE_SCAN_ACTIVE
            : UI_RUNTIME_BLE_SCAN_IDLE;
    model->ble_candidate_generation = maintenance.ble_candidates.generation;
    model->ble_candidate_count =
        maintenance.ble_candidates.count < UI_RUNTIME_BLE_CANDIDATE_COUNT
            ? maintenance.ble_candidates.count
            : UI_RUNTIME_BLE_CANDIDATE_COUNT;
    for (uint8_t index = 0U; index < model->ble_candidate_count; ++index)
    {
        const maintenance_ble_candidate_t *candidate =
            &maintenance.ble_candidates.candidates[index];
        copy_text(model->ble_candidates[index].mac,
                  sizeof(model->ble_candidates[index].mac),
                  candidate->mac);
        copy_text(model->ble_candidates[index].name,
                  sizeof(model->ble_candidates[index].name),
                  is_printable_ascii_text(candidate->advertised_name)
                      ? candidate->advertised_name
                      : candidate->mac);
        model->ble_candidates[index].rssi = candidate->rssi;
        if (strcmp(candidate->mac,
                   maintenance.ble_selection_feedback.mac) == 0)
        {
            model->selected_ble_candidate = index;
            if (maintenance.ble_selection_feedback.result ==
                MAINTENANCE_BLE_SELECTION_CONNECTING)
            {
                model->ble_connection_state =
                    UI_RUNTIME_BLE_CONNECTION_CONNECTING;
            }
            else if (maintenance.ble_selection_feedback.result ==
                         MAINTENANCE_BLE_SELECTION_ERROR ||
                     maintenance.ble_selection_feedback.result ==
                         MAINTENANCE_BLE_SELECTION_STALE)
            {
                model->ble_connection_state =
                    UI_RUNTIME_BLE_CONNECTION_FAILED;
            }
        }
    }
    return ESP_OK;
}

static bool refresh_runtime_model(void)
{
    ui_runtime_model_t model = {0};
    if (build_runtime_model(&model, 0) != ESP_OK)
    {
        return false;
    }
    const bool selftest_active =
        model.selftest_running || model.retry_running;
    bool selftest_display_started = false;
    bool selftest_display_was_off = false;
    if (selftest_active && !s_selftest_display_forced_on &&
        !s_selftest_display_retry_suppressed)
    {
        selftest_display_was_off = !s_display_on;
        const esp_err_t display_error =
            resume_display_with_single_retry();
        if (display_error != ESP_OK)
        {
            s_selftest_display_retry_suppressed = true;
            report_power_runtime_result(
                WATCH_POWER_ERROR_DISPLAY_FAILED);
            ESP_LOGE(TAG,
                     "整机自检进入运行时恢复 AMOLED 失败，错误=0x%x",
                     (unsigned)display_error);
            return false;
        }
        if (selftest_display_was_off)
        {
            s_display_flush_enabled = true;
            s_brightness_valid = false;
        }
        const esp_err_t brightness_error = apply_runtime_brightness(
            model.power_level,
            true,
            selftest_display_was_off);
        if (brightness_error != ESP_OK)
        {
            s_selftest_display_retry_suppressed = true;
            report_power_runtime_result(
                WATCH_POWER_ERROR_DISPLAY_FAILED);
            ESP_LOGE(TAG,
                     "整机自检恢复 AMOLED 亮度失败，错误=0x%x",
                     (unsigned)brightness_error);
            return false;
        }
        selftest_display_started = true;
    }
    else if (!selftest_active)
    {
        /* 终态仅解除熄屏抑制，后续显式睡眠请求再执行普通熄屏策略。 */
        s_selftest_display_forced_on = false;
        s_selftest_display_retry_suppressed = false;
    }
    if (!selftest_active &&
        apply_runtime_brightness(model.power_level,
                                 false,
                                 false) != ESP_OK)
    {
        report_power_runtime_result(WATCH_POWER_ERROR_DISPLAY_FAILED);
        ESP_LOGE(TAG, "按手环电量等级切换 AMOLED 亮度失败");
        return false;
    }
    if (!render_runtime_model(&model))
    {
        return false;
    }
    if (selftest_display_started && selftest_display_was_off)
    {
        const esp_err_t reveal_error = reveal_display_after_full_redraw();
        if (reveal_error != ESP_OK)
        {
            s_selftest_display_retry_suppressed = true;
            report_power_runtime_result(WATCH_POWER_ERROR_DISPLAY_FAILED);
            ESP_LOGE(TAG,
                     "整机自检首帧完整提交后点亮 AMOLED 失败，错误=0x%x",
                     (unsigned)reveal_error);
            return false;
        }
    }
    if (selftest_display_started)
    {
        s_selftest_display_forced_on = true;
        s_selftest_display_retry_suppressed = false;
        report_power_runtime_result(WATCH_POWER_ERROR_NONE);
    }
    sync_ble_binding_scan_session();
    return true;
}

static bool render_runtime_model(const ui_runtime_model_t *model)
{
    if (model == NULL || !ui_runtime_binding_render(model))
    {
        return false;
    }
    s_last_rendered_model = *model;
    s_last_rendered_model_valid = true;
    return true;
}

static void sync_ble_binding_scan_session(void)
{
    const bool page_active =
        ui_runtime_binding_ble_candidate_page_active();
    if (page_active != s_ble_page_idle_timeout_active)
    {
        const esp_err_t power_error =
            power_service_set_ble_page_active(page_active, 0);
        if (power_error == ESP_OK)
        {
            s_ble_page_idle_timeout_active = page_active;
            ESP_LOGI(TAG,
                     "BLE 候选页固定 30 秒熄屏已随页面%s",
                     page_active ? "启用" : "停用");
        }
        else
        {
            ESP_LOGW(TAG,
                     "BLE 候选页固定 30 秒熄屏%s请求入队失败，错误=0x%x",
                     page_active ? "启用" : "停用",
                     (unsigned)power_error);
        }
    }
    if (page_active == s_ble_binding_scan_session_requested)
    {
        return;
    }

    const esp_err_t error =
        page_active
            ? ble_service_begin_binding_scan_session(0)
            : ble_service_end_binding_scan_session(0);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 候选页扫描会话%s请求入队失败，错误=0x%x",
                 page_active ? "开始" : "结束",
                 (unsigned)error);
        return;
    }
    s_ble_binding_scan_session_requested = page_active;
    ESP_LOGI(TAG,
             "BLE 候选页扫描会话已随页面%s",
             page_active ? "开启" : "关闭");
}

static void handle_ui_intent(const ui_runtime_intent_t *intent, void *context)
{
    (void)context;
    if (intent == NULL)
    {
        return;
    }
    const bool local_shell_navigation =
        is_local_shell_navigation_intent(intent->type);
    ui_runtime_model_t model = {0};
    if (local_shell_navigation && s_last_rendered_model_valid)
    {
        model = s_last_rendered_model;
    }
    else if (build_runtime_model(
                 &model,
                 pdMS_TO_TICKS(UI_EVENT_SNAPSHOT_TIMEOUT_MS)) != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "用户事件在 %u ms 内无法读取最新事实，拒绝副作用 intent=%d",
                 UI_EVENT_SNAPSHOT_TIMEOUT_MS,
                 (int)intent->type);
        return;
    }
    if (is_control_intent(intent->type))
    {
        log_control_click(intent, &model);
    }
    else if (intent->type == UI_INTENT_VERIFY_PAYMENT)
    {
        ESP_LOGI(TAG,
                 "支付验证按钮点击：可提交=%u，处理中=%u",
                 model.payment_action_enabled ? 1U : 0U,
                 model.payment_pending ? 1U : 0U);
    }
    else if (intent->type == UI_INTENT_CONNECT_CELLULAR)
    {
        ESP_LOGI(TAG,
                 "维护页 4G 网络点击：可提交=%u，连接中=%u",
                 model.cellular_action_enabled ? 1U : 0U,
                 model.cellular_connecting ? 1U : 0U);
    }
    else if (intent->type == UI_INTENT_REQUEST_GPS_ACQUISITION)
    {
        ESP_LOGI(TAG,
                 "维护页 GPS 搜星点击：可提交=%u，已定位=%u",
                 model.gps_action_enabled ? 1U : 0U,
                 model.gps_fixed ? 1U : 0U);
    }

    const ui_runtime_intent_disposition_t disposition =
        ui_runtime_binding_accept_intent(intent, &model);
    if (intent->type == UI_INTENT_SELFTEST_TOUCH_PROGRESS &&
        disposition == UI_RUNTIME_INTENT_HANDLED_LOCAL)
    {
        /*
         * PRESSING 热路径已在 binding 内只更新 fill、thumb 和通过门；
         * 不再让每个采样重复静态 patch、完整模型投影及二次事实刷新。
         */
        return;
    }
    if (is_control_intent(intent->type))
    {
        if (disposition == UI_RUNTIME_INTENT_SUBMIT_SERVICE)
        {
            ESP_LOGI(TAG,
                     "UI 控制门禁通过：类型=%s",
                     ui_intent_name(intent->type, &model));
        }
        else
        {
            ESP_LOGW(TAG,
                     "UI 控制门禁拒绝：类型=%s，原因=%s，内部门禁=%s",
                     ui_intent_name(intent->type, &model),
                     control_rejection_reason(intent, &model),
                     control_block_reason_name(model.control_block_reason));
        }
    }
    else if (intent->type == UI_INTENT_VERIFY_PAYMENT &&
             disposition == UI_RUNTIME_INTENT_REJECTED)
    {
        ESP_LOGW(TAG,
                 "支付验证提交被拒绝：链路或状态尚未满足授权前置条件");
    }
    const bool button_accepted = ui_service_should_request_feedback(
        intent->source == UI_RUNTIME_INTENT_SOURCE_BUTTON,
        disposition != UI_RUNTIME_INTENT_REJECTED);
    const bool preference_action =
        intent->type == UI_INTENT_TOGGLE_HAPTICS ||
        intent->type == UI_INTENT_TOGGLE_CLICK_AUDIO ||
        intent->type == UI_INTENT_TOGGLE_RAISE_WAKE ||
        intent->type == UI_INTENT_SET_BRIGHTNESS ||
        intent->type == UI_INTENT_SET_SCREEN_TIMEOUT;
    if (button_accepted && !preference_action)
    {
        request_ui_feedback(model.haptics_enabled,
                            model.click_audio_enabled);
    }

    if (!render_runtime_model(&model))
    {
        ESP_LOGW(TAG, "typed intent 路由未能使用当前事实完成渲染");
    }
    sync_ble_binding_scan_session();
    esp_err_t err = ESP_OK;
    if (disposition == UI_RUNTIME_INTENT_SUBMIT_SERVICE)
    {
        switch (intent->type)
        {
        case UI_INTENT_GEAR_DECREMENT:
        case UI_INTENT_GEAR_INCREMENT:
        case UI_INTENT_TOGGLE_ASSIST:
        case UI_INTENT_SET_MODE_STANDARD:
        case UI_INTENT_SET_MODE_EXTREME:
        case UI_INTENT_SET_MODE_SPORT:
        case UI_INTENT_SET_MODE_ECO:
        case UI_INTENT_EXO_POWEROFF:
            err = submit_control_intent(intent, &model);
            if (err == ESP_OK)
            {
                if (intent->type == UI_INTENT_EXO_POWEROFF)
                {
                    clear_control_projections();
                }
                else
                {
                    lock_control_projection(
                        intent,
                        &model,
                        (uint64_t)esp_timer_get_time() / 1000U);
                }
            }
            break;
        case UI_INTENT_VERIFY_PAYMENT:
        {
            s_payment_verification_attempted = true;
            s_payment_intent_sequence = 0U;
            uint32_t intent_sequence = 0U;
            err = ble_service_request_first_unlock(&intent_sequence, 0);
            if (err == ESP_OK)
            {
                ESP_LOGI(TAG, "首次云授权请求已进入 BLE owner 队列");
                s_payment_verification_pending = true;
                s_payment_intent_sequence = intent_sequence;
            }
            else
            {
                ESP_LOGW(TAG,
                         "首次云授权请求入队失败，错误=0x%x",
                         (unsigned)err);
                s_payment_verification_pending = false;
                s_payment_intent_sequence = 0U;
            }
            break;
        }
        case UI_INTENT_CONNECT_CELLULAR:
        {
            const maintenance_result_t result =
                maintenance_service_request_modem_connect();
            err = result == MAINTENANCE_RESULT_OK
                      ? ESP_OK
                      : result == MAINTENANCE_RESULT_BUSY
                            ? ESP_ERR_TIMEOUT
                            : ESP_FAIL;
            if (err != ESP_OK)
            {
                ESP_LOGW(TAG,
                         "维护页 4G 手动联网请求被拒绝，结果=%u",
                         (unsigned)result);
            }
            break;
        }
        case UI_INTENT_REQUEST_GPS_ACQUISITION:
        {
            const maintenance_result_t result =
                maintenance_service_request_gps_acquisition();
            err = result == MAINTENANCE_RESULT_OK
                      ? ESP_OK
                      : result == MAINTENANCE_RESULT_BUSY
                            ? ESP_ERR_TIMEOUT
                            : ESP_FAIL;
            if (err != ESP_OK)
            {
                ESP_LOGW(TAG,
                         "维护页 GPS 手动搜星请求被拒绝，结果=%u",
                         (unsigned)result);
            }
            break;
        }
        case UI_INTENT_TOGGLE_HAPTICS:
        {
            config_service_ui_preferences_t preferences =
                current_ui_preferences();
            preferences.haptics_enabled = !s_haptics_enabled;
            err = config_service_ui_preferences_write(
                &preferences,
                pdMS_TO_TICKS(20));
            if (err == ESP_OK)
            {
                s_haptics_enabled = preferences.haptics_enabled;
            }
            break;
        }
        case UI_INTENT_TOGGLE_CLICK_AUDIO:
        {
            config_service_ui_preferences_t preferences =
                current_ui_preferences();
            preferences.click_audio_enabled = !s_click_audio_enabled;
            err = config_service_ui_preferences_write(
                &preferences,
                pdMS_TO_TICKS(20));
            if (err == ESP_OK)
            {
                s_click_audio_enabled = preferences.click_audio_enabled;
            }
            break;
        }
        case UI_INTENT_TOGGLE_RAISE_WAKE:
        {
            config_service_ui_preferences_t preferences =
                current_ui_preferences();
            preferences.raise_to_wake_enabled =
                !s_raise_to_wake_enabled;
            err = config_service_ui_preferences_write(
                &preferences,
                pdMS_TO_TICKS(20));
            if (err == ESP_OK)
            {
                s_raise_to_wake_enabled =
                    preferences.raise_to_wake_enabled;
                err = power_service_set_raise_to_wake_enabled(
                    s_raise_to_wake_enabled,
                    pdMS_TO_TICKS(20));
            }
            break;
        }
        case UI_INTENT_SET_BRIGHTNESS:
        {
            const config_service_ui_brightness_t target =
                (config_service_ui_brightness_t)intent->value;
            const bool selftest_active =
                model.selftest_running || model.retry_running;
            const uint16_t old_effective =
                power_display_brightness_for_user(
                    model.power_level,
                    selftest_active,
                    brightness_raw_for_level(s_brightness_level));
            const uint16_t target_effective =
                power_display_brightness_for_user(
                    model.power_level,
                    selftest_active,
                    brightness_raw_for_level(target));
            err = co5300_bsp_set_brightness(target_effective);
            if (err == ESP_OK)
            {
                s_brightness_valid = true;
                s_brightness_value = target_effective;
                config_service_ui_preferences_t preferences =
                    current_ui_preferences();
                preferences.brightness = target;
                err = config_service_ui_preferences_write(
                    &preferences,
                    pdMS_TO_TICKS(20));
                if (err == ESP_OK)
                {
                    s_brightness_level = target;
                }
                else
                {
                    const esp_err_t rollback_error =
                        co5300_bsp_set_brightness(old_effective);
                    if (rollback_error == ESP_OK)
                    {
                        s_brightness_value = old_effective;
                    }
                    else
                    {
                        s_brightness_valid = false;
                        ESP_LOGE(TAG,
                                 "亮度偏好持久化失败后的硬件回滚也失败，错误=0x%x",
                                 (unsigned)rollback_error);
                    }
                }
            }
            else
            {
                s_brightness_valid = false;
            }
            break;
        }
        case UI_INTENT_SET_SCREEN_TIMEOUT:
        {
            const config_service_ui_screen_timeout_t target =
                intent->value == UI_RUNTIME_SCREEN_TIMEOUT_5S
                    ? CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S
                    : intent->value == UI_RUNTIME_SCREEN_TIMEOUT_30S
                          ? CONFIG_SERVICE_UI_SCREEN_TIMEOUT_30S
                          : CONFIG_SERVICE_UI_SCREEN_TIMEOUT_15S;
            config_service_ui_preferences_t preferences =
                current_ui_preferences();
            preferences.screen_timeout = target;
            err = config_service_ui_preferences_write(
                &preferences,
                pdMS_TO_TICKS(20));
            if (err == ESP_OK)
            {
                s_screen_timeout = target;
                err = power_service_set_idle_timeout_ms(
                    timeout_ms_for_preference(target),
                    0);
            }
            break;
        }
        case UI_INTENT_CONFIRM_CLEAR_BINDING:
            if (maintenance_service_confirm_clear_ble_binding(0) ==
                    MAINTENANCE_RESULT_OK &&
                ble_service_end_binding_scan_session(0) == ESP_OK)
            {
                s_clear_binding_submitted = true;
                s_clear_binding_submission_refresh_pending = true;
                s_clear_binding_failed = false;
            }
            else
            {
                s_clear_binding_failed = true;
                err = ESP_FAIL;
            }
            break;
        case UI_INTENT_SELECT_BLE_CANDIDATE:
            if (maintenance_service_select_ble_candidate(
                    model.ble_candidate_generation,
                    model.ble_candidates[intent->value].mac,
                    0) != MAINTENANCE_RESULT_OK ||
                ble_service_begin_binding_scan_session(0) != ESP_OK)
            {
                err = ESP_FAIL;
            }
            break;
#if !SCENIC_AREA_MANAGEMENT_DEBUG
        case UI_INTENT_START_SELFTEST:
        case UI_INTENT_RERUN_SELFTEST:
            if (maintenance_service_request_selftest() !=
                MAINTENANCE_RESULT_OK)
            {
                err = ESP_FAIL;
            }
            break;
#endif
        case UI_INTENT_GPS_OUTDOOR:
        case UI_INTENT_GPS_INDOOR:
            s_gps_submit_state = UI_RUNTIME_GPS_SUBMIT_LOADING;
            if (ui_runtime_binding_pending_retry_item() ==
                SELFTEST_ITEM_GPS_FIX)
            {
                if (maintenance_service_request_selftest_retry(
                        model.selftest_run_id,
                        SELFTEST_ITEM_GPS_FIX,
                        intent->type == UI_INTENT_GPS_INDOOR) !=
                    MAINTENANCE_RESULT_OK)
                {
                    s_gps_submit_state = UI_RUNTIME_GPS_SUBMIT_FAILED;
                    err = ESP_FAIL;
                }
                else
                {
                    ui_runtime_binding_clear_pending_retry();
                }
            }
            else if (maintenance_service_request_selftest_with_gps_context(
                         intent->type == UI_INTENT_GPS_INDOOR) !=
                     MAINTENANCE_RESULT_OK)
            {
                s_gps_submit_state = UI_RUNTIME_GPS_SUBMIT_FAILED;
                err = ESP_FAIL;
            }
            break;
        case UI_INTENT_SELFTEST_MANUAL_FAIL:
        case UI_INTENT_SELFTEST_MANUAL_PASS:
        case UI_INTENT_SELFTEST_OUTPUT_REPLAY:
            err = submit_selftest_reply(intent, &model);
            if (err == ESP_OK &&
                intent->type != UI_INTENT_SELFTEST_OUTPUT_REPLAY)
            {
                s_manual_reply_submitted = true;
                s_manual_reply_run_id = model.selftest_run_id;
                s_manual_reply_item = model.selftest_item;
            }
            break;
        case UI_INTENT_RETRY_FAILED_ITEM:
        {
            const selftest_item_id_t item =
                ui_runtime_binding_pending_retry_item();
            if (item >= SELFTEST_ITEM_COUNT ||
                maintenance_service_request_selftest_retry(
                    model.selftest_run_id,
                    item,
                    false) != MAINTENANCE_RESULT_OK)
            {
                err = ESP_FAIL;
            }
            else
            {
                ui_runtime_binding_clear_pending_retry();
            }
            break;
        }
        case UI_INTENT_RECONNECT_BLE:
            err = ble_service_request_reconnect_now(0);
            break;
        case UI_INTENT_RESCAN_BLE:
            err = ble_service_request_rescan(0);
            break;
        default:
            break;
        }
    }
    if (button_accepted && preference_action)
    {
        request_ui_feedback(s_haptics_enabled,
                            s_click_audio_enabled);
    }
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "typed intent %d 未被业务 owner 接受，错误=0x%x",
                 intent->type,
                 (unsigned)err);
    }
    if (local_shell_navigation)
    {
        /*
         * 常驻页路由已经使用最近一次完整模型完成渲染；手势结束后
         * 不再重复争用状态 mutex，后台事实变化仍由合并刷新投影。
         */
        return;
    }
    if (!refresh_runtime_model())
    {
        ESP_LOGW(TAG, "typed intent 处理后的模型刷新失败，保留当前页面");
        atomic_store(&s_pending_model_refresh, true);
    }
}

static bool is_local_shell_navigation_intent(
    ui_runtime_intent_type_t type)
{
    switch (type)
    {
    case UI_INTENT_BACK_TO_SHELL:
    case UI_INTENT_SHELL_HOME:
    case UI_INTENT_SHELL_CONTROL:
    case UI_INTENT_SHELL_ALERTS:
    case UI_INTENT_SHELL_MODE_POWER:
    case UI_INTENT_SHELL_PREVIOUS:
    case UI_INTENT_SHELL_NEXT:
        return true;
    default:
        return false;
    }
}

static void drain_ble_results(void)
{
    bool refresh_needed = false;
    for (size_t index = 0U;
         index < BLE_UNLOCK_RESULT_QUEUE_DEPTH;
         ++index)
    {
        ble_unlock_result_t result = {0};
        if (ble_service_receive_unlock_result(&result, 0) != ESP_OK)
        {
            break;
        }
        const bool result_matches_payment_request = ui_payment_result_matches(
            s_payment_verification_pending,
            s_payment_intent_sequence,
            s_payment_link_generation,
            result.intent_sequence,
            result.link_generation);
        if (result_matches_payment_request)
        {
            s_payment_verification_pending = false;
            s_payment_intent_sequence = 0U;
        }
        refresh_needed = true;
        ESP_LOGI(TAG,
                 "已消费 BLE 解锁结果：intent=%lu，请求=%lu，结果=%d，链路代次=%lu，稳定码=%s",
                 (unsigned long)result.intent_sequence,
                 (unsigned long)result.request_id,
                 (int)result.result,
                 (unsigned long)result.link_generation,
                 result.error_code);
    }

    for (size_t index = 0U;
         index < BLE_CONTROL_RESULT_QUEUE_DEPTH;
         ++index)
    {
        ble_control_result_t result = {0};
        if (ble_service_receive_control_result(&result, 0) != ESP_OK)
        {
            break;
        }
        if (control_result_is_terminal(result.result))
        {
            clear_control_projection_for_kind(result.kind);
        }
        refresh_needed = true;
        ESP_LOGI(TAG,
                 "已消费 BLE 控制结果：请求=%lu，类型=%d，目标=%u，attempt=%u，结果=%d，稳定码=%s",
                 (unsigned long)result.request_id,
                 (int)result.kind,
                 (unsigned)result.target,
                 (unsigned)result.attempt,
                 (int)result.result,
                 result.error_code);
    }
    if (refresh_needed)
    {
        atomic_store(&s_pending_model_refresh, true);
    }
}

static void request_ui_feedback(bool haptics_enabled,
                                bool click_audio_enabled)
{
    const audio_service_result_t result =
        audio_service_request_ui_feedback(haptics_enabled,
                                          click_audio_enabled);
    if (result != AUDIO_RESULT_OK)
    {
        ESP_LOGW(TAG,
                 "UI 按钮反馈请求未被接受，稳定错误码=%s",
                 audio_service_result_code(result));
    }
}

static void report_power_runtime_result(watch_power_error_t error)
{
    const esp_err_t notify_error =
        power_service_notify_runtime_result(s_display_on, error, 0);
    if (notify_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "显示或触摸运行结果通知 power_task 失败，错误=0x%x",
                 (unsigned)notify_error);
    }
}

static esp_err_t submit_control_intent(const ui_runtime_intent_t *intent,
                                       const ui_runtime_model_t *model)
{
    control_ui_action_t action;
    uint8_t target;
    switch (intent->type)
    {
    case UI_INTENT_GEAR_DECREMENT:
        if (model->gear <= 1U)
        {
            return ESP_ERR_INVALID_ARG;
        }
        action = CONTROL_UI_ACTION_GEAR;
        target = (uint8_t)(model->gear - 1U);
        break;
    case UI_INTENT_GEAR_INCREMENT:
        if (model->gear >= watch_exoskeleton_scene_config_max_gear(
                               model->scene_config))
        {
            return ESP_ERR_INVALID_ARG;
        }
        action = CONTROL_UI_ACTION_GEAR;
        target = (uint8_t)(model->gear + 1U);
        break;
    case UI_INTENT_TOGGLE_ASSIST:
        action = CONTROL_UI_ACTION_MOTOR;
        target = model->assist_enabled ? 0U : 1U;
        break;
    case UI_INTENT_SET_MODE_STANDARD:
        action = CONTROL_UI_ACTION_SCENE_MODE;
        target = 1U;
        break;
    case UI_INTENT_SET_MODE_EXTREME:
        action = CONTROL_UI_ACTION_SCENE_MODE;
        target = 3U;
        break;
    case UI_INTENT_SET_MODE_SPORT:
        action = CONTROL_UI_ACTION_SCENE_MODE;
        target = ui_runtime_binding_scene_target_for_intent(
            intent->type,
            model->scene_config);
        break;
    case UI_INTENT_SET_MODE_ECO:
        action = CONTROL_UI_ACTION_SCENE_MODE;
        target = 4U;
        break;
    case UI_INTENT_EXO_POWEROFF:
        action = CONTROL_UI_ACTION_POWEROFF;
        target = 1U;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    watch_state_snapshot_t latest = {0};
    const uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000U;
    const esp_err_t snapshot_error = watch_state_snapshot(&latest, 0);
    control_ui_model_t latest_control = {0};
    const esp_err_t control_model_error =
        snapshot_error == ESP_OK
            ? control_ui_model_from_snapshot(&latest,
                                             true,
                                             now_ms,
                                             &latest_control)
            : snapshot_error;
    const bool latest_allowed =
        control_model_error == ESP_OK &&
        ((action == CONTROL_UI_ACTION_POWEROFF &&
          control_ui_poweroff_submit_allowed(&latest,
                                             true,
                                             now_ms)) ||
         (action != CONTROL_UI_ACTION_POWEROFF &&
          latest_control.controls_enabled));
    if (!latest_allowed)
    {
        ESP_LOGW(TAG,
                 "控制提交前复核拒绝：类型=%s，错误=0x%x，内部门禁=%s",
                 ui_intent_name(intent->type, model),
                 (unsigned)control_model_error,
                 control_model_error == ESP_OK
                     ? control_block_reason_name(latest_control.block_reason)
                     : "状态快照读取失败");
        return ESP_ERR_INVALID_STATE;
    }
    if (action == CONTROL_UI_ACTION_SCENE_MODE &&
        !watch_exoskeleton_scene_config_allows_control(
            latest.exoskeleton_state.scene_config,
            target))
    {
        ESP_LOGW(TAG,
                 "控制提交前复核拒绝：型号=%s 不支持场景=%u",
                 exoskeleton_model_name(latest.exoskeleton_model),
                 (unsigned)target);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (action == CONTROL_UI_ACTION_GEAR &&
        target > watch_exoskeleton_scene_config_max_gear(
                     latest.exoskeleton_state.scene_config))
    {
        ESP_LOGW(TAG,
                 "控制提交前复核拒绝：型号=%s 最高档=%u，目标=%u",
                 exoskeleton_model_name(latest.exoskeleton_model),
                 (unsigned)watch_exoskeleton_scene_config_max_gear(
                     latest.exoskeleton_state.scene_config),
                 (unsigned)target);
        return ESP_ERR_NOT_SUPPORTED;
    }

    control_ui_intent_t converted = {0};
    esp_err_t err = control_ui_intent_from_action(action, target, &converted);
    if (err != ESP_OK)
    {
        return err;
    }
    const ble_control_intent_t ble_intent = {
        .kind = converted.kind,
        .target = converted.target,
    };
    err = ble_service_request_control(&ble_intent, 0);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "控制请求已进入 BLE owner 队列：类型=%d，目标=%u",
                 (int)ble_intent.kind,
                 (unsigned)ble_intent.target);
    }
    return err;
}

static esp_err_t submit_selftest_reply(const ui_runtime_intent_t *intent,
                                       const ui_runtime_model_t *model)
{
    if (model->selftest_run_id == 0U ||
        model->selftest_item >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_STATE;
    }
    ui_runtime_binding_stop_amoled_test();
    const selftest_ui_reply_t reply = {
        .run_id = model->selftest_run_id,
        .item_id = model->selftest_item,
        .action = intent->type == UI_INTENT_SELFTEST_OUTPUT_REPLAY
                      ? SELFTEST_UI_REPLY_ACTION_OUTPUT_REPLAY
                      : SELFTEST_UI_REPLY_ACTION_DECISION,
        .outcome = intent->type == UI_INTENT_SELFTEST_MANUAL_PASS
                       ? SELFTEST_OUTCOME_PASS
                       : intent->type == UI_INTENT_SELFTEST_MANUAL_FAIL
                             ? SELFTEST_OUTCOME_FAIL
                             : SELFTEST_OUTCOME_NOT_RUN,
    };
    return selftest_service_submit_ui_reply(&reply, 0);
}

static void apply_control_projection(ui_runtime_model_t *model,
                                     bool ble_connected,
                                     uint64_t now_ms)
{
    if (!ble_connected)
    {
        clear_control_projections();
        return;
    }
    if (s_gear_projection.active)
    {
        if (now_ms >= s_gear_projection.expires_ms)
        {
            s_gear_projection.active = false;
        }
        else
        {
            model->gear = s_gear_projection.target;
            model->gear_projection_locked = true;
        }
    }
    if (s_assist_projection.active)
    {
        if (now_ms >= s_assist_projection.expires_ms)
        {
            s_assist_projection.active = false;
        }
        else
        {
            model->assist_enabled = s_assist_projection.target != 0U;
            model->assist_projection_locked = true;
        }
    }
    if (s_mode_projection.active)
    {
        if (now_ms >= s_mode_projection.expires_ms)
        {
            s_mode_projection.active = false;
        }
        else
        {
            model->drive_mode = s_mode_projection.target;
            model->mode_projection_locked = true;
        }
    }
}

static void lock_control_projection(const ui_runtime_intent_t *intent,
                                    const ui_runtime_model_t *model,
                                    uint64_t now_ms)
{
    ui_control_projection_lock_t *lock = NULL;
    uint8_t target = 0U;
    switch (intent->type)
    {
    case UI_INTENT_GEAR_DECREMENT:
        lock = &s_gear_projection;
        target = model->gear > 1U
                     ? (uint8_t)(model->gear - 1U)
                     : 1U;
        break;
    case UI_INTENT_GEAR_INCREMENT:
        lock = &s_gear_projection;
        target = (uint8_t)(model->gear + 1U);
        break;
    case UI_INTENT_TOGGLE_ASSIST:
        lock = &s_assist_projection;
        target = model->assist_enabled ? 0U : 1U;
        break;
    case UI_INTENT_SET_MODE_STANDARD:
        lock = &s_mode_projection;
        target = 1U;
        break;
    case UI_INTENT_SET_MODE_EXTREME:
        lock = &s_mode_projection;
        target = 3U;
        break;
    case UI_INTENT_SET_MODE_SPORT:
        lock = &s_mode_projection;
        target = ui_runtime_binding_scene_target_for_intent(
            intent->type,
            model->scene_config);
        break;
    case UI_INTENT_SET_MODE_ECO:
        lock = &s_mode_projection;
        target = 4U;
        break;
    default:
        return;
    }
    *lock = (ui_control_projection_lock_t){
        .active = true,
        .target = target,
        .expires_ms = now_ms + UI_CONTROL_PROJECTION_LOCK_MS,
    };
}

static bool control_projection_refresh_due(uint64_t now_ms)
{
    return (s_gear_projection.active &&
            now_ms >= s_gear_projection.expires_ms) ||
           (s_assist_projection.active &&
            now_ms >= s_assist_projection.expires_ms) ||
           (s_mode_projection.active &&
            now_ms >= s_mode_projection.expires_ms);
}

static void clear_control_projections(void)
{
    s_gear_projection = (ui_control_projection_lock_t){0};
    s_assist_projection = (ui_control_projection_lock_t){0};
    s_mode_projection = (ui_control_projection_lock_t){0};
    s_boundary_projection_request_id = 0U;
}

static void clear_control_projection_for_kind(watch_control_kind_t kind)
{
    switch (kind)
    {
    case WATCH_CONTROL_KIND_MOTOR_ENABLE:
        s_assist_projection = (ui_control_projection_lock_t){0};
        break;
    case WATCH_CONTROL_KIND_GEAR:
        s_gear_projection = (ui_control_projection_lock_t){0};
        break;
    case WATCH_CONTROL_KIND_SCENE_MODE:
        s_mode_projection = (ui_control_projection_lock_t){0};
        break;
    case WATCH_CONTROL_KIND_POWEROFF:
        break;
    case WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY:
        s_gear_projection = (ui_control_projection_lock_t){0};
        s_mode_projection = (ui_control_projection_lock_t){0};
        s_boundary_projection_request_id = 0U;
        break;
    case WATCH_CONTROL_KIND_NONE:
    default:
        break;
    }
}

static bool control_result_is_terminal(ble_control_result_code_t result)
{
    return result != BLE_CONTROL_RESULT_SUBMITTED_UNCONFIRMED &&
           result != BLE_CONTROL_RESULT_RETRYING &&
           result != BLE_CONTROL_RESULT_POWEROFF_PENDING;
}

static bool is_control_intent(ui_runtime_intent_type_t type)
{
    return type == UI_INTENT_GEAR_DECREMENT ||
           type == UI_INTENT_GEAR_INCREMENT ||
           type == UI_INTENT_TOGGLE_ASSIST ||
           type == UI_INTENT_SET_MODE_STANDARD ||
           type == UI_INTENT_SET_MODE_EXTREME ||
           type == UI_INTENT_SET_MODE_SPORT ||
           type == UI_INTENT_SET_MODE_ECO ||
           type == UI_INTENT_EXO_POWEROFF;
}

static const char *ui_intent_name(
    ui_runtime_intent_type_t type,
    const ui_runtime_model_t *model)
{
    switch (type)
    {
    case UI_INTENT_GEAR_DECREMENT:
        return "档位减";
    case UI_INTENT_GEAR_INCREMENT:
        return "档位加";
    case UI_INTENT_TOGGLE_ASSIST:
        return "助力开关";
    case UI_INTENT_SET_MODE_STANDARD:
        return "标准模式";
    case UI_INTENT_SET_MODE_EXTREME:
        return "极限模式";
    case UI_INTENT_SET_MODE_SPORT:
        return ui_runtime_binding_scene_target_for_intent(
                   type,
                   model != NULL ? model->scene_config
                                 : WATCH_EXOSKELETON_MODEL_UNKNOWN) ==
                       WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP
                   ? "小碎步模式"
                   : "健身模式";
    case UI_INTENT_SET_MODE_ECO:
        return "下山模式";
    case UI_INTENT_EXO_POWEROFF:
        return "外骨骼关机";
    case UI_INTENT_VERIFY_PAYMENT:
        return "支付验证";
    default:
        return "其他";
    }
}

static const char *control_block_reason_name(
    control_ui_block_reason_t reason)
{
    switch (reason)
    {
    case CONTROL_UI_BLOCK_NONE:
        return "允许控制";
    case CONTROL_UI_BLOCK_SNAPSHOT_FAILED:
        return "状态快照读取失败";
    case CONTROL_UI_BLOCK_UNBOUND:
        return "尚未绑定";
    case CONTROL_UI_BLOCK_BLE_DISCONNECTED:
        return "BLE 未连接";
    case CONTROL_UI_BLOCK_GATT_NOT_READY:
        return "GATT 未就绪";
    case CONTROL_UI_BLOCK_NOTIFY_NOT_READY:
        return "Notify 未就绪";
    case CONTROL_UI_BLOCK_DATA_NOT_READY:
        return "设备状态数据未就绪";
    case CONTROL_UI_BLOCK_STATUS_STALE:
        return "设备状态已过期";
    case CONTROL_UI_BLOCK_LINK_NOT_READY:
        return "链路控制准入未就绪";
    case CONTROL_UI_BLOCK_PENDING:
        return "已有授权或控制处理中";
    case CONTROL_UI_BLOCK_RETRY_EXHAUSTED:
        return "控制重试已耗尽";
    case CONTROL_UI_BLOCK_AUTH_REQUIRED:
        return "当前会话尚未授权";
    case CONTROL_UI_BLOCK_RENTAL_UNPAID:
        return "租赁尚未支付";
    case CONTROL_UI_BLOCK_DEVICE_DISABLED:
        return "设备已禁用";
    case CONTROL_UI_BLOCK_UNKNOWN_DEVICE:
        return "云端未登记设备";
    case CONTROL_UI_BLOCK_CLOUD_UNAVAILABLE:
        return "云授权服务不可用";
    default:
        return "未知控制门禁";
    }
}

static const char *exoskeleton_model_name(watch_exoskeleton_model_t model)
{
    switch (model)
    {
    case WATCH_EXOSKELETON_MODEL_MINI:
        return "Mini";
    case WATCH_EXOSKELETON_MODEL_PRO:
        return "Pro";
    case WATCH_EXOSKELETON_MODEL_MAX:
        return "Max";
    case WATCH_EXOSKELETON_MODEL_MINI_Y:
        return "MiniY";
    case WATCH_EXOSKELETON_MODEL_PRO_Y:
        return "ProY";
    case WATCH_EXOSKELETON_MODEL_MAX_Y:
        return "MaxY";
    case WATCH_EXOSKELETON_MODEL_OLD_A:
        return "OldA";
    case WATCH_EXOSKELETON_MODEL_OLD_C:
        return "OldC";
    case WATCH_EXOSKELETON_MODEL_CHILD:
        return "Child";
    case WATCH_EXOSKELETON_MODEL_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

static const char *control_rejection_reason(
    const ui_runtime_intent_t *intent,
    const ui_runtime_model_t *model)
{
    if (intent == NULL || model == NULL)
    {
        return "intent 或模型无效";
    }
    if (model->payment_required)
    {
        return "需要先完成云授权";
    }
    if (model->selftest_running)
    {
        return "整机自检正在运行";
    }
    switch (intent->type)
    {
    case UI_INTENT_GEAR_DECREMENT:
        if (model->gear_projection_locked)
        {
            return "上一条档位指令等待确认";
        }
        if (model->gear <= 1U)
        {
            return "档位已到下限";
        }
        break;
    case UI_INTENT_GEAR_INCREMENT:
        if (model->gear_projection_locked)
        {
            return "上一条档位指令等待确认";
        }
        if (model->gear >= watch_exoskeleton_scene_config_max_gear(
                               model->scene_config))
        {
            return "档位已到上限";
        }
        break;
    case UI_INTENT_TOGGLE_ASSIST:
        if (model->assist_projection_locked)
        {
            return "上一条助力指令等待确认";
        }
        break;
    case UI_INTENT_SET_MODE_STANDARD:
    case UI_INTENT_SET_MODE_EXTREME:
    case UI_INTENT_SET_MODE_SPORT:
    case UI_INTENT_SET_MODE_ECO:
    {
        const uint8_t target = ui_runtime_binding_scene_target_for_intent(
            intent->type,
            model->scene_config);
        if (!watch_exoskeleton_scene_config_allows_control(
                model->scene_config,
                target))
        {
            return "当前型号不支持目标场景模式";
        }
        if (model->mode_projection_locked)
        {
            return "上一条模式指令等待确认";
        }
        break;
    }
    case UI_INTENT_EXO_POWEROFF:
        if (model->poweroff_committed)
        {
            return "外骨骼关机已提交";
        }
        if (!model->power_action_enabled)
        {
            return control_block_reason_name(model->control_block_reason);
        }
        break;
    default:
        break;
    }
    return control_block_reason_name(model->control_block_reason);
}

static void log_control_click(const ui_runtime_intent_t *intent,
                              const ui_runtime_model_t *model)
{
    if (intent == NULL || model == NULL)
    {
        return;
    }
    switch (intent->type)
    {
    case UI_INTENT_GEAR_DECREMENT:
        ESP_LOGI(TAG,
                 "控件点击：类型=档位减，当前=%u，目标=%u",
                 (unsigned)model->gear,
                 model->gear > 1U
                     ? (unsigned)model->gear - 1U
                     : 1U);
        break;
    case UI_INTENT_GEAR_INCREMENT:
        ESP_LOGI(TAG,
                 "控件点击：类型=档位加，当前=%u，目标=%u",
                 (unsigned)model->gear,
                 (unsigned)model->gear + 1U);
        break;
    case UI_INTENT_TOGGLE_ASSIST:
        ESP_LOGI(TAG,
                 "控件点击：类型=助力开关，当前=%s，目标=%s",
                 model->assist_enabled ? "开启" : "关闭",
                 model->assist_enabled ? "关闭" : "开启");
        break;
    case UI_INTENT_SET_MODE_STANDARD:
    case UI_INTENT_SET_MODE_EXTREME:
    case UI_INTENT_SET_MODE_SPORT:
    case UI_INTENT_SET_MODE_ECO:
    {
        const uint8_t target = ui_runtime_binding_scene_target_for_intent(
            intent->type,
            model->scene_config);
        ESP_LOGI(TAG,
                 "控件点击：类型=%s，当前场景=%u，目标场景=%u",
                 ui_intent_name(intent->type, model),
                 (unsigned)model->drive_mode,
                 (unsigned)target);
        break;
    }
    case UI_INTENT_EXO_POWEROFF:
        ESP_LOGI(TAG, "控件点击：类型=外骨骼关机，目标=1");
        break;
    default:
        break;
    }
}

static void log_runtime_model_transitions(
    const watch_state_snapshot_t *watch,
    const ui_runtime_model_t *model)
{
    if (watch == NULL || model == NULL)
    {
        return;
    }
    if (watch->device_state_available &&
        watch->exoskeleton_state.version[0] != '\0' &&
        (!s_logged_exoskeleton_model_valid ||
        s_logged_exoskeleton_model != model->exoskeleton_model ||
         strcmp(s_logged_exoskeleton_version,
                watch->exoskeleton_state.version) != 0))
    {
        ESP_LOGI(TAG,
                 "外骨骼能力识别：原始版本=%s，型号=%s，scene_config=%u，极限模式=%s",
                 watch->exoskeleton_state.version,
                 exoskeleton_model_name(model->exoskeleton_model),
                 (unsigned)model->scene_config,
                 watch_exoskeleton_scene_config_supports_mode(
                     model->scene_config,
                     3U)
                     ? "支持"
                     : "不支持");
        s_logged_exoskeleton_model_valid = true;
        s_logged_exoskeleton_model = model->exoskeleton_model;
        copy_text(s_logged_exoskeleton_version,
                  sizeof(s_logged_exoskeleton_version),
                  watch->exoskeleton_state.version);
    }
    else if (!watch->has_bound_exoskeleton)
    {
        s_logged_exoskeleton_model_valid = false;
        memset(s_logged_exoskeleton_version,
               0,
               sizeof(s_logged_exoskeleton_version));
    }

    if (!s_logged_control_availability_valid ||
        s_logged_control_block_reason != model->control_block_reason ||
        s_logged_payment_required != model->payment_required ||
        s_logged_authorization_display != model->authorization_display ||
        s_logged_payment_action_enabled != model->payment_action_enabled)
    {
        ESP_LOGI(TAG,
                 "控制可用性变化：允许控制=%u，内部门禁=%s，授权显示=%u，授权按钮=%u，BLE 代次=%lu，控制代次=%lu，租赁阶段=%u，解锁证明=%u",
                 model->controls_enabled ? 1U : 0U,
                 control_block_reason_name(model->control_block_reason),
                 (unsigned)model->authorization_display,
                 model->payment_action_enabled ? 1U : 0U,
                 (unsigned long)watch->ble_link_generation,
                 (unsigned long)watch->control_link_generation,
                 (unsigned)watch->rental_phase,
                 watch->unlock_session_valid ? 1U : 0U);
        s_logged_control_availability_valid = true;
        s_logged_control_block_reason = model->control_block_reason;
        s_logged_payment_required = model->payment_required;
        s_logged_authorization_display = model->authorization_display;
        s_logged_payment_action_enabled = model->payment_action_enabled;
    }

    if (!s_logged_cellular_state_valid ||
        s_logged_cellular_lifecycle != watch->modem.lifecycle ||
        s_logged_cellular_connected != model->cellular_connected ||
        s_logged_cellular_action_enabled !=
            model->cellular_action_enabled ||
        s_logged_cellular_connecting != model->cellular_connecting)
    {
        ESP_LOGI(TAG,
                 "4G 维护状态变化：生命周期=%u，已联网=%u，允许手动=%u，连接中=%u，错误=%s",
                 (unsigned)watch->modem.lifecycle,
                 model->cellular_connected ? 1U : 0U,
                 model->cellular_action_enabled ? 1U : 0U,
                 model->cellular_connecting ? 1U : 0U,
                 watch->modem.last_error);
        s_logged_cellular_state_valid = true;
        s_logged_cellular_lifecycle = watch->modem.lifecycle;
        s_logged_cellular_connected = model->cellular_connected;
        s_logged_cellular_action_enabled =
            model->cellular_action_enabled;
        s_logged_cellular_connecting = model->cellular_connecting;
    }
}

static bool manual_countdown_refresh_due(uint64_t now_ms)
{
    if (s_manual_prompt_started_ms == 0U ||
        now_ms < s_manual_prompt_started_ms)
    {
        return false;
    }
    const uint64_t elapsed_ms = now_ms - s_manual_prompt_started_ms;
    const uint32_t second = elapsed_ms >= 30000U
                                ? 30U
                                : (uint32_t)(elapsed_ms / 1000U);
    if (second == s_manual_countdown_second)
    {
        return false;
    }
    s_manual_countdown_second = second;
    return true;
}

static bool clock_refresh_due(uint64_t now_ms)
{
    if (s_next_clock_refresh_ms == UINT64_MAX ||
        now_ms < s_next_clock_refresh_ms)
    {
        return false;
    }
    s_next_clock_refresh_ms = now_ms <= UINT64_MAX - 1000U
                                  ? now_ms + 1000U
                                  : UINT64_MAX;
    return true;
}

static esp_err_t apply_runtime_brightness(
    watch_power_level_t power_level,
    bool selftest_active,
    bool force)
{
    const uint16_t brightness =
        power_display_brightness_for_user(
            power_level,
            selftest_active,
            brightness_raw_for_level(s_brightness_level));
    if (!force && s_brightness_valid &&
        s_brightness_value == brightness)
    {
        return ESP_OK;
    }
    const esp_err_t err = co5300_bsp_set_brightness(brightness);
    if (err == ESP_OK)
    {
        s_brightness_valid = true;
        s_brightness_value = brightness;
    }
    return err;
}

static uint16_t brightness_raw_for_level(
    config_service_ui_brightness_t level)
{
    switch (level)
    {
    case CONFIG_SERVICE_UI_BRIGHTNESS_LOW:
        return CONFIG_SERVICE_UI_BRIGHTNESS_LOW_RAW;
    case CONFIG_SERVICE_UI_BRIGHTNESS_MEDIUM:
        return CONFIG_SERVICE_UI_BRIGHTNESS_MEDIUM_RAW;
    case CONFIG_SERVICE_UI_BRIGHTNESS_HIGH:
    default:
        return CONFIG_SERVICE_UI_BRIGHTNESS_HIGH_RAW;
    }
}

static uint32_t timeout_ms_for_preference(
    config_service_ui_screen_timeout_t timeout)
{
    switch (timeout)
    {
    case CONFIG_SERVICE_UI_SCREEN_TIMEOUT_30S:
        return POWER_SERVICE_IDLE_TIMEOUT_30S_MS;
    case CONFIG_SERVICE_UI_SCREEN_TIMEOUT_15S:
        return POWER_SERVICE_IDLE_TIMEOUT_15S_MS;
    case CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S:
    default:
        return POWER_SERVICE_IDLE_TIMEOUT_5S_MS;
    }
}

static config_service_ui_preferences_t current_ui_preferences(void)
{
    return (config_service_ui_preferences_t){
        .haptics_enabled = s_haptics_enabled,
        .click_audio_enabled = s_click_audio_enabled,
        .raise_to_wake_enabled = s_raise_to_wake_enabled,
        .brightness = s_brightness_level,
        .screen_timeout = s_screen_timeout,
    };
}

static void append_alert(ui_runtime_model_t *model,
                         ui_runtime_alert_type_t type,
                         bool danger,
                         const char *label,
                         const char *value)
{
    if (model == NULL)
    {
        return;
    }
    if (model->visible_alert_count < UI_RUNTIME_VISIBLE_ALERT_COUNT)
    {
        ui_runtime_alert_t *alert =
            &model->alerts[model->visible_alert_count++];
        alert->type = type;
        alert->danger = danger;
        copy_text(alert->label, sizeof(alert->label), label);
        copy_text(alert->value, sizeof(alert->value), value);
    }
    if (model->reminder_count < UINT8_MAX)
    {
        ++model->reminder_count;
    }
}

static bool is_printable_ascii_text(const char *text)
{
    if (text == NULL || text[0] == '\0')
    {
        return false;
    }
    for (const unsigned char *cursor = (const unsigned char *)text;
         *cursor != '\0';
         ++cursor)
    {
        if (*cursor < 0x20U || *cursor > 0x7EU)
        {
            return false;
        }
    }
    return true;
}

static void copy_text(char *destination, size_t capacity, const char *source)
{
    if (destination == NULL || capacity == 0U)
    {
        return;
    }
    const char *safe_source = source != NULL ? source : "";
    size_t length = 0U;
    while (length + 1U < capacity && safe_source[length] != '\0')
    {
        ++length;
    }
    memcpy(destination, safe_source, length);
    destination[length] = '\0';
}

static const char *selftest_item_text(selftest_item_id_t item)
{
    const ui_selftest_category_t category =
        ui_selftest_category_for_item(item);
    return category < UI_SELFTEST_CATEGORY_COUNT
               ? ui_selftest_category_name(category)
               : "--";
}

static const char *selftest_prompt(selftest_item_id_t item)
{
    switch (item)
    {
    case SELFTEST_ITEM_AMOLED:
        return "画面是否完整、无闪烁？";
    case SELFTEST_ITEM_TOUCH:
        return "点击目标，滑块超过 50% 即可";
    case SELFTEST_ITEM_AUDIO:
        return "是否听到清晰的提示音？";
    case SELFTEST_ITEM_VIBRATION:
        return "是否感受到振动？";
    default:
        return "请等待自动检查完成";
    }
}

static esp_err_t reveal_display_after_full_redraw(void)
{
    if (s_display_on)
    {
        return ESP_OK;
    }
    if (!s_display_flush_enabled)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!refresh_full_screen_with_recovery())
    {
        return s_flush_pending ? ESP_ERR_TIMEOUT : ESP_FAIL;
    }

    const esp_err_t error = co5300_bsp_set_display(true);
    if (error == ESP_OK)
    {
        s_display_on = true;
        ui_wake_touch_gate_set_display(&s_wake_touch_gate, true);
        ui_runtime_binding_set_display_on(true);
        ESP_LOGI(TAG, "AMOLED 确定首帧已完整提交，现执行 DISPON");
    }
    return error;
}

static esp_err_t suspend_display_with_single_retry(void)
{
    esp_err_t error = co5300_bsp_suspend();
    if (error == ESP_OK)
    {
        return ESP_OK;
    }
    ESP_LOGW(TAG,
             "AMOLED 低功耗休眠首次失败，执行唯一一次重试，错误=0x%x",
             (unsigned)error);
    error = co5300_bsp_suspend();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "AMOLED 低功耗休眠唯一重试仍失败，错误=0x%x",
                 (unsigned)error);
    }
    return error;
}

static esp_err_t resume_display_with_single_retry(void)
{
    esp_err_t error = co5300_bsp_resume();
    if (error == ESP_OK)
    {
        return ESP_OK;
    }
    ESP_LOGW(TAG,
             "AMOLED 供电与面板恢复首次失败，执行唯一一次重试，错误=0x%x",
             (unsigned)error);
    error = co5300_bsp_resume();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "AMOLED 供电与面板恢复唯一重试仍失败，错误=0x%x",
                 (unsigned)error);
    }
    return error;
}

static esp_err_t reset_display_touch(bool recover_timed_out_flush)
{
    if (!recover_timed_out_flush &&
        !wait_for_pending_flush(UI_FULL_REDRAW_FLUSH_TIMEOUT_MS))
    {
        ESP_LOGE(TAG, "显示恢复前等待 DMA flush 超时");
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = co5300_bsp_reset();
    if (recover_timed_out_flush)
    {
        /*
         * co5300_bsp_reset() 的同步参数事务会先回收所有旧颜色事务；
         * 只有它成功返回后，才允许把超时缓冲区归还 LVGL。
         */
        complete_pending_flush(0);
        if (err == ESP_OK && s_flush_pending)
        {
            complete_flush_after_display_reset();
        }
        if (s_flush_pending)
        {
            ESP_LOGE(TAG, "显示恢复未确认旧 DMA 完成，继续保留 LVGL 缓冲区");
            return err == ESP_OK ? ESP_ERR_TIMEOUT : err;
        }
    }
    if (err != ESP_OK)
    {
        return err;
    }
    err = i2c_manager_acquire(
        pdMS_TO_TICKS(UI_TOUCH_SETUP_I2C_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        return err;
    }
    err = cst9217_bsp_reset();
    i2c_manager_release();
    if (err == ESP_OK)
    {
        /* 复位完成后不存在旧颜色事务，清理已消费通知再允许下一次 flush。 */
        (void)ulTaskNotifyTake(pdTRUE, 0);
        lv_obj_invalidate(lv_scr_act());
        ESP_LOGI(TAG, "显示与触摸恢复成功，LVGL display/input 未重复注册");
    }
    return err;
}

static void cleanup_prepare_failure(void)
{
    (void)cst9217_bsp_deinit();
    (void)co5300_bsp_deinit();
    if (s_buffer1 != NULL)
    {
        heap_caps_free(s_buffer1);
        s_buffer1 = NULL;
    }
    if (s_buffer2 != NULL)
    {
        heap_caps_free(s_buffer2);
        s_buffer2 = NULL;
    }
    s_prepared = false;
    s_display_on = false;
    s_display_flush_enabled = false;
    s_brightness_valid = false;
    memset(&s_last_rendered_model, 0, sizeof(s_last_rendered_model));
    s_last_rendered_model_valid = false;
}
