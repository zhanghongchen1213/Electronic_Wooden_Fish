/**
 * @file     bsp_resources.h
 * @brief    电子木鱼 EWF 权威 BSP 资源表
 * @details  集中定义板级 GPIO、总线实例、初始化阶段和资源归属，禁止业务模块重复硬编码板级资源。
 *           事实源：docs/hardware/电子木鱼-硬件原理图设计基线.md#2.1 与 docs/hardware/电源网络命名规范.md#3。
 *           保留的通用 legbot 驱动（CO5300/CST9217/CW2015/ES8311/NS4150B/QMI8658C）只保留板级映射与所有权，
 *           其完整业务行为由对应 Story 负责。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef LEGBOT_BSP_RESOURCES_H
#define LEGBOT_BSP_RESOURCES_H

#include <stdbool.h>
#include <stddef.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** BSP 稳定错误码和模块名的公共前缀。 */
#define LEGBOT_BSP_MODULE_PREFIX "BSP"

/** 项目共享 I2C 总线端口。 */
#define LEGBOT_BSP_I2C_PORT I2C_NUM_0
/** 项目共享 I2C SDA 引脚。 */
#define LEGBOT_BSP_I2C_SDA_GPIO GPIO_NUM_1
/** 项目共享 I2C SCL 引脚。 */
#define LEGBOT_BSP_I2C_SCL_GPIO GPIO_NUM_2
/** 项目共享 I2C 时钟频率。 */
#define LEGBOT_BSP_I2C_FREQ_HZ 400000
/** 项目共享 I2C 单次设备事务超时。 */
#define LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS 20

/** QMI8658C INT1 中断引脚。 */
#define LEGBOT_BSP_QMI8658C_INT1_GPIO GPIO_NUM_41
/** QMI8658C INT2 未分配；权威 GPIO 总表禁止占用 IO45。 */
#define LEGBOT_BSP_QMI8658C_INT2_GPIO GPIO_NUM_NC

/** CST9217 触摸复位引脚。 */
#define LEGBOT_BSP_CST9217_RST_GPIO GPIO_NUM_38
/** CST9217 触摸中断引脚。 */
#define LEGBOT_BSP_CST9217_INT_GPIO GPIO_NUM_39

/** CO5300 屏幕 SPI Host。 */
#define LEGBOT_BSP_DISPLAY_SPI_HOST SPI3_HOST
/** CO5300 片选引脚。 */
#define LEGBOT_BSP_DISPLAY_CS_GPIO GPIO_NUM_40
/** CO5300 SPI 时钟引脚。 */
#define LEGBOT_BSP_DISPLAY_SCL_GPIO GPIO_NUM_5
/** CO5300 数据线 D0。 */
#define LEGBOT_BSP_DISPLAY_D0_GPIO GPIO_NUM_6
/** CO5300 数据线 D1。 */
#define LEGBOT_BSP_DISPLAY_D1_GPIO GPIO_NUM_7
/** CO5300 数据线 D2。 */
#define LEGBOT_BSP_DISPLAY_D2_GPIO GPIO_NUM_12
/** CO5300 数据线 D3。 */
#define LEGBOT_BSP_DISPLAY_D3_GPIO GPIO_NUM_42
/** CO5300 复位引脚。 */
#define LEGBOT_BSP_DISPLAY_RST_GPIO GPIO_NUM_4
/** CO5300 使能引脚。 */
#define LEGBOT_BSP_DISPLAY_EN_GPIO GPIO_NUM_47

/** ES8311 I2S 端口号。 */
#define LEGBOT_BSP_ES8311_I2S_PORT 0
/** ES8311 I2S 数据输出引脚。 */
#define LEGBOT_BSP_ES8311_I2S_DO_GPIO GPIO_NUM_13
/** ES8311 I2S 帧同步引脚。 */
#define LEGBOT_BSP_ES8311_I2S_WS_GPIO GPIO_NUM_14
/** ES8311 I2S 数据输入引脚。 */
#define LEGBOT_BSP_ES8311_I2S_DI_GPIO GPIO_NUM_17
/** ES8311 I2S 位时钟引脚。 */
#define LEGBOT_BSP_ES8311_I2S_BCLK_GPIO GPIO_NUM_18
/** ES8311 I2S 主时钟引脚。 */
#define LEGBOT_BSP_ES8311_I2S_MCLK_GPIO GPIO_NUM_21

