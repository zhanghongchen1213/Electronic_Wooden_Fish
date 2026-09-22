/**
 * @file     key.c
 * @brief    板载按键 BSP 实现
 * @details  配置 LTC2954 PWR_INT 与 BOOT0 为上拉只读输入，并分别提供任意边沿 ISR 与 light-sleep 唤醒。
 * @author   ZHC
 * @date     2026-07-09
 */

#include "key.h"

#include <stdatomic.h>

#include "driver/gpio.h"

/** 按键 GPIO 是否已由 BSP 配置为输入。 */
static bool s_initialized;
/** PWR GPIO handler 是否已登记到共享 ISR service。 */
static bool s_pwr_handler_installed;
/** BOOT0 GPIO handler 是否已登记到共享 ISR service。 */
static bool s_boot_handler_installed;
/** 唯一上层 PWR ISR 回调。 */
static _Atomic(legbot_key_pwr_isr_callback_t) s_pwr_callback;
/** 与 PWR ISR 回调配对的上下文。 */
static _Atomic(void *) s_pwr_callback_context;
/** 唯一上层 BOOT0 ISR 回调。 */
static _Atomic(legbot_key_boot_isr_callback_t) s_boot_callback;
/** 与 BOOT0 ISR 回调配对的上下文。 */
static _Atomic(void *) s_boot_callback_context;

static void key_pwr_isr(void *argument);
static void key_boot_isr(void *argument);

esp_err_t key_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }
    const gpio_config_t pwr_config = {
        .pin_bit_mask = 1ULL << LEGBOT_KEY_PWR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&pwr_config);
    if (err != ESP_OK)
    {
        return err;
    }
    err = gpio_intr_disable(LEGBOT_KEY_PWR_GPIO);
    if (err != ESP_OK)
    {
        return err;
    }

    /* BOOT0 只配置运行态上拉输入，应用不改变下载模式硬件路径。 */
    const gpio_config_t boot_config = {
        .pin_bit_mask = 1ULL << LEGBOT_KEY_BOOT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&boot_config);
    if (err != ESP_OK)
    {
        return err;
    }
    err = gpio_intr_disable(LEGBOT_KEY_BOOT_GPIO);
    if (err == ESP_OK)
    {
        s_initialized = true;
    }
    return err;
}

