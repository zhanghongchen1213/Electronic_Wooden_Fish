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
#include "device_nav_service.h"
#include "feedback_service.h"
#include "sync_service.h"
#include "ui_service.h"
#include "ewf_ui_geometry.h"
#include "rgb_bsp.h"
#include "power_boot_policy.h"
#include "power_auto_mode_policy.h"
#include "power_core_path_policy.h"
#include "power_service.h"
#include "pvdf_bsp.h"
#include "pvdf_confirm_policy.h"
#include "pvdf_input_service.h"
#include "selftest_service.h"
#include "fault_gate_policy.h"
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
_Static_assert(LEGBOT_SERVICE_COUNT == 10,
               "The EWF startup graph adds ui_task to the prior nine "
               "services (power/state/selftest/pvdf/tap/progress/feedback/nav/sync).");
_Static_assert(LEGBOT_FEEDBACK_SERVICE_ID == LEGBOT_SERVICE_FEEDBACK,
               "Feedback must own its fixed service slot.");
_Static_assert(LEGBOT_DEVICE_NAV_SERVICE_ID == LEGBOT_SERVICE_DEVICE_NAV,
               "Device nav must own its fixed service slot.");
_Static_assert(LEGBOT_SYNC_SERVICE_ID == LEGBOT_SERVICE_SYNC,
               "Sync must own its fixed service slot.");
_Static_assert(LEGBOT_UI_SERVICE_ID == LEGBOT_SERVICE_UI,
               "UI must own its fixed service slot.");
_Static_assert(EWF_UI_CANVAS_W == 410 && EWF_UI_CANVAS_H == 502,
               "DEVICE-01 canvas must be 410x502.");
_Static_assert(EWF_UI_CORNER_RADIUS == 110,
               "DEVICE-01 corner radius must be 110.");
_Static_assert(EWF_UI_MARGIN_PX == 16,
               "DEVICE-01 margin must be 16px.");
_Static_assert(EWF_UI_STATUSBAR_H == 24,
               "DEVICE-01 statusbar height must be 24.");
_Static_assert(EWF_UI_PAGER_CONTENT_W == 1230,
               "DEVICE-01 pager content width must be 1230.");
_Static_assert(EWF_UI_TASK_CORE == 1,
               "ui_task must be pinned to Core 1.");
_Static_assert(EWF_BSP_RGB_DATA_GPIO == GPIO_NUM_3, "RGB_DATA must be IO3.");
_Static_assert(RGB_BSP_DATA_GPIO_ID == (int)EWF_BSP_RGB_DATA_GPIO,
               "RGB BSP must consume the authoritative RGB_DATA pin.");
_Static_assert(LEGBOT_SELFTEST_SERVICE_ID == LEGBOT_SERVICE_SELFTEST,
               "Self-test must own its fixed service slot.");
_Static_assert(SELFTEST_ITEM_COUNT == 7,
               "Story 1.2 canonical self-test list must hold seven items.");

/* PWR/BOOT 边沿语义与自检证据类别不可被静默改写。 */
_Static_assert(EWF_POWER_BOOT_DEBOUNCE_MS == 20U,
               "PWR/BOOT debounce threshold is fixed by Story 1.1.");
_Static_assert(EWF_POWER_BOOT_RUNTIME_TAP_MAX_MS == 3000U,
               "BOOT0 runtime tap boundary is fixed by Story 1.1.");
_Static_assert(EWF_POWER_AUTO_MODE_PERIOD_MS == 3000U,
               "BOOT0 auto-mode period is fixed by Story 2.6 / FR-E-011.");
_Static_assert(EWF_POWER_AUTO_MODE_PERIOD_TOLERANCE_MS == 100U,
               "BOOT0 auto-mode period tolerance is fixed by Story 2.6.");
_Static_assert(EWF_POWER_SOC_WARN_ENTER_PERCENT == 20U &&
                   EWF_POWER_SOC_WARN_EXIT_PERCENT == 23U &&
                   EWF_POWER_SOC_CRITICAL_ENTER_PERCENT == 10U &&
                   EWF_POWER_SOC_CRITICAL_EXIT_PERCENT == 13U,
               "Low-battery hysteresis thresholds are fixed by Story 2.7 "
               "(hardware_pending calibration, not a product promise).");
_Static_assert(EWF_FAULT_GATE_PERSIST_FAIL_THRESHOLD == 3U,
               "fault_locked persist-failure threshold is fixed by Story 2.7.");
_Static_assert(EWF_POWER_BATTERY_POLL_PERIOD_MS >= 30000U &&
                   EWF_POWER_BATTERY_POLL_PERIOD_MS <= 60000U,
               "CW2015 product poll period must stay in the 30-60s band.");
