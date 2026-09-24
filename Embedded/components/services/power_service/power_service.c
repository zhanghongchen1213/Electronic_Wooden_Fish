/**
 * @file     power_service.c
 * @brief    PWR_INT/BOOT0 板级输入边界服务实现
 * @details  ISR 只做时间戳、原子状态更新和 ISR-safe typed 消息投递；去抖、低电平时长和
 *           typed 结论交接全部在 power_task 上下文完成。BOOT0 运行态短按切换自动模式，
 *           周期投递由 esp_timer 回调只入队、power_task 再 submit（AD-7）。
 *           服务不读取或推断充电状态，不驱动 LTC2954 KILL，也不调用任何软件关机 API。
 * @author   ZHC
 * @date     2026-07-09
 */

#include "power_service.h"

#include <string.h>

#include "cw2015_bsp.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "key.h"
#include "power_core_path_policy.h"
#include "state_service.h"
#include "sync_service.h"
#include "tap_input_service.h"
#include "device_nav_service.h"

static const char *TAG = "PWR_BOOT";
static const char *TAG_BATT = "PWR_BATT";
static const char *TAG_CORE = "PWR_CORE";

/** 电量读失败日志限频单调毫秒。 */
#define POWER_BATTERY_FAIL_LOG_INTERVAL_MS 60000U
#define POWER_BATTERY_OK_CODE "POWER_BATTERY_OK"
#define POWER_BATTERY_UNAVAILABLE_CODE "POWER_BATTERY_UNAVAILABLE"

/** power_task 私有 typed 消息类型。 */
typedef enum
{
    POWER_SERVICE_MESSAGE_EDGE_PWR = 1U,     /**< PWR_INT 原始边沿。 */
    POWER_SERVICE_MESSAGE_EDGE_BOOT = 2U,    /**< BOOT0 原始边沿。 */
    POWER_SERVICE_MESSAGE_AUTO_PERIOD = 3U,  /**< 自动模式周期到期投递。 */
} power_service_message_type_t;

/** power_task 私有 ISR-safe 消息载荷。 */
typedef struct
{
    uint32_t type;  /**< 消息类型。 */
    uint32_t level; /**< 边沿时刻的原始电平。 */
    uint32_t ticks; /**< 边沿时刻的 ISR tick；AUTO_PERIOD 未使用。 */
} power_service_message_t;

/** 服务状态机。 */
typedef enum
{
    POWER_SERVICE_IDLE = 0,    /**< 尚未创建队列。 */
    POWER_SERVICE_PREPARED,    /**< 队列就绪，等待框架创建任务。 */
    POWER_SERVICE_RUNNING,     /**< power_task 正在运行。 */
} power_service_phase_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_snapshot_mutex;
static power_service_phase_t s_phase;
static bool s_isr_registered;
static esp_timer_handle_t s_auto_timer;
/** 定时器回调入队失败次数；仅在 power_task 内消费并打中文日志（AD-7）。 */
static volatile uint32_t s_auto_period_queue_drops;

static ewf_power_boot_policy_t s_policy;
static ewf_power_auto_mode_state_t s_auto_mode;
static power_service_snapshot_t s_snapshot;
static ewf_power_battery_level_state_t s_battery_level;
static bool s_battery_have_valid_sample;
static uint8_t s_battery_last_percent;
static uint32_t s_battery_last_poll_ms;
static uint32_t s_battery_last_fail_log_ms;
/** CRITICAL 亮度上限只读建议（供 nav/Epic 3 消费；不直改 LVGL/不改 NVS 默认）。 */
static bool s_brightness_cap_suggested;
static uint8_t s_brightness_cap_value;
static bool s_cw2015_ready;

static void power_service_pwr_isr(void *context);
static void power_service_boot_isr(void *context);
static void power_service_auto_timer_cb(void *arg);
static void power_service_publish_snapshot(const power_service_snapshot_t *snapshot);
static const char *power_service_event_name(ewf_power_boot_event_kind_t kind);
static void power_service_fill_auto_snapshot(power_service_snapshot_t *snapshot);
static void power_service_apply_auto_decision(
    const ewf_power_auto_mode_decision_t *decision,
    power_service_snapshot_t *snapshot);
