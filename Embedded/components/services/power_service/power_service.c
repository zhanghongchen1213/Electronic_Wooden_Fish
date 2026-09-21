/**
 * @file     power_service.c
 * @brief    手环电量与日常屏幕交互服务实现
 * @details  在固定 power_task 中以 ISR 唤醒和绝对 deadline 推进 CW2015、idle、PWR、QMI 与显示意图。
 * @author   ZHC
 * @date     2026-07-28
 */

#include "power_service.h"

#include <limits.h>
#include <stdatomic.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "ble_service.h"
#include "gps_service.h"
#include "i2c_manager.h"
#include "config_service.h"
#include "cw2015_bsp.h"
#include "key.h"
#include "qmi8658c_bsp.h"
#include "state_service.h"
#include "ui_service.h"
#include "wrist_raise_policy.h"

#if !defined(LEGBOT_CAP_GPS_TIME) || \
    (LEGBOT_CAP_GPS_TIME != 0 && LEGBOT_CAP_GPS_TIME != 1)
#error "LEGBOT_CAP_GPS_TIME must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_GPS_CONTINUOUS_LOCATION)
#error "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION must be defined."
#endif

static const char *TAG = "SVC_POWER";

/** 连续失败次数上限，达到后使用扩展轮询间隔减少总线争用。 */
#define POWER_SERVICE_BACKOFF_THRESHOLD_3 3U
#define POWER_SERVICE_BACKOFF_THRESHOLD_6 6U
/** 扩展轮询间隔：轻度退避。 */
#define POWER_SERVICE_BACKOFF_MS_30S 30000U
/** 扩展轮询间隔：深度退避。 */
#define POWER_SERVICE_BACKOFF_MS_60S 60000U
/** latest 运行结果中 error+1 的低位掩码，零值表示槽为空。 */
#define POWER_SERVICE_RUNTIME_RESULT_ERROR_MASK 0xFFU
/** latest 运行结果中的实际亮屏标志位。 */
#define POWER_SERVICE_RUNTIME_RESULT_DISPLAY_ON (1U << 8)
/** latest 抬腕开关槽中的关闭编码，零值保留为空。 */
#define POWER_SERVICE_RAISE_SETTING_DISABLED 1U
/** latest 抬腕开关槽中的开启编码。 */
#define POWER_SERVICE_RAISE_SETTING_ENABLED 2U
/** 连续第三次 QMI 故障后的轻度退避等待。 */
#define POWER_SERVICE_RAISE_RETRY_MS_1S 1000U
/** 第四次连续 QMI 故障重试等待。 */
#define POWER_SERVICE_RAISE_RETRY_MS_5S 5000U
/** 第五次及以后连续 QMI 故障重试等待。 */
#define POWER_SERVICE_RAISE_RETRY_MS_30S 30000U
/** QMI8658C ±4g 加速度每个 g 的原始计数。 */
#define POWER_SERVICE_ACCEL_COUNTS_PER_G 8192
/** 从低功耗模式切换后完整丢弃的加速度滤波启动样本数。 */
#define POWER_SERVICE_ACCEL_SETTLE_SAMPLES 3U
/** 武装 WoM 前尝试取得稳定姿态基线的最长时间。 */
#define POWER_SERVICE_ARM_BASELINE_TIMEOUT_MS 300U

static void poll_battery(void);
static uint32_t compute_poll_period(bool display_on);
static watch_power_error_t default_power_error(void);
static uint64_t monotonic_elapsed_ms(uint64_t now_ms, uint64_t started_ms);
static uint64_t monotonic_now_ms(void);
static bool monotonic_deadline_reached(uint64_t now_ms,
                                       uint64_t deadline_ms);
static void sample_selftest_fact(power_interaction_input_t *input);
static bool submit_display_intent(power_display_intent_t intent);
static const char *power_intent_name(power_display_intent_t intent);
static void deliver_pending_intent(
    power_display_intent_mailbox_t *mailbox,
    uint64_t *retry_after_ms,
    uint64_t now_ms);
static uint32_t configured_idle_timeout_ms(void);
static void sample_pwr_input(power_interaction_input_t *input,
                             bool read_gpio);
static bool take_runtime_result(bool *display_on,
                                watch_power_error_t *error);
static void publish_power_fact(const power_interaction_policy_t *policy,
                               watch_power_error_t error);
static void retry_pending_power_fact(uint64_t now_ms);
static bool configured_raise_to_wake_enabled(void);
static void apply_pending_raise_setting(uint64_t now_ms);
static void initialize_wrist_raise(bool actual_display_on,
                                   uint64_t now_ms);
static void sample_wrist_raise(
    const power_interaction_policy_t *interaction,
    const power_display_intent_mailbox_t *display_mailbox,
    power_interaction_input_t *input);
static void cancel_wrist_raise(void);
static void shutdown_wrist_raise(void);
static void reject_wrist_raise_candidate(uint64_t now_ms);
static void record_wrist_raise_failure(uint64_t now_ms,
                                       const char *operation,
                                       esp_err_t err);
static esp_err_t arm_wrist_raise_wom(uint64_t now_ms);
static uint32_t wrist_raise_retry_delay_ms(uint32_t failure_count);
static esp_err_t ensure_qmi_ready(void);
static wrist_raise_accel_t map_wrist_raise_sample(
    const qmi8658c_bsp_accel_sample_t *sample);
static int32_t accel_raw_to_mg(int16_t raw);
static void pwr_edge_isr_callback(void *context);
static void qmi_wake_isr_callback(void *context);
static bool register_input_callbacks(QueueHandle_t queue);
static esp_err_t ensure_qmi_callback_registered(void);
static void finish_power_run(void);
static uint64_t wrist_raise_deadline_ms(
    const power_interaction_policy_t *interaction,
    const power_display_intent_mailbox_t *display_mailbox,
    uint64_t now_ms,
    bool *active);
static uint32_t compute_owner_wait_ms(
    const power_interaction_policy_t *interaction,
    const power_display_intent_mailbox_t *display_mailbox,
    const power_display_intent_mailbox_t *navigation_mailbox,
    uint64_t next_battery_ms,
    uint64_t now_ms);
static TickType_t owner_wait_ticks(uint32_t wait_ms);

