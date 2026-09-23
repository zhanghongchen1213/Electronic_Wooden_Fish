/**
 * @file     app_main.c
 * @brief    电子木鱼 EWF 固件主入口
 * @details  按固定顺序串行执行初始化阶段：日志/NVS/配置、typed event/state、板级引脚、
 *           共享总线、服务、自检挂钩。每阶段失败都输出中文日志与稳定错误码并停止继续启动。
 *           固件不检测充电状态、不驱动 LTC2954 KILL，也不提供软件关机。
 * @author   ZHC
 * @date     2026-08-03
 */

#include <inttypes.h>
#include <stdio.h>

#include "app_state.h"
#include "bsp_board.h"
#include "bsp_resources.h"
#include "cst9217_bsp.h"
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
#include "nvs_flash.h"
#include "qmi8658c_bsp.h"
#include "sdkconfig.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "EWF firmware is pinned to esp32s3."
#endif
#if !CONFIG_PM_ENABLE
#error "EWF requires CONFIG_PM_ENABLE=y."
#endif
#if !CONFIG_FREERTOS_USE_TICKLESS_IDLE
#error "EWF requires CONFIG_FREERTOS_USE_TICKLESS_IDLE=y."
#endif
#if !defined(CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP) || \
    CONFIG_FREERTOS_IDLE_TIME_BEFORE_SLEEP != 3
#error "EWF requires a three-tick automatic light sleep threshold."
#endif
#if !CONFIG_RTC_CLK_SRC_INT_RC
#error "EWF requires the board-proven internal RC RTC clock."
#endif
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED) && \
    CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED && \
    (!defined(CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION) || \
     !CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION)
#error "EWF requires USB Serial/JTAG monitoring protection during automatic light sleep."
#endif

static const char *TAG = "MAIN";

/** 启动期间阻止硬件初始化延时误入自动 light sleep 的 PM 锁。 */
static esp_pm_lock_handle_t s_startup_light_sleep_lock;
/** 启动期 PM 锁当前是否仍被持有。 */
static bool s_startup_light_sleep_lock_held;

static void print_chip_info(void);
static void print_flash_psram_info(void);
static void log_heap_allocation_failure(size_t size,
                                        uint32_t caps,
                                        const char *function_name);
static esp_err_t init_power_management(void);
static esp_err_t release_startup_light_sleep_lock(void);
static esp_err_t abort_startup_power_management(void);
static esp_err_t run_stage(legbot_bsp_init_stage_t stage, esp_err_t (*fn)(void));
static esp_err_t init_log_nvs_config(void);
static esp_err_t init_event_state(void);
static esp_err_t init_board_pins(void);
static esp_err_t init_shared_buses(void);
static esp_err_t init_services(void);
static esp_err_t init_selftest_hooks(void);

void app_main(void)
{
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

    ESP_LOGI(TAG, "启动电子木鱼 EWF 固件骨架");
    print_chip_info();
    ESP_LOGI(TAG,
             "应用日志使用原生 USB Serial/JTAG；Air780EGP UART1 IO43/IO44 只保留资源边界，"
             "本 Story 不启用任何模组业务");
    print_flash_psram_info();

    /* 初始化阶段按架构顺序串行执行，任何阶段失败都停止继续启动。 */
    if (run_stage(LEGBOT_BSP_STAGE_LOG_NVS_CONFIG, init_log_nvs_config) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_EVENT_STATE, init_event_state) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_BOARD_PINS, init_board_pins) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_SHARED_BUSES, init_shared_buses) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_SERVICES, init_services) != ESP_OK)
    {
        goto startup_failed;
    }
    if (run_stage(LEGBOT_BSP_STAGE_SELFTEST_HOOKS, init_selftest_hooks) != ESP_OK)
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

    ESP_LOGI(TAG,
             "固件骨架已就绪：PWR_INT=IO8/BOOT0=IO0 只读输入边界与 BSP 自检挂钩已就绪；"
             "三种供电条件共享同一启动与运行路径，主控不检测充电状态");
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

