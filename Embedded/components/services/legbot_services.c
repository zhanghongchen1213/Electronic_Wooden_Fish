/**
 * @file     legbot_services.c
 * @brief    统一服务框架实现
 * @details  维护 EWF 固定服务描述表、服务队列、共享事件组和服务任务生命周期。
 *           默认启动图只包含 power_task 与 state_task；selftest_task 按需启动。
 * @author   ZHC
 * @date     2026-08-05
 */

#include "legbot_services.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/task.h"
#include "power_service.h"
#include "pvdf_input_service.h"
#include "selftest_service.h"
#include "state_service.h"

static const char *TAG = "SVC_CORE";

/** state_service 私有类型化队列深度。 */
#define LEGBOT_STATE_QUEUE_DEPTH 8U
/** 服务任务默认栈空间（IDF 以字节计）。 */
#define LEGBOT_SERVICE_STACK_BYTES_DEFAULT 4096U
/** 板级输入边界任务栈空间。 */
#define LEGBOT_POWER_SERVICE_STACK_BYTES 4096U
/** PVDF 候选输入边界任务栈空间。 */
#define LEGBOT_PVDF_SERVICE_STACK_BYTES 4096U
/** state_task 的状态应用与错误日志调用链专用内部栈空间。 */
#define LEGBOT_STATE_SERVICE_STACK_BYTES 4096U
/** 自检编排任务栈空间。 */
#define LEGBOT_SELFTEST_SERVICE_STACK_BYTES 6144U
/** stop_all 等待全部任务退出的总上限。 */
#define LEGBOT_SERVICE_STOP_TIMEOUT_MS 5000U
/** 停止命令进入专用队列的短有界等待。 */
#define LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS 50U

/** 固定服务描述符表，定义服务任务名称、归属组件和输入边界。 */
static const legbot_service_descriptor_t s_descriptors[LEGBOT_SERVICE_COUNT] = {
    [LEGBOT_SERVICE_POWER] = {
        .id = LEGBOT_SERVICE_POWER,
        .task_name = "power_task",
        .owner_component = "components/services/power_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_TYPED_UPDATE,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_STATE] = {
        .id = LEGBOT_SERVICE_STATE,
        .task_name = "state_task",
        .owner_component = "components/services/state_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_TYPED_UPDATE,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_SELFTEST] = {
        .id = LEGBOT_SERVICE_SELFTEST,
        .task_name = "selftest_task",
        .owner_component = "components/services/selftest_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_TYPED_UPDATE,
        .starts_by_default = false,
        .on_demand = true,
    },
    [LEGBOT_SERVICE_PVDF] = {
        .id = LEGBOT_SERVICE_PVDF,
        .task_name = "pvdf_task",
        .owner_component = "components/services/pvdf_input_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE,
        .starts_by_default = true,
    },
};

/** 服务共享事件组句柄。 */
static EventGroupHandle_t s_service_event_group;
/** state_service 私有类型化队列句柄。 */
static QueueHandle_t s_state_queue;
/** 各服务任务句柄。 */
static TaskHandle_t s_tasks[LEGBOT_SERVICE_COUNT];
/** 各服务任务是否仍在运行，由创建者与任务退出路径原子同步。 */
static atomic_bool s_task_running[LEGBOT_SERVICE_COUNT];
/** 各服务任务最近一次退出结果。 */
static atomic_int s_task_exit_errors[LEGBOT_SERVICE_COUNT];
/** 服务契约资源初始化状态。 */
static bool s_contracts_initialized;

static bool valid_service_id(legbot_service_id_t id);
static esp_err_t start_service(legbot_service_id_t id);
static esp_err_t stop_service(legbot_service_id_t id);
static esp_err_t wait_for_service_exit(legbot_service_id_t id);
static void service_task_entry(void *argument);
static void cleanup_service_contracts(void);

const legbot_service_descriptor_t *legbot_service_descriptor(legbot_service_id_t id)
{
    if (!valid_service_id(id))
    {
        return NULL;
    }
    return &s_descriptors[id];
}

size_t legbot_service_descriptor_count(void)
{
    return (size_t)LEGBOT_SERVICE_COUNT;
}

QueueHandle_t legbot_service_queue(legbot_service_id_t id)
{
    if (!valid_service_id(id))
    {
        return NULL;
    }
    /* state 使用专用类型化队列，selftest 使用自检私有命令队列。 */
    switch (id)
    {
    case LEGBOT_SERVICE_POWER:
        return power_service_queue();
    case LEGBOT_SERVICE_PVDF:
        return pvdf_input_service_queue();
    case LEGBOT_SERVICE_STATE:
    case LEGBOT_SERVICE_SELFTEST:
    default:
        return NULL;
    }
}

QueueHandle_t legbot_state_service_queue(void)
{
    return s_state_queue;
}

