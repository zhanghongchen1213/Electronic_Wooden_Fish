/**
 * @file     selftest_service.c
 * @brief    typed 硬件自检服务实现
 * @details  维护有界 handler 注册表和固定 PRD 项目元数据，为按需 selftest_task 提供串行执行基础。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "selftest_service.h"

#include <stdatomic.h>
#include <string.h>

#include "audio_service.h"
#include "config_service.h"
#include "cw2015_bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "gps_service.h"
#include "i2c_manager.h"
#include "modem_selftest_policy.h"
#include "modem_service.h"
#include "qmi8658c_bsp.h"
#include "state_service.h"
#include "ui_service.h"
#include "vibration_bsp.h"

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_GPS_TIME)
#error "LEGBOT_CAP_GPS_TIME must be defined."
#endif
#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "SVC_SELFTEST";

/** 自检完成提示的单次短振动时长。 */
#define SELFTEST_COMPLETION_PULSE_MS 120U
/** 两次失败提示振动之间的静默间隔。 */
#define SELFTEST_COMPLETION_GAP_MS 100U
/** 振动人工确认与 UI 合同一致的超时。 */
#define SELFTEST_VIBRATION_TIMEOUT_MS 30000U
/** 4G 自检等待自动 GPS 进入 standby 的有界 RF 交接时间。 */
#define SELFTEST_MODEM_RF_HANDOFF_TIMEOUT_MS 5000U
_Static_assert(SELFTEST_AUDIO_TERMINAL_MARGIN_MS >=
                   100U + SELFTEST_SERVICE_POLL_MS +
                       SELFTEST_AUDIO_CANCEL_TIMEOUT_MS,
               "音频自检终态提前量必须至少覆盖 100 ms、轮询与取消等待");
_Static_assert(SELFTEST_MODEM_RF_HANDOFF_TIMEOUT_MS <
                   MODEM_SELFTEST_RESULT_DEADLINE_MS,
               "4G 自检 RF 交接必须保留 modem owner 与结果回收预算");

typedef struct
{
    selftest_item_id_t item_id;      /**< 稳定项目 ID。 */
    uint32_t timeout_ms;             /**< 注册的有界超时。 */
    selftest_item_handler_t handler; /**< 项目 handler。 */
    void *handler_context;           /**< 长生命周 handler 上下文。 */
} selftest_registration_t;

/** 当前启用自检项目的固定顺序与超时表。 */
static const selftest_item_metadata_t s_canonical_items[SELFTEST_ITEM_COUNT] = {
    {.item_id = SELFTEST_ITEM_AMOLED, .name = "AMOLED", .timeout_ms = 30000U, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_TOUCH, .name = "touch", .timeout_ms = 30000U, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_I2C_SCAN, .name = "I2C scan", .timeout_ms = 3000U, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_CW2015, .name = "CW2015", .timeout_ms = 3000U, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_QMI8658C, .name = "QMI8658C", .timeout_ms = 3000U, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_AUDIO, .name = "audio", .timeout_ms = SELFTEST_AUDIO_TIMEOUT_MS, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_GPS_UART, .name = "GPS UART", .timeout_ms = GPS_SERVICE_SELFTEST_TIMEOUT_MS, .required_in_story = false},
    {.item_id = SELFTEST_ITEM_GPS_FIX, .name = "GPS fix", .timeout_ms = GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_ML307R, .name = "ML307R", .timeout_ms = 60000U, .required_in_story = false},
    {.item_id = SELFTEST_ITEM_VIBRATION, .name = "vibration", .timeout_ms = SELFTEST_VIBRATION_TIMEOUT_MS, .required_in_story = true},
    {.item_id = SELFTEST_ITEM_SPIFFS, .name = "SPIFFS", .timeout_ms = 3000U, .required_in_story = true},
};

/** 有界 handler 注册表。 */
static selftest_registration_t s_registry[SELFTEST_REGISTRY_CAPACITY];
/** 当前有效注册数量。 */
static size_t s_registry_count;
/** 运行标志，阻止运行期修改注册表。 */
static atomic_bool s_running;
/** 注册表的短持有 C11 互斥标志，冲突时立即返回而不忙等。 */
static atomic_flag s_registry_lock = ATOMIC_FLAG_INIT;
/** selftest_task 私有 typed 命令队列。 */
static QueueHandle_t s_command_queue;
/** ui_task 到 selftest_task 的 typed 人工回复队列。 */
static QueueHandle_t s_ui_reply_queue;
/** 框架已预留或已创建 selftest_task。 */
static atomic_bool s_task_active;
/** 已排队或正在执行 RUN_ALL/RETRY_ITEM，用于立即拒绝重复请求。 */
static atomic_bool s_run_requested;
/** STOP 取消标志，供小步轮询 handler 读取。 */
static atomic_bool s_stop_requested;
/** 启动周期内的最后一个运行 ID。 */
static atomic_uint s_last_run_id;
/** GPS UART/FIX typed probe 的非零单调代次。 */
#if LEGBOT_CAP_GPS_TIME
static atomic_uint s_gps_probe_generation;
/** 当前整机自检 GPS UART/FIX 共享观察窗口所属运行 ID。 */
static uint32_t s_gps_observation_run_id;
/** 当前整机自检 GPS 共享观察窗口起始单调毫秒。 */
static uint64_t s_gps_observation_started_ms;
#endif
/** modem typed probe 的非零单调请求 ID。 */
#if LEGBOT_CAP_MODEM
static atomic_uint s_modem_request_id;
#endif
/** Story 1.5 基础 handler 是否已注册。 */
static bool s_defaults_registered;
/** selftest_task 最近测得的 IDF 字节口径栈高水位。 */
static atomic_uint s_stack_high_water_bytes;

static bool try_lock_registry(void);
static void unlock_registry(void);
static void wait_for_registry_quiescent(void);
static const selftest_registration_t *find_registration(selftest_item_id_t item_id);
static esp_err_t execute_all(selftest_gps_fix_context_t gps_fix_context);
static esp_err_t execute_retry(uint32_t run_id,
                               selftest_item_id_t item_id,
                               selftest_gps_fix_context_t gps_fix_context);
static void execute_item(uint32_t run_id,
                         const selftest_item_metadata_t *metadata,
                         selftest_gps_fix_context_t gps_fix_context,
                         selftest_item_result_t *result);
static void set_result(selftest_item_result_t *result,
                       selftest_outcome_t outcome,
                       selftest_reason_t reason,
                       const char *detail_code);
static const char *firmware_profile_skip_code(selftest_item_id_t item_id);
static bool result_is_valid(const selftest_item_result_t *result);
static const char *outcome_name(selftest_outcome_t outcome);
static esp_err_t register_default_handlers(void);
static esp_err_t handle_i2c_scan(const selftest_handler_context_t *context,
                                 selftest_item_result_t *result);
static esp_err_t handle_cw2015(const selftest_handler_context_t *context,
                                  selftest_item_result_t *result);
static esp_err_t handle_qmi8658c(const selftest_handler_context_t *context,
                                 selftest_item_result_t *result);
static esp_err_t handle_audio(const selftest_handler_context_t *context,
                              selftest_item_result_t *result);
static bool play_selftest_audio_once(
    const selftest_handler_context_t *context,
    int64_t deadline_us,
    selftest_item_result_t *result);
#if LEGBOT_CAP_GPS_TIME
static esp_err_t handle_gps_uart(const selftest_handler_context_t *context,
                                 selftest_item_result_t *result);
static esp_err_t handle_gps_fix(const selftest_handler_context_t *context,
                                selftest_item_result_t *result);
static esp_err_t handle_gps_probe(const selftest_handler_context_t *context,
                                  selftest_item_result_t *result,
                                  gps_selftest_probe_kind_t kind);
#endif
static esp_err_t handle_ml307r(const selftest_handler_context_t *context,
                               selftest_item_result_t *result);
static esp_err_t handle_spiffs(const selftest_handler_context_t *context,
                               selftest_item_result_t *result);
static esp_err_t handle_ui_item(const selftest_handler_context_t *context,
                                selftest_item_result_t *result);
static esp_err_t handle_vibration(const selftest_handler_context_t *context,
                                  selftest_item_result_t *result);
static selftest_reason_t reason_from_i2c_status(i2c_manager_status_t status);
static selftest_reason_t reason_from_cw2015_status(cw2015_status_t status);
static selftest_reason_t reason_from_qmi_status(qmi8658c_status_t status);
static selftest_reason_t reason_from_audio_result(audio_service_result_t audio_result);
static audio_service_result_t audio_result_from_watch_error(watch_audio_error_t error);
static void cancel_audio_with_result(selftest_item_result_t *result,
                                     selftest_reason_t reason,
                                     const char *detail_code);
static void play_completion_haptics(bool has_failure);
static esp_err_t release_vibration_lease(void);

