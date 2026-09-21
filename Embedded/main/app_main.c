/**
 * @file     app_main.c
 * @brief    legbot_watch 固件主入口
 * @details  负责启动诊断、NVS 初始化和 BSP、平台、服务初始化阶段调度。
 * @author   ZHC
 * @date     2026-08-03
 */

#include <inttypes.h>
#include <stdio.h>

#include "app_state.h"
#include "audio_service.h"
#include "at_core.h"
#include "ble_service.h"
#include "bsp_board.h"
#include "cloud_provision.h"
#include "config_service.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_psram.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "event_bus.h"
#include "i2c_manager.h"
#include "legbot_services.h"
#include "l76kb_a58_bsp.h"
#include "maintenance_service.h"
#include "ml307r_bsp.h"
#include "nvs_flash.h"
#include "qmi8658c_bsp.h"
#include "sdkconfig.h"
#include "state_service.h"
#include "ui_service.h"

#if defined(CONFIG_UNITY_ENABLE_IDF_TEST_RUNNER) && \
    CONFIG_UNITY_ENABLE_IDF_TEST_RUNNER
#include "unity.h"
#endif

#if !CONFIG_IDF_TARGET_ESP32S3
#error "legbot_watch firmware is pinned to esp32s3 for ESPWatch-S3-4G."
#endif
#if !defined(CONFIG_PM_ENABLE) || !CONFIG_PM_ENABLE
#error "legbot_watch requires CONFIG_PM_ENABLE=y."
#endif
#if !defined(CONFIG_FREERTOS_USE_TICKLESS_IDLE) || \
    !CONFIG_FREERTOS_USE_TICKLESS_IDLE
#error "legbot_watch requires CONFIG_FREERTOS_USE_TICKLESS_IDLE=y."
#endif
#if !defined(CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP) || \
    CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP != 3
#error "legbot_watch requires a three-tick automatic light sleep threshold."
#endif
#if !defined(CONFIG_BT_CTRL_MODEM_SLEEP_MODE_1) || \
    !CONFIG_BT_CTRL_MODEM_SLEEP_MODE_1
#error "legbot_watch requires Bluetooth modem sleep mode 1."
#endif
#if !defined(CONFIG_BT_CTRL_LPCLK_SEL_MAIN_XTAL) || \
    !CONFIG_BT_CTRL_LPCLK_SEL_MAIN_XTAL
#error "legbot_watch requires the Bluetooth main crystal low-power clock."
#endif
#if !defined(CONFIG_BT_CTRL_MAIN_XTAL_PU_DURING_LIGHT_SLEEP) || \
    !CONFIG_BT_CTRL_MAIN_XTAL_PU_DURING_LIGHT_SLEEP
#error "legbot_watch must keep the main crystal powered during light sleep."
#endif
#if !defined(CONFIG_RTC_CLK_SRC_INT_RC) || !CONFIG_RTC_CLK_SRC_INT_RC
#error "legbot_watch requires the board-proven internal RC RTC clock."
#endif
#if !defined(CONFIG_ESP_PHY_MAC_BB_PD) || !CONFIG_ESP_PHY_MAC_BB_PD
#error "legbot_watch requires Wi-Fi/Bluetooth MAC and baseband power-down."
#endif
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED) && \
    CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED && \
    (!defined(CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION) || \
     !CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION)
#error "legbot_watch requires USB Serial/JTAG monitoring protection during automatic light sleep."
#endif

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "MAIN";

/** board stage 记录的音频子资源降级状态。 */
static bool s_audio_board_degraded;
/** board stage 记录的音频子资源原始错误。 */
static esp_err_t s_audio_board_cause;
/** 启动期间阻止硬件初始化延时误入自动 light sleep 的 PM 锁。 */
static esp_pm_lock_handle_t s_startup_light_sleep_lock;
/** 启动期 PM 锁当前是否仍被持有。 */
static bool s_startup_light_sleep_lock_held;

static void print_flash_psram_info(void);
static void print_chip_info(void);
static void log_heap_allocation_failure(size_t size,
                                        uint32_t caps,
                                        const char *function_name);
