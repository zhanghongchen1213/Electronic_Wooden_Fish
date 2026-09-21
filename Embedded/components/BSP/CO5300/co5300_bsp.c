/**
 * @file     co5300_bsp.c
 * @brief    CO5300 AMOLED 项目 BSP 实现
 * @details  独占 SPI3_HOST 和显示 GPIO，维护 panel 生命周期并提供稳定恢复边界。
 * @author   ZHC
 * @date     2026-07-14
 */

#include "co5300_bsp.h"
#include "co5300_bounded_panel_io.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_co5300.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BSP_CO5300";

/** 单一 LVGL flush 同时只需要一个颜色事务，保留一个额外队列槽用于驱动回收。 */
#define CO5300_BSP_SPI_TRANSACTION_QUEUE_DEPTH 2
/** CO5300 QSPI 列地址设置命令头。 */
#define CO5300_BSP_QSPI_CASET_COMMAND 0x02002A00U
/** CO5300 QSPI 行地址设置命令头。 */
#define CO5300_BSP_QSPI_RASET_COMMAND 0x02002B00U
/** CO5300 QSPI 颜色写入命令头。 */
#define CO5300_BSP_QSPI_RAMWR_COMMAND 0x32002C00U

_Static_assert(CO5300_BSP_SLEEP_IN_SETTLE_MS >= 120U,
               "CO5300 sleep-in must wait for booster-off completion.");
_Static_assert(CO5300_BSP_SLEEP_OUT_SETTLE_MS >= 120U,
               "CO5300 sleep-out must wait for booster-on completion.");

/** 按本地 CO5300 规格保留的 QSPI 初始化表；SLPOUT 与 DISPON 由 BSP 显式排序。 */
static const co5300_lcd_init_cmd_t s_panel_init_commands[] = {
    {0xFE, NULL, 0U, 0U},
    {0xC4, (const uint8_t[]){0x80}, 1U, 0U},
    {0x35, NULL, 0U, 10U},
    {0x53, (const uint8_t[]){0x20}, 1U, 10U},
    {0x51, (const uint8_t[]){0xFF}, 1U, 10U},
    {0x63, (const uint8_t[]){0xFF}, 1U, 10U},
    {0x2A, (const uint8_t[]){0x00, 0x06, 0x01, 0xDD}, 4U, 0U},
    {0x2B, (const uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4U, 0U},
};

/** AMOLED 休眠事务的可恢复分阶段状态。 */
typedef enum
{
    CO5300_POWER_ACTIVE = 0, /**< 面板已初始化并允许颜色与亮度事务。 */
    CO5300_POWER_DISPLAY_OFF, /**< 已完成 DISPOFF，尚未确认 SLPIN。 */
    CO5300_POWER_PANEL_SLEEP, /**< 已完成 SLPIN，IO47 偏压仍可能开启。 */
    CO5300_POWER_RAIL_OFF, /**< 已完成 SLPIN 且 IO47 偏压关闭。 */
} co5300_power_state_t;

/** CO5300 panel IO 句柄。 */
static esp_lcd_panel_io_handle_t s_panel_io;
/** CO5300 panel 句柄。 */
static esp_lcd_panel_handle_t s_panel;
/** SPI3_HOST 总线是否由本 BSP 初始化。 */
static bool s_spi_bus_initialized;
/** CO5300 初始化完成状态。 */
static bool s_initialized;
/** 上层颜色传输完成回调。 */
static co5300_bsp_flush_done_cb_t s_flush_done_cb;
/** 上层颜色传输完成回调上下文。 */
static void *s_flush_done_ctx;
/** 最近一次成功设置或驱动初始化恢复的 CO5300 原始亮度。 */
static uint8_t s_brightness = CO5300_BSP_BRIGHTNESS_DEFAULT;
/** 当前亮度缓存是否对应有效的 panel 状态。 */
static bool s_brightness_valid;
/** 下次面板初始化或休眠恢复后应采用的原始亮度。 */
static uint8_t s_requested_brightness = CO5300_BSP_BRIGHTNESS_DEFAULT;
/** CO5300 当前是否已执行 DISPON。 */
static bool s_display_on;
/** 当前 AMOLED 休眠事务阶段，部分失败时用于后续恢复。 */
static co5300_power_state_t s_power_state = CO5300_POWER_ACTIVE;
/** 显示供电、复位和休眠转换期间禁止自动 light sleep 的电源管理锁。 */
static esp_pm_lock_handle_t s_transition_pm_lock;
/** 显示转换电源管理锁是否已被当前 UI owner 持有。 */
static bool s_transition_pm_lock_held;

static bool panel_io_color_done(esp_lcd_panel_io_handle_t panel_io,
                                esp_lcd_panel_io_event_data_t *event_data,
                                void *user_ctx);
static esp_err_t initialize_panel(bool display_on);
static esp_err_t write_brightness(uint8_t raw_level);
static esp_err_t begin_power_transition(void);
static esp_err_t end_power_transition(esp_err_t operation_error);
static void delay_at_least_ms(uint32_t delay_ms);
static void cleanup_failed_init(void);

const legbot_bsp_resource_t *co5300_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_CO5300);
}

