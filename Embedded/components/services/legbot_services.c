/**
 * @file     legbot_services.c
 * @brief    统一服务框架实现
 * @details  维护固定服务描述表、服务队列、共享事件组和服务任务生命周期。
 * @author   ZHC
 * @date     2026-08-05
 */

#include "legbot_services.h"

#include <stdint.h>
#include <stdatomic.h>
#include <string.h>

#include "audio_service.h"
#include "ble_service.h"
#if LEGBOT_CAP_CLOUD
#include "cloud_service.h"
#endif
#include "config_service.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "gps_service.h"
#if LEGBOT_CAP_MODEM
#include "modem_service.h"
#endif
#include "power_service.h"
#include "selftest_service.h"
#include "state_service.h"
#include "time_service.h"
#include "ui_service.h"
#include "voice_service.h"

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_GPS_TIME) || !defined(LEGBOT_CAP_GPS_CONTINUOUS_LOCATION) || \
    !defined(LEGBOT_CAP_MODEM) || !defined(LEGBOT_CAP_CLOUD)
#error "LEGBOT capability macros must be defined."
#endif

static const char *TAG = "SVC_CORE";

/** 每个服务消息队列的默认深度。 */
#define LEGBOT_SERVICE_QUEUE_DEPTH 8

/** 服务桩任务默认栈大小。 */
#define LEGBOT_SERVICE_STACK_BYTES 3072
/** state task 的状态应用、NVS 重投与错误日志调用链专用内部栈空间。 */
#define LEGBOT_STATE_SERVICE_STACK_BYTES 4096
/** 自检编排任务的栈空间。 */
#define LEGBOT_SELFTEST_SERVICE_STACK_BYTES 5120
/** modem task 需要容纳有界 AT transaction 与 URL/HTTP 格式化栈。 */
#define LEGBOT_MODEM_SERVICE_STACK_BYTES 8192
/** cloud task 需要容纳严格 JSON 编解码、HTTPS 值对象与完整产品状态快照。 */
#define LEGBOT_CLOUD_SERVICE_STACK_BYTES 15360
/** voice task 模型初始化允许占用的最长启动窗口。 */
#define LEGBOT_VOICE_SERVICE_START_TIMEOUT_MS 7000
/** ui_task 固定运行的 ESP32-S3 核心。 */
#define LEGBOT_UI_SERVICE_CORE 1
/** ui_task 优先级，确保横滑渲染高于普通服务任务。 */
#define LEGBOT_UI_SERVICE_PRIORITY (tskIDLE_PRIORITY + 3)
/** stop_all 总上限覆盖 3 秒诊断片段及 terminal apply ACK 清理。 */
#define LEGBOT_SERVICE_STOP_TIMEOUT_MS 5000
/** producer 停止命令进入专用队列的短有界等待。 */
#define LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS 50
/** 分阶段启动等待上限。 */
#define LEGBOT_SERVICE_START_PHASE_TIMEOUT_MS 2000
/** BLE 冷启动等待上限，覆盖 NimBLE host 的 5 秒同步门限。 */
#define LEGBOT_BLE_SERVICE_START_TIMEOUT_MS 6000
/** v2 首次 BLE PHY 唤醒、I2S DMA 及运行期控制块所需内部 DMA 最低余量。 */
#define LEGBOT_RUNTIME_INTERNAL_DMA_FLOOR_BYTES (16U * 1024U)
/** 防止总空闲尚可但碎片已无法承载后续内部 DMA 块。 */
#define LEGBOT_RUNTIME_INTERNAL_DMA_LARGEST_BLOCK_FLOOR_BYTES (8U * 1024U)
/** 加载中文 MultiNet 后必须保留的 PSRAM 总空闲。 */
#define LEGBOT_RUNTIME_PSRAM_FLOOR_BYTES (1024U * 1024U)
/** 加载中文 MultiNet 后必须保留的 PSRAM 最大连续块。 */
#define LEGBOT_RUNTIME_PSRAM_LARGEST_BLOCK_FLOOR_BYTES (1024U * 1024U)
/** voice task 不执行 model 分区 mmap/munmap，固定栈保留在 PSRAM。 */
#define LEGBOT_VOICE_TASK_STACK_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

/** 常驻服务启动阶段，先保留 NimBLE 低功耗资源，再创建其余任务栈与音频 DMA。 */
typedef enum
{
    LEGBOT_SERVICE_START_PHASE_TASKS = 0, /**< 正在创建全部常驻任务栈。 */
    LEGBOT_SERVICE_START_PHASE_BLE,       /**< 仅放行 BLE 初始化。 */
    LEGBOT_SERVICE_START_PHASE_AUDIO,     /**< BLE 已就绪，继续放行音频 DMA。 */
    LEGBOT_SERVICE_START_PHASE_VOICE,     /**< 音频 DMA 已就绪，继续加载语音模型。 */
    LEGBOT_SERVICE_START_PHASE_ALL,       /**< 关键资源已落位，放行其他服务。 */
} legbot_service_start_phase_t;

