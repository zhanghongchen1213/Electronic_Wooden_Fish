/**
 * @file     cst9217_bsp.c
 * @brief    CST9217 触摸项目 BSP 实现
 * @details  接收 manager-owned I2C0 bus handle，维护 panel IO 与触摸句柄的无泄漏生命周期。
 * @author   ZHC
 * @date     2026-07-10
 */

#include "cst9217_bsp.h"

#include "driver/gpio.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst9217.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "BSP_CST9217";

/** CST9217 I2C panel IO 句柄。 */
static esp_lcd_panel_io_handle_t s_touch_io;
/** CST9217 触摸控制器句柄。 */
static esp_lcd_touch_handle_t s_touch;
/** CST9217 当前使用的 manager-owned bus handle。 */
static i2c_master_bus_handle_t s_bus_handle;
/** CST9217 初始化完成状态。 */
static bool s_initialized;
/** 上层注册的 ISR 安全触摸中断回调。 */
static cst9217_bsp_interrupt_cb_t s_interrupt_callback;
/** 触摸中断回调上下文。 */
static void *s_interrupt_user_ctx;
/** 任务与 GPIO ISR 之间保护回调快照的自旋锁。 */
static portMUX_TYPE s_interrupt_lock = portMUX_INITIALIZER_UNLOCKED;

static esp_err_t create_touch(i2c_master_bus_handle_t bus_handle);
static esp_err_t rearm_touch_interrupt(void);
static void touch_interrupt_bridge(esp_lcd_touch_handle_t touch);
static void interrupt_callback_snapshot(
    cst9217_bsp_interrupt_cb_t *callback,
    void **user_ctx);

const legbot_bsp_resource_t *cst9217_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_CST9217);
}

esp_err_t cst9217_bsp_init(i2c_master_bus_handle_t bus_handle)
{
    if (s_initialized)
    {
        return s_bus_handle == bus_handle ? ESP_OK : ESP_ERR_INVALID_STATE;
    }
    if (bus_handle == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const legbot_bsp_resource_t *resource = cst9217_bsp_resource();
    if (resource == NULL || resource->i2c_port != LEGBOT_BSP_I2C_PORT ||
        resource->gpio_primary != LEGBOT_BSP_CST9217_RST_GPIO ||
        resource->gpio_secondary != LEGBOT_BSP_CST9217_INT_GPIO)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = create_touch(bus_handle);
    if (err != ESP_OK)
    {
        (void)cst9217_bsp_deinit();
        ESP_LOGE(TAG, "CST9217 初始化失败，错误=0x%x", (unsigned)err);
        return err;
    }
    s_bus_handle = bus_handle;
    s_initialized = true;
    ESP_LOGI(TAG,
             "CST9217 初始化成功：I2C%d，RST=IO%d，INT=IO%d，尺寸=%dx%d",
             LEGBOT_BSP_I2C_PORT,
             LEGBOT_BSP_CST9217_RST_GPIO,
             LEGBOT_BSP_CST9217_INT_GPIO,
             CST9217_BSP_WIDTH,
             CST9217_BSP_HEIGHT);
    return ESP_OK;
}

esp_err_t cst9217_bsp_deinit(void)
{
    esp_err_t first_error = ESP_OK;
    cst9217_bsp_interrupt_cb_t registered_callback = NULL;
    void *registered_user_ctx = NULL;
    interrupt_callback_snapshot(&registered_callback,
                                &registered_user_ctx);
    if (s_touch != NULL && registered_callback != NULL)
    {
        const esp_err_t interrupt_error =
            cst9217_bsp_register_interrupt_callback(NULL, NULL);
        if (interrupt_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "CST9217 中断回调解注册失败，保留触摸句柄防止 ISR 悬空，错误=0x%x",
                     (unsigned)interrupt_error);
            return interrupt_error;
        }
    }
    if (s_touch != NULL)
    {
        esp_err_t err = esp_lcd_touch_del(s_touch);
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
        s_touch = NULL;
    }
    if (s_touch_io != NULL)
    {
        esp_err_t err = esp_lcd_panel_io_del(s_touch_io);
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
        s_touch_io = NULL;
    }
    s_initialized = false;
    s_bus_handle = NULL;
    portENTER_CRITICAL(&s_interrupt_lock);
    s_interrupt_callback = NULL;
    s_interrupt_user_ctx = NULL;
    portEXIT_CRITICAL(&s_interrupt_lock);
    return first_error;
}

