/**
 * @file     selftest_service.c
 * @brief    typed BSP 自检服务实现
 * @details  串行执行固定顺序的电源、启动绑带、复位观察、板级资源和初始化阶段自检项，
 *           每项结果固定携带资源名、观测值、错误码和证据类别，并发布到 typed 运行状态。
 *           设计输入与软件编译成功都不允许升级为 hardware_verified。
 * @author   ZHC
 * @date     2026-07-14
 */

#include "selftest_service.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "bsp_resources.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "key.h"
#include "power_service.h"
#include "pvdf_input_service.h"
#include "state_service.h"

static const char *TAG = "SELFTEST";

/** 发布 typed 自检快照的有界等待。 */
#define SELFTEST_STATE_PUBLISH_TIMEOUT_MS 100U

/** 服务状态机。 */
typedef enum
{
    SELFTEST_SERVICE_IDLE = 0, /**< 尚未创建队列。 */
    SELFTEST_SERVICE_PREPARED, /**< 队列就绪，等待框架创建任务。 */
    SELFTEST_SERVICE_RUNNING,  /**< selftest_task 正在运行。 */
} selftest_service_phase_t;

/** 有界注册表条目。 */
typedef struct
{
    bool registered;                  /**< 该 ID 是否已注册 handler。 */
    uint32_t timeout_ms;              /**< 注册时固定的超时。 */
    selftest_item_handler_t handler;  /**< 唯一 handler。 */
    void *handler_context;            /**< 长生命周期上下文。 */
} selftest_registry_entry_t;

/** 固定顺序的 canonical 自检清单。 */
static const selftest_item_metadata_t s_canonical_items[SELFTEST_ITEM_COUNT] = {
    {SELFTEST_ITEM_BSP_RESOURCE_TABLE,
     "BSP 资源表",
     2000U,
     true},
    {SELFTEST_ITEM_PWR_BOOT_INPUT,
     "PWR_INT/BOOT0 输入边界",
     3000U,
     true},
    {SELFTEST_ITEM_RESET_OBSERVATION,
     "EN/RESET_N 软件观察",
     1000U,
     true},
    {SELFTEST_ITEM_INIT_STAGES,
     "板级初始化阶段",
     1000U,
     true},
    {SELFTEST_ITEM_DISABLED_RESOURCES,
     "禁用与保留资源",
     1000U,
     true},
    {SELFTEST_ITEM_POWER_MATRIX_PENDING,
     "三种供电与 LTC2954 时序",
     1000U,
     true},
    {SELFTEST_ITEM_PVDF_INPUT,
     "PVDF 候选输入二次确认",
     2000U,
     true},
};

static QueueHandle_t s_queue;
static selftest_service_phase_t s_phase;
static selftest_registry_entry_t s_registry[SELFTEST_REGISTRY_CAPACITY];
static uint32_t s_next_run_id;
static _Atomic bool s_cancel_requested;
static _Atomic bool s_running;
static selftest_service_metrics_t s_metrics;

static esp_err_t handle_bsp_resource_table(const selftest_handler_context_t *context,
                                           selftest_item_result_t *result);
static esp_err_t handle_pwr_boot_input(const selftest_handler_context_t *context,
                                       selftest_item_result_t *result);
static esp_err_t handle_reset_observation(const selftest_handler_context_t *context,
                                          selftest_item_result_t *result);
static esp_err_t handle_init_stages(const selftest_handler_context_t *context,
                                    selftest_item_result_t *result);
static esp_err_t handle_disabled_resources(const selftest_handler_context_t *context,
                                           selftest_item_result_t *result);
static esp_err_t handle_power_matrix_pending(const selftest_handler_context_t *context,
                                             selftest_item_result_t *result);
static esp_err_t handle_pvdf_input(const selftest_handler_context_t *context,
                                   selftest_item_result_t *result);

static esp_err_t register_canonical_handlers(void);
static esp_err_t run_once(void);
static void evaluate_item(uint32_t run_id,
                          const selftest_item_metadata_t *metadata,
                          selftest_item_result_t *result);
static esp_err_t publish_item(uint32_t run_id,
                              selftest_item_id_t next_item,
                              const selftest_item_result_t *result);
static void set_detail_code(selftest_item_result_t *result, const char *text);
static void set_terminal(selftest_item_result_t *result,
                         selftest_outcome_t outcome,
                         selftest_reason_t reason,
                         selftest_evidence_t evidence,
                         const char *detail_code);
static bool registry_lookup(selftest_item_id_t item_id,
                            selftest_registry_entry_t *entry);