esp_err_t selftest_service_register(selftest_item_id_t item_id,
                                    uint32_t timeout_ms,
                                    selftest_item_handler_t handler,
                                    void *handler_context)
{
    if (item_id < SELFTEST_ITEM_AMOLED || item_id >= SELFTEST_ITEM_COUNT ||
        timeout_ms == 0U || timeout_ms > SELFTEST_MAX_TIMEOUT_MS || handler == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!try_lock_registry())
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t result = ESP_OK;
    if (atomic_load(&s_running))
    {
        result = ESP_ERR_INVALID_STATE;
    }
    else
    {
        for (size_t index = 0; index < s_registry_count; ++index)
        {
            if (s_registry[index].item_id == item_id)
            {
                result = ESP_ERR_INVALID_ARG;
                break;
            }
        }
        if (result == ESP_OK && s_registry_count >= SELFTEST_REGISTRY_CAPACITY)
        {
            result = ESP_ERR_NO_MEM;
        }
        if (result == ESP_OK)
        {
            s_registry[s_registry_count] = (selftest_registration_t){
                .item_id = item_id,
                .timeout_ms = timeout_ms,
                .handler = handler,
                .handler_context = handler_context,
            };
            ++s_registry_count;
        }
    }
    unlock_registry();
    return result;
}

esp_err_t selftest_service_registry_reset(void)
{
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!try_lock_registry())
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (atomic_load(&s_running))
    {
        unlock_registry();
        return ESP_ERR_INVALID_STATE;
    }
    memset(s_registry, 0, sizeof(s_registry));
    s_registry_count = 0U;
    s_defaults_registered = false;
    unlock_registry();
    return ESP_OK;
}

const selftest_item_metadata_t *selftest_service_canonical_items(size_t *count)
{
    if (count != NULL)
    {
        *count = SELFTEST_ITEM_COUNT;
    }
    return s_canonical_items;
}

