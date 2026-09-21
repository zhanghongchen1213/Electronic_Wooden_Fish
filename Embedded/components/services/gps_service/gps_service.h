/**
 * @file     gps_service.h
 * @brief    GPS 服务公共接口
 * @details  定义固定 gps_task 的有界 NMEA、定位 freshness 与后续 typed 生命周期合同。
 * @author   ZHC
 * @date     2026-07-14
 */

#ifndef LEGBOT_GPS_SERVICE_H
#define LEGBOT_GPS_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gps_acquisition_policy.h"

#ifndef GPS_SERVICE_TEST
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"
#endif

#ifndef GPS_SERVICE_TEST
/** GPS 服务在统一服务表中的固定 ID。 */
#define LEGBOT_GPS_SERVICE_ID LEGBOT_SERVICE_GPS
#endif
/** NMEA 单行固定容量，包含字符串终止符。 */
#define GPS_SERVICE_NMEA_LINE_CAPACITY 128U
/** 单次从 BSP 搬运到 parser 的有界字节数。 */
#define GPS_SERVICE_UART_READ_CHUNK_BYTES 64U
/** 合法 fix 的严格新鲜窗口毫秒数。 */
#define GPS_SERVICE_FIX_FRESHNESS_MS 10000U
/** NMEA 句最大字段数量，覆盖带 Signal ID 的四卫星 GSV。 */
#define GPS_SERVICE_NMEA_MAX_FIELDS 21U
/** RMC 完整结构至少覆盖到磁偏角方向字段。 */
#define GPS_SERVICE_RMC_MIN_FIELDS 12U
/** GGA 完整结构必须覆盖全部标准字段。 */
#define GPS_SERVICE_GGA_MIN_FIELDS 15U
/** 固定 gps_task 的 IDF 字节口径栈预算。 */
#define GPS_SERVICE_TASK_STACK_BYTES 3072U
/** GPS owner 私有 typed 命令队列深度。 */
#define GPS_SERVICE_COMMAND_QUEUE_DEPTH 8U
/** GPS owner 私有 typed 自检结果队列深度。 */
#define GPS_SERVICE_RESULT_QUEUE_DEPTH 2U
/** GPS owner 单次 UART/命令轮询的最大等待。 */
#define GPS_SERVICE_OWNER_POLL_MS 20U
/** GPS UART 自检 canonical 总预算。 */
#define GPS_SERVICE_SELFTEST_TIMEOUT_MS 30000U
/** GPS owner 在 handler 总预算前返回终态的截止预算。 */
#define GPS_SERVICE_SELFTEST_OWNER_DEADLINE_MS 29500U
/** GPS 首次定位自检 canonical 总预算。 */
#define GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS 180000U
/** GPS owner 在首次定位总预算前返回终态的截止预算。 */
#define GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS 179500U
/** 启动后的首次自动搜星预算。 */
#define GPS_SERVICE_INITIAL_ACQUISITION_MS \
    GPS_ACQUISITION_INITIAL_BUDGET_MS
/** 首次失败后的每次重试搜星预算。 */
#define GPS_SERVICE_RETRY_ACQUISITION_MS \
    GPS_ACQUISITION_RETRY_BUDGET_MS
/** 当前会话没有合法 NMEA 时提前结束的通信预算。 */
#define GPS_SERVICE_NMEA_STARTUP_DEADLINE_MS 30000U
/** 当前会话在通信预算内没有合法 NMEA 时的稳定失败码。 */
#define GPS_SERVICE_ERROR_NMEA_TIMEOUT "GPS_NMEA_TIMEOUT"
/** v2 运动状态下相邻自动定位的固定间隔。 */
#define GPS_SERVICE_MOTION_INTERVAL_MS GPS_ACQUISITION_MOTION_INTERVAL_MS
/** v2 最近运动事件后的静止宽限。 */
#define GPS_SERVICE_MOTION_GRACE_MS GPS_ACQUISITION_MOTION_GRACE_MS
/** 手动或亮屏提前搜星后的主动冷却。 */
#define GPS_SERVICE_ACTIVE_TRIGGER_COOLDOWN_MS \
    GPS_ACQUISITION_ACTIVE_TRIGGER_COOLDOWN_MS