esp_err_t key_register_pwr_isr_callback(
    legbot_key_pwr_isr_callback_t callback,
    void *context)
{
    if (callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const legbot_key_pwr_isr_callback_t registered =
        atomic_load_explicit(&s_pwr_callback, memory_order_acquire);
    if (registered != NULL)
    {
        return registered == callback &&
                       atomic_load_explicit(&s_pwr_callback_context,
                                            memory_order_relaxed) == context
                   ? ESP_OK
                   : ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = gpio_intr_disable(LEGBOT_KEY_PWR_GPIO);
    if (err != ESP_OK)
    {
        return err;
    }
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }
    atomic_store_explicit(&s_pwr_callback_context,
                          context,
                          memory_order_relaxed);
    atomic_store_explicit(&s_pwr_callback,
                          callback,
                          memory_order_release);
    err = gpio_isr_handler_add(LEGBOT_KEY_PWR_GPIO,
                               key_pwr_isr,
                               NULL);
    if (err != ESP_OK)
    {
        atomic_store_explicit(&s_pwr_callback, NULL, memory_order_release);
        atomic_store_explicit(&s_pwr_callback_context,
                              NULL,
                              memory_order_relaxed);
        return err;
    }
    s_pwr_handler_installed = true;
    err = key_rearm_pwr_isr_for_next_level();
    if (err != ESP_OK)
    {
        (void)key_unregister_pwr_isr_callback();
    }
    return err;
}

esp_err_t key_unregister_pwr_isr_callback(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t first_error = gpio_intr_disable(LEGBOT_KEY_PWR_GPIO);
    const esp_err_t wake_error = gpio_wakeup_disable(LEGBOT_KEY_PWR_GPIO);
    if (first_error == ESP_OK)
    {
        first_error = wake_error;
    }
    atomic_store_explicit(&s_pwr_callback, NULL, memory_order_release);
    atomic_store_explicit(&s_pwr_callback_context,
                          NULL,
                          memory_order_relaxed);
    if (s_pwr_handler_installed)
    {
        const esp_err_t remove_error =
            gpio_isr_handler_remove(LEGBOT_KEY_PWR_GPIO);
        if (first_error == ESP_OK)
        {
            first_error = remove_error;
        }
        s_pwr_handler_installed = false;
    }
    return first_error;
}

esp_err_t key_rearm_pwr_isr_for_next_level(void)
{
    if (!s_initialized || !s_pwr_handler_installed)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = gpio_intr_disable(LEGBOT_KEY_PWR_GPIO);
    if (err != ESP_OK)
    {
        return err;
    }
    /* light-sleep 期间按有效低电平武装逐脚唤醒；运行态仍用任意边沿中断测时长。
       两条链路是不同机制，不得互相替代（见 docs/embedded/guides/低功耗策略与实测验收.md）。 */
    err = gpio_wakeup_enable(LEGBOT_KEY_PWR_GPIO, GPIO_INTR_LOW_LEVEL);
    if (err == ESP_OK)
    {
        /* PWR_INT 是持续低有效信号；只捕获下降/上升边沿，低电平时长由任务测量。 */
        err = gpio_set_intr_type(LEGBOT_KEY_PWR_GPIO, GPIO_INTR_ANYEDGE);
    }
    if (err == ESP_OK)
    {
        err = gpio_intr_enable(LEGBOT_KEY_PWR_GPIO);
    }
    return err;
}

esp_err_t key_register_boot_isr_callback(
    legbot_key_boot_isr_callback_t callback,
    void *context)
{
    if (callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const legbot_key_boot_isr_callback_t registered =
        atomic_load_explicit(&s_boot_callback, memory_order_acquire);
    if (registered != NULL)
    {
        return registered == callback &&
                       atomic_load_explicit(&s_boot_callback_context,
                                            memory_order_relaxed) == context
                   ? ESP_OK
                   : ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = gpio_intr_disable(LEGBOT_KEY_BOOT_GPIO);
    if (err != ESP_OK)
    {
        return err;
    }
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }
    atomic_store_explicit(&s_boot_callback_context,
                          context,
                          memory_order_relaxed);
    atomic_store_explicit(&s_boot_callback,
                          callback,
                          memory_order_release);
    err = gpio_isr_handler_add(LEGBOT_KEY_BOOT_GPIO,
                               key_boot_isr,
                               NULL);
    if (err != ESP_OK)
    {
        atomic_store_explicit(&s_boot_callback, NULL, memory_order_release);
        atomic_store_explicit(&s_boot_callback_context,
                              NULL,
                              memory_order_relaxed);
        return err;
    }
    s_boot_handler_installed = true;
    err = key_rearm_boot_isr_for_next_level();
    if (err != ESP_OK)
    {
        (void)key_unregister_boot_isr_callback();
    }
    return err;
}

esp_err_t key_unregister_boot_isr_callback(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t first_error = gpio_intr_disable(LEGBOT_KEY_BOOT_GPIO);
    const esp_err_t wake_error = gpio_wakeup_disable(LEGBOT_KEY_BOOT_GPIO);
    if (first_error == ESP_OK)
    {
        first_error = wake_error;
    }
    atomic_store_explicit(&s_boot_callback, NULL, memory_order_release);
    atomic_store_explicit(&s_boot_callback_context,
                          NULL,
                          memory_order_relaxed);
    if (s_boot_handler_installed)
    {
        const esp_err_t remove_error =
            gpio_isr_handler_remove(LEGBOT_KEY_BOOT_GPIO);
        if (first_error == ESP_OK)
        {
            first_error = remove_error;
        }
        s_boot_handler_installed = false;
    }
    return first_error;
}

esp_err_t key_rearm_boot_isr_for_next_level(void)
{
    if (!s_initialized || !s_boot_handler_installed)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = gpio_intr_disable(LEGBOT_KEY_BOOT_GPIO);
    if (err != ESP_OK)
    {
        return err;
    }
    /* light-sleep 期间按有效低电平武装逐脚唤醒；运行态仍只交接原始边沿。 */
    err = gpio_wakeup_enable(LEGBOT_KEY_BOOT_GPIO, GPIO_INTR_LOW_LEVEL);
    if (err == ESP_OK)
    {
        /* BOOT0 运行态只交接原始边沿，绝不改变 Boot ROM 的启动采样。 */
        err = gpio_set_intr_type(LEGBOT_KEY_BOOT_GPIO, GPIO_INTR_ANYEDGE);
    }
    if (err == ESP_OK)
    {
        err = gpio_intr_enable(LEGBOT_KEY_BOOT_GPIO);
    }
    return err;
}

esp_err_t key_read_state(legbot_key_state_t *state)
{
    if (state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    state->pwr_level = (uint8_t)gpio_get_level(LEGBOT_KEY_PWR_GPIO);
    state->boot_level = (uint8_t)gpio_get_level(LEGBOT_KEY_BOOT_GPIO);
    return ESP_OK;
}

gpio_num_t key_gpio(legbot_key_id_t key)
{
    /* 非法按键统一返回 GPIO_NUM_NC，避免上层误操作真实引脚。 */
    switch (key)
    {
    case LEGBOT_KEY_PWR:
        return LEGBOT_KEY_PWR_GPIO;
    case LEGBOT_KEY_BOOT:
        return LEGBOT_KEY_BOOT_GPIO;
    default:
        return GPIO_NUM_NC;
    }
}

static void key_pwr_isr(void *argument)
{
    (void)argument;
    /* 电平保持期间先停中断，owner 采样后按相反电平重武装。 */
    (void)gpio_intr_disable(LEGBOT_KEY_PWR_GPIO);
    const legbot_key_pwr_isr_callback_t callback =
        atomic_load_explicit(&s_pwr_callback, memory_order_acquire);
    if (callback != NULL)
    {
        callback(atomic_load_explicit(&s_pwr_callback_context,
                                      memory_order_relaxed));
    }
}

static void key_boot_isr(void *argument)
{
    (void)argument;
    /* 电平保持期间先停中断，owner 采样后按任意边沿重武装。 */
    (void)gpio_intr_disable(LEGBOT_KEY_BOOT_GPIO);
    const legbot_key_boot_isr_callback_t callback =
        atomic_load_explicit(&s_boot_callback, memory_order_acquire);
    if (callback != NULL)
    {
        callback(atomic_load_explicit(&s_boot_callback_context,
                                      memory_order_relaxed));
    }
}
