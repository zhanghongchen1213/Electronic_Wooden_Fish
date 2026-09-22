/**
 * @file     pvdf_bsp.h
 * @brief    PVDF 敲击前端 BSP 接口
 * @details  本模块是 IO9（PVDF_ADC / ADC1_CH8）与 IO11（PVDF_CMP_WAKE）的唯一固件所有者：
 *           比较器只负责 light sleep 逐脚唤醒与运行态上升沿中断，ADC 采样与二次确认判定
 *           由上层服务在任务上下文完成。除本模块外任何模块不得直调 ADC/GPIO 驱动。
 *           本头文件不暴露任何 esp_adc 类型，服务层只经这里的稳定 API 访问。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_PVDF_BSP_H
#define EWF_PVDF_BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** PVDF ADC 输入脚，来自权威 BSP 资源表。 */
#define EWF_PVDF_ADC_GPIO EWF_BSP_PVDF_ADC_GPIO
/** PVDF 比较器唤醒脚，来自权威 BSP 资源表。 */
#define EWF_PVDF_CMP_WAKE_GPIO EWF_BSP_PVDF_CMP_WAKE_GPIO

/**
 * 比较器输出有效极性：静止时开漏输出恒低，正向敲击使信号超过阈值后输出释放为高，
 * 因此 light sleep 逐脚唤醒与运行态中断都必须按高电平/上升沿配置。
 */
#define EWF_PVDF_CMP_WAKE_ACTIVE_LEVEL 1U

/** ADC 衰减档：设计值、未冻结，必须按 HW-OI-008 样机实测重新冻结。 */
#define EWF_PVDF_ADC_ATTEN_LEVEL 12U