/** 连续读取/初始化失败计数，用于退避重试间隔。 */
static uint32_t s_consecutive_failures;
/** 本地输入 owner 合并写入、power_task 原子取走的用户活动。 */
static atomic_bool s_local_activity_pending;
/** PWR ISR 先保留、power_task 原子取走的 latest 边沿事实。 */
static atomic_bool s_pwr_edge_pending;
/** ui_task 合并写入、power_task 原子取走的实际显示状态与 typed error。 */
static atomic_uint s_runtime_result_pending;
/** ui_task 合并写入、power_task 原子取走的自动熄屏毫秒数。 */
static atomic_uint_fast32_t s_idle_timeout_pending;
/** ui_task 合并写入、power_task 原子取走的 BLE 页面活动状态。 */
static atomic_uint s_ble_page_active_pending;
/** ui_task 合并写入、power_task 原子取走的抬腕亮屏开关。 */
static atomic_uint s_raise_to_wake_pending;
/** power owner 最近发布的屏幕转换序号。 */
static uint32_t s_power_transition_sequence;
/** state queue 暂满时保留的最新正交 power 真值。 */
static watch_power_update_t s_pending_power_update;
/** 是否存在尚未被 state_task 接受的最新 power 真值。 */
static bool s_pending_power_update_valid;
/** 待提交 power 真值下一次重试的绝对毫秒。 */
static uint64_t s_power_fact_retry_after_ms;
/** 待提交显示意图下一次重试的绝对毫秒。 */
static uint64_t s_display_intent_retry_after_ms;
/** 待提交常驻页导航意图下一次重试的绝对毫秒。 */
static uint64_t s_navigation_intent_retry_after_ms;
/** power_task 串行复用的电源小型状态快照。 */
static watch_power_snapshot_t s_power_snapshot;
/** 运行态 PWR 读取失败的稳定诊断是否已输出。 */
static bool s_pwr_read_warning_logged;
/** 目标板实测参数驱动的 PWR 稳定去抖状态。 */
static power_pwr_debounce_t s_pwr_debounce;
/** PWR 去抖器是否已经与首次原始输入同步。 */
static bool s_pwr_debounce_initialized;
/** PWR 是否已在启动后观察到稳定释放态并允许识别短按。 */
static bool s_pwr_input_armed;
/** 唯一 power_task 串行拥有的手环电量迟滞策略。 */
static power_battery_level_policy_t s_battery_level_policy;
/** 唯一 power_task 串行拥有的抬腕纯策略。 */
static wrist_raise_policy_t s_wrist_raise_policy;
/** 当前运行态是否允许抬腕亮屏。 */
static bool s_raise_to_wake_enabled;
/** ui_task 最近一次确认的真实显示开关状态。 */
static bool s_actual_display_on;
/** 连续 QMI 抬腕路径失败次数。 */
static uint32_t s_wrist_raise_failures;
/** 故障后允许再次访问 QMI 的单调毫秒。 */
static uint64_t s_wrist_raise_retry_after_ms;
/** 下一次候选加速度采样时间。 */
static uint64_t s_next_wrist_sample_ms;
/** 当前候选仍需完整读取并丢弃的加速度滤波启动样本数。 */
static uint32_t s_wrist_settle_samples_remaining;
/** 当前是否正在普通加速度模式下采集 WoM 武装前姿态基线。 */
static bool s_wrist_arm_baseline_active;
/** 当前武装前姿态基线开始采集的单调毫秒。 */
static uint64_t s_wrist_arm_baseline_started_ms;
/** PWR ISR 回调是否已安全登记。 */
static bool s_pwr_callback_registered;
/** QMI ISR 回调是否已安全登记。 */
static bool s_qmi_callback_registered;
/** 当前固定 power_task 队列，供 QMI 回调失败后的设置重试复用。 */
static QueueHandle_t s_power_owner_queue;
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
/** 标定轨迹中最近一次 WoM 候选开始时间。 */
static uint64_t s_wrist_raise_trace_started_ms;
/** 标定轨迹是否正在等待分类结论。 */
static bool s_wrist_raise_trace_candidate_active;
/** 标定轨迹是否正在等待物理显示回报。 */
static bool s_wrist_raise_trace_waiting_display;
#endif

esp_err_t power_service_notify_local_activity(TickType_t timeout_ticks)
{
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_local_activity_pending, true);
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_LOCAL_ACTIVITY,
    };
    if (xQueueSend(queue, &message, timeout_ticks) != pdTRUE)
    {
        ESP_LOGW(TAG,
                 "本地用户活动唤醒消息队列暂满，已保留 latest 活动事实");
    }
    return ESP_OK;
}

esp_err_t power_service_notify_runtime_result(
    bool display_on,
    watch_power_error_t error,
    TickType_t timeout_ticks)
{
    if (error < WATCH_POWER_ERROR_NONE ||
        error >= WATCH_POWER_ERROR_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const unsigned encoded =
        ((unsigned)error + 1U) |
        (display_on ? POWER_SERVICE_RUNTIME_RESULT_DISPLAY_ON : 0U);
    atomic_store(&s_runtime_result_pending, encoded);
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_RUNTIME_RESULT,
    };
    if (xQueueSend(queue, &message, timeout_ticks) != pdTRUE)
    {
        ESP_LOGW(TAG,
                 "显示运行结果唤醒消息队列暂满，已保留 latest 结果");
    }
    return ESP_OK;
}

esp_err_t power_service_set_idle_timeout_ms(
    uint32_t timeout_ms,
    TickType_t timeout_ticks)
{
    if (!power_interaction_policy_idle_timeout_valid(timeout_ms))
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_idle_timeout_pending, timeout_ms);
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_IDLE_TIMEOUT,
    };
    if (xQueueSend(queue, &message, timeout_ticks) != pdTRUE)
    {
        ESP_LOGW(TAG,
                 "自动熄屏时间唤醒消息队列暂满，已保留 latest 值");
    }
    return ESP_OK;
}

esp_err_t power_service_set_ble_page_active(
    bool active,
    TickType_t timeout_ticks)
{
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_ble_page_active_pending, active ? 2U : 1U);
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_BLE_PAGE_ACTIVE,
    };
    if (xQueueSend(queue, &message, timeout_ticks) != pdTRUE)
    {
        ESP_LOGW(TAG,
                 "BLE 页面熄屏策略消息队列暂满，已保留 latest 值");
    }
    return ESP_OK;
}

esp_err_t power_service_set_raise_to_wake_enabled(
    bool enabled,
    TickType_t timeout_ticks)
{
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_raise_to_wake_pending,
                 enabled ? POWER_SERVICE_RAISE_SETTING_ENABLED
                         : POWER_SERVICE_RAISE_SETTING_DISABLED);
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_RAISE_TO_WAKE,
    };
    if (xQueueSend(queue, &message, timeout_ticks) != pdTRUE)
    {
        ESP_LOGW(TAG,
                 "抬腕亮屏开关唤醒消息队列暂满，已保留 latest 值");
    }
    return ESP_OK;
}