EventGroupHandle_t legbot_services_event_group(void)
{
    return s_service_event_group;
}

esp_err_t legbot_services_init_contracts(void)
{
    if (s_contracts_initialized)
    {
        return ESP_OK;
    }

    s_service_event_group = xEventGroupCreate();
    if (s_service_event_group == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    s_state_queue = xQueueCreate(LEGBOT_STATE_QUEUE_DEPTH,
                                 sizeof(state_service_update_t));
    if (s_state_queue == NULL)
    {
        cleanup_service_contracts();
        return ESP_ERR_NO_MEM;
    }

    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        atomic_store(&s_task_running[id], false);
        atomic_store(&s_task_exit_errors[id], ESP_OK);
        s_tasks[id] = NULL;
    }

    esp_err_t err = power_service_init_contracts();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务契约初始化失败：服务=%s，错误=%s",
                 s_descriptors[LEGBOT_SERVICE_POWER].task_name,
                 esp_err_to_name(err));
        cleanup_service_contracts();
        return err;
    }
    err = pvdf_input_service_init_contracts();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务契约初始化失败：服务=%s，错误=%s",
                 s_descriptors[LEGBOT_SERVICE_PVDF].task_name,
                 esp_err_to_name(err));
        cleanup_service_contracts();
        return err;
    }
    err = selftest_service_init_contracts();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务契约初始化失败：服务=%s，错误=%s",
                 s_descriptors[LEGBOT_SERVICE_SELFTEST].task_name,
                 esp_err_to_name(err));
        cleanup_service_contracts();
        return err;
    }

    s_contracts_initialized = true;
    return ESP_OK;
}

esp_err_t legbot_services_start_all(void)
{
    esp_err_t err = legbot_services_init_contracts();
    if (err != ESP_OK)
    {
        return err;
    }
    err = power_service_prepare_run();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "power_task 进入 prepared 状态失败，错误=%s", esp_err_to_name(err));
        return err;
    }
    err = pvdf_input_service_prepare_run();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "pvdf_task 进入 prepared 状态失败，错误=%s", esp_err_to_name(err));
        power_service_cancel_prepared_run();
        return err;
    }

    err = start_service(LEGBOT_SERVICE_STATE);
    if (err != ESP_OK)
    {
        pvdf_input_service_cancel_prepared_run();
        power_service_cancel_prepared_run();
        return err;
    }
    err = start_service(LEGBOT_SERVICE_POWER);
    if (err != ESP_OK)
    {
        (void)stop_service(LEGBOT_SERVICE_STATE);
        pvdf_input_service_cancel_prepared_run();
        return err;
    }
    /* PVDF 只纳管 IO9/IO11，不占用共享总线，排在输入边界之后启动。 */
    err = start_service(LEGBOT_SERVICE_PVDF);
    if (err != ESP_OK)
    {
        (void)stop_service(LEGBOT_SERVICE_POWER);
        (void)stop_service(LEGBOT_SERVICE_STATE);
        pvdf_input_service_cancel_prepared_run();
        return err;
    }

    ESP_LOGI(TAG,
             "默认服务已启动：state_task、power_task 与 pvdf_task；BLE/GPS/ML307R/cloud/voice/audio "
             "旧链路不在本 Story 的启动图中");
    return ESP_OK;
}

esp_err_t legbot_services_stop_all(void)
{
    /* 自检依赖 state_task 才能发布结果，因此自检必须先停，state_task 最后停。 */
    static const legbot_service_id_t stop_order[LEGBOT_SERVICE_COUNT] = {
        LEGBOT_SERVICE_SELFTEST,
        LEGBOT_SERVICE_PVDF,
        LEGBOT_SERVICE_POWER,
        LEGBOT_SERVICE_STATE,
    };
    esp_err_t first_error = ESP_OK;
    for (size_t index = 0; index < (size_t)LEGBOT_SERVICE_COUNT; ++index)
    {
        const legbot_service_id_t id = stop_order[index];
        if (!atomic_load(&s_task_running[id]))
        {
            continue;
        }
        const esp_err_t err = stop_service(id);
        if (err != ESP_OK && first_error == ESP_OK)
        {
            first_error = err;
        }
    }
    for (size_t index = 0; index < (size_t)LEGBOT_SERVICE_COUNT; ++index)
    {
        const legbot_service_id_t id = stop_order[index];
        if (!atomic_load(&s_task_running[id]))
        {
            continue;
        }
        const esp_err_t err = wait_for_service_exit(id);
        if (err != ESP_OK && first_error == ESP_OK)
        {
            first_error = err;
        }
    }
    return first_error;
}