esp_err_t selftest_service_init_contracts(void)
{
    if (s_command_queue != NULL)
    {
        return ESP_OK;
    }
    s_command_queue = xQueueCreate(SELFTEST_SERVICE_QUEUE_DEPTH,
                                   sizeof(selftest_service_command_t));
    if (s_command_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    s_ui_reply_queue = xQueueCreate(SELFTEST_UI_REPLY_QUEUE_DEPTH,
                                    sizeof(selftest_ui_reply_t));
    if (s_ui_reply_queue == NULL)
    {
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = register_default_handlers();
    if (err != ESP_OK)
    {
        vQueueDelete(s_ui_reply_queue);
        s_ui_reply_queue = NULL;
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
        return err;
    }
    return ESP_OK;
}

esp_err_t selftest_service_prepare_run(void)
{
    if (s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_task_active, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)xQueueReset(s_command_queue);
    (void)xQueueReset(s_ui_reply_queue);
    atomic_store(&s_run_requested, false);
    atomic_store(&s_stop_requested, false);
    return ESP_OK;
}

void selftest_service_cancel_prepared_run(void)
{
    atomic_store(&s_run_requested, false);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_task_active, false);
}

esp_err_t selftest_service_deinit_contracts(void)
{
    if (atomic_load(&s_task_active) || atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_command_queue != NULL)
    {
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
    }
    if (s_ui_reply_queue != NULL)
    {
        vQueueDelete(s_ui_reply_queue);
        s_ui_reply_queue = NULL;
    }
    return ESP_OK;
}

esp_err_t selftest_service_request_run(TickType_t timeout_ticks)
{
    return selftest_service_request_run_with_gps_context(
        SELFTEST_GPS_FIX_CONTEXT_DEFAULT,
        timeout_ticks);
}

esp_err_t selftest_service_request_run_with_gps_context(
    selftest_gps_fix_context_t gps_fix_context,
    TickType_t timeout_ticks)
{
    if (gps_fix_context > SELFTEST_GPS_FIX_CONTEXT_INDOOR_CONFIRMED)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_command_queue == NULL || !atomic_load(&s_task_active) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_run_requested, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const selftest_service_command_t command = {
        .type = SELFTEST_SERVICE_COMMAND_RUN_ALL,
        .requested_at_ticks = xTaskGetTickCount(),
        .item_id = SELFTEST_ITEM_COUNT,
        .gps_fix_context = gps_fix_context,
    };
    if (xQueueSend(s_command_queue, &command, timeout_ticks) != pdTRUE)
    {
        atomic_store(&s_run_requested, false);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t selftest_service_request_retry_item(
    uint32_t run_id,
    selftest_item_id_t item_id,
    selftest_gps_fix_context_t gps_fix_context,
    TickType_t timeout_ticks)
{
    if (run_id == 0U || item_id < SELFTEST_ITEM_AMOLED || item_id >= SELFTEST_ITEM_COUNT ||
        gps_fix_context > SELFTEST_GPS_FIX_CONTEXT_INDOOR_CONFIRMED)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_command_queue == NULL || !atomic_load(&s_task_active) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_run_requested, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const selftest_service_command_t command = {
        .type = SELFTEST_SERVICE_COMMAND_RETRY_ITEM,
        .requested_at_ticks = xTaskGetTickCount(),
        .run_id = run_id,
        .item_id = item_id,
        .gps_fix_context = gps_fix_context,
    };
    if (xQueueSend(s_command_queue, &command, timeout_ticks) != pdTRUE)
    {
        atomic_store(&s_run_requested, false);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t selftest_service_request_stop(TickType_t timeout_ticks)
{
    if (s_command_queue == NULL || !atomic_load(&s_task_active))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, true);
    const selftest_service_command_t command = {
        .type = SELFTEST_SERVICE_COMMAND_STOP,
        .requested_at_ticks = xTaskGetTickCount(),
        .item_id = SELFTEST_ITEM_COUNT,
    };
    return xQueueSend(s_command_queue, &command, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t selftest_service_run(void)
{
    if (s_command_queue == NULL || !atomic_load(&s_task_active))
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t first_error = ESP_OK;
    while (!atomic_load(&s_stop_requested))
    {
        selftest_service_command_t command = {0};
        if (xQueueReceive(s_command_queue,
                          &command,
                          portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        if (command.type == SELFTEST_SERVICE_COMMAND_STOP)
        {
            break;
        }
        if (command.type != SELFTEST_SERVICE_COMMAND_RUN_ALL &&
            command.type != SELFTEST_SERVICE_COMMAND_RETRY_ITEM)
        {
            if (first_error == ESP_OK)
            {
                first_error = ESP_ERR_INVALID_ARG;
            }
            continue;
        }

        atomic_store(&s_running, true);
        /* 等待已通过运行前检查的注册写入退出临界区，之后注册表在本轮保持只读。 */
        wait_for_registry_quiescent();
        esp_err_t run_error = command.type == SELFTEST_SERVICE_COMMAND_RUN_ALL
                                  ? execute_all(command.gps_fix_context)
                                  : execute_retry(command.run_id,
                                                  command.item_id,
                                                  command.gps_fix_context);
        atomic_store(&s_running, false);
        atomic_store(&s_run_requested, false);
        if (run_error != ESP_OK && first_error == ESP_OK)
        {
            first_error = run_error;
        }
    }

    (void)xQueueReset(s_command_queue);
    atomic_store(&s_running, false);
    atomic_store(&s_run_requested, false);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_task_active, false);
    ESP_LOGI(TAG, "selftest_task 已安全退出");
    return first_error;
}

bool selftest_service_cancel_requested(void)
{
    return atomic_load(&s_stop_requested);
}

esp_err_t selftest_service_submit_ui_reply(const selftest_ui_reply_t *reply,
                                           TickType_t timeout_ticks)
{
    if (reply == NULL || reply->run_id == 0U ||
        reply->item_id < SELFTEST_ITEM_AMOLED ||
        reply->item_id >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const bool valid_decision =
        reply->action == SELFTEST_UI_REPLY_ACTION_DECISION &&
        (reply->outcome == SELFTEST_OUTCOME_PASS ||
         reply->outcome == SELFTEST_OUTCOME_FAIL);
    const bool valid_output_replay =
        reply->action == SELFTEST_UI_REPLY_ACTION_OUTPUT_REPLAY &&
        (reply->item_id == SELFTEST_ITEM_AUDIO ||
         reply->item_id == SELFTEST_ITEM_VIBRATION) &&
        reply->outcome == SELFTEST_OUTCOME_NOT_RUN;
    if (!valid_decision && !valid_output_replay)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ui_reply_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueSend(s_ui_reply_queue, reply, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t selftest_service_metrics_snapshot(selftest_service_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    metrics->stack_high_water_bytes = atomic_load(&s_stack_high_water_bytes);
    return ESP_OK;
}

static bool try_lock_registry(void)
{
    return !atomic_flag_test_and_set(&s_registry_lock);
}

static void unlock_registry(void)
{
    atomic_flag_clear(&s_registry_lock);
}

static void wait_for_registry_quiescent(void)
{
    while (!try_lock_registry())
    {
        taskYIELD();
    }
    unlock_registry();
}

static const selftest_registration_t *find_registration(selftest_item_id_t item_id)
{
    for (size_t index = 0; index < s_registry_count; ++index)
    {
        if (s_registry[index].item_id == item_id)
        {
            return &s_registry[index];
        }
    }
    return NULL;
}

static esp_err_t execute_all(selftest_gps_fix_context_t gps_fix_context)
{
    uint32_t run_id = 0U;
    do
    {
        run_id = atomic_fetch_add(&s_last_run_id, 1U) + 1U;
    } while (run_id == 0U);
#if LEGBOT_CAP_GPS_TIME
    s_gps_observation_run_id = 0U;
    s_gps_observation_started_ms = 0U;
#endif
    uint32_t pass_count = 0U;
    uint32_t fail_count = 0U;
    uint32_t skip_count = 0U;
    uint32_t inconclusive_count = 0U;
    esp_err_t first_error = ESP_OK;
    bool cancelled = false;
#if LEGBOT_CAP_GPS_TIME
    bool gps_uart_passed = false;
#endif

    const watch_selftest_begin_update_t begin_update = {
        .run_id = run_id,
        .first_item = SELFTEST_ITEM_AMOLED,
        .started_at_ticks = xTaskGetTickCount(),
    };
    esp_err_t marker_error = config_service_selftest_running_write(
        true,
        pdMS_TO_TICKS(20));
    if (marker_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "整机自检运行标记未持久化，拒绝进入运行，run_id=%lu，错误=0x%x",
                 (unsigned long)run_id,
                 (unsigned)marker_error);
        return marker_error;
    }
    esp_err_t publish_error = state_service_publish_selftest_begin(
        &begin_update,
        pdMS_TO_TICKS(20));
    if (publish_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "自检 begin 状态未必达，run_id=%lu，错误=0x%x",
                 (unsigned long)run_id,
                 (unsigned)publish_error);
        marker_error = config_service_selftest_running_write(
            false,
            pdMS_TO_TICKS(20));
        if (marker_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "自检未进入运行且标记清理失败，run_id=%lu，错误=0x%x",
                     (unsigned long)run_id,
                     (unsigned)marker_error);
        }
        return publish_error;
    }

    ESP_LOGI(TAG, "开始自检运行，run_id=%lu", (unsigned long)run_id);
    for (size_t index = 0; index < SELFTEST_ITEM_COUNT; ++index)
    {
        if (selftest_service_cancel_requested())
        {
            ESP_LOGW(TAG, "自检运行已取消，run_id=%lu，已完成=%u",
                     (unsigned long)run_id,
                     (unsigned)index);
            cancelled = true;
            break;
        }

        selftest_item_result_t result = {
            .item_id = s_canonical_items[index].item_id,
            .outcome = SELFTEST_OUTCOME_NOT_RUN,
            .reason = SELFTEST_REASON_NONE,
        };
#if LEGBOT_CAP_GPS_TIME
        if (result.item_id == SELFTEST_ITEM_GPS_FIX &&
            !gps_uart_passed)
        {
            /*
             * UART 已硬失败时终止本轮 FIX 观察，避免再等待剩余窗口；
             * GPS 类别仍由前一项 UART FAIL 聚合为硬失败。
             */
            set_result(&result,
                       SELFTEST_OUTCOME_SKIP,
                       SELFTEST_REASON_PREREQUISITE,
                       "GPS_FIX_UART_PREREQUISITE_FAILED");
        }
        else
#endif
        {
            execute_item(run_id,
                         &s_canonical_items[index],
                         gps_fix_context,
                         &result);
        }
#if LEGBOT_CAP_GPS_TIME
        if (result.item_id == SELFTEST_ITEM_GPS_UART)
        {
            gps_uart_passed =
                result.outcome == SELFTEST_OUTCOME_PASS;
        }
#endif
        const watch_selftest_item_update_t item_update = {
            .run_id = run_id,
            .result = result,
            .next_item = index + 1U < SELFTEST_ITEM_COUNT
                             ? s_canonical_items[index + 1U].item_id
                             : SELFTEST_ITEM_COUNT,
        };
        publish_error = state_service_publish_selftest_item(
            &item_update,
            pdMS_TO_TICKS(20));
        const bool delivery_failed = publish_error != ESP_OK;
        if (publish_error != ESP_OK && first_error == ESP_OK)
        {
            first_error = publish_error;
        }
        if (result.outcome == SELFTEST_OUTCOME_PASS)
        {
            ++pass_count;
            ESP_LOGI(TAG,
                     "自检项通过，item=%s，outcome=%s，elapsed=%lu ms，detail=%s",
                     s_canonical_items[index].name,
                     outcome_name(result.outcome),
                     (unsigned long)result.elapsed_ms,
                     result.detail_code);
        }
        else if (result.outcome == SELFTEST_OUTCOME_SKIP)
        {
            ++skip_count;
            ESP_LOGW(TAG,
                     "自检项跳过，item=%s，outcome=%s，elapsed=%lu ms，detail=%s",
                     s_canonical_items[index].name,
                     outcome_name(result.outcome),
                     (unsigned long)result.elapsed_ms,
                     result.detail_code);
        }
        else if (result.outcome == SELFTEST_OUTCOME_INCONCLUSIVE)
        {
            ++inconclusive_count;
            ESP_LOGW(TAG,
                     "自检项未完成，item=%s，outcome=%s，elapsed=%lu ms，detail=%s",
                     s_canonical_items[index].name,
                     outcome_name(result.outcome),
                     (unsigned long)result.elapsed_ms,
                     result.detail_code);
        }
        else
        {
            ++fail_count;
            ESP_LOGE(TAG,
                     "自检项失败，item=%s，outcome=%s，elapsed=%lu ms，detail=%s",
                     s_canonical_items[index].name,
                     outcome_name(result.outcome),
                     (unsigned long)result.elapsed_ms,
                     result.detail_code);
        }
        if (delivery_failed)
        {
            ESP_LOGE(TAG,
                     "自检 terminal 未被 state_task 确认，终止本轮，run_id=%lu，item=%s，错误=0x%x",
                     (unsigned long)run_id,
                     s_canonical_items[index].name,
                     (unsigned)publish_error);
            cancelled = true;
            break;
        }
    }

    const watch_selftest_finish_update_t finish_update = {
        .run_id = run_id,
        .state = cancelled || selftest_service_cancel_requested()
                     ? SELFTEST_RUN_CANCELLED
                     : SELFTEST_RUN_FINISHED,
        .finished_at_ticks = xTaskGetTickCount(),
    };
    publish_error = state_service_publish_selftest_finish(
        &finish_update,
        pdMS_TO_TICKS(20));
    if (publish_error != ESP_OK && first_error == ESP_OK)
    {
        first_error = publish_error;
    }
    const bool terminal_published = publish_error == ESP_OK;
    if (terminal_published)
    {
        marker_error = config_service_selftest_running_write(
            false,
            pdMS_TO_TICKS(20));
        if (marker_error != ESP_OK && first_error == ESP_OK)
        {
            first_error = marker_error;
        }
    }
    if (terminal_published && finish_update.state == SELFTEST_RUN_FINISHED)
    {
        play_completion_haptics(fail_count > 0U);
    }

    const UBaseType_t stack_high_water_bytes = uxTaskGetStackHighWaterMark(NULL);
    atomic_store(&s_stack_high_water_bytes, (unsigned)stack_high_water_bytes);

    ESP_LOGI(TAG,
             "自检运行完成，run_id=%lu，pass=%lu，fail=%lu，skip=%lu，未完成=%lu，剩余栈高水位=%lu 字节",
             (unsigned long)run_id,
             (unsigned long)pass_count,
             (unsigned long)fail_count,
             (unsigned long)skip_count,
             (unsigned long)inconclusive_count,
             (unsigned long)stack_high_water_bytes);
    return first_error;
}

static esp_err_t execute_retry(uint32_t run_id,
                               selftest_item_id_t item_id,
                               selftest_gps_fix_context_t gps_fix_context)
{
    if (run_id == 0U || item_id < SELFTEST_ITEM_AMOLED || item_id >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
#if LEGBOT_CAP_GPS_TIME
    s_gps_observation_run_id = 0U;
    s_gps_observation_started_ms = 0U;
#endif

    const watch_selftest_retry_begin_update_t begin_update = {
        .run_id = run_id,
        .item_id = item_id,
        .started_at_ticks = xTaskGetTickCount(),
    };
    esp_err_t publish_error = state_service_publish_selftest_retry_begin(
        &begin_update,
        pdMS_TO_TICKS(20));
    if (publish_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "自检单项重试 begin 未被接受，run_id=%lu，item=%s，错误=0x%x",
                 (unsigned long)run_id,
                 s_canonical_items[item_id].name,
                 (unsigned)publish_error);
        return publish_error;
    }

    selftest_item_result_t result = {
        .item_id = item_id,
        .outcome = SELFTEST_OUTCOME_NOT_RUN,
        .reason = SELFTEST_REASON_NONE,
    };
    execute_item(run_id,
                 &s_canonical_items[item_id],
                 gps_fix_context,
                 &result);

    const watch_selftest_retry_finish_update_t finish_update = {
        .run_id = run_id,
        .result = result,
        .finished_at_ticks = xTaskGetTickCount(),
    };
    publish_error = state_service_publish_selftest_retry_finish(
        &finish_update,
        pdMS_TO_TICKS(20));

    const UBaseType_t stack_high_water_bytes = uxTaskGetStackHighWaterMark(NULL);
    atomic_store(&s_stack_high_water_bytes, (unsigned)stack_high_water_bytes);
    if (publish_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "自检单项重试终态未被确认，run_id=%lu，item=%s，错误=0x%x",
                 (unsigned long)run_id,
                 s_canonical_items[item_id].name,
                 (unsigned)publish_error);
        return publish_error;
    }

    ESP_LOGI(TAG,
             "自检单项重试完成，run_id=%lu，item=%s，outcome=%s，elapsed=%lu ms",
             (unsigned long)run_id,
             s_canonical_items[item_id].name,
             outcome_name(result.outcome),
             (unsigned long)result.elapsed_ms);
    return ESP_OK;
}

static void execute_item(uint32_t run_id,
                         const selftest_item_metadata_t *metadata,
                         selftest_gps_fix_context_t gps_fix_context,
                         selftest_item_result_t *result)
{
    const char *profile_skip_code =
        firmware_profile_skip_code(metadata->item_id);
    if (profile_skip_code != NULL)
    {
        result->item_id = metadata->item_id;
        result->elapsed_ms = 0U;
        set_result(result,
                   SELFTEST_OUTCOME_SKIP,
                   SELFTEST_REASON_PREREQUISITE,
                   profile_skip_code);
        return;
    }

    const selftest_registration_t *registration = find_registration(metadata->item_id);
    if (registration == NULL)
    {
        if (metadata->required_in_story)
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_HANDLER_MISSING,
                       "DRV_SELFTEST_HANDLER_MISSING");
        }
        else
        {
            set_result(result,
                       SELFTEST_OUTCOME_SKIP,
                       SELFTEST_REASON_NOT_REGISTERED,
                       "DRV_SELFTEST_NOT_REGISTERED");
        }
        return;
    }

    const selftest_handler_context_t context = {
        .run_id = run_id,
        .item_id = metadata->item_id,
        .timeout_ms = registration->timeout_ms < metadata->timeout_ms
                          ? registration->timeout_ms
                          : metadata->timeout_ms,
        .gps_fix_context = gps_fix_context,
        .handler_context = registration->handler_context,
    };
    const int64_t started_at_us = esp_timer_get_time();
    esp_err_t handler_error = registration->handler(&context, result);
    result->item_id = metadata->item_id;
    result->elapsed_ms = (uint32_t)((esp_timer_get_time() - started_at_us) / 1000);
    const bool preserve_gps_fix_inconclusive =
        metadata->item_id == SELFTEST_ITEM_GPS_FIX &&
        result_is_valid(result) &&
        result->outcome == SELFTEST_OUTCOME_INCONCLUSIVE &&
        result->reason == SELFTEST_REASON_WINDOW_EXPIRED;

    if (selftest_service_cancel_requested() &&
        !((metadata->item_id == SELFTEST_ITEM_GPS_UART ||
           metadata->item_id == SELFTEST_ITEM_GPS_FIX) &&
          result_is_valid(result) &&
          result->outcome == SELFTEST_OUTCOME_FAIL &&
          result->reason == SELFTEST_REASON_CANCELLED &&
          strcmp(result->detail_code, "GPS_SELFTEST_CANCELLED") == 0))
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_CANCELLED,
                   "DRV_SELFTEST_CANCELLED");
    }
    else if (result->elapsed_ms >= context.timeout_ms &&
             !preserve_gps_fix_inconclusive)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_TIMEOUT,
                   "DRV_SELFTEST_TIMEOUT");
    }
    else if (handler_error != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   "DRV_SELFTEST_HANDLER_FAILED");
    }
    else if (!result_is_valid(result) ||
             (metadata->required_in_story && result->outcome == SELFTEST_OUTCOME_SKIP &&
              result->reason != SELFTEST_REASON_PREREQUISITE))
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_INVALID_RESULT,
                   "DRV_SELFTEST_INVALID_RESULT");
    }
}