/** GPS owner 退出前保留最终 off 真值的有界尝试次数。 */
#define GPS_SERVICE_FINAL_STATE_PUBLISH_ATTEMPTS 4U
/** 最终 off 真值发布重试之间的调度让出毫秒数。 */
#define GPS_SERVICE_FINAL_STATE_PUBLISH_RETRY_DELAY_MS 1U
/** GPS typed 自检 detail code 固定容量，包含字符串终止符。 */
#define GPS_SELFTEST_DETAIL_CODE_CAPACITY 40U

typedef enum
{
    GPS_SELFTEST_OUTCOME_PASS = 0, /**< UART 出现新完整目标 NMEA 帧，或 FIX 拥有当前 fresh 定位。 */
    GPS_SELFTEST_OUTCOME_FAIL,     /**< owner 超时、取消或内部读取失败。 */
    GPS_SELFTEST_OUTCOME_SKIP,     /**< GPS driver 或 WAKE 产品策略不可用。 */
    GPS_SELFTEST_OUTCOME_INCONCLUSIVE, /**< UART 正常但 180 秒窗口内尚未形成导航解。 */
} gps_selftest_outcome_t;

typedef enum
{
    GPS_PURPOSE_TIME_SYNC = 0, /**< 获取可信 UTC；v1 成功后进入待机。 */
    GPS_PURPOSE_LOCATION,      /**< 执行 v2 运动窗口内的单次定位。 */
    GPS_PURPOSE_SELFTEST,      /**< 为整机自检提供共享观察会话。 */
} gps_acquisition_purpose_t;

typedef enum
{
    GPS_TRIGGER_BOOT = 0,      /**< BLE 与音频就绪后的首次自动搜星。 */
    GPS_TRIGGER_SCHEDULED,     /**< 退避截止后的自动重试。 */
    GPS_TRIGGER_SCREEN_WAKE,   /**< 真实熄屏到亮屏的机会请求。 */
    GPS_TRIGGER_MANUAL,        /**< 维护页主动请求。 */
    GPS_TRIGGER_SELFTEST,      /**< 自检观察者请求。 */
    GPS_TRIGGER_MOTION,        /**< QMI 或 BLE 步数提供的新运动证据。 */
} gps_acquisition_trigger_t;

typedef enum
{
    GPS_SELFTEST_PROBE_UART = 0, /**< 验证 UART 与完整 RMC/GGA 帧活动。 */
    GPS_SELFTEST_PROBE_FIX,      /**< 验证当前 fresh 真实定位。 */
} gps_selftest_probe_kind_t;

typedef enum
{
    GPS_SELFTEST_CONTEXT_DEFAULT = 0,      /**< 默认室外或靠窗定位条件。 */
    GPS_SELFTEST_CONTEXT_INDOOR_CONFIRMED, /**< 操作员已显式确认室内条件。 */
} gps_selftest_context_t;

typedef struct
{
    uint32_t run_id;                /**< selftest_service 运行 ID。 */
    uint32_t request_id;            /**< 调用方本次非零请求 ID。 */
    uint32_t probe_generation;      /**< 调用方本次非零 probe 代次。 */
    uint64_t started_at_ms;         /**< handler 发起 probe 的本地单调毫秒。 */
    gps_selftest_probe_kind_t kind; /**< UART 或 FIX typed probe 类型。 */
    gps_selftest_context_t context; /**< 定位环境的显式 typed 上下文。 */
} gps_selftest_request_t;

typedef struct
{
    uint32_t run_id;                                     /**< 与请求匹配的 selftest_service 运行 ID。 */
    uint32_t request_id;                                 /**< 与请求匹配的请求 ID。 */
    uint32_t probe_generation;                           /**< 与请求匹配的 probe 代次。 */
    gps_selftest_probe_kind_t kind;                      /**< 与请求匹配的 UART/FIX 类型。 */
    gps_selftest_outcome_t outcome;                      /**< pass/fail/skip 终态。 */
    char detail_code[GPS_SELFTEST_DETAIL_CODE_CAPACITY]; /**< GPS_/DRV_ 稳定终态码。 */
} gps_selftest_result_t;