static void power_service_submit_automatic_tap(uint32_t at_ms);
static void power_service_stop_auto_timer(void);
static esp_err_t power_service_start_auto_timer(void);
static void power_service_handle_auto_period(power_service_snapshot_t *snapshot);
static void power_service_handle_runtime_tap(
    const ewf_power_boot_event_t *event,
    power_service_snapshot_t *snapshot);
static void power_service_ensure_cw2015(void);
static void power_service_poll_battery(uint32_t now_ms);

esp_err_t power_service_init_contracts(void)
{
    if (s_queue != NULL && s_snapshot_mutex != NULL)
    {
        return ESP_OK;
    }
    if (s_queue == NULL)
    {
        s_queue = xQueueCreate(POWER_SERVICE_QUEUE_DEPTH,
                               sizeof(power_service_message_t));
        if (s_queue == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
    }
    if (s_snapshot_mutex == NULL)
    {
        s_snapshot_mutex = xSemaphoreCreateMutex();
        if (s_snapshot_mutex == NULL)
        {
            vQueueDelete(s_queue);
            s_queue = NULL;
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

esp_err_t power_service_prepare_run(void)
{
    if (s_queue == NULL || s_snapshot_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_phase == POWER_SERVICE_RUNNING)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_phase = POWER_SERVICE_PREPARED;
    return ESP_OK;
}

void power_service_cancel_prepared_run(void)
{
    if (s_phase == POWER_SERVICE_PREPARED)
    {
        s_phase = POWER_SERVICE_IDLE;
    }
}

esp_err_t power_service_deinit_contracts(void)
{
    if (s_phase == POWER_SERVICE_RUNNING)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_queue != NULL)
    {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }
    if (s_snapshot_mutex != NULL)
    {
        vSemaphoreDelete(s_snapshot_mutex);
        s_snapshot_mutex = NULL;
    }
    s_phase = POWER_SERVICE_IDLE;
    return ESP_OK;
}

QueueHandle_t power_service_queue(void)
{
    return s_queue;
}

esp_err_t power_service_run(void)
{
    if (s_queue == NULL || s_phase != POWER_SERVICE_PREPARED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.last_event = EWF_POWER_BOOT_EVENT_NONE;
    ewf_power_boot_policy_reset(&s_policy);
    /* 自动模式只存在运行内存，重启/run 入口默认关闭，不从 NVS 恢复。 */
    ewf_power_auto_mode_reset(&s_auto_mode);
    ewf_power_battery_level_reset(&s_battery_level);
    s_battery_have_valid_sample = false;
    s_battery_last_percent = 50U;
    s_battery_last_poll_ms = 0U;
    s_battery_last_fail_log_ms = 0U;
    s_brightness_cap_suggested = false;
    s_brightness_cap_value = 100U;
    s_cw2015_ready = false;
    power_service_ensure_cw2015();

    const esp_timer_create_args_t auto_timer_args = {
        .callback = &power_service_auto_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "pwr_auto",
        /* 不跳过未处理到期：避免与队列满叠加导致整拍丢失。 */
        .skip_unhandled_events = false,
    };
    esp_err_t err = esp_timer_create(&auto_timer_args, &s_auto_timer);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "自动模式周期定时器创建失败，错误=%s", esp_err_to_name(err));
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }

    err = key_register_pwr_isr_callback(power_service_pwr_isr, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PWR_INT 边沿中断登记失败，错误=%s", esp_err_to_name(err));
        (void)esp_timer_delete(s_auto_timer);
        s_auto_timer = NULL;
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }
    err = key_register_boot_isr_callback(power_service_boot_isr, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BOOT0 边沿中断登记失败，错误=%s", esp_err_to_name(err));
        (void)key_unregister_pwr_isr_callback();
        (void)esp_timer_delete(s_auto_timer);
        s_auto_timer = NULL;
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }
    s_isr_registered = true;
    s_phase = POWER_SERVICE_RUNNING;

    /* 结束启动采样窗口：BOOT0 若仍为低，本次属于下载/启动绑带，不产生运行态事件。 */
    legbot_key_state_t state = {0};
    err = key_read_state(&state);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "读取 PWR/BOOT 原始电平失败，错误=%s", esp_err_to_name(err));
        (void)key_unregister_boot_isr_callback();
        (void)key_unregister_pwr_isr_callback();
        (void)esp_timer_delete(s_auto_timer);
        s_auto_timer = NULL;
        s_isr_registered = false;
        s_phase = POWER_SERVICE_IDLE;
        return err;
    }
    ewf_power_boot_policy_arm_runtime(&s_policy, state.boot_level, 0U);
    ESP_LOGI(TAG,
             "PWR_INT/BOOT0 只读输入边界已就绪：PWR_INT=IO8 持续低有效、BOOT0=IO0 任意边沿；"
             "启动采样窗口已结束，主控不检测充电状态，也不驱动 KILL；"
             "自动模式默认关闭，周期=%u ms；CW2015 轮询周期=%u ms",
             (unsigned)EWF_POWER_AUTO_MODE_PERIOD_MS,
             (unsigned)EWF_POWER_BATTERY_POLL_PERIOD_MS);

    /* 先固化一份初始快照，供自检在没有任何边沿时也能观测到输入边界。 */
    power_service_snapshot_t initial = {0};
    initial.pwr_level = state.pwr_level;
    initial.boot_level = state.boot_level;
    initial.pwr_low = ewf_power_boot_policy_pwr_is_low(&s_policy);
    initial.pwr_low_duration_ms = 0U;
    initial.pwr_interval_count = s_policy.pwr_interval_count;
    initial.boot_tap_count = s_policy.boot_tap_count;
    initial.last_event = EWF_POWER_BOOT_EVENT_NONE;
    initial.last_event_duration_ms = 0U;
    initial.boot0_runtime_armed = s_policy.boot_runtime_armed;
    initial.isr_registered = s_isr_registered;
    power_service_fill_auto_snapshot(&initial);
    power_service_publish_snapshot(&initial);

    /* 启动后立即尝试一次 SOC，供 sync 字段与低电事实。 */
    power_service_poll_battery(0U);

    for (;;)
    {
        power_service_message_t message = {0};
        const TickType_t wait_ticks =
            pdMS_TO_TICKS(EWF_POWER_BATTERY_POLL_PERIOD_MS);
        const BaseType_t got = xQueueReceive(s_queue, &message, wait_ticks);
        const uint32_t now_ms =
            (uint32_t)(esp_timer_get_time() / 1000LL);
        power_service_poll_battery(now_ms);

        if (got != pdTRUE)
        {
            continue;
        }
        if (message.type == LEGBOT_SERVICE_STOP_MESSAGE)
        {
            break;
        }

        if (message.type == POWER_SERVICE_MESSAGE_AUTO_PERIOD)
        {
            power_service_snapshot_t snapshot = s_snapshot;
            power_service_handle_auto_period(&snapshot);
            power_service_publish_snapshot(&snapshot);
            continue;
        }

        legbot_key_state_t current = {0};
        if (key_read_state(&current) != ESP_OK)
        {
            ESP_LOGW(TAG, "边沿到达后读取原始电平失败，跳过本次交接");
            continue;
        }
        const uint32_t edge_ms = (uint32_t)pdTICKS_TO_MS(message.ticks);

        power_service_snapshot_t snapshot = s_snapshot;
        snapshot.pwr_level = current.pwr_level;
        snapshot.boot_level = current.boot_level;

        if (message.type == POWER_SERVICE_MESSAGE_EDGE_PWR)
        {
            ewf_power_boot_event_t event = {0};
            if (ewf_power_boot_policy_on_pwr_level(&s_policy,
                                                   (uint8_t)message.level,
                                                   edge_ms,
                                                   &event))
            {
                snapshot.last_event = event.kind;
                snapshot.last_event_duration_ms = event.duration_ms;
                ESP_LOGI(TAG,
                         "PWR_INT 结论=%s，低电平时长=%u ms，累计按压区间=%u",
                         power_service_event_name(event.kind),
                         (unsigned)event.duration_ms,
                         (unsigned)s_policy.pwr_interval_count);
                if (event.kind == EWF_POWER_BOOT_EVENT_PWR_INTERVAL)
                {
                    const esp_err_t nav_error = device_nav_service_on_pwr_interval(
                        event.duration_ms, event.at_ms);
                    if (nav_error != ESP_OK)
                    {
                        ESP_LOGW(TAG,
                                 "PWR 短按导航交接失败，错误=%s",
                                 esp_err_to_name(nav_error));
                    }
                }
            }
            (void)key_rearm_pwr_isr_for_next_level();
        }
        else if (message.type == POWER_SERVICE_MESSAGE_EDGE_BOOT)
        {
            ewf_power_boot_event_t event = {0};
            if (ewf_power_boot_policy_on_boot_level(&s_policy,
                                                    (uint8_t)message.level,
                                                    edge_ms,
                                                    &event))
            {
                snapshot.last_event = event.kind;
                snapshot.last_event_duration_ms = event.duration_ms;
                ESP_LOGI(TAG,
                         "BOOT0 结论=%s，区间时长=%u ms，累计运行态短按=%u",
                         power_service_event_name(event.kind),
                         (unsigned)event.duration_ms,
                         (unsigned)s_policy.boot_tap_count);
                if (event.kind == EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP)
                {
                    /* 短按只切换自动模式，不再直接 submit_automatic_tap（FR-E-011）。 */
                    power_service_handle_runtime_tap(&event, &snapshot);
                }
            }
            (void)key_rearm_boot_isr_for_next_level();
        }

        snapshot.pwr_low = ewf_power_boot_policy_pwr_is_low(&s_policy);
        snapshot.pwr_low_duration_ms =
            ewf_power_boot_policy_pwr_low_duration_ms(&s_policy, edge_ms);
        snapshot.pwr_interval_count = s_policy.pwr_interval_count;
        snapshot.boot_tap_count = s_policy.boot_tap_count;
        snapshot.boot0_runtime_armed = s_policy.boot_runtime_armed;
        snapshot.isr_registered = s_isr_registered;
        power_service_fill_auto_snapshot(&snapshot);
        power_service_publish_snapshot(&snapshot);
    }

    power_service_stop_auto_timer();
    if (s_auto_timer != NULL)
    {
        (void)esp_timer_delete(s_auto_timer);
        s_auto_timer = NULL;
    }
    ewf_power_auto_mode_reset(&s_auto_mode);
    (void)key_unregister_boot_isr_callback();
    (void)key_unregister_pwr_isr_callback();
    s_isr_registered = false;
    s_phase = POWER_SERVICE_IDLE;
    ESP_LOGW(TAG, "power_task 已收到 typed STOP 并退出，边沿中断已解注册");
    return ESP_OK;
}