esp_err_t power_service_notify_state_change(TickType_t timeout_ticks)
{
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_STATE_CHANGE,
    };
    return xQueueSend(queue, &message, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

void power_service_run(void)
{
    QueueHandle_t queue = legbot_service_queue(LEGBOT_POWER_SERVICE_ID);
    if (queue == NULL)
    {
        ESP_LOGE(TAG, "power_task 消息队列未初始化，无法启动电量轮询");
        return;
    }

    memset(&s_power_snapshot, 0, sizeof(s_power_snapshot));
    if (watch_state_power_snapshot(&s_power_snapshot, 0) == ESP_OK)
    {
        s_power_transition_sequence =
            s_power_snapshot.screen_transition_sequence;
        s_actual_display_on =
            s_power_snapshot.screen_state != WATCH_SCREEN_STATE_OFF;
    }
    else
    {
        s_power_transition_sequence = 0U;
        s_actual_display_on = true;
    }
    s_pending_power_update_valid = false;
    s_power_fact_retry_after_ms = 0U;
    s_display_intent_retry_after_ms = 0U;
    s_navigation_intent_retry_after_ms = 0U;
    s_pwr_read_warning_logged = false;
    memset(&s_pwr_debounce, 0, sizeof(s_pwr_debounce));
    s_pwr_debounce_initialized = false;
    s_pwr_input_armed = false;
    atomic_store(&s_pwr_edge_pending, false);
    power_battery_level_policy_init(&s_battery_level_policy);
    power_interaction_policy_t interaction = {0};
    power_interaction_policy_init(&interaction,
                                  monotonic_now_ms(),
                                  s_actual_display_on);
    (void)power_interaction_policy_set_idle_timeout_ms(
        &interaction,
        configured_idle_timeout_ms());
    atomic_store(&s_ble_page_active_pending, 0U);
    atomic_store(&s_raise_to_wake_pending, 0U);
    s_raise_to_wake_enabled = configured_raise_to_wake_enabled();
    initialize_wrist_raise(s_actual_display_on, monotonic_now_ms());
    s_power_owner_queue = queue;
    (void)register_input_callbacks(queue);
    if (!s_qmi_callback_registered && s_raise_to_wake_enabled)
    {
        s_raise_to_wake_enabled = false;
        cancel_wrist_raise();
        ESP_LOGW(TAG, "QMI ISR 唤醒回调不可用，抬腕亮屏已 fail-closed");
    }
    watch_power_error_t current_error = default_power_error();
    publish_power_fact(&interaction, current_error);

    poll_battery();
    uint64_t next_battery_ms =
        monotonic_now_ms() +
        (uint64_t)compute_poll_period(s_actual_display_on);
    power_display_intent_mailbox_t display_mailbox = {0};
    power_display_intent_mailbox_t navigation_mailbox = {0};
    uint32_t owner_wait_ms = 0U;
    bool pwr_sample_requested = true;
    for (;;)
    {
        legbot_service_message_t message = {0};
        bool state_change_requested = false;
        if (xQueueReceive(queue,
                          &message,
                          owner_wait_ticks(owner_wait_ms)) == pdTRUE)
        {
            if (message.type == LEGBOT_SERVICE_STOP_MESSAGE)
            {
                finish_power_run();
                return;
            }
            pwr_sample_requested =
                pwr_sample_requested ||
                message.type == POWER_SERVICE_MESSAGE_PWR_EDGE;
            state_change_requested =
                message.type == POWER_SERVICE_MESSAGE_STATE_CHANGE;
        }

        const uint64_t now_ms = monotonic_now_ms();
        pwr_sample_requested =
            pwr_sample_requested ||
            atomic_exchange(&s_pwr_edge_pending, false);
        retry_pending_power_fact(now_ms);
        apply_pending_raise_setting(now_ms);
        const uint32_t pending_idle_timeout =
            (uint32_t)atomic_exchange(&s_idle_timeout_pending, 0U);
        if (pending_idle_timeout != 0U &&
            power_interaction_policy_set_idle_timeout_ms(
                &interaction,
                pending_idle_timeout))
        {
            ESP_LOGI(TAG,
                     "已采用最新自动熄屏时间，毫秒=%lu",
                     (unsigned long)pending_idle_timeout);
        }
        const unsigned pending_ble_page_active =
            atomic_exchange(&s_ble_page_active_pending, 0U);
        if (pending_ble_page_active != 0U)
        {
            const bool active = pending_ble_page_active == 2U;
            power_interaction_policy_set_ble_page_active(
                &interaction,
                active,
                now_ms);
            ESP_LOGI(TAG,
                     "BLE 页面固定 30 秒熄屏策略已%s",
                     active ? "启用" : "停用");
        }
        bool actual_display_on = false;
        watch_power_error_t runtime_error = WATCH_POWER_ERROR_NONE;
        if (take_runtime_result(&actual_display_on, &runtime_error))
        {
            const bool completed_screen_wake =
                !s_actual_display_on &&
                actual_display_on &&
                runtime_error == WATCH_POWER_ERROR_NONE;
            const bool completed_screen_sleep =
                s_actual_display_on &&
                !actual_display_on &&
                runtime_error == WATCH_POWER_ERROR_NONE;
            s_actual_display_on = actual_display_on;
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
            if (s_wrist_raise_trace_waiting_display &&
                actual_display_on)
            {
                ESP_LOGI(TAG,
                         "抬腕标定显示回报：成功=%d，WoM到显示=%llums，错误=%d",
                         runtime_error == WATCH_POWER_ERROR_NONE,
                         (unsigned long long)monotonic_elapsed_ms(
                             now_ms,
                             s_wrist_raise_trace_started_ms),
                         (int)runtime_error);
                s_wrist_raise_trace_waiting_display = false;
            }
#endif
            power_interaction_policy_reconcile_display(
                &interaction,
                actual_display_on);
            if (runtime_error == WATCH_POWER_ERROR_DISPLAY_FAILED)
            {
                /*
                 * ui_task 已在单次转换内执行唯一一次硬件重试。熄屏仍失败时
                 * 从当前时刻重新建立 idle 窗口，避免同一故障形成 10/20 ms
                 * 自动重投；自检强制亮屏失败则等待新的用户事件或自检会话。
                 */
                if (actual_display_on)
                {
                    interaction.last_activity_ms = now_ms;
                }
                else
                {
                    power_interaction_policy_note_display_failure(
                        &interaction);
                }
            }
            current_error =
                runtime_error == WATCH_POWER_ERROR_NONE
                    ? default_power_error()
                    : runtime_error;
            publish_power_fact(&interaction, current_error);
            if (completed_screen_wake)
            {
                next_battery_ms = now_ms;
#if LEGBOT_CAP_GPS_TIME && !LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
                const gps_acquisition_purpose_t purpose =
                    GPS_PURPOSE_TIME_SYNC;
                (void)gps_service_request_acquisition(
                    purpose, GPS_TRIGGER_SCREEN_WAKE, 0);
#endif
            }
            else if (completed_screen_sleep)
            {
#if LEGBOT_CAP_GPS_TIME && !LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
                (void)gps_service_pause_for_radio(0);
#endif
            }
        }
        power_interaction_input_t input = {
            .now_ms = now_ms,
            .local_activity =
                atomic_exchange(&s_local_activity_pending, false),
        };
        sample_selftest_fact(&input);
        const bool debounce_due =
            s_pwr_debounce_initialized &&
            s_pwr_debounce.candidate_active &&
            monotonic_deadline_reached(
                now_ms,
                s_pwr_debounce.candidate_since_ms +
                    s_pwr_debounce.debounce_ms);
        sample_pwr_input(&input,
                         pwr_sample_requested || debounce_due);
        pwr_sample_requested = false;
        sample_wrist_raise(&interaction, &display_mailbox, &input);
        const power_display_intent_t intent =
            power_interaction_policy_advance(&interaction, &input);
        if (intent != POWER_DISPLAY_INTENT_NONE ||
            input.local_activity ||
            (input.pwr_valid && input.pwr_pressed) ||
            (input.selftest_fact_valid && input.selftest_active))
        {
            cancel_wrist_raise();
        }
        if (intent == POWER_DISPLAY_INTENT_WAKE ||
            intent == POWER_DISPLAY_INTENT_SLEEP)
        {
            power_display_intent_mailbox_offer(&display_mailbox, intent);
            s_display_intent_retry_after_ms = now_ms;
        }
        else if (intent == POWER_DISPLAY_INTENT_NEXT_PAGE)
        {
            if (s_actual_display_on)
            {
                power_display_intent_mailbox_offer(&navigation_mailbox,
                                                   intent);
                s_navigation_intent_retry_after_ms = now_ms;
            }
            publish_power_fact(&interaction, current_error);
        }
        if (state_change_requested)
        {
            /* 自检事实不改变显示转换序号，只用既有屏幕事实唤醒 BLE owner。 */
            if (s_power_transition_sequence != 0U)
            {
                (void)ble_service_notify_screen_state(
                    s_actual_display_on,
                    s_power_transition_sequence);
            }
        }
        deliver_pending_intent(&display_mailbox,
                               &s_display_intent_retry_after_ms,
                               now_ms);
        deliver_pending_intent(&navigation_mailbox,
                               &s_navigation_intent_retry_after_ms,
                               now_ms);
        if (input.local_activity && intent == POWER_DISPLAY_INTENT_NONE)
        {
            publish_power_fact(&interaction, current_error);
        }
        if (monotonic_deadline_reached(now_ms, next_battery_ms))
        {
            poll_battery();
            next_battery_ms =
                now_ms +
                (uint64_t)compute_poll_period(s_actual_display_on);
        }
        owner_wait_ms = compute_owner_wait_ms(
            &interaction,
            &display_mailbox,
            &navigation_mailbox,
            next_battery_ms,
            monotonic_now_ms());
    }
}

static void poll_battery(void)
{
    cw2015_bsp_soc_t soc = {0};
    esp_err_t read_err = ESP_OK;
    if (!cw2015_bsp_is_initialized())
    {
        legbot_bsp_i2c_access_t access = {0};
        read_err = i2c_manager_get_access(&access);
        if (read_err == ESP_OK)
        {
            /* 临时上电或总线失败不会永久关闭电量能力，每个有界周期最多重试一次。 */
            read_err = cw2015_bsp_init(&access);
        }
    }
    if (read_err == ESP_OK)
    {
        read_err = cw2015_bsp_read_soc(&soc);
    }
    else
    {
        soc.error_code = (read_err == ESP_ERR_NOT_FOUND || read_err == ESP_ERR_INVALID_STATE)
                             ? CW2015_ERROR_DEVICE_MISSING
                             : (read_err == ESP_ERR_TIMEOUT ? CW2015_ERROR_BUS_BUSY
                                                            : CW2015_ERROR_INIT_FAILED);
    }
    bool valid = read_err == ESP_OK && soc.valid;
    uint8_t percent = valid ? (uint8_t)(soc.percent + 0.5f) : 0;
    if (percent > 100)
    {
        percent = 100;
    }

    if (valid)
    {
        s_consecutive_failures = 0;
    }
    else
    {
        ++s_consecutive_failures;
    }

    const watch_battery_update_t update = {
        .valid = valid,
        .percent = percent,
        .power_level = power_battery_level_policy_advance(
            &s_battery_level_policy,
            valid,
            percent),
        .attempted_at_ticks = xTaskGetTickCount(),
        .power_error_code = valid ? POWER_BATTERY_OK : POWER_BATTERY_UNAVAILABLE,
        .driver_error_code = soc.error_code != NULL ? soc.error_code : CW2015_ERROR_NOT_READY,
    };
    esp_err_t publish_err = state_service_publish_battery(
        &update,
        pdMS_TO_TICKS(POWER_SERVICE_STATE_QUEUE_TIMEOUT_MS));
    if (publish_err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "电量状态发布失败，稳定错误码=%s，错误=0x%x",
                 POWER_BATTERY_UNAVAILABLE,
                 (unsigned)publish_err);
    }
    else if (!valid)
    {
        ESP_LOGW(TAG,
                 "手环电量不可用，稳定错误码=%s，驱动错误码=%s，错误=0x%x",
                 POWER_BATTERY_UNAVAILABLE,
                 update.driver_error_code,
                 (unsigned)read_err);
    }
}