static const char *reset_reason_name(esp_reset_reason_t reason);

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
    if (s_queue != NULL)
    {
        return ESP_OK;
    }
    s_queue = xQueueCreate(SELFTEST_SERVICE_QUEUE_DEPTH,
                           sizeof(selftest_service_command_t));
    if (s_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t selftest_service_prepare_run(void)
{
    if (s_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = register_canonical_handlers();
    if (err != ESP_OK)
    {
        return err;
    }
    s_phase = SELFTEST_SERVICE_PREPARED;
    return ESP_OK;
}

void selftest_service_cancel_prepared_run(void)
{
    if (s_phase == SELFTEST_SERVICE_PREPARED)
    {
        s_phase = SELFTEST_SERVICE_IDLE;
    }
}

esp_err_t selftest_service_deinit_contracts(void)
{
    if (atomic_load(&s_running) || s_phase == SELFTEST_SERVICE_RUNNING)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_queue != NULL)
    {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }
    s_phase = SELFTEST_SERVICE_IDLE;
    (void)selftest_service_registry_reset();
    return ESP_OK;
}

QueueHandle_t selftest_service_queue(void)
{
    return s_queue;
}

esp_err_t selftest_service_register(selftest_item_id_t item_id,
                                    uint32_t timeout_ms,
                                    selftest_item_handler_t handler,
                                    void *handler_context)
{
    if (handler == NULL || timeout_ms == 0U ||
        timeout_ms > SELFTEST_MAX_TIMEOUT_MS ||
        (unsigned)item_id >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_registry[item_id].registered)
    {
        return ESP_ERR_INVALID_ARG;
    }
    s_registry[item_id].registered = true;
    s_registry[item_id].timeout_ms = timeout_ms;
    s_registry[item_id].handler = handler;
    s_registry[item_id].handler_context = handler_context;
    return ESP_OK;
}

esp_err_t selftest_service_registry_reset(void)
{
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    memset(s_registry, 0, sizeof(s_registry));
    return ESP_OK;
}

esp_err_t selftest_service_request_run(TickType_t timeout_ticks)
{
    /* 允许在任务尚未进入 run 循环前入队，避免启动竞态。 */
    if (s_queue == NULL ||
        (s_phase != SELFTEST_SERVICE_RUNNING &&
         s_phase != SELFTEST_SERVICE_PREPARED))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const selftest_service_command_t command = {
        .type = SELFTEST_SERVICE_COMMAND_RUN_ALL,
        .requested_at_ticks = xTaskGetTickCount(),
    };
    return xQueueSend(s_queue, &command, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t selftest_service_request_stop(TickType_t timeout_ticks)
{
    /* 允许在任务尚未进入 run 循环前投递停止命令，避免启动竞态。 */
    if (s_queue == NULL ||
        (s_phase != SELFTEST_SERVICE_RUNNING &&
         s_phase != SELFTEST_SERVICE_PREPARED))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_cancel_requested, true);
    const selftest_service_command_t command = {
        .type = SELFTEST_SERVICE_COMMAND_STOP,
        .requested_at_ticks = xTaskGetTickCount(),
    };
    return xQueueSend(s_queue, &command, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

bool selftest_service_cancel_requested(void)
{
    return atomic_load(&s_cancel_requested);
}

esp_err_t selftest_service_metrics_snapshot(selftest_service_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *metrics = s_metrics;
    return ESP_OK;
}

esp_err_t selftest_service_run(void)
{
    if (s_queue == NULL || s_phase != SELFTEST_SERVICE_PREPARED)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_running, true);
    s_phase = SELFTEST_SERVICE_RUNNING;
    s_metrics.stack_high_water_bytes = 0U;
    ESP_LOGI(TAG, "selftest_task 已进入 typed 命令循环，等待 RUN_ALL/STOP");

    for (;;)
    {
        selftest_service_command_t command = {0};
        if (xQueueReceive(s_queue, &command, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        if (command.type == SELFTEST_SERVICE_COMMAND_STOP)
        {
            break;
        }
        const esp_err_t err = run_once();
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "本轮自检未完整执行，错误=%s", esp_err_to_name(err));
        }
        s_metrics.stack_high_water_bytes =
            (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    }

    atomic_store(&s_running, false);
    s_phase = SELFTEST_SERVICE_IDLE;
    ESP_LOGW(TAG, "selftest_task 已收到 typed STOP 并退出");
    return ESP_OK;
}

static esp_err_t run_once(void)
{
    const uint32_t run_id = ++s_next_run_id;
    atomic_store(&s_cancel_requested, false);

    const watch_selftest_begin_update_t begin = {
        .run_id = run_id,
        .first_item = s_canonical_items[0].item_id,
        .started_at_ticks = xTaskGetTickCount(),
    };
    if (state_service_publish_selftest_begin(
            &begin,
            pdMS_TO_TICKS(SELFTEST_STATE_PUBLISH_TIMEOUT_MS)) != ESP_OK)
    {
        ESP_LOGW(TAG, "自检启动快照发布失败，运行继续但运行态不可见");
    }
    ESP_LOGI(TAG, "自检开始：运行 ID=%lu，项目数=%u", (unsigned long)run_id,
             (unsigned)SELFTEST_ITEM_COUNT);

    for (size_t index = 0; index < (size_t)SELFTEST_ITEM_COUNT; ++index)
    {
        const selftest_item_metadata_t *metadata = &s_canonical_items[index];
        if (selftest_service_cancel_requested())
        {
            const watch_selftest_finish_update_t cancelled = {
                .run_id = run_id,
                .state = SELFTEST_RUN_CANCELLED,
                .finished_at_ticks = xTaskGetTickCount(),
            };
            (void)state_service_publish_selftest_finish(
                &cancelled,
                pdMS_TO_TICKS(SELFTEST_STATE_PUBLISH_TIMEOUT_MS));
            ESP_LOGW(TAG, "自检被 typed STOP 取消，运行 ID=%lu", (unsigned long)run_id);
            return ESP_OK;
        }

        selftest_item_result_t result = {0};
        result.item_id = metadata->item_id;
        evaluate_item(run_id, metadata, &result);

        const selftest_item_id_t next_item =
            (index + 1U) < (size_t)SELFTEST_ITEM_COUNT
                ? s_canonical_items[index + 1U].item_id
                : SELFTEST_ITEM_COUNT;
        if (publish_item(run_id, next_item, &result) != ESP_OK)
        {
            ESP_LOGW(TAG, "自检项结果发布失败，项目=%s", metadata->name);
        }
        ESP_LOGI(TAG,
                 "自检项=%s，结果=%d，原因=%d，证据=%d，耗时=%u ms，detail=%s",
                 metadata->name,
                 (int)result.outcome,
                 (int)result.reason,
                 (int)result.evidence,
                 (unsigned)result.elapsed_ms,
                 result.detail_code);
    }

    const watch_selftest_finish_update_t finished = {
        .run_id = run_id,
        .state = SELFTEST_RUN_FINISHED,
        .finished_at_ticks = xTaskGetTickCount(),
    };
    const esp_err_t finish_error = state_service_publish_selftest_finish(
        &finished,
        pdMS_TO_TICKS(SELFTEST_STATE_PUBLISH_TIMEOUT_MS));
    ESP_LOGI(TAG,
             "自检结束：运行 ID=%lu，完成项=%u；软件结果不代表样机或电气验收",
             (unsigned long)run_id,
             (unsigned)SELFTEST_ITEM_COUNT);
    return finish_error;
}

static void evaluate_item(uint32_t run_id,
                          const selftest_item_metadata_t *metadata,
                          selftest_item_result_t *result)
{
    selftest_registry_entry_t entry = {0};
    if (!registry_lookup(metadata->item_id, &entry))
    {
        if (metadata->required_in_story)
        {
            set_terminal(result,
                         SELFTEST_OUTCOME_FAIL,
                         SELFTEST_REASON_HANDLER_MISSING,
                         SELFTEST_EVIDENCE_FAILED,
                         "SELFTEST_HANDLER_MISSING");
        }
        else
        {
            set_terminal(result,
                         SELFTEST_OUTCOME_SKIP,
                         SELFTEST_REASON_NOT_REGISTERED,
                         SELFTEST_EVIDENCE_DESIGN_INPUT,
                         "SELFTEST_NOT_REGISTERED");
        }
        return;
    }

    const selftest_handler_context_t context = {
        .run_id = run_id,
        .item_id = metadata->item_id,
        .timeout_ms = entry.timeout_ms,
        .handler_context = entry.handler_context,
    };
    set_detail_code(result, "SELFTEST_NO_OBSERVATION");
    result->outcome = SELFTEST_OUTCOME_FAIL;
    result->reason = SELFTEST_REASON_DRIVER_FAILED;
    result->evidence = SELFTEST_EVIDENCE_FAILED;

    const int64_t started_us = esp_timer_get_time();
    const esp_err_t handler_error = entry.handler(&context, result);
    const int64_t elapsed_us = esp_timer_get_time() - started_us;
    result->elapsed_ms = (uint32_t)(elapsed_us / 1000);

    if (handler_error != ESP_OK)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_DRIVER_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "SELFTEST_HANDLER_ERROR");
        return;
    }
    if (result->item_id != metadata->item_id ||
        (result->outcome != SELFTEST_OUTCOME_PASS &&
         result->outcome != SELFTEST_OUTCOME_FAIL &&
         result->outcome != SELFTEST_OUTCOME_SKIP &&
         result->outcome != SELFTEST_OUTCOME_INCONCLUSIVE))
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_INVALID_RESULT,
                     SELFTEST_EVIDENCE_FAILED,
                     "SELFTEST_INVALID_RESULT");
        return;
    }
    if ((unsigned)result->evidence >= (unsigned)SELFTEST_EVIDENCE_COUNT)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_INVALID_RESULT,
                     SELFTEST_EVIDENCE_FAILED,
                     "SELFTEST_INVALID_EVIDENCE");
        return;
    }
    if (result->evidence == SELFTEST_EVIDENCE_HARDWARE_VERIFIED &&
        SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE == 0U)
    {
        /* 没有板级回执时禁止把任何结论标记为样机已验证。 */
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_INVALID_RESULT,
                     SELFTEST_EVIDENCE_FAILED,
                     "SELFTEST_EVIDENCE_UNPROVEN");
        return;
    }
    if (memchr(result->detail_code, '\0', sizeof(result->detail_code)) == NULL)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_INVALID_RESULT,
                     SELFTEST_EVIDENCE_FAILED,
                     "SELFTEST_DETAIL_UNTERMINATED");
        return;
    }
    if (result->elapsed_ms > context.timeout_ms)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_TIMEOUT,
                     SELFTEST_EVIDENCE_FAILED,
                     "SELFTEST_ITEM_TIMEOUT");
    }
}