esp_err_t legbot_selftest_service_start(void)
{
    esp_err_t err = legbot_services_init_contracts();
    if (err != ESP_OK)
    {
        return err;
    }
    err = selftest_service_prepare_run();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "selftest_task 进入 prepared 状态失败，错误=%s", esp_err_to_name(err));
        return err;
    }
    err = start_service(LEGBOT_SERVICE_SELFTEST);
    if (err != ESP_OK)
    {
        selftest_service_cancel_prepared_run();
        return err;
    }
    return selftest_service_request_run(0);
}

static esp_err_t stop_service(legbot_service_id_t id)
{
    switch (id)
    {
    case LEGBOT_SERVICE_POWER:
        return power_service_request_stop(
            pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS));
    case LEGBOT_SERVICE_STATE:
        return state_service_request_stop(
            pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS));
    case LEGBOT_SERVICE_SELFTEST:
        return selftest_service_request_stop(
            pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS));
    case LEGBOT_SERVICE_PVDF:
        return pvdf_input_service_request_stop(
            pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS));
    default:
        return ESP_ERR_INVALID_ARG;
    }
}

static esp_err_t wait_for_service_exit(legbot_service_id_t id)
{
    const TickType_t started_at = xTaskGetTickCount();
    while (atomic_load(&s_task_running[id]) &&
           xTaskGetTickCount() - started_at <
               pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_TIMEOUT_MS))
    {
        vTaskDelay(1U);
    }
    return atomic_load(&s_task_running[id]) ? ESP_ERR_TIMEOUT : ESP_OK;
}

static esp_err_t start_service(legbot_service_id_t id)
{
    if (!valid_service_id(id))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (atomic_load(&s_task_running[id]))
    {
        return ESP_OK;
    }

    uint32_t stack_bytes = LEGBOT_SERVICE_STACK_BYTES_DEFAULT;
    UBaseType_t priority = tskIDLE_PRIORITY + 3;
    switch (id)
    {
    case LEGBOT_SERVICE_POWER:
        stack_bytes = LEGBOT_POWER_SERVICE_STACK_BYTES;
        break;
    case LEGBOT_SERVICE_STATE:
        stack_bytes = LEGBOT_STATE_SERVICE_STACK_BYTES;
        priority = tskIDLE_PRIORITY + 4;
        break;
    case LEGBOT_SERVICE_SELFTEST:
        stack_bytes = LEGBOT_SELFTEST_SERVICE_STACK_BYTES;
        priority = tskIDLE_PRIORITY + 2;
        break;
    case LEGBOT_SERVICE_PVDF:
        /* 确认窗口内是有界采样循环，与板级输入边界同优先级即可满足 20 ms 窗口。 */
        stack_bytes = LEGBOT_PVDF_SERVICE_STACK_BYTES;
        priority = tskIDLE_PRIORITY + 3;
        break;
    default:
        break;
    }

    atomic_store(&s_task_exit_errors[id], ESP_OK);
    atomic_store(&s_task_running[id], true);
    /* ESP-IDF 的 xTaskCreate 以字节解释栈深度。 */
    const BaseType_t created = xTaskCreate(service_task_entry,
                                           s_descriptors[id].task_name,
                                           (configSTACK_DEPTH_TYPE)stack_bytes,
                                           (void *)(uintptr_t)id,
                                           priority,
                                           &s_tasks[id]);
    if (created != pdPASS)
    {
        atomic_store(&s_task_running[id], false);
        s_tasks[id] = NULL;
        ESP_LOGE(TAG,
                 "服务任务创建失败：服务=%s",
                 s_descriptors[id].task_name);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void service_task_entry(void *argument)
{
    const legbot_service_id_t id = (legbot_service_id_t)(uintptr_t)argument;
    esp_err_t err = ESP_OK;
    switch (id)
    {
    case LEGBOT_SERVICE_POWER:
        err = power_service_run();
        break;
    case LEGBOT_SERVICE_STATE:
        state_service_run();
        break;
    case LEGBOT_SERVICE_SELFTEST:
        err = selftest_service_run();
        break;
    case LEGBOT_SERVICE_PVDF:
        err = pvdf_input_service_run();
        break;
    default:
        err = ESP_ERR_INVALID_ARG;
        break;
    }
    atomic_store(&s_task_exit_errors[id], err);
    s_tasks[id] = NULL;
    atomic_store(&s_task_running[id], false);
    vTaskDelete(NULL);
}

static bool valid_service_id(legbot_service_id_t id)
{
    return (unsigned)id < (unsigned)LEGBOT_SERVICE_COUNT;
}

static void cleanup_service_contracts(void)
{
    (void)power_service_deinit_contracts();
    (void)pvdf_input_service_deinit_contracts();
    (void)selftest_service_deinit_contracts();
    if (s_state_queue != NULL)
    {
        vQueueDelete(s_state_queue);
        s_state_queue = NULL;
    }
    if (s_service_event_group != NULL)
    {
        vEventGroupDelete(s_service_event_group);
        s_service_event_group = NULL;
    }
    s_contracts_initialized = false;
}