/** 稳定 BSP 错误码前缀。 */
#define EWF_PVDF_BSP_ERROR_PREFIX "PVDF"

    /** BSP 稳定错误码；全部为可追溯的 ASCII 常量。 */
    typedef enum
    {
        EWF_PVDF_BSP_OK = 0,          /**< 无错误。 */
        EWF_PVDF_BSP_ERROR_INIT,      /**< 引脚或 ADC 单元初始化失败。 */
        EWF_PVDF_BSP_ERROR_IO_MAP,    /**< IO9 未解析为期望的 ADC1_CH8，链路降级。 */
        EWF_PVDF_BSP_ERROR_CALI,      /**< 校准不可用，降级为原始读数路径。 */
        EWF_PVDF_BSP_ERROR_RANGE,     /**< 采样读数触量程边界。 */
        EWF_PVDF_BSP_ERROR_TIMEOUT,   /**< 采样返回超时。 */
        EWF_PVDF_BSP_ERROR_READ,      /**< 其它读取失败。 */
        EWF_PVDF_BSP_ERROR_NOT_INIT,  /**< 链路尚未初始化。 */
        EWF_PVDF_BSP_ERROR_COUNT      /**< 稳定错误码数量，不是有效错误。 */
    } ewf_pvdf_bsp_error_t;

    /** 比较器唤醒脚的运行态 ISR 回调；实现只能调用 ISR-safe API。 */
    typedef void (*ewf_pvdf_bsp_isr_callback_t)(void *context);

    /** PVDF 链路当前可观测事实，供自检项按需读取。 */
    typedef struct
    {
        bool initialized;              /**< 引脚与 ADC 单元是否已初始化。 */
        bool io_mapping_consistent;    /**< IO9 是否解析为期望的 ADC1_CH8。 */
        bool calibration_available;    /**< 曲线拟合校准句柄是否可用。 */
        bool millivolt_ceiling_known;  /**< 是否已由校准曲线推出该衰减档的毫伏上限。 */
        bool degraded;                 /**< 是否处于降级路径（映射不一致或校准不可用）。 */
        int resolved_unit;             /**< 运行期解析出的 ADC 单元编号，0 表示 ADC1、1 表示 ADC2。 */
        int resolved_channel;          /**< 运行期解析出的 ADC 通道编号。 */
        int32_t millivolt_ceiling;     /**< 该衰减档校准后可测上限（mV），未知时为 0。 */
        int32_t raw_full_scale;        /**< 配置位宽的全量程原始码上限。 */
        uint8_t comparator_level;      /**< 比较器输出当前原始电平，不解释极性。 */
        bool wake_enabled;             /**< 逐脚 light sleep 唤醒是否已武装。 */
        bool isr_registered;           /**< 运行态上升沿中断是否已登记。 */
        ewf_pvdf_bsp_error_t last_error; /**< 最近一次稳定错误码。 */
    } pvdf_bsp_selfcheck_t;

    /**
     * @brief 初始化 PVDF 引脚与 ADC oneshot 单元
     * @details IO11 配置为上拉关闭的输入并同时挂运行态上升沿中断与 light sleep 高电平逐脚唤醒；
     *          IO9 用 adc_oneshot_io_to_channel() 在运行期解析通道并校验为期望的 ADC1_CH8。
     *          单元固定 ADC_UNIT_1（ADC2 与 Wi-Fi 共享，MVP 禁用）。
     * @return ESP_OK 成功或已初始化（含降级路径）
     *         其它 ESP-IDF 错误码表示引脚、ADC 单元或通道配置失败
     */
    esp_err_t pvdf_bsp_init(void);

    /**
     * @brief 读取一次 PVDF ADC 的校准毫伏值
     * @details 原始读数落在配置位宽的全量程边界时返回 ESP_ERR_INVALID_RESPONSE；读取超时返回
     *          ESP_ERR_TIMEOUT；校准后毫伏值越过该衰减档上限同样返回 ESP_ERR_INVALID_RESPONSE。
     *          本函数只允许在任务上下文调用，不得在 ISR 中使用。
     * @param out_millivolt 校准后的毫伏值输出，非 NULL
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE 链路未初始化或处于降级路径
     *         ESP_ERR_TIMEOUT ADC 读数超时，本次读数无效
     *         ESP_ERR_INVALID_RESPONSE 读数越界，本次读数无效
     *         其它 ESP-IDF 错误码表示驱动失败
     */
    esp_err_t pvdf_bsp_read_mv(int32_t *out_millivolt);

    /**
     * @brief 读取比较器输出当前原始电平
     * @param out_level 原始电平输出，非 NULL
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效，ESP_ERR_INVALID_STATE 尚未初始化
     */
    esp_err_t pvdf_bsp_comparator_level(uint8_t *out_level);

    /**
     * @brief 武装逐脚 light sleep 唤醒与运行态上升沿中断
     * @details 必须在 app_main 的全局 esp_sleep_enable_gpio_wakeup() 之前完成；
     *          ISR 停中断后由任务采样完成再经本入口重武装，避免电平保持期间的重复中断。
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 尚未初始化，其它值表示 GPIO 配置失败
     */
    esp_err_t pvdf_bsp_wake_enable(void);

    /**
     * @brief 关闭比较器唤醒与运行态中断
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 尚未初始化，其它值表示 GPIO 配置失败
     */
    esp_err_t pvdf_bsp_wake_disable(void);

    /**
     * @brief 为比较器唤醒脚登记唯一运行态 ISR 回调
     * @details 回调在中断上下文执行，只能取单调时间戳并投递 ISR-safe 消息；
     *          BSP 在调用回调前先停该脚中断，禁止在回调内读 ADC、打印或阻塞。
     * @param callback 中断上下文回调，非 NULL
     * @param context 原样传回给 callback 的上下文
     * @return ESP_OK 登记成功或已登记同一回调
     *         ESP_ERR_INVALID_ARG 回调为空
     *         ESP_ERR_INVALID_STATE 尚未初始化或已被其它所有者占用
     *         其它 ESP-IDF 错误码表示 GPIO ISR 登记失败
     */
    esp_err_t pvdf_bsp_isr_register(ewf_pvdf_bsp_isr_callback_t callback,
                                    void *context);

    /**
     * @brief 解注册比较器唤醒脚的运行态 ISR 回调
     * @details 先关闭该脚中断与唤醒再移除 handler，不卸载共享 GPIO ISR service。
     * @return ESP_OK 成功或原本未登记，ESP_ERR_INVALID_STATE 尚未初始化
     */
    esp_err_t pvdf_bsp_isr_unregister(void);

    /**
     * @brief 获取最近一次稳定 BSP 错误码文本
     * @return 形如 "PVDF-OK" / "PVDF-E04" 的稳定错误码；正常情况下始终非 NULL
     */
    const char *pvdf_bsp_error_code(void);

    /**
     * @brief 获取 PVDF 链路当前可观测事实
     * @param out 事实输出，非 NULL
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效
     */
    esp_err_t pvdf_bsp_selfcheck(pvdf_bsp_selfcheck_t *out);

#ifdef __cplusplus
}
#endif

#endif /* EWF_PVDF_BSP_H */