static uint32_t compute_poll_period(bool display_on)
{
    if (s_consecutive_failures >= POWER_SERVICE_BACKOFF_THRESHOLD_6)
    {
        return POWER_SERVICE_BACKOFF_MS_60S;
    }
    if (s_consecutive_failures >= POWER_SERVICE_BACKOFF_THRESHOLD_3)
    {
        return POWER_SERVICE_BACKOFF_MS_30S;
    }
    return display_on ? POWER_SERVICE_POLL_PERIOD_MS
                      : POWER_SERVICE_SCREEN_OFF_POLL_PERIOD_MS;
}

static watch_power_error_t default_power_error(void)
{
    return POWER_SERVICE_PWR_INPUT_EVIDENCE_VERIFIED
               ? WATCH_POWER_ERROR_NONE
               : WATCH_POWER_ERROR_PWR_INPUT_UNVERIFIED;
}

static uint64_t monotonic_elapsed_ms(uint64_t now_ms, uint64_t started_ms)
{
    const uint64_t elapsed = now_ms - started_ms;
    /*
     * 无符号减法自然覆盖真实回绕；大于 INT64_MAX 的差值视为时钟倒退，
     * 防止校时或错误 fixture 让 idle 或电量轮询立即越过截止线。
     */
    return elapsed <= (uint64_t)INT64_MAX ? elapsed : 0U;
}

static uint64_t monotonic_now_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000U;
}

static bool monotonic_deadline_reached(uint64_t now_ms,
                                       uint64_t deadline_ms)
{
    return now_ms == deadline_ms ||
           monotonic_elapsed_ms(now_ms, deadline_ms) > 0U;
}

static void sample_selftest_fact(power_interaction_input_t *input)
{
    if (input == NULL)
    {
        return;
    }
    if (watch_state_power_snapshot(&s_power_snapshot, 0) != ESP_OK)
    {
        /* 锁竞争只表示本轮事实未知，策略继续保留最近一次有效事实。 */
        input->selftest_fact_valid = false;
        return;
    }
    input->selftest_fact_valid = true;
    input->selftest_active = s_power_snapshot.selftest_active;
}

static bool submit_display_intent(power_display_intent_t intent)
{
    ui_service_request_type_t request_type = UI_SERVICE_REQUEST_MODEL_UPDATE;
    switch (intent)
    {
    case POWER_DISPLAY_INTENT_WAKE:
        request_type = UI_SERVICE_REQUEST_WAKE;
        break;
    case POWER_DISPLAY_INTENT_SLEEP:
        request_type = UI_SERVICE_REQUEST_SLEEP;
        break;
    case POWER_DISPLAY_INTENT_NEXT_PAGE:
        request_type = UI_SERVICE_REQUEST_NEXT_RESIDENT_PAGE;
        break;
    default:
        return false;
    }
    const ui_service_request_t request = {
        .type = request_type,
        .item_id = SELFTEST_ITEM_COUNT,
    };
    const esp_err_t err = ui_service_post_request(
        &request,
        pdMS_TO_TICKS(POWER_SERVICE_UI_QUEUE_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "PWR 显示或导航意图暂未进入 ui_task，动作=%s，错误=0x%x，将有界重试",
                 power_intent_name(intent),
                 (unsigned)err);
        return false;
    }
    return true;
}

static const char *power_intent_name(power_display_intent_t intent)
{
    switch (intent)
    {
    case POWER_DISPLAY_INTENT_WAKE:
        return "WAKE";
    case POWER_DISPLAY_INTENT_SLEEP:
        return "SLEEP";
    case POWER_DISPLAY_INTENT_NEXT_PAGE:
        return "NEXT_PAGE";
    default:
        return "INVALID";
    }
}

static void deliver_pending_intent(
    power_display_intent_mailbox_t *mailbox,
    uint64_t *retry_after_ms,
    uint64_t now_ms)
{
    if (mailbox == NULL || retry_after_ms == NULL)
    {
        return;
    }
    const power_display_intent_t pending =
        power_display_intent_mailbox_peek(mailbox);
    if (pending == POWER_DISPLAY_INTENT_NONE ||
        !monotonic_deadline_reached(now_ms, *retry_after_ms))
    {
        return;
    }
    if (submit_display_intent(pending))
    {
        power_display_intent_mailbox_complete(mailbox);
        *retry_after_ms = now_ms;
    }
    else
    {
        *retry_after_ms = now_ms + POWER_SERVICE_PENDING_RETRY_MS;
    }
}

static uint32_t configured_idle_timeout_ms(void)
{
    config_service_ui_preferences_t preferences = {0};
    const esp_err_t err = config_service_ui_preferences_read(
        &preferences,
        pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "读取自动熄屏偏好失败，使用 5 秒安全默认值，错误=0x%x",
                 (unsigned)err);
    }
    switch (preferences.screen_timeout)
    {
    case CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S:
        return POWER_SERVICE_IDLE_TIMEOUT_5S_MS;
    case CONFIG_SERVICE_UI_SCREEN_TIMEOUT_30S:
        return POWER_SERVICE_IDLE_TIMEOUT_30S_MS;
    case CONFIG_SERVICE_UI_SCREEN_TIMEOUT_15S:
        return POWER_SERVICE_IDLE_TIMEOUT_15S_MS;
    default:
        return POWER_SERVICE_IDLE_TIMEOUT_MS;
    }
}

static bool configured_raise_to_wake_enabled(void)
{
    config_service_ui_preferences_t preferences = {
        .raise_to_wake_enabled =
            CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED,
    };
    const esp_err_t err = config_service_ui_preferences_read(
        &preferences,
        pdMS_TO_TICKS(100));
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "读取抬腕亮屏偏好失败，保留默认关闭值，错误=0x%x",
                 (unsigned)err);
    }
    return preferences.raise_to_wake_enabled;
}

static void apply_pending_raise_setting(uint64_t now_ms)
{
    const unsigned pending =
        atomic_exchange(&s_raise_to_wake_pending, 0U);
    if (pending != POWER_SERVICE_RAISE_SETTING_DISABLED &&
        pending != POWER_SERVICE_RAISE_SETTING_ENABLED)
    {
        return;
    }

    const bool enabled =
        pending == POWER_SERVICE_RAISE_SETTING_ENABLED;
    if (s_raise_to_wake_enabled == enabled)
    {
        return;
    }
    if (enabled && !s_qmi_callback_registered)
    {
        const esp_err_t callback_error =
            ensure_qmi_callback_registered();
        if (callback_error != ESP_OK)
        {
            s_raise_to_wake_enabled = false;
            cancel_wrist_raise();
            ESP_LOGW(TAG,
                     "QMI ISR 回调重试失败，抬腕亮屏保持关闭，错误=0x%x",
                     (unsigned)callback_error);
            return;
        }
    }
    s_raise_to_wake_enabled = enabled;
    ESP_LOGI(TAG,
             "抬腕亮屏已%s",
             enabled ? "开启" : "关闭");
    if (!enabled)
    {
        cancel_wrist_raise();
        return;
    }
    if (!s_actual_display_on)
    {
        wrist_raise_policy_schedule_arm(
            &s_wrist_raise_policy,
            now_ms,
            WRIST_RAISE_SCREEN_OFF_REARM_MS);
    }
}