static esp_err_t init_power_management(void);
static esp_err_t release_startup_light_sleep_lock(void);
static esp_err_t abort_startup_power_management(void);
static esp_err_t init_log_nvs_config(void);
static esp_err_t run_stage(legbot_bsp_init_stage_t stage, esp_err_t (*fn)(void));
static esp_err_t init_board_stage(void);
static esp_err_t init_event_state_stage(void);
static esp_err_t init_shared_buses_stage(void);
static esp_err_t init_ui_storage_stage(void);
static esp_err_t init_services_stage(void);
static esp_err_t init_selftest_hooks_stage(void);
#if CONFIG_LEGBOT_POWER_DIAGNOSTIC_BUILD
static void print_power_diagnostic_snapshot(void);
#endif

void app_main(void)
{
    /* 先于 Unity 与所有可失败初始化固定低电平，确保 GPS 上电后立即待机。 */
    const esp_err_t gps_standby_error = l76kb_a58_bsp_hold_standby();
    if (gps_standby_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "无法保持 GPS IO3 待机，停止启动，错误=%s",
                 esp_err_to_name(gps_standby_error));
        return;
    }
#if !LEGBOT_CAP_MODEM
    /* 先于 Unity 与所有可失败初始化固定低电平，确保 v1 不启动 ML307R。 */
    const esp_err_t modem_disable_error = ml307r_bsp_hold_disabled();
    if (modem_disable_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "v1 固件无法保持 ML307R 关闭，停止启动，错误=%s",
                 esp_err_to_name(modem_disable_error));
        return;
    }
#endif
#if defined(CONFIG_UNITY_ENABLE_IDF_TEST_RUNNER) && \
    CONFIG_UNITY_ENABLE_IDF_TEST_RUNNER
    /* 测试固件只进入 Unity 菜单，不初始化生产服务和持久化资源。 */
    unity_run_menu();
    return;
#endif

    const esp_err_t heap_hook_error =
        heap_caps_register_failed_alloc_callback(log_heap_allocation_failure);
    if (heap_hook_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "堆分配失败诊断回调注册失败，错误=%s",
                 esp_err_to_name(heap_hook_error));
    }

    const esp_err_t power_management_error = init_power_management();
    if (power_management_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "动态电源管理初始化失败，停止启动，错误=%s",
                 esp_err_to_name(power_management_error));
        return;
    }

    ESP_LOGI(TAG, "启动 legbot_watch 固件骨架");
    print_chip_info();
#if LEGBOT_CAP_MODEM
    ESP_LOGI(TAG,
             "应用日志使用原生 USB Serial/JTAG，GPIO43/GPIO44 仅保留给 ML307R UART2");
#else
    ESP_LOGI(TAG, "当前为蓝牙通信固件模式，4G 与 GPS 模块保持关闭");
#endif
    print_flash_psram_info();

    /* 初始化阶段按架构顺序串行执行，任何阶段失败都停止继续启动。 */
    if (run_stage(LEGBOT_BSP_STAGE_LOG_NVS_CONFIG, init_log_nvs_config) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_EVENT_STATE, init_event_state_stage) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_BOARD_PINS, init_board_stage) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_SHARED_BUSES, init_shared_buses_stage) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_UI_STORAGE, init_ui_storage_stage) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_SERVICES, init_services_stage) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_SELFTEST_HOOKS, init_selftest_hooks_stage) != ESP_OK)
    {
        goto startup_failed;
    }

    const esp_err_t light_sleep_error = release_startup_light_sleep_lock();
    if (light_sleep_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "自动 light sleep 启动锁释放失败，执行安全失败收尾，错误=%s",
                 esp_err_to_name(light_sleep_error));
        goto startup_failed;
    }

#if CONFIG_LEGBOT_POWER_DIAGNOSTIC_BUILD
    /* 专用测量构建只在启动锁释放后记录一次，不制造稳态周期唤醒。 */
    print_power_diagnostic_snapshot();
#endif
    ESP_LOGI(TAG, "固件骨架已就绪，未启用演示重启循环");
    return;

startup_failed:
    {
        const esp_err_t cleanup_error = abort_startup_power_management();
        if (cleanup_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "启动失败后的电源管理收尾失败，为避免不安全休眠已保留锁，错误=%s",
                     esp_err_to_name(cleanup_error));
        }
    }
}

static void log_heap_allocation_failure(size_t size,
                                        uint32_t caps,
                                        const char *function_name)
{
    ESP_EARLY_LOGE(TAG,
                   "堆内存申请失败：函数=%s，字节=%u，能力=0x%08" PRIx32,
                   function_name != NULL ? function_name : "unknown",
                   (unsigned)size,
                   caps);
}

