/**
 * @file     bsp_resources.c
 * @brief    电子木鱼 EWF BSP 资源表实现
 * @details  定义权威资源描述表、初始化阶段名称、稳定 BSP 错误码映射和阶段返回值记录。
 * @author   ZHC
 * @date     2026-07-09
 */

#include "bsp_resources.h"

/** 无 GPIO 资源占位值。 */
#define NO_GPIO GPIO_NUM_NC

/** 无 I2C 端口占位值。 */
#define NO_I2C ((i2c_port_t) - 1)

/** 无 SPI Host 占位值。 */
#define NO_SPI ((spi_host_device_t) - 1)

/** 无 UART 端口占位值。 */
#define NO_UART ((uart_port_t) - 1)

/** 无 I2S 端口占位值。 */
#define NO_I2S (-1)

/** EWF 权威 BSP 资源描述表。 */
static const legbot_bsp_resource_t s_resources[LEGBOT_BSP_RESOURCE_COUNT] = {
    [LEGBOT_BSP_RESOURCE_I2C0_SHARED] = {
        .id = LEGBOT_BSP_RESOURCE_I2C0_SHARED,
        .name = "shared_i2c0",
        .owner = "components/BSP/BSP_INCLUDE",
        .boundary = "components/platform/i2c_manager",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = LEGBOT_BSP_I2C_SDA_GPIO,
        .gpio_secondary = LEGBOT_BSP_I2C_SCL_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = LEGBOT_BSP_I2C_PORT,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_CW2015] = {
        .id = LEGBOT_BSP_RESOURCE_CW2015,
        .name = "cw2015",
        .owner = "components/BSP/CW2015",
        .boundary = "components/platform/i2c_manager",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = NO_GPIO,
        .gpio_secondary = NO_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = LEGBOT_BSP_I2C_PORT,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_QMI8658C] = {
        .id = LEGBOT_BSP_RESOURCE_QMI8658C,
        .name = "qmi8658c",
        .owner = "components/BSP/QMI8658C",
        .boundary = "components/platform/i2c_manager",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = LEGBOT_BSP_QMI8658C_INT1_GPIO,
        .gpio_secondary = LEGBOT_BSP_QMI8658C_INT2_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = LEGBOT_BSP_I2C_PORT,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_CST9217] = {
        .id = LEGBOT_BSP_RESOURCE_CST9217,
        .name = "cst9217_touch",
        .owner = "components/BSP/CST9217",
        .boundary = "components/platform/i2c_manager",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = LEGBOT_BSP_CST9217_RST_GPIO,
        .gpio_secondary = LEGBOT_BSP_CST9217_INT_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = LEGBOT_BSP_I2C_PORT,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_CO5300] = {
        .id = LEGBOT_BSP_RESOURCE_CO5300,
        .name = "co5300_amoled",
        .owner = "components/BSP/CO5300",
        .boundary = "components/BSP/CO5300",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = LEGBOT_BSP_DISPLAY_CS_GPIO,
        .gpio_secondary = LEGBOT_BSP_DISPLAY_SCL_GPIO,
        .gpio_aux0 = LEGBOT_BSP_DISPLAY_D0_GPIO,
        .gpio_aux1 = LEGBOT_BSP_DISPLAY_D1_GPIO,
        .gpio_aux2 = LEGBOT_BSP_DISPLAY_D2_GPIO,
        .gpio_aux3 = LEGBOT_BSP_DISPLAY_D3_GPIO,
        .gpio_aux4 = LEGBOT_BSP_DISPLAY_RST_GPIO,
        .gpio_aux5 = LEGBOT_BSP_DISPLAY_EN_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = LEGBOT_BSP_DISPLAY_SPI_HOST,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_ES8311] = {
        .id = LEGBOT_BSP_RESOURCE_ES8311,
        .name = "es8311_codec",
        .owner = "components/BSP/ES8311",
        .boundary = "components/platform/i2c_manager",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = LEGBOT_BSP_ES8311_I2S_MCLK_GPIO,
        .gpio_secondary = LEGBOT_BSP_ES8311_I2S_BCLK_GPIO,
        .gpio_aux0 = LEGBOT_BSP_ES8311_I2S_WS_GPIO,
        .gpio_aux1 = LEGBOT_BSP_ES8311_I2S_DO_GPIO,
        .gpio_aux2 = LEGBOT_BSP_ES8311_I2S_DI_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = LEGBOT_BSP_I2C_PORT,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = LEGBOT_BSP_ES8311_I2S_PORT,
    },
    [LEGBOT_BSP_RESOURCE_NS4150] = {
        .id = LEGBOT_BSP_RESOURCE_NS4150,
        .name = "ns4150_pa",
        .owner = "components/BSP/NS4150",
        .boundary = "components/BSP/NS4150",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = LEGBOT_BSP_NS4150_PA_EN_GPIO,
        .gpio_secondary = NO_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_PWR_INT] = {
        .id = LEGBOT_BSP_RESOURCE_PWR_INT,
        .name = "pwr_int_io8",
        .owner = "components/BSP/KEY",
        .boundary = "services/power_service",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = EWF_BSP_PWR_INT_GPIO,
        .gpio_secondary = NO_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
        .reserved = true,
    },
    [LEGBOT_BSP_RESOURCE_BOOT0] = {
        .id = LEGBOT_BSP_RESOURCE_BOOT0,
        .name = "boot0_io0",
        .owner = "components/BSP/KEY",
        .boundary = "services/power_service",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = EWF_BSP_BOOT0_GPIO,
        .gpio_secondary = NO_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
        .reserved = true,
    },
    [LEGBOT_BSP_RESOURCE_PVDF] = {
        .id = LEGBOT_BSP_RESOURCE_PVDF,
        .name = "pvdf_adc_cmp",
        .owner = "components/BSP/PVDF",
        .boundary = "只允许 PVDF 输入服务经 components/BSP/PVDF 的稳定 API 访问",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = EWF_BSP_PVDF_ADC_GPIO,
        .gpio_secondary = EWF_BSP_PVDF_CMP_WAKE_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
    },
    [LEGBOT_BSP_RESOURCE_RESET_N_EN] = {
        .id = LEGBOT_BSP_RESOURCE_RESET_N_EN,
        .name = "reset_n_en_testpoint",
        .owner = "hardware",
        .boundary = "not_driven_by_firmware",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = EWF_BSP_EN_RESET_N_GPIO,
        .gpio_secondary = EWF_BSP_KILL_LTC2954_GPIO,
        .gpio_aux0 = EWF_BSP_PWR_STATE_LTC2954_EN_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
        .reserved = true,
    },
    [LEGBOT_BSP_RESOURCE_RESERVED_PINS] = {
        .id = LEGBOT_BSP_RESOURCE_RESERVED_PINS,
        .name = "io46_io45_unassigned",
        .owner = "components/BSP/BSP_INCLUDE",
        .boundary = "must_not_be_initialized",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = EWF_BSP_IO46_RESERVED_GPIO,
        .gpio_secondary = EWF_BSP_IO45_RESERVED_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
        .reserved = true,
    },
    [LEGBOT_BSP_RESOURCE_USB_SERIAL_JTAG] = {
        .id = LEGBOT_BSP_RESOURCE_USB_SERIAL_JTAG,
        .name = "usb_serial_jtag",
        .owner = "esp-idf usb/download",
        .boundary = "Story 1.3",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = EWF_BSP_USB_SERIAL_JTAG_DM_GPIO,
        .gpio_secondary = EWF_BSP_USB_SERIAL_JTAG_DP_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
        .reserved = true,
    },
    [LEGBOT_BSP_RESOURCE_AIR780EGP_UART] = {
        .id = LEGBOT_BSP_RESOURCE_AIR780EGP_UART,
        .name = "air780egp_uart",
        .owner = "hardware",
        .boundary = "resource_boundary_only_no_modem_service",
        .init_stage = LEGBOT_BSP_STAGE_SHARED_BUSES,
        .gpio_primary = EWF_BSP_AIR780EGP_TX_GPIO,
        .gpio_secondary = EWF_BSP_AIR780EGP_RX_GPIO,
        .gpio_aux0 = EWF_BSP_AIR780EGP_DTR_GPIO,
        .gpio_aux1 = EWF_BSP_AIR780EGP_RST_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = EWF_BSP_AIR780EGP_UART_PORT,
        .i2s_port = NO_I2S,
        .reserved = true,
    },
    [LEGBOT_BSP_RESOURCE_SDMMC_POLICY] = {
        .id = LEGBOT_BSP_RESOURCE_SDMMC_POLICY,
        .name = "sdmmc_disabled",
        .owner = "components/BSP/BSP_INCLUDE",
        .boundary = "disabled",
        .init_stage = LEGBOT_BSP_STAGE_BOARD_PINS,
        .gpio_primary = NO_GPIO,
        .gpio_secondary = NO_GPIO,
        .gpio_aux0 = NO_GPIO,
        .gpio_aux1 = NO_GPIO,
        .gpio_aux2 = NO_GPIO,
        .gpio_aux3 = NO_GPIO,
        .gpio_aux4 = NO_GPIO,
        .gpio_aux5 = NO_GPIO,
        .i2c_port = NO_I2C,
        .spi_host = NO_SPI,
        .uart_port = NO_UART,
        .i2s_port = NO_I2S,
        .disabled = true,
    },
};