esp_err_t co5300_bsp_init(const co5300_bsp_config_t *config)
{
    if (s_initialized)
    {
        return ESP_OK;
    }
    if (s_transition_pm_lock != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (config == NULL || config->flush_done_cb == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const legbot_bsp_resource_t *resource = co5300_bsp_resource();
    if (resource == NULL || resource->spi_host != LEGBOT_BSP_DISPLAY_SPI_HOST)
    {
        return ESP_ERR_INVALID_STATE;
    }

    s_flush_done_cb = config->flush_done_cb;
    s_flush_done_ctx = config->user_ctx;

    esp_err_t err = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,
                                       0,
                                       "co5300_transition",
                                       &s_transition_pm_lock);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    err = begin_power_transition();
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }

    err = gpio_set_direction(LEGBOT_BSP_DISPLAY_EN_GPIO, GPIO_MODE_OUTPUT);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    /* CO5300 规格书说明 RESX 无内部上拉，light sleep 中必须持续保持释放高电平。 */
    err = gpio_sleep_sel_dis(LEGBOT_BSP_DISPLAY_RST_GPIO);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    /* CSX 低有效，空闲期保持 SPI 活动态高电平，避免睡眠浮空产生伪命令。 */
    err = gpio_sleep_sel_dis(LEGBOT_BSP_DISPLAY_CS_GPIO);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    /* 自动 light sleep 默认隔离 GPIO；IO47 必须保持当前输出，避免 AMOLED 偏压 EN 悬空。 */
    err = gpio_sleep_sel_dis(LEGBOT_BSP_DISPLAY_EN_GPIO);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    err = gpio_set_level(LEGBOT_BSP_DISPLAY_EN_GPIO, 1);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    /* AMOLED 偏压使能后等待电源稳定，再复位控制器。 */
    delay_at_least_ms(CO5300_BSP_POWER_RAIL_SETTLE_MS);

    const spi_bus_config_t bus_config = CO5300_PANEL_BUS_QSPI_CONFIG(
        LEGBOT_BSP_DISPLAY_SCL_GPIO,
        LEGBOT_BSP_DISPLAY_D0_GPIO,
        LEGBOT_BSP_DISPLAY_D1_GPIO,
        LEGBOT_BSP_DISPLAY_D2_GPIO,
        LEGBOT_BSP_DISPLAY_D3_GPIO,
        CO5300_BSP_WIDTH * CO5300_BSP_DRAW_BUFFER_LINES * sizeof(uint16_t));
    err = spi_bus_initialize(LEGBOT_BSP_DISPLAY_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    s_spi_bus_initialized = true;

    esp_lcd_panel_io_spi_config_t io_config = CO5300_PANEL_IO_QSPI_CONFIG(
        LEGBOT_BSP_DISPLAY_CS_GPIO, panel_io_color_done, NULL);
    io_config.pclk_hz = CO5300_BSP_QSPI_CLOCK_HZ;
    io_config.trans_queue_depth = CO5300_BSP_SPI_TRANSACTION_QUEUE_DEPTH;
    err = co5300_bounded_panel_io_new(
        (esp_lcd_spi_bus_handle_t)LEGBOT_BSP_DISPLAY_SPI_HOST,
        &io_config,
        &s_panel_io);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }

    const co5300_vendor_config_t vendor_config = {
        .init_cmds = s_panel_init_commands,
        .init_cmds_size =
            sizeof(s_panel_init_commands) / sizeof(s_panel_init_commands[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LEGBOT_BSP_DISPLAY_RST_GPIO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = CO5300_BSP_BITS_PER_PIXEL,
        .vendor_config = (void *)&vendor_config,
    };
    err = esp_lcd_new_panel_co5300(s_panel_io, &panel_config, &s_panel);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }

    err = initialize_panel(false);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }
    err = end_power_transition(ESP_OK);
    if (err != ESP_OK)
    {
        cleanup_failed_init();
        return err;
    }

    s_initialized = true;
    s_power_state = CO5300_POWER_ACTIVE;
    ESP_LOGI(TAG,
             "CO5300 初始化成功：QSPI=%d Hz，RGB565=%d 位，尺寸=%dx%d，偏移=%d,%d",
             CO5300_BSP_QSPI_CLOCK_HZ,
             CO5300_BSP_BITS_PER_PIXEL,
             CO5300_BSP_WIDTH,
             CO5300_BSP_HEIGHT,
             CO5300_BSP_X_GAP,
             CO5300_BSP_Y_GAP);
    return ESP_OK;
}