static void print_flash_psram_info(void)
{
    uint32_t image_flash_size = 0;
    uint32_t physical_flash_size = 0;
    uint32_t flash_id = 0;

    if (esp_flash_read_id(NULL, &flash_id) == ESP_OK)
    {
        ESP_LOGI(TAG, "Flash JEDEC ID：0x%06" PRIx32, flash_id);
    }
    else
    {
        ESP_LOGW(TAG, "读取 Flash JEDEC ID 失败");
    }

    if (esp_flash_get_size(NULL, &image_flash_size) == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "镜像头 Flash 大小：%" PRIu32 " 字节（%" PRIu32 " MB）",
                 image_flash_size,
                 image_flash_size / (1024 * 1024));
    }
    else
    {
        ESP_LOGW(TAG, "读取镜像头 Flash 大小失败");
    }

    if (esp_flash_get_physical_size(NULL, &physical_flash_size) == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "物理 Flash 大小：%" PRIu32 " 字节（%" PRIu32 " MB）",
                 physical_flash_size,
                 physical_flash_size / (1024 * 1024));
    }
    else
    {
        ESP_LOGW(TAG, "读取物理 Flash 大小失败");
    }

    if (image_flash_size != 0 && physical_flash_size != 0 && image_flash_size != physical_flash_size)
    {
        ESP_LOGW(TAG,
                 "Flash 大小不一致：镜像头=%" PRIu32 " MB，物理=%" PRIu32 " MB",
                 image_flash_size / (1024 * 1024),
                 physical_flash_size / (1024 * 1024));
    }

    if (esp_psram_is_initialized())
    {
        size_t psram_size = esp_psram_get_size();
        size_t psram_heap_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        size_t psram_heap_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

        ESP_LOGI(TAG,
                 "PSRAM 大小：%u 字节（%u MB）",
                 (unsigned)psram_size,
                 (unsigned)(psram_size / (1024 * 1024)));
        ESP_LOGI(TAG,
                 "PSRAM 堆总量/空闲：%u/%u 字节",
                 (unsigned)psram_heap_total,
                 (unsigned)psram_heap_free);
    }
    else
    {
        ESP_LOGW(TAG, "PSRAM 未初始化；如果该板有 PSRAM，请启用 CONFIG_SPIRAM");
    }
}

static void print_chip_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG,
             "目标芯片 %s，CPU 核心数=%d，硅片版本 v%u.%u",
             CONFIG_IDF_TARGET,
             chip_info.cores,
             chip_info.revision / 100,
             chip_info.revision % 100);
    ESP_LOGI(TAG, "最小剩余堆内存：%" PRIu32 " 字节", esp_get_minimum_free_heap_size());
}

static esp_err_t init_power_management(void)
{
    esp_err_t error = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,
                                         0,
                                         "startup_hw",
                                         &s_startup_light_sleep_lock);
    if (error != ESP_OK)
    {
        return error;
    }
    error = esp_pm_lock_acquire(s_startup_light_sleep_lock);
    if (error != ESP_OK)
    {
        (void)esp_pm_lock_delete(s_startup_light_sleep_lock);
        s_startup_light_sleep_lock = NULL;
        return error;
    }
    s_startup_light_sleep_lock_held = true;

    /* 先预留 CPU/Cache 保持内存；启动锁保证硬件与唤醒链就绪前不会真正休眠。 */
    ESP_LOGI(TAG,
             "CPU 保持区申请前：RETENTION 空闲/最大块=%u/%u，内部 DMA 空闲/最大块=%u/%u 字节",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_RETENTION),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_RETENTION),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    const esp_pm_config_t config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 40,
        .light_sleep_enable = true,
    };
    error = esp_pm_configure(&config);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "动态调频与自动 light sleep 已预配置：CPU=40~240 MHz，启动 PM 锁正保护硬件初始化；RETENTION 空闲/最大块=%u/%u 字节",
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_RETENTION),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_RETENTION));
    }
    else
    {
        const esp_err_t cleanup_error = abort_startup_power_management();
        if (cleanup_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "动态电源管理配置失败后收尾失败，错误=%s",
                     esp_err_to_name(cleanup_error));
        }
    }
    return error;
}