/** NS4150 功放使能引脚。 */
#define LEGBOT_BSP_NS4150_PA_EN_GPIO GPIO_NUM_48
/** NS4150B 数据手册规定 CTRL 高电平进入工作模式。 */
#define LEGBOT_BSP_NS4150_PA_ACTIVE_LEVEL 1U
/** NS4150B 数据手册规定 CTRL 低电平进入 shutdown。 */
#define LEGBOT_BSP_NS4150_PA_INACTIVE_LEVEL 0U

/** LTC2954 PWR_INT：开漏低有效，10 kΩ 上拉至 V3V3，按住期间持续为低。 */
#define EWF_BSP_PWR_INT_GPIO GPIO_NUM_8
/** PWR_INT 有效电平，0 表示按下期间为低。 */
#define EWF_BSP_PWR_INT_ACTIVE_LEVEL 0U
/** BOOT0 启动绑带：下载时为低，正常 SPI 启动为高。 */
#define EWF_BSP_BOOT0_GPIO GPIO_NUM_0
/** BOOT0 运行态按下电平。 */
#define EWF_BSP_BOOT0_ACTIVE_LEVEL 0U

/** EN/RESET_N 是独立硬件复位输入/测试点，不接 ESP32 GPIO，也不由应用驱动。 */
#define EWF_BSP_EN_RESET_N_GPIO GPIO_NUM_NC
/** LTC2954 PWR_STATE(EN) 只驱动 TPS22965/TLV62569，不接 ESP32 GPIO。 */
#define EWF_BSP_PWR_STATE_LTC2954_EN_GPIO GPIO_NUM_NC
/** LTC2954 KILL(pin 8) 只按硬件基线端接到 V3V3，固件不得驱动。 */
#define EWF_BSP_KILL_LTC2954_GPIO GPIO_NUM_NC

/** PVDF 敲击检测 ADC 输入脚，归 components/BSP/PVDF 独占。 */
#define EWF_BSP_PVDF_ADC_GPIO GPIO_NUM_9
/** PVDF 比较器唤醒脚，归 components/BSP/PVDF 独占。 */
#define EWF_BSP_PVDF_CMP_WAKE_GPIO GPIO_NUM_11

/** IO46 未分配/悬空启动绑带保留脚，固件不得初始化。 */
#define EWF_BSP_IO46_RESERVED_GPIO GPIO_NUM_46
/** IO45 未分配保留脚，固件不得初始化，也不得接 QMI8658A INT2。 */
#define EWF_BSP_IO45_RESERVED_GPIO GPIO_NUM_45

/** USB-Serial-JTAG D- 引脚，Story 1.3 独占。 */
#define EWF_BSP_USB_SERIAL_JTAG_DM_GPIO GPIO_NUM_19
/** USB-Serial-JTAG D+ 引脚，Story 1.3 独占。 */
#define EWF_BSP_USB_SERIAL_JTAG_DP_GPIO GPIO_NUM_20

/** Air780EGP 模块 UART 端口。 */
#define EWF_BSP_AIR780EGP_UART_PORT UART_NUM_1
/** Air780EGP UART TX 引脚。 */
#define EWF_BSP_AIR780EGP_TX_GPIO GPIO_NUM_43
/** Air780EGP UART RX 引脚。 */
#define EWF_BSP_AIR780EGP_RX_GPIO GPIO_NUM_44
/** Air780EGP DTR 引脚，仅保留资源所有权。 */
#define EWF_BSP_AIR780EGP_DTR_GPIO GPIO_NUM_10
/** Air780EGP RST 引脚，仅保留资源所有权。 */
#define EWF_BSP_AIR780EGP_RST_GPIO GPIO_NUM_15
/** Air780EGP UART 波特率。 */
#define EWF_BSP_AIR780EGP_UART_BAUD 115200