_Static_assert(EWF_POWER_CORE_ACTION_FLUSH_PROGRESS == 0 &&
                   EWF_POWER_CORE_ACTION_ALLOW_FEEDBACK == 1 &&
                   EWF_POWER_CORE_ACTION_ALLOW_SYNC == 2 &&
                   EWF_POWER_CORE_ACTION_DEFER_NONCRITICAL == 3,
               "core path action enum order is fixed by Story 2.7.");
/* PRD final：三种供电同一路径；生产源不得引入充电门控符号（见 test_bsp_contract）。 */
_Static_assert(SELFTEST_EVIDENCE_DESIGN_INPUT == 0 &&
                   SELFTEST_EVIDENCE_SOFTWARE_OBSERVED == 1 &&
                   SELFTEST_EVIDENCE_HARDWARE_PENDING == 2 &&
                   SELFTEST_EVIDENCE_HARDWARE_VERIFIED == 3 &&
                   SELFTEST_EVIDENCE_FAILED == 4 &&
                   SELFTEST_EVIDENCE_COUNT == 5,
               "Self-test evidence categories are a frozen contract.");
_Static_assert(SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE == 0U,
               "No board receipt exists yet, so hardware_verified stays forbidden.");

/* Story 3.2：木鱼页三环/触区/琥珀色常量契约。 */
#include "ui_muyu_constants.h"
_Static_assert(EWF_UI_MUYU_TAP_RING_COUNT == 3U, "tap-rings must be exactly 3.");
_Static_assert(EWF_UI_MUYU_TAP_FLASH_MS == 160U, "tap flash duration is 160ms.");
_Static_assert(EWF_UI_MUYU_TOUCH_MIN_PX == 96U, "woodfish touch min edge is 96px.");
_Static_assert(EWF_UI_MUYU_AMBER_300_HEX == 0xE6BD69U, "flash amber.300 must match DESIGN.");
_Static_assert(EWF_UI_MUYU_AMBER_400_HEX == 0xD9A441U, "glyph amber.400 must match DESIGN.");
_Static_assert(EWF_UI_MUYU_BELT_SLOTS == 7U, "MUYU belt viewport is 7 slots.");
_Static_assert(EWF_UI_MUYU_PROGRESS_DENOM == 260U, "progress denominator is consumable count.");
_Static_assert(EWF_UI_MUYU_MODAL_OVERLAY_OPA == 0x99U, "modal overlay opa is ~60% black.");
_Static_assert(EWF_UI_MUYU_MODAL_OVERLAY_W == 410, "modal overlay width matches canvas.");
_Static_assert(EWF_UI_MUYU_MODAL_OVERLAY_H == 502, "modal overlay height matches canvas.");
_Static_assert(EWF_UI_MUYU_MODAL_CARD_W == 338, "modal-done card width.");
_Static_assert(EWF_UI_MUYU_MODAL_CARD_H == 190, "modal-done card height.");
_Static_assert(EWF_UI_MUYU_MODAL_CARD_X == 36, "modal-done card x.");
_Static_assert(EWF_UI_MUYU_MODAL_CARD_Y == 150, "modal-done card y.");
_Static_assert(EWF_UI_MUYU_MODAL_CARD_RADIUS == 16, "modal-done card radius.");
_Static_assert(EWF_UI_MUYU_MODAL_CARD_BG_HEX == 0x121212U, "modal card bg.");
_Static_assert(EWF_UI_MUYU_MODAL_RESTART_HEX == 0xE6BD69U, "restart amber.300.");
_Static_assert(EWF_UI_MUYU_MODAL_MUTED_HEX == 0xCFC4B0U, "summary/exit muted.");
_Static_assert(EWF_UI_MUYU_MODAL_TITLE_HEX == 0xFBFAF0U, "title color.");
/* 禁止第二套顶层 OVERLAY Screen：遮罩挂在 MUYU 页内（见 ui_scr_shell.c）。 */

/* Story 3.3：经文页 13 槽 append-only 流常量契约。 */
#include "ui_jingwen_constants.h"
_Static_assert(EWF_UI_JINGWEN_ROW_SLOTS == 13U, "JINGWEN history row width is 13 slots.");
_Static_assert(EWF_UI_JINGWEN_DISPLAY_CAP == 303U, "display stream cap is totalChars.");
_Static_assert(EWF_UI_JINGWEN_MAX_ROWS == 24U, "max rows is ceil(303/13).");
_Static_assert(EWF_UI_JINGWEN_HISTORY_W == 362, "scripture-history width matches HTML.");
_Static_assert(EWF_UI_JINGWEN_HISTORY_H == 286, "scripture-history height matches HTML.");
_Static_assert(EWF_UI_JINGWEN_HISTORY_X == 24, "scripture-history left matches HTML.");
_Static_assert(EWF_UI_JINGWEN_HISTORY_Y == 102, "scripture-history top matches HTML.");
_Static_assert(EWF_UI_MUYU_AMBER_400_HEX == 0xD9A441U,
               "glyph-current reuses amber.400 from muyu constants.");

