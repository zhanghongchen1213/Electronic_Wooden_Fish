/**
 * @file     contract_checks.c
 * @brief    编译期 EWF 架构契约检查
 * @details  用静态断言固定芯片目标、PWR/BOOT/保留脚合同、电源边界、服务表与自检证据类别。
 *           本文件不产生运行时代码，只保证 Story 1.1 的板级事实不能在后续改动中静默漂移。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "app_state.h"
#include "bsp_resources.h"
#include "control_gate.h"
#include "key.h"
#include "legbot_services.h"
#include "power_boot_policy.h"
#include "power_service.h"
#include "pvdf_bsp.h"
#include "pvdf_confirm_policy.h"
#include "pvdf_input_service.h"
#include "selftest_service.h"
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

#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#error "Application console must use native USB Serial/JTAG."
#endif

#ifdef CONFIG_ESP_CONSOLE_UART
#error "Application UART console would take over the Air780EGP UART pins."
#endif

#if !CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION
#error "EWF requires USB Serial/JTAG monitoring protection during automatic light sleep."
#endif

/* PWR_INT 只作为 LTC2954 开漏低有效只读输入，且按下期间持续为低。 */
_Static_assert(EWF_BSP_PWR_INT_GPIO == GPIO_NUM_8, "PWR_INT must be IO8.");
_Static_assert(EWF_BSP_PWR_INT_ACTIVE_LEVEL == 0U, "PWR_INT is active low.");
_Static_assert(LEGBOT_KEY_PWR_GPIO == EWF_BSP_PWR_INT_GPIO,
               "Key BSP must consume the authoritative PWR_INT pin.");
_Static_assert(LEGBOT_POWER_SERVICE_ID == LEGBOT_SERVICE_POWER,
               "PWR/BOOT boundary must own the power service slot.");

/* BOOT0 只作为启动绑带只读输入，固件不得改写复位采样条件。 */
_Static_assert(EWF_BSP_BOOT0_GPIO == GPIO_NUM_0, "BOOT0 must be IO0.");
_Static_assert(EWF_BSP_BOOT0_ACTIVE_LEVEL == 0U, "BOOT0 is active low.");
_Static_assert(LEGBOT_KEY_BOOT_GPIO == EWF_BSP_BOOT0_GPIO,
               "Key BSP must consume the authoritative BOOT0 pin.");

/* EN/RESET_N、PWR_STATE、KILL 均不接 ESP32 GPIO，固件不得驱动。 */
_Static_assert(EWF_BSP_EN_RESET_N_GPIO == GPIO_NUM_NC,
               "EN/RESET_N is a hardware reset test point, not an application pin.");
_Static_assert(EWF_BSP_PWR_STATE_LTC2954_EN_GPIO == GPIO_NUM_NC,
               "PWR_STATE only drives the board rails.");
_Static_assert(EWF_BSP_KILL_LTC2954_GPIO == GPIO_NUM_NC,
               "Firmware must never drive the LTC2954 KILL pin.");

/* IO46/IO45 保持未分配，QMI8658A INT2 不得占用 IO45。 */
_Static_assert(EWF_BSP_IO46_RESERVED_GPIO == GPIO_NUM_46,
               "IO46 must stay as an unassigned boot strap pin.");
_Static_assert(EWF_BSP_IO45_RESERVED_GPIO == GPIO_NUM_45,
               "IO45 must stay unassigned.");
_Static_assert(LEGBOT_BSP_QMI8658C_INT2_GPIO == GPIO_NUM_NC,
               "IO45 must not be taken by the QMI8658A INT2 pin.");

/* PVDF ADC/比较器唤醒脚归 components/BSP/PVDF 纳管。 */
_Static_assert(EWF_BSP_PVDF_ADC_GPIO == GPIO_NUM_9, "PVDF ADC must be IO9.");
_Static_assert(EWF_BSP_PVDF_CMP_WAKE_GPIO == GPIO_NUM_11,
               "PVDF comparator wake must be IO11.");
_Static_assert(EWF_PVDF_ADC_GPIO == EWF_BSP_PVDF_ADC_GPIO,
               "PVDF BSP must consume the authoritative ADC pin.");
_Static_assert(EWF_PVDF_CMP_WAKE_GPIO == EWF_BSP_PVDF_CMP_WAKE_GPIO,
               "PVDF BSP must consume the authoritative comparator wake pin.");
/* 静止时开漏输出恒低，正向敲击释放为高，因此唤醒与运行态中断极性固定为高电平/上升沿。 */
_Static_assert(EWF_PVDF_CMP_WAKE_ACTIVE_LEVEL == 1U,
               "PVDF comparator wake must be active high.");
/* 上电盲窗必须严格覆盖比较器阈值节点 5τ ≈ 22.4 ms 的建立时间。 */
_Static_assert(EWF_PVDF_SETTLING_BLIND_MS >= 25U,
               "PVDF settling blind window must cover the comparator threshold 5tau.");