static esp_err_t publish_item(uint32_t run_id,
                              selftest_item_id_t next_item,
                              const selftest_item_result_t *result)
{
    const watch_selftest_item_update_t update = {
        .run_id = run_id,
        .result = *result,
        .next_item = next_item,
    };
    return state_service_publish_selftest_item(
        &update,
        pdMS_TO_TICKS(SELFTEST_STATE_PUBLISH_TIMEOUT_MS));
}

static esp_err_t register_canonical_handlers(void)
{
    if (s_registry[SELFTEST_ITEM_BSP_RESOURCE_TABLE].registered)
    {
        return ESP_OK;
    }
    (void)selftest_service_registry_reset();
    const selftest_item_metadata_t *metadata = s_canonical_items;
    esp_err_t err = selftest_service_register(
        SELFTEST_ITEM_BSP_RESOURCE_TABLE,
        metadata[SELFTEST_ITEM_BSP_RESOURCE_TABLE].timeout_ms,
        handle_bsp_resource_table,
        NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    err = selftest_service_register(SELFTEST_ITEM_PWR_BOOT_INPUT,
                                    metadata[SELFTEST_ITEM_PWR_BOOT_INPUT].timeout_ms,
                                    handle_pwr_boot_input,
                                    NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    err = selftest_service_register(
        SELFTEST_ITEM_RESET_OBSERVATION,
        metadata[SELFTEST_ITEM_RESET_OBSERVATION].timeout_ms,
        handle_reset_observation,
        NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    err = selftest_service_register(SELFTEST_ITEM_INIT_STAGES,
                                    metadata[SELFTEST_ITEM_INIT_STAGES].timeout_ms,
                                    handle_init_stages,
                                    NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    err = selftest_service_register(
        SELFTEST_ITEM_DISABLED_RESOURCES,
        metadata[SELFTEST_ITEM_DISABLED_RESOURCES].timeout_ms,
        handle_disabled_resources,
        NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    err = selftest_service_register(
        SELFTEST_ITEM_POWER_MATRIX_PENDING,
        metadata[SELFTEST_ITEM_POWER_MATRIX_PENDING].timeout_ms,
        handle_power_matrix_pending,
        NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    return selftest_service_register(SELFTEST_ITEM_PVDF_INPUT,
                                     metadata[SELFTEST_ITEM_PVDF_INPUT].timeout_ms,
                                     handle_pvdf_input,
                                     NULL);
}

static bool registry_lookup(selftest_item_id_t item_id,
                            selftest_registry_entry_t *entry)
{
    if ((unsigned)item_id >= SELFTEST_ITEM_COUNT ||
        !s_registry[item_id].registered)
    {
        return false;
    }
    if (entry != NULL)
    {
        *entry = s_registry[item_id];
    }
    return true;
}

static void set_detail_code(selftest_item_result_t *result, const char *text)
{
    if (result == NULL || text == NULL)
    {
        return;
    }
    (void)snprintf(result->detail_code, sizeof(result->detail_code), "%s", text);
}

static void set_terminal(selftest_item_result_t *result,
                         selftest_outcome_t outcome,
                         selftest_reason_t reason,
                         selftest_evidence_t evidence,
                         const char *detail_code)
{
    if (result == NULL)
    {
        return;
    }
    result->outcome = outcome;
    result->reason = reason;
    result->evidence = evidence;
    set_detail_code(result, detail_code);
}

static esp_err_t handle_bsp_resource_table(const selftest_handler_context_t *context,
                                           selftest_item_result_t *result)
{
    (void)context;
    result->item_id = SELFTEST_ITEM_BSP_RESOURCE_TABLE;

    const legbot_bsp_resource_t *i2c0 =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_I2C0_SHARED);
    const legbot_bsp_resource_t *pwr =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_PWR_INT);
    const legbot_bsp_resource_t *boot =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_BOOT0);
    const legbot_bsp_resource_t *pvdf =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_PVDF);
    const legbot_bsp_resource_t *reset_n =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_RESET_N_EN);
    const legbot_bsp_resource_t *reserved =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_RESERVED_PINS);
    const legbot_bsp_resource_t *usb =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_USB_SERIAL_JTAG);
    const legbot_bsp_resource_t *air780 =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_AIR780EGP_UART);
    const legbot_bsp_resource_t *qmi =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_QMI8658C);
    const legbot_bsp_resource_t *sdmmc =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_SDMMC_POLICY);

    const bool table_complete =
        legbot_bsp_resource_count() == LEGBOT_BSP_RESOURCE_COUNT && i2c0 != NULL &&
        pwr != NULL && boot != NULL && pvdf != NULL && reset_n != NULL &&
        reserved != NULL && usb != NULL && air780 != NULL && qmi != NULL &&
        sdmmc != NULL;
    const bool pins_match =
        table_complete && i2c0->gpio_primary == GPIO_NUM_1 &&
        i2c0->gpio_secondary == GPIO_NUM_2 &&
        i2c0->i2c_port == I2C_NUM_0 && pwr->gpio_primary == EWF_BSP_PWR_INT_GPIO &&
        pwr->reserved && boot->gpio_primary == EWF_BSP_BOOT0_GPIO &&
        boot->reserved && pvdf->gpio_primary == EWF_BSP_PVDF_ADC_GPIO &&
        pvdf->gpio_secondary == EWF_BSP_PVDF_CMP_WAKE_GPIO && !pvdf->reserved &&
        pvdf->init_stage == LEGBOT_BSP_STAGE_BOARD_PINS &&
        reset_n->gpio_primary == EWF_BSP_EN_RESET_N_GPIO &&
        reset_n->gpio_secondary == EWF_BSP_KILL_LTC2954_GPIO &&
        reset_n->gpio_aux0 == EWF_BSP_PWR_STATE_LTC2954_EN_GPIO &&
        reserved->gpio_primary == EWF_BSP_IO46_RESERVED_GPIO &&
        reserved->gpio_secondary == EWF_BSP_IO45_RESERVED_GPIO &&
        reserved->reserved && usb->gpio_primary == EWF_BSP_USB_SERIAL_JTAG_DM_GPIO &&
        usb->gpio_secondary == EWF_BSP_USB_SERIAL_JTAG_DP_GPIO &&
        air780->gpio_primary == EWF_BSP_AIR780EGP_TX_GPIO &&
        air780->gpio_secondary == EWF_BSP_AIR780EGP_RX_GPIO &&
        air780->gpio_aux0 == EWF_BSP_AIR780EGP_DTR_GPIO &&
        air780->gpio_aux1 == EWF_BSP_AIR780EGP_RST_GPIO &&
        air780->uart_port == EWF_BSP_AIR780EGP_UART_PORT &&
        qmi->gpio_secondary == GPIO_NUM_NC && sdmmc->disabled;

    ESP_LOGI(TAG,
             "资源表观测：I2C0=IO%u/IO%u，PWR_INT=IO%d，BOOT0=IO%d，PVDF=IO%d/IO%d 已纳管，"
             "IO46/IO45 保留，USB=IO%d/IO%d，Air780EGP=IO%d/IO%d",
             (unsigned)i2c0->gpio_primary,
             (unsigned)i2c0->gpio_secondary,
             (int)EWF_BSP_PWR_INT_GPIO,
             (int)EWF_BSP_BOOT0_GPIO,
             (int)EWF_BSP_PVDF_ADC_GPIO,
             (int)EWF_BSP_PVDF_CMP_WAKE_GPIO,
             (int)EWF_BSP_USB_SERIAL_JTAG_DM_GPIO,
             (int)EWF_BSP_USB_SERIAL_JTAG_DP_GPIO,
             (int)EWF_BSP_AIR780EGP_TX_GPIO,
             (int)EWF_BSP_AIR780EGP_RX_GPIO);

    if (!table_complete)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_RESOURCE_MISSING,
                     SELFTEST_EVIDENCE_FAILED,
                     "BSP_TABLE_INCOMPLETE");
        return ESP_OK;
    }
    if (!pins_match)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_DRIVER_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "BSP_TABLE_MISMATCH");
        return ESP_OK;
    }
    if (key_gpio(LEGBOT_KEY_PWR) != EWF_BSP_PWR_INT_GPIO ||
        key_gpio(LEGBOT_KEY_BOOT) != EWF_BSP_BOOT0_GPIO)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_DRIVER_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "BSP_KEY_MAPPING_MISMATCH");
        return ESP_OK;
    }

    set_terminal(result,
                 SELFTEST_OUTCOME_PASS,
                 SELFTEST_REASON_NONE,
                 SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                 "BSP_TABLE_OK");
    return ESP_OK;
}