esp_err_t power_service_request_stop(TickType_t timeout_ticks)
{
    /* 允许在任务尚未进入 run 循环前投递停止命令，避免启动竞态。 */
    if (s_queue == NULL ||
        (s_phase != POWER_SERVICE_RUNNING && s_phase != POWER_SERVICE_PREPARED))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const power_service_message_t message = {
        .type = LEGBOT_SERVICE_STOP_MESSAGE,
        .level = 0U,
        .ticks = 0U,
    };
    return xQueueSend(s_queue, &message, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t power_service_snapshot(power_service_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_snapshot_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_snapshot_mutex, 0) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *snapshot = s_snapshot;
    xSemaphoreGive(s_snapshot_mutex);
    return ESP_OK;
}

static void power_service_publish_snapshot(const power_service_snapshot_t *snapshot)
{
    if (s_snapshot_mutex == NULL ||
        xSemaphoreTake(s_snapshot_mutex, 0) != pdTRUE)
    {
        return;
    }
    s_snapshot = *snapshot;
    xSemaphoreGive(s_snapshot_mutex);
}

static const char *power_service_event_name(ewf_power_boot_event_kind_t kind)
{
    switch (kind)
    {
    case EWF_POWER_BOOT_EVENT_PWR_INTERVAL:
        return "PWR_INTERVAL";
    case EWF_POWER_BOOT_EVENT_PWR_GLITCH_IGNORED:
        return "PWR_GLITCH_IGNORED";
    case EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP:
        return "BOOT0_RUNTIME_TAP";
    case EWF_POWER_BOOT_EVENT_BOOT0_LONG_SUPPRESSED:
        return "BOOT0_LONG_SUPPRESSED";
    case EWF_POWER_BOOT_EVENT_BOOT0_GLITCH_IGNORED:
        return "BOOT0_GLITCH_IGNORED";
    case EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED:
        return "BOOT0_STARTUP_SUPPRESSED";
    default:
        return "NONE";
    }
}

static void power_service_fill_auto_snapshot(power_service_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }
    snapshot->auto_mode_enabled = s_auto_mode.enabled;
    snapshot->auto_mode_anchor_ms = s_auto_mode.anchor_ms;
    snapshot->auto_period_emit_count = s_auto_mode.emit_count;
}