/** 本次启动周期每个初始化阶段的真实返回值。 */
static esp_err_t s_stage_results[LEGBOT_BSP_STAGE_COUNT];
/** 本次启动周期每个初始化阶段是否已经记录过返回值。 */
static bool s_stage_result_recorded[LEGBOT_BSP_STAGE_COUNT];

const legbot_bsp_resource_t *legbot_bsp_get_resource(legbot_bsp_resource_id_t id)
{
    if (id < 0 || id >= LEGBOT_BSP_RESOURCE_COUNT)
    {
        return NULL;
    }
    return &s_resources[id];
}

size_t legbot_bsp_resource_count(void)
{
    return LEGBOT_BSP_RESOURCE_COUNT;
}

const char *legbot_bsp_stage_name(legbot_bsp_init_stage_t stage)
{
    switch (stage)
    {
    case LEGBOT_BSP_STAGE_LOG_NVS_CONFIG:
        return "log_nvs_config";
    case LEGBOT_BSP_STAGE_EVENT_STATE:
        return "event_state";
    case LEGBOT_BSP_STAGE_BOARD_PINS:
        return "board_pins";
    case LEGBOT_BSP_STAGE_SHARED_BUSES:
        return "shared_buses";
    case LEGBOT_BSP_STAGE_SERVICES:
        return "services";
    case LEGBOT_BSP_STAGE_SELFTEST_HOOKS:
        return "selftest_hooks";
    default:
        return "unknown";
    }
}

