/**
 * @file     pvdf_bsp.c
 * @brief    PVDF 敲击前端 BSP 实现
 * @details  初始化 IO9（PVDF_ADC）的 ADC oneshot 单元与校准句柄，并把 IO11（PVDF_CMP_WAKE）
 *           同时挂上 light sleep 逐脚高电平唤醒与运行态上升沿中断。ISR 只停中断并转交上层，
 *           ADC 读取与判定全部由服务任务完成。本模块不推进任何累计、不保存任何波形。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "pvdf_bsp.h"

#include <stdatomic.h>

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "soc/soc_caps.h"

static const char *TAG = "BSP_PVDF";

/** ADC 衰减档：12 dB，对应 EWF_PVDF_ADC_ATTEN_LEVEL。设计值、未冻结，需按 HW-OI-008 样机实测重标。 */
#define EWF_PVDF_BSP_ADC_ATTEN ADC_ATTEN_DB_12
/** ADC 位宽：使用驱动默认位宽，原始码全量程由 SOC_ADC_DIGI_MAX_BITWIDTH 运行期推出。 */
#define EWF_PVDF_BSP_ADC_BITWIDTH ADC_BITWIDTH_DEFAULT

/** 期望的 ADC 单元：PVDF 固定 ADC1；ADC2 与 Wi-Fi 共享，MVP 禁用。 */
#define EWF_PVDF_BSP_EXPECTED_ADC_UNIT ADC_UNIT_1
/** 期望的 ADC 通道：硬件事实源固定 PVDF_ADC = IO9 = ADC1_CH8，仅用于校验解析结果。 */
#define EWF_PVDF_BSP_EXPECTED_ADC_CHANNEL ADC_CHANNEL_8

/** light sleep 逐脚唤醒极性：正向敲击使开漏比较器输出释放为高，故为高电平有效。 */
#define EWF_PVDF_BSP_WAKE_INTR_TYPE GPIO_INTR_HIGH_LEVEL
/** 运行态中断极性：只捕获比较器输出的上升沿，不得使用下降沿或低电平。 */
#define EWF_PVDF_BSP_RUNTIME_INTR_TYPE GPIO_INTR_POSEDGE

/**
 * 校准不可用时的降级换算满量程（mV）。
 * 这是 ESP32-S3 ADC 12 dB 衰减档的名义满量程设计值、未冻结，必须按 HW-OI-008 样机实测重标；
 * 它只在降级路径参与越界上限判定，不参与比较器阈值判定。
 */
#define EWF_PVDF_BSP_FALLBACK_FULL_SCALE_MV 3300

_Static_assert(EWF_PVDF_BSP_ADC_ATTEN == ADC_ATTEN_DB_12,
               "PVDF ADC attenuation must stay at the 12 dB design value, which is "
               "unfrozen and must be re-frozen from HW-OI-008 prototype measurements.");

/** 逐脚唤醒与运行态中断必须是两条独立链路，且极性为高电平有效。 */
_Static_assert(EWF_PVDF_BSP_WAKE_INTR_TYPE == GPIO_INTR_HIGH_LEVEL,
               "PVDF comparator wake must be active high.");
_Static_assert(EWF_PVDF_BSP_RUNTIME_INTR_TYPE == GPIO_INTR_POSEDGE,
               "PVDF runtime interrupt must trigger on the rising edge.");