static esp_err_t release_startup_light_sleep_lock(void)
{
    if (s_startup_light_sleep_lock == NULL || !s_startup_light_sleep_lock_held)
    {
        return ESP_ERR_INVALID_STATE;
    }
    /* IO46、IO0、IO39、IO41 已由各 BSP 配置电平，此时再统一开放 GPIO 唤醒并进入自动休眠。 */
    esp_err_t error = esp_sleep_enable_gpio_wakeup();
    if (error != ESP_OK)
    {
        return error;
    }
    error = esp_pm_lock_release(s_startup_light_sleep_lock);
    if (error != ESP_OK)
    {
        return error;
    }
    s_startup_light_sleep_lock_held = false;
    error = esp_pm_lock_delete(s_startup_light_sleep_lock);
    if (error != ESP_OK)
    {
        return error;
    }
    s_startup_light_sleep_lock = NULL;
    ESP_LOGI(TAG,
             "自动 light sleep 启动锁已在硬件初始化后释放：空闲阈值=3 tick，唤醒=IO46/IO0/IO39/IO41；USB 主机连接期间保持调试可用");
    return ESP_OK;
}

static esp_err_t abort_startup_power_management(void)
{
    const esp_pm_config_t safe_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 40,
        .light_sleep_enable = false,
    };
    esp_err_t error = esp_pm_configure(&safe_config);
    if (error != ESP_OK)
    {
        return error;
    }
    if (s_startup_light_sleep_lock_held)
    {
        error = esp_pm_lock_release(s_startup_light_sleep_lock);
        if (error != ESP_OK)
        {
            return error;
        }
        s_startup_light_sleep_lock_held = false;
    }
    if (s_startup_light_sleep_lock != NULL)
    {
        error = esp_pm_lock_delete(s_startup_light_sleep_lock);
        if (error != ESP_OK)
        {
            return error;
        }
        s_startup_light_sleep_lock = NULL;
    }
    ESP_LOGW(TAG,
             "启动失败收尾已关闭 automatic light sleep 并回收启动 PM 锁");
    return ESP_OK;
}

static esp_err_t init_log_nvs_config(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS 初始化前需要擦除：%s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err != ESP_OK)
        {
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        return err;
    }
    err = config_service_init();
    if (err != ESP_OK)
    {
        return err;
    }
#if SCENIC_AREA_MANAGEMENT_DEBUG
    return cloud_provision_apply();
#else
    ESP_LOGI(TAG, "蓝牙通信模式跳过 cloud 配置预置");
    return ESP_OK;
#endif
}

static esp_err_t run_stage(legbot_bsp_init_stage_t stage, esp_err_t (*fn)(void))
{
    ESP_LOGI(TAG, "初始化阶段：%s", legbot_bsp_stage_name(stage));
    esp_err_t err = fn();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "阶段 %s 初始化失败：稳定错误码=%s，ESP 错误=%s",
                 legbot_bsp_stage_name(stage),
                 legbot_bsp_error_code(stage, err),
                 esp_err_to_name(err));
    }
    return err;
}

static esp_err_t init_board_stage(void)
{
    legbot_board_init_result_t result = {0};
    esp_err_t err = legbot_board_init(&result);
    s_audio_board_degraded = result.audio_degraded;
    s_audio_board_cause = result.audio_cause;
    return err;
}

static esp_err_t init_event_state_stage(void)
{
    esp_err_t err = event_bus_init();
    if (err != ESP_OK)
    {
        return err;
    }
    return watch_state_init();
}

static esp_err_t init_shared_buses_stage(void)
{
    esp_err_t err = i2c_manager_init();
    if (err != ESP_OK)
    {
        return err;
    }

    legbot_bsp_i2c_access_t i2c_access = {0};
    err = i2c_manager_get_access(&i2c_access);
    if (err != ESP_OK)
    {
        return err;
    }
    err = qmi8658c_bsp_init(&i2c_access);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "QMI8658C 不可用，六轴能力降级，稳定错误码=%s，错误=0x%x",
                 qmi8658c_bsp_error_code(qmi8658c_bsp_last_status()),
                 (unsigned)err);
    }
    /* CW2015 由 power_task 在服务启动后单独初始化并按周期重试。 */
    ESP_LOGI(TAG, "CW2015 初始化所有权已交给 power_task");
#if LEGBOT_CAP_MODEM
    return at_core_init();
#else
    ESP_LOGI(TAG, "蓝牙通信模式跳过 ML307R AT 核心初始化");
    return ESP_OK;
#endif
}