/** 固定服务描述符表，定义服务任务名称、归属组件和输入边界。 */
static const legbot_service_descriptor_t s_descriptors[LEGBOT_SERVICE_COUNT] = {
    [LEGBOT_SERVICE_UI] = {
        .id = LEGBOT_SERVICE_UI,
        .task_name = "ui_task",
        .owner_component = "components/services/ui_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_TYPED_UPDATE,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_BLE] = {
        .id = LEGBOT_SERVICE_BLE,
        .task_name = "ble_task",
        .owner_component = "components/services/ble_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_EVENT_GROUP,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_MODEM] = {
        .id = LEGBOT_SERVICE_MODEM,
        .task_name = "modem_task",
        .owner_component = "components/services/modem_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_EVENT_GROUP,
        .starts_by_default = LEGBOT_CAP_MODEM == 1,
    },
    [LEGBOT_SERVICE_GPS] = {
        .id = LEGBOT_SERVICE_GPS,
        .task_name = "gps_task",
        .owner_component = "components/services/gps_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE,
        .starts_by_default =
            LEGBOT_CAP_GPS_TIME == 1 ||
            LEGBOT_CAP_GPS_CONTINUOUS_LOCATION == 1,
    },
    [LEGBOT_SERVICE_AUDIO] = {
        .id = LEGBOT_SERVICE_AUDIO,
        .task_name = "audio_task",
        .owner_component = "components/services/audio_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_MUTEX_BOUNDARY,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_POWER] = {
        .id = LEGBOT_SERVICE_POWER,
        .task_name = "power_task",
        .owner_component = "components/services/power_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_EVENT_GROUP,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_CLOUD] = {
        .id = LEGBOT_SERVICE_CLOUD,
        .task_name = "cloud_task",
        .owner_component = "components/services/cloud_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_TYPED_UPDATE,
        .starts_by_default = LEGBOT_CAP_CLOUD == 1,
    },
    [LEGBOT_SERVICE_STATE] = {
        .id = LEGBOT_SERVICE_STATE,
        .task_name = "state_task",
        .owner_component = "components/services/state_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE | LEGBOT_SERVICE_INPUT_TYPED_UPDATE,
        .starts_by_default = true,
    },
    [LEGBOT_SERVICE_LOG] = {
        .id = LEGBOT_SERVICE_LOG,
        .task_name = "log_task",
        .owner_component = "components/services/log_service",
        .input_mask = LEGBOT_SERVICE_INPUT_QUEUE,
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
    [LEGBOT_SERVICE_VOICE] = {
        .id = LEGBOT_SERVICE_VOICE,
        .task_name = "voice_task",
        .owner_component = "components/services/voice_service",
        .input_mask = LEGBOT_SERVICE_INPUT_TYPED_UPDATE |
                      LEGBOT_SERVICE_INPUT_MUTEX_BOUNDARY,
        .starts_by_default = true,
    },
};

/** 各服务消息队列句柄。 */
static QueueHandle_t s_queues[LEGBOT_SERVICE_COUNT];

/** 各服务任务句柄。 */
static TaskHandle_t s_tasks[LEGBOT_SERVICE_COUNT];
/** 自检任务固定栈，避免按需启动受内部堆碎片影响。 */
static StackType_t s_selftest_task_stack[LEGBOT_SELFTEST_SERVICE_STACK_BYTES];
/** 自检任务固定控制块，确保启动不再动态申请 TCB。 */
static StaticTask_t s_selftest_task_buffer;
/** 各服务任务是否仍在运行，由创建者与任务退出路径原子同步。 */
static atomic_bool s_task_running[LEGBOT_SERVICE_COUNT];
/** 各服务任务最近一次退出结果。 */
static atomic_int s_task_exit_errors[LEGBOT_SERVICE_COUNT];
/** 堆分配的低实时服务 PSRAM 静态任务栈；访问 Flash/NVS 的任务不得使用。 */
static StackType_t *s_external_task_stacks[LEGBOT_SERVICE_COUNT];
/** 堆分配静态栈任务对应的内部静态任务控制块。 */
static StaticTask_t s_external_task_buffers[LEGBOT_SERVICE_COUNT];
/** 当前常驻服务启动阶段。 */
static atomic_int s_start_phase;

_Static_assert(sizeof(s_selftest_task_stack) ==
                   LEGBOT_SELFTEST_SERVICE_STACK_BYTES,
               "Self-test static task stack must use the IDF byte-size contract.");

/** 服务共享事件组句柄。 */
static EventGroupHandle_t s_service_event_group;

/** 服务契约资源初始化状态。 */
static bool s_contracts_initialized;
/** 启动配置 ACK 校验快照，避免完整产品状态占用 main 任务栈。 */
static watch_state_snapshot_t s_initial_config_applied_snapshot;

static void cleanup_service_contracts(void);
static bool valid_service_id(legbot_service_id_t id);
static bool service_allowed_by_profile(legbot_service_id_t id);
static bool service_uses_external_stack(legbot_service_id_t id);
static bool service_queue_uses_external_storage(legbot_service_id_t id);
static esp_err_t ensure_service_queue(legbot_service_id_t id);
static esp_err_t wait_for_service_runtime_ready(legbot_service_id_t id);
static void service_stub_task(void *arg);
static esp_err_t start_service(legbot_service_id_t id);
static esp_err_t publish_config_snapshot(const config_service_snapshot_t *config,
                                         watch_config_update_t *published_update);
static esp_err_t publish_config_change(const config_service_snapshot_t *config,
                                       void *context);
static esp_err_t publish_initial_config_truth(void);

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
    return LEGBOT_SERVICE_COUNT;
}

QueueHandle_t legbot_service_queue(legbot_service_id_t id)
{
    if (!valid_service_id(id) || id == LEGBOT_SERVICE_STATE || id == LEGBOT_SERVICE_AUDIO ||
        id == LEGBOT_SERVICE_GPS || id == LEGBOT_SERVICE_SELFTEST ||
        id == LEGBOT_SERVICE_VOICE)
    {
        return NULL;
    }
    return s_queues[id];
}