/** 链路是否已由本模块初始化。 */
static bool s_initialized;
/** ADC oneshot 单元句柄；降级或未初始化时为 NULL。 */
static adc_oneshot_unit_handle_t s_adc_unit;
/** ADC 校准句柄；校准不可用时为 NULL 并退化为原始读数路径。 */
static adc_cali_handle_t s_cali;
/** 运行期由 adc_oneshot_io_to_channel() 解析出的通道。 */
static adc_channel_t s_channel;
/** IO9 是否解析为期望的 ADC1_CH8。 */
static bool s_io_mapping_consistent;
/** 运行期解析出的 ADC 单元编号，用于自检可观测事实。 */
static int s_resolved_unit;
/** 运行期解析出的 ADC 通道编号，用于自检可观测事实。 */
static int s_resolved_channel;
/** 配置位宽的全量程原始码上限。 */
static int32_t s_raw_full_scale;
/** 该衰减档校准后可测上限（mV）。 */
static int32_t s_millivolt_ceiling;
/** 是否已由校准曲线推出毫伏上限。 */
static bool s_millivolt_ceiling_known;
/** 校准句柄是否可用。 */
static bool s_calibration_available;
/** ISR 当前是否已登记。 */
static bool s_isr_registered;
/** 逐脚唤醒是否已武装。 */
static bool s_wake_enabled;
/** 最近一次稳定错误码。 */
static ewf_pvdf_bsp_error_t s_last_error;
/** 唯一上层 ISR 回调。 */
static _Atomic(ewf_pvdf_bsp_isr_callback_t) s_isr_callback;
/** 与 ISR 回调配对的上下文。 */
static _Atomic(void *) s_isr_context;

static esp_err_t set_error(ewf_pvdf_bsp_error_t code, esp_err_t err);
static esp_err_t configure_adc_unit(void);
static esp_err_t create_calibration(void);
static void resolve_channel(void);
static void pvdf_bsp_isr(void *argument);

esp_err_t pvdf_bsp_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    s_raw_full_scale = (int32_t)((1U << (unsigned)SOC_ADC_DIGI_MAX_BITWIDTH) - 1U);

    /* IO11 是开漏比较器输出，外部已有 10 kΩ 上拉，固件不再叠加内部上拉。 */
    const gpio_config_t wake_config = {
        .pin_bit_mask = 1ULL << EWF_PVDF_CMP_WAKE_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = EWF_PVDF_BSP_RUNTIME_INTR_TYPE,
    };
    esp_err_t err = gpio_config(&wake_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF 比较器 IO%d 配置失败，错误=%s",
                 (int)EWF_PVDF_CMP_WAKE_GPIO, esp_err_to_name(err));
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }
    /* handler 登记前先停中断，避免未登记的边沿进入默认 ISR。 */
    err = gpio_intr_disable(EWF_PVDF_CMP_WAKE_GPIO);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF 比较器 IO%d 中断禁用失败，错误=%s",
                 (int)EWF_PVDF_CMP_WAKE_GPIO, esp_err_to_name(err));
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }
    /* light sleep 期间按有效高电平武装逐脚唤醒；与运行态上升沿中断是两条独立链路。
       本调用发生在板级引脚阶段，严格早于 app_main 的全局 esp_sleep_enable_gpio_wakeup()。 */
    err = gpio_wakeup_enable(EWF_PVDF_CMP_WAKE_GPIO, EWF_PVDF_BSP_WAKE_INTR_TYPE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF 比较器 IO%d 逐脚唤醒武装失败，错误=%s",
                 (int)EWF_PVDF_CMP_WAKE_GPIO, esp_err_to_name(err));
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }
    s_wake_enabled = true;

    err = configure_adc_unit();
    if (err != ESP_OK)
    {
        return err;
    }
    s_initialized = true;
    ESP_LOGI(TAG,
             "PVDF 前端已纳管：ADC=IO%d，通道=%d/%d，原始量程上限=%ld，比较器唤醒=IO%d 高电平有效；"
             "电气阈值与滞回未冻结，需按 HW-OI-008 样机实测",
             (int)EWF_PVDF_ADC_GPIO,
             s_resolved_unit,
             s_resolved_channel,
             (long)s_raw_full_scale,
             (int)EWF_PVDF_CMP_WAKE_GPIO);
    return ESP_OK;
}