/** SDMMC 策略，0 表示当前硬件配置禁用 SDMMC。 */
#define LEGBOT_BSP_SDMMC_ENABLED 0

    typedef enum
    {
        LEGBOT_BSP_STAGE_LOG_NVS_CONFIG = 0, /**< 日志、NVS 和配置初始化阶段。 */
        LEGBOT_BSP_STAGE_EVENT_STATE,        /**< 事件总线和运行状态初始化阶段。 */
        LEGBOT_BSP_STAGE_BOARD_PINS,         /**< 板级引脚初始化阶段。 */
        LEGBOT_BSP_STAGE_SHARED_BUSES,       /**< 共享总线初始化阶段。 */
        LEGBOT_BSP_STAGE_SERVICES,           /**< 服务启动阶段。 */
        LEGBOT_BSP_STAGE_SELFTEST_HOOKS,     /**< 自检挂钩初始化阶段。 */
        LEGBOT_BSP_STAGE_COUNT               /**< 初始化阶段数量。 */
    } legbot_bsp_init_stage_t;

    typedef enum
    {
        LEGBOT_BSP_RESOURCE_I2C0_SHARED = 0,    /**< 共享 I2C0 总线资源。 */
        LEGBOT_BSP_RESOURCE_CW2015,             /**< CW2015 电量计资源。 */
        LEGBOT_BSP_RESOURCE_QMI8658C,           /**< QMI8658C IMU 资源。 */
        LEGBOT_BSP_RESOURCE_CST9217,            /**< CST9217 触摸资源。 */
        LEGBOT_BSP_RESOURCE_CO5300,             /**< CO5300 屏幕资源。 */
        LEGBOT_BSP_RESOURCE_ES8311,             /**< ES8311 音频编解码资源。 */
        LEGBOT_BSP_RESOURCE_NS4150,             /**< NS4150 功放资源。 */
        LEGBOT_BSP_RESOURCE_PWR_INT,            /**< LTC2954 PWR_INT 只读输入资源。 */
        LEGBOT_BSP_RESOURCE_BOOT0,              /**< BOOT0 启动绑带只读输入资源。 */
        LEGBOT_BSP_RESOURCE_PVDF,               /**< PVDF 敲击检测 ADC/比较器资源，归 components/BSP/PVDF。 */
        LEGBOT_BSP_RESOURCE_RESET_N_EN,         /**< EN/RESET_N 硬件复位测试点资源。 */
        LEGBOT_BSP_RESOURCE_RESERVED_PINS,      /**< IO46/IO45 保留脚资源。 */
        LEGBOT_BSP_RESOURCE_USB_SERIAL_JTAG,    /**< USB-Serial-JTAG 资源。 */
        LEGBOT_BSP_RESOURCE_AIR780EGP_UART,     /**< Air780EGP UART 资源边界。 */
        LEGBOT_BSP_RESOURCE_SDMMC_POLICY,       /**< SDMMC 禁用策略资源。 */
        LEGBOT_BSP_RESOURCE_COUNT               /**< BSP 资源数量。 */
    } legbot_bsp_resource_id_t;

    /**
     * @brief 获取共享 I2C 短互斥的函数类型
     * @param timeout_ticks 等待互斥锁的 tick 数
     * @return ESP_OK 成功，其他值表示 manager 未就绪或等待超时
     */
    typedef esp_err_t (*legbot_bsp_i2c_acquire_fn_t)(TickType_t timeout_ticks);

    /** @brief 释放共享 I2C 短互斥的函数类型。 */
    typedef void (*legbot_bsp_i2c_release_fn_t)(void);

    typedef struct
    {
        i2c_master_bus_handle_t bus_handle;  /**< manager-owned I2C0 bus handle。 */
        legbot_bsp_i2c_acquire_fn_t acquire; /**< 获取共享总线短互斥的入口。 */
        legbot_bsp_i2c_release_fn_t release; /**< 释放共享总线短互斥的入口。 */
    } legbot_bsp_i2c_access_t;

    typedef struct
    {
        legbot_bsp_resource_id_t id;        /**< BSP 资源 ID。 */
        const char *name;                   /**< 稳定资源名称。 */
        const char *owner;                  /**< 资源归属组件。 */
        const char *boundary;               /**< 允许访问该资源的边界。 */
        legbot_bsp_init_stage_t init_stage; /**< 资源所属初始化阶段。 */
        gpio_num_t gpio_primary;            /**< 主 GPIO。 */
        gpio_num_t gpio_secondary;          /**< 次 GPIO。 */
        gpio_num_t gpio_aux0;               /**< 附加 GPIO 0。 */
        gpio_num_t gpio_aux1;               /**< 附加 GPIO 1。 */
        gpio_num_t gpio_aux2;               /**< 附加 GPIO 2。 */
        gpio_num_t gpio_aux3;               /**< 附加 GPIO 3。 */
        gpio_num_t gpio_aux4;               /**< 附加 GPIO 4。 */
        gpio_num_t gpio_aux5;               /**< 附加 GPIO 5。 */
        i2c_port_t i2c_port;                /**< 关联 I2C 端口。 */
        spi_host_device_t spi_host;         /**< 关联 SPI Host。 */
        uart_port_t uart_port;              /**< 关联 UART 端口。 */
        int i2s_port;                       /**< 关联 I2S 端口。 */
        bool reserved;                      /**< 资源是否仅预留不可业务占用。 */
        bool disabled;                      /**< 资源策略是否为禁用。 */
    } legbot_bsp_resource_t;

    /**
     * @brief 查询指定 BSP 资源描述
     * @param id BSP 资源 ID
     * @return 成功返回资源描述指针，ID 非法时返回 NULL
     */
    const legbot_bsp_resource_t *legbot_bsp_get_resource(legbot_bsp_resource_id_t id);

    /**
     * @brief 获取 BSP 资源数量
     * @return BSP 资源数量
     */
    size_t legbot_bsp_resource_count(void);

    /**
     * @brief 获取初始化阶段稳定名称
     * @param stage 初始化阶段
     * @return 阶段名称字符串，未知阶段返回 "unknown"
     */
    const char *legbot_bsp_stage_name(legbot_bsp_init_stage_t stage);

    /**
     * @brief 获取初始化阶段对应的稳定 BSP 错误码
     * @param stage 初始化阶段
     * @param err ESP-IDF 原始错误码
     * @return ESP_OK 返回 "BSP-OK"，其他错误返回阶段稳定错误码
     */
    const char *legbot_bsp_error_code(legbot_bsp_init_stage_t stage, esp_err_t err);

    /**
     * @brief 记录一个初始化阶段的返回值
     * @details 只记录最近一次启动周期的真实返回值，供自检输出可追溯的阶段结果。
     * @param stage 初始化阶段
     * @param err 该阶段的真实返回码
     */
    void legbot_bsp_record_stage_result(legbot_bsp_init_stage_t stage, esp_err_t err);

    /**
     * @brief 读取一个初始化阶段最近记录的真实返回值
     * @param stage 初始化阶段
     * @return 已记录的真实返回码，非法或未记录阶段返回 ESP_ERR_INVALID_STATE
     */
    esp_err_t legbot_bsp_stage_result(legbot_bsp_init_stage_t stage);

    /**
     * @brief 查询某阶段是否已经由本次启动周期记录过返回值
     * @param stage 初始化阶段
     * @return true 表示已有真实记录
     */
    bool legbot_bsp_stage_result_recorded(legbot_bsp_init_stage_t stage);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BSP_RESOURCES_H */