static void initialize_wrist_raise(bool actual_display_on,
                                   uint64_t now_ms)
{
    wrist_raise_policy_init(&s_wrist_raise_policy);
    s_wrist_raise_failures = 0U;
    s_wrist_raise_retry_after_ms = now_ms;
    s_next_wrist_sample_ms = now_ms;
    s_wrist_settle_samples_remaining = 0U;
    s_wrist_arm_baseline_active = false;
    s_wrist_arm_baseline_started_ms = now_ms;
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    s_wrist_raise_trace_candidate_active = false;
    s_wrist_raise_trace_waiting_display = false;
#endif
    if (s_raise_to_wake_enabled && !actual_display_on)
    {
        wrist_raise_policy_schedule_arm(
            &s_wrist_raise_policy,
            now_ms,
            WRIST_RAISE_SCREEN_OFF_REARM_MS);
        return;
    }
    cancel_wrist_raise();
}

static void sample_wrist_raise(
    const power_interaction_policy_t *interaction,
    const power_display_intent_mailbox_t *display_mailbox,
    power_interaction_input_t *input)
{
    if (interaction == NULL || display_mailbox == NULL ||
        input == NULL)
    {
        return;
    }

    const uint64_t now_ms = input->now_ms;
    const bool pwr_transition =
        input->pwr_valid &&
        ((!interaction->pwr_valid && input->pwr_pressed) ||
         (interaction->pwr_valid &&
          input->pwr_pressed != interaction->pwr_pressed));
    const bool higher_priority_activity =
        input->local_activity ||
        pwr_transition ||
        (input->pwr_valid && input->pwr_pressed) ||
        interaction->selftest_active ||
        (input->selftest_fact_valid && input->selftest_active);
    const bool display_transition_pending =
        power_display_intent_mailbox_peek(display_mailbox) !=
        POWER_DISPLAY_INTENT_NONE;
    if (!s_raise_to_wake_enabled ||
        s_actual_display_on ||
        interaction->display_on ||
        higher_priority_activity ||
        display_transition_pending)
    {
        cancel_wrist_raise();
        return;
    }

    if (!monotonic_deadline_reached(
            now_ms,
            s_wrist_raise_retry_after_ms))
    {
        return;
    }
    if (s_wrist_raise_policy.state ==
        WRIST_RAISE_STATE_DISARMED)
    {
        wrist_raise_policy_schedule_arm(
            &s_wrist_raise_policy,
            now_ms,
            WRIST_RAISE_SCREEN_OFF_REARM_MS);
    }
    if (s_wrist_raise_policy.state ==
        WRIST_RAISE_STATE_REARM_DELAY)
    {
        if (!wrist_raise_policy_arm_ready(
                &s_wrist_raise_policy,
                now_ms))
        {
            return;
        }
        const esp_err_t ready_err = ensure_qmi_ready();
        if (ready_err != ESP_OK)
        {
            record_wrist_raise_failure(
                now_ms,
                "初始化 QMI8658C",
                ready_err);
            return;
        }
        if (!s_wrist_arm_baseline_active &&
            qmi8658c_bsp_mode() != QMI8658C_BSP_MODE_STANDBY)
        {
            const esp_err_t standby_err =
                qmi8658c_bsp_set_mode(
                    QMI8658C_BSP_MODE_STANDBY);
            if (standby_err != ESP_OK)
            {
                record_wrist_raise_failure(
                    now_ms,
                    "进入待机模式",
                    standby_err);
                return;
            }
        }
        if (s_wrist_raise_policy.anchor_valid)
        {
            const esp_err_t arm_err =
                arm_wrist_raise_wom(now_ms);
            if (arm_err != ESP_OK)
            {
                record_wrist_raise_failure(
                    now_ms,
                    "续用姿态基线武装 WoM",
                    arm_err);
            }
            return;
        }
        if (!s_wrist_arm_baseline_active)
        {
            const esp_err_t classify_err =
                qmi8658c_bsp_set_mode(
                    QMI8658C_BSP_MODE_ACCEL_CLASSIFY);
            if (classify_err != ESP_OK)
            {
                record_wrist_raise_failure(
                    now_ms,
                    "进入武装前姿态采样模式",
                    classify_err);
                return;
            }
            s_wrist_arm_baseline_active = true;
            s_wrist_arm_baseline_started_ms = now_ms;
            s_wrist_settle_samples_remaining =
                POWER_SERVICE_ACCEL_SETTLE_SAMPLES;
            s_next_wrist_sample_ms = now_ms;
            return;
        }

        if (monotonic_elapsed_ms(
                now_ms,
                s_wrist_arm_baseline_started_ms) >=
            POWER_SERVICE_ARM_BASELINE_TIMEOUT_MS)
        {
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
            ESP_LOGI(TAG,
                     "抬腕标定基线：未在 %lu ms 内取得稳定姿态，按无基线模式武装",
                     (unsigned long)POWER_SERVICE_ARM_BASELINE_TIMEOUT_MS);
#endif
            const esp_err_t arm_err = arm_wrist_raise_wom(now_ms);
            if (arm_err != ESP_OK)
            {
                record_wrist_raise_failure(
                    now_ms,
                    "基线超时后武装 WoM",
                    arm_err);
            }
            return;
        }
        if (!monotonic_deadline_reached(
                now_ms,
                s_next_wrist_sample_ms))
        {
            return;
        }

        qmi8658c_bsp_accel_sample_t raw_sample = {0};
        const esp_err_t sample_err =
            qmi8658c_bsp_read_locked_accel(&raw_sample);
        if (sample_err == ESP_ERR_NOT_FINISHED)
        {
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
            ESP_LOGI(TAG, "抬腕标定基线：等待新的加速度样本");
#endif
            s_next_wrist_sample_ms =
                now_ms + POWER_SERVICE_ACTIVE_RETRY_MS;
            return;
        }
        if (sample_err != ESP_OK || !raw_sample.valid)
        {
            record_wrist_raise_failure(
                now_ms,
                "读取武装前姿态",
                sample_err != ESP_OK
                    ? sample_err
                    : ESP_ERR_INVALID_RESPONSE);
            return;
        }
        s_next_wrist_sample_ms =
            now_ms + WRIST_RAISE_SAMPLE_PERIOD_MS;
        if (s_wrist_settle_samples_remaining > 0U)
        {
            --s_wrist_settle_samples_remaining;
            return;
        }

        const wrist_raise_accel_t mapped =
            map_wrist_raise_sample(&raw_sample);
        const bool baseline_ready =
            wrist_raise_policy_add_arm_baseline_sample(
                &s_wrist_raise_policy,
                &mapped);
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        ESP_LOGI(TAG,
                 "抬腕标定基线样本：raw=(%d,%d,%d)，screen_mg=(%ld,%ld,%ld)，窗口=%u，稳定=%u",
                 raw_sample.accel_x,
                 raw_sample.accel_y,
                 raw_sample.accel_z,
                 (long)mapped.right_mg,
                 (long)mapped.top_mg,
                 (long)mapped.normal_mg,
                 (unsigned)s_wrist_raise_policy.window_count,
                 baseline_ready ? 1U : 0U);
#endif
        if (!baseline_ready)
        {
            return;
        }
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        ESP_LOGI(TAG,
                 "抬腕标定基线：screen_mg=(%ld,%ld,%ld)",
                 (long)s_wrist_raise_policy.anchor.right_mg,
                 (long)s_wrist_raise_policy.anchor.top_mg,
                 (long)s_wrist_raise_policy.anchor.normal_mg);
#endif
        const esp_err_t arm_err = arm_wrist_raise_wom(now_ms);
        if (arm_err != ESP_OK)
        {
            record_wrist_raise_failure(
                now_ms,
                "取得姿态基线后武装 WoM",
                arm_err);
        }
        return;
    }

    if (s_wrist_raise_policy.state ==
        WRIST_RAISE_STATE_WOM_ARMED)
    {
        if (!qmi8658c_bsp_take_wom_event())
        {
            return;
        }

        bool detected = false;
        const esp_err_t poll_err =
            qmi8658c_bsp_poll_wom_status(&detected);
        if (poll_err != ESP_OK)
        {
            record_wrist_raise_failure(
                now_ms,
                "确认 WoM 状态",
                poll_err);
            return;
        }
        s_wrist_raise_failures = 0U;
        if (!detected)
        {
            return;
        }
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        (void)gps_service_notify_motion(0);
#endif
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        s_wrist_raise_trace_started_ms = now_ms;
        s_wrist_raise_trace_candidate_active = true;
        s_wrist_raise_trace_waiting_display = false;
        ESP_LOGI(TAG,
                 "抬腕标定 WoM：时间=%llums",
                 (unsigned long long)now_ms);
#endif

        const esp_err_t classify_err =
            qmi8658c_bsp_set_mode(
                QMI8658C_BSP_MODE_ACCEL_CLASSIFY);
        if (classify_err != ESP_OK ||
            !wrist_raise_policy_begin_candidate(
                &s_wrist_raise_policy,
                now_ms))
        {
            record_wrist_raise_failure(
                now_ms,
                "进入抬腕分类模式",
                classify_err != ESP_OK
                    ? classify_err
                    : ESP_ERR_INVALID_STATE);
            return;
        }
        s_wrist_settle_samples_remaining =
            POWER_SERVICE_ACCEL_SETTLE_SAMPLES;
        s_next_wrist_sample_ms = now_ms;
    }

    if (s_wrist_raise_policy.state !=
        WRIST_RAISE_STATE_COLLECTING)
    {
        return;
    }
    if (wrist_raise_policy_candidate_expired(
            &s_wrist_raise_policy,
            now_ms))
    {
        reject_wrist_raise_candidate(now_ms);
        return;
    }
    if (!monotonic_deadline_reached(
            now_ms,
            s_next_wrist_sample_ms))
    {
        return;
    }

    qmi8658c_bsp_accel_sample_t raw_sample = {0};
    const esp_err_t sample_err =
        qmi8658c_bsp_read_locked_accel(&raw_sample);
    if (sample_err == ESP_ERR_NOT_FINISHED)
    {
        s_next_wrist_sample_ms =
            now_ms + POWER_SERVICE_ACTIVE_RETRY_MS;
        return;
    }
    if (sample_err != ESP_OK || !raw_sample.valid)
    {
        record_wrist_raise_failure(
            now_ms,
            "读取分类加速度",
            sample_err != ESP_OK
                ? sample_err
                : ESP_ERR_INVALID_RESPONSE);
        return;
    }
    s_next_wrist_sample_ms =
        now_ms + WRIST_RAISE_SAMPLE_PERIOD_MS;
    if (s_wrist_settle_samples_remaining > 0U)
    {
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        ESP_LOGI(TAG,
                 "抬腕校准丢弃启动样本：raw=(%d,%d,%d)，剩余=%lu",
                 raw_sample.accel_x,
                 raw_sample.accel_y,
                 raw_sample.accel_z,
                 (unsigned long)(s_wrist_settle_samples_remaining - 1U));
#endif
        --s_wrist_settle_samples_remaining;
        return;
    }
    const wrist_raise_accel_t mapped =
        map_wrist_raise_sample(&raw_sample);
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    ESP_LOGI(TAG,
             "抬腕校准样本：raw=(%d,%d,%d)，screen_mg=(%ld,%ld,%ld)，序号=%lu",
             raw_sample.accel_x,
             raw_sample.accel_y,
             raw_sample.accel_z,
             (long)mapped.right_mg,
             (long)mapped.top_mg,
             (long)mapped.normal_mg,
             (unsigned long)s_wrist_raise_policy.sample_count);
#endif
    const wrist_raise_result_t result =
        wrist_raise_policy_add_sample(
            &s_wrist_raise_policy,
            &mapped,
            now_ms);
    if (result == WRIST_RAISE_RESULT_CONFIRMED)
    {
        input->wrist_raise_activity = true;
        s_wrist_raise_failures = 0U;
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        ESP_LOGI(TAG,
                 "抬腕标定分类：结果=确认，耗时=%llums，样本=%lu",
                 (unsigned long long)monotonic_elapsed_ms(
                     now_ms,
                     s_wrist_raise_trace_started_ms),
                 (unsigned long)s_wrist_raise_policy.sample_count);
        s_wrist_raise_trace_candidate_active = false;
        s_wrist_raise_trace_waiting_display = true;
#endif
        ESP_LOGI(TAG, "抬腕姿态已确认，提交一次亮屏活动");
    }
    else if (result == WRIST_RAISE_RESULT_REJECTED)
    {
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        ESP_LOGI(TAG,
                 "抬腕标定分类：结果=拒绝，耗时=%llums，样本=%lu",
                 (unsigned long long)monotonic_elapsed_ms(
                     now_ms,
                     s_wrist_raise_trace_started_ms),
                 (unsigned long)s_wrist_raise_policy.sample_count);
        s_wrist_raise_trace_candidate_active = false;
#endif
        reject_wrist_raise_candidate(now_ms);
    }
}