/* Story 3.4：统计页几何与进度分母契约。 */
#include "ui_tongji_constants.h"
_Static_assert(EWF_UI_PAGE_TONGJI_X == 820, "TONGJI root frame X is 820.");
_Static_assert(EWF_UI_TONGJI_CARD_W == 378, "stat-card width matches HTML.");
_Static_assert(EWF_UI_TONGJI_CARD_H == 64, "stat-card height matches HTML.");
_Static_assert(EWF_UI_TONGJI_TODAY_Y == 112, "today card top matches HTML.");
_Static_assert(EWF_UI_TONGJI_TOTAL_Y == 184, "total card top matches HTML.");
_Static_assert(EWF_UI_TONGJI_PROGRESS_Y == 264, "reading-progress top matches HTML.");
_Static_assert(EWF_UI_TONGJI_PROGRESS_H == 176, "reading-progress height matches HTML.");
_Static_assert(EWF_UI_TONGJI_RING_SIZE == 120, "progress ring size matches HTML.");
_Static_assert(EWF_UI_TONGJI_PROGRESS_DENOM == 260U,
               "TONGJI progress denominator is consumable count.");
_Static_assert(EWF_UI_TONGJI_ACCENT_HEX == 0xD9A441U, "amber accent matches DESIGN.");

/* Story 3.5：设置页几何与首屏行数契约；五态不复制 Screen。 */
#include "sync_https_codec.h"
#include "ui_shezhi_constants.h"
_Static_assert(EWF_UI_SHEZHI_VIEWPORT_W == 378, "settings viewport width matches HTML.");
_Static_assert(EWF_UI_SHEZHI_VIEWPORT_H == 344, "settings viewport height matches HTML.");
_Static_assert(EWF_UI_SHEZHI_CONTENT_H == 520, "settings content height matches HTML.");
_Static_assert(EWF_UI_SHEZHI_PAGE1_ROW_COUNT == 4, "settings page-1 has exactly 4 rows.");
_Static_assert(EWF_UI_SHEZHI_PAGE2_TOP == 344, "settings page-2 top matches HTML.");
_Static_assert(EWF_UI_SHEZHI_ROW_H == 80, "settings row height matches HTML.");
_Static_assert(EWF_UI_SHEZHI_SELECTED_BG_HEX == 0xD9A441U, "capsule selected amber matches DESIGN.");
_Static_assert(EWF_UI_SHEZHI_ACTION_HEX == 0xE6BD69U, "sync action amber.300 matches DESIGN.");
_Static_assert(EWF_UI_SHEZHI_SYNC_STATUS_IDLE == (unsigned)WATCH_SYNC_STATUS_IDLE,
               "shezhi sync IDLE mirrors watch_sync_status.");
_Static_assert(EWF_UI_SHEZHI_SYNC_STATUS_PENDING == (unsigned)WATCH_SYNC_STATUS_PENDING,
               "shezhi sync PENDING mirrors watch_sync_status.");
_Static_assert(EWF_UI_SHEZHI_SYNC_STATUS_BUSY == (unsigned)WATCH_SYNC_STATUS_BUSY,
               "shezhi sync BUSY mirrors watch_sync_status.");
_Static_assert(EWF_UI_SHEZHI_SYNC_STATUS_OK == (unsigned)WATCH_SYNC_STATUS_OK,
               "shezhi sync OK mirrors watch_sync_status.");
_Static_assert(EWF_UI_SHEZHI_SYNC_STATUS_FAIL == (unsigned)WATCH_SYNC_STATUS_FAIL,
               "shezhi sync FAIL mirrors watch_sync_status.");
_Static_assert(EWF_UI_SHEZHI_DEVICE_ID_TEXT_CAP == EWF_SYNC_DEVICE_ID_CAPACITY,
               "shezhi device_id text capacity matches sync codec.");
_Static_assert(EWF_UI_SHEZHI_FIRMWARE_TEXT_CAP == EWF_SYNC_FIRMWARE_VERSION_CAPACITY,
               "shezhi firmware text capacity matches sync codec.");

/* 保留的 BLE 控制 reducer 仍然只能被显式调用，不得自行启动任何链路。 */
_Static_assert(CONTROL_MAX_ATTEMPTS == WATCH_CONTROL_MAX_ATTEMPTS,
               "Retained control reducer must keep the frozen attempt contract.");