esp_err_t co5300_bsp_deinit(void)
{
    esp_err_t first_error = ESP_OK;
    if (s_panel != NULL)
    {
        esp_err_t err = esp_lcd_panel_del(s_panel);
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
        s_panel = NULL;
    }
    if (s_panel_io != NULL)
    {
        esp_err_t err = esp_lcd_panel_io_del(s_panel_io);
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
        s_panel_io = NULL;
    }
    if (s_spi_bus_initialized)
    {
        esp_err_t err = spi_bus_free(LEGBOT_BSP_DISPLAY_SPI_HOST);
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
        s_spi_bus_initialized = false;
    }
    (void)gpio_set_level(LEGBOT_BSP_DISPLAY_EN_GPIO, 0);
    if (s_transition_pm_lock_held)
    {
        esp_err_t err = esp_pm_lock_release(s_transition_pm_lock);
        if (err == ESP_OK)
        {
            s_transition_pm_lock_held = false;
        }
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
    }
    if (s_transition_pm_lock != NULL && !s_transition_pm_lock_held)
    {
        esp_err_t err = esp_pm_lock_delete(s_transition_pm_lock);
        if (err == ESP_OK)
        {
            s_transition_pm_lock = NULL;
        }
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
    }
    s_initialized = false;
    s_flush_done_cb = NULL;
    s_flush_done_ctx = NULL;
    s_brightness = CO5300_BSP_BRIGHTNESS_DEFAULT;
    s_brightness_valid = false;
    s_requested_brightness = CO5300_BSP_BRIGHTNESS_DEFAULT;
    s_display_on = false;
    s_power_state = CO5300_POWER_ACTIVE;
    return first_error;
}