static void cancel_wrist_raise(void)
{
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    if (s_wrist_raise_trace_candidate_active)
    {
        ESP_LOGI(TAG, "抬腕标定分类：结果=取消");
        s_wrist_raise_trace_candidate_active = false;
    }
#endif
    s_wrist_arm_baseline_active = false;
    wrist_raise_policy_disarm(&s_wrist_raise_policy);
    (void)qmi8658c_bsp_take_wom_event();
    if (!qmi8658c_bsp_is_initialized() ||
        qmi8658c_bsp_mode() == QMI8658C_BSP_MODE_STANDBY)
    {
        return;
    }
    const esp_err_t err =
        qmi8658c_bsp_set_mode(
            QMI8658C_BSP_MODE_STANDBY);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "取消抬腕监测后进入待机模式失败，错误=0x%x",
                 (unsigned)err);
    }
}

static void shutdown_wrist_raise(void)
{
    s_raise_to_wake_enabled = false;
    cancel_wrist_raise();
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    s_wrist_raise_trace_waiting_display = false;
#endif
}

static void reject_wrist_raise_candidate(uint64_t now_ms)
{
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    if (s_wrist_raise_trace_candidate_active)
    {
        ESP_LOGI(TAG,
                 "抬腕标定分类：结果=超时或拒绝，耗时=%llums，样本=%lu",
                 (unsigned long long)monotonic_elapsed_ms(
                     now_ms,
                     s_wrist_raise_trace_started_ms),
                 (unsigned long)s_wrist_raise_policy.sample_count);
        s_wrist_raise_trace_candidate_active = false;
    }
#endif
    s_wrist_arm_baseline_active = false;
    wrist_raise_policy_reject(&s_wrist_raise_policy, now_ms);
    if (qmi8658c_bsp_is_initialized())
    {
        const esp_err_t err =
            qmi8658c_bsp_set_mode(
                QMI8658C_BSP_MODE_STANDBY);
        if (err != ESP_OK)
        {
            record_wrist_raise_failure(
                now_ms,
                "拒绝候选后进入待机模式",
                err);
            return;
        }
    }
    wrist_raise_policy_schedule_retry_arm(
        &s_wrist_raise_policy,
        now_ms,
        WRIST_RAISE_REJECTED_REARM_MS);
}