static void power_service_stop_auto_timer(void)
{
    if (s_auto_timer == NULL)
    {
        return;
    }
    (void)esp_timer_stop(s_auto_timer);
}

static esp_err_t power_service_start_auto_timer(void)
{
    if (s_auto_timer == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    /* 先停再启，避免重复 start；周期自启动时刻起每 PERIOD 触发一次。 */
    (void)esp_timer_stop(s_auto_timer);
    return esp_timer_start_periodic(
        s_auto_timer,
        (uint64_t)EWF_POWER_AUTO_MODE_PERIOD_MS * 1000ULL);
}

static void power_service_apply_auto_decision(
    const ewf_power_auto_mode_decision_t *decision,
    power_service_snapshot_t *snapshot)
{
    if (decision == NULL)
    {
        return;
    }

    if (decision->action == EWF_POWER_AUTO_ACTION_ENABLE)
    {
        const esp_err_t timer_error = power_service_start_auto_timer();
        if (timer_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "自动模式周期定时器启动失败，错误=%s，回滚为关闭",
                     esp_err_to_name(timer_error));
            (void)ewf_power_auto_mode_force_disable(
                &s_auto_mode,
                decision->next_due_ms);
        }
        else
        {
            ESP_LOGI(TAG,
                     "自动模式已开启，锚点=%u ms，首拍=%u ms",
                     (unsigned)s_auto_mode.anchor_ms,
                     (unsigned)s_auto_mode.next_due_ms);
        }
    }
    else if (decision->action == EWF_POWER_AUTO_ACTION_DISABLE)
    {
        power_service_stop_auto_timer();
        ESP_LOGI(TAG, "自动模式已关闭，周期定时器已停止");
    }
    else if (decision->action == EWF_POWER_AUTO_ACTION_EMIT_TAP)
    {
        power_service_submit_automatic_tap(decision->emit_at_ms);
    }

    power_service_fill_auto_snapshot(snapshot);
}