static void set_result(selftest_item_result_t *result,
                       selftest_outcome_t outcome,
                       selftest_reason_t reason,
                       const char *detail_code)
{
    result->outcome = outcome;
    result->reason = reason;
    size_t length = strlen(detail_code);
    if (length >= sizeof(result->detail_code))
    {
        detail_code = "DRV_SELFTEST_DETAIL_TOO_LONG";
        length = strlen(detail_code);
    }
    memcpy(result->detail_code, detail_code, length + 1U);
}

static const char *firmware_profile_skip_code(selftest_item_id_t item_id)
{
#if !LEGBOT_CAP_GPS_TIME
    if (item_id == SELFTEST_ITEM_GPS_UART ||
        item_id == SELFTEST_ITEM_GPS_FIX)
    {
        return "GPS_DISABLED_BY_FIRMWARE_PROFILE";
    }
#endif
#if !LEGBOT_CAP_MODEM
    if (item_id == SELFTEST_ITEM_ML307R)
    {
        return "MODEM_DISABLED_BY_FIRMWARE_PROFILE";
    }
#endif
    return NULL;
}

static bool result_is_valid(const selftest_item_result_t *result)
{
    if (result->detail_code[0] == '\0' ||
        memchr(result->detail_code, '\0', sizeof(result->detail_code)) == NULL)
    {
        return false;
    }
    if (result->outcome == SELFTEST_OUTCOME_PASS)
    {
        return result->reason == SELFTEST_REASON_NONE;
    }
    if (result->outcome == SELFTEST_OUTCOME_FAIL ||
        result->outcome == SELFTEST_OUTCOME_SKIP ||
        result->outcome == SELFTEST_OUTCOME_INCONCLUSIVE)
    {
        return result->reason != SELFTEST_REASON_NONE;
    }
    return false;
}

static const char *outcome_name(selftest_outcome_t outcome)
{
    switch (outcome)
    {
    case SELFTEST_OUTCOME_PASS:
        return "pass";
    case SELFTEST_OUTCOME_FAIL:
        return "fail";
    case SELFTEST_OUTCOME_SKIP:
        return "skip";
    case SELFTEST_OUTCOME_INCONCLUSIVE:
        return "inconclusive";
    default:
        return "not_run";
    }
}

static esp_err_t register_default_handlers(void)
{
    const struct
    {
        selftest_item_id_t item_id;
        uint32_t timeout_ms;
        selftest_item_handler_t handler;
    } handlers[] = {
        {SELFTEST_ITEM_AMOLED, 30000U, handle_ui_item},
        {SELFTEST_ITEM_TOUCH, 30000U, handle_ui_item},
        {SELFTEST_ITEM_I2C_SCAN, 3000U, handle_i2c_scan},
        {SELFTEST_ITEM_CW2015, 3000U, handle_cw2015},
        {SELFTEST_ITEM_QMI8658C, 3000U, handle_qmi8658c},
        {SELFTEST_ITEM_AUDIO, SELFTEST_AUDIO_TIMEOUT_MS, handle_audio},
#if LEGBOT_CAP_GPS_TIME
        {SELFTEST_ITEM_GPS_UART, GPS_SERVICE_SELFTEST_TIMEOUT_MS, handle_gps_uart},
        {SELFTEST_ITEM_GPS_FIX, GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS, handle_gps_fix},
#endif
        {SELFTEST_ITEM_ML307R, MODEM_SELFTEST_TIMEOUT_MS, handle_ml307r},
        {SELFTEST_ITEM_VIBRATION, SELFTEST_VIBRATION_TIMEOUT_MS, handle_vibration},
        {SELFTEST_ITEM_SPIFFS, 3000U, handle_spiffs},
    };
    const size_t handler_count = sizeof(handlers) / sizeof(handlers[0]);

    if (atomic_load(&s_running) || !try_lock_registry())
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t result = ESP_OK;
    if (s_defaults_registered)
    {
        unlock_registry();
        return ESP_OK;
    }
    if (atomic_load(&s_running))
    {
        result = ESP_ERR_INVALID_STATE;
    }
    else if (s_registry_count + handler_count > SELFTEST_REGISTRY_CAPACITY)
    {
        result = ESP_ERR_NO_MEM;
    }
    for (size_t handler_index = 0;
         result == ESP_OK && handler_index < handler_count;
         ++handler_index)
    {
        for (size_t registry_index = 0;
             registry_index < s_registry_count;
             ++registry_index)
        {
            if (s_registry[registry_index].item_id == handlers[handler_index].item_id)
            {
                result = ESP_ERR_INVALID_ARG;
                break;
            }
        }
    }
    if (result == ESP_OK)
    {
        for (size_t index = 0; index < handler_count; ++index)
        {
            s_registry[s_registry_count++] = (selftest_registration_t){
                .item_id = handlers[index].item_id,
                .timeout_ms = handlers[index].timeout_ms,
                .handler = handlers[index].handler,
                .handler_context = NULL,
            };
        }
        s_defaults_registered = true;
    }
    unlock_registry();
    return result;
}