static void record_wrist_raise_failure(uint64_t now_ms,
                                       const char *operation,
                                       esp_err_t err)
{
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    if (s_wrist_raise_trace_candidate_active)
    {
        ESP_LOGI(TAG,
                 "抬腕标定分类：结果=硬件失败，步骤=%s，错误=0x%x",
                 operation != NULL ? operation : "未知",
                 (unsigned)err);
        s_wrist_raise_trace_candidate_active = false;
    }
#endif
    s_wrist_arm_baseline_active = false;
    if (s_wrist_raise_failures < UINT32_MAX)
    {
        ++s_wrist_raise_failures;
    }
    const uint32_t retry_ms =
        wrist_raise_retry_delay_ms(s_wrist_raise_failures);
    s_wrist_raise_retry_after_ms = now_ms + retry_ms;
    wrist_raise_policy_schedule_arm(
        &s_wrist_raise_policy,
        now_ms,
        retry_ms);
    if (qmi8658c_bsp_is_initialized() &&
        qmi8658c_bsp_mode() != QMI8658C_BSP_MODE_STANDBY)
    {
        /* 故障退避期间尽力关闭传感器，恢复失败仍保持软件 fail-closed。 */
        (void)qmi8658c_bsp_set_mode(
            QMI8658C_BSP_MODE_STANDBY);
    }
    ESP_LOGW(TAG,
             "抬腕能力暂时关闭，步骤=%s，连续失败=%lu，错误=0x%x，%lu ms 后重试",
             operation != NULL ? operation : "未知",
             (unsigned long)s_wrist_raise_failures,
             (unsigned)err,
             (unsigned long)retry_ms);
}

static esp_err_t arm_wrist_raise_wom(uint64_t now_ms)
{
    const esp_err_t err =
        qmi8658c_bsp_set_mode(QMI8658C_BSP_MODE_WOM_MONITOR);
    if (err != ESP_OK)
    {
        return err;
    }
    wrist_raise_policy_mark_armed(&s_wrist_raise_policy, now_ms);
    s_wrist_arm_baseline_active = false;
    return ESP_OK;
}

static uint32_t wrist_raise_retry_delay_ms(uint32_t failure_count)
{
    if (failure_count >= 5U)
    {
        return POWER_SERVICE_RAISE_RETRY_MS_30S;
    }
    if (failure_count >= 4U)
    {
        return POWER_SERVICE_RAISE_RETRY_MS_5S;
    }
    if (failure_count >= 3U)
    {
        return POWER_SERVICE_RAISE_RETRY_MS_1S;
    }
    return 0U;
}

static esp_err_t ensure_qmi_ready(void)
{
    if (qmi8658c_bsp_is_initialized())
    {
        return ESP_OK;
    }
    legbot_bsp_i2c_access_t access = {0};
    const esp_err_t access_err = i2c_manager_get_access(&access);
    if (access_err != ESP_OK)
    {
        return access_err;
    }
    return qmi8658c_bsp_init(&access);
}

static wrist_raise_accel_t map_wrist_raise_sample(
    const qmi8658c_bsp_accel_sample_t *sample)
{
    /*
     * 当前板级安装初值：QMI X=屏幕向右，-Y=屏幕向上，Z=表盘法向。
     * 校准构建会同时输出 raw 和 screen_mg，量产前用静态六面姿态复核符号。
     */
    return (wrist_raise_accel_t){
        .right_mg = accel_raw_to_mg(sample->accel_x),
        .top_mg = -accel_raw_to_mg(sample->accel_y),
        .normal_mg = accel_raw_to_mg(sample->accel_z),
    };
}

static int32_t accel_raw_to_mg(int16_t raw)
{
    return (int32_t)(((int64_t)raw * 1000) /
                     POWER_SERVICE_ACCEL_COUNTS_PER_G);
}

static void sample_pwr_input(power_interaction_input_t *input,
                             bool read_gpio)
{
    if (input == NULL)
    {
        return;
    }
    if (read_gpio)
    {
        legbot_key_state_t keys = {0};
        const esp_err_t err = key_read_state(&keys);
        if (err != ESP_OK)
        {
            if (!s_pwr_read_warning_logged)
            {
                ESP_LOGW(TAG,
                         "运行态 PWR 原始输入读取失败，保持 fail-closed，错误=0x%x",
                         (unsigned)err);
                s_pwr_read_warning_logged = true;
            }
        }
        else
        {
#if POWER_SERVICE_PWR_INPUT_EVIDENCE_VERIFIED
            const bool raw_pressed =
                keys.pwr_level == POWER_SERVICE_PWR_ACTIVE_LEVEL;
            bool stable_changed = false;
            if (!s_pwr_debounce_initialized)
            {
                power_pwr_debounce_init(&s_pwr_debounce,
                                        raw_pressed,
                                        POWER_SERVICE_PWR_DEBOUNCE_MS);
                s_pwr_debounce_initialized = true;
                ESP_LOGI(TAG,
                         "运行态 PWR 已启用，按下电平=%u，去抖=%u ms",
                         POWER_SERVICE_PWR_ACTIVE_LEVEL,
                         POWER_SERVICE_PWR_DEBOUNCE_MS);
            }
            else
            {
                (void)power_pwr_debounce_advance(
                    &s_pwr_debounce,
                    raw_pressed,
                    input->now_ms,
                    &stable_changed);
            }
            if (stable_changed)
            {
                ESP_LOGD(TAG,
                         "PWR 稳定状态已更新：%s",
                         s_pwr_debounce.stable_pressed ? "按下" : "释放");
            }
#else
            (void)keys;
            if (!s_pwr_read_warning_logged)
            {
                ESP_LOGW(TAG,
                         "运行态 PWR 有效电平与去抖尚无目标板证据，raw 输入仅采样不解释");
                s_pwr_read_warning_logged = true;
            }
#endif
        }
        const esp_err_t rearm_error =
            key_rearm_pwr_isr_for_next_level();
        if (rearm_error != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "PWR IO46 下一相反电平唤醒重武装失败，保持 fail-closed，错误=0x%x",
                     (unsigned)rearm_error);
        }
    }

#if POWER_SERVICE_PWR_INPUT_EVIDENCE_VERIFIED
    /*
     * 长按开机时应用可能在按键释放前启动；先观察稳定释放态，
     * 避免把开机键释放误判为一次运行态短按。
     */
    if (s_pwr_debounce_initialized &&
        !s_pwr_input_armed &&
        !s_pwr_debounce.stable_pressed)
    {
        s_pwr_input_armed = true;
    }
    input->pwr_valid = s_pwr_input_armed;
    input->pwr_pressed =
        s_pwr_debounce_initialized &&
        s_pwr_debounce.stable_pressed;
#endif
}

static bool take_runtime_result(bool *display_on,
                                watch_power_error_t *error)
{
    if (display_on == NULL || error == NULL)
    {
        return false;
    }
    const unsigned encoded =
        atomic_exchange(&s_runtime_result_pending, 0U);
    const unsigned error_value =
        encoded & POWER_SERVICE_RUNTIME_RESULT_ERROR_MASK;
    if (error_value == 0U)
    {
        return false;
    }
    *display_on =
        (encoded & POWER_SERVICE_RUNTIME_RESULT_DISPLAY_ON) != 0U;
    *error = (watch_power_error_t)(error_value - 1U);
    return true;
}

static void publish_power_fact(const power_interaction_policy_t *policy,
                               watch_power_error_t error)
{
    ++s_power_transition_sequence;
    if (s_power_transition_sequence == 0U)
    {
        ++s_power_transition_sequence;
    }
    s_pending_power_update = (watch_power_update_t){
        .screen_state = policy->display_on
                            ? WATCH_SCREEN_STATE_ON
                            : WATCH_SCREEN_STATE_OFF,
        .last_local_activity_ms = policy->last_activity_ms,
        .transition_sequence = s_power_transition_sequence,
        .error = error,
    };
    s_pending_power_update_valid = true;
    (void)ble_service_notify_screen_state(
        s_pending_power_update.screen_state == WATCH_SCREEN_STATE_ON,
        s_pending_power_update.transition_sequence);
    const uint64_t now_ms = monotonic_now_ms();
    s_power_fact_retry_after_ms = now_ms;
    retry_pending_power_fact(now_ms);
}