static void power_service_submit_automatic_tap(uint32_t at_ms)
{
    const ewf_tap_event_t tap = {
        .source = EWF_TAP_SOURCE_AUTOMATIC_TAP,
        .sequence = tap_input_service_next_sequence(),
        .at_ms = at_ms,
        .candidate_confirmed = true,
        /* AUTOMATIC_TAP 不依赖木鱼触区/亮屏 gate；填合理默认。 */
        .screen_on = true,
        .wood_fish_hit = true,
    };
    ewf_tap_decision_t decision = {0};
    const esp_err_t error = tap_input_service_submit_automatic_tap(
        &tap, pdMS_TO_TICKS(10U), &decision);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "自动敲击未进入统一队列：原因=%s，错误=%s",
                 ewf_tap_reason_name(decision.reason),
                 esp_err_to_name(error));
    }
}

static void power_service_handle_runtime_tap(
    const ewf_power_boot_event_t *event,
    power_service_snapshot_t *snapshot)
{
    if (event == NULL)
    {
        return;
    }
    /* 锚点与周期核对共用 esp_timer 单调时钟，避免 tickless 下与 FreeRTOS tick 错位。 */
    (void)event->at_ms;
    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000LL);
    const ewf_power_auto_mode_decision_t decision =
        ewf_power_auto_mode_on_runtime_tap(&s_auto_mode, now_ms);
    power_service_apply_auto_decision(&decision, snapshot);
}

