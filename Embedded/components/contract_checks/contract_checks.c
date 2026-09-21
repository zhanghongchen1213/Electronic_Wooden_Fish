/**
 * @file     contract_checks.c
 * @brief    编译期架构契约检查
 * @details  使用静态断言固定 BSP、服务数量、BLE 完整控制包与 typed 状态边界。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "app_state.h"
#include "audio_service.h"
#include "ble_service.h"
#include "ble_control_packet.h"
#include "ble_status_frame.h"
#include "ble_unlock.h"
#include "bsp_resources.h"
#include "co5300_bsp.h"
#include "cloud_service.h"
#include "config_service.h"
#include "control_gate.h"
#include "cst9217_bsp.h"
#include "es8311_bsp.h"
#include "event_bus.h"
#include "gps_service.h"
#include "i2c_manager.h"
#include "key.h"
#include "legbot_services.h"
#include "l76kb_a58_bsp.h"
#include "cw2015_bsp.h"
#include "maintenance_service.h"
#include "ml307r_bsp.h"
#include "ml307r_https_transport.h"
#include "modem_recovery_policy.h"
#include "modem_service.h"
#include "power_service.h"
#include "qmi8658c_bsp.h"
#include "sdkconfig.h"
#include "state_service.h"
#include "selftest_service.h"
#include "ui_service.h"
#include "ui_navigation.h"
#include "ui_runtime_binding.h"
#include "ui_selftest_summary.h"
#include "vibration_bsp.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "Contract checks require CONFIG_IDF_TARGET=esp32s3."
#endif

#if !CONFIG_SPIRAM
#error "Story 1.2 requires CONFIG_SPIRAM for LVGL double buffers."
#endif

#if !CONFIG_SPIRAM_USE_MALLOC
#error "LVGL custom allocator requires CONFIG_SPIRAM_USE_MALLOC."
#endif

#if !CONFIG_LV_MEM_CUSTOM
#error "Generated main shell exceeds the LVGL fixed 32 KiB heap; use the custom allocator."
#endif

#if !CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#error "Application console must use native USB Serial/JTAG."
#endif

#ifdef CONFIG_ESP_CONSOLE_UART
#error "Application UART console conflicts with ML307R GPIO43/GPIO44."
#endif

#if CONFIG_LV_COLOR_DEPTH != 16
#error "Story 1.2 requires RGB565 (CONFIG_LV_COLOR_DEPTH=16)."
#endif

#if !CONFIG_LV_COLOR_16_SWAP
#error "Story 1.2 requires CONFIG_LV_COLOR_16_SWAP=y for CO5300."
#endif

#if CONFIG_LV_DISP_DEF_REFR_PERIOD != 8
#error "Resident shell 60 FPS requires an 8 ms LVGL refresh period."
#endif

#if !CONFIG_COMPILER_OPTIMIZATION_PERF
#error "Resident shell 60 FPS requires the ESP-IDF performance optimization profile."
#endif

#if !CONFIG_SPI_MASTER_IN_IRAM
#error "Resident shell 60 FPS requires the SPI master main path in IRAM."
#endif

#if CONFIG_FREERTOS_UNICORE
#error "Resident shell 60 FPS requires ESP32-S3 Core 1 for ui_task."
#endif

#if !CONFIG_BT_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
#error "Story 2.1 requires the ESP-IDF NimBLE host."
#endif

#if CONFIG_BT_NIMBLE_PINNED_TO_CORE != 0
#error "Resident shell 60 FPS keeps the NimBLE host pinned to Core 0."
#endif

#if !CONFIG_BT_NIMBLE_ROLE_CENTRAL || !CONFIG_BT_NIMBLE_ROLE_OBSERVER
#error "Story 2.1 requires NimBLE Central and Observer roles."
#endif

#if !CONFIG_BT_NIMBLE_GATT_CLIENT
#error "Story 2.2 requires the NimBLE GATT client."
#endif

#if CONFIG_BT_NIMBLE_ENABLE_CONN_REATTEMPT
#error "Story 2.5 requires ble_task to be the only product reconnect owner."
#endif

_Static_assert(LEGBOT_BSP_I2C_PORT == I2C_NUM_0, "I2C0 must be the shared I2C bus.");
_Static_assert(I2C_MANAGER_EXPECTED_DEVICE_COUNT == 4, "Expected logical I2C scan table must remain complete.");
_Static_assert(I2C_MANAGER_SCAN_TIMEOUT_BUDGET_MS < 3000, "Expected I2C scan timeout budget must stay below 3 seconds.");
_Static_assert(CW2015_BSP_I2C_ADDRESS == 0x62, "CW2015 must remain at 7-bit address 0x62.");
_Static_assert(QMI8658C_BSP_I2C_ADDRESS_HIGH == 0x6A, "QMI8658C high strap address must remain 0x6A.");
_Static_assert(QMI8658C_BSP_I2C_ADDRESS_LOW == 0x6B, "QMI8658C low strap address must remain 0x6B.");
_Static_assert(LEGBOT_BSP_QMI8658C_INT1_GPIO == GPIO_NUM_41,
               "QMI8658C wrist wake must use INT1 on GPIO41.");
_Static_assert(LEGBOT_BSP_QMI8658C_INT2_GPIO == GPIO_NUM_45,
               "QMI8658C INT2 must remain on the ESP32-S3 strapping GPIO45.");
_Static_assert(QMI8658C_BSP_WOM_THRESHOLD_MG == 32U,
               "QMI8658C initial WoM threshold must remain 32 mg.");
_Static_assert(QMI8658C_BSP_WOM_BLANKING_SAMPLES == 4U,
               "QMI8658C initial WoM blanking must remain four samples.");
_Static_assert(QMI8658C_BSP_CTRL9_TIMEOUT_MS == 50U,
               "QMI8658C CTRL9 handshake must remain bounded to 50 ms.");
_Static_assert(QMI8658C_BSP_MODE_STANDBY == 0 &&
                   QMI8658C_BSP_MODE_WOM_MONITOR == 1 &&
                   QMI8658C_BSP_MODE_ACCEL_CLASSIFY == 2 &&
                   QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS == 3,
               "QMI8658C public mode values must remain stable.");
_Static_assert(CW2015_BSP_DIAGNOSTIC_SAMPLE_COUNT == 3, "CW2015 diagnostics must attempt three reads.");
_Static_assert(QMI8658C_BSP_DIAGNOSTIC_SAMPLE_COUNT == 3, "QMI8658C diagnostics must attempt three reads.");
_Static_assert(CW2015_BSP_CONTINUOUS_TIMEOUT_BUDGET_MS < 3000, "CW2015 diagnostic timeout budget must stay below 3 seconds.");
_Static_assert(QMI8658C_BSP_CONTINUOUS_TIMEOUT_BUDGET_MS < 3000, "QMI8658C diagnostic timeout budget must stay below 3 seconds.");
_Static_assert(LEGBOT_BSP_DISPLAY_SPI_HOST == SPI3_HOST, "CO5300 must own SPI3_HOST.");
_Static_assert(LEGBOT_BSP_DISPLAY_D1_GPIO == GPIO_NUM_7, "CO5300 D1 must be IO7.");
_Static_assert(LEGBOT_BSP_DISPLAY_D2_GPIO == GPIO_NUM_12, "CO5300 D2 must be IO12.");
_Static_assert(LEGBOT_BSP_DISPLAY_D3_GPIO == GPIO_NUM_42, "CO5300 D3 must be IO42.");
_Static_assert(LEGBOT_BSP_GPS_TX_GPIO == GPIO_NUM_8, "GPS TX must own IO8.");
_Static_assert(LEGBOT_BSP_GPS_RX_GPIO == GPIO_NUM_9, "GPS RX must own IO9.");
_Static_assert(LEGBOT_BSP_GPS_UART_PORT == UART_NUM_1, "GPS must be behind UART1.");
_Static_assert(LEGBOT_BSP_GPS_WAKE_GPIO == GPIO_NUM_3, "GPS WAKE must own IO3.");
_Static_assert(L76KB_A58_UART_BAUD_RATE == 9600U,
               "L76KB-A58 UART must remain at 9600 baud.");
_Static_assert(L76KB_A58_UART_DATA_BITS == UART_DATA_8_BITS &&
                   L76KB_A58_UART_PARITY == UART_PARITY_DISABLE &&
                   L76KB_A58_UART_STOP_BITS == UART_STOP_BITS_1 &&
                   L76KB_A58_UART_FLOW_CONTROL == UART_HW_FLOWCTRL_DISABLE,
               "L76KB-A58 UART framing must remain 8N1 without flow control.");
_Static_assert(L76KB_A58_WAKE_POLICY == L76KB_A58_WAKE_POLICY_RELEASED_INPUT,
               "L76KB-A58 WAKE must be released to a pull-free high-impedance input while active.");
_Static_assert(LEGBOT_BSP_ML307R_UART_PORT == UART_NUM_2, "ML307R must be behind UART2 AT core.");
_Static_assert(LEGBOT_BSP_ML307R_TX_GPIO == GPIO_NUM_43,
               "ML307R UART2 TX must remain on GPIO43.");
_Static_assert(LEGBOT_BSP_ML307R_RX_GPIO == GPIO_NUM_44,
               "ML307R UART2 RX must remain on GPIO44.");
_Static_assert(LEGBOT_BSP_ML307R_EN_GPIO == GPIO_NUM_10,
               "ML307R active-high enable must remain on GPIO10.");
_Static_assert(LEGBOT_BSP_ML307R_EN_ACTIVE_LEVEL == 1U,
               "ML307R enable level contract must remain active-high.");
_Static_assert(LEGBOT_BSP_ML307R_EN_INACTIVE_LEVEL == 0U,
               "ML307R disabled profile must hold enable low.");
_Static_assert(LEGBOT_BSP_ML307R_EN_ACTIVE_LEVEL !=
                   LEGBOT_BSP_ML307R_EN_INACTIVE_LEVEL,
               "ML307R active and inactive enable levels must differ.");
_Static_assert(ML307R_BSP_UART_READY_DELAY_MS >= 10000U,
               "ML307R first AT must wait at least 10 seconds after enable.");
_Static_assert(MODEM_RECOVERY_BACKOFF_FIRST_MS == 30000U,
               "ML307R automatic recovery backoff must start at 30 seconds.");
_Static_assert(MODEM_RECOVERY_BACKOFF_MAX_MS == 14400000U,
               "ML307R automatic recovery backoff must cap at four hours.");
_Static_assert(LEGBOT_BSP_PWR_KEY_GPIO == GPIO_NUM_46, "PWR key must be IO46.");
_Static_assert(LEGBOT_BSP_BOOT_KEY_GPIO == GPIO_NUM_0, "BOOT key must be IO0.");
_Static_assert(LEGBOT_BSP_SDMMC_ENABLED == 0, "SDMMC must remain disabled for Story 1.1.");
_Static_assert(LEGBOT_BSP_RESOURCE_COUNT == 13, "BSP resource table must cover authoritative board resources.");
_Static_assert(LEGBOT_SERVICE_COUNT == 11, "All fixed service task boundaries must be declared.");
_Static_assert(CONTROL_GATE_UNLOCK_CONFIRM_TIMEOUT_MS == 5000U,
               "Unlock confirmation must remain exactly five seconds.");
_Static_assert(BLE_CONTROL_PACKET_LENGTH == 32U,
               "Exoskeleton control packets must remain exactly 32 bytes.");
_Static_assert(BLE_CONTROL_DEVICE_CHECK_OFFSET == 19U,
               "The cloud authorization byte must remain at offset nineteen.");
_Static_assert(BLE_CONTROL_SHUTDOWN_OFFSET == 20U,
               "The power-off byte must remain at offset twenty.");
_Static_assert(BLE_CONTROL_RESERVED_DATA_LENGTH == 9U,
               "The control reserved-data field must remain nine bytes.");
_Static_assert(BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT_MS == 5000U,
               "The GATT write callback wait must remain bounded to five seconds.");
_Static_assert(CLOUD_RENTAL_REQUEST_QUEUE_DEPTH == 2U,
               "Rental requests must use the fixed bounded cloud queue.");
_Static_assert(CLOUD_RENTAL_RESULT_QUEUE_DEPTH == 2U,
               "Rental results must use the fixed bounded cloud queue.");
_Static_assert(sizeof(CLOUD_DEFAULT_BASE_URL) ==
                       sizeof("https://watch-api.xianlitech.com") &&
                   sizeof(CLOUD_DEFAULT_APN) == 1U,
               "Default cloud deployment must retain the production URL and auto APN.");
_Static_assert(CLOUD_TELEMETRY_INTERVAL_MS == 180000U,
               "Telemetry fixed windows must remain exactly three minutes.");
_Static_assert(CLOUD_PAYMENT_SLOW_INTERVAL_MS ==
                   CLOUD_TELEMETRY_INTERVAL_MS,
               "Payment SLOW cadence must reuse the telemetry fixed window.");
_Static_assert(CLOUD_PAYMENT_FAST_INTERVAL_MS == 15000U &&
                   CLOUD_PAYMENT_FAST_LAST_OFFSET_MS == 165000U &&
                   CLOUD_PAYMENT_FAST_WINDOW_MS == 180000U &&
                   CLOUD_PAYMENT_FAST_HTTP_MAX == 12U,
               "Payment FAST cadence must retain the approved finite schedule.");
_Static_assert(CLOUD_PAYMENT_FAST_LAST_OFFSET_MS ==
                       CLOUD_PAYMENT_FAST_INTERVAL_MS *
                           (CLOUD_PAYMENT_FAST_HTTP_MAX - 1U) &&
                   CLOUD_PAYMENT_FAST_WINDOW_MS >
                       CLOUD_PAYMENT_FAST_LAST_OFFSET_MS,
               "Payment FAST offsets must remain internally consistent.");
_Static_assert(CLOUD_PAYMENT_BACKOFF_STEP_COUNT == 7U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_1_MS == 30000U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_2_MS == 60000U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_3_MS == 120000U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_4_MS == 300000U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_5_MS == 3600000U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_6_MS == 7200000U &&
                   CLOUD_PAYMENT_BACKOFF_STEP_7_MS == 14400000U,
               "Payment network backoff must retain all seven approved steps.");
_Static_assert(CLOUD_TELEMETRY_LATEST_SLOT_CAPACITY == 1U,
               "Telemetry cache must remain a single latest-only slot.");
_Static_assert(CLOUD_TELEMETRY_FINAL_PUBLISH_ATTEMPTS == 4U,
               "Telemetry shutdown diagnostic retries must remain bounded.");
_Static_assert(CLOUD_OWNER_POLL_MS == 20U,
               "Cloud owner polling must remain bounded to twenty milliseconds.");
_Static_assert(CLOUD_REQUEST_GROUP_TIMEOUT_MS == 80000U,
               "Cloud logical request groups must retain the eighty-second budget.");
_Static_assert(CLOUD_MODEM_WAKE_WAIT_MS == 65000U,
               "Payment modem wake wait must cover one sixty-second connect session.");
_Static_assert(sizeof(CLOUD_RENTAL_CHECK_PATH) ==
                       sizeof("/api/v1/iot/rental/check") &&
                   sizeof(CLOUD_RENTAL_CHECK_PATH) <=
                       sizeof(((cloud_rental_http_request_t *)0)->path),
               "Rental path must remain fixed and fit the bounded request.");
_Static_assert(sizeof(CLOUD_TELEMETRY_PATH) ==
                       sizeof("/api/v1/iot/telemetry") &&
                   sizeof(CLOUD_TELEMETRY_PATH) <=
                       sizeof(((cloud_telemetry_http_request_t *)0)->path),
               "Telemetry path must remain fixed and fit the bounded request.");
_Static_assert(CLOUD_TELEMETRY_BODY_CAPACITY <= ML307R_HTTPS_BODY_CAPACITY,
               "Telemetry body must fit the ML307R HTTPS transport payload.");
_Static_assert(CLOUD_TELEMETRY_BODY_CAPACITY <= UINT16_MAX,
               "Telemetry explicit body length must fit uint16_t.");
_Static_assert(CLOUD_TELEMETRY_RESPONSE_BODY_CAPACITY == 512U,
               "Telemetry response parsing must remain explicitly bounded.");
_Static_assert(CLOUD_TELEMETRY_RESPONSE_BODY_CAPACITY <=
                   ML307R_HTTPS_RESPONSE_BODY_CAPACITY,
               "Telemetry response parsing must fit the transport terminal body.");
_Static_assert(CLOUD_REMOTE_ERROR_CODE_MAX_LENGTH < CLOUD_ERROR_CODE_CAPACITY,
               "Remote error codes must fit below the local stable-code capacity.");
_Static_assert(CLOUD_REMOTE_MESSAGE_MAX_LENGTH <
                   CLOUD_TELEMETRY_RESPONSE_BODY_CAPACITY,
               "Remote messages must remain below the complete response bound.");
_Static_assert(CLOUD_SERVER_TIME_MAX_MS == WATCH_CLOUD_SERVER_TIME_MAX_MS,
               "Cloud parser and product state must share the JSON-safe time limit.");
_Static_assert(CLOUD_ERROR_CODE_CAPACITY == 40U &&
                   WATCH_STATE_ERROR_CODE_CAPACITY == 40U &&
                   MAINTENANCE_ERROR_CODE_CAPACITY == 48U,
               "Telemetry stable-code capacities must remain explicitly bounded.");
_Static_assert(CLOUD_ERROR_CODE_CAPACITY == WATCH_STATE_ERROR_CODE_CAPACITY &&
                   MAINTENANCE_ERROR_CODE_CAPACITY >= CLOUD_ERROR_CODE_CAPACITY,
               "Cloud stable codes must preserve capacity through state and maintenance.");
_Static_assert(sizeof(cloud_telemetry_response_t) <= 64U,
               "Telemetry response DTO must remain a compact by-value terminal.");
_Static_assert(sizeof(((cloud_telemetry_http_request_t *)0)->body) ==
                   CLOUD_TELEMETRY_BODY_CAPACITY,
               "Telemetry HTTP body storage must match its public capacity.");
_Static_assert(sizeof(cloud_telemetry_snapshot_t) <= 96U,
               "Telemetry projection must remain bounded and replaceable by value.");
_Static_assert(sizeof(cloud_telemetry_http_request_t) <= 512U,
               "Telemetry HTTP request must remain a bounded by-value payload.");
_Static_assert(sizeof(((cloud_telemetry_snapshot_t *)0)->watch_id) ==
                   WATCH_STATE_WATCH_ID_CAPACITY,
               "Telemetry must preserve the complete watch identity capacity.");
_Static_assert(sizeof(((cloud_telemetry_snapshot_t *)0)->exoskeleton_mac) ==
                   WATCH_BLE_MAC_CAPACITY,
               "Telemetry must preserve the complete bound MAC capacity.");
_Static_assert(sizeof(((cloud_telemetry_snapshot_t *)0)->binding_generation) ==
                   sizeof(uint32_t),
               "Telemetry binding generation must remain a 32-bit epoch.");
_Static_assert(BLE_UNLOCK_REQUEST_QUEUE_DEPTH == 2U,
               "First-unlock intents must remain bounded inside ble_task.");
_Static_assert(BLE_UNLOCK_RESULT_QUEUE_DEPTH == 2U,
               "First-unlock outcomes must remain bounded inside ble_task.");
_Static_assert(BLE_CONTROL_RESULT_QUEUE_DEPTH == 4U,
               "Ordinary control outcomes must remain bounded inside ble_task.");
_Static_assert(BLE_CONTROL_PENDING_RESULT_DEPTH > BLE_EVENT_QUEUE_DEPTH,
               "Every accepted control intent needs reserved terminal FIFO credit.");
_Static_assert(BLE_CONTROL_WRITE_CALLBACK_TIMEOUT_MS == 5000U,
               "Initial ordinary-control callback wait must remain bounded.");
_Static_assert(CONTROL_ACK_TIMEOUT_MS == 3000U,
               "Ordinary-control product ACK must remain exactly three seconds.");
_Static_assert(CONTROL_MAX_ATTEMPTS == 3U,
               "Ordinary control must allow one initial write plus two retries.");
_Static_assert(POWEROFF_RESULT_TIMEOUT_MS == 15000U,
               "Power-off product result must remain exactly fifteen seconds.");
_Static_assert(WATCH_CONTROL_SINGLE_PENDING == 1U,
               "Authorization and ordinary control must share one pending slot.");
_Static_assert(sizeof(cloud_rental_request_t) <= 32U,
               "Rental request queue payload must remain compact and by value.");
_Static_assert(sizeof(cloud_rental_result_t) <= 160U,
               "Rental result queue payload must remain bounded and by value.");
_Static_assert(sizeof(ble_unlock_result_t) <= 80U,
               "BLE unlock result queue payload must remain bounded and by value.");
_Static_assert(sizeof(ble_control_intent_t) <= 8U,
               "BLE ordinary-control intent must remain compact and typed.");
_Static_assert(sizeof(ble_control_result_t) <= 80U,
               "BLE ordinary-control result must remain bounded and by value.");
_Static_assert(sizeof(watch_control_update_t) <= 128U,
               "Control state update must remain bounded and by value.");
_Static_assert(WATCH_UNLOCK_SESSION_RAM_ONLY == 1U,
               "Unlock session proof must remain volatile and RAM-only.");
_Static_assert(ML307R_HTTPS_ATTEMPT_TIMEOUT_MS == 20000U,
               "ML307R HTTPS attempt timeout must remain exactly twenty seconds.");
_Static_assert(ML307R_HTTPS_CONNECT_TIMEOUT_MS == 10000U,
               "ML307R connect and TLS timeout must remain at most ten seconds.");
_Static_assert(CLOUD_REQUEST_RETRY_MAX == 2U,
               "Telemetry HTTPS must allow at most two retries.");
_Static_assert(CLOUD_REQUEST_RETRY_FIRST_MS == 5000U &&
                   CLOUD_REQUEST_RETRY_SECOND_MS == 15000U,
               "Telemetry HTTPS retry backoff must remain five and fifteen seconds.");
_Static_assert(ML307R_HTTPS_AUTH_NONE == 0 &&
                   ML307R_HTTPS_AUTH_SERVER == 1,
               "ML307R HTTPS auth modes must match the vendor SSL contract.");
_Static_assert(sizeof(legbot_at_transaction_t) <= 3072U,
               "AT transaction queue payload must remain bounded.");
_Static_assert(sizeof(watch_modem_update_t) <= 384U,
               "Modem latest truth must remain a compact by-value payload.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.modem) ==
                   sizeof(watch_modem_update_t),
               "State queue modem payload must remain typed and size-safe.");
_Static_assert(GPS_SERVICE_FIX_FRESHNESS_MS == 10000U &&
                   WATCH_GPS_FIX_FRESHNESS_MS == GPS_SERVICE_FIX_FRESHNESS_MS,
               "GPS freshness must remain exactly ten seconds across owner and reducer.");
_Static_assert(GPS_SERVICE_SELFTEST_TIMEOUT_MS == 30000U,
               "GPS UART self-test must remain exactly thirty seconds.");
_Static_assert(GPS_SERVICE_SELFTEST_OWNER_DEADLINE_MS <
                   GPS_SERVICE_SELFTEST_TIMEOUT_MS,
               "GPS owner must return before the self-test handler deadline.");
_Static_assert(GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS == 180000U,
               "GPS fix self-test must retain the canonical 180-second window.");
_Static_assert(GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS <
                   GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS,
               "GPS fix owner must return before the outer self-test deadline.");
_Static_assert(GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS == 179500U,
               "GPS fix owner deadline must preserve a result-polling margin.");
_Static_assert((GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS -
                GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS) >=
                   (2U * GPS_SERVICE_OWNER_POLL_MS),
               "GPS fix owner deadline must leave at least two polling intervals.");
_Static_assert(GPS_SELFTEST_PROBE_UART != GPS_SELFTEST_PROBE_FIX,
               "GPS UART and FIX probes must remain distinct typed identities.");
_Static_assert(sizeof(gps_selftest_request_t) <= 32U &&
                   sizeof(gps_selftest_result_t) <= 64U,
               "GPS typed requests and terminals must remain bounded by value.");
_Static_assert(GPS_SERVICE_COMMAND_QUEUE_DEPTH == 8U &&
                   GPS_SERVICE_RESULT_QUEUE_DEPTH == 2U,
               "GPS typed command and result queues must remain bounded.");
_Static_assert(GPS_SERVICE_TASK_STACK_BYTES == 3072U,
               "GPS parser task stack budget must remain evidence-bounded.");
_Static_assert(BLE_SERVICE_TASK_STACK_BYTES == 8192U,
               "BLE owner task stack must preserve the hardware-observed overflow fix.");
_Static_assert(UI_SERVICE_TASK_STACK_BYTES == 16384U,
               "UI owner task stack must cover the measured first-frame model and LVGL call chain.");
_Static_assert(sizeof(watch_gps_update_t) <= 160U,
               "GPS latest truth must remain a bounded by-value payload.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.gps) ==
                   sizeof(watch_gps_update_t),
               "State queue GPS payload must remain typed and size-safe.");
_Static_assert(BLE_TARGET_SERVICE_UUID16 == 0x00FFU,
               "BLE target service UUID must remain 0x00FF.");
_Static_assert(BLE_TARGET_CHARACTERISTIC_UUID16 == 0xFF01U,
               "BLE target characteristic UUID must remain 0xFF01.");
_Static_assert(BLE_TARGET_CCCD_UUID16 == 0x2902U,
               "BLE target CCCD UUID must remain 0x2902.");
_Static_assert(STATUS_STALE_MS == 5000U,
               "Watch-state freshness boundary must remain exactly five seconds.");
_Static_assert(WATCH_EXOSKELETON_VERSION_CAPACITY == 17U,
               "Product version buffers must hold 16 bytes plus a terminator.");
_Static_assert(BLE_NOTIFY_ENABLE_MAX_ATTEMPTS == 3U,
               "Notify enable must allow one initial attempt and two retries.");
_Static_assert(BLE_RECONNECT_BACKOFF_COUNT == 5U,
               "BLE reconnect backoff must keep five fixed product tiers.");
_Static_assert(BLE_NOTIFY_PAYLOAD_CAPACITY == 69U,
               "BLE Notify ingress must preserve its bounded event payload.");
_Static_assert(BLE_STATUS_FRAME_LENGTH == 67U,
               "BLE status frames must match main_control 67-byte user packets.");
_Static_assert(BLE_STATUS_HEADER_FIRST == 0x55U,
               "BLE status frames must start with 0x55.");
_Static_assert(BLE_STATUS_HEADER_SECOND == 0xAAU,
               "BLE status frames must continue with 0xAA.");
_Static_assert(BLE_STATUS_VERSION_CAPACITY == 17U,
               "BLE status version buffers must include a NUL terminator.");
_Static_assert(BLE_STATUS_TIMESTAMP_LENGTH == 4U,
               "BLE status timestamps must remain opaque four-byte values.");
_Static_assert(BLE_STATUS_RESERVED_DATA_LENGTH == 9U,
               "BLE status reserved-data fields must remain nine bytes.");
_Static_assert(CONFIG_BT_NIMBLE_MAX_CONNECTIONS == 1,
               "BLE Central must keep a single connection.");
_Static_assert(BLE_EVENT_QUEUE_DEPTH > MAINTENANCE_BLE_CANDIDATE_MAX_COUNT,
               "BLE private event queue must absorb one complete candidate burst.");
_Static_assert(MAINTENANCE_BLE_CANDIDATE_MAX_COUNT == 8U,
               "Maintenance BLE candidate list must remain bounded to eight values.");
_Static_assert(MAINTENANCE_BLE_MAC_CAPACITY == 18U,
               "Maintenance MAC buffers must hold canonical text and terminator.");
_Static_assert(MAINTENANCE_BLE_NAME_CAPACITY == 32U, "Maintenance BLE names must remain bounded small values.");
_Static_assert(sizeof(((maintenance_ble_candidate_t *)0)->advertised_name) == MAINTENANCE_BLE_NAME_CAPACITY,
               "Maintenance candidates must preserve the bounded advertised name.");
_Static_assert(sizeof(maintenance_ble_candidate_list_t) <= 512U,
               "Maintenance BLE candidate snapshots must remain stack-safe "
               "bounded values.");
_Static_assert(MAINTENANCE_SELECTION_QUEUE_DEPTH == 2U,
               "Maintenance BLE selection queue must remain small and bounded.");
_Static_assert(MAINTENANCE_CLEAR_QUEUE_DEPTH == 2U,
               "Maintenance BLE clear queue must remain small and bounded.");
_Static_assert(CONFIG_SERVICE_MAC_CAPACITY == WATCH_BLE_MAC_CAPACITY,
               "Config and watch-state BLE MAC values must have the same capacity.");
_Static_assert(CONFIG_SERVICE_WATCH_ID_CAPACITY == WATCH_STATE_WATCH_ID_CAPACITY,
               "Config and watch-state identity capacities must match.");
_Static_assert(sizeof(CONFIG_SERVICE_CONFIG_EPOCH_KEY) <= 16U,
               "The cfg activation epoch key must satisfy the NVS key limit.");
_Static_assert(sizeof(CONFIG_SERVICE_CLOUD_EPOCH_KEY) <= 16U,
               "The cloud candidate epoch key must satisfy the NVS key limit.");
_Static_assert(sizeof(CONFIG_SERVICE_TOKEN_MASK) <= CONFIG_SERVICE_TOKEN_MASK_CAPACITY,
               "The fixed token mask must remain terminated and bounded.");
_Static_assert(sizeof(((maintenance_snapshot_t *)0)->watch_id) ==
                   WATCH_STATE_WATCH_ID_CAPACITY,
               "Maintenance identity projection must preserve the bounded watch ID.");
_Static_assert(sizeof(((maintenance_snapshot_t *)0)->token_mask) ==
                   CONFIG_SERVICE_TOKEN_MASK_CAPACITY,
               "Maintenance may carry only the fixed bounded token mask.");
_Static_assert(sizeof(config_service_snapshot_t) <
                   sizeof(config_service_cloud_credentials_t),
               "The public config snapshot must remain smaller than private credentials.");
_Static_assert(sizeof(((maintenance_ble_selection_command_t *)0)->mac) ==
                   MAINTENANCE_BLE_MAC_CAPACITY,
               "Maintenance selection commands must carry the complete MAC value.");
_Static_assert(sizeof(maintenance_ble_selection_command_t) <= 24U,
               "Maintenance selection commands must remain compact typed values.");
_Static_assert(SELFTEST_ITEM_COUNT == 11, "Self-test result model must cover the enabled test matrix.");
_Static_assert(UI_SELFTEST_CATEGORY_COUNT == 7,
               "Final UI must aggregate the eleven internal checks into seven categories.");
_Static_assert(SELFTEST_REGISTRY_CAPACITY >= SELFTEST_ITEM_COUNT,
               "Self-test registry must cover the matrix and bounded future extensions.");
_Static_assert(SELFTEST_MAX_TIMEOUT_MS == 180000U,
               "GPS fix remains the longest canonical 180-second self-test.");
_Static_assert(SELFTEST_SERVICE_QUEUE_DEPTH == 2U,
               "Self-test command queue must remain small so STOP cannot sit behind normal work.");
_Static_assert(sizeof(selftest_service_command_t) <= 24U,
               "Self-test run and retry commands must remain compact typed values.");
_Static_assert(sizeof(state_service_update_t) < sizeof(watch_selftest_summary_t),
               "State queue must carry a single self-test update, never the complete summary.");
_Static_assert(sizeof(watch_audio_snapshot_t) < sizeof(watch_state_snapshot_t),
               "Self-test audio observation must not copy the complete watch state.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.selftest_item) ==
                   sizeof(watch_selftest_item_update_t),
               "Self-test state queue payload must remain typed and size-safe.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.selftest_retry_finish) ==
                   sizeof(watch_selftest_retry_finish_update_t),
               "Self-test retry state payload must remain typed and size-safe.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.ble) ==
                   sizeof(watch_ble_update_t),
               "BLE state queue payload must remain typed and size-safe.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.control) ==
                   sizeof(watch_control_update_t),
               "Control state queue payload must remain typed and size-safe.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.cloud) ==
                   sizeof(watch_cloud_update_t),
               "Cloud state queue payload must remain typed and size-safe.");
_Static_assert(sizeof(watch_telemetry_update_t) <= 128U,
               "Telemetry product diagnostics must remain bounded by value.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.telemetry) ==
                   sizeof(watch_telemetry_update_t),
               "State telemetry payload must remain typed and size-safe.");
_Static_assert(STATE_SERVICE_TELEMETRY_TERMINAL_CAPACITY == 1U,
               "Telemetry terminals must use one non-overwriting retained slot.");
_Static_assert(sizeof(watch_ble_update_t) <= 256U,
               "BLE product truth must remain a bounded by-value queue payload.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.config) ==
                   sizeof(watch_config_update_t),
               "Config state queue payload must remain typed and size-safe.");
_Static_assert(sizeof(watch_config_update_t) <= 128U,
               "Identity and redacted config truth must remain a compact by-value payload.");
_Static_assert(STATE_SERVICE_APPLY_ACK_TIMEOUT_MS > STATE_SERVICE_APPLY_TIMEOUT_MS,
               "Self-test terminal ACK must cover at least one bounded state apply attempt.");
_Static_assert(POWER_SERVICE_IDLE_TIMEOUT_MS == 5000U,
               "Normal display idle timeout must remain exactly five seconds.");
_Static_assert(POWER_SERVICE_BLE_PAGE_IDLE_TIMEOUT_MS == 30000U,
               "BLE search and connection pages must use exactly thirty seconds.");
_Static_assert(CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S == 0 &&
                   CONFIG_SERVICE_UI_SCREEN_TIMEOUT_15S == 1 &&
                   CONFIG_SERVICE_UI_SCREEN_TIMEOUT_30S == 2,
               "Screen timeout ordinals must shift legacy 15/30/60 choices to 5/15/30.");
_Static_assert(UI_RUNTIME_SCREEN_TIMEOUT_5S * 1000U ==
                       POWER_SERVICE_IDLE_TIMEOUT_5S_MS &&
                   UI_RUNTIME_SCREEN_TIMEOUT_15S * 1000U ==
                       POWER_SERVICE_IDLE_TIMEOUT_15S_MS &&
                   UI_RUNTIME_SCREEN_TIMEOUT_30S * 1000U ==
                       POWER_SERVICE_IDLE_TIMEOUT_30S_MS,
               "UI seconds and power-owner milliseconds must remain one 5/15/30 mapping.");
_Static_assert(CONFIG_SERVICE_UI_DEFAULT_HAPTICS_ENABLED &&
                   !CONFIG_SERVICE_UI_DEFAULT_CLICK_AUDIO_ENABLED &&
                   !CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED &&
                   CONFIG_SERVICE_UI_DEFAULT_BRIGHTNESS ==
                       CONFIG_SERVICE_UI_BRIGHTNESS_MEDIUM &&
                   CONFIG_SERVICE_UI_DEFAULT_SCREEN_TIMEOUT ==
                       CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S,
               "Factory UI defaults must remain haptics on, audio and raise-to-wake off, medium brightness, and five seconds.");
_Static_assert(POWER_WATCH_LOW_BATTERY_ENTER_PERCENT == 30U &&
                   POWER_WATCH_LOW_BATTERY_EXIT_PERCENT == 33U,
               "Watch low-battery hysteresis must remain 30/33 percent.");
_Static_assert(
    POWER_WATCH_CRITICAL_BATTERY_ENTER_PERCENT == 15U &&
        POWER_WATCH_CRITICAL_BATTERY_EXIT_PERCENT == 18U,
    "Watch critical-battery hysteresis must remain 15/18 percent.");
_Static_assert(WATCH_EXOSKELETON_LOW_BATTERY_ENTER_PERCENT == 20U &&
                   WATCH_EXOSKELETON_LOW_BATTERY_EXIT_PERCENT == 23U,
               "Exoskeleton low-battery hysteresis must remain 20/23 percent.");
_Static_assert(WATCH_POWER_LEVEL_NORMAL == 0 &&
                   WATCH_POWER_LEVEL_LOW_BATTERY_WARN == 1 &&
                   WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY == 2 &&
                   WATCH_POWER_LEVEL_COUNT == 3,
               "Watch power level must remain a closed three-state fact.");
_Static_assert(POWER_DISPLAY_NORMAL_BRIGHTNESS ==
                   CO5300_BSP_BRIGHTNESS_DEFAULT &&
                   POWER_DISPLAY_CRITICAL_BRIGHTNESS_MAX <= 76U,
               "Critical display brightness must not exceed thirty percent.");
_Static_assert(POWER_DISPLAY_INTENT_NEXT_PAGE == 3 &&
                   POWER_DISPLAY_INTENT_STOP == 4 &&
                   POWER_DISPLAY_INTENT_COUNT == 5,
               "Power policy must not expose a software local-shutdown action.");
_Static_assert(UI_RUNTIME_INTENT_SOURCE_POWER_KEY == 2 &&
                   UI_RUNTIME_INTENT_SOURCE_COUNT == 3,
               "PWR navigation must remain a distinct non-button UI source.");
_Static_assert(POWER_SERVICE_PWR_INPUT_EVIDENCE_VERIFIED == 1U,
               "Runtime PWR decoding must remain backed by target-board evidence.");
_Static_assert(POWER_SERVICE_PWR_ACTIVE_LEVEL == 0U,
               "Target-board PWR input must remain active-low.");
_Static_assert(POWER_SERVICE_PWR_DEBOUNCE_MS == 20U,
               "Target-board PWR debounce must remain twenty milliseconds.");
_Static_assert(POWER_SERVICE_ACTIVE_RETRY_MS == 10U &&
                   POWER_SERVICE_PENDING_RETRY_MS == 20U,
               "Only active classification and pending delivery may retain short retries.");
_Static_assert(POWER_SERVICE_SCREEN_OFF_POLL_PERIOD_MS == 60000U,
               "Screen-off battery sampling must use the sixty-second low-power deadline.");
_Static_assert(POWER_EVENT_WAIT_FOREVER_MS == UINT32_MAX,
               "Power owner must represent the no-deadline state without periodic polling.");
_Static_assert(POWER_SERVICE_MESSAGE_PWR_EDGE !=
                   POWER_SERVICE_MESSAGE_QMI_WAKE &&
                   POWER_SERVICE_MESSAGE_QMI_WAKE !=
                       POWER_SERVICE_MESSAGE_STATE_CHANGE,
               "Power ISR and state wake messages must remain distinct.");
_Static_assert(WATCH_SCREEN_STATE_COUNT == 2,
               "Screen state must remain an orthogonal on/off fact.");
_Static_assert(sizeof(watch_power_snapshot_t) <
                   sizeof(watch_state_snapshot_t),
               "Power consumers must not copy the complete watch state.");
_Static_assert(sizeof(watch_battery_update_t) <= 80U,
               "Battery state updates must remain compact by-value payloads.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.battery) ==
                   sizeof(watch_battery_update_t),
               "State queue battery payload must remain typed and size-safe.");
_Static_assert(WATCH_POWER_ERROR_COUNT == 4,
               "Power errors must remain a closed typed set.");
_Static_assert(sizeof(watch_power_update_t) <= 24U,
               "Power state updates must remain compact by-value payloads.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.power) ==
                   sizeof(watch_power_update_t),
               "State queue power payload must remain typed and size-safe.");
_Static_assert(UI_SERVICE_QUEUE_DEPTH == 8U &&
                   sizeof(ui_service_request_t) <= 16U,
               "Display intents must use the fixed bounded typed UI queue.");
_Static_assert(LEGBOT_KEY_PWR_GPIO == LEGBOT_BSP_PWR_KEY_GPIO,
               "PWR input must remain owned by the KEY BSP.");
_Static_assert(LEGBOT_KEY_BOOT_GPIO == LEGBOT_BSP_BOOT_KEY_GPIO,
               "BOOT input must remain owned by the KEY BSP.");
_Static_assert(LEGBOT_BSP_VIBRATION_GPIO == GPIO_NUM_11,
               "Vibration BSP must remain the unique IO11 owner.");
_Static_assert(VIBRATION_BSP_LEDC_SPEED_MODE == LEDC_LOW_SPEED_MODE,
               "ESP32-S3 vibration PWM must use LEDC low-speed mode.");
_Static_assert(VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED == 1,
               "Vibration software profile must remain explicitly enabled.");
_Static_assert(VIBRATION_BSP_PWM_FREQUENCY_HZ == 20000U,
               "Vibration PWM must remain at the selected 20 kHz profile.");
_Static_assert(VIBRATION_BSP_ACTIVE_LEVEL == 1U &&
                   VIBRATION_BSP_IDLE_LEVEL == 0U &&
                   VIBRATION_BSP_IDLE_DUTY == 0U,
               "Vibration PWM must remain active-high and idle-low.");
_Static_assert(VIBRATION_BSP_ACTIVE_DUTY == VIBRATION_BSP_DUTY_MAX &&
                   VIBRATION_BSP_PWM_PERIOD_STEPS == 1024U &&
                   VIBRATION_BSP_ACTIVE_DUTY <= VIBRATION_BSP_DUTY_MAX,
               "Vibration self-test PWM must use the maximum valid 10-bit duty.");
_Static_assert(VIBRATION_BSP_SELFTEST_PULSE_MS == VIBRATION_BSP_PULSE_MAX_MS &&
                   VIBRATION_BSP_SELFTEST_PULSE_MS == 1000U,
               "Vibration self-test pulse must remain bounded to one second.");
_Static_assert(CO5300_BSP_WIDTH == 410, "CO5300 width must remain 410 pixels.");
_Static_assert(CO5300_BSP_HEIGHT == 502, "CO5300 height must remain 502 pixels.");
_Static_assert(CO5300_BSP_X_GAP == 22, "CO5300 X gap must remain 22 pixels.");
_Static_assert(CO5300_BSP_Y_GAP == 0, "CO5300 Y gap must remain 0 pixels.");
_Static_assert(CO5300_BSP_BITS_PER_PIXEL == 16, "CO5300 must remain RGB565.");
_Static_assert(CO5300_BSP_QSPI_CLOCK_HZ == 80000000,
               "CO5300 QSPI clock must remain at the required 80 MHz profile.");
_Static_assert(CO5300_BSP_BRIGHTNESS_MIN == 0U &&
                   CO5300_BSP_BRIGHTNESS_MAX == 255U &&
                   CO5300_BSP_BRIGHTNESS_DEFAULT ==
                       CO5300_BSP_BRIGHTNESS_MAX,
               "CO5300 brightness seam must preserve the raw 0x51 range.");
_Static_assert(CO5300_BSP_QSPI_BRIGHTNESS_COMMAND == 0x02005100U,
               "CO5300 brightness must use the component's encoded QSPI write command.");
_Static_assert(CST9217_BSP_WIDTH == CO5300_BSP_WIDTH, "Touch and display width must align.");
_Static_assert(CST9217_BSP_HEIGHT == CO5300_BSP_HEIGHT, "Touch and display height must align.");
_Static_assert(LEGBOT_BSP_CST9217_RST_GPIO == GPIO_NUM_38, "CST9217 reset must be IO38.");
_Static_assert(LEGBOT_BSP_CST9217_INT_GPIO == GPIO_NUM_39, "CST9217 interrupt must be IO39.");
_Static_assert(UI_SERVICE_MAX_LOOP_DELAY_MS == 1000,
               "Idle UI timer work must use a one-second deadline and interrupt wakeups.");
_Static_assert(UI_PAGE_COUNT == 23,
               "watch-lvgl.pen defines exactly twenty-three UI pages and states.");
_Static_assert(UI_SHELL_SLOT_COUNT == 4,
               "The daily shell must keep home, gear, mode/power, and alerts slots.");
_Static_assert(sizeof(((state_service_update_t *)0)->payload.audio) == sizeof(watch_audio_update_t),
               "Audio state queue payload must remain typed and size-safe.");
_Static_assert(AUDIO_RESOURCE_COUNT == 3,
               "Audio resource catalog must contain selftest, UI click, and voice cue WAV files.");
_Static_assert(AUDIO_PRIORITY_DEFAULT == 0 &&
                   AUDIO_PRIORITY_SELFTEST == 1,
               "Critical battery audio admission depends on the closed priorities.");
_Static_assert(AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS > 0 && AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS <= 20,
               "Audio I2S writes must remain bounded to at most 20 ms.");
_Static_assert(AUDIO_BSP_DMA_FRAME_NUM * 1000U <=
                   AUDIO_BSP_SAMPLE_RATE_HZ * AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS,
               "I2S DMA reclaim interval must fit the bounded write timeout.");
_Static_assert(LEGBOT_BSP_NS4150_PA_ACTIVE_LEVEL != LEGBOT_BSP_NS4150_PA_INACTIVE_LEVEL,
               "NS4150 active and inactive levels must differ.");

void legbot_contract_checks_link_anchor(void)
{
}