esp_err_t cst9217_bsp_read_point(cst9217_bsp_point_t *point)
{
    if (point == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *point = (cst9217_bsp_point_t){0};
    if (!s_initialized || s_touch == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = esp_lcd_touch_read_data(s_touch);
    esp_lcd_touch_point_data_t data = {0};
    uint8_t point_count = 0;
    if (err == ESP_OK)
    {
        err = esp_lcd_touch_get_data(s_touch, &data, &point_count, 1);
    }
    if (err == ESP_OK && point_count > 0)
    {
        point->x = data.x;
        point->y = data.y;
        point->strength = data.strength;
        point->pressed = true;
    }
    const esp_err_t rearm_error = rearm_touch_interrupt();
    return err != ESP_OK ? err : rearm_error;
}

esp_err_t cst9217_bsp_register_interrupt_callback(
    cst9217_bsp_interrupt_cb_t callback,
    void *user_ctx)
{
    if (callback == NULL && user_ctx != NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized || s_touch == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    cst9217_bsp_interrupt_cb_t registered_callback = NULL;
    void *registered_user_ctx = NULL;
    interrupt_callback_snapshot(&registered_callback,
                                &registered_user_ctx);
    if (callback == NULL)
    {
        if (registered_callback == NULL)
        {
            return ESP_OK;
        }
        esp_err_t error =
            esp_lcd_touch_register_interrupt_callback(s_touch, NULL);
        if (error != ESP_OK)
        {
            return error;
        }
        portENTER_CRITICAL(&s_interrupt_lock);
        s_interrupt_callback = NULL;
        s_interrupt_user_ctx = NULL;
        portEXIT_CRITICAL(&s_interrupt_lock);
        error = gpio_wakeup_disable(
            LEGBOT_BSP_CST9217_INT_GPIO);
        if (error == ESP_OK)
        {
            error = gpio_set_intr_type(
                LEGBOT_BSP_CST9217_INT_GPIO,
                GPIO_INTR_NEGEDGE);
        }
        ESP_LOGI(TAG, "CST9217 GPIO 中断回调已解注册");
        return error;
    }
    if (registered_callback != NULL)
    {
        return registered_callback == callback &&
                       registered_user_ctx == user_ctx
                   ? ESP_OK
                   : ESP_ERR_INVALID_STATE;
    }

    portENTER_CRITICAL(&s_interrupt_lock);
    s_interrupt_callback = callback;
    s_interrupt_user_ctx = user_ctx;
    portEXIT_CRITICAL(&s_interrupt_lock);
    esp_err_t error = gpio_wakeup_enable(
        LEGBOT_BSP_CST9217_INT_GPIO,
        GPIO_INTR_LOW_LEVEL);
    if (error != ESP_OK)
    {
        portENTER_CRITICAL(&s_interrupt_lock);
        s_interrupt_callback = NULL;
        s_interrupt_user_ctx = NULL;
        portEXIT_CRITICAL(&s_interrupt_lock);
        return error;
    }
    error = esp_lcd_touch_register_interrupt_callback(
        s_touch,
        touch_interrupt_bridge);
    if (error != ESP_OK)
    {
        portENTER_CRITICAL(&s_interrupt_lock);
        s_interrupt_callback = NULL;
        s_interrupt_user_ctx = NULL;
        portEXIT_CRITICAL(&s_interrupt_lock);
        (void)gpio_wakeup_disable(LEGBOT_BSP_CST9217_INT_GPIO);
        (void)gpio_set_intr_type(LEGBOT_BSP_CST9217_INT_GPIO,
                                 GPIO_INTR_NEGEDGE);
        ESP_LOGE(TAG,
                 "CST9217 GPIO 中断回调注册失败，错误=0x%x",
                 (unsigned)error);
        return error;
    }
    ESP_LOGI(TAG, "CST9217 GPIO 中断回调已注册，未写入私有休眠寄存器");
    return ESP_OK;
}

esp_err_t cst9217_bsp_reset(void)
{
    i2c_master_bus_handle_t bus_handle = s_bus_handle;
    if (bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    cst9217_bsp_interrupt_cb_t interrupt_callback = NULL;
    void *interrupt_user_ctx = NULL;
    interrupt_callback_snapshot(&interrupt_callback,
                                &interrupt_user_ctx);
    esp_err_t err = cst9217_bsp_deinit();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "CST9217 释放旧句柄失败，错误=0x%x", (unsigned)err);
        return err;
    }
    err = cst9217_bsp_init(bus_handle);
    if (err == ESP_OK && interrupt_callback != NULL)
    {
        err = cst9217_bsp_register_interrupt_callback(
            interrupt_callback,
            interrupt_user_ctx);
        if (err != ESP_OK)
        {
            const esp_err_t cleanup_error = cst9217_bsp_deinit();
            ESP_LOGE(TAG,
                     "CST9217 复位后中断恢复失败，已回收半初始化句柄，错误=0x%x，清理=0x%x",
                     (unsigned)err,
                     (unsigned)cleanup_error);
            const esp_err_t retry_init_error = cst9217_bsp_init(bus_handle);
            if (retry_init_error == ESP_OK)
            {
                err = cst9217_bsp_register_interrupt_callback(
                    interrupt_callback,
                    interrupt_user_ctx);
            }
            else
            {
                err = retry_init_error;
            }
            if (err != ESP_OK)
            {
                (void)cst9217_bsp_deinit();
                return err;
            }
        }
    }
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "CST9217 复位恢复成功，LVGL 输入设备无需重复注册");
    }
    return err;
}