static esp_err_t run_stage(legbot_bsp_init_stage_t stage, esp_err_t (*fn)(void))
{
    ESP_LOGI(TAG, "初始化阶段：%s", legbot_bsp_stage_name(stage));
    const esp_err_t err = fn();
    /* 真实返回值写入 BSP 阶段表，供自检按 evidence 类别追溯。 */
    legbot_bsp_record_stage_result(stage, err);
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
    ESP_LOGI(TAG, "日志、NVS 与配置服务已就绪");
    return ESP_OK;
}

static esp_err_t init_event_state(void)
{
    esp_err_t err = event_bus_init();
    if (err != ESP_OK)
    {
        return err;
    }
    err = watch_state_init();
    if (err != ESP_OK)
    {
        return err;
    }
    ESP_LOGI(TAG, "typed event/state 边界已就绪");
    return ESP_OK;
}

static esp_err_t init_board_pins(void)
{
    legbot_board_init_result_t result = {0};
    const esp_err_t err = legbot_board_init(&result);
    if (err != ESP_OK)
    {
        return err;
    }
    if (result.audio_degraded)
    {
        ESP_LOGW(TAG,
                 "NS4150 未能进入安全关断状态，音频能力降级，原始错误=0x%x；"
                 "本 Story 不启用音频业务",
                 (unsigned)result.audio_cause);
    }
    return ESP_OK;
}

static esp_err_t init_shared_buses(void)
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
    err = cst9217_bsp_init(i2c_access.bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "CST9217 触摸链路不可用，device_touch 生产路径降级，错误=0x%x",
                 (unsigned)err);
    }
    ESP_LOGI(TAG,
             "共享 I2C0（IO1/IO2）已就绪；I2S 与屏幕总线所有权保留给对应 Story，"
             "本阶段不启用显示或音频业务");
    return ESP_OK;
}

static esp_err_t init_services(void)
{
    return legbot_services_start_all();
}

static esp_err_t init_selftest_hooks(void)
{
#if CONFIG_EWF_BENCH_SELFTEST_AUTORUN
    const esp_err_t selftest_error = legbot_selftest_service_start();
    if (selftest_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "开发台架自检自动运行请求失败，错误=0x%x",
                 (unsigned)selftest_error);
        return ESP_OK;
    }
    ESP_LOGW(TAG, "开发台架自检已自动运行一次；正式默认配置必须保持关闭");
#else
    ESP_LOGI(TAG,
             "自检挂钩已就绪：本构建不自动运行自检，"
             "可通过 legbot_selftest_service_start() 按需触发");
#endif
    return ESP_OK;
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
    const esp_app_desc_t *app = esp_app_get_description();
    ESP_LOGI(TAG,
             "固件版本=%s，最小剩余堆内存=%" PRIu32 " 字节",
             app != NULL ? app->version : "unknown",
             esp_get_minimum_free_heap_size());
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

    if (esp_psram_is_initialized())
    {
        const size_t psram_size = esp_psram_get_size();
        ESP_LOGI(TAG,
                 "PSRAM 大小：%u 字节（%u MB），堆总量/空闲：%u/%u 字节",
                 (unsigned)psram_size,
                 (unsigned)(psram_size / (1024 * 1024)),
                 (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    }
    else
    {
        ESP_LOGW(TAG, "PSRAM 未初始化；如果该板有 PSRAM，请启用 CONFIG_SPIRAM");
    }
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

    const esp_pm_config_t config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 40,
        .light_sleep_enable = true,
    };
    error = esp_pm_configure(&config);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "动态调频与自动 light sleep 已预配置：CPU=40~240 MHz，"
                 "启动 PM 锁正保护硬件初始化");
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
    /* 只有 PWR_INT/BOOT0/PVDF 比较器被配置为输入，GPIO 唤醒不会触碰未分配的 IO46/IO45。 */
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
             "自动 light sleep 启动锁已释放：空闲阈值=3 tick，"
             "唤醒源=PWR_INT IO8/BOOT0 IO0/PVDF 比较器 IO11；"
             "USB 主机连接期间保持调试可用");
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