const char *legbot_bsp_error_code(legbot_bsp_init_stage_t stage, esp_err_t err)
{
    if (err == ESP_OK)
    {
        return "BSP-OK";
    }

    /* 错误码按初始化阶段稳定分配，便于串口日志和现场问题定位。 */
    switch (stage)
    {
    case LEGBOT_BSP_STAGE_LOG_NVS_CONFIG:
        return "BSP-E0101";
    case LEGBOT_BSP_STAGE_EVENT_STATE:
        return "BSP-E0201";
    case LEGBOT_BSP_STAGE_BOARD_PINS:
        return "BSP-E0301";
    case LEGBOT_BSP_STAGE_SHARED_BUSES:
        return "BSP-E0401";
    case LEGBOT_BSP_STAGE_SERVICES:
        return "BSP-E0501";
    case LEGBOT_BSP_STAGE_SELFTEST_HOOKS:
        return "BSP-E0601";
    default:
        return "BSP-E0001";
    }
}

void legbot_bsp_record_stage_result(legbot_bsp_init_stage_t stage, esp_err_t err)
{
    if (stage < 0 || stage >= LEGBOT_BSP_STAGE_COUNT)
    {
        return;
    }
    s_stage_results[stage] = err;
    s_stage_result_recorded[stage] = true;
}

esp_err_t legbot_bsp_stage_result(legbot_bsp_init_stage_t stage)
{
    if (stage < 0 || stage >= LEGBOT_BSP_STAGE_COUNT ||
        !s_stage_result_recorded[stage])
    {
        return ESP_ERR_INVALID_STATE;
    }
    return s_stage_results[stage];
}

bool legbot_bsp_stage_result_recorded(legbot_bsp_init_stage_t stage)
{
    if (stage < 0 || stage >= LEGBOT_BSP_STAGE_COUNT)
    {
        return false;
    }
    return s_stage_result_recorded[stage];
}