esp_err_t pvdf_bsp_read_mv(int32_t *out_millivolt)
{
    if (out_millivolt == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized || s_adc_unit == NULL || !s_io_mapping_consistent)
    {
        return set_error(EWF_PVDF_BSP_ERROR_NOT_INIT, ESP_ERR_INVALID_STATE);
    }

    int raw = 0;
    esp_err_t err = adc_oneshot_read(s_adc_unit, s_channel, &raw);
    if (err == ESP_ERR_TIMEOUT)
    {
        return set_error(EWF_PVDF_BSP_ERROR_TIMEOUT, ESP_ERR_TIMEOUT);
    }
    if (err != ESP_OK)
    {
        return set_error(EWF_PVDF_BSP_ERROR_READ, err);
    }
    /* 只判上限：读数触配置位宽的全量程边界时判为越界，不重试、不放大、不硬扛。
       低轨读数不是越界：静止时 PVDF_RAW 经泄放电阻趋近 0 V，负半周被单电源缓冲钳在 0 V，
       因此窗口内出现接近 0 的读数是设计上的正常情形，必须照常进入下游峰值与候选判定。 */
    if (raw >= s_raw_full_scale)
    {
        return set_error(EWF_PVDF_BSP_ERROR_RANGE, ESP_ERR_INVALID_RESPONSE);
    }

    int32_t millivolt = 0;
    if (s_cali != NULL)
    {
        int calibrated = 0;
        err = adc_cali_raw_to_voltage(s_cali, raw, &calibrated);
        if (err != ESP_OK)
        {
            return set_error(EWF_PVDF_BSP_ERROR_READ, err);
        }
        millivolt = (int32_t)calibrated;
    }
    else
    {
        millivolt = ((int32_t)raw * EWF_PVDF_BSP_FALLBACK_FULL_SCALE_MV) /
                    s_raw_full_scale;
    }
    /* 校准后毫伏值必须落在该衰减档可测上限内。 */
    if (s_millivolt_ceiling_known && millivolt >= s_millivolt_ceiling)
    {
        return set_error(EWF_PVDF_BSP_ERROR_RANGE, ESP_ERR_INVALID_RESPONSE);
    }

    s_last_error = EWF_PVDF_BSP_OK;
    *out_millivolt = millivolt;
    return ESP_OK;
}

esp_err_t pvdf_bsp_comparator_level(uint8_t *out_level)
{
    if (out_level == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *out_level = (uint8_t)gpio_get_level(EWF_PVDF_CMP_WAKE_GPIO);
    return ESP_OK;
}

esp_err_t pvdf_bsp_wake_enable(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = gpio_intr_disable(EWF_PVDF_CMP_WAKE_GPIO);
    if (err != ESP_OK)
    {
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }
    err = gpio_wakeup_enable(EWF_PVDF_CMP_WAKE_GPIO, EWF_PVDF_BSP_WAKE_INTR_TYPE);
    if (err == ESP_OK)
    {
        err = gpio_set_intr_type(EWF_PVDF_CMP_WAKE_GPIO,
                                 EWF_PVDF_BSP_RUNTIME_INTR_TYPE);
    }
    if (err == ESP_OK && s_isr_registered)
    {
        err = gpio_intr_enable(EWF_PVDF_CMP_WAKE_GPIO);
    }
    if (err != ESP_OK)
    {
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }
    s_wake_enabled = true;
    return ESP_OK;
}

esp_err_t pvdf_bsp_wake_disable(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t first_error = gpio_intr_disable(EWF_PVDF_CMP_WAKE_GPIO);
    const esp_err_t wake_error = gpio_wakeup_disable(EWF_PVDF_CMP_WAKE_GPIO);
    if (first_error == ESP_OK)
    {
        first_error = wake_error;
    }
    if (first_error != ESP_OK)
    {
        return set_error(EWF_PVDF_BSP_ERROR_INIT, first_error);
    }
    s_wake_enabled = false;
    return ESP_OK;
}

esp_err_t pvdf_bsp_isr_register(ewf_pvdf_bsp_isr_callback_t callback,
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
    const ewf_pvdf_bsp_isr_callback_t registered =
        atomic_load_explicit(&s_isr_callback, memory_order_acquire);
    if (registered != NULL)
    {
        return registered == callback &&
                       atomic_load_explicit(&s_isr_context,
                                            memory_order_relaxed) == context
                   ? ESP_OK
                   : ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = gpio_intr_disable(EWF_PVDF_CMP_WAKE_GPIO);
    if (err != ESP_OK)
    {
        return err;
    }
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }
    atomic_store_explicit(&s_isr_context, context, memory_order_relaxed);
    atomic_store_explicit(&s_isr_callback, callback, memory_order_release);
    err = gpio_isr_handler_add(EWF_PVDF_CMP_WAKE_GPIO, pvdf_bsp_isr, NULL);
    if (err != ESP_OK)
    {
        atomic_store_explicit(&s_isr_callback, NULL, memory_order_release);
        atomic_store_explicit(&s_isr_context, NULL, memory_order_relaxed);
        return err;
    }
    s_isr_registered = true;
    err = pvdf_bsp_wake_enable();
    if (err != ESP_OK)
    {
        (void)pvdf_bsp_isr_unregister();
    }
    return err;
}

esp_err_t pvdf_bsp_isr_unregister(void)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t disable_error = pvdf_bsp_wake_disable();
    atomic_store_explicit(&s_isr_callback, NULL, memory_order_release);
    atomic_store_explicit(&s_isr_context, NULL, memory_order_relaxed);
    esp_err_t first_error = disable_error;
    if (s_isr_registered)
    {
        const esp_err_t remove_error =
            gpio_isr_handler_remove(EWF_PVDF_CMP_WAKE_GPIO);
        s_isr_registered = false;
        if (first_error == ESP_OK)
        {
            first_error = remove_error;
        }
    }
    return first_error;
}