static esp_err_t handle_i2c_scan(const selftest_handler_context_t *context,
                                 selftest_item_result_t *result)
{
    (void)context;
    i2c_manager_scan_report_t report = {0};
    esp_err_t err = i2c_manager_scan_expected(&report);
    const size_t classified_count = report.count < I2C_MANAGER_EXPECTED_DEVICE_COUNT
                                        ? report.count
                                        : I2C_MANAGER_EXPECTED_DEVICE_COUNT;
    for (size_t index = 0; index < classified_count; ++index)
    {
        if (report.devices[index].status != I2C_MANAGER_STATUS_PRESENT)
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       reason_from_i2c_status(report.devices[index].status),
                       i2c_manager_error_code(report.devices[index].status));
            return ESP_OK;
        }
    }
    if (err != ESP_OK || report.count != I2C_MANAGER_EXPECTED_DEVICE_COUNT)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   "DRV_I2C_SCAN_INCOMPLETE");
        return ESP_OK;
    }
    set_result(result, SELFTEST_OUTCOME_PASS, SELFTEST_REASON_NONE, "DRV_I2C_SCAN_OK");
    return ESP_OK;
}

static esp_err_t handle_cw2015(const selftest_handler_context_t *context,
                                  selftest_item_result_t *result)
{
    (void)context;
    cw2015_bsp_continuous_report_t report = {0};
    esp_err_t err = cw2015_bsp_read_continuous(&report);
    for (size_t index = 0; index < report.count; ++index)
    {
        const cw2015_bsp_soc_t *sample = &report.samples[index];
        if (!sample->valid || sample->percent < 0.0f || sample->percent > 100.0f)
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       reason_from_cw2015_status(sample->status),
                       sample->error_code != NULL ? sample->error_code : CW2015_ERROR_READ_FAILED);
            return ESP_OK;
        }
    }
    if (err != ESP_OK || report.count != CW2015_BSP_DIAGNOSTIC_SAMPLE_COUNT)
    {
        const cw2015_bsp_soc_t *last_sample = report.count > 0U
                                                     ? &report.samples[report.count - 1U]
                                                     : NULL;
        cw2015_status_t status = last_sample != NULL
                                        ? last_sample->status
                                        : CW2015_STATUS_NOT_READY;
        const char *detail_code = last_sample != NULL && last_sample->error_code != NULL
                                      ? last_sample->error_code
                                      : CW2015_ERROR_NOT_READY;
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   reason_from_cw2015_status(status),
                   detail_code);
        return ESP_OK;
    }
    set_result(result, SELFTEST_OUTCOME_PASS, SELFTEST_REASON_NONE, CW2015_ERROR_OK);
    return ESP_OK;
}

static esp_err_t handle_qmi8658c(const selftest_handler_context_t *context,
                                 selftest_item_result_t *result)
{
    (void)context;
    qmi8658c_bsp_continuous_report_t report = {0};
    esp_err_t err = qmi8658c_bsp_read_continuous(&report);
    for (size_t index = 0; index < report.count; ++index)
    {
        const qmi8658c_bsp_sample_t *sample = &report.samples[index];
        if (!sample->valid)
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       reason_from_qmi_status(sample->status),
                       sample->error_code != NULL ? sample->error_code : QMI8658C_ERROR_READ_FAILED);
            return ESP_OK;
        }
    }
    if (err != ESP_OK || report.count != QMI8658C_BSP_DIAGNOSTIC_SAMPLE_COUNT)
    {
        qmi8658c_status_t status = qmi8658c_bsp_last_status();
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   reason_from_qmi_status(status),
                   qmi8658c_bsp_error_code(status));
        return ESP_OK;
    }
    set_result(result, SELFTEST_OUTCOME_PASS, SELFTEST_REASON_NONE, QMI8658C_ERROR_OK);
    return ESP_OK;
}

static esp_err_t handle_audio(const selftest_handler_context_t *context,
                              selftest_item_result_t *result)
{
    if (context->timeout_ms <= SELFTEST_AUDIO_TERMINAL_MARGIN_MS)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   "AUDIO_TIMEOUT_BUDGET_INVALID");
        return ESP_OK;
    }

    esp_err_t request_error = ui_service_request_selftest_item(
        context->run_id,
        context->item_id,
        pdMS_TO_TICKS(20));
    if (request_error != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "UI_SELFTEST_REQUEST_FAILED");
        return ESP_OK;
    }

    const int64_t deadline_us =
        esp_timer_get_time() +
        (int64_t)(context->timeout_ms - SELFTEST_AUDIO_TERMINAL_MARGIN_MS) *
            1000;
    if (!play_selftest_audio_once(context, deadline_us, result))
    {
        return ESP_OK;
    }

    while (esp_timer_get_time() < deadline_us)
    {
        if (selftest_service_cancel_requested())
        {
            cancel_audio_with_result(result,
                                     SELFTEST_REASON_CANCELLED,
                                     "DRV_SELFTEST_CANCELLED");
            return ESP_OK;
        }
        selftest_ui_reply_t reply = {0};
        if (xQueueReceive(s_ui_reply_queue,
                          &reply,
                          pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS)) != pdTRUE)
        {
            continue;
        }
        if (reply.run_id != context->run_id ||
            reply.item_id != context->item_id)
        {
            ESP_LOGW(TAG,
                     "丢弃过期音频自检回复，当前 run=%lu，回复 run=%lu/item=%d",
                     (unsigned long)context->run_id,
                     (unsigned long)reply.run_id,
                     (int)reply.item_id);
            continue;
        }
        if (reply.action == SELFTEST_UI_REPLY_ACTION_OUTPUT_REPLAY)
        {
            ESP_LOGI(TAG,
                     "音频自检按用户请求再次播放，run_id=%lu",
                     (unsigned long)context->run_id);
            if (!play_selftest_audio_once(context, deadline_us, result))
            {
                return ESP_OK;
            }
            continue;
        }
        if (reply.outcome == SELFTEST_OUTCOME_PASS)
        {
            set_result(result,
                       SELFTEST_OUTCOME_PASS,
                       SELFTEST_REASON_NONE,
                       "AUDIO_AUDIBLE_CONFIRMED");
        }
        else
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_USER_REJECTED,
                       "AUDIO_NOT_HEARD");
        }
        return ESP_OK;
    }
    set_result(result,
               SELFTEST_OUTCOME_FAIL,
               SELFTEST_REASON_TIMEOUT,
               "AUDIO_CONFIRM_TIMEOUT");
    return ESP_OK;
}