/* 确认窗口必须显著小于 20 次/秒的 50 ms 间隔，否则会吞并相邻敲击。 */
_Static_assert(EWF_PVDF_CONFIRM_WINDOW_MS <= 50U,
               "PVDF confirm window must stay below the 20 taps per second interval.");
_Static_assert(EWF_PVDF_CONFIRM_WINDOW_MS > 0U &&
                   EWF_PVDF_CONFIRM_SAMPLE_INTERVAL_MS > 0U,
               "PVDF confirm window and sample interval must be positive.");
_Static_assert(EWF_PVDF_CONFIRM_MAX_SAMPLES >= 1U &&
                   EWF_PVDF_CONFIRM_MAX_SAMPLES <= 32U,
               "PVDF confirmation must sample a bounded number of times.");
_Static_assert(LEGBOT_PVDF_INPUT_SERVICE_ID == LEGBOT_SERVICE_PVDF,
               "PVDF candidate input must own its fixed service slot.");

/* 共享 I2C0、USB-Serial-JTAG 与 Air780EGP UART 资源所有权。 */
_Static_assert(LEGBOT_BSP_I2C_PORT == I2C_NUM_0, "I2C0 must be the shared I2C bus.");
_Static_assert(LEGBOT_BSP_I2C_SDA_GPIO == GPIO_NUM_1, "Shared I2C SDA must be IO1.");
_Static_assert(LEGBOT_BSP_I2C_SCL_GPIO == GPIO_NUM_2, "Shared I2C SCL must be IO2.");
_Static_assert(EWF_BSP_USB_SERIAL_JTAG_DM_GPIO == GPIO_NUM_19,
               "USB-Serial-JTAG D- must be IO19.");
_Static_assert(EWF_BSP_USB_SERIAL_JTAG_DP_GPIO == GPIO_NUM_20,
               "USB-Serial-JTAG D+ must be IO20.");
_Static_assert(EWF_BSP_AIR780EGP_TX_GPIO == GPIO_NUM_43,
               "Air780EGP UART TX must be IO43.");
_Static_assert(EWF_BSP_AIR780EGP_RX_GPIO == GPIO_NUM_44,
               "Air780EGP UART RX must be IO44.");
_Static_assert(EWF_BSP_AIR780EGP_DTR_GPIO == GPIO_NUM_10,
               "Air780EGP DTR resource boundary must be IO10.");
_Static_assert(EWF_BSP_AIR780EGP_RST_GPIO == GPIO_NUM_15,
               "Air780EGP RST resource boundary must be IO15.");
_Static_assert(EWF_BSP_AIR780EGP_UART_PORT == UART_NUM_1,
               "Air780EGP must stay behind UART1.");
_Static_assert(LEGBOT_BSP_SDMMC_ENABLED == 0, "SDMMC must remain disabled for Story 1.1.");

/* 资源表、初始化阶段、服务表与自检清单规模。 */
_Static_assert(LEGBOT_BSP_RESOURCE_COUNT == 15,
               "BSP resource table must cover the authoritative EWF board resources.");
_Static_assert(LEGBOT_BSP_STAGE_COUNT == 6,
               "Story 1.1 fixes six serial initialization stages.");
_Static_assert(LEGBOT_SERVICE_COUNT == 6,
               "The EWF startup graph adds the unified tap input service and the "
               "progress owner to the input boundary.");
_Static_assert(LEGBOT_SELFTEST_SERVICE_ID == LEGBOT_SERVICE_SELFTEST,
               "Self-test must own its fixed service slot.");
_Static_assert(SELFTEST_ITEM_COUNT == 7,
               "Story 1.2 canonical self-test list must hold seven items.");

/* PWR/BOOT 边沿语义与自检证据类别不可被静默改写。 */
_Static_assert(EWF_POWER_BOOT_DEBOUNCE_MS == 20U,
               "PWR/BOOT debounce threshold is fixed by Story 1.1.");
_Static_assert(EWF_POWER_BOOT_RUNTIME_TAP_MAX_MS == 3000U,
               "BOOT0 runtime tap boundary is fixed by Story 1.1.");
_Static_assert(SELFTEST_EVIDENCE_DESIGN_INPUT == 0 &&
                   SELFTEST_EVIDENCE_SOFTWARE_OBSERVED == 1 &&
                   SELFTEST_EVIDENCE_HARDWARE_PENDING == 2 &&
                   SELFTEST_EVIDENCE_HARDWARE_VERIFIED == 3 &&
                   SELFTEST_EVIDENCE_FAILED == 4 &&
                   SELFTEST_EVIDENCE_COUNT == 5,
               "Self-test evidence categories are a frozen contract.");
_Static_assert(SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE == 0U,
               "No board receipt exists yet, so hardware_verified stays forbidden.");

/* 保留的 BLE 控制 reducer 仍然只能被显式调用，不得自行启动任何链路。 */
_Static_assert(CONTROL_MAX_ATTEMPTS == WATCH_CONTROL_MAX_ATTEMPTS,
               "Retained control reducer must keep the frozen attempt contract.");