static esp_err_t handle_pwr_boot_input(const selftest_handler_context_t *context,
                                       selftest_item_result_t *result)
{
    result->item_id = SELFTEST_ITEM_PWR_BOOT_INPUT;

    power_service_snapshot_t snapshot = {0};
    const uint32_t observe_budget_ms = context->timeout_ms;
    uint32_t waited_ms = 0U;
    while (waited_ms < observe_budget_ms)
    {
        if (selftest_service_cancel_requested())
        {
            set_terminal(result,
                         SELFTEST_OUTCOME_FAIL,
                         SELFTEST_REASON_CANCELLED,
                         SELFTEST_EVIDENCE_FAILED,
                         "PWR_BOOT_CANCELLED");
            return ESP_OK;
        }
        if (power_service_snapshot(&snapshot) == ESP_OK && snapshot.isr_registered)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
        waited_ms += SELFTEST_SERVICE_POLL_MS;
    }
    if (!snapshot.isr_registered)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_INCONCLUSIVE,
                     SELFTEST_REASON_WINDOW_EXPIRED,
                     SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                     "PWR_BOOT_ISR_NOT_ARMED");
        return ESP_OK;
    }
    if (power_service_snapshot(&snapshot) != ESP_OK)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_READ_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "PWR_BOOT_SNAPSHOT_FAILED");
        return ESP_OK;
    }

    char detail[SELFTEST_DETAIL_CODE_CAPACITY] = {0};
    (void)snprintf(detail,
                   sizeof(detail),
                   "PWR8=%u,BOOT0=%u,LOW=%u",
                   (unsigned)snapshot.pwr_level,
                   (unsigned)snapshot.boot_level,
                   snapshot.pwr_low ? 1U : 0U);
    ESP_LOGI(TAG,
             "PWR_INT/BOOT0 原始观测：PWR=%u，BOOT0=%u，低电平区间=%s，"
             "累计按压区间=%u，累计运行态短按=%u；该结论只证明软件输入边界，"
             "不证明电气电平与 PWR 时序",
             (unsigned)snapshot.pwr_level,
             (unsigned)snapshot.boot_level,
             snapshot.pwr_low ? "进行中" : "无",
             (unsigned)snapshot.pwr_interval_count,
             (unsigned)snapshot.boot_tap_count);
    set_terminal(result,
                 SELFTEST_OUTCOME_PASS,
                 SELFTEST_REASON_NONE,
                 SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                 detail);
    return ESP_OK;
}