static bool play_selftest_audio_once(
    const selftest_handler_context_t *context,
    int64_t deadline_us,
    selftest_item_result_t *result)
{
    watch_audio_snapshot_t initial_snapshot = {0};
    esp_err_t snapshot_error = watch_state_audio_snapshot(&initial_snapshot,
                                                          pdMS_TO_TICKS(20));
    if (snapshot_error != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "DRV_SELFTEST_STATE_BUSY");
        return false;
    }

    audio_service_result_t play_result = audio_service_request_play(
        AUDIO_RESOURCE_SELFTEST_OK,
        AUDIO_PRIORITY_SELFTEST,
        0);
    if (play_result != AUDIO_RESULT_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   reason_from_audio_result(play_result),
                   audio_service_result_code(play_result));
        return false;
    }

    bool saw_playing = false;
    uint32_t observed_sequence = initial_snapshot.update_sequence;
    while (esp_timer_get_time() < deadline_us)
    {
        if (selftest_service_cancel_requested())
        {
            cancel_audio_with_result(result,
                                     SELFTEST_REASON_CANCELLED,
                                     "DRV_SELFTEST_CANCELLED");
            return false;
        }
        watch_audio_snapshot_t snapshot = {0};
        if (watch_state_audio_snapshot(&snapshot, pdMS_TO_TICKS(20)) == ESP_OK &&
            snapshot.update_sequence != observed_sequence)
        {
            observed_sequence = snapshot.update_sequence;
            if (snapshot.state == WATCH_AUDIO_STATE_PLAYING)
            {
                saw_playing = true;
            }
            else if (snapshot.state == WATCH_AUDIO_STATE_COMPLETED && saw_playing)
            {
                ESP_LOGI(TAG,
                         "音频 PCM 已完整物理输出，等待用户确认实际听感，run_id=%lu",
                         (unsigned long)context->run_id);
                return true;
            }
            else if (snapshot.state == WATCH_AUDIO_STATE_COMPLETED)
            {
                ESP_LOGW(TAG, "忽略未观察到本次 PLAYING 的音频完成状态");
            }
            else if (snapshot.state == WATCH_AUDIO_STATE_FAILED)
            {
                audio_service_result_t terminal = audio_result_from_watch_error(snapshot.error);
                set_result(result,
                           SELFTEST_OUTCOME_FAIL,
                           reason_from_audio_result(terminal),
                           audio_service_result_code(terminal));
                return false;
            }
            else if (snapshot.state == WATCH_AUDIO_STATE_STOPPED)
            {
                set_result(result,
                           SELFTEST_OUTCOME_FAIL,
                           SELFTEST_REASON_CANCELLED,
                           "AUDIO_PLAYBACK_STOPPED");
                return false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
    }
    cancel_audio_with_result(result,
                             SELFTEST_REASON_TIMEOUT,
                             "DRV_SELFTEST_TIMEOUT");
    return false;
}

#if LEGBOT_CAP_GPS_TIME
static esp_err_t handle_gps_uart(const selftest_handler_context_t *context,
                                 selftest_item_result_t *result)
{
    return handle_gps_probe(context, result, GPS_SELFTEST_PROBE_UART);
}

static esp_err_t handle_gps_fix(const selftest_handler_context_t *context,
                                selftest_item_result_t *result)
{
    return handle_gps_probe(context, result, GPS_SELFTEST_PROBE_FIX);
}

static esp_err_t handle_gps_probe(const selftest_handler_context_t *context,
                                  selftest_item_result_t *result,
                                  gps_selftest_probe_kind_t kind)
{
    const uint32_t expected_timeout_ms =
        kind == GPS_SELFTEST_PROBE_FIX
            ? GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS
            : GPS_SERVICE_SELFTEST_TIMEOUT_MS;
    if (context == NULL || result == NULL ||
        kind > GPS_SELFTEST_PROBE_FIX ||
        context->timeout_ms != expected_timeout_ms)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint32_t probe_generation =
        atomic_fetch_add(&s_gps_probe_generation, 1U) + 1U;
    if (probe_generation == 0U)
    {
        probe_generation = atomic_fetch_add(&s_gps_probe_generation, 1U) + 1U;
    }
    const uint64_t request_started_ms =
        (uint64_t)esp_timer_get_time() / 1000U;
    if (kind == GPS_SELFTEST_PROBE_UART)
    {
        s_gps_observation_run_id = context->run_id;
        s_gps_observation_started_ms = request_started_ms;
    }
    const uint64_t observation_started_ms =
        kind == GPS_SELFTEST_PROBE_FIX &&
                s_gps_observation_run_id == context->run_id &&
                s_gps_observation_started_ms != 0U
            ? s_gps_observation_started_ms
            : request_started_ms;
    const gps_selftest_request_t request = {
        .run_id = context->run_id,
        .request_id = context->run_id,
        .probe_generation = probe_generation,
        .started_at_ms = request_started_ms,
        .kind = kind,
        .context = kind == GPS_SELFTEST_PROBE_FIX &&
                           context->gps_fix_context ==
                               SELFTEST_GPS_FIX_CONTEXT_INDOOR_CONFIRMED
                       ? GPS_SELFTEST_CONTEXT_INDOOR_CONFIRMED
                       : GPS_SELFTEST_CONTEXT_DEFAULT,
    };
    esp_err_t error = gps_service_request_selftest(
        &request,
        pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
    if (error == ESP_ERR_INVALID_STATE)
    {
        set_result(result,
                   SELFTEST_OUTCOME_SKIP,
                   SELFTEST_REASON_PREREQUISITE,
                   kind == GPS_SELFTEST_PROBE_FIX
                       ? "GPS_FIX_UNAVAILABLE"
                       : "GPS_SELFTEST_SERVICE_UNAVAILABLE");
        return ESP_OK;
    }
    if (error != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "GPS_SELFTEST_QUEUE_FULL");
        return ESP_OK;
    }

    while ((uint64_t)esp_timer_get_time() / 1000U - observation_started_ms <
           context->timeout_ms)
    {
        if (selftest_service_cancel_requested())
        {
            const esp_err_t cancel_error = gps_service_cancel_selftest(
                &request,
                pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
            if (cancel_error != ESP_OK)
            {
                ESP_LOGW(TAG,
                         "GPS 自检取消请求未被 owner 保留，请求=%lu，类型=%u，稳定码=GPS_SELFTEST_CANCEL_REQUEST_FAILED",
                         (unsigned long)request.request_id,
                         (unsigned)request.kind);
            }
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_CANCELLED,
                       cancel_error == ESP_OK
                           ? "GPS_SELFTEST_CANCELLED"
                           : "GPS_SELFTEST_CANCEL_REQUEST_FAILED");
            return ESP_OK;
        }
        gps_selftest_result_t gps_result = {0};
        error = gps_service_receive_selftest_result(
            &gps_result,
            pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
        if (error == ESP_ERR_TIMEOUT)
        {
            continue;
        }
        if (error != ESP_OK)
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_READ_FAILED,
                       "DRV_GPS_SELFTEST_RESULT_FAILED");
            return ESP_OK;
        }
        if (gps_result.run_id != request.run_id ||
            gps_result.request_id != request.request_id ||
            gps_result.probe_generation != request.probe_generation ||
            gps_result.kind != request.kind)
        {
            continue;
        }
        if (gps_result.outcome == GPS_SELFTEST_OUTCOME_PASS)
        {
            if (kind == GPS_SELFTEST_PROBE_FIX)
            {
                double lat = 0.0, lon = 0.0;
                if (gps_service_get_coordinates(&lat, &lon) == ESP_OK)
                {
                    ESP_LOGI(TAG,
                             "GPS 定位成功：纬度=%.6f, 经度=%.6f",
                             lat, lon);
                }
            }
            set_result(result,
                       SELFTEST_OUTCOME_PASS,
                       SELFTEST_REASON_NONE,
                       gps_result.detail_code);
        }
        else if (gps_result.outcome == GPS_SELFTEST_OUTCOME_SKIP)
        {
            set_result(result,
                       SELFTEST_OUTCOME_SKIP,
                       SELFTEST_REASON_PREREQUISITE,
                       gps_result.detail_code);
        }
        else if (gps_result.outcome == GPS_SELFTEST_OUTCOME_INCONCLUSIVE)
        {
            set_result(result,
                       SELFTEST_OUTCOME_INCONCLUSIVE,
                       SELFTEST_REASON_WINDOW_EXPIRED,
                       gps_result.detail_code);
        }
        else
        {
            const bool cancelled = strcmp(gps_result.detail_code,
                                          "GPS_SELFTEST_CANCELLED") == 0;
            const bool timed_out = strstr(gps_result.detail_code,
                                          "TIMEOUT") != NULL ||
                                   strcmp(gps_result.detail_code,
                                          "GPS_FIX_SEARCHING") == 0;
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       cancelled
                           ? SELFTEST_REASON_CANCELLED
                           : (timed_out ? SELFTEST_REASON_TIMEOUT
                                        : SELFTEST_REASON_DRIVER_FAILED),
                       gps_result.detail_code);
        }
        return ESP_OK;
    }

    const esp_err_t cancel_error = gps_service_cancel_selftest(
        &request,
        pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
    if (cancel_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "GPS 自检超时后取消未被 owner 保留，请求=%lu，类型=%u，稳定码=GPS_SELFTEST_CANCEL_REQUEST_FAILED",
                 (unsigned long)request.request_id,
                 (unsigned)request.kind);
    }
    if (kind == GPS_SELFTEST_PROBE_FIX && cancel_error == ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_INCONCLUSIVE,
                   SELFTEST_REASON_WINDOW_EXPIRED,
                   "GPS_FIX_WINDOW_EXPIRED");
    }
    else
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_TIMEOUT,
                   cancel_error == ESP_OK
                       ? "GPS_SELFTEST_HANDLER_TIMEOUT"
                       : "GPS_SELFTEST_CANCEL_REQUEST_FAILED");
    }
    return ESP_OK;
}
#endif