esp_err_t co5300_bsp_draw_bitmap(int x_start,
                                 int y_start,
                                 int x_end,
                                 int y_end,
                                 const void *color_data)
{
    if (!s_initialized || s_panel == NULL ||
        s_power_state != CO5300_POWER_ACTIVE)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (color_data == NULL || x_start < 0 || y_start < 0 || x_end <= x_start || y_end <= y_start ||
        x_end > CO5300_BSP_WIDTH || y_end > CO5300_BSP_HEIGHT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    x_start += CO5300_BSP_X_GAP;
    x_end += CO5300_BSP_X_GAP;
    y_start += CO5300_BSP_Y_GAP;
    y_end += CO5300_BSP_Y_GAP;
    const uint8_t column_window[4] = {
        (uint8_t)((uint32_t)x_start >> 8U),
        (uint8_t)x_start,
        (uint8_t)((uint32_t)(x_end - 1) >> 8U),
        (uint8_t)(x_end - 1),
    };
    esp_err_t error = esp_lcd_panel_io_tx_param(
        s_panel_io,
        (int)CO5300_BSP_QSPI_CASET_COMMAND,
        column_window,
        sizeof(column_window));
    if (error != ESP_OK)
    {
        return error;
    }
    const uint8_t row_window[4] = {
        (uint8_t)((uint32_t)y_start >> 8U),
        (uint8_t)y_start,
        (uint8_t)((uint32_t)(y_end - 1) >> 8U),
        (uint8_t)(y_end - 1),
    };
    error = esp_lcd_panel_io_tx_param(
        s_panel_io,
        (int)CO5300_BSP_QSPI_RASET_COMMAND,
        row_window,
        sizeof(row_window));
    if (error != ESP_OK)
    {
        return error;
    }
    const size_t color_bytes =
        (size_t)(x_end - x_start) *
        (size_t)(y_end - y_start) *
        sizeof(uint16_t);
    return esp_lcd_panel_io_tx_color(
        s_panel_io,
        (int)CO5300_BSP_QSPI_RAMWR_COMMAND,
        color_data,
        color_bytes);
}

esp_err_t co5300_bsp_set_display(bool on)
{
    if (!s_initialized || s_panel == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_power_state != CO5300_POWER_ACTIVE)
    {
        return on ? ESP_ERR_INVALID_STATE : ESP_OK;
    }
    if (s_display_on == on)
    {
        return ESP_OK;
    }
    esp_err_t err = esp_lcd_panel_disp_on_off(s_panel, on);
    if (err == ESP_OK)
    {
        s_display_on = on;
        ESP_LOGI(TAG, "CO5300 已%s", on ? "亮屏" : "熄屏");
    }
    else
    {
        ESP_LOGE(TAG, "CO5300 %s失败，错误=0x%x", on ? "亮屏" : "熄屏", (unsigned)err);
    }
    return err;
}

esp_err_t co5300_bsp_suspend(void)
{
    if (!s_initialized || s_panel == NULL || s_panel_io == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_power_state == CO5300_POWER_RAIL_OFF)
    {
        return ESP_OK;
    }

    esp_err_t error = begin_power_transition();
    if (error != ESP_OK)
    {
        return error;
    }
    if (s_power_state == CO5300_POWER_ACTIVE)
    {
        if (s_display_on)
        {
            error = esp_lcd_panel_disp_on_off(s_panel, false);
            if (error != ESP_OK)
            {
                ESP_LOGE(TAG,
                         "CO5300 DISPOFF 失败，仍保持活动态，错误=0x%x",
                         (unsigned)error);
                return end_power_transition(error);
            }
        }
        s_display_on = false;
        s_power_state = CO5300_POWER_DISPLAY_OFF;
    }
    if (s_power_state == CO5300_POWER_DISPLAY_OFF)
    {
        error = esp_lcd_panel_io_tx_param(
            s_panel_io,
            (int)CO5300_BSP_QSPI_SLEEP_IN_COMMAND,
            NULL,
            0U);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "CO5300 SLPIN 失败，已保留 DISPOFF 阶段供重试或恢复，错误=0x%x",
                     (unsigned)error);
            return end_power_transition(error);
        }
        s_power_state = CO5300_POWER_PANEL_SLEEP;
    }

    delay_at_least_ms(CO5300_BSP_SLEEP_IN_SETTLE_MS);
    error = gpio_set_level(LEGBOT_BSP_DISPLAY_EN_GPIO, 0);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "CO5300 SLPIN 完成但 IO47 偏压断电失败，已保留可恢复阶段，错误=0x%x",
                 (unsigned)error);
        return end_power_transition(error);
    }
    s_power_state = CO5300_POWER_RAIL_OFF;
    s_brightness_valid = false;
    error = end_power_transition(ESP_OK);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG, "CO5300 已进入 SLPIN 并关闭 IO47 AMOLED 偏压");
    }
    return error;
}