#ifndef GPS_SERVICE_TEST
/**
 * @brief 初始化 GPS owner 私有有界队列
 * @return ESP_OK 成功，其他值表示内存或生命周期状态错误
 */
esp_err_t gps_service_init_contracts(void);

/**
 * @brief 回收未运行 GPS owner 的私有有界队列
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 表示任务仍活动
 */
esp_err_t gps_service_deinit_contracts(void);

/**
 * @brief 为固定 gps_task 准备一次运行
 * @return ESP_OK 已准备，其他值表示资源或生命周期错误
 */
esp_err_t gps_service_prepare(void);

/** @brief 取消尚未启动的 prepared GPS run。 */
void gps_service_cancel_prepared_run(void);

/**
 * @brief 请求 GPS owner 停止并使 BSP 有界读取尽快返回
 * @param timeout_ticks 尝试投递可选唤醒命令的有界 tick
 * @return ESP_OK 原子停止请求已生效，ESP_ERR_INVALID_STATE 表示任务未活动
 */
esp_err_t gps_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 异步请求一次 GPS 搜星或复用当前会话
 * @param purpose 授时、运动定位或自检用途
 * @param trigger 启动、定时、亮屏、手动或自检触发
 * @param timeout_ticks 仅用于命令入队的有界等待，不是搜星预算
 * @return ESP_OK 已入队
 *         ESP_ERR_INVALID_ARG 参数非法
 *         ESP_ERR_INVALID_STATE 服务未运行或已满足请求
 *         ESP_ERR_TIMEOUT 队列拥塞
 */
esp_err_t gps_service_request_acquisition(gps_acquisition_purpose_t purpose,
                                          gps_acquisition_trigger_t trigger,
                                          TickType_t timeout_ticks);

/**
 * @brief 提交一次运动证据并按十分钟节拍触发 v2 定位
 * @param timeout_ticks 等待 GPS 私有命令队列空间的有界 tick
 * @return ESP_OK 已提交，其他值表示 profile、生命周期或队列不可用
 */
esp_err_t gps_service_notify_motion(TickType_t timeout_ticks);

/**
 * @brief 请求自动定位为 LTE RF 会话立即回到 standby
 * @param timeout_ticks 等待 GPS 私有命令队列空间的有界 tick
 * @return ESP_OK 已提交，其他值表示生命周期或队列不可用
 */
esp_err_t gps_service_pause_for_radio(TickType_t timeout_ticks);

/**
 * @brief 运行固定 gps_task 的唯一 UART/parser owner 循环
 * @return ESP_OK 正常停止，其他值表示清理或状态错误
 */
esp_err_t gps_service_run(void);

/**
 * @brief 向 GPS owner 发起 UART 或 FIX typed 自检
 * @param request 运行 ID、请求 ID、probe 代次、类型、上下文与开始时间
 * @param timeout_ticks 尝试投递可选唤醒命令的有界 tick
 * @return ESP_OK 请求已入队，其他值表示参数、任务状态或队列空间不可用
 */
esp_err_t gps_service_request_selftest(const gps_selftest_request_t *request,
                                       TickType_t timeout_ticks);

/**
 * @brief 取消仍匹配的 GPS UART 或 FIX 自检 probe
 * @param request 原始 typed 请求身份
 * @param timeout_ticks 等待私有命令队列空间的有界 tick
 * @return ESP_OK 取消身份已原子锁存，其他值表示参数或任务状态无效
 */
esp_err_t gps_service_cancel_selftest(const gps_selftest_request_t *request,
                                      TickType_t timeout_ticks);

/**
 * @brief 有界领取 GPS owner 自检终态
 * @param result 自检终态输出
 * @param timeout_ticks 单次有界等待 tick
 * @return ESP_OK 已领取，其他值表示参数、状态或本次等待超时
 */
esp_err_t gps_service_receive_selftest_result(gps_selftest_result_t *result,
                                              TickType_t timeout_ticks);