static esp_err_t handle_reset_observation(const selftest_handler_context_t *context,
                                          selftest_item_result_t *result)
{
    (void)context;
    result->item_id = SELFTEST_ITEM_RESET_OBSERVATION;
    const esp_reset_reason_t reason = esp_reset_reason();
    char detail[SELFTEST_DETAIL_CODE_CAPACITY] = {0};
    (void)snprintf(detail,
                   sizeof(detail),
                   "RESET_REASON_%s",
                   reset_reason_name(reason));
    ESP_LOGI(TAG,
             "EN/RESET_N 只有软件观察：reset_reason=%s；EN 是独立硬件复位输入/测试点，"
             "本项不是 RESET_N 波形验证",
             reset_reason_name(reason));
    set_terminal(result,
                 SELFTEST_OUTCOME_PASS,
                 SELFTEST_REASON_NONE,
                 SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                 detail);
    return ESP_OK;
}

static esp_err_t handle_init_stages(const selftest_handler_context_t *context,
                                    selftest_item_result_t *result)
{
    (void)context;
    result->item_id = SELFTEST_ITEM_INIT_STAGES;

    /* SELFTEST_HOOKS 阶段正在运行本项，因此只评估其之前的阶段。 */
    static const legbot_bsp_init_stage_t evaluated_stages[] = {
        LEGBOT_BSP_STAGE_LOG_NVS_CONFIG,
        LEGBOT_BSP_STAGE_EVENT_STATE,
        LEGBOT_BSP_STAGE_BOARD_PINS,
        LEGBOT_BSP_STAGE_SHARED_BUSES,
        LEGBOT_BSP_STAGE_SERVICES,
    };
    const size_t stage_count =
        sizeof(evaluated_stages) / sizeof(evaluated_stages[0]);

    for (size_t index = 0; index < stage_count; ++index)
    {
        const legbot_bsp_init_stage_t stage = evaluated_stages[index];
        char detail[SELFTEST_DETAIL_CODE_CAPACITY] = {0};
        if (!legbot_bsp_stage_result_recorded(stage))
        {
            (void)snprintf(detail,
                           sizeof(detail),
                           "STAGE_%s_UNRECORDED",
                           legbot_bsp_stage_name(stage));
            set_terminal(result,
                         SELFTEST_OUTCOME_INCONCLUSIVE,
                         SELFTEST_REASON_WINDOW_EXPIRED,
                         SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                         detail);
            return ESP_OK;
        }
        const esp_err_t stage_result = legbot_bsp_stage_result(stage);
        if (stage_result != ESP_OK)
        {
            (void)snprintf(detail,
                           sizeof(detail),
                           "STAGE_%s_%s",
                           legbot_bsp_stage_name(stage),
                           legbot_bsp_error_code(stage, stage_result));
            ESP_LOGE(TAG,
                     "初始化阶段 %s 返回非零：稳定错误码=%s",
                     legbot_bsp_stage_name(stage),
                     legbot_bsp_error_code(stage, stage_result));
            set_terminal(result,
                         SELFTEST_OUTCOME_FAIL,
                         SELFTEST_REASON_DRIVER_FAILED,
                         SELFTEST_EVIDENCE_FAILED,
                         detail);
            return ESP_OK;
        }
    }

    char detail[SELFTEST_DETAIL_CODE_CAPACITY] = {0};
    (void)snprintf(detail, sizeof(detail), "STAGES_OK_%u", (unsigned)stage_count);
    ESP_LOGI(TAG, "初始化阶段观测：%u 个阶段均已记录且返回 ESP_OK",
             (unsigned)stage_count);
    set_terminal(result,
                 SELFTEST_OUTCOME_PASS,
                 SELFTEST_REASON_NONE,
                 SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                 detail);
    return ESP_OK;
}