bool cst9217_bsp_is_initialized(void)
{
    return s_initialized;
}

static esp_err_t create_touch(i2c_master_bus_handle_t bus_handle)
{
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_CST9217_CONFIG();
    /* 第三方配置宏未填写新 I2C 驱动必需的 SCL 频率，统一采用 BSP 总线频率。 */
    io_config.scl_speed_hz = LEGBOT_BSP_I2C_FREQ_HZ;
    esp_err_t err = esp_lcd_new_panel_io_i2c(bus_handle,
                                             &io_config,
                                             &s_touch_io);
    if (err != ESP_OK)
    {
        return err;
    }

    const esp_lcd_touch_config_t touch_config = {
        .x_max = CST9217_BSP_WIDTH,
        .y_max = CST9217_BSP_HEIGHT,
        .rst_gpio_num = LEGBOT_BSP_CST9217_RST_GPIO,
        .int_gpio_num = LEGBOT_BSP_CST9217_INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
    };
    return esp_lcd_touch_new_i2c_cst9217(s_touch_io, &touch_config, &s_touch);
}

static void touch_interrupt_bridge(esp_lcd_touch_handle_t touch)
{
    (void)touch;
    /* 芯片只承诺下降沿；GPIO 唤醒需低电平触发，先停中断，上层回调必须立即锁存边沿。 */
    (void)gpio_intr_disable(LEGBOT_BSP_CST9217_INT_GPIO);
    cst9217_bsp_interrupt_cb_t callback = NULL;
    void *user_ctx = NULL;
    portENTER_CRITICAL_ISR(&s_interrupt_lock);
    callback = s_interrupt_callback;
    user_ctx = s_interrupt_user_ctx;
    portEXIT_CRITICAL_ISR(&s_interrupt_lock);
    if (callback != NULL)
    {
        callback(user_ctx);
    }
}

static esp_err_t rearm_touch_interrupt(void)
{
    cst9217_bsp_interrupt_cb_t callback = NULL;
    void *user_ctx = NULL;
    interrupt_callback_snapshot(&callback, &user_ctx);
    (void)user_ctx;
    if (callback == NULL)
    {
        return ESP_OK;
    }
    esp_err_t err = gpio_wakeup_enable(
        LEGBOT_BSP_CST9217_INT_GPIO,
        GPIO_INTR_LOW_LEVEL);
    if (err == ESP_OK)
    {
        err = gpio_intr_enable(LEGBOT_BSP_CST9217_INT_GPIO);
    }
    return err;
}

static void interrupt_callback_snapshot(
    cst9217_bsp_interrupt_cb_t *callback,
    void **user_ctx)
{
    if (callback == NULL || user_ctx == NULL)
    {
        return;
    }
    portENTER_CRITICAL(&s_interrupt_lock);
    *callback = s_interrupt_callback;
    *user_ctx = s_interrupt_user_ctx;
    portEXIT_CRITICAL(&s_interrupt_lock);
}