/**
 * @brief 获取当前有效的 GPS 定位坐标
 * @param latitude 纬度输出（WGS84），坐标无效时写 0
 * @param longitude 经度输出（WGS84），坐标无效时写 0
 * @return ESP_OK 坐标有效且在 freshness 窗口内
 *         ESP_ERR_INVALID_ARG 输出指针无效
 *         ESP_ERR_NOT_FOUND 尚无有效定位或已过期
 */
esp_err_t gps_service_get_coordinates(double *latitude, double *longitude);
#endif

#ifdef GPS_SERVICE_TEST
#if defined(_WIN32)
#define GPS_TEST_API __declspec(dllexport)
#else
#define GPS_TEST_API __attribute__((visibility("default")))
#endif
/** @brief 重置 production parser host seam。 */
GPS_TEST_API void gps_service_test_reset(void);

/**
 * @brief 评估维护页手动搜星资格
 * @param session_state GPS owner 内部会话状态数值
 * @param continuous_location 是否为 v2 连续定位 profile
 * @param time_valid 当前是否已有可信时间
 * @param now_ms 当前本地单调毫秒
 * @param cooldown_deadline_ms 主动搜星冷却截止单调毫秒
 * @return 1 允许手动搜星，0 不允许或状态参数非法
 */
GPS_TEST_API int gps_service_test_manual_request_available(
    uint8_t session_state,
    bool continuous_location,
    bool time_valid,
    uint64_t now_ms,
    uint64_t cooldown_deadline_ms);

/**
 * @brief 设置 host seam 的服务与驱动可用状态
 * @param running 服务是否明确运行
 * @param available BSP driver 是否可用
 */
GPS_TEST_API void gps_service_test_set_driver(bool running, bool available);

/**
 * @brief 向 production parser 喂入 UART 字节分片
 * @param bytes 输入字节
 * @param length 输入长度
 * @param now_ms 本批字节的本地单调接收毫秒
 */
GPS_TEST_API void gps_service_test_feed(const uint8_t *bytes,
                                        size_t length,
                                        uint64_t now_ms);

/**
 * @brief 在指定单调时间刷新并读取状态
 * @param now_ms 当前本地单调毫秒
 * @return 0=off、1=searching、2=fixed、3=unavailable
 */
GPS_TEST_API int gps_service_test_status(uint64_t now_ms);

/**
 * @brief 获取当前仍有效的坐标
 * @param latitude 纬度输出
 * @param longitude 经度输出
 * @return 1 坐标有效，0 坐标 absent
 */
GPS_TEST_API int gps_service_test_coordinates(double *latitude,
                                              double *longitude);

/**
 * @brief 获取完整且结构合法的目标 NMEA 接收序号
 * @return 启动周期内单调非递减序号
 */
GPS_TEST_API uint32_t gps_service_test_nmea_sequence(void);

/**
 * @brief 获取完整目标 NMEA 帧接收序号
 * @details 帧级证据不要求 RMC/GGA 已满足定位语义，仅要求目标类型、行边界与可选 checksum 完整可信。
 * @return 启动周期内单调非递减序号
 */
GPS_TEST_API uint32_t gps_service_test_nmea_frame_sequence(void);

/**
 * @brief 获取最近完整合法目标 NMEA 的本地接收时间
 * @return 单调毫秒；尚无证据时为 0
 */
GPS_TEST_API uint64_t gps_service_test_last_nmea_ms(void);

/**
 * @brief 获取结构非法目标 NMEA 的接收序号
 * @return 启动周期内单调非递减畸形目标句序号
 */
GPS_TEST_API uint32_t gps_service_test_malformed_sequence(void);

/**
 * @brief 获取指定星系累计收到的 GSV 句数
 * @param talker P=GPS、B=北斗、L=GLONASS、N=多星系
 * @return 当前 parser 周期内的累计句数
 */
GPS_TEST_API uint32_t gps_service_test_gsv_sentence_count(char talker);