static void retry_pending_power_fact(uint64_t now_ms)
{
    if (!s_pending_power_update_valid ||
        !monotonic_deadline_reached(
            now_ms,
            s_power_fact_retry_after_ms))
    {
        return;
    }
    const esp_err_t err = state_service_publish_power(
        &s_pending_power_update,
        pdMS_TO_TICKS(POWER_SERVICE_STATE_QUEUE_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        s_pending_power_update_valid = false;
        return;
    }
    ESP_LOGW(TAG,
             "正交屏幕状态暂未进入 state_task，序号=%lu，错误=0x%x，将有界重试",
             (unsigned long)s_pending_power_update.transition_sequence,
             (unsigned)err);
    s_power_fact_retry_after_ms =
        now_ms + POWER_SERVICE_PENDING_RETRY_MS;
}

static void pwr_edge_isr_callback(void *context)
{
    QueueHandle_t queue = (QueueHandle_t)context;
    if (queue == NULL)
    {
        return;
    }
    /* 队列只负责唤醒；latest 边沿先保留，队列满也不会静默丢失。 */
    atomic_store(&s_pwr_edge_pending, true);
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_PWR_EDGE,
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(queue,
                            &message,
                            &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

static void qmi_wake_isr_callback(void *context)
{
    QueueHandle_t queue = (QueueHandle_t)context;
    if (queue == NULL)
    {
        return;
    }
    const legbot_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_QMI_WAKE,
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(queue,
                            &message,
                            &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

static bool register_input_callbacks(QueueHandle_t queue)
{
    s_pwr_callback_registered = false;
    s_qmi_callback_registered = false;
    esp_err_t err = key_register_pwr_isr_callback(
        pwr_edge_isr_callback,
        queue);
    if (err == ESP_OK)
    {
        s_pwr_callback_registered = true;
    }
    else
    {
        ESP_LOGW(TAG,
                 "PWR IO46 ISR 回调登记失败，运行态短按保持 fail-closed，错误=0x%x",
                 (unsigned)err);
    }

    err = ensure_qmi_callback_registered();
    if (err == ESP_OK)
    {
        s_qmi_callback_registered = true;
    }
    else
    {
        ESP_LOGW(TAG,
                 "QMI IO41 ISR 回调登记失败，错误=0x%x",
                 (unsigned)err);
    }
    return s_pwr_callback_registered && s_qmi_callback_registered;
}

static esp_err_t ensure_qmi_callback_registered(void)
{
    if (s_qmi_callback_registered)
    {
        return ESP_OK;
    }
    if (s_power_owner_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t error = qmi8658c_bsp_register_wake_isr_callback(
        qmi_wake_isr_callback,
        s_power_owner_queue);
    if (error == ESP_OK)
    {
        s_qmi_callback_registered = true;
    }
    return error;
}

static void finish_power_run(void)
{
    if (s_pwr_callback_registered)
    {
        const esp_err_t err = key_unregister_pwr_isr_callback();
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "power_task 停止时 PWR ISR 解注册失败，错误=0x%x",
                     (unsigned)err);
        }
        s_pwr_callback_registered = false;
    }
    shutdown_wrist_raise();
    if (s_qmi_callback_registered)
    {
        qmi8658c_bsp_unregister_wake_isr_callback();
        s_qmi_callback_registered = false;
    }
    s_power_owner_queue = NULL;
}

static uint64_t wrist_raise_deadline_ms(
    const power_interaction_policy_t *interaction,
    const power_display_intent_mailbox_t *display_mailbox,
    uint64_t now_ms,
    bool *active)
{
    *active = false;
    if (!s_raise_to_wake_enabled || s_actual_display_on ||
        interaction->display_on || interaction->selftest_active ||
        power_display_intent_mailbox_peek(display_mailbox) !=
            POWER_DISPLAY_INTENT_NONE)
    {
        return 0U;
    }
    if (s_wrist_raise_policy.state == WRIST_RAISE_STATE_DISARMED ||
        s_wrist_raise_policy.state == WRIST_RAISE_STATE_CONFIRMED)
    {
        *active = true;
        return now_ms;
    }
    if (s_wrist_raise_policy.state == WRIST_RAISE_STATE_REARM_DELAY)
    {
        *active = true;
        if (s_wrist_arm_baseline_active)
        {
            const uint64_t baseline_timeout =
                s_wrist_arm_baseline_started_ms +
                POWER_SERVICE_ARM_BASELINE_TIMEOUT_MS;
            return s_next_wrist_sample_ms < baseline_timeout
                       ? s_next_wrist_sample_ms
                       : baseline_timeout;
        }
        uint64_t deadline =
            s_wrist_raise_policy.state_since_ms +
            s_wrist_raise_policy.rearm_delay_ms;
        if (deadline < s_wrist_raise_retry_after_ms)
        {
            deadline = s_wrist_raise_retry_after_ms;
        }
        return deadline;
    }
    if (s_wrist_raise_policy.state == WRIST_RAISE_STATE_COLLECTING)
    {
        *active = true;
        const uint64_t classify_timeout =
            s_wrist_raise_policy.state_since_ms +
            WRIST_RAISE_COLLECTION_TIMEOUT_MS;
        return s_next_wrist_sample_ms < classify_timeout
                   ? s_next_wrist_sample_ms
                   : classify_timeout;
    }
    return 0U;
}

static uint32_t compute_owner_wait_ms(
    const power_interaction_policy_t *interaction,
    const power_display_intent_mailbox_t *display_mailbox,
    const power_display_intent_mailbox_t *navigation_mailbox,
    uint64_t next_battery_ms,
    uint64_t now_ms)
{
    power_event_deadlines_t deadlines = {
        .immediate_work =
            atomic_load(&s_local_activity_pending) ||
            atomic_load(&s_pwr_edge_pending) ||
            atomic_load(&s_runtime_result_pending) != 0U ||
            atomic_load(&s_idle_timeout_pending) != 0U ||
            atomic_load(&s_ble_page_active_pending) != 0U ||
            atomic_load(&s_raise_to_wake_pending) != 0U,
        .battery = {
            .deadline_ms = next_battery_ms,
            .active = true,
        },
    };
    if (interaction->display_on && !interaction->selftest_active &&
        !(interaction->pwr_valid && interaction->pwr_pressed))
    {
        const uint32_t idle_timeout_ms =
            interaction->ble_page_idle_timeout_active
                ? POWER_SERVICE_BLE_PAGE_IDLE_TIMEOUT_MS
                : interaction->idle_timeout_ms;
        deadlines.idle = (power_event_deadline_t){
            .deadline_ms = interaction->last_activity_ms +
                           idle_timeout_ms,
            .active = true,
        };
    }
    if (s_pwr_debounce_initialized &&
        s_pwr_debounce.candidate_active)
    {
        deadlines.pwr_debounce = (power_event_deadline_t){
            .deadline_ms = s_pwr_debounce.candidate_since_ms +
                           s_pwr_debounce.debounce_ms,
            .active = true,
        };
    }
    deadlines.wrist_raise.deadline_ms = wrist_raise_deadline_ms(
        interaction,
        display_mailbox,
        now_ms,
        &deadlines.wrist_raise.active);

    if (s_pending_power_update_valid)
    {
        deadlines.pending_delivery = (power_event_deadline_t){
            .deadline_ms = s_power_fact_retry_after_ms,
            .active = true,
        };
    }
    if (power_display_intent_mailbox_peek(display_mailbox) !=
        POWER_DISPLAY_INTENT_NONE)
    {
        if (!deadlines.pending_delivery.active ||
            s_display_intent_retry_after_ms <
                deadlines.pending_delivery.deadline_ms)
        {
            deadlines.pending_delivery = (power_event_deadline_t){
                .deadline_ms = s_display_intent_retry_after_ms,
                .active = true,
            };
        }
    }
    if (power_display_intent_mailbox_peek(navigation_mailbox) !=
        POWER_DISPLAY_INTENT_NONE)
    {
        if (!deadlines.pending_delivery.active ||
            s_navigation_intent_retry_after_ms <
                deadlines.pending_delivery.deadline_ms)
        {
            deadlines.pending_delivery = (power_event_deadline_t){
                .deadline_ms = s_navigation_intent_retry_after_ms,
                .active = true,
            };
        }
    }
    return power_event_deadlines_wait_ms(now_ms, &deadlines);
}

static TickType_t owner_wait_ticks(uint32_t wait_ms)
{
    if (wait_ms == POWER_EVENT_WAIT_FOREVER_MS)
    {
        return portMAX_DELAY;
    }
    const TickType_t ticks = pdMS_TO_TICKS(wait_ms);
    return wait_ms > 0U && ticks == 0U ? 1U : ticks;
}