static esp_err_t init_ui_storage_stage(void)
{
    esp_err_t err = maintenance_service_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "维护契约初始化失败，稳定错误码=SVC_MAINT_INIT_FAILED，错误=0x%x",
                 (unsigned)err);
        return err;
    }
    err = ui_service_init();
    if (err != ESP_OK)
    {
        (void)maintenance_service_deinit();
        return err;
    }
    audio_service_result_t audio_result = AUDIO_RESULT_NOT_READY;
    err = audio_service_storage_init(&audio_result);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "storage/assets 音频资源降级，稳定错误码=%s，错误=0x%x",
                 audio_service_result_code(audio_result), (unsigned)err);
    }
    return ESP_OK;
}

static esp_err_t init_services_stage(void)
{
    esp_err_t err = legbot_services_start_all();
    if (err != ESP_OK)
    {
        return err;
    }
    if (s_audio_board_degraded)
    {
        const watch_audio_update_t update = {
            .resource_available = audio_service_storage_ready(),
            .state = WATCH_AUDIO_STATE_FAILED,
            .error = WATCH_AUDIO_ERROR_PA_FAILED,
            .updated_at_ticks = xTaskGetTickCount(),
        };
        err = state_service_publish_audio(&update, pdMS_TO_TICKS(20));
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "早期 PA 降级状态补发失败，原始错误=0x%x，发布错误=0x%x",
                     (unsigned)s_audio_board_cause, (unsigned)err);
        }
    }
    return ESP_OK;
}

static esp_err_t init_selftest_hooks_stage(void)
{
#if CONFIG_LEGBOT_AUDIO_BRINGUP_AUTOPLAY
    audio_service_result_t result = audio_service_request_play(
        AUDIO_RESOURCE_SELFTEST_OK,
        AUDIO_PRIORITY_SELFTEST,
        0);
    if (result != AUDIO_RESULT_OK)
    {
        ESP_LOGW(TAG, "音频 bring-up 自动播放请求失败，稳定错误码=%s",
                 audio_service_result_code(result));
    }
#endif
#if CONFIG_LEGBOT_SELFTEST_DEV_BENCH_AUTORUN
    esp_err_t selftest_error = legbot_selftest_service_start();
    if (selftest_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "开发台架自检自动运行请求失败，错误=0x%x",
                 (unsigned)selftest_error);
    }
#endif
    return ESP_OK;
}

#if CONFIG_LEGBOT_POWER_DIAGNOSTIC_BUILD
static void print_power_diagnostic_snapshot(void)
{
    esp_pm_config_t pm_config = {0};
    const esp_err_t pm_error = esp_pm_get_configuration(&pm_config);
    watch_state_snapshot_t state = {0};
    const esp_err_t state_error = watch_state_snapshot(&state, 0);
    const esp_app_desc_t *app = esp_app_get_description();
    const bool screen_state_valid =
        state_error == ESP_OK && state.screen_state < WATCH_SCREEN_STATE_COUNT;
    const bool screen_on =
        screen_state_valid && state.screen_state == WATCH_SCREEN_STATE_ON;

    ESP_LOGI(TAG,
             "低功耗诊断仅输出一次：固件=%s，profile=%s，诊断构建=是",
             app != NULL ? app->version : "unknown",
#if LEGBOT_CAP_MODEM
             "v2"
#else
             "v1"
#endif
    );
    if (pm_error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "动态电源目标：CPU=%d~%d MHz，automatic light sleep=%s",
                 pm_config.min_freq_mhz,
                 pm_config.max_freq_mhz,
                 pm_config.light_sleep_enable ? "开启" : "关闭");
    }
    else
    {
        ESP_LOGW(TAG,
                 "读取动态电源配置失败，错误=%s",
                 esp_err_to_name(pm_error));
    }
    ESP_LOGI(TAG,
             "外设目标：GPS=Standby(IO3低，无独立EN)，ML307R=%s，AMOLED=%s，屏幕序号=%" PRIu32 "，BLE=%s",
#if LEGBOT_CAP_MODEM
             "由v2 owner按既有策略控制",
#else
             "关闭(IO10低，主动高EN)",
#endif
             screen_state_valid ? (screen_on ? "开启(IO47高)" : "关闭(IO47低)")
                                : "状态未知",
             state_error == ESP_OK ? state.screen_transition_sequence : 0U,
             ble_service_runtime_ready()
                 ? "READY(无线栈已启用)"
                 : (ble_service_startup_ready()
                        ? "DORMANT(无线栈未初始化)"
                        : "尚未就绪"));
    ESP_LOGI(TAG, "当前 PM 锁快照如下；功耗验收必须断开 USB 后另行测量：");
    esp_pm_dump_locks(stdout);
}
#endif