/**
 * @brief 获取指定星系当前 GSV 周期中具有非空 C/N0 的卫星数
 * @param talker P=GPS、B=北斗、L=GLONASS、N=多星系
 * @return 当前 GSV 周期的实际跟踪卫星数
 */
GPS_TEST_API uint8_t gps_service_test_tracked_satellites(char talker);

/**
 * @brief 获取指定星系当前 GSV 周期中 C/N0 最高的卫星 ID
 * @param talker P=GPS、B=北斗、L=GLONASS、N=多星系
 * @return 卫星 ID；尚无非空 C/N0 时为 0
 */
GPS_TEST_API uint16_t gps_service_test_strongest_satellite_id(char talker);

/**
 * @brief 获取指定 GSA System ID 最近一次参与解算的卫星数量
 * @param system_id NMEA System ID，1=GPS、2=GLONASS、4=北斗、5=QZSS
 * @return 参与解算卫星数量
 */
GPS_TEST_API uint8_t gps_service_test_gsa_used_satellites(uint8_t system_id);

/**
 * @brief 获取指定 GSA System ID 最近一次参与解算的卫星 ID
 * @param system_id NMEA System ID
 * @param index 从 0 开始的卫星索引
 * @return 卫星 ID；参数越界或该位置为空时为 0
 */
GPS_TEST_API uint16_t gps_service_test_gsa_satellite_id(uint8_t system_id,
                                                       uint8_t index);

/**
 * @brief 获取最近一次目标 NMEA 拒绝原因
 * @return 稳定 ASCII 原因码；尚无拒绝时返回 GPS_NMEA_REJECT_NONE
 */
GPS_TEST_API const char *gps_service_test_last_reject_reason(void);

/**
 * @brief 判断 baseline 后是否出现新完整目标 NMEA 帧
 * @param baseline_sequence probe 启动时捕获的 NMEA frame sequence
 * @return 1 已出现新证据，0 尚未出现
 */
GPS_TEST_API int gps_service_test_probe_completed(uint32_t baseline_sequence);

/**
 * @brief 评估 production probe 的 pass/unavailable/cancel/timeout 决策
 * @param baseline_sequence probe 启动时的 NMEA frame sequence
 * @param baseline_malformed_sequence probe 启动时的畸形目标句 sequence
 * @param driver_available 当前 driver 是否可用
 * @param cancelled 本次请求是否已取消
 * @param elapsed_ms owner 已执行毫秒数
 * @return 0=pending、1=pass、2=unavailable、3=cancelled、4=timeout、5=malformed
 */
GPS_TEST_API int gps_service_test_probe_decision(uint32_t baseline_sequence,
                                                 uint32_t baseline_malformed_sequence,
                                                 bool driver_available,
                                                 bool cancelled,
                                                 uint32_t elapsed_ms);

/**
 * @brief 使用可注入单调时间评估 production FIX probe
 * @param driver_available 当前 driver 是否可用
 * @param cancelled 本次请求是否已取消
 * @param indoor_confirmed 操作员是否已显式确认室内
 * @param elapsed_ms owner 已执行毫秒数
 * @param now_ms 注入的当前单调毫秒
 * @return 0=pending、1=pass、2=unavailable、3=cancelled、4=timeout、6=indoor skip
 */
GPS_TEST_API int gps_service_test_fix_probe_decision(bool driver_available,
                                                     bool cancelled,
                                                     bool indoor_confirmed,
                                                     uint32_t elapsed_ms,
                                                     uint64_t now_ms);

/**
 * @brief 为 host parser 开始新的授时会话代次
 * @param generation 非零会话代次
 */
GPS_TEST_API void gps_service_test_begin_session(uint32_t generation);

/**
 * @brief 查询是否已经形成连续双帧 GPS 时间
 * @param utc_ms 可选的第二帧 UTC 毫秒输出
 * @param rx_ms 可选的第二帧接收单调毫秒输出
 * @return 1 已形成可信样本，0 尚未形成
 */
GPS_TEST_API int gps_service_test_time_sample(int64_t *utc_ms,
                                              uint64_t *rx_ms);
#endif

#endif /* LEGBOT_GPS_SERVICE_H */