const char *pvdf_bsp_error_code(void)
{
    switch (s_last_error)
    {
    case EWF_PVDF_BSP_OK:
        return EWF_PVDF_BSP_ERROR_PREFIX "-OK";
    case EWF_PVDF_BSP_ERROR_INIT:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E01";
    case EWF_PVDF_BSP_ERROR_IO_MAP:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E02";
    case EWF_PVDF_BSP_ERROR_CALI:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E03";
    case EWF_PVDF_BSP_ERROR_RANGE:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E04";
    case EWF_PVDF_BSP_ERROR_TIMEOUT:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E05";
    case EWF_PVDF_BSP_ERROR_READ:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E06";
    case EWF_PVDF_BSP_ERROR_NOT_INIT:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E07";
    default:
        return EWF_PVDF_BSP_ERROR_PREFIX "-E00";
    }
}

esp_err_t pvdf_bsp_selfcheck(pvdf_bsp_selfcheck_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    out->initialized = s_initialized;
    out->io_mapping_consistent = s_io_mapping_consistent;
    out->calibration_available = s_calibration_available;
    out->millivolt_ceiling_known = s_millivolt_ceiling_known;
    out->degraded = s_initialized &&
                    (!s_io_mapping_consistent || !s_calibration_available);
    out->resolved_unit = s_resolved_unit;
    out->resolved_channel = s_resolved_channel;
    out->millivolt_ceiling = s_millivolt_ceiling_known ? s_millivolt_ceiling : 0;
    out->raw_full_scale = s_raw_full_scale;
    out->comparator_level = 0U;
    out->wake_enabled = s_wake_enabled;
    out->isr_registered = s_isr_registered;
    out->last_error = s_last_error;
    (void)pvdf_bsp_comparator_level(&out->comparator_level);
    return ESP_OK;
}

static esp_err_t set_error(ewf_pvdf_bsp_error_t code, esp_err_t err)
{
    s_last_error = code;
    return err;
}