QueueHandle_t legbot_state_service_queue(void)
{
    return s_queues[LEGBOT_SERVICE_STATE];
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

    time_service_reset();
    s_service_event_group = xEventGroupCreate();
    if (s_service_event_group == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        atomic_store(&s_task_running[id], false);
        atomic_store(&s_task_exit_errors[id], ESP_OK);
    }
    atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_TASKS);

    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        if (!service_allowed_by_profile(id))
        {
            continue;
        }
        esp_err_t err = ensure_service_queue(id);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "服务契约初始化失败：服务=%s，错误=%s",
                     s_descriptors[id].task_name,
                     esp_err_to_name(err));
            cleanup_service_contracts();
            return err;
        }
    }

    esp_err_t config_callback_err = config_service_set_change_callback(
        publish_config_change,
        NULL);
    if (config_callback_err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务契约初始化失败：边界=config_callback，错误=%s",
                 esp_err_to_name(config_callback_err));
        cleanup_service_contracts();
        return config_callback_err;
    }

    esp_err_t audio_state_err = audio_service_publish_initial_state();
    if (audio_state_err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务契约初始化失败：边界=audio_initial_state，错误=%s",
                 esp_err_to_name(audio_state_err));
        cleanup_service_contracts();
        return audio_state_err;
    }

    esp_err_t err = ui_service_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务契约初始化失败：边界=ui_service，错误=%s",
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
    atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_TASKS);

    const legbot_service_descriptor_t *state_descriptor =
        legbot_service_descriptor(LEGBOT_SERVICE_STATE);
    if (state_descriptor == NULL || !state_descriptor->starts_by_default)
    {
        return ESP_ERR_INVALID_STATE;
    }
    err = start_service(LEGBOT_SERVICE_STATE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务启动失败：服务=%s，错误=%s",
                 state_descriptor->task_name,
                 esp_err_to_name(err));
        (void)legbot_services_stop_all();
        return err;
    }
    err = publish_initial_config_truth();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务启动失败：边界=initial_config_truth，错误=%s",
                 esp_err_to_name(err));
        (void)legbot_services_stop_all();
        return err;
    }

    /* BLE controller 的 1100 字节 RETENTION 块必须先于普通内部任务栈落位。 */
    const legbot_service_descriptor_t *ble_descriptor =
        legbot_service_descriptor(LEGBOT_SERVICE_BLE);
    if (ble_descriptor == NULL || !ble_descriptor->starts_by_default)
    {
        (void)legbot_services_stop_all();
        return ESP_ERR_INVALID_STATE;
    }
    err = start_service(LEGBOT_SERVICE_BLE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务启动失败：服务=%s，错误=%s",
                 ble_descriptor->task_name,
                 esp_err_to_name(err));
        (void)legbot_services_stop_all();
        return err;
    }
    atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_BLE);
    err = wait_for_service_runtime_ready(LEGBOT_SERVICE_BLE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务分阶段启动失败：阶段=BLE，错误=%s",
                 esp_err_to_name(err));
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        (void)legbot_services_stop_all();
        return err;
    }

    /* BLE retention 落位后，再预留 voice 的固定 PSRAM 栈。 */
    if (s_external_task_stacks[LEGBOT_SERVICE_VOICE] == NULL)
    {
        s_external_task_stacks[LEGBOT_SERVICE_VOICE] =
            heap_caps_malloc(VOICE_SERVICE_TASK_STACK_BYTES,
                             LEGBOT_VOICE_TASK_STACK_CAPS);
    }
    if (s_external_task_stacks[LEGBOT_SERVICE_VOICE] == NULL)
    {
        ESP_LOGE(TAG,
                 "voice task PSRAM 固定栈预分配失败，禁用语音并继续启动现有 UI/BLE");
    }

    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        const legbot_service_descriptor_t *descriptor = legbot_service_descriptor(id);
        if (id == LEGBOT_SERVICE_STATE || id == LEGBOT_SERVICE_BLE ||
            id == LEGBOT_SERVICE_AUDIO || id == LEGBOT_SERVICE_VOICE ||
            !descriptor->starts_by_default)
        {
            continue;
        }
        err = start_service(id);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "服务启动失败：服务=%s，错误=%s",
                     descriptor->task_name,
                     esp_err_to_name(err));
            atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
            (void)legbot_services_stop_all();
            return err;
        }
    }

    /* 所有常驻任务栈先落位，最后才允许音频驱动占用大块内部 DMA 堆。 */
    const legbot_service_descriptor_t *audio_descriptor =
        legbot_service_descriptor(LEGBOT_SERVICE_AUDIO);
    if (audio_descriptor == NULL || !audio_descriptor->starts_by_default)
    {
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        (void)legbot_services_stop_all();
        return ESP_ERR_INVALID_STATE;
    }
    err = start_service(LEGBOT_SERVICE_AUDIO);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务启动失败：服务=%s，错误=%s",
                 audio_descriptor->task_name,
                 esp_err_to_name(err));
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        (void)legbot_services_stop_all();
        return err;
    }

    atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_AUDIO);
    err = wait_for_service_runtime_ready(LEGBOT_SERVICE_AUDIO);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "服务分阶段启动失败：阶段=音频，错误=%s",
                 esp_err_to_name(err));
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        (void)legbot_services_stop_all();
        return err;
    }

    const size_t runtime_internal_dma_free =
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    const size_t runtime_internal_dma_largest =
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (runtime_internal_dma_free < LEGBOT_RUNTIME_INTERNAL_DMA_FLOOR_BYTES ||
        runtime_internal_dma_largest <
            LEGBOT_RUNTIME_INTERNAL_DMA_LARGEST_BLOCK_FLOOR_BYTES)
    {
        ESP_LOGE(TAG,
                 "服务启动拒绝进入运行态：内部 DMA 空闲/最大块=%lu/%lu 字节，最低要求=%u/%u 字节",
                 (unsigned long)runtime_internal_dma_free,
                 (unsigned long)runtime_internal_dma_largest,
                 (unsigned)LEGBOT_RUNTIME_INTERNAL_DMA_FLOOR_BYTES,
                 (unsigned)LEGBOT_RUNTIME_INTERNAL_DMA_LARGEST_BLOCK_FLOOR_BYTES);
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        (void)legbot_services_stop_all();
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG,
             "服务运行态内部 DMA 余量已通过：空闲/最大块=%lu/%lu 字节",
             (unsigned long)runtime_internal_dma_free,
             (unsigned long)runtime_internal_dma_largest);

    if (s_external_task_stacks[LEGBOT_SERVICE_VOICE] == NULL)
    {
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        return ESP_OK;
    }

    /* 当前仍运行在内部 main task 栈，只有这里允许 mmap model 分区。 */
    err = voice_service_prepare_model_mapping();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "语音模型分区映射失败，现有 UI/BLE 保持运行，错误=%s",
                 esp_err_to_name(err));
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        return ESP_OK;
    }

    const legbot_service_descriptor_t *voice_descriptor =
        legbot_service_descriptor(LEGBOT_SERVICE_VOICE);
    if (voice_descriptor == NULL || !voice_descriptor->starts_by_default)
    {
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        (void)legbot_services_stop_all();
        return ESP_ERR_INVALID_STATE;
    }
    err = start_service(LEGBOT_SERVICE_VOICE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "语音服务任务创建失败，现有 UI/BLE 保持运行，错误=%s",
                 esp_err_to_name(err));
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        return ESP_OK;
    }
    atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_VOICE);
    err = wait_for_service_runtime_ready(LEGBOT_SERVICE_VOICE);
    if (err != ESP_OK || !voice_service_is_ready())
    {
        if (!voice_service_startup_complete())
        {
            const esp_err_t disable_error = voice_service_disable(0U);
            if (disable_error != ESP_OK && disable_error != ESP_ERR_TIMEOUT)
            {
                ESP_LOGE(TAG,
                         "语音初始化超时后的禁用请求失败，错误=%s",
                         esp_err_to_name(disable_error));
            }
        }
        ESP_LOGE(TAG,
                 "语音模型未就绪，现有 UI/BLE 保持运行，发布验收必须判定失败，错误=%s",
                 esp_err_to_name(err));
        atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
        return ESP_OK;
    }

    const size_t post_voice_dma_free =
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    const size_t post_voice_dma_largest =
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    const size_t post_voice_psram_free =
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const size_t post_voice_psram_largest =
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    if (post_voice_dma_free < LEGBOT_RUNTIME_INTERNAL_DMA_FLOOR_BYTES ||
        post_voice_dma_largest <
            LEGBOT_RUNTIME_INTERNAL_DMA_LARGEST_BLOCK_FLOOR_BYTES ||
        post_voice_psram_free < LEGBOT_RUNTIME_PSRAM_FLOOR_BYTES ||
        post_voice_psram_largest <
            LEGBOT_RUNTIME_PSRAM_LARGEST_BLOCK_FLOOR_BYTES)
    {
        ESP_LOGE(TAG,
                 "语音加载后资源门槛失败：DMA=%lu/%lu，PSRAM=%lu/%lu；禁用语音并保留 UI/BLE",
                 (unsigned long)post_voice_dma_free,
                 (unsigned long)post_voice_dma_largest,
                 (unsigned long)post_voice_psram_free,
                 (unsigned long)post_voice_psram_largest);
        const esp_err_t disable_error = voice_service_disable(
            pdMS_TO_TICKS(LEGBOT_SERVICE_START_PHASE_TIMEOUT_MS));
        if (disable_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "语音资源门槛失败后的禁用清理未确认，错误=%s",
                     esp_err_to_name(disable_error));
        }
    }
    else
    {
        ESP_LOGI(TAG,
                 "语音加载后资源门槛通过：DMA=%lu/%lu，PSRAM=%lu/%lu",
                 (unsigned long)post_voice_dma_free,
                 (unsigned long)post_voice_dma_largest,
                 (unsigned long)post_voice_psram_free,
                 (unsigned long)post_voice_psram_largest);
    }

    atomic_store(&s_start_phase, LEGBOT_SERVICE_START_PHASE_ALL);
    return ESP_OK;
}