static void power_service_handle_auto_period(power_service_snapshot_t *snapshot)
{
    if (s_auto_period_queue_drops != 0U)
    {
        const uint32_t drops = s_auto_period_queue_drops;
        s_auto_period_queue_drops = 0U;
        ESP_LOGW(TAG,
                 "自动模式周期消息曾投递失败，丢弃次数=%u",
                 (unsigned)drops);
    }

    bool service_ready = false;
    bool completed = false;
    bool fault_locked = false;
    bool queue_full = false;
    const esp_err_t gate_error = state_service_read_tap_gate(
        &service_ready, &completed, &fault_locked, &queue_full);
    const uint32_t now_ms =
        (uint32_t)(esp_timer_get_time() / 1000LL);
    if (gate_error != ESP_OK)
    {
        /* 无法确认 completed 时保守停表，避免完成锁定后仍继续周期投递。 */
        ESP_LOGW(TAG,
                 "读取敲击 gate 失败，强制停止自动模式，错误=%s",
                 esp_err_to_name(gate_error));
        const ewf_power_auto_mode_decision_t decision =
            ewf_power_auto_mode_force_disable(&s_auto_mode, now_ms);
        power_service_apply_auto_decision(&decision, snapshot);
        return;
    }
    (void)service_ready;
    (void)fault_locked;
    (void)queue_full;

    const ewf_power_auto_mode_decision_t decision =
        ewf_power_auto_mode_on_period_due(&s_auto_mode, now_ms, completed);
    if (decision.action == EWF_POWER_AUTO_ACTION_DISABLE)
    {
        ESP_LOGI(TAG, "完成锁定强制停止自动模式");
    }
    power_service_apply_auto_decision(&decision, snapshot);
}

/**
 * @brief 自动模式周期定时器回调（ESP Timer 任务上下文）
 * @details 只投递短消息到 power_task；禁止在此调用 submit / NVS / 阻塞日志（AD-7）。
 *          查证：ESP-IDF v5.5.4 ESP Timer — esp_timer_create / start_periodic /
 *          stop / delete / get_time。
 */
static void power_service_auto_timer_cb(void *arg)
{
    (void)arg;
    if (s_queue == NULL)
    {
        return;
    }
    const power_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_AUTO_PERIOD,
        .level = 0U,
        .ticks = 0U,
    };
    /* 回调内禁止阻塞日志；失败只记计数，由 power_task 观测（AD-7）。 */
    if (xQueueSend(s_queue, &message, 0) != pdTRUE)
    {
        s_auto_period_queue_drops++;
    }
}