esp_err_t co5300_bsp_resume(void)
{
    if (!s_initialized || s_panel == NULL || s_panel_io == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_power_state == CO5300_POWER_ACTIVE)
    {
        return ESP_OK;
    }

    esp_err_t error = begin_power_transition();
    if (error != ESP_OK)
    {
        return error;
    }
    if (s_power_state == CO5300_POWER_RAIL_OFF)
    {
        error = gpio_set_level(LEGBOT_BSP_DISPLAY_EN_GPIO, 1);
        if (error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "CO5300 恢复时 IO47 上电失败，错误=0x%x",
                     (unsigned)error);
            return end_power_transition(error);
        }
        delay_at_least_ms(CO5300_BSP_POWER_RAIL_SETTLE_MS);
        s_power_state = CO5300_POWER_PANEL_SLEEP;
    }
    error = initialize_panel(false);
    if (error == ESP_OK)
    {
        error = write_brightness(s_requested_brightness);
    }
    if (error != ESP_OK)
    {
        const esp_err_t rail_error =
            gpio_set_level(LEGBOT_BSP_DISPLAY_EN_GPIO, 0);
        s_power_state = rail_error == ESP_OK
                            ? CO5300_POWER_RAIL_OFF
                            : CO5300_POWER_DISPLAY_OFF;
        s_brightness_valid = false;
        ESP_LOGE(TAG,
                 "CO5300 休眠恢复失败，已尝试重新关闭偏压，错误=0x%x，断电错误=0x%x",
                 (unsigned)error,
                 (unsigned)rail_error);
        return end_power_transition(error);
    }

    s_power_state = CO5300_POWER_ACTIVE;
    error = end_power_transition(ESP_OK);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "CO5300 已恢复供电、面板初始化与亮度=%u",
                 (unsigned)s_requested_brightness);
    }
    return error;
}

esp_err_t co5300_bsp_build_brightness_command(
    uint16_t raw_level,
    co5300_bsp_brightness_command_t *command)
{
    if (command == NULL || raw_level > CO5300_BSP_BRIGHTNESS_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    command->command = CO5300_BSP_QSPI_BRIGHTNESS_COMMAND;
    command->value = (uint8_t)raw_level;
    return ESP_OK;
}

esp_err_t co5300_bsp_set_brightness(uint16_t raw_level)
{
    co5300_bsp_brightness_command_t command;
    esp_err_t err = co5300_bsp_build_brightness_command(raw_level, &command);
    if (err != ESP_OK)
    {
        return err;
    }
    if (!s_initialized || s_panel_io == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    s_requested_brightness = command.value;
    if (s_power_state != CO5300_POWER_ACTIVE)
    {
        s_brightness_valid = false;
        return ESP_OK;
    }
    if (s_brightness_valid && s_brightness == command.value)
    {
        return ESP_OK;
    }

    err = write_brightness(command.value);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "CO5300 亮度已设置为 %u", (unsigned)command.value);
    }
    else
    {
        ESP_LOGE(TAG, "CO5300 亮度设置失败，错误=0x%x", (unsigned)err);
    }
    return err;
}

esp_err_t co5300_bsp_reset(void)
{
    if (!s_initialized || s_panel == NULL ||
        s_power_state != CO5300_POWER_ACTIVE)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = begin_power_transition();
    if (err != ESP_OK)
    {
        return err;
    }
    err = initialize_panel(s_display_on);
    if (err == ESP_OK)
    {
        err = write_brightness(s_requested_brightness);
    }
    err = end_power_transition(err);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "CO5300 复位恢复成功，沿用原 panel 句柄");
    }
    else
    {
        ESP_LOGE(TAG, "CO5300 复位恢复失败，错误=0x%x", (unsigned)err);
    }
    return err;
}

bool co5300_bsp_is_initialized(void)
{
    return s_initialized;
}

bool co5300_bsp_is_suspended(void)
{
    return s_initialized && s_power_state != CO5300_POWER_ACTIVE;
}