esp_err_t legbot_services_stop_all(void)
{
    legbot_service_message_t stop = {
        .type = LEGBOT_SERVICE_STOP_MESSAGE,
        .value = 0,
    };

    esp_err_t first_error = ESP_OK;
    /* voice owner 先终止会话并释放模型，之后才允许 audio owner 退出。 */
    if (atomic_load(&s_task_running[LEGBOT_SERVICE_VOICE]))
    {
        if (voice_service_request_stop() != ESP_OK)
        {
            first_error = ESP_ERR_TIMEOUT;
        }
        const TickType_t voice_started_at = xTaskGetTickCount();
        while (atomic_load(&s_task_running[LEGBOT_SERVICE_VOICE]) &&
               xTaskGetTickCount() - voice_started_at <
                   pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_TIMEOUT_MS))
        {
            vTaskDelay(1U);
        }
        if (atomic_load(&s_task_running[LEGBOT_SERVICE_VOICE]))
        {
            return ESP_ERR_TIMEOUT;
        }
    }
    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        if (id == LEGBOT_SERVICE_STATE || id == LEGBOT_SERVICE_VOICE ||
            !atomic_load(&s_task_running[id]))
        {
            continue;
        }
        if (id == LEGBOT_SERVICE_BLE)
        {
            if (ble_service_request_stop(
                    pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS)) != ESP_OK &&
                first_error == ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
#if LEGBOT_CAP_MODEM
        if (id == LEGBOT_SERVICE_MODEM)
        {
            if (modem_service_request_stop(0) != ESP_OK && first_error == ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
#endif
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        if (id == LEGBOT_SERVICE_GPS)
        {
            if (gps_service_request_stop(
                    pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_ENQUEUE_TIMEOUT_MS)) != ESP_OK &&
                first_error == ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
#endif
#if LEGBOT_CAP_CLOUD
        if (id == LEGBOT_SERVICE_CLOUD)
        {
            if (cloud_service_request_stop(0) != ESP_OK &&
                first_error == ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
#endif
        if (id == LEGBOT_SERVICE_AUDIO)
        {
            if (audio_service_request_stop(0) != ESP_OK && first_error == ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
        if (id == LEGBOT_SERVICE_SELFTEST)
        {
            if (selftest_service_request_stop(0) != ESP_OK && first_error == ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
        if (id == LEGBOT_SERVICE_UI)
        {
            const ui_service_request_t ui_stop = {
                .type = UI_SERVICE_REQUEST_STOP,
                .value = 0,
            };
            if (ui_service_post_request(&ui_stop, 0) != ESP_OK)
            {
                first_error = ESP_ERR_TIMEOUT;
            }
            continue;
        }
        if (s_queues[id] == NULL || xQueueSend(s_queues[id], &stop, 0) != pdTRUE)
        {
            first_error = ESP_ERR_TIMEOUT;
        }
    }

    TickType_t started_at = xTaskGetTickCount();
    for (;;)
    {
        bool any_producer_running = false;
        for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
        {
            if (id != LEGBOT_SERVICE_STATE)
            {
                any_producer_running = any_producer_running ||
                                       atomic_load(&s_task_running[id]);
            }
        }
        if (!any_producer_running)
        {
            break;
        }
        if (xTaskGetTickCount() - started_at >= pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_TIMEOUT_MS))
        {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    /* state_task 最后停止，确保 audio/power 的 terminal 更新已被消费。 */
    if (atomic_load(&s_task_running[LEGBOT_SERVICE_STATE]) &&
        state_service_request_stop(pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS)) != ESP_OK &&
        first_error == ESP_OK)
    {
        first_error = ESP_ERR_TIMEOUT;
    }
    for (;;)
    {
        bool any_running = false;
        for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
        {
            any_running = any_running || atomic_load(&s_task_running[id]);
        }
        if (!any_running)
        {
            break;
        }
        if (xTaskGetTickCount() - started_at >= pdMS_TO_TICKS(LEGBOT_SERVICE_STOP_TIMEOUT_MS))
        {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        esp_err_t exit_error = (esp_err_t)atomic_load(&s_task_exit_errors[id]);
        if (exit_error != ESP_OK && first_error == ESP_OK)
        {
            first_error = exit_error;
        }
        atomic_store(&s_task_exit_errors[id], ESP_OK);
    }
    return first_error;
}

esp_err_t legbot_selftest_service_start(void)
{
    return legbot_selftest_service_start_with_gps_context(false);
}

esp_err_t legbot_selftest_service_start_with_gps_context(bool indoor_confirmed)
{
    esp_err_t err = legbot_services_init_contracts();
    if (err != ESP_OK)
    {
        return err;
    }
    err = start_service(LEGBOT_SERVICE_SELFTEST);
    if (err != ESP_OK)
    {
        return err;
    }
    return selftest_service_request_run_with_gps_context(
        indoor_confirmed
            ? SELFTEST_GPS_FIX_CONTEXT_INDOOR_CONFIRMED
            : SELFTEST_GPS_FIX_CONTEXT_DEFAULT,
        0);
}

static void cleanup_service_contracts(void)
{
    (void)config_service_set_change_callback(NULL, NULL);
    ble_service_cancel_prepared_run();
#if LEGBOT_CAP_CLOUD
    cloud_service_cancel_prepared_run();
#endif
#if LEGBOT_CAP_MODEM
    modem_service_cancel_prepared_run();
#endif
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    gps_service_cancel_prepared_run();
    (void)gps_service_deinit_contracts();
#endif
    voice_service_cancel_prepared_run();
    (void)audio_service_deinit_contracts();
    (void)selftest_service_deinit_contracts();
    /* 初始化失败路径必须回收已创建队列，避免服务框架处于半初始化状态。 */
    for (legbot_service_id_t id = 0; id < LEGBOT_SERVICE_COUNT; ++id)
    {
        if (s_queues[id] != NULL)
        {
            if (service_queue_uses_external_storage(id))
            {
                vQueueDeleteWithCaps(s_queues[id]);
            }
            else
            {
                vQueueDelete(s_queues[id]);
            }
            s_queues[id] = NULL;
        }
    }
    if (s_service_event_group != NULL)
    {
        vEventGroupDelete(s_service_event_group);
        s_service_event_group = NULL;
    }
    s_contracts_initialized = false;
}

static bool valid_service_id(legbot_service_id_t id)
{
    return id >= 0 && id < LEGBOT_SERVICE_COUNT;
}

static bool service_allowed_by_profile(legbot_service_id_t id)
{
    if (id == LEGBOT_SERVICE_MODEM)
    {
        return LEGBOT_CAP_MODEM == 1;
    }
    if (id == LEGBOT_SERVICE_GPS)
    {
        return LEGBOT_CAP_GPS_TIME == 1 ||
               LEGBOT_CAP_GPS_CONTINUOUS_LOCATION == 1;
    }
    if (id == LEGBOT_SERVICE_CLOUD)
    {
        return LEGBOT_CAP_CLOUD == 1;
    }
    return true;
}

static bool service_uses_external_stack(legbot_service_id_t id)
{
    return service_allowed_by_profile(id) &&
           (id == LEGBOT_SERVICE_MODEM || id == LEGBOT_SERVICE_GPS ||
            id == LEGBOT_SERVICE_CLOUD || id == LEGBOT_SERVICE_LOG ||
            id == LEGBOT_SERVICE_VOICE);
}

static bool service_queue_uses_external_storage(legbot_service_id_t id)
{
    /* power queue 由 PWR/QMI ISR 写入，必须在 cache 关闭时仍可访问。 */
    return service_allowed_by_profile(id) && id != LEGBOT_SERVICE_POWER;
}

static esp_err_t ensure_service_queue(legbot_service_id_t id)
{
    if (!valid_service_id(id))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!service_allowed_by_profile(id))
    {
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (id == LEGBOT_SERVICE_AUDIO)
    {
        return audio_service_init_contracts();
    }
    if (id == LEGBOT_SERVICE_VOICE)
    {
        return ESP_OK;
    }
    if (id == LEGBOT_SERVICE_SELFTEST)
    {
        return selftest_service_init_contracts();
    }
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    if (id == LEGBOT_SERVICE_GPS)
    {
        return gps_service_init_contracts();
    }
#endif
    if (s_queues[id] != NULL)
    {
        return ESP_OK;
    }

    size_t item_size = id == LEGBOT_SERVICE_STATE ? sizeof(state_service_update_t)
                                                  : sizeof(legbot_service_message_t);
    if (service_queue_uses_external_storage(id))
    {
        s_queues[id] = xQueueCreateWithCaps(
            LEGBOT_SERVICE_QUEUE_DEPTH,
            item_size,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    else
    {
        s_queues[id] = xQueueCreate(LEGBOT_SERVICE_QUEUE_DEPTH, item_size);
    }
    return s_queues[id] == NULL ? ESP_ERR_NO_MEM : ESP_OK;
}

static esp_err_t wait_for_service_runtime_ready(legbot_service_id_t id)
{
    const TickType_t started_at = xTaskGetTickCount();
    for (;;)
    {
        const bool ready =
            id == LEGBOT_SERVICE_BLE
                ? ble_service_startup_ready()
                : (id == LEGBOT_SERVICE_AUDIO &&
                       audio_service_startup_complete()) ||
                      (id == LEGBOT_SERVICE_VOICE &&
                       voice_service_startup_complete());
        if (ready)
        {
            return ESP_OK;
        }
        if (!valid_service_id(id) || !atomic_load(&s_task_running[id]))
        {
            return ESP_ERR_INVALID_STATE;
        }
        const uint32_t timeout_ms =
            id == LEGBOT_SERVICE_BLE
                ? LEGBOT_BLE_SERVICE_START_TIMEOUT_MS
                : id == LEGBOT_SERVICE_VOICE
                ? LEGBOT_VOICE_SERVICE_START_TIMEOUT_MS
                : LEGBOT_SERVICE_START_PHASE_TIMEOUT_MS;
        if (xTaskGetTickCount() - started_at >= pdMS_TO_TICKS(timeout_ms))
        {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(1);
    }
}

static void service_stub_task(void *arg)
{
    legbot_service_id_t id = (legbot_service_id_t)(intptr_t)arg;
    legbot_service_message_t message;
    esp_err_t task_error = ESP_OK;

    if (id != LEGBOT_SERVICE_STATE)
    {
        const legbot_service_start_phase_t required_phase =
            id == LEGBOT_SERVICE_BLE
                ? LEGBOT_SERVICE_START_PHASE_BLE
                : (id == LEGBOT_SERVICE_AUDIO
                       ? LEGBOT_SERVICE_START_PHASE_AUDIO
                       : (id == LEGBOT_SERVICE_VOICE
                              ? LEGBOT_SERVICE_START_PHASE_VOICE
                              : LEGBOT_SERVICE_START_PHASE_ALL));
        while ((legbot_service_start_phase_t)atomic_load(&s_start_phase) <
               required_phase)
        {
            vTaskDelay(1);
        }
    }

    if (id == LEGBOT_SERVICE_UI)
    {
        ui_service_run();
    }
    else if (id == LEGBOT_SERVICE_BLE)
    {
        task_error = ble_service_run();
    }
#if LEGBOT_CAP_MODEM
    else if (id == LEGBOT_SERVICE_MODEM)
    {
        task_error = modem_service_run();
    }
#endif
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    else if (id == LEGBOT_SERVICE_GPS)
    {
        task_error = gps_service_run();
    }
#endif
#if LEGBOT_CAP_CLOUD
    else if (id == LEGBOT_SERVICE_CLOUD)
    {
        task_error = cloud_service_run();
    }
#endif
    else if (id == LEGBOT_SERVICE_POWER)
    {
        power_service_run();
    }
    else if (id == LEGBOT_SERVICE_STATE)
    {
        state_service_run();
    }
    else if (id == LEGBOT_SERVICE_AUDIO)
    {
        task_error = audio_service_run();
    }
    else if (id == LEGBOT_SERVICE_VOICE)
    {
        task_error = voice_service_run();
    }
    else if (id == LEGBOT_SERVICE_SELFTEST)
    {
        task_error = selftest_service_run();
    }
    else
    {
        for (;;)
        {
            if (xQueueReceive(s_queues[id], &message, portMAX_DELAY) != pdTRUE)
            {
                continue;
            }
            if (message.type == LEGBOT_SERVICE_STOP_MESSAGE)
            {
                break;
            }
        }
    }

    /* 任务退出前清空句柄，避免后续按需启动看到旧句柄。 */
    atomic_store(&s_task_exit_errors[id], task_error);
    s_tasks[id] = NULL;
    atomic_store(&s_task_running[id], false);
    vTaskDelete(NULL);
}

static esp_err_t start_service(legbot_service_id_t id)
{
    if (!valid_service_id(id))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!service_allowed_by_profile(id))
    {
        ESP_LOGW(TAG,
                 "固件编译模式拒绝启动服务：服务=%s",
                 s_descriptors[id].task_name);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (atomic_load(&s_task_running[id]))
    {
        return ESP_OK;
    }

    const legbot_service_descriptor_t *descriptor = legbot_service_descriptor(id);
    uint32_t stack_bytes = LEGBOT_SERVICE_STACK_BYTES;
    if (id == LEGBOT_SERVICE_UI)
    {
        stack_bytes = UI_SERVICE_TASK_STACK_BYTES;
    }
    else if (id == LEGBOT_SERVICE_BLE)
    {
        stack_bytes = BLE_SERVICE_TASK_STACK_BYTES;
    }
#if LEGBOT_CAP_MODEM
    else if (id == LEGBOT_SERVICE_MODEM)
    {
        stack_bytes = LEGBOT_MODEM_SERVICE_STACK_BYTES;
    }
#endif
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    else if (id == LEGBOT_SERVICE_GPS)
    {
        stack_bytes = GPS_SERVICE_TASK_STACK_BYTES;
    }
#endif
#if LEGBOT_CAP_CLOUD
    else if (id == LEGBOT_SERVICE_CLOUD)
    {
        stack_bytes = LEGBOT_CLOUD_SERVICE_STACK_BYTES;
    }
#endif
    else if (id == LEGBOT_SERVICE_STATE)
    {
        stack_bytes = LEGBOT_STATE_SERVICE_STACK_BYTES;
    }
    else if (id == LEGBOT_SERVICE_SELFTEST)
    {
        stack_bytes = LEGBOT_SELFTEST_SERVICE_STACK_BYTES;
    }
    else if (id == LEGBOT_SERVICE_VOICE)
    {
        stack_bytes = VOICE_SERVICE_TASK_STACK_BYTES;
    }
    if (id == LEGBOT_SERVICE_BLE)
    {
        esp_err_t prepare_error = ble_service_prepare_run();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
#if LEGBOT_CAP_MODEM
    else if (id == LEGBOT_SERVICE_MODEM)
    {
        esp_err_t prepare_error = modem_service_prepare();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
#endif
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    else if (id == LEGBOT_SERVICE_GPS)
    {
        esp_err_t prepare_error = gps_service_prepare();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
#endif
#if LEGBOT_CAP_CLOUD
    else if (id == LEGBOT_SERVICE_CLOUD)
    {
        esp_err_t prepare_error = cloud_service_prepare();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
#endif
    else if (id == LEGBOT_SERVICE_AUDIO)
    {
        esp_err_t prepare_error = audio_service_prepare_run();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
    else if (id == LEGBOT_SERVICE_VOICE)
    {
        esp_err_t prepare_error = voice_service_prepare_run();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
    else if (id == LEGBOT_SERVICE_SELFTEST)
    {
        esp_err_t prepare_error = selftest_service_prepare_run();
        if (prepare_error != ESP_OK)
        {
            return prepare_error;
        }
    }
    atomic_store(&s_task_exit_errors[id], ESP_OK);
    atomic_store(&s_task_running[id], true);
    BaseType_t task_result = pdFAIL;
    if (id == LEGBOT_SERVICE_SELFTEST)
    {
        s_tasks[id] = xTaskCreateStatic(service_stub_task,
                                        descriptor->task_name,
                                        stack_bytes,
                                        (void *)(intptr_t)id,
                                        tskIDLE_PRIORITY + 1,
                                        s_selftest_task_stack,
                                        &s_selftest_task_buffer);
        task_result = s_tasks[id] != NULL ? pdPASS : pdFAIL;
    }
    else
    {
        if (id == LEGBOT_SERVICE_UI)
        {
            /* UI 会同步读写 NVS，必须使用默认的内部 RAM 动态栈。 */
            task_result = xTaskCreatePinnedToCore(
                service_stub_task,
                descriptor->task_name,
                stack_bytes,
                (void *)(intptr_t)id,
                LEGBOT_UI_SERVICE_PRIORITY,
                &s_tasks[id],
                LEGBOT_UI_SERVICE_CORE);
        }
        else if (service_uses_external_stack(id))
        {
            if (s_external_task_stacks[id] == NULL)
            {
                const uint32_t stack_caps =
                    id == LEGBOT_SERVICE_VOICE
                        ? LEGBOT_VOICE_TASK_STACK_CAPS
                        : (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                s_external_task_stacks[id] =
                    heap_caps_malloc(stack_bytes,
                                     stack_caps);
            }
            if (s_external_task_stacks[id] != NULL)
            {
                s_tasks[id] = xTaskCreateStatic(
                    service_stub_task,
                    descriptor->task_name,
                    stack_bytes,
                    (void *)(intptr_t)id,
                    tskIDLE_PRIORITY + 1,
                    s_external_task_stacks[id],
                    &s_external_task_buffers[id]);
                task_result = s_tasks[id] != NULL ? pdPASS : pdFAIL;
            }
        }
        else
        {
            task_result = xTaskCreate(service_stub_task,
                                      descriptor->task_name,
                                      stack_bytes,
                                      (void *)(intptr_t)id,
                                      tskIDLE_PRIORITY + 1,
                                      &s_tasks[id]);
        }
    }
    if (task_result != pdPASS)
    {
        ESP_LOGE(TAG,
                 "服务任务创建失败，service=%s，栈=%lu 字节，静态栈=%s",
                 descriptor->task_name,
                 (unsigned long)stack_bytes,
                 (id == LEGBOT_SERVICE_SELFTEST ||
                  service_uses_external_stack(id))
                     ? "是"
                     : "否");
        if (id == LEGBOT_SERVICE_BLE)
        {
            ble_service_cancel_prepared_run();
        }
#if LEGBOT_CAP_MODEM
        else if (id == LEGBOT_SERVICE_MODEM)
        {
            modem_service_cancel_prepared_run();
        }
#endif
#if LEGBOT_CAP_GPS_TIME || LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        else if (id == LEGBOT_SERVICE_GPS)
        {
            gps_service_cancel_prepared_run();
        }
#endif
#if LEGBOT_CAP_CLOUD
        else if (id == LEGBOT_SERVICE_CLOUD)
        {
            cloud_service_cancel_prepared_run();
        }
#endif
        else if (id == LEGBOT_SERVICE_AUDIO)
        {
            audio_service_cancel_prepared_run();
        }
        else if (id == LEGBOT_SERVICE_VOICE)
        {
            voice_service_cancel_prepared_run();
        }
        else if (id == LEGBOT_SERVICE_SELFTEST)
        {
            selftest_service_cancel_prepared_run();
        }
        s_tasks[id] = NULL;
        atomic_store(&s_task_running[id], false);
        return ESP_ERR_NO_MEM;
    }
    if (id == LEGBOT_SERVICE_UI)
    {
        ESP_LOGI(TAG,
                 "UI 服务任务已固定至 Core %d，优先级=%u",
                 LEGBOT_UI_SERVICE_CORE,
                 (unsigned)LEGBOT_UI_SERVICE_PRIORITY);
    }
    return ESP_OK;
}

static esp_err_t publish_initial_config_truth(void)
{
    config_service_snapshot_t config = {0};
    esp_err_t err = config_service_snapshot(&config,
                                            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        return err;
    }
    watch_config_update_t update = {0};
    err = publish_config_snapshot(&config, &update);
    if (err != ESP_OK)
    {
        return err;
    }

    const TickType_t started_at = xTaskGetTickCount();
    do
    {
        memset(&s_initial_config_applied_snapshot,
               0,
               sizeof(s_initial_config_applied_snapshot));
        err = watch_state_snapshot(&s_initial_config_applied_snapshot,
                                   pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
        if (err == ESP_OK &&
            s_initial_config_applied_snapshot.config_revision == update.revision &&
            memcmp(s_initial_config_applied_snapshot.watch_id,
                   update.watch_id,
                   sizeof(s_initial_config_applied_snapshot.watch_id)) == 0 &&
            strcmp(s_initial_config_applied_snapshot.cloud_error_code,
                   update.cloud_error_code) == 0)
        {
            memset(&s_initial_config_applied_snapshot,
                   0,
                   sizeof(s_initial_config_applied_snapshot));
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    } while (xTaskGetTickCount() - started_at <
             pdMS_TO_TICKS(STATE_SERVICE_APPLY_ACK_TIMEOUT_MS));
    memset(&s_initial_config_applied_snapshot,
           0,
           sizeof(s_initial_config_applied_snapshot));
    return ESP_ERR_TIMEOUT;
}

static esp_err_t publish_config_snapshot(const config_service_snapshot_t *config,
                                         watch_config_update_t *published_update)
{
    if (config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const bool invalid = config->cloud_base_url_status == CONFIG_SERVICE_VALUE_INVALID ||
                         config->apn_mode == CONFIG_SERVICE_APN_INVALID ||
                         config->token_status == CONFIG_SERVICE_VALUE_INVALID ||
                         config->certificate_status == CONFIG_SERVICE_VALUE_INVALID ||
                         config->schema_status == CONFIG_SERVICE_SCHEMA_CORRUPT;
    watch_config_update_t update = {
        .revision = config->config_revision,
        .cloud_configured = config->cloud_configured,
        .cloud_status = config->cloud_configured
                            ? WATCH_CLOUD_CONFIGURED
                            : (invalid ? WATCH_CLOUD_INVALID : WATCH_CLOUD_UNCONFIGURED),
        .base_url_status = (watch_config_value_status_t)config->cloud_base_url_status,
        .apn_mode = (watch_apn_mode_t)config->apn_mode,
        .token_status = (watch_config_value_status_t)config->token_status,
        .certificate_status = (watch_config_value_status_t)config->certificate_status,
        .schema_status = (watch_config_schema_status_t)config->schema_status,
    };
    memcpy(update.watch_id, config->watch_id, sizeof(update.watch_id));
    const char *error_code = config_service_cloud_error_code(config->cloud_error);
    const size_t error_length = strnlen(error_code, sizeof(update.cloud_error_code));
    if (error_length == 0U || error_length >= sizeof(update.cloud_error_code))
    {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(update.cloud_error_code, error_code, error_length + 1U);
    const esp_err_t err = state_service_publish_config(&update, 0);
    if (err != ESP_OK)
    {
        return err;
    }
    if (published_update != NULL)
    {
        *published_update = update;
    }
    return ESP_OK;
}

static esp_err_t publish_config_change(const config_service_snapshot_t *config,
                                       void *context)
{
    (void)context;
    return publish_config_snapshot(config, NULL);
}