static esp_err_t handle_disabled_resources(const selftest_handler_context_t *context,
                                           selftest_item_result_t *result)
{
    (void)context;
    result->item_id = SELFTEST_ITEM_DISABLED_RESOURCES;

    const legbot_bsp_resource_t *sdmmc =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_SDMMC_POLICY);
    const legbot_bsp_resource_t *reset_n =
        legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_RESET_N_EN);
    if (sdmmc == NULL || reset_n == NULL)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_RESOURCE_MISSING,
                     SELFTEST_EVIDENCE_FAILED,
                     "DISABLED_TABLE_MISSING");
        return ESP_OK;
    }

    const bool firmware_never_drives =
        LEGBOT_BSP_SDMMC_ENABLED == 0 && sdmmc->disabled &&
        reset_n->gpio_primary == GPIO_NUM_NC &&
        reset_n->gpio_secondary == GPIO_NUM_NC &&
        reset_n->gpio_aux0 == GPIO_NUM_NC &&
        LEGBOT_BSP_QMI8658C_INT2_GPIO == GPIO_NUM_NC;
    if (!firmware_never_drives)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_DRIVER_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "DISABLED_RESOURCE_TAKEN");
        return ESP_OK;
    }

    ESP_LOGI(TAG,
             "禁用资源清单：SDMMC=禁用；EN/RESET_N、PWR_STATE、KILL 均不接 ESP32 GPIO；"
             "QMI8658A INT2 未占用 IO45；GPS/ML307R/振动旧组件已移出启动图");
    set_terminal(result,
                 SELFTEST_OUTCOME_PASS,
                 SELFTEST_REASON_NONE,
                 SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                 "DISABLED_RESOURCES_OK");
    return ESP_OK;
}