static void power_service_pwr_isr(void *context)
{
    (void)context;
    power_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_EDGE_PWR,
        .level = (uint32_t)gpio_get_level(LEGBOT_KEY_PWR_GPIO),
        .ticks = (uint32_t)xTaskGetTickCountFromISR(),
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static void power_service_boot_isr(void *context)
{
    (void)context;
    power_service_message_t message = {
        .type = POWER_SERVICE_MESSAGE_EDGE_BOOT,
        .level = (uint32_t)gpio_get_level(LEGBOT_KEY_BOOT_GPIO),
        .ticks = (uint32_t)xTaskGetTickCountFromISR(),
    };
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static void power_service_ensure_cw2015(void)
{
    if (s_cw2015_ready || cw2015_bsp_is_initialized())
    {
        s_cw2015_ready = true;
        return;
    }
    legbot_bsp_i2c_access_t access = {0};
    const esp_err_t access_error = i2c_manager_get_access(&access);
    if (access_error != ESP_OK)
    {
        ESP_LOGW(TAG_BATT, "获取 I2C 访问失败，电量轮询延后，错误=%s",
                 esp_err_to_name(access_error));
        return;
    }
    const esp_err_t init_error = cw2015_bsp_init(&access);
    if (init_error != ESP_OK)
    {
        ESP_LOGW(TAG_BATT, "CW2015 初始化失败，错误=%s；保留上次有效电量，不伪装 100%%",
                 esp_err_to_name(init_error));
        return;
    }
    s_cw2015_ready = true;
    ESP_LOGI(TAG_BATT, "CW2015 产品轮询路径已就绪（ALRT 不接，周期=%u ms）",
             (unsigned)EWF_POWER_BATTERY_POLL_PERIOD_MS);
}

static void power_service_poll_battery(uint32_t now_ms)
{
    if (s_battery_last_poll_ms != 0U &&
        (now_ms - s_battery_last_poll_ms) < EWF_POWER_BATTERY_POLL_PERIOD_MS &&
        now_ms != 0U)
    {
        return;
    }
    s_battery_last_poll_ms = (now_ms == 0U) ? 1U : now_ms;

    power_service_ensure_cw2015();

    cw2015_bsp_soc_t soc = {0};
    esp_err_t read_error = ESP_ERR_INVALID_STATE;
    if (s_cw2015_ready)
    {
        read_error = cw2015_bsp_read_soc(&soc);
    }
    else
    {
        soc.valid = false;
        soc.error_code = CW2015_ERROR_NOT_READY;
    }

    watch_battery_update_t update = {
        .valid = false,
        .percent = s_battery_last_percent,
        .power_level = (watch_power_level_t)s_battery_level.level,
        .attempted_at_ticks = xTaskGetTickCount(),
        .power_error_code = POWER_BATTERY_UNAVAILABLE_CODE,
        .driver_error_code =
            (soc.error_code != NULL) ? soc.error_code : CW2015_ERROR_NOT_READY,
    };

    if (read_error == ESP_OK && soc.valid)
    {
        uint8_t percent = (uint8_t)(soc.percent + 0.5f);
        if (percent > 100U)
        {
            percent = 100U;
        }
        s_battery_have_valid_sample = true;
        s_battery_last_percent = percent;
        const uint8_t level =
            ewf_power_battery_level_update(&s_battery_level, percent);
        update.valid = true;
        update.percent = percent;
        update.power_level = (watch_power_level_t)level;
        update.power_error_code = POWER_BATTERY_OK_CODE;
        update.driver_error_code = CW2015_ERROR_OK;

        (void)sync_service_set_battery_percent(percent);

        if (level == (uint8_t)WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY)
        {
            const ewf_power_core_path_input_t core_input = {
                .power_level = level,
                .persist_pending = false,
                .persist_inflight = false,
                .sync_window_requested = false,
                .feedback_pending = false,
            };
            const ewf_power_core_path_decision_t core =
                ewf_power_core_path_decide(&core_input);
            if (core.suggest_brightness_cap)
            {
                s_brightness_cap_suggested = true;
                s_brightness_cap_value = core.brightness_cap;
                ESP_LOGW(TAG_CORE,
                         "严重低电：建议亮度上限=%u（经 power 快照只读建议位，不直改 LVGL；"
                         "不主动断网；禁止软件关机）",
                         (unsigned)core.brightness_cap);
            }
            else
            {
                s_brightness_cap_suggested = false;
                s_brightness_cap_value = 100U;
            }
        }
        else
        {
            s_brightness_cap_suggested = false;
            s_brightness_cap_value = 100U;
        }
        ESP_LOGI(TAG_BATT,
                 "电量样本有效：percent=%u，power_level=%u",
                 (unsigned)percent,
                 (unsigned)level);
    }
    else
    {
        /* 读失败：保留上次有效值；valid=false；不得伪装 100% 或充电态。 */
        update.valid = false;
        update.percent = s_battery_last_percent;
        update.power_level = (watch_power_level_t)s_battery_level.level;
        if ((now_ms - s_battery_last_fail_log_ms) >= POWER_BATTERY_FAIL_LOG_INTERVAL_MS ||
            s_battery_last_fail_log_ms == 0U)
        {
            s_battery_last_fail_log_ms = now_ms;
            ESP_LOGW(TAG_BATT,
                     "电量读取失败，保留上次有效值=%u，valid=false，驱动码=%s，"
                     "错误=%s（无有效样本前 sync 仍可占位；have_valid=%d）",
                     (unsigned)s_battery_last_percent,
                     update.driver_error_code,
                     esp_err_to_name(read_error),
                     (int)s_battery_have_valid_sample);
        }
    }

    /* 回写快照只读建议位，供 settings/nav（Epic 3）消费，本 Story 不改产品默认。 */
    if (s_snapshot_mutex != NULL &&
        xSemaphoreTake(s_snapshot_mutex, 0) == pdTRUE)
    {
        s_snapshot.critical_brightness_cap_active = s_brightness_cap_suggested;
        s_snapshot.critical_brightness_cap = s_brightness_cap_value;
        xSemaphoreGive(s_snapshot_mutex);
    }

    const esp_err_t pub = state_service_publish_battery(&update, 0);
    if (pub != ESP_OK)
    {
        ESP_LOGW(TAG_BATT, "电量事实发布失败，错误=%s", esp_err_to_name(pub));
    }
}