static bool panel_io_color_done(esp_lcd_panel_io_handle_t panel_io,
                                esp_lcd_panel_io_event_data_t *event_data,
                                void *user_ctx)
{
    (void)panel_io;
    (void)event_data;
    (void)user_ctx;
    return s_flush_done_cb != NULL && s_flush_done_cb(s_flush_done_ctx);
}

static esp_err_t initialize_panel(bool display_on)
{
    s_display_on = false;
    esp_err_t err = esp_lcd_panel_reset(s_panel);
    if (err != ESP_OK)
    {
        return err;
    }
    err = esp_lcd_panel_init(s_panel);
    if (err != ESP_OK)
    {
        return err;
    }
    err = esp_lcd_panel_set_gap(s_panel, CO5300_BSP_X_GAP, CO5300_BSP_Y_GAP);
    if (err != ESP_OK)
    {
        return err;
    }
    err = esp_lcd_panel_io_tx_param(
        s_panel_io,
        (int)CO5300_BSP_QSPI_SLEEP_OUT_COMMAND,
        NULL,
        0U);
    if (err != ESP_OK)
    {
        return err;
    }
    delay_at_least_ms(CO5300_BSP_SLEEP_OUT_SETTLE_MS);
    /* 初始化表会通过 0x51 恢复为 0xFF，但在确定首帧写入前保持 DISPOFF。 */
    s_brightness = CO5300_BSP_BRIGHTNESS_DEFAULT;
    s_brightness_valid = true;
    if (!display_on)
    {
        return ESP_OK;
    }
    err = esp_lcd_panel_disp_on_off(s_panel, true);
    s_display_on = err == ESP_OK;
    return err;
}

static esp_err_t write_brightness(uint8_t raw_level)
{
    const uint32_t command = CO5300_BSP_QSPI_BRIGHTNESS_COMMAND;
    const esp_err_t error = esp_lcd_panel_io_tx_param(
        s_panel_io,
        (int)command,
        &raw_level,
        sizeof(raw_level));
    if (error == ESP_OK)
    {
        s_brightness = raw_level;
        s_brightness_valid = true;
    }
    return error;
}

static esp_err_t begin_power_transition(void)
{
    if (s_transition_pm_lock == NULL || s_transition_pm_lock_held)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t error = esp_pm_lock_acquire(s_transition_pm_lock);
    if (error == ESP_OK)
    {
        s_transition_pm_lock_held = true;
    }
    else
    {
        ESP_LOGE(TAG,
                 "CO5300 显示转换无法禁止 automatic light sleep，错误=0x%x",
                 (unsigned)error);
    }
    return error;
}

static esp_err_t end_power_transition(esp_err_t operation_error)
{
    if (s_transition_pm_lock == NULL || !s_transition_pm_lock_held)
    {
        return operation_error != ESP_OK
                   ? operation_error
                   : ESP_ERR_INVALID_STATE;
    }
    const esp_err_t release_error =
        esp_pm_lock_release(s_transition_pm_lock);
    if (release_error == ESP_OK)
    {
        s_transition_pm_lock_held = false;
    }
    else
    {
        ESP_LOGE(TAG,
                 "CO5300 显示转换释放 light sleep 锁失败，错误=0x%x",
                 (unsigned)release_error);
    }
    return operation_error != ESP_OK ? operation_error : release_error;
}

static void delay_at_least_ms(uint32_t delay_ms)
{
    const int64_t deadline_us =
        esp_timer_get_time() + (int64_t)delay_ms * 1000LL;
    while (true)
    {
        const int64_t now_us = esp_timer_get_time();
        if (now_us >= deadline_us)
        {
            break;
        }
        const int64_t remaining_us = deadline_us - now_us;
        const uint32_t remaining_ms =
            (uint32_t)((remaining_us + 999LL) / 1000LL);
        TickType_t delay_ticks = pdMS_TO_TICKS(remaining_ms);
        if (delay_ticks == 0U)
        {
            delay_ticks = 1U;
        }
        vTaskDelay(delay_ticks);
    }
}

static void cleanup_failed_init(void)
{
    ESP_LOGE(TAG, "CO5300 初始化失败，执行无残留清理");
    (void)co5300_bsp_deinit();
}