static esp_err_t handle_power_matrix_pending(const selftest_handler_context_t *context,
                                             selftest_item_result_t *result)
{
    (void)context;
    result->item_id = SELFTEST_ITEM_POWER_MATRIX_PENDING;
    ESP_LOGW(TAG,
             "待样机验证项：rail 电压、LTC2954 时序、USB/下载采样和"
             "USB-only/电池-only/USB+电池三种供电行为均无示波器或实板回执；"
             "本次运行不输出 PASS");
    set_terminal(result,
                 SELFTEST_OUTCOME_INCONCLUSIVE,
                 SELFTEST_REASON_WINDOW_EXPIRED,
                 SELFTEST_EVIDENCE_HARDWARE_PENDING,
                 "HW_PENDING_NO_PROTOTYPE");
    return ESP_OK;
}

static esp_err_t handle_pvdf_input(const selftest_handler_context_t *context,
                                   selftest_item_result_t *result)
{
    result->item_id = SELFTEST_ITEM_PVDF_INPUT;

    pvdf_input_service_snapshot_t snapshot = {0};
    const uint32_t observe_budget_ms = context->timeout_ms;
    uint32_t waited_ms = 0U;
    while (waited_ms < observe_budget_ms)
    {
        if (selftest_service_cancel_requested())
        {
            set_terminal(result,
                         SELFTEST_OUTCOME_FAIL,
                         SELFTEST_REASON_CANCELLED,
                         SELFTEST_EVIDENCE_FAILED,
                         "PVDF_CANCELLED");
            return ESP_OK;
        }
        if (pvdf_input_service_snapshot(&snapshot) == ESP_OK &&
            snapshot.isr_registered)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(SELFTEST_SERVICE_POLL_MS));
        waited_ms += SELFTEST_SERVICE_POLL_MS;
    }
    if (!snapshot.isr_registered)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_INCONCLUSIVE,
                     SELFTEST_REASON_WINDOW_EXPIRED,
                     SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                     "PVDF_ISR_NOT_ARMED");
        return ESP_OK;
    }
    if (pvdf_input_service_snapshot(&snapshot) != ESP_OK)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_READ_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "PVDF_SNAPSHOT_FAILED");
        return ESP_OK;
    }

    pvdf_selfcheck_result_t selfcheck = {0};
    if (pvdf_input_service_selfcheck(&selfcheck) != ESP_OK)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_READ_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "PVDF_SELFCHECK_READ_FAILED");
        return ESP_OK;
    }

    ESP_LOGI(TAG,
             "PVDF 候选输入观测：单次敲击候选=%lu，1 秒 20 次候选=%lu，误触发=%lu，"
             "环境振动拒绝=%lu，越界拒绝=%lu，盲窗拒绝=%lu，采样失败拒绝=%lu，"
             "唤醒累计=%lu，候选累计=%lu；本项只证明判定逻辑确定性，"
             "电气结论待 HW-OI-008 样机验证",
             (unsigned long)selfcheck.counts
                 .candidate_count[EWF_PVDF_SELFCHECK_SINGLE_TAP],
             (unsigned long)selfcheck.counts
                 .candidate_count[EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND],
             (unsigned long)selfcheck.counts.total_false_triggers,
             (unsigned long)selfcheck.counts
                 .below_threshold_count[EWF_PVDF_SELFCHECK_CARRY_VIBRATION],
             (unsigned long)selfcheck.counts
                 .out_of_range_count[EWF_PVDF_SELFCHECK_OUT_OF_RANGE],
             (unsigned long)selfcheck.counts
                 .blind_window_count[EWF_PVDF_SELFCHECK_BLIND_WINDOW],
             (unsigned long)selfcheck.counts
                 .sample_failed_count[EWF_PVDF_SELFCHECK_SAMPLE_FAILURE],
             (unsigned long)snapshot.wake_event_count,
             (unsigned long)snapshot.candidate_count);

    if (!selfcheck.all_expectations_met)
    {
        set_terminal(result,
                     SELFTEST_OUTCOME_FAIL,
                     SELFTEST_REASON_DRIVER_FAILED,
                     SELFTEST_EVIDENCE_FAILED,
                     "PVDF_POLICY_MISMATCH_HW_OI_008_PENDING");
        return ESP_OK;
    }
    if (snapshot.degraded)
    {
        /* 映射或校准降级时只报观察窗口到期，不冒充策略已通过。 */
        set_terminal(result,
                     SELFTEST_OUTCOME_INCONCLUSIVE,
                     SELFTEST_REASON_WINDOW_EXPIRED,
                     SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                     "PVDF_DEGRADED_HW_OI_008_PENDING");
        return ESP_OK;
    }
    set_terminal(result,
                 SELFTEST_OUTCOME_PASS,
                 SELFTEST_REASON_NONE,
                 SELFTEST_EVIDENCE_SOFTWARE_OBSERVED,
                 "PVDF_POLICY_OK_HW_OI_008_PENDING");
    return ESP_OK;
}

static const char *reset_reason_name(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON:
        return "POWERON";
    case ESP_RST_EXT:
        return "EXTERNAL_PIN";
    case ESP_RST_SW:
        return "SOFTWARE";
    case ESP_RST_PANIC:
        return "PANIC";
    case ESP_RST_INT_WDT:
        return "INT_WDT";
    case ESP_RST_TASK_WDT:
        return "TASK_WDT";
    case ESP_RST_WDT:
        return "WDT";
    case ESP_RST_DEEPSLEEP:
        return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT";
    case ESP_RST_SDIO:
        return "SDIO";
    case ESP_RST_USB:
        return "USB";
    case ESP_RST_JTAG:
        return "JTAG";
    case ESP_RST_EFUSE:
        return "EFUSE";
    case ESP_RST_PWR_GLITCH:
        return "PWR_GLITCH";
    case ESP_RST_CPU_LOCKUP:
        return "CPU_LOCKUP";
    case ESP_RST_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}