static esp_err_t configure_adc_unit(void)
{
    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = EWF_PVDF_BSP_EXPECTED_ADC_UNIT,
        /* 置 0 表示回落到驱动默认 ADC 时钟源，不在此处发明时钟树配置。 */
        .clk_src = 0,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_config, &s_adc_unit);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF ADC1 oneshot 单元创建失败，错误=%s", esp_err_to_name(err));
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }

    resolve_channel();
    if (!s_io_mapping_consistent)
    {
        /* 通道不得硬编码：解析结果与硬件事实源不一致时降级，不配置任何通道。 */
        ESP_LOGE(TAG,
                 "PVDF ADC 引脚映射不一致：IO%d 解析为 单元=%d/通道=%d，期望 单元=%d/通道=%d；"
                 "链路降级，稳定错误码=%s",
                 (int)EWF_PVDF_ADC_GPIO,
                 s_resolved_unit,
                 s_resolved_channel,
                 (int)EWF_PVDF_BSP_EXPECTED_ADC_UNIT,
                 (int)EWF_PVDF_BSP_EXPECTED_ADC_CHANNEL,
                 EWF_PVDF_BSP_ERROR_PREFIX "-E02");
        return set_error(EWF_PVDF_BSP_ERROR_IO_MAP, ESP_OK);
    }

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = EWF_PVDF_BSP_ADC_ATTEN,
        .bitwidth = EWF_PVDF_BSP_ADC_BITWIDTH,
    };
    err = adc_oneshot_config_channel(s_adc_unit, s_channel, &channel_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "PVDF ADC 通道配置失败，错误=%s", esp_err_to_name(err));
        return set_error(EWF_PVDF_BSP_ERROR_INIT, err);
    }

    err = create_calibration();
    if (err != ESP_OK)
    {
        /* 校准不可用时降级为原始读数路径并显式记中文日志，不得静默通过。 */
        ESP_LOGW(TAG,
                 "PVDF ADC 校准不可用，降级为原始读数换算路径，稳定错误码=%s",
                 EWF_PVDF_BSP_ERROR_PREFIX "-E03");
        s_last_error = EWF_PVDF_BSP_ERROR_CALI;
    }
    return ESP_OK;
}

static esp_err_t create_calibration(void)
{
    adc_cali_scheme_ver_t scheme_mask = 0;
    esp_err_t err = adc_cali_check_scheme(&scheme_mask);
    if (err != ESP_OK)
    {
        s_calibration_available = false;
        return set_error(EWF_PVDF_BSP_ERROR_CALI, err);
    }
    if ((scheme_mask & ADC_CALI_SCHEME_VER_CURVE_FITTING) == 0)
    {
        s_calibration_available = false;
        return set_error(EWF_PVDF_BSP_ERROR_CALI, ESP_ERR_NOT_SUPPORTED);
    }

    const adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = EWF_PVDF_BSP_EXPECTED_ADC_UNIT,
        .chan = s_channel,
        .atten = EWF_PVDF_BSP_ADC_ATTEN,
        .bitwidth = EWF_PVDF_BSP_ADC_BITWIDTH,
    };
    err = adc_cali_create_scheme_curve_fitting(&cali_config, &s_cali);
    if (err != ESP_OK)
    {
        s_cali = NULL;
        s_calibration_available = false;
        return set_error(EWF_PVDF_BSP_ERROR_CALI, err);
    }
    s_calibration_available = true;

    /* 越界上限必须来自运行期可获得的量程，不得凭记忆写死电压数字。 */
    int ceiling = 0;
    if (adc_cali_raw_to_voltage(s_cali, s_raw_full_scale, &ceiling) == ESP_OK &&
        ceiling > 0)
    {
        s_millivolt_ceiling = (int32_t)ceiling;
        s_millivolt_ceiling_known = true;
    }
    return ESP_OK;
}

static void resolve_channel(void)
{
    adc_unit_t unit = EWF_PVDF_BSP_EXPECTED_ADC_UNIT;
    adc_channel_t channel = ADC_CHANNEL_0;
    const esp_err_t err =
        adc_oneshot_io_to_channel((int)EWF_PVDF_ADC_GPIO, &unit, &channel);
    if (err != ESP_OK)
    {
        s_resolved_unit = -1;
        s_resolved_channel = -1;
        s_io_mapping_consistent = false;
        return;
    }
    s_resolved_unit = (int)unit;
    s_resolved_channel = (int)channel;
    s_io_mapping_consistent = unit == EWF_PVDF_BSP_EXPECTED_ADC_UNIT &&
                              channel == EWF_PVDF_BSP_EXPECTED_ADC_CHANNEL;
}

static void pvdf_bsp_isr(void *argument)
{
    (void)argument;
    /* 比较器输出在敲击期间保持高：先停中断，owner 采样完成后再经 wake_enable 重武装。 */
    (void)gpio_intr_disable(EWF_PVDF_CMP_WAKE_GPIO);
    const ewf_pvdf_bsp_isr_callback_t callback =
        atomic_load_explicit(&s_isr_callback, memory_order_acquire);
    if (callback != NULL)
    {
        callback(atomic_load_explicit(&s_isr_context, memory_order_relaxed));
    }
}