static esp_err_t handle_ml307r(const selftest_handler_context_t *context,
                               selftest_item_result_t *result)
{
#if !LEGBOT_CAP_MODEM
    if (context == NULL || result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    set_result(result,
               SELFTEST_OUTCOME_SKIP,
               SELFTEST_REASON_PREREQUISITE,
               "MODEM_DISABLED_BY_FIRMWARE_PROFILE");
    return ESP_OK;
#else
    if (context == NULL || result == NULL ||
        context->timeout_ms != MODEM_SELFTEST_TIMEOUT_MS)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (selftest_service_cancel_requested())
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_CANCELLED,
                   "MODEM_SELFTEST_CANCELLED");
        return ESP_OK;
    }
    uint32_t request_id = atomic_fetch_add(&s_modem_request_id, 1U) + 1U;
    if (request_id == 0U)
    {
        request_id = atomic_fetch_add(&s_modem_request_id, 1U) + 1U;
    }
    const TickType_t handler_started_at = xTaskGetTickCount();
    esp_err_t err = ESP_ERR_NOT_FINISHED;
    while (err == ESP_ERR_NOT_FINISHED &&
           xTaskGetTickCount() - handler_started_at <
               pdMS_TO_TICKS(SELFTEST_MODEM_RF_HANDOFF_TIMEOUT_MS))
    {
        err = modem_service_selftest_begin(request_id);
        if (err != ESP_ERR_NOT_FINISHED)
        {
            break;
        }
        const esp_err_t pause_error = gps_service_pause_for_radio(0);
        if (pause_error != ESP_OK && pause_error != ESP_ERR_TIMEOUT)
        {
            ESP_LOGW(TAG,
                     "4G 自检请求 GPS 让出 RF 暂未成功，错误=0x%x",
                     (unsigned)pause_error);
        }
        if (selftest_service_cancel_requested())
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_CANCELLED,
                       "MODEM_SELFTEST_CANCELLED");
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
    }
    if (err == ESP_ERR_INVALID_STATE)
    {
        set_result(result,
                   SELFTEST_OUTCOME_SKIP,
                   SELFTEST_REASON_PREREQUISITE,
                   "MODEM_NOT_STARTED");
        return ESP_OK;
    }
    if (err == ESP_ERR_NOT_FINISHED)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "MODEM_RF_BUSY");
        return ESP_OK;
    }
    if (err != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "MODEM_SELFTEST_BUSY");
        return ESP_OK;
    }
    modem_selftest_result_t modem_result = {0};
    while (xTaskGetTickCount() - handler_started_at <
           pdMS_TO_TICKS(MODEM_SELFTEST_RESULT_DEADLINE_MS))
    {
        if (selftest_service_cancel_requested())
        {
            const esp_err_t cancel_error =
                modem_service_selftest_cancel(request_id);
            if (cancel_error != ESP_OK &&
                cancel_error != ESP_ERR_NOT_FOUND)
            {
                ESP_LOGW(TAG,
                         "4G 自检取消未被 owner 接受，请求=%lu，错误=0x%x",
                         (unsigned long)request_id,
                         (unsigned)cancel_error);
            }
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_CANCELLED,
                       "MODEM_SELFTEST_CANCELLED");
            return ESP_OK;
        }
        err = modem_service_selftest_receive(
            &modem_result,
            pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
        if (err == ESP_ERR_TIMEOUT)
        {
            continue;
        }
        if (err != ESP_OK)
        {
            (void)modem_service_selftest_cancel(request_id);
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_READ_FAILED,
                       "MODEM_SELFTEST_RESULT");
            return ESP_OK;
        }
        if (modem_result.request_id != request_id)
        {
            ESP_LOGW(TAG,
                     "丢弃非当前 4G 自检终态，期望请求=%lu，收到请求=%lu",
                     (unsigned long)request_id,
                     (unsigned long)modem_result.request_id);
            continue;
        }
        if (modem_result.outcome == MODEM_SELFTEST_PASS)
        {
            set_result(result,
                       SELFTEST_OUTCOME_PASS,
                       SELFTEST_REASON_NONE,
                       "MODEM_SELFTEST_OK");
        }
        else if (modem_result.outcome == MODEM_SELFTEST_SKIP)
        {
            set_result(result,
                       SELFTEST_OUTCOME_SKIP,
                       SELFTEST_REASON_PREREQUISITE,
                       modem_result.reason);
        }
        else
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_READ_FAILED,
                       modem_result.reason);
        }
        return ESP_OK;
    }
    (void)modem_service_selftest_cancel(request_id);
    set_result(result,
               SELFTEST_OUTCOME_FAIL,
               SELFTEST_REASON_TIMEOUT,
               "MODEM_SELFTEST_TIMEOUT");
    return ESP_OK;
#endif
}

static esp_err_t handle_spiffs(const selftest_handler_context_t *context,
                               selftest_item_result_t *result)
{
    (void)context;
    audio_wav_info_t wav_info = {0};
    if (!audio_service_storage_ready())
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_RESOURCE_MISSING,
                   audio_service_result_code(AUDIO_RESULT_NOT_READY));
        return ESP_OK;
    }
    audio_service_result_t probe_result = audio_service_probe_resource(
        AUDIO_RESOURCE_SELFTEST_OK,
        &wav_info);
    if (probe_result != AUDIO_RESULT_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   reason_from_audio_result(probe_result),
                   audio_service_result_code(probe_result));
        return ESP_OK;
    }
    set_result(result, SELFTEST_OUTCOME_PASS, SELFTEST_REASON_NONE, "AUDIO_RESOURCE_OK");
    return ESP_OK;
}

static esp_err_t handle_ui_item(const selftest_handler_context_t *context,
                                selftest_item_result_t *result)
{
    esp_err_t request_error = ui_service_request_selftest_item(
        context->run_id,
        context->item_id,
        pdMS_TO_TICKS(20));
    if (request_error != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "UI_SELFTEST_REQUEST_FAILED");
        return ESP_OK;
    }

    const int64_t started_at_us = esp_timer_get_time();
    while ((uint32_t)((esp_timer_get_time() - started_at_us) / 1000) < context->timeout_ms)
    {
        if (selftest_service_cancel_requested())
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_CANCELLED,
                       "DRV_SELFTEST_CANCELLED");
            return ESP_OK;
        }
        selftest_ui_reply_t reply = {0};
        if (xQueueReceive(s_ui_reply_queue,
                          &reply,
                          pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS)) != pdTRUE)
        {
            continue;
        }
        if (reply.run_id != context->run_id || reply.item_id != context->item_id)
        {
            ESP_LOGW(TAG,
                     "丢弃过期 UI 自检回复，当前 run=%lu/item=%d，回复 run=%lu/item=%d",
                     (unsigned long)context->run_id,
                     (int)context->item_id,
                     (unsigned long)reply.run_id,
                     (int)reply.item_id);
            continue;
        }
        if (reply.outcome == SELFTEST_OUTCOME_PASS)
        {
            set_result(result,
                       SELFTEST_OUTCOME_PASS,
                       SELFTEST_REASON_NONE,
                       "UI_SELFTEST_CONFIRMED");
        }
        else
        {
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_USER_REJECTED,
                       "UI_SELFTEST_REJECTED");
        }
        return ESP_OK;
    }
    set_result(result,
               SELFTEST_OUTCOME_FAIL,
               SELFTEST_REASON_TIMEOUT,
               "UI_SELFTEST_TIMEOUT");
    return ESP_OK;
}

static void play_completion_haptics(bool has_failure)
{
    config_service_ui_preferences_t preferences = {0};
    watch_power_snapshot_t power = {0};
    if (config_service_ui_preferences_read(&preferences,
                                           pdMS_TO_TICKS(20)) != ESP_OK ||
        !preferences.haptics_enabled ||
        watch_state_power_snapshot(&power, 0) != ESP_OK ||
        !audio_service_ui_feedback_allowed(power.power_level))
    {
        return;
    }
    esp_err_t err = vibration_bsp_lease_acquire(
        pdMS_TO_TICKS(VIBRATION_BSP_PULSE_MAX_MS));
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "自检完成振动提示无法获取硬件租约，稳定错误码=VIBRATION_BUSY，错误=0x%x",
                 (unsigned)err);
        return;
    }
    err = vibration_bsp_init();
    const uint8_t pulse_count = has_failure ? 2U : 1U;
    for (uint8_t pulse = 0U; err == ESP_OK && pulse < pulse_count; ++pulse)
    {
        err = vibration_bsp_start_pulse(SELFTEST_COMPLETION_PULSE_MS);
        if (err != ESP_OK)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(SELFTEST_COMPLETION_PULSE_MS));
        err = vibration_bsp_stop();
        if (err == ESP_OK && pulse + 1U < pulse_count)
        {
            vTaskDelay(pdMS_TO_TICKS(SELFTEST_COMPLETION_GAP_MS));
        }
    }
    const esp_err_t release_error = release_vibration_lease();
    if (err != ESP_OK || release_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "自检完成振动提示未完整执行，错误=0x%x，释放错误=0x%x",
                 (unsigned)err,
                 (unsigned)release_error);
    }
}

static esp_err_t release_vibration_lease(void)
{
    const esp_err_t stop_error = vibration_bsp_stop();
    const esp_err_t release_error = vibration_bsp_lease_release();
    return stop_error != ESP_OK ? stop_error : release_error;
}

static esp_err_t handle_vibration(const selftest_handler_context_t *context,
                                  selftest_item_result_t *result)
{
    if (!vibration_bsp_profile_verified())
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   "DRV_VIB_PROFILE_UNVERIFIED");
        return ESP_OK;
    }

    /* 先异步请求人工确认状态，让用户在振动结束前获得正式页面提示。 */
    esp_err_t err = ui_service_request_selftest_item(
        context->run_id,
        context->item_id,
        pdMS_TO_TICKS(20));
    if (err != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "UI_SELFTEST_REQUEST_FAILED");
        return ESP_OK;
    }

    /* 获取 IO11/LEDC 独占租约后初始化振动马达 BSP。 */
    err = vibration_bsp_lease_acquire(
        pdMS_TO_TICKS(VIBRATION_BSP_PULSE_MAX_MS));
    if (err != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_BUSY,
                   "DRV_VIB_BUSY");
        return ESP_OK;
    }
    bool lease_held = true;
    err = vibration_bsp_init();
    if (err != ESP_OK)
    {
        (void)release_vibration_lease();
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   "DRV_VIB_INIT_FAILED");
        return ESP_OK;
    }

    /* 启动振动脉冲 */
    err = vibration_bsp_start_pulse(VIBRATION_BSP_SELFTEST_PULSE_MS);
    if (err != ESP_OK)
    {
        const esp_err_t release_error = release_vibration_lease();
        lease_held = false;
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   release_error == ESP_OK
                       ? "DRV_VIB_START_FAILED"
                       : "DRV_VIB_SAFE_STOP_FAILED");
        return ESP_OK;
    }

    /* 合并轮询：等待用户回复 / 脉宽结束 / 取消 / 整体超时 */
    const int64_t item_started_at_us = esp_timer_get_time();
    int64_t pulse_started_at_us = esp_timer_get_time();
    bool pulse_stopped = false;

    while ((uint32_t)((esp_timer_get_time() - item_started_at_us) / 1000) <
           context->timeout_ms)
    {
        /* 检查取消信号 */
        if (selftest_service_cancel_requested())
        {
            if (lease_held)
            {
                (void)release_vibration_lease();
                lease_held = false;
            }
            set_result(result,
                       SELFTEST_OUTCOME_FAIL,
                       SELFTEST_REASON_CANCELLED,
                       "DRV_SELFTEST_CANCELLED");
            return ESP_OK;
        }

        /* 脉宽到达后停止输出，但继续保持 UI 等待人工确认 */
        if (!pulse_stopped &&
            (uint32_t)((esp_timer_get_time() - pulse_started_at_us) / 1000) >=
                VIBRATION_BSP_SELFTEST_PULSE_MS)
        {
            const esp_err_t release_error = release_vibration_lease();
            lease_held = false;
            pulse_stopped = true;
            if (release_error != ESP_OK)
            {
                set_result(result,
                           SELFTEST_OUTCOME_FAIL,
                           SELFTEST_REASON_DRIVER_FAILED,
                           "DRV_VIB_SAFE_STOP_FAILED");
                return ESP_OK;
            }
            ESP_LOGI(TAG, "振动自检脉冲已结束并释放硬件，继续等待人工确认");
        }

        /* 检查用户动作（最终判定或振动重放） */
        selftest_ui_reply_t reply = {0};
        if (xQueueReceive(s_ui_reply_queue,
                          &reply,
                          pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS)) == pdTRUE)
        {
            if (reply.run_id == context->run_id &&
                reply.item_id == context->item_id)
            {
                if (reply.action == SELFTEST_UI_REPLY_ACTION_OUTPUT_REPLAY)
                {
                    esp_err_t replay_error = ESP_OK;
                    if (lease_held)
                    {
                        replay_error = vibration_bsp_stop();
                    }
                    else
                    {
                        replay_error = vibration_bsp_lease_acquire(
                            pdMS_TO_TICKS(VIBRATION_BSP_PULSE_MAX_MS));
                        if (replay_error == ESP_OK)
                        {
                            lease_held = true;
                            replay_error = vibration_bsp_init();
                        }
                    }
                    if (replay_error == ESP_OK && lease_held)
                    {
                        replay_error = vibration_bsp_start_pulse(
                            VIBRATION_BSP_SELFTEST_PULSE_MS);
                    }
                    if (replay_error == ESP_OK)
                    {
                        pulse_started_at_us = esp_timer_get_time();
                        pulse_stopped = false;
                        ESP_LOGI(TAG,
                                 "振动自检已重放有界脉冲，run_id=%lu",
                                 (unsigned long)context->run_id);
                    }
                    else
                    {
                        if (lease_held)
                        {
                            (void)release_vibration_lease();
                            lease_held = false;
                        }
                        pulse_stopped = true;
                        ESP_LOGW(TAG,
                                 "振动自检重放失败，继续等待最终判定，run_id=%lu，错误=0x%x",
                                 (unsigned long)context->run_id,
                                 (unsigned)replay_error);
                    }
                    continue;
                }
                if (lease_held)
                {
                    (void)release_vibration_lease();
                    lease_held = false;
                }
                if (reply.outcome == SELFTEST_OUTCOME_PASS)
                {
                    set_result(result,
                               SELFTEST_OUTCOME_PASS,
                               SELFTEST_REASON_NONE,
                               "UI_SELFTEST_CONFIRMED");
                }
                else
                {
                    set_result(result,
                               SELFTEST_OUTCOME_FAIL,
                               SELFTEST_REASON_USER_REJECTED,
                               "UI_SELFTEST_REJECTED");
                }
                ESP_LOGI(TAG,
                         "振动自检收到人工回复，run_id=%lu，outcome=%s",
                         (unsigned long)context->run_id,
                         reply.outcome == SELFTEST_OUTCOME_PASS ? "PASS" : "FAIL");
                return ESP_OK;
            }
            ESP_LOGW(TAG,
                     "丢弃过期 UI 自检回复，当前 run=%lu/item=%d，"
                     "回复 run=%lu/item=%d",
                     (unsigned long)context->run_id,
                     (int)context->item_id,
                     (unsigned long)reply.run_id,
                     (int)reply.item_id);
        }
    }

    /* 整体超时 */
    if (lease_held)
    {
        (void)release_vibration_lease();
    }
    set_result(result,
               SELFTEST_OUTCOME_FAIL,
               SELFTEST_REASON_TIMEOUT,
               "UI_SELFTEST_TIMEOUT");
    return ESP_OK;
}

static selftest_reason_t reason_from_i2c_status(i2c_manager_status_t status)
{
    switch (status)
    {
    case I2C_MANAGER_STATUS_DEVICE_MISSING:
        return SELFTEST_REASON_DEVICE_MISSING;
    case I2C_MANAGER_STATUS_BUS_BUSY_OR_TIMEOUT:
        return SELFTEST_REASON_BUS_BUSY;
    case I2C_MANAGER_STATUS_READ_FAILED:
        return SELFTEST_REASON_READ_FAILED;
    case I2C_MANAGER_STATUS_WRITE_FAILED:
        return SELFTEST_REASON_WRITE_FAILED;
    default:
        return SELFTEST_REASON_DRIVER_FAILED;
    }
}

static selftest_reason_t reason_from_cw2015_status(cw2015_status_t status)
{
    switch (status)
    {
    case CW2015_STATUS_DEVICE_MISSING:
        return SELFTEST_REASON_DEVICE_MISSING;
    case CW2015_STATUS_BUS_BUSY:
        return SELFTEST_REASON_BUS_BUSY;
    case CW2015_STATUS_READ_FAILED:
        return SELFTEST_REASON_READ_FAILED;
    default:
        return SELFTEST_REASON_DRIVER_FAILED;
    }
}

static selftest_reason_t reason_from_qmi_status(qmi8658c_status_t status)
{
    switch (status)
    {
    case QMI8658C_STATUS_DEVICE_MISSING:
        return SELFTEST_REASON_DEVICE_MISSING;
    case QMI8658C_STATUS_BUS_BUSY:
        return SELFTEST_REASON_BUS_BUSY;
    case QMI8658C_STATUS_READ_FAILED:
        return SELFTEST_REASON_READ_FAILED;
    case QMI8658C_STATUS_WRITE_FAILED:
        return SELFTEST_REASON_WRITE_FAILED;
    default:
        return SELFTEST_REASON_DRIVER_FAILED;
    }
}

static selftest_reason_t reason_from_audio_result(audio_service_result_t audio_result)
{
    switch (audio_result)
    {
    case AUDIO_RESULT_NOT_READY:
    case AUDIO_RESULT_RESOURCE_MISSING:
        return SELFTEST_REASON_RESOURCE_MISSING;
    case AUDIO_RESULT_CORRUPT:
    case AUDIO_RESULT_UNSUPPORTED:
        return SELFTEST_REASON_RESOURCE_CORRUPT;
    case AUDIO_RESULT_BUSY:
        return SELFTEST_REASON_BUSY;
    case AUDIO_RESULT_IO_FAILED:
        return SELFTEST_REASON_READ_FAILED;
    default:
        return SELFTEST_REASON_DRIVER_FAILED;
    }
}

static audio_service_result_t audio_result_from_watch_error(watch_audio_error_t error)
{
    switch (error)
    {
    case WATCH_AUDIO_ERROR_NOT_READY:
        return AUDIO_RESULT_NOT_READY;
    case WATCH_AUDIO_ERROR_RESOURCE_MISSING:
        return AUDIO_RESULT_RESOURCE_MISSING;
    case WATCH_AUDIO_ERROR_CORRUPT:
        return AUDIO_RESULT_CORRUPT;
    case WATCH_AUDIO_ERROR_UNSUPPORTED:
        return AUDIO_RESULT_UNSUPPORTED;
    case WATCH_AUDIO_ERROR_IO_FAILED:
        return AUDIO_RESULT_IO_FAILED;
    case WATCH_AUDIO_ERROR_BUSY:
        return AUDIO_RESULT_BUSY;
    case WATCH_AUDIO_ERROR_CODEC_FAILED:
        return AUDIO_RESULT_CODEC_FAILED;
    case WATCH_AUDIO_ERROR_I2S_FAILED:
        return AUDIO_RESULT_I2S_FAILED;
    case WATCH_AUDIO_ERROR_PA_FAILED:
        return AUDIO_RESULT_PA_FAILED;
    default:
        return AUDIO_RESULT_HW_FAILED;
    }
}

static void cancel_audio_with_result(selftest_item_result_t *result,
                                     selftest_reason_t reason,
                                     const char *detail_code)
{
    esp_err_t cancel_error = audio_service_request_cancel_playback(
        pdMS_TO_TICKS(SELFTEST_AUDIO_CANCEL_TIMEOUT_MS));
    if (cancel_error != ESP_OK)
    {
        set_result(result,
                   SELFTEST_OUTCOME_FAIL,
                   SELFTEST_REASON_DRIVER_FAILED,
                   "AUDIO_CANCEL_REQUEST_FAILED");
        return;
    }
    set_result(result, SELFTEST_OUTCOME_FAIL, reason, detail_code);
}
