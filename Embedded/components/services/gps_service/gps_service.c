/**
 * @file     gps_service.c
 * @brief    GPS 服务与 NMEA 定位状态机实现
 * @details  由固定 gps_task 独占 BSP transport，并在固定缓冲中解析 RMC/GGA 与维护 10 秒 freshness。
 * @author   ZHC
 * @date     2026-07-14
 */

#include "gps_service.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GPS_SERVICE_TEST
#include <stdatomic.h>
#include "app_state.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "l76kb_a58_bsp.h"
#include "radio_power_arbiter.h"
#include "state_service.h"
#include "time_service.h"
#endif

#ifndef GPS_SERVICE_TEST
static const char *TAG = "SVC_GPS";
#endif

/** 启动后优先输出的目标 NMEA 拒绝日志数量。 */
#define GPS_SERVICE_REJECT_LOG_BURST 3U
/** 拒绝日志突发额度耗尽后的最小输出间隔。 */
#define GPS_SERVICE_REJECT_LOG_INTERVAL_MS 10000U
/** FIX 自检期间卫星诊断进度日志间隔。 */
#define GPS_SERVICE_FIX_PROGRESS_LOG_INTERVAL_MS 15000U
/** GSA System ID 最大值为 QZSS 的 5，数组额外保留 0 作为未知系统。 */
#define GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT 6U
/** 单条 GSA 最多携带的参与解算卫星数量。 */
#define GPS_SERVICE_GSA_SATELLITES_MAX 12U
/** 转态诊断保留的最近完整 NMEA 句数量，覆盖 L76K 一个典型输出历元。 */
#define GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT 12U
/** 双帧 GPS 授时允许的最小接收间隔。 */
#define GPS_SERVICE_TIME_SAMPLE_MIN_INTERVAL_MS 500U
/** 双帧 GPS 授时允许的最大接收间隔。 */
#define GPS_SERVICE_TIME_SAMPLE_MAX_INTERVAL_MS 1500U
/** GPS 授时允许的最早产品日期。 */
#define GPS_SERVICE_TIME_MIN_YEAR 2024U
/** GPS RMC 两位年份大于等于该值时解释为 1900 年代，用于拒绝 1980 默认日期。 */
#define GPS_SERVICE_TIME_YEAR_PIVOT 80U
/** GPS 时间样本的最大 UTC 毫秒。 */
#define GPS_SERVICE_UTC_MAX_MS INT64_C(9007199254740991)
typedef enum
{
    GPS_PARSER_STATUS_OFF = 0,    /**< 服务明确关闭。 */
    GPS_PARSER_STATUS_SEARCHING,  /**< 驱动可用但没有新鲜 fix。 */
    GPS_PARSER_STATUS_FIXED,      /**< 最近 10 秒内存在合法 fix。 */
    GPS_PARSER_STATUS_UNAVAILABLE /**< 驱动或板级资源不可用。 */
} gps_parser_status_t;

typedef enum
{
    GPS_PROBE_PENDING = 0, /**< 尚无新证据且仍在 owner 预算内。 */
    GPS_PROBE_PASS,        /**< probe 后出现新完整合法目标句。 */
    GPS_PROBE_UNAVAILABLE, /**< driver 当前不可用。 */
    GPS_PROBE_CANCELLED,   /**< typed probe 已被取消。 */
    GPS_PROBE_TIMEOUT,     /**< owner deadline 已到。 */
    GPS_PROBE_MALFORMED,   /**< owner deadline 前只出现畸形目标 NMEA。 */
    GPS_PROBE_INDOOR_SKIP, /**< 操作员通过 typed 上下文确认室内。 */
} gps_probe_decision_t;

typedef enum
{
    GPS_SESSION_STANDBY = 0, /**< WAKE 低电平，等待触发或截止时间。 */
    GPS_SESSION_ACQUIRING,   /**< 接收机工作并等待授时或首次定位。 */
    GPS_SESSION_TRACKING,    /**< v2 已定位并持续读取位置。 */
    GPS_SESSION_BACKOFF,     /**< 自动退避等待阶段。 */
    GPS_SESSION_UNAVAILABLE, /**< 最近一次 UART 或驱动启动不可用。 */
} gps_session_state_t;

typedef enum
{
    GPS_NMEA_REJECT_NONE = 0,        /**< 尚未拒绝目标 NMEA。 */
    GPS_NMEA_REJECT_CR_SEQUENCE,     /**< CR 后未紧跟 LF。 */
    GPS_NMEA_REJECT_NONPRINTABLE,    /**< 行内出现非可打印字节。 */
    GPS_NMEA_REJECT_LINE_OVERFLOW,   /**< 单行超过固定缓冲容量。 */
    GPS_NMEA_REJECT_TARGET_ENVELOPE, /**< 目标类型后缺少字段分隔符。 */
    GPS_NMEA_REJECT_FIELD_SPLIT,     /**< 字段数量超过固定解析上限。 */
    GPS_NMEA_REJECT_CHECKSUM_SYNTAX, /**< 可选 checksum 不是 *HH 形式。 */
    GPS_NMEA_REJECT_CHECKSUM_VALUE,  /**< 可选 checksum 与 NMEA XOR 不匹配。 */
    GPS_NMEA_REJECT_RMC_FIELDS,      /**< RMC 缺少状态或 fixed 必需字段。 */
    GPS_NMEA_REJECT_RMC_STATUS,      /**< RMC 状态不是 A/V/D。 */
    GPS_NMEA_REJECT_RMC_SEARCHING,   /**< RMC 无定位字段存在结构冲突。 */
    GPS_NMEA_REJECT_RMC_FIXED,       /**< RMC 有效定位字段未通过严格校验。 */
    GPS_NMEA_REJECT_GGA_FIELDS,      /**< GGA 缺少质量或 fixed 必需字段。 */
    GPS_NMEA_REJECT_GGA_QUALITY,     /**< GGA fix quality 不是单个数字。 */
    GPS_NMEA_REJECT_GGA_SEARCHING,   /**< GGA 无定位字段存在结构冲突。 */
    GPS_NMEA_REJECT_GGA_FIXED,       /**< GGA 有效定位字段未通过严格校验。 */
} gps_nmea_reject_reason_t;

typedef struct
{
    char sentence[GPS_SERVICE_NMEA_LINE_CAPACITY]; /**< checksum 校验通过的原始 NMEA 句。 */
    uint64_t received_at_ms;                       /**< 该句到达时的本地单调毫秒。 */
} gps_nmea_history_entry_t;

typedef struct
{
    char line[GPS_SERVICE_NMEA_LINE_CAPACITY]; /**< 当前 NMEA 行的固定缓冲。 */
    size_t line_length;                        /**< 当前已保存的可打印 ASCII 字节数。 */
    bool dropping_overflow;                    /**< 超长行恢复前丢弃到下一换行。 */
    bool pending_cr;                           /**< 已收到且仅允许由 LF 紧随的行尾 CR。 */
    gps_nmea_history_entry_t diagnostic_history
        [GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT]; /**< 最近一个接收机输出历元的原始句环。 */
    uint8_t diagnostic_history_next;           /**< 下一条原始诊断句的写入位置。 */
    uint8_t diagnostic_history_count;          /**< 当前有效原始诊断句数量。 */
    bool service_running;                      /**< 服务是否处于明确运行状态。 */
    bool driver_available;                     /**< BSP driver 是否可读。 */
    bool coordinates_valid;                    /**< 当前坐标是否仍在 freshness 窗口内。 */
    bool valid_utc_seen;                        /**< 本次运行是否见过合法非空 UTC。 */
    bool nonzero_cn0_seen;                      /**< 本次运行是否见过非空 C/N0。 */
    bool fix_ever_accepted;                     /**< 本次运行是否已接受过有效定位。 */
    double latitude;                           /**< 有效时的 WGS84 纬度。 */
    double longitude;                          /**< 有效时的 WGS84 经度。 */
    uint64_t uart_rx_bytes;                     /**< 本次运行累计从 UART 接收的字节数。 */
    uint64_t first_uart_rx_ms;                  /**< 本次运行首批 UART 字节到达时间。 */
    uint64_t last_uart_rx_ms;                   /**< 本次运行最近一批 UART 字节到达时间。 */
    uint64_t longest_uart_gap_ms;               /**< 相邻两批 UART 字节的最长间隔。 */
    uint32_t uart_tx_command_count;             /**< 主机向 GPS 发送的命令数量，当前应始终为零。 */
    uint64_t last_nmea_rx_ms;                  /**< 最近合法目标句接收时间。 */
    uint64_t last_fix_rx_ms;                   /**< 最近合法 fix 接收时间。 */
    uint32_t nmea_frame_sequence;              /**< 完整目标 NMEA 帧接收序号。 */
    uint32_t nmea_sequence;                    /**< 合法目标句接收序号。 */
    uint32_t malformed_target_sequence;        /**< 结构非法目标句接收序号。 */
    uint32_t rejection_count;                  /**< 本次运行累计目标 NMEA 拒绝数量。 */
    uint64_t last_rejection_log_ms;            /**< 最近拒绝日志的单调毫秒。 */
    char last_rmc_status;                       /**< 最近合法 RMC 的 A/V/D 状态。 */
    uint8_t last_gga_quality;                   /**< 最近合法 GGA 的定位质量。 */
    uint8_t last_gga_satellites_used;           /**< 最近合法 GGA 的参与解算卫星数。 */
    uint8_t gps_visible_satellites;              /**< 最近 GPGSV 报告的可见卫星数。 */
    uint8_t beidou_visible_satellites;           /**< 最近 BDGSV 报告的可见卫星数。 */
    uint8_t glonass_visible_satellites;          /**< 最近 GLGSV 报告的可见卫星数。 */
    uint8_t multi_visible_satellites;            /**< 最近 GNGSV 报告的可见卫星数。 */
    uint8_t gps_max_cn0;                         /**< 当前 GPGSV 周期最大载噪比。 */
    uint8_t beidou_max_cn0;                      /**< 当前 BDGSV 周期最大载噪比。 */
    uint8_t glonass_max_cn0;                     /**< 当前 GLGSV 周期最大载噪比。 */
    uint8_t multi_max_cn0;                       /**< 当前 GNGSV 周期最大载噪比。 */
    uint32_t gps_gsv_sentence_count;             /**< 本次诊断累计收到的 GPGSV 句数。 */
    uint32_t beidou_gsv_sentence_count;          /**< 本次诊断累计收到的 BDGSV 句数。 */
    uint32_t glonass_gsv_sentence_count;         /**< 本次诊断累计收到的 GLGSV 句数。 */
    uint32_t multi_gsv_sentence_count;           /**< 本次诊断累计收到的 GNGSV 句数。 */
    uint8_t gps_tracked_satellites;              /**< 当前 GPGSV 周期具有非空 C/N0 的卫星数。 */
    uint8_t beidou_tracked_satellites;           /**< 当前 BDGSV 周期具有非空 C/N0 的卫星数。 */
    uint8_t glonass_tracked_satellites;          /**< 当前 GLGSV 周期具有非空 C/N0 的卫星数。 */
    uint8_t multi_tracked_satellites;            /**< 当前 GNGSV 周期具有非空 C/N0 的卫星数。 */
    uint16_t gps_strongest_satellite_id;         /**< 当前 GPGSV 周期最高 C/N0 的卫星 ID。 */
    uint16_t beidou_strongest_satellite_id;      /**< 当前 BDGSV 周期最高 C/N0 的卫星 ID。 */
    uint16_t glonass_strongest_satellite_id;     /**< 当前 GLGSV 周期最高 C/N0 的卫星 ID。 */
    uint16_t multi_strongest_satellite_id;       /**< 当前 GNGSV 周期最高 C/N0 的卫星 ID。 */
    uint8_t last_gsa_fix_mode;                   /**< 最近 GSA 的 1/2/3 定位模式。 */
    uint8_t last_gsa_system_id;                  /**< 最近 GSA 的 GNSS System ID。 */
    uint8_t gsa_used_satellites[GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT]; /**< 各 System ID 最近一次 GSA 的参与解算卫星数。 */
    uint16_t gsa_satellite_ids[GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT]
                              [GPS_SERVICE_GSA_SATELLITES_MAX]; /**< 各 System ID 最近一次 GSA 的参与解算卫星 ID。 */
    double last_gsa_pdop;                        /**< 最近 GSA 的位置精度因子。 */
    double last_gsa_hdop;                        /**< 最近 GSA 的水平精度因子。 */
    double last_gsa_vdop;                        /**< 最近 GSA 的垂直精度因子。 */
    uint32_t receiver_boot_sequence;             /**< TXT MA=CASIC 接收序号。 */
    uint32_t antenna_open_count;                 /**< 本次运行累计 ANTENNA OPEN 诊断句数量。 */
    uint64_t last_accepted_fix_rx_ms;            /**< 本次运行最近一次接受定位的时间，过期后仍保留诊断。 */
    bool time_candidate_valid;                   /**< 是否已有第一帧可信 RMC 时间候选。 */
    int64_t time_candidate_utc_ms;               /**< 第一帧可信 RMC UTC 毫秒。 */
    uint64_t time_candidate_rx_ms;               /**< 第一帧可信 RMC 接收单调毫秒。 */
    uint32_t time_candidate_session_generation;  /**< 第一帧候选所属搜星会话代次。 */
    bool time_sample_ready;                      /**< 是否已形成连续两帧可信 GPS 时间。 */
    int64_t time_sample_utc_ms;                  /**< 第二帧可信 RMC UTC 毫秒。 */
    uint64_t time_sample_rx_ms;                  /**< 第二帧可信 RMC 接收单调毫秒。 */
    uint32_t time_sample_sequence;               /**< 成功形成时间样本的单调序号。 */
    uint32_t active_session_generation;          /**< owner 当前搜星会话代次。 */
    gps_nmea_reject_reason_t last_reject_reason; /**< 最近一次目标 NMEA 拒绝原因。 */
} gps_parser_owner_t;

/** gps_task 独占的 parser 与局部定位事实。 */
static gps_parser_owner_t s_parser;

#ifndef GPS_SERVICE_TEST
_Static_assert(GPS_SERVICE_UART_READ_CHUNK_BYTES < GPS_SERVICE_NMEA_LINE_CAPACITY,
               "GPS UART chunk must fit below the fixed NMEA line capacity");
_Static_assert(GPS_SERVICE_SELFTEST_OWNER_DEADLINE_MS <
                   GPS_SERVICE_SELFTEST_TIMEOUT_MS,
               "GPS owner deadline must leave selftest scheduling margin");
_Static_assert(GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS <
                   GPS_SERVICE_FIX_SELFTEST_TIMEOUT_MS,
               "GPS fix owner deadline must leave selftest scheduling margin");
_Static_assert((int)TIME_SOURCE_NONE == (int)WATCH_TIME_SOURCE_NONE &&
                   (int)TIME_SOURCE_GPS == (int)WATCH_TIME_SOURCE_GPS &&
                   (int)TIME_SOURCE_CLOUD == (int)WATCH_TIME_SOURCE_CLOUD,
               "Time source projection enums must remain aligned");
_Static_assert((int)GPS_PURPOSE_TIME_SYNC ==
                       (int)WATCH_GPS_PURPOSE_TIME_SYNC &&
                   (int)GPS_PURPOSE_LOCATION ==
                       (int)WATCH_GPS_PURPOSE_LOCATION &&
                   (int)GPS_PURPOSE_SELFTEST ==
                       (int)WATCH_GPS_PURPOSE_SELFTEST,
               "GPS purpose projection enums must remain aligned");
_Static_assert((int)GPS_TRIGGER_BOOT == (int)WATCH_GPS_TRIGGER_BOOT &&
                   (int)GPS_TRIGGER_SCHEDULED ==
                       (int)WATCH_GPS_TRIGGER_SCHEDULED &&
                   (int)GPS_TRIGGER_SCREEN_WAKE ==
                       (int)WATCH_GPS_TRIGGER_SCREEN_WAKE &&
                   (int)GPS_TRIGGER_MANUAL ==
                       (int)WATCH_GPS_TRIGGER_MANUAL &&
                   (int)GPS_TRIGGER_SELFTEST ==
                       (int)WATCH_GPS_TRIGGER_SELFTEST &&
                   (int)GPS_TRIGGER_MOTION ==
                       (int)WATCH_GPS_TRIGGER_MOTION,
               "GPS trigger projection enums must remain aligned");

typedef enum
{
    GPS_COMMAND_STOP = 0,        /**< 请求 GPS owner 停止。 */
    GPS_COMMAND_SELFTEST_BEGIN,  /**< 开始一个 typed UART/FIX probe。 */
    GPS_COMMAND_SELFTEST_CANCEL, /**< 取消仍匹配的 typed UART/FIX probe。 */
    GPS_COMMAND_ACQUIRE,         /**< 请求开始或复用一次搜星会话。 */
    GPS_COMMAND_PAUSE_FOR_RADIO, /**< LTE RF 会话前暂停自动定位。 */
} gps_command_type_t;

typedef struct
{
    gps_command_type_t type;        /**< GPS 私有命令类型。 */
    gps_selftest_request_t request; /**< begin/cancel 的完整请求身份。 */
    gps_acquisition_purpose_t purpose; /**< 搜星请求用途。 */
    gps_acquisition_trigger_t trigger; /**< 搜星请求触发来源。 */
} gps_command_t;

/** GPS owner 私有 typed 命令队列。 */
static QueueHandle_t s_command_queue;
/** GPS owner 私有 typed 自检终态队列。 */
static QueueHandle_t s_result_queue;
/** GPS acquisition 期间禁止自动 light sleep 的最小作用域锁。 */
static esp_pm_lock_handle_t s_gps_pm_lock;
/** GPS acquisition 电源管理锁是否已由 owner 持有。 */
static bool s_gps_pm_lock_held;
/** GPS 是否持有全局 GPS/LTE 射频互斥 owner。 */
static bool s_gps_radio_lock_held;
/** 固定 gps_task 已预留或正在运行。 */
static atomic_bool s_task_active;
/** GPS owner 是否仍接受新的 typed 命令。 */
static atomic_bool s_accepting_commands;
/** stop 可从任意任务请求，由 gps_task 单向消费。 */
static atomic_bool s_stop_requested;
/** selftest cancel 的权威 probe 代次，由 gps_task 单向确认。 */
static atomic_uint s_cancel_probe_generation;
/** selftest cancel 的权威运行 ID，与代次共同构成完整身份。 */
static atomic_uint s_cancel_run_id;
/** selftest cancel 的权威请求 ID，与代次共同构成完整身份。 */
static atomic_uint s_cancel_request_id;
/** selftest cancel 的权威 probe 类型，与代次共同匹配。 */
static atomic_uint s_cancel_probe_kind;
/** GPS 状态更新的启动周期内单调序号。 */
static atomic_uint s_update_sequence;
/** 主动搜星请求是否已经入队但尚未被 owner 消费。 */
static atomic_bool s_active_request_pending;
/** 最近一次发布后对外可见的手动搜星资格。 */
static atomic_bool s_manual_request_available;
/** 当前是否存在 GPS UART/FIX probe。 */
static bool s_probe_active;
/** 当前 probe 的完整请求身份。 */
static gps_selftest_request_t s_probe_request;
/** 当前 probe 启动时捕获的目标 NMEA frame sequence。 */
static uint32_t s_probe_baseline_frame_sequence;
/** 当前 probe 启动时捕获的畸形目标 NMEA sequence。 */
static uint32_t s_probe_baseline_malformed_sequence;
/** FIX probe 最近一次卫星诊断进度日志时间。 */
static uint64_t s_last_fix_progress_log_ms;
/** 无显式自检时常驻搜索状态的最近一次诊断日志时间。 */
static uint64_t s_last_search_diagnostic_ms;
/** owner 接受当前 probe 的本地单调毫秒。 */
static uint64_t s_probe_accepted_at_ms;
/** 是否已经发布过 GPS typed 真值。 */
static bool s_state_published;
/** 最近已发布的 parser 产品状态。 */
static gps_parser_status_t s_last_published_status;
/** 最近已发布真值对应的目标 NMEA sequence。 */
static uint32_t s_last_published_nmea_sequence;
/** 最近已发布的 owner 会话状态。 */
static gps_session_state_t s_last_published_session_state;
/** 最近已发布的会话用途。 */
static gps_acquisition_purpose_t s_last_published_session_purpose;
/** 最近已发布的会话触发原因。 */
static gps_acquisition_trigger_t s_last_published_session_trigger;
/** 最近已发布的自动重试截止。 */
static uint64_t s_last_published_retry_deadline_ms;
/** 最近已发布的统一时间来源。 */
static time_service_source_t s_last_published_time_source;
/** 最近已发布时间样本的单调毫秒。 */
static uint64_t s_last_published_time_sample_ms;
/** 最近已发布的手动搜星资格。 */
static bool s_last_published_manual_request_available;
/** GPS 服务启动周期的单调代次。 */
static uint32_t s_run_generation;
/** 最近一次 FIX 自检超时的本地单调毫秒，后续用于识别晚到 FIX。 */
static uint64_t s_last_fix_timeout_ms;
/** GPS owner 当前会话状态。 */
static gps_session_state_t s_session_state;
/** 当前搜星会话代次。 */
static uint32_t s_session_generation;
/** 当前会话用途。 */
static gps_acquisition_purpose_t s_session_purpose;
/** 当前会话触发来源。 */
static gps_acquisition_trigger_t s_session_trigger;
/** 当前会话截止单调毫秒。 */
static uint64_t s_session_deadline_ms;
/** 当前会话 30 秒 UART 证据截止单调毫秒。 */
static uint64_t s_session_nmea_deadline_ms;
/** 当前会话开始时的目标 NMEA 帧序号。 */
static uint32_t s_session_baseline_frame_sequence;
/** 自动退避截止单调毫秒。 */
static uint64_t s_auto_retry_deadline_ms;
/** 手动和亮屏请求共享的主动冷却截止单调毫秒。 */
static uint64_t s_active_trigger_cooldown_deadline_ms;
/** 当前 20/40/60 分钟长退避窗口是否已经使用亮屏机会。 */
static bool s_screen_opportunity_used;
/** 当前会话期间自动截止是否已经到达并被合并。 */
static bool s_scheduled_retry_merged;
/** 首次自动会话是否已经结束。 */
static bool s_initial_session_completed;
/** 当前退避阶段，0=10 分钟、1=20 分钟、2=40 分钟、3=60 分钟。 */
static uint8_t s_backoff_stage;
/** v2 最近一次有效运动证据的单调毫秒。 */
static uint64_t s_last_motion_ms;
/** v2 活动定位期间下一次检查 BLE、支付与 LTE 前置的时间。 */
static uint64_t s_next_location_prerequisite_check_ms;
/** 最近已提交给 time_service 的 parser 时间样本序号。 */
static uint32_t s_submitted_time_sample_sequence;
#endif

static void parser_reset(gps_parser_owner_t *owner);
static void parser_set_driver(gps_parser_owner_t *owner,
                              bool running,
                              bool available);
static void reset_receiver_diagnostics(gps_parser_owner_t *owner);
static void parser_consume(gps_parser_owner_t *owner,
                           const uint8_t *bytes,
                           size_t length,
                           uint64_t now_ms);
static void parser_consume_byte(gps_parser_owner_t *owner,
                                uint8_t byte,
                                uint64_t now_ms);
static void parser_finish_line(gps_parser_owner_t *owner,
                               uint64_t now_ms);
static void parser_drop_malformed_line(gps_parser_owner_t *owner,
                                       gps_nmea_reject_reason_t reason,
                                       uint64_t now_ms);
static void parser_parse_complete_line(gps_parser_owner_t *owner,
                                       uint64_t now_ms);
static void parser_parse_diagnostic_sentence(gps_parser_owner_t *owner,
                                             char *fields[],
                                             size_t field_count,
                                             const char *sample,
                                             uint64_t now_ms);
static void record_diagnostic_history(gps_parser_owner_t *owner,
                                      const char *sentence,
                                      uint64_t now_ms);
static void record_target_rejection(gps_parser_owner_t *owner,
                                    gps_nmea_reject_reason_t reason,
                                    const char *line,
                                    size_t length,
                                    uint64_t now_ms);
static const char *nmea_reject_reason_name(gps_nmea_reject_reason_t reason);
static size_t split_fields(char *line,
                           char *fields[GPS_SERVICE_NMEA_MAX_FIELDS]);
static bool parse_rmc(char *fields[],
                      size_t field_count,
                      double *latitude,
                      double *longitude,
                      bool *fix_valid,
                      gps_nmea_reject_reason_t *reject_reason);
static bool parse_gga(char *fields[],
                      size_t field_count,
                      double *latitude,
                      double *longitude,
                      bool *fix_valid,
                      gps_nmea_reject_reason_t *reject_reason);
static bool parse_coordinate_pair(const char *latitude_text,
                                  const char *latitude_direction,
                                  const char *longitude_text,
                                  const char *longitude_direction,
                                  double *latitude,
                                  double *longitude);
static bool parse_coordinate(const char *text,
                             const char *direction,
                             bool is_latitude,
                             double *coordinate);
static bool coordinate_text_format_is_valid(const char *text,
                                            bool is_latitude);
static bool utc_field_is_valid(const char *text);
static bool date_field_is_valid(const char *text);
static bool rmc_utc_ms(const char *utc_text,
                       const char *date_text,
                       int64_t *utc_ms);
static unsigned rmc_year(const char *date_text);
static bool leap_year(unsigned year);
static unsigned days_in_month(unsigned year, unsigned month);
static int64_t days_from_civil(int year, unsigned month, unsigned day);
static void update_time_candidate(gps_parser_owner_t *owner,
                                  const char *utc_text,
                                  const char *date_text,
                                  uint64_t now_ms);
static bool decimal_digits_are_valid(const char *text, size_t length);
static bool decimal_field_is_valid(const char *text,
                                   bool allow_sign,
                                   bool allow_empty);
static bool unsigned_integer_field_is_valid(const char *text,
                                            size_t maximum_digits,
                                            bool allow_empty);
static bool paired_unit_field_is_valid(const char *value,
                                       const char *unit,
                                       const char *expected_unit);
static bool optional_checksum_is_valid(char *line,
                                       gps_nmea_reject_reason_t *reject_reason);
static bool hexadecimal_digit_is_valid(char value);
static uint8_t hexadecimal_digit_value(char value);
static bool all_coordinate_fields_empty(const char *latitude_text,
                                        const char *latitude_direction,
                                        const char *longitude_text,
                                        const char *longitude_direction);
static bool target_sentence_has_formatter(const char *type,
                                          const char *formatter);
static bool target_sentence_is_rmc(const char *type);
static bool target_sentence_is_gga(const char *type);
static bool diagnostic_sentence_is_gsv(const char *type);
static bool diagnostic_sentence_is_gsa(const char *type);
static bool diagnostic_sentence_is_txt(const char *type);
static bool diagnostic_sentence_is_zda(const char *type);
static bool line_targets_supported_sentence(const char *line, size_t length);
static bool line_targets_diagnostic_sentence(const char *line, size_t length);
static double normalize_coordinate(double coordinate);
static void clear_fix(gps_parser_owner_t *owner);
static gps_parser_status_t parser_refresh(gps_parser_owner_t *owner,
                                          uint64_t now_ms);
static uint32_t next_nonzero_sequence(uint32_t value);
static bool sequence_is_after(uint32_t candidate, uint32_t baseline);
static gps_probe_decision_t typed_probe_decision(
    const gps_parser_owner_t *owner,
    uint32_t baseline_sequence,
    uint32_t baseline_malformed_sequence,
    gps_selftest_probe_kind_t kind,
    gps_selftest_context_t context,
    bool driver_available,
    bool cancelled,
    uint32_t elapsed_ms,
    uint64_t now_ms);
static bool fix_is_eligible(const gps_parser_owner_t *owner,
                            uint64_t now_ms);
static bool manual_request_available_for_state(
    gps_session_state_t session_state,
    bool continuous_location,
    bool time_valid,
    uint64_t now_ms,
    uint64_t cooldown_deadline_ms);

#ifndef GPS_SERVICE_TEST
static uint64_t monotonic_ms(void);
static uint32_t next_update_sequence(void);
static watch_gps_status_t watch_status_from_parser(gps_parser_status_t status);
static watch_gps_acquisition_state_t watch_acquisition_state(void);
static const char *status_error_code(gps_parser_status_t status,
                                     const char *driver_error_code);
static esp_err_t publish_parser_state(uint64_t now_ms,
                                      const char *driver_error_code,
                                      bool force);
static bool probe_cancel_requested(void);
static void log_fix_diagnostics(const char *stage);
static void log_diagnostic_history(const char *reason);
static void log_gsa_satellite_ids(uint8_t system_id,
                                  const char *system_name);
static void format_gsa_satellite_ids(uint8_t system_id,
                                     char *buffer,
                                     size_t capacity);
static void process_command(const gps_command_t *command,
                            bool *driver_available,
                            uint64_t now_ms,
                            const char **driver_error_code);
static bool complete_probe(gps_selftest_outcome_t outcome,
                           const char *detail_code);
static bool emit_probe_result(const gps_selftest_request_t *request,
                              gps_selftest_outcome_t outcome,
                              const char *detail_code);
static bool request_identity_matches(const gps_selftest_request_t *left,
                                     const gps_selftest_request_t *right);
static esp_err_t enqueue_command(gps_command_type_t type,
                                 const gps_selftest_request_t *request,
                                 TickType_t timeout_ticks);
static esp_err_t enqueue_acquisition(gps_acquisition_purpose_t purpose,
                                     gps_acquisition_trigger_t trigger,
                                     TickType_t timeout_ticks);
static bool session_is_active(void);
static esp_err_t acquire_gps_pm_lock(void);
static void release_gps_pm_lock(void);
static void release_gps_radio_lock(void);
static void finish_gps_radio_shutdown(esp_err_t standby_error,
                                      esp_err_t deinit_error);
static const char *session_state_name(gps_session_state_t state);
static const char *trigger_name(gps_acquisition_trigger_t trigger);
static esp_err_t start_acquisition(gps_acquisition_purpose_t purpose,
                                   gps_acquisition_trigger_t trigger,
                                   uint64_t now_ms);
static void enter_backoff(uint64_t now_ms, bool advance_stage);
static void finish_successful_acquisition(uint64_t now_ms,
                                          bool *driver_available,
                                          const char **driver_error_code);
static void finish_failed_acquisition(uint64_t now_ms,
                                      const char *reason,
                                      bool unavailable,
                                      bool *driver_available,
                                      const char **driver_error_code);
static void finish_selftest_only_session(uint64_t now_ms);
static void process_acquisition_request(gps_acquisition_purpose_t purpose,
                                        gps_acquisition_trigger_t trigger,
                                        uint64_t now_ms);
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
static bool automatic_location_allowed(void);
#endif
static void pause_automatic_acquisition(uint64_t now_ms,
                                        const char *reason,
                                        bool preserve_retry_deadline);
static void advance_session(uint64_t now_ms,
                            bool *driver_available,
                            const char **driver_error_code);
static void submit_ready_time_sample(void);
#endif

static bool manual_request_available_for_state(
    gps_session_state_t session_state,
    bool continuous_location,
    bool time_valid,
    uint64_t now_ms,
    uint64_t cooldown_deadline_ms)
{
    const bool session_accepts_manual_request =
        session_state == GPS_SESSION_STANDBY ||
        session_state == GPS_SESSION_BACKOFF ||
        session_state == GPS_SESSION_UNAVAILABLE;
    if (!session_accepts_manual_request ||
        now_ms < cooldown_deadline_ms)
    {
        return false;
    }
    return continuous_location || !time_valid;
}

#ifdef GPS_SERVICE_TEST
GPS_TEST_API void gps_service_test_reset(void)
{
    parser_reset(&s_parser);
    s_parser.active_session_generation = 1U;
}

GPS_TEST_API int gps_service_test_manual_request_available(
    uint8_t session_state,
    bool continuous_location,
    bool time_valid,
    uint64_t now_ms,
    uint64_t cooldown_deadline_ms)
{
    if (session_state > (uint8_t)GPS_SESSION_UNAVAILABLE)
    {
        return 0;
    }
    return manual_request_available_for_state(
               (gps_session_state_t)session_state,
               continuous_location,
               time_valid,
               now_ms,
               cooldown_deadline_ms)
               ? 1
               : 0;
}

GPS_TEST_API void gps_service_test_set_driver(bool running, bool available)
{
    parser_set_driver(&s_parser, running, available);
}

GPS_TEST_API void gps_service_test_feed(const uint8_t *bytes,
                                        size_t length,
                                        uint64_t now_ms)
{
    parser_consume(&s_parser, bytes, length, now_ms);
}

GPS_TEST_API int gps_service_test_status(uint64_t now_ms)
{
    return (int)parser_refresh(&s_parser, now_ms);
}

GPS_TEST_API int gps_service_test_coordinates(double *latitude,
                                              double *longitude)
{
    if (latitude == NULL || longitude == NULL || !s_parser.coordinates_valid)
    {
        return 0;
    }
    *latitude = s_parser.latitude;
    *longitude = s_parser.longitude;
    return 1;
}

GPS_TEST_API uint32_t gps_service_test_nmea_sequence(void)
{
    return s_parser.nmea_sequence;
}

GPS_TEST_API uint32_t gps_service_test_nmea_frame_sequence(void)
{
    return s_parser.nmea_frame_sequence;
}

GPS_TEST_API uint64_t gps_service_test_last_nmea_ms(void)
{
    return s_parser.last_nmea_rx_ms;
}

GPS_TEST_API uint32_t gps_service_test_malformed_sequence(void)
{
    return s_parser.malformed_target_sequence;
}

GPS_TEST_API char gps_service_test_rmc_status(void)
{
    return s_parser.last_rmc_status;
}

GPS_TEST_API uint8_t gps_service_test_gga_quality(void)
{
    return s_parser.last_gga_quality;
}

GPS_TEST_API uint8_t gps_service_test_gga_satellites_used(void)
{
    return s_parser.last_gga_satellites_used;
}

GPS_TEST_API uint8_t gps_service_test_visible_satellites(char talker)
{
    switch (talker)
    {
    case 'P':
        return s_parser.gps_visible_satellites;
    case 'B':
        return s_parser.beidou_visible_satellites;
    case 'L':
        return s_parser.glonass_visible_satellites;
    case 'N':
        return s_parser.multi_visible_satellites;
    default:
        return 0U;
    }
}

GPS_TEST_API uint8_t gps_service_test_max_cn0(char talker)
{
    switch (talker)
    {
    case 'P':
        return s_parser.gps_max_cn0;
    case 'B':
        return s_parser.beidou_max_cn0;
    case 'L':
        return s_parser.glonass_max_cn0;
    case 'N':
        return s_parser.multi_max_cn0;
    default:
        return 0U;
    }
}

GPS_TEST_API uint32_t gps_service_test_gsv_sentence_count(char talker)
{
    switch (talker)
    {
    case 'P':
        return s_parser.gps_gsv_sentence_count;
    case 'B':
        return s_parser.beidou_gsv_sentence_count;
    case 'L':
        return s_parser.glonass_gsv_sentence_count;
    case 'N':
        return s_parser.multi_gsv_sentence_count;
    default:
        return 0U;
    }
}

GPS_TEST_API uint8_t gps_service_test_tracked_satellites(char talker)
{
    switch (talker)
    {
    case 'P':
        return s_parser.gps_tracked_satellites;
    case 'B':
        return s_parser.beidou_tracked_satellites;
    case 'L':
        return s_parser.glonass_tracked_satellites;
    case 'N':
        return s_parser.multi_tracked_satellites;
    default:
        return 0U;
    }
}

GPS_TEST_API uint16_t gps_service_test_strongest_satellite_id(char talker)
{
    switch (talker)
    {
    case 'P':
        return s_parser.gps_strongest_satellite_id;
    case 'B':
        return s_parser.beidou_strongest_satellite_id;
    case 'L':
        return s_parser.glonass_strongest_satellite_id;
    case 'N':
        return s_parser.multi_strongest_satellite_id;
    default:
        return 0U;
    }
}

GPS_TEST_API uint8_t gps_service_test_gsa_fix_mode(void)
{
    return s_parser.last_gsa_fix_mode;
}

GPS_TEST_API uint8_t gps_service_test_gsa_used_satellites(uint8_t system_id)
{
    return system_id < GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT
               ? s_parser.gsa_used_satellites[system_id]
               : 0U;
}

GPS_TEST_API uint16_t gps_service_test_gsa_satellite_id(uint8_t system_id,
                                                       uint8_t index)
{
    return system_id < GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT &&
                   index < GPS_SERVICE_GSA_SATELLITES_MAX
               ? s_parser.gsa_satellite_ids[system_id][index]
               : 0U;
}

GPS_TEST_API uint32_t gps_service_test_receiver_boot_sequence(void)
{
    return s_parser.receiver_boot_sequence;
}

GPS_TEST_API const char *gps_service_test_last_reject_reason(void)
{
    return nmea_reject_reason_name(s_parser.last_reject_reason);
}

GPS_TEST_API int gps_service_test_probe_completed(uint32_t baseline_sequence)
{
    return sequence_is_after(s_parser.nmea_frame_sequence, baseline_sequence) ? 1 : 0;
}

GPS_TEST_API int gps_service_test_probe_decision(uint32_t baseline_sequence,
                                                 uint32_t baseline_malformed_sequence,
                                                 bool driver_available,
                                                 bool cancelled,
                                                 uint32_t elapsed_ms)
{
    return (int)typed_probe_decision(&s_parser,
                                     baseline_sequence,
                                     baseline_malformed_sequence,
                                     GPS_SELFTEST_PROBE_UART,
                                     GPS_SELFTEST_CONTEXT_DEFAULT,
                                     driver_available,
                                     cancelled,
                                     elapsed_ms,
                                     0U);
}

GPS_TEST_API int gps_service_test_fix_probe_decision(bool driver_available,
                                                     bool cancelled,
                                                     bool indoor_confirmed,
                                                     uint32_t elapsed_ms,
                                                     uint64_t now_ms)
{
    return (int)typed_probe_decision(
        &s_parser,
        s_parser.nmea_sequence,
        s_parser.malformed_target_sequence,
        GPS_SELFTEST_PROBE_FIX,
        indoor_confirmed ? GPS_SELFTEST_CONTEXT_INDOOR_CONFIRMED
                         : GPS_SELFTEST_CONTEXT_DEFAULT,
        driver_available,
        cancelled,
        elapsed_ms,
        now_ms);
}

GPS_TEST_API void gps_service_test_begin_session(uint32_t generation)
{
    s_parser.active_session_generation = generation;
    s_parser.time_candidate_valid = false;
    s_parser.time_sample_ready = false;
}

GPS_TEST_API int gps_service_test_time_sample(int64_t *utc_ms,
                                              uint64_t *rx_ms)
{
    if (!s_parser.time_sample_ready)
    {
        return 0;
    }
    if (utc_ms != NULL)
    {
        *utc_ms = s_parser.time_sample_utc_ms;
    }
    if (rx_ms != NULL)
    {
        *rx_ms = s_parser.time_sample_rx_ms;
    }
    return 1;
}
#endif

#ifndef GPS_SERVICE_TEST
esp_err_t gps_service_init_contracts(void)
{
    if (s_gps_pm_lock == NULL)
    {
        const esp_err_t lock_error = esp_pm_lock_create(
            ESP_PM_NO_LIGHT_SLEEP,
            0,
            "gps_acquire",
            &s_gps_pm_lock);
        if (lock_error != ESP_OK)
        {
            return lock_error;
        }
    }
    if (s_command_queue != NULL && s_result_queue != NULL)
    {
        return ESP_OK;
    }
    if (s_command_queue == NULL)
    {
        s_command_queue = xQueueCreate(GPS_SERVICE_COMMAND_QUEUE_DEPTH,
                                       sizeof(gps_command_t));
    }
    if (s_command_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    if (s_result_queue == NULL)
    {
        s_result_queue = xQueueCreate(GPS_SERVICE_RESULT_QUEUE_DEPTH,
                                      sizeof(gps_selftest_result_t));
    }
    if (s_result_queue == NULL)
    {
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t gps_service_deinit_contracts(void)
{
    if (atomic_load(&s_task_active))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_result_queue != NULL)
    {
        vQueueDelete(s_result_queue);
        s_result_queue = NULL;
    }
    if (s_command_queue != NULL)
    {
        vQueueDelete(s_command_queue);
        s_command_queue = NULL;
    }
    esp_err_t error = l76kb_a58_bsp_deinit();
    release_gps_pm_lock();
    finish_gps_radio_shutdown(ESP_FAIL, error);
    if (s_gps_pm_lock != NULL)
    {
        const esp_err_t lock_error = esp_pm_lock_delete(s_gps_pm_lock);
        if (lock_error == ESP_OK)
        {
            s_gps_pm_lock = NULL;
        }
        else if (error == ESP_OK)
        {
            error = lock_error;
        }
    }
    return error;
}

esp_err_t gps_service_prepare(void)
{
    esp_err_t error = gps_service_init_contracts();
    if (error != ESP_OK)
    {
        return error;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_task_active, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)xQueueReset(s_command_queue);
    (void)xQueueReset(s_result_queue);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_cancel_run_id, 0U);
    atomic_store(&s_cancel_request_id, 0U);
    atomic_store(&s_cancel_probe_kind, GPS_SELFTEST_PROBE_UART);
    atomic_store(&s_cancel_probe_generation, 0U);
    atomic_store(&s_active_request_pending, false);
    atomic_store(&s_manual_request_available, false);
    parser_reset(&s_parser);
    s_probe_active = false;
    s_probe_baseline_frame_sequence = 0U;
    s_probe_baseline_malformed_sequence = 0U;
    s_last_fix_progress_log_ms = 0U;
    memset(&s_probe_request, 0, sizeof(s_probe_request));
    s_state_published = false;
    s_last_published_nmea_sequence = 0U;
    s_last_published_session_state = GPS_SESSION_STANDBY;
    s_last_published_session_purpose = GPS_PURPOSE_TIME_SYNC;
    s_last_published_session_trigger = GPS_TRIGGER_BOOT;
    s_last_published_retry_deadline_ms = 0U;
    s_last_published_time_source = TIME_SOURCE_NONE;
    s_last_published_time_sample_ms = 0U;
    s_last_published_manual_request_available = false;
    s_session_state = GPS_SESSION_STANDBY;
    s_session_generation = 0U;
    s_session_purpose =
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        GPS_PURPOSE_LOCATION;
#else
        GPS_PURPOSE_TIME_SYNC;
#endif
    s_session_trigger = GPS_TRIGGER_BOOT;
    s_session_deadline_ms = 0U;
    s_session_nmea_deadline_ms = 0U;
    s_session_baseline_frame_sequence = 0U;
    s_auto_retry_deadline_ms = 0U;
    s_active_trigger_cooldown_deadline_ms = 0U;
    s_screen_opportunity_used = false;
    s_scheduled_retry_merged = false;
    s_initial_session_completed = false;
    s_backoff_stage = 0U;
    s_last_motion_ms = 0U;
    s_next_location_prerequisite_check_ms = 0U;
    s_submitted_time_sample_sequence = 0U;
    atomic_store(&s_accepting_commands, true);
    return ESP_OK;
}

void gps_service_cancel_prepared_run(void)
{
    atomic_store(&s_accepting_commands, false);
    l76kb_a58_bsp_request_stop();
    const esp_err_t deinit_error = l76kb_a58_bsp_deinit();
    release_gps_pm_lock();
    finish_gps_radio_shutdown(ESP_FAIL, deinit_error);
    if (s_command_queue != NULL)
    {
        (void)xQueueReset(s_command_queue);
    }
    if (s_result_queue != NULL)
    {
        (void)xQueueReset(s_result_queue);
    }
    s_probe_active = false;
    atomic_store(&s_cancel_run_id, 0U);
    atomic_store(&s_cancel_request_id, 0U);
    atomic_store(&s_cancel_probe_kind, GPS_SELFTEST_PROBE_UART);
    atomic_store(&s_cancel_probe_generation, 0U);
    atomic_store(&s_active_request_pending, false);
    atomic_store(&s_manual_request_available, false);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_task_active, false);
}

esp_err_t gps_service_request_stop(TickType_t timeout_ticks)
{
    if (!atomic_load(&s_task_active) || s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_accepting_commands, false);
    atomic_store(&s_stop_requested, true);
    l76kb_a58_bsp_request_stop();
    /* 原子标志是权威停止请求；队列消息只用于提前唤醒 owner。 */
    (void)enqueue_command(GPS_COMMAND_STOP, NULL, timeout_ticks);
    return ESP_OK;
}

esp_err_t gps_service_request_acquisition(gps_acquisition_purpose_t purpose,
                                          gps_acquisition_trigger_t trigger,
                                          TickType_t timeout_ticks)
{
    if (purpose < GPS_PURPOSE_TIME_SYNC ||
        purpose > GPS_PURPOSE_SELFTEST ||
        trigger < GPS_TRIGGER_BOOT ||
        trigger > GPS_TRIGGER_MOTION)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const bool active_trigger =
        trigger == GPS_TRIGGER_MANUAL ||
        trigger == GPS_TRIGGER_SCREEN_WAKE;
    if (active_trigger)
    {
        if (!atomic_load(&s_manual_request_available))
        {
            return ESP_ERR_INVALID_STATE;
        }
        bool expected = false;
        if (!atomic_compare_exchange_strong(&s_active_request_pending,
                                            &expected,
                                            true))
        {
            return ESP_ERR_INVALID_STATE;
        }
    }
    const esp_err_t error =
        enqueue_acquisition(purpose, trigger, timeout_ticks);
    if (error != ESP_OK && active_trigger)
    {
        atomic_store(&s_active_request_pending, false);
    }
    return error;
}

esp_err_t gps_service_notify_motion(TickType_t timeout_ticks)
{
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    return enqueue_acquisition(GPS_PURPOSE_LOCATION,
                               GPS_TRIGGER_MOTION,
                               timeout_ticks);
#else
    (void)timeout_ticks;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t gps_service_pause_for_radio(TickType_t timeout_ticks)
{
    return enqueue_command(GPS_COMMAND_PAUSE_FOR_RADIO,
                           NULL,
                           timeout_ticks);
}

esp_err_t gps_service_request_selftest(const gps_selftest_request_t *request,
                                       TickType_t timeout_ticks)
{
    if (request == NULL || request->run_id == 0U || request->request_id == 0U ||
        request->probe_generation == 0U ||
        request->kind < GPS_SELFTEST_PROBE_UART ||
        request->kind > GPS_SELFTEST_PROBE_FIX ||
        request->context < GPS_SELFTEST_CONTEXT_DEFAULT ||
        request->context > GPS_SELFTEST_CONTEXT_INDOOR_CONFIRMED ||
        (request->kind == GPS_SELFTEST_PROBE_UART &&
         request->context != GPS_SELFTEST_CONTEXT_DEFAULT))
    {
        return ESP_ERR_INVALID_ARG;
    }
    return enqueue_command(GPS_COMMAND_SELFTEST_BEGIN, request, timeout_ticks);
}

esp_err_t gps_service_cancel_selftest(const gps_selftest_request_t *request,
                                      TickType_t timeout_ticks)
{
    if (request == NULL || request->run_id == 0U || request->request_id == 0U ||
        request->probe_generation == 0U ||
        request->kind < GPS_SELFTEST_PROBE_UART ||
        request->kind > GPS_SELFTEST_PROBE_FIX ||
        request->context < GPS_SELFTEST_CONTEXT_DEFAULT ||
        request->context > GPS_SELFTEST_CONTEXT_INDOOR_CONFIRMED ||
        (request->kind == GPS_SELFTEST_PROBE_UART &&
         request->context != GPS_SELFTEST_CONTEXT_DEFAULT))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_task_active) ||
        !atomic_load(&s_accepting_commands) ||
        s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    /* generation 最后发布，owner 只有在完整 typed 身份可见后才会匹配取消。 */
    atomic_store(&s_cancel_run_id, request->run_id);
    atomic_store(&s_cancel_request_id, request->request_id);
    atomic_store(&s_cancel_probe_kind, request->kind);
    atomic_store(&s_cancel_probe_generation, request->probe_generation);
    /* 代次标志是权威取消请求；队列消息只用于提前唤醒 owner。 */
    (void)enqueue_command(GPS_COMMAND_SELFTEST_CANCEL, request, timeout_ticks);
    return ESP_OK;
}

esp_err_t gps_service_receive_selftest_result(gps_selftest_result_t *result,
                                              TickType_t timeout_ticks)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueReceive(s_result_queue, result, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t gps_service_get_coordinates(double *latitude, double *longitude)
{
    if (latitude == NULL || longitude == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *latitude = 0.0;
    *longitude = 0.0;
    if (!s_parser.coordinates_valid)
    {
        return ESP_ERR_NOT_FOUND;
    }
    *latitude = s_parser.latitude;
    *longitude = s_parser.longitude;
    return ESP_OK;
}

esp_err_t gps_service_run(void)
{
    if (!atomic_load(&s_task_active) || s_command_queue == NULL ||
        s_result_queue == NULL)
    {
        atomic_store(&s_accepting_commands, false);
        return ESP_ERR_INVALID_STATE;
    }

    s_run_generation = next_nonzero_sequence(s_run_generation);
    s_last_fix_timeout_ms = 0U;
    s_last_search_diagnostic_ms = monotonic_ms();
    ESP_LOGI(TAG,
             "GPS 服务开始运行，代次=%lu，UART TX 命令计数=0",
             (unsigned long)s_run_generation);

    esp_err_t driver_error = l76kb_a58_bsp_prepare();
    if (driver_error == ESP_OK)
    {
        driver_error = l76kb_a58_bsp_start();
    }
    bool driver_available = driver_error == ESP_OK;
    const char *driver_error_code = l76kb_a58_bsp_last_error_code();
    parser_set_driver(&s_parser, true, driver_available);
    if (driver_available)
    {
        driver_error = l76kb_a58_bsp_set_standby();
        driver_available = driver_error == ESP_OK;
        driver_error_code = l76kb_a58_bsp_last_error_code();
    }
    (void)publish_parser_state(monotonic_ms(), driver_error_code, true);
    if (!driver_available)
    {
        ESP_LOGE(TAG, "GPS BSP 不可用，服务保持可停止状态，原因=%s", driver_error_code);
        s_initial_session_completed = true;
        s_backoff_stage = 0U;
        enter_backoff(monotonic_ms(), false);
        s_session_state = GPS_SESSION_UNAVAILABLE;
    }
    else
    {
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        ESP_LOGI(TAG,
                 "v2 GPS 启动后保持待机，等待新鲜 BLE 与运动证据");
#else
        if (start_acquisition(GPS_PURPOSE_TIME_SYNC,
                              GPS_TRIGGER_BOOT,
                              monotonic_ms()) != ESP_OK)
        {
            driver_available = false;
            driver_error_code = l76kb_a58_bsp_last_error_code();
            parser_set_driver(&s_parser, true, false);
            s_initial_session_completed = true;
            s_backoff_stage = 0U;
            enter_backoff(monotonic_ms(), false);
            s_session_state = GPS_SESSION_UNAVAILABLE;
        }
#endif
    }

    uint8_t read_buffer[GPS_SERVICE_UART_READ_CHUNK_BYTES];
    while (!atomic_load(&s_stop_requested))
    {
        gps_command_t command = {0};
        while (xQueueReceive(s_command_queue, &command, 0) == pdTRUE)
        {
            process_command(&command,
                            &driver_available,
                            monotonic_ms(),
                            &driver_error_code);
        }
        if (atomic_load(&s_stop_requested))
        {
            break;
        }

        uint64_t now_ms = monotonic_ms();
        if (!session_is_active())
        {
            advance_session(now_ms,
                            &driver_available,
                            &driver_error_code);
        }
        if (!session_is_active())
        {
            (void)publish_parser_state(now_ms, driver_error_code, false);
            TickType_t wait_ticks = portMAX_DELAY;
            uint64_t next_wake_ms = s_auto_retry_deadline_ms;
            if (s_active_trigger_cooldown_deadline_ms > now_ms &&
                (next_wake_ms == 0U ||
                 s_active_trigger_cooldown_deadline_ms < next_wake_ms))
            {
                next_wake_ms = s_active_trigger_cooldown_deadline_ms;
            }
            if (next_wake_ms != 0U)
            {
                const uint64_t wait_ms =
                    now_ms >= next_wake_ms
                        ? 0U
                        : next_wake_ms - now_ms;
                wait_ticks = pdMS_TO_TICKS(
                    wait_ms > UINT32_MAX ? UINT32_MAX : (uint32_t)wait_ms);
            }
            if (xQueueReceive(s_command_queue, &command, wait_ticks) == pdTRUE)
            {
                process_command(&command,
                                &driver_available,
                                monotonic_ms(),
                                &driver_error_code);
            }
            continue;
        }
        if (driver_available)
        {
            const int read_count = l76kb_a58_bsp_read(read_buffer,
                                                      sizeof(read_buffer),
                                                      GPS_SERVICE_OWNER_POLL_MS);
            if (read_count < 0)
            {
                driver_available = false;
                driver_error_code = l76kb_a58_bsp_last_error_code();
                parser_set_driver(&s_parser, true, false);
                ESP_LOGE(TAG, "GPS UART 有界读取失败，原因=%s", driver_error_code);
                finish_failed_acquisition(now_ms,
                                          "UART 读取失败",
                                          true,
                                          &driver_available,
                                          &driver_error_code);
            }
            else if (read_count > 0)
            {
                parser_consume(&s_parser,
                               read_buffer,
                               (size_t)read_count,
                               now_ms);
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(GPS_SERVICE_OWNER_POLL_MS));
        }

        const uint64_t refreshed_ms = monotonic_ms();
        submit_ready_time_sample();
        advance_session(refreshed_ms, &driver_available, &driver_error_code);
        if (s_probe_active)
        {
            const uint32_t owner_deadline_ms =
                s_probe_request.kind == GPS_SELFTEST_PROBE_FIX
                    ? GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS
                    : GPS_SERVICE_SELFTEST_OWNER_DEADLINE_MS;
            const uint32_t elapsed_ms =
                refreshed_ms >= s_probe_accepted_at_ms &&
                        refreshed_ms - s_probe_accepted_at_ms <= UINT32_MAX
                    ? (uint32_t)(refreshed_ms - s_probe_accepted_at_ms)
                    : owner_deadline_ms;
            switch (typed_probe_decision(&s_parser,
                                         s_probe_baseline_frame_sequence,
                                         s_probe_baseline_malformed_sequence,
                                         s_probe_request.kind,
                                         s_probe_request.context,
                                         driver_available,
                                         probe_cancel_requested(),
                                         elapsed_ms,
                                         refreshed_ms))
            {
            case GPS_PROBE_UNAVAILABLE:
                complete_probe(
                    GPS_SELFTEST_OUTCOME_FAIL,
                    s_probe_request.kind == GPS_SELFTEST_PROBE_FIX
                        ? "GPS_FIX_UNAVAILABLE"
                        : (driver_error_code != NULL
                               ? driver_error_code
                               : "GPS_UNAVAILABLE"));
                break;
            case GPS_PROBE_PASS:
                if (s_probe_request.kind == GPS_SELFTEST_PROBE_FIX)
                {
                    /** 自检日志使用判定时刻与最近 fix 的单调时间差。 */
                    const uint64_t fix_age_ms =
                        refreshed_ms >= s_parser.last_fix_rx_ms
                            ? refreshed_ms - s_parser.last_fix_rx_ms
                            : 0U;
                    ESP_LOGI(TAG,
                             "GPS 定位自检通过，纬度=%.6f，经度=%.6f，定位年龄=%llu ms，NMEA序号=%lu，目标帧序号=%lu",
                             s_parser.latitude,
                             s_parser.longitude,
                             (unsigned long long)fix_age_ms,
                             (unsigned long)s_parser.nmea_sequence,
                             (unsigned long)s_parser.nmea_frame_sequence);
                }
                complete_probe(
                    GPS_SELFTEST_OUTCOME_PASS,
                    s_probe_request.kind == GPS_SELFTEST_PROBE_FIX
                        ? "GPS_FIX_OK"
                        : "GPS_SELFTEST_NMEA_OK");
                break;
            case GPS_PROBE_TIMEOUT:
                if (s_probe_request.kind == GPS_SELFTEST_PROBE_FIX)
                {
                    s_last_fix_timeout_ms = refreshed_ms;
                    log_fix_diagnostics("超时");
                    log_diagnostic_history("FIX 自检超时");
                }
                complete_probe(
                    s_probe_request.kind == GPS_SELFTEST_PROBE_FIX
                        ? GPS_SELFTEST_OUTCOME_INCONCLUSIVE
                        : GPS_SELFTEST_OUTCOME_FAIL,
                    s_probe_request.kind == GPS_SELFTEST_PROBE_FIX
                        ? "GPS_FIX_WINDOW_EXPIRED"
                        : "GPS_SELFTEST_NO_NMEA_TIMEOUT");
                break;
            case GPS_PROBE_MALFORMED:
                complete_probe(GPS_SELFTEST_OUTCOME_FAIL,
                               "GPS_SELFTEST_MALFORMED_NMEA");
                break;
            case GPS_PROBE_CANCELLED:
                complete_probe(GPS_SELFTEST_OUTCOME_FAIL,
                               "GPS_SELFTEST_CANCELLED");
                break;
            case GPS_PROBE_INDOOR_SKIP:
                complete_probe(GPS_SELFTEST_OUTCOME_SKIP,
                               "GPS_FIX_INDOOR_SKIP");
                break;
            case GPS_PROBE_PENDING:
                if (s_probe_request.kind == GPS_SELFTEST_PROBE_FIX &&
                    (refreshed_ms < s_last_fix_progress_log_ms ||
                     refreshed_ms - s_last_fix_progress_log_ms >=
                         GPS_SERVICE_FIX_PROGRESS_LOG_INTERVAL_MS))
                {
                    log_fix_diagnostics("搜索进度");
                    s_last_fix_progress_log_ms = refreshed_ms;
                }
                break;
            default:
                break;
            }
        }
        else if (driver_available &&
                 !fix_is_eligible(&s_parser, refreshed_ms) &&
                 (refreshed_ms < s_last_search_diagnostic_ms ||
                  refreshed_ms - s_last_search_diagnostic_ms >=
                      GPS_SERVICE_FIX_PROGRESS_LOG_INTERVAL_MS))
        {
            log_fix_diagnostics("常驻搜索");
            s_last_search_diagnostic_ms = refreshed_ms;
        }
        (void)publish_parser_state(refreshed_ms, driver_error_code, false);
    }

    if (s_probe_active)
    {
        complete_probe(GPS_SELFTEST_OUTCOME_FAIL, "GPS_SELFTEST_CANCELLED");
    }
    atomic_store(&s_accepting_commands, false);
    l76kb_a58_bsp_request_stop();
    esp_err_t stop_error = l76kb_a58_bsp_stop();
    esp_err_t deinit_error = l76kb_a58_bsp_deinit();
    release_gps_pm_lock();
    finish_gps_radio_shutdown(stop_error, deinit_error);
    parser_set_driver(&s_parser, false, false);
    clear_fix(&s_parser);
    s_session_state = GPS_SESSION_STANDBY;
    s_session_deadline_ms = 0U;
    s_session_nmea_deadline_ms = 0U;
    s_auto_retry_deadline_ms = 0U;
    s_active_trigger_cooldown_deadline_ms = 0U;
    s_screen_opportunity_used = false;
    s_scheduled_retry_merged = false;
    esp_err_t off_publish_error = ESP_FAIL;
    for (uint32_t attempt = 0U;
         attempt < GPS_SERVICE_FINAL_STATE_PUBLISH_ATTEMPTS;
         ++attempt)
    {
        off_publish_error = publish_parser_state(monotonic_ms(), "GPS_OFF", true);
        if (off_publish_error == ESP_OK)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(GPS_SERVICE_FINAL_STATE_PUBLISH_RETRY_DELAY_MS));
    }
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_cancel_run_id, 0U);
    atomic_store(&s_cancel_request_id, 0U);
    atomic_store(&s_cancel_probe_generation, 0U);
    atomic_store(&s_task_active, false);
    ESP_LOGI(TAG,
             "GPS 服务已停止，代次=%lu，UART RX=%llu 字节，UART TX 命令=%lu",
             (unsigned long)s_run_generation,
             (unsigned long long)s_parser.uart_rx_bytes,
             (unsigned long)s_parser.uart_tx_command_count);
    if (stop_error != ESP_OK)
    {
        return stop_error;
    }
    if (deinit_error != ESP_OK)
    {
        return deinit_error;
    }
    return off_publish_error;
}
#endif

static void parser_reset(gps_parser_owner_t *owner)
{
    if (owner != NULL)
    {
        memset(owner, 0, sizeof(*owner));
    }
}

static void reset_receiver_diagnostics(gps_parser_owner_t *owner)
{
    if (owner == NULL)
    {
        return;
    }
    owner->last_rmc_status = '\0';
    owner->last_gga_quality = 0U;
    owner->last_gga_satellites_used = 0U;
    owner->gps_visible_satellites = 0U;
    owner->beidou_visible_satellites = 0U;
    owner->glonass_visible_satellites = 0U;
    owner->multi_visible_satellites = 0U;
    owner->gps_max_cn0 = 0U;
    owner->beidou_max_cn0 = 0U;
    owner->glonass_max_cn0 = 0U;
    owner->multi_max_cn0 = 0U;
    owner->gps_gsv_sentence_count = 0U;
    owner->beidou_gsv_sentence_count = 0U;
    owner->glonass_gsv_sentence_count = 0U;
    owner->multi_gsv_sentence_count = 0U;
    owner->gps_tracked_satellites = 0U;
    owner->beidou_tracked_satellites = 0U;
    owner->glonass_tracked_satellites = 0U;
    owner->multi_tracked_satellites = 0U;
    owner->gps_strongest_satellite_id = 0U;
    owner->beidou_strongest_satellite_id = 0U;
    owner->glonass_strongest_satellite_id = 0U;
    owner->multi_strongest_satellite_id = 0U;
    owner->last_gsa_fix_mode = 0U;
    owner->last_gsa_system_id = 0U;
    memset(owner->gsa_used_satellites,
           0,
           sizeof(owner->gsa_used_satellites));
    memset(owner->gsa_satellite_ids,
           0,
           sizeof(owner->gsa_satellite_ids));
    owner->last_gsa_pdop = 0.0;
    owner->last_gsa_hdop = 0.0;
    owner->last_gsa_vdop = 0.0;
    owner->antenna_open_count = 0U;
}

static void parser_set_driver(gps_parser_owner_t *owner,
                              bool running,
                              bool available)
{
    if (owner == NULL)
    {
        return;
    }
    owner->service_running = running;
    owner->driver_available = running && available;
    if (!owner->driver_available)
    {
        clear_fix(owner);
    }
}

static void parser_consume(gps_parser_owner_t *owner,
                           const uint8_t *bytes,
                           size_t length,
                           uint64_t now_ms)
{
    if (owner == NULL || bytes == NULL || !owner->service_running ||
        !owner->driver_available)
    {
        return;
    }
    if (length > 0U)
    {
        if (owner->first_uart_rx_ms == 0U)
        {
            owner->first_uart_rx_ms = now_ms;
        }
        if (owner->last_uart_rx_ms != 0U && now_ms >= owner->last_uart_rx_ms)
        {
            const uint64_t gap_ms = now_ms - owner->last_uart_rx_ms;
            if (gap_ms > owner->longest_uart_gap_ms)
            {
                owner->longest_uart_gap_ms = gap_ms;
            }
        }
        owner->last_uart_rx_ms = now_ms;
        owner->uart_rx_bytes += length;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        parser_consume_byte(owner, bytes[index], now_ms);
    }
}

static void parser_consume_byte(gps_parser_owner_t *owner,
                                uint8_t byte,
                                uint64_t now_ms)
{
    if (owner->dropping_overflow)
    {
        if (byte == '\n')
        {
            owner->dropping_overflow = false;
            owner->pending_cr = false;
            owner->line_length = 0U;
        }
        return;
    }

    if (owner->pending_cr)
    {
        owner->pending_cr = false;
        if (byte == '\n')
        {
            parser_finish_line(owner, now_ms);
            return;
        }
        parser_drop_malformed_line(owner,
                                   GPS_NMEA_REJECT_CR_SEQUENCE,
                                   now_ms);
        return;
    }

    if (byte == '\r')
    {
        owner->pending_cr = true;
        return;
    }
    if (byte == '\n')
    {
        parser_finish_line(owner, now_ms);
        return;
    }
    if (byte < 0x20U || byte > 0x7EU)
    {
        parser_drop_malformed_line(owner,
                                   GPS_NMEA_REJECT_NONPRINTABLE,
                                   now_ms);
        return;
    }
    if (owner->line_length + 1U >= sizeof(owner->line))
    {
        parser_drop_malformed_line(owner,
                                   GPS_NMEA_REJECT_LINE_OVERFLOW,
                                   now_ms);
        return;
    }
    owner->line[owner->line_length++] = (char)byte;
}

static void parser_finish_line(gps_parser_owner_t *owner,
                               uint64_t now_ms)
{
    if (owner->line_length > 0U)
    {
        owner->line[owner->line_length] = '\0';
        parser_parse_complete_line(owner, now_ms);
    }
    owner->pending_cr = false;
    owner->line_length = 0U;
}

static void parser_drop_malformed_line(gps_parser_owner_t *owner,
                                       gps_nmea_reject_reason_t reason,
                                       uint64_t now_ms)
{
    if (line_targets_supported_sentence(owner->line, owner->line_length))
    {
        record_target_rejection(owner,
                                reason,
                                owner->line,
                                owner->line_length,
                                now_ms);
    }
    owner->line_length = 0U;
    owner->pending_cr = false;
    owner->dropping_overflow = true;
}

static void parser_parse_complete_line(gps_parser_owner_t *owner,
                                       uint64_t now_ms)
{
    const bool target_line =
        line_targets_supported_sentence(owner->line, owner->line_length);
    const bool diagnostic_line =
        line_targets_diagnostic_sentence(owner->line, owner->line_length);
    if (!target_line && !diagnostic_line)
    {
        return;
    }
    if (owner->line_length <= 6U || owner->line[6] != ',')
    {
        if (target_line)
        {
            record_target_rejection(owner,
                                    GPS_NMEA_REJECT_TARGET_ENVELOPE,
                                    owner->line,
                                    owner->line_length,
                                    now_ms);
        }
        return;
    }

    char sample[GPS_SERVICE_NMEA_LINE_CAPACITY];
    memcpy(sample, owner->line, owner->line_length + 1U);
    gps_nmea_reject_reason_t reject_reason = GPS_NMEA_REJECT_NONE;
    if (!optional_checksum_is_valid(owner->line, &reject_reason))
    {
        if (target_line)
        {
            record_target_rejection(owner,
                                    reject_reason,
                                    sample,
                                    owner->line_length,
                                    now_ms);
        }
        return;
    }
    char *fields[GPS_SERVICE_NMEA_MAX_FIELDS] = {0};
    const size_t field_count = split_fields(owner->line, fields);
    if (field_count == 0U || fields[0] == NULL)
    {
        if (target_line)
        {
            record_target_rejection(owner,
                                    GPS_NMEA_REJECT_FIELD_SPLIT,
                                    sample,
                                    owner->line_length,
                                    now_ms);
        }
        return;
    }
    record_diagnostic_history(owner, sample, now_ms);
    if (diagnostic_line)
    {
        parser_parse_diagnostic_sentence(owner,
                                         fields,
                                         field_count,
                                         sample,
                                         now_ms);
        return;
    }
#ifndef GPS_SERVICE_TEST
    const bool first_target_frame = owner->nmea_frame_sequence == 0U;
#endif
    owner->nmea_frame_sequence = next_nonzero_sequence(owner->nmea_frame_sequence);
#ifndef GPS_SERVICE_TEST
    if (first_target_frame)
    {
        ESP_LOGI(TAG, "GPS 已收到首个完整目标 NMEA 帧，类型=%s", fields[0]);
    }
#endif

    double latitude = 0.0;
    double longitude = 0.0;
    bool fix_valid = false;
    bool sentence_valid = false;
    if (target_sentence_is_rmc(fields[0]))
    {
        sentence_valid = parse_rmc(fields,
                                   field_count,
                                   &latitude,
                                   &longitude,
                                   &fix_valid,
                                   &reject_reason);
    }
    else if (target_sentence_is_gga(fields[0]))
    {
        sentence_valid = parse_gga(fields,
                                   field_count,
                                   &latitude,
                                   &longitude,
                                   &fix_valid,
                                   &reject_reason);
    }
    if (!sentence_valid)
    {
        record_target_rejection(owner,
                                reject_reason,
                                sample,
                                owner->line_length,
                                now_ms);
        return;
    }

    owner->last_nmea_rx_ms = now_ms;
    owner->nmea_sequence = next_nonzero_sequence(owner->nmea_sequence);
    if (!owner->valid_utc_seen && fields[1] != NULL &&
        fields[1][0] != '\0' && utc_field_is_valid(fields[1]))
    {
        owner->valid_utc_seen = true;
#ifndef GPS_SERVICE_TEST
        ESP_LOGI(TAG,
                 "GPS 首次收到合法 UTC，时间=%s，原始句=%s",
                 fields[1],
                 sample);
#endif
    }
    if (target_sentence_is_rmc(fields[0]))
    {
        const char previous_status = owner->last_rmc_status;
        owner->last_rmc_status = fields[2][0];
        if (fix_valid)
        {
            update_time_candidate(owner, fields[1], fields[9], now_ms);
        }
#ifndef GPS_SERVICE_TEST
        if (previous_status != owner->last_rmc_status)
        {
            ESP_LOGI(TAG,
                     "GPS RMC 状态转态：%c→%c，原始句=%s",
                     previous_status != '\0' ? previous_status : '?',
                     owner->last_rmc_status,
                     sample);
        }
#endif
    }
    else if (target_sentence_is_gga(fields[0]))
    {
        const uint8_t previous_quality = owner->last_gga_quality;
        owner->last_gga_quality = (uint8_t)(fields[6][0] - '0');
        owner->last_gga_satellites_used = 0U;
        if (field_count > 7U &&
            unsigned_integer_field_is_valid(fields[7], 2U, true) &&
            fields[7][0] != '\0')
        {
            owner->last_gga_satellites_used =
                (uint8_t)strtoul(fields[7], NULL, 10);
        }
#ifndef GPS_SERVICE_TEST
        if (previous_quality != owner->last_gga_quality)
        {
            ESP_LOGI(TAG,
                     "GPS GGA 质量转态：%u→%u，解算卫星=%u，原始句=%s",
                     (unsigned)previous_quality,
                     (unsigned)owner->last_gga_quality,
                     (unsigned)owner->last_gga_satellites_used,
                     sample);
        }
#endif
    }
    if (fix_valid)
    {
        const bool first_fix = !owner->fix_ever_accepted;
        owner->fix_ever_accepted = true;
        owner->coordinates_valid = true;
        owner->latitude = normalize_coordinate(latitude);
        owner->longitude = normalize_coordinate(longitude);
        owner->last_fix_rx_ms = now_ms;
        owner->last_accepted_fix_rx_ms = now_ms;
#ifndef GPS_SERVICE_TEST
        if (first_fix)
        {
            ESP_LOGI(TAG,
                     "GPS 首次接受有效定位，纬度=%.6f，经度=%.6f，原始句=%s",
                     owner->latitude,
                     owner->longitude,
                     sample);
            if (s_last_fix_timeout_ms != 0U && now_ms >= s_last_fix_timeout_ms)
            {
                ESP_LOGW(TAG,
                         "GPS 在自检超时后获得晚到 FIX，超时后等待=%llu ms",
                         (unsigned long long)(now_ms - s_last_fix_timeout_ms));
            }
            log_diagnostic_history("首次有效定位");
        }
#endif
    }
}

static void record_diagnostic_history(gps_parser_owner_t *owner,
                                      const char *sentence,
                                      uint64_t now_ms)
{
    if (owner == NULL || sentence == NULL)
    {
        return;
    }
    const size_t length = strnlen(sentence, GPS_SERVICE_NMEA_LINE_CAPACITY);
    if (length == 0U || length >= GPS_SERVICE_NMEA_LINE_CAPACITY)
    {
        return;
    }
    gps_nmea_history_entry_t *entry =
        &owner->diagnostic_history[owner->diagnostic_history_next];
    memcpy(entry->sentence, sentence, length + 1U);
    entry->received_at_ms = now_ms;
    owner->diagnostic_history_next =
        (uint8_t)((owner->diagnostic_history_next + 1U) %
                  GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT);
    if (owner->diagnostic_history_count <
        GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT)
    {
        ++owner->diagnostic_history_count;
    }
}

static void parser_parse_diagnostic_sentence(gps_parser_owner_t *owner,
                                             char *fields[],
                                             size_t field_count,
                                             const char *sample,
                                             uint64_t now_ms)
{
    if (owner == NULL || fields == NULL || field_count == 0U ||
        fields[0] == NULL)
    {
        return;
    }
    if (diagnostic_sentence_is_zda(fields[0]))
    {
        if (!owner->valid_utc_seen && field_count > 4U &&
            fields[1] != NULL && utc_field_is_valid(fields[1]) &&
            fields[2] != NULL &&
            unsigned_integer_field_is_valid(fields[2], 2U, false) &&
            fields[3] != NULL &&
            unsigned_integer_field_is_valid(fields[3], 2U, false) &&
            fields[4] != NULL &&
            unsigned_integer_field_is_valid(fields[4], 4U, false))
        {
            owner->valid_utc_seen = true;
#ifndef GPS_SERVICE_TEST
            ESP_LOGI(TAG,
                     "GPS 首次收到合法 ZDA 时间，UTC=%s，日期=%s-%s-%s，原始句=%s",
                     fields[1],
                     fields[4],
                     fields[3],
                     fields[2],
                     sample);
#endif
        }
        return;
    }
    if (diagnostic_sentence_is_gsa(fields[0]))
    {
        const uint8_t previous_fix_mode = owner->last_gsa_fix_mode;
        if (field_count > 2U && fields[2] != NULL &&
            fields[2][0] >= '1' && fields[2][0] <= '3' &&
            fields[2][1] == '\0')
        {
            owner->last_gsa_fix_mode = (uint8_t)(fields[2][0] - '0');
        }

        uint8_t system_id = 0U;
        if (field_count > 18U && fields[18] != NULL &&
            unsigned_integer_field_is_valid(fields[18], 1U, false))
        {
            const unsigned long parsed_system_id = strtoul(fields[18], NULL, 10);
            if (parsed_system_id < GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT)
            {
                system_id = (uint8_t)parsed_system_id;
            }
        }
        else if (fields[0][1] == 'G' && fields[0][2] == 'P')
        {
            system_id = 1U;
        }
        else if (fields[0][1] == 'G' && fields[0][2] == 'L')
        {
            system_id = 2U;
        }
        else if (fields[0][1] == 'B' && fields[0][2] == 'D')
        {
            system_id = 4U;
        }
        owner->last_gsa_system_id = system_id;
        memset(owner->gsa_satellite_ids[system_id],
               0,
               sizeof(owner->gsa_satellite_ids[system_id]));
        owner->gsa_used_satellites[system_id] = 0U;
        for (size_t index = 3U;
             index <= 14U && index < field_count;
             ++index)
        {
            if (fields[index] == NULL ||
                !unsigned_integer_field_is_valid(fields[index], 3U, true) ||
                fields[index][0] == '\0')
            {
                continue;
            }
            const uint8_t satellite_index =
                owner->gsa_used_satellites[system_id];
            if (satellite_index >= GPS_SERVICE_GSA_SATELLITES_MAX)
            {
                break;
            }
            owner->gsa_satellite_ids[system_id][satellite_index] =
                (uint16_t)strtoul(fields[index], NULL, 10);
            owner->gsa_used_satellites[system_id] =
                (uint8_t)(satellite_index + 1U);
        }
        owner->last_gsa_pdop =
            field_count > 15U &&
                    decimal_field_is_valid(fields[15], false, true) &&
                    fields[15][0] != '\0'
                ? strtod(fields[15], NULL)
                : 0.0;
        owner->last_gsa_hdop =
            field_count > 16U &&
                    decimal_field_is_valid(fields[16], false, true) &&
                    fields[16][0] != '\0'
                ? strtod(fields[16], NULL)
                : 0.0;
        owner->last_gsa_vdop =
            field_count > 17U &&
                    decimal_field_is_valid(fields[17], false, true) &&
                    fields[17][0] != '\0'
                ? strtod(fields[17], NULL)
                : 0.0;
#ifndef GPS_SERVICE_TEST
        if (previous_fix_mode != owner->last_gsa_fix_mode)
        {
            ESP_LOGI(TAG,
                     "GPS GSA 模式转态：%u→%u，系统=%u，原始句=%s",
                     (unsigned)previous_fix_mode,
                     (unsigned)owner->last_gsa_fix_mode,
                     (unsigned)owner->last_gsa_system_id,
                     sample);
            if (owner->last_gsa_fix_mode >= 2U)
            {
                log_diagnostic_history("GSA 进入定位模式");
            }
        }
#endif
        return;
    }
    if (diagnostic_sentence_is_txt(fields[0]))
    {
        for (size_t index = 1U; index < field_count; ++index)
        {
            if (fields[index] == NULL)
            {
                continue;
            }
            if (strstr(fields[index], "ANTENNA OPEN") != NULL)
            {
                owner->antenna_open_count =
                    next_nonzero_sequence(owner->antenna_open_count);
                continue;
            }
            if (strstr(fields[index], "MA=CASIC") != NULL)
            {
                owner->receiver_boot_sequence =
                    next_nonzero_sequence(owner->receiver_boot_sequence);
#ifndef GPS_SERVICE_TEST
                ESP_LOGI(TAG,
                         "GPS 接收机启动报文，启动序号=%lu，原始句=%s",
                         (unsigned long)owner->receiver_boot_sequence,
                         sample);
#endif
            }
            else if (strstr(fields[index], "IC=") != NULL ||
                     strstr(fields[index], "SW=") != NULL ||
                     strstr(fields[index], "TB=") != NULL ||
                     strstr(fields[index], "MO=") != NULL)
            {
#ifndef GPS_SERVICE_TEST
                ESP_LOGI(TAG, "GPS 接收机版本报文，原始句=%s", sample);
#endif
            }
        }
        return;
    }
    if (!diagnostic_sentence_is_gsv(fields[0]) || field_count < 4U ||
        !unsigned_integer_field_is_valid(fields[1], 1U, false) ||
        !unsigned_integer_field_is_valid(fields[2], 1U, false) ||
        !unsigned_integer_field_is_valid(fields[3], 2U, false))
    {
        return;
    }

    uint8_t *visible_satellites = NULL;
    uint8_t *max_cn0 = NULL;
    uint32_t *gsv_sentence_count = NULL;
    uint8_t *tracked_satellites = NULL;
    uint16_t *strongest_satellite_id = NULL;
    if (fields[0][1] == 'G' && fields[0][2] == 'P')
    {
        visible_satellites = &owner->gps_visible_satellites;
        max_cn0 = &owner->gps_max_cn0;
        gsv_sentence_count = &owner->gps_gsv_sentence_count;
        tracked_satellites = &owner->gps_tracked_satellites;
        strongest_satellite_id = &owner->gps_strongest_satellite_id;
    }
    else if (fields[0][1] == 'B' && fields[0][2] == 'D')
    {
        visible_satellites = &owner->beidou_visible_satellites;
        max_cn0 = &owner->beidou_max_cn0;
        gsv_sentence_count = &owner->beidou_gsv_sentence_count;
        tracked_satellites = &owner->beidou_tracked_satellites;
        strongest_satellite_id = &owner->beidou_strongest_satellite_id;
    }
    else if (fields[0][1] == 'G' && fields[0][2] == 'L')
    {
        visible_satellites = &owner->glonass_visible_satellites;
        max_cn0 = &owner->glonass_max_cn0;
        gsv_sentence_count = &owner->glonass_gsv_sentence_count;
        tracked_satellites = &owner->glonass_tracked_satellites;
        strongest_satellite_id = &owner->glonass_strongest_satellite_id;
    }
    else if (fields[0][1] == 'G' && fields[0][2] == 'N')
    {
        visible_satellites = &owner->multi_visible_satellites;
        max_cn0 = &owner->multi_max_cn0;
        gsv_sentence_count = &owner->multi_gsv_sentence_count;
        tracked_satellites = &owner->multi_tracked_satellites;
        strongest_satellite_id = &owner->multi_strongest_satellite_id;
    }
    if (visible_satellites == NULL || max_cn0 == NULL ||
        gsv_sentence_count == NULL || tracked_satellites == NULL ||
        strongest_satellite_id == NULL)
    {
        return;
    }

    *gsv_sentence_count =
        next_nonzero_sequence(*gsv_sentence_count);
    *visible_satellites = (uint8_t)strtoul(fields[3], NULL, 10);
    if (strcmp(fields[2], "1") == 0)
    {
        *max_cn0 = 0U;
        *tracked_satellites = 0U;
        *strongest_satellite_id = 0U;
    }
    for (size_t index = 7U; index < field_count; index += 4U)
    {
        if (fields[index] == NULL ||
            !unsigned_integer_field_is_valid(fields[index], 2U, true) ||
            fields[index][0] == '\0')
        {
            continue;
        }
        const uint8_t cn0 = (uint8_t)strtoul(fields[index], NULL, 10);
        if (*tracked_satellites < UINT8_MAX)
        {
            ++(*tracked_satellites);
        }
        if (cn0 > *max_cn0)
        {
            *max_cn0 = cn0;
            if (fields[index - 3U] != NULL &&
                unsigned_integer_field_is_valid(fields[index - 3U],
                                                3U,
                                                false))
            {
                *strongest_satellite_id =
                    (uint16_t)strtoul(fields[index - 3U], NULL, 10);
            }
        }
    }
    if (!owner->nonzero_cn0_seen && *tracked_satellites > 0U)
    {
        owner->nonzero_cn0_seen = true;
#ifndef GPS_SERVICE_TEST
        ESP_LOGI(TAG,
                 "GPS 首次出现非空 C/N0，类型=%s，跟踪=%u，最大C/N0=%u，原始句=%s",
                 fields[0],
                 (unsigned)*tracked_satellites,
                 (unsigned)*max_cn0,
                 sample);
        log_diagnostic_history("首次非空 C/N0");
#endif
    }
    (void)now_ms;
}

static void record_target_rejection(gps_parser_owner_t *owner,
                                    gps_nmea_reject_reason_t reason,
                                    const char *line,
                                    size_t length,
                                    uint64_t now_ms)
{
    if (owner == NULL)
    {
        return;
    }
    owner->malformed_target_sequence =
        next_nonzero_sequence(owner->malformed_target_sequence);
    owner->last_reject_reason = reason;
    if (owner->rejection_count < UINT32_MAX)
    {
        ++owner->rejection_count;
    }
#ifndef GPS_SERVICE_TEST
    const bool burst_log = owner->rejection_count <= GPS_SERVICE_REJECT_LOG_BURST;
    const bool interval_log = now_ms < owner->last_rejection_log_ms ||
                              now_ms - owner->last_rejection_log_ms >=
                                  GPS_SERVICE_REJECT_LOG_INTERVAL_MS;
    if (burst_log || interval_log)
    {
        const size_t bounded_length =
            length < GPS_SERVICE_NMEA_LINE_CAPACITY - 1U
                ? length
                : GPS_SERVICE_NMEA_LINE_CAPACITY - 1U;
        ESP_LOGW(TAG,
                 "GPS NMEA 被拒绝，原因=%s，长度=%u，样本=%.*s",
                 nmea_reject_reason_name(reason),
                 (unsigned)length,
                 (int)bounded_length,
                 line != NULL ? line : "");
        owner->last_rejection_log_ms = now_ms;
    }
#else
    (void)line;
    (void)length;
    (void)now_ms;
#endif
}

static const char *nmea_reject_reason_name(gps_nmea_reject_reason_t reason)
{
    switch (reason)
    {
    case GPS_NMEA_REJECT_CR_SEQUENCE:
        return "GPS_NMEA_REJECT_CR_SEQUENCE";
    case GPS_NMEA_REJECT_NONPRINTABLE:
        return "GPS_NMEA_REJECT_NONPRINTABLE";
    case GPS_NMEA_REJECT_LINE_OVERFLOW:
        return "GPS_NMEA_REJECT_LINE_OVERFLOW";
    case GPS_NMEA_REJECT_TARGET_ENVELOPE:
        return "GPS_NMEA_REJECT_TARGET_ENVELOPE";
    case GPS_NMEA_REJECT_FIELD_SPLIT:
        return "GPS_NMEA_REJECT_FIELD_SPLIT";
    case GPS_NMEA_REJECT_CHECKSUM_SYNTAX:
        return "GPS_NMEA_REJECT_CHECKSUM_SYNTAX";
    case GPS_NMEA_REJECT_CHECKSUM_VALUE:
        return "GPS_NMEA_REJECT_CHECKSUM_VALUE";
    case GPS_NMEA_REJECT_RMC_FIELDS:
        return "GPS_NMEA_REJECT_RMC_FIELDS";
    case GPS_NMEA_REJECT_RMC_STATUS:
        return "GPS_NMEA_REJECT_RMC_STATUS";
    case GPS_NMEA_REJECT_RMC_SEARCHING:
        return "GPS_NMEA_REJECT_RMC_SEARCHING";
    case GPS_NMEA_REJECT_RMC_FIXED:
        return "GPS_NMEA_REJECT_RMC_FIXED";
    case GPS_NMEA_REJECT_GGA_FIELDS:
        return "GPS_NMEA_REJECT_GGA_FIELDS";
    case GPS_NMEA_REJECT_GGA_QUALITY:
        return "GPS_NMEA_REJECT_GGA_QUALITY";
    case GPS_NMEA_REJECT_GGA_SEARCHING:
        return "GPS_NMEA_REJECT_GGA_SEARCHING";
    case GPS_NMEA_REJECT_GGA_FIXED:
        return "GPS_NMEA_REJECT_GGA_FIXED";
    case GPS_NMEA_REJECT_NONE:
    default:
        return "GPS_NMEA_REJECT_NONE";
    }
}

static size_t split_fields(char *line,
                           char *fields[GPS_SERVICE_NMEA_MAX_FIELDS])
{
    if (line == NULL || fields == NULL || line[0] == '\0')
    {
        return 0U;
    }
    size_t count = 1U;
    fields[0] = line;
    for (char *cursor = line; *cursor != '\0'; ++cursor)
    {
        if (*cursor != ',')
        {
            continue;
        }
        if (count >= GPS_SERVICE_NMEA_MAX_FIELDS)
        {
            return 0U;
        }
        *cursor = '\0';
        fields[count++] = cursor + 1;
    }
    return count;
}

static bool parse_rmc(char *fields[],
                      size_t field_count,
                      double *latitude,
                      double *longitude,
                      bool *fix_valid,
                      gps_nmea_reject_reason_t *reject_reason)
{
    if (reject_reason == NULL || field_count < 3U)
    {
        if (reject_reason != NULL)
        {
            *reject_reason = GPS_NMEA_REJECT_RMC_FIELDS;
        }
        return false;
    }
    if (fields[2] == NULL ||
        (strcmp(fields[2], "A") != 0 &&
         strcmp(fields[2], "V") != 0 &&
         strcmp(fields[2], "D") != 0))
    {
        *reject_reason = GPS_NMEA_REJECT_RMC_STATUS;
        return false;
    }
    if (strcmp(fields[2], "V") == 0)
    {
        const bool utc_valid = fields[1][0] == '\0' || utc_field_is_valid(fields[1]);
        bool coordinate_valid = false;
        bool empty_searching_coordinate = true;
        if (field_count >= 7U)
        {
            coordinate_valid = parse_coordinate_pair(fields[3],
                                                     fields[4],
                                                     fields[5],
                                                     fields[6],
                                                     latitude,
                                                     longitude);
            empty_searching_coordinate = all_coordinate_fields_empty(fields[3],
                                                                     fields[4],
                                                                     fields[5],
                                                                     fields[6]);
        }
        const bool date_valid = field_count <= 9U || fields[9][0] == '\0' ||
                                date_field_is_valid(fields[9]);
        if (!utc_valid || !date_valid ||
            (!coordinate_valid && !empty_searching_coordinate))
        {
            *reject_reason = GPS_NMEA_REJECT_RMC_SEARCHING;
            return false;
        }
        *fix_valid = false;
        return true;
    }
    if (field_count < GPS_SERVICE_RMC_MIN_FIELDS)
    {
        *reject_reason = GPS_NMEA_REJECT_RMC_FIELDS;
        return false;
    }
    if (!utc_field_is_valid(fields[1]) || !date_field_is_valid(fields[9]) ||
        !decimal_field_is_valid(fields[7], false, true) ||
        !decimal_field_is_valid(fields[8], false, true) ||
        !decimal_field_is_valid(fields[10], false, true) ||
        !paired_unit_field_is_valid(fields[10], fields[11], "E|W"))
    {
        *reject_reason = GPS_NMEA_REJECT_RMC_FIXED;
        return false;
    }
    if (fields[8][0] != '\0' && strtod(fields[8], NULL) > 360.0)
    {
        *reject_reason = GPS_NMEA_REJECT_RMC_FIXED;
        return false;
    }
    if (field_count > GPS_SERVICE_RMC_MIN_FIELDS &&
        fields[12][0] != '\0' &&
        (fields[12][1] != '\0' || strchr("ADEMNS", fields[12][0]) == NULL))
    {
        *reject_reason = GPS_NMEA_REJECT_RMC_FIXED;
        return false;
    }
    const bool coordinate_valid = parse_coordinate_pair(fields[3],
                                                        fields[4],
                                                        fields[5],
                                                        fields[6],
                                                        latitude,
                                                        longitude);
    const bool empty_searching_coordinate = all_coordinate_fields_empty(fields[3],
                                                                        fields[4],
                                                                        fields[5],
                                                                        fields[6]);
    if (!coordinate_valid)
    {
        (void)empty_searching_coordinate;
        *reject_reason = GPS_NMEA_REJECT_RMC_FIXED;
        return false;
    }
    *fix_valid = true;
    return true;
}

static bool parse_gga(char *fields[],
                      size_t field_count,
                      double *latitude,
                      double *longitude,
                      bool *fix_valid,
                      gps_nmea_reject_reason_t *reject_reason)
{
    if (reject_reason == NULL || field_count < 7U)
    {
        if (reject_reason != NULL)
        {
            *reject_reason = GPS_NMEA_REJECT_GGA_FIELDS;
        }
        return false;
    }
    if (fields[6] == NULL || fields[6][0] < '0' || fields[6][0] > '9' ||
        fields[6][1] != '\0')
    {
        *reject_reason = GPS_NMEA_REJECT_GGA_QUALITY;
        return false;
    }
    const long quality = (long)(fields[6][0] - '0');
    const bool coordinate_valid = parse_coordinate_pair(fields[2],
                                                        fields[3],
                                                        fields[4],
                                                        fields[5],
                                                        latitude,
                                                        longitude);
    const bool empty_searching_coordinate = all_coordinate_fields_empty(fields[2],
                                                                        fields[3],
                                                                        fields[4],
                                                                        fields[5]);
    if (quality == 0L)
    {
        const bool utc_valid = fields[1][0] == '\0' || utc_field_is_valid(fields[1]);
        if (!utc_valid || (!coordinate_valid && !empty_searching_coordinate))
        {
            *reject_reason = GPS_NMEA_REJECT_GGA_SEARCHING;
            return false;
        }
        *fix_valid = false;
        return true;
    }
    if (field_count < GPS_SERVICE_GGA_MIN_FIELDS)
    {
        *reject_reason = GPS_NMEA_REJECT_GGA_FIELDS;
        return false;
    }
    if (!utc_field_is_valid(fields[1]) ||
        !unsigned_integer_field_is_valid(fields[7], 2U, false) ||
        !decimal_field_is_valid(fields[8], false, false) ||
        !decimal_field_is_valid(fields[9], true, true) ||
        !paired_unit_field_is_valid(fields[9], fields[10], "M") ||
        !decimal_field_is_valid(fields[11], true, true) ||
        !paired_unit_field_is_valid(fields[11], fields[12], "M") ||
        !decimal_field_is_valid(fields[13], false, true) ||
        !unsigned_integer_field_is_valid(fields[14], 4U, true))
    {
        *reject_reason = GPS_NMEA_REJECT_GGA_FIXED;
        return false;
    }
    if (!coordinate_valid)
    {
        *reject_reason = GPS_NMEA_REJECT_GGA_FIXED;
        return false;
    }
    *fix_valid = true;
    return true;
}

static bool parse_coordinate_pair(const char *latitude_text,
                                  const char *latitude_direction,
                                  const char *longitude_text,
                                  const char *longitude_direction,
                                  double *latitude,
                                  double *longitude)
{
    return parse_coordinate(latitude_text,
                            latitude_direction,
                            true,
                            latitude) &&
           parse_coordinate(longitude_text,
                            longitude_direction,
                            false,
                            longitude);
}

static bool parse_coordinate(const char *text,
                             const char *direction,
                             bool is_latitude,
                             double *coordinate)
{
    if (text == NULL || direction == NULL || coordinate == NULL ||
        !coordinate_text_format_is_valid(text, is_latitude) ||
        direction[0] == '\0' || direction[1] != '\0')
    {
        return false;
    }
    const bool direction_valid = is_latitude
                                     ? (direction[0] == 'N' || direction[0] == 'S')
                                     : (direction[0] == 'E' || direction[0] == 'W');
    if (!direction_valid)
    {
        return false;
    }

    char *end = NULL;
    const double raw = strtod(text, &end);
    if (end == text || *end != '\0' || !isfinite(raw) || raw < 0.0)
    {
        return false;
    }
    const double degrees = floor(raw / 100.0);
    const double minutes = raw - degrees * 100.0;
    const double maximum = is_latitude ? 90.0 : 180.0;
    if (minutes < 0.0 || minutes >= 60.0 || degrees > maximum ||
        (degrees == maximum && minutes != 0.0))
    {
        return false;
    }
    double converted = degrees + minutes / 60.0;
    if (direction[0] == 'S' || direction[0] == 'W')
    {
        converted = -converted;
    }
    if (!isfinite(converted) || converted < -maximum || converted > maximum)
    {
        return false;
    }
    *coordinate = converted;
    return true;
}

static bool coordinate_text_format_is_valid(const char *text,
                                            bool is_latitude)
{
    if (text == NULL || text[0] == '\0')
    {
        return false;
    }
    const size_t required_integer_digits = is_latitude ? 4U : 5U;
    size_t integer_digits = 0U;
    bool saw_decimal = false;
    size_t fractional_digits = 0U;
    for (const char *cursor = text; *cursor != '\0'; ++cursor)
    {
        if (*cursor == '.')
        {
            if (saw_decimal)
            {
                return false;
            }
            saw_decimal = true;
            continue;
        }
        if (*cursor < '0' || *cursor > '9')
        {
            return false;
        }
        if (saw_decimal)
        {
            ++fractional_digits;
        }
        else
        {
            ++integer_digits;
        }
    }
    return integer_digits == required_integer_digits &&
           (!saw_decimal || fractional_digits > 0U);
}

static bool utc_field_is_valid(const char *text)
{
    if (text == NULL || !decimal_digits_are_valid(text, 6U))
    {
        return false;
    }
    const unsigned hour = (unsigned)(text[0] - '0') * 10U +
                          (unsigned)(text[1] - '0');
    const unsigned minute = (unsigned)(text[2] - '0') * 10U +
                            (unsigned)(text[3] - '0');
    const unsigned second = (unsigned)(text[4] - '0') * 10U +
                            (unsigned)(text[5] - '0');
    if (hour > 23U || minute > 59U || second > 59U)
    {
        return false;
    }
    if (text[6] == '\0')
    {
        return true;
    }
    return text[6] == '.' && text[7] != '\0' &&
           decimal_digits_are_valid(text + 7, strlen(text + 7));
}

static bool date_field_is_valid(const char *text)
{
    if (text == NULL || strlen(text) != 6U ||
        !decimal_digits_are_valid(text, 6U))
    {
        return false;
    }
    const unsigned day = (unsigned)(text[0] - '0') * 10U +
                         (unsigned)(text[1] - '0');
    const unsigned month = (unsigned)(text[2] - '0') * 10U +
                           (unsigned)(text[3] - '0');
    const unsigned year = rmc_year(text);
    return month >= 1U && month <= 12U &&
           day >= 1U && day <= days_in_month(year, month);
}

static bool rmc_utc_ms(const char *utc_text,
                       const char *date_text,
                       int64_t *utc_ms)
{
    if (utc_ms == NULL || !utc_field_is_valid(utc_text) ||
        !date_field_is_valid(date_text))
    {
        return false;
    }
    const unsigned year = rmc_year(date_text);
    if (year < GPS_SERVICE_TIME_MIN_YEAR)
    {
        return false;
    }
    const unsigned month = (unsigned)(date_text[2] - '0') * 10U +
                           (unsigned)(date_text[3] - '0');
    const unsigned day = (unsigned)(date_text[0] - '0') * 10U +
                         (unsigned)(date_text[1] - '0');
    const unsigned hour = (unsigned)(utc_text[0] - '0') * 10U +
                          (unsigned)(utc_text[1] - '0');
    const unsigned minute = (unsigned)(utc_text[2] - '0') * 10U +
                            (unsigned)(utc_text[3] - '0');
    const unsigned second = (unsigned)(utc_text[4] - '0') * 10U +
                            (unsigned)(utc_text[5] - '0');
    unsigned milliseconds = 0U;
    if (utc_text[6] == '.')
    {
        const size_t fractional_length = strlen(utc_text + 7U);
        for (size_t index = 0U; index < 3U; ++index)
        {
            milliseconds *= 10U;
            if (index < fractional_length)
            {
                milliseconds +=
                    (unsigned)(utc_text[7U + index] - '0');
            }
        }
    }
    const int64_t days = days_from_civil((int)year, month, day);
    if (days < 0)
    {
        return false;
    }
    *utc_ms =
        (((days * 24LL + (int64_t)hour) * 60LL + (int64_t)minute) *
             60LL +
         (int64_t)second) *
            1000LL +
        (int64_t)milliseconds;
    return *utc_ms >= 0 && *utc_ms <= GPS_SERVICE_UTC_MAX_MS;
}

static unsigned rmc_year(const char *date_text)
{
    if (date_text == NULL || strlen(date_text) != 6U)
    {
        return 0U;
    }
    const unsigned year_two_digits =
        (unsigned)(date_text[4] - '0') * 10U +
        (unsigned)(date_text[5] - '0');
    return year_two_digits >= GPS_SERVICE_TIME_YEAR_PIVOT
               ? 1900U + year_two_digits
               : 2000U + year_two_digits;
}

static bool leap_year(unsigned year)
{
    return (year % 4U == 0U && year % 100U != 0U) ||
           year % 400U == 0U;
}

static unsigned days_in_month(unsigned year, unsigned month)
{
    static const uint8_t days[12] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U,
    };
    if (month < 1U || month > 12U)
    {
        return 0U;
    }
    return month == 2U && leap_year(year)
               ? 29U
               : days[month - 1U];
}

static int64_t days_from_civil(int year, unsigned month, unsigned day)
{
    year -= month <= 2U;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned year_of_era = (unsigned)(year - era * 400);
    const unsigned shifted_month =
        month > 2U ? month - 3U : month + 9U;
    const unsigned day_of_year =
        (153U * shifted_month + 2U) / 5U +
        day - 1U;
    const unsigned day_of_era =
        year_of_era * 365U + year_of_era / 4U -
        year_of_era / 100U + day_of_year;
    return (int64_t)era * 146097LL + (int64_t)day_of_era - 719468LL;
}

static void update_time_candidate(gps_parser_owner_t *owner,
                                  const char *utc_text,
                                  const char *date_text,
                                  uint64_t now_ms)
{
    if (owner == NULL || owner->active_session_generation == 0U)
    {
        return;
    }
    int64_t utc_ms = 0;
    if (!rmc_utc_ms(utc_text, date_text, &utc_ms))
    {
        return;
    }
    if (owner->time_candidate_valid &&
        owner->time_candidate_session_generation ==
            owner->active_session_generation &&
        utc_ms - owner->time_candidate_utc_ms == 1000LL &&
        now_ms >= owner->time_candidate_rx_ms)
    {
        const uint64_t interval_ms = now_ms - owner->time_candidate_rx_ms;
        if (interval_ms >= GPS_SERVICE_TIME_SAMPLE_MIN_INTERVAL_MS &&
            interval_ms <= GPS_SERVICE_TIME_SAMPLE_MAX_INTERVAL_MS)
        {
            owner->time_sample_ready = true;
            owner->time_sample_utc_ms = utc_ms;
            owner->time_sample_rx_ms = now_ms;
            owner->time_sample_sequence =
                next_nonzero_sequence(owner->time_sample_sequence);
        }
    }
    owner->time_candidate_valid = true;
    owner->time_candidate_utc_ms = utc_ms;
    owner->time_candidate_rx_ms = now_ms;
    owner->time_candidate_session_generation =
        owner->active_session_generation;
}

static bool decimal_digits_are_valid(const char *text, size_t length)
{
    if (text == NULL || length == 0U)
    {
        return false;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        if (text[index] < '0' || text[index] > '9')
        {
            return false;
        }
    }
    return true;
}

static bool decimal_field_is_valid(const char *text,
                                   bool allow_sign,
                                   bool allow_empty)
{
    if (text == NULL)
    {
        return false;
    }
    if (text[0] == '\0')
    {
        return allow_empty;
    }
    size_t index = 0U;
    if (allow_sign && (text[index] == '+' || text[index] == '-'))
    {
        ++index;
    }
    bool saw_digit = false;
    bool saw_decimal = false;
    for (; text[index] != '\0'; ++index)
    {
        if (text[index] == '.')
        {
            if (saw_decimal)
            {
                return false;
            }
            saw_decimal = true;
            continue;
        }
        if (text[index] < '0' || text[index] > '9')
        {
            return false;
        }
        saw_digit = true;
    }
    return saw_digit;
}

static bool unsigned_integer_field_is_valid(const char *text,
                                            size_t maximum_digits,
                                            bool allow_empty)
{
    if (text == NULL)
    {
        return false;
    }
    const size_t length = strlen(text);
    if (length == 0U)
    {
        return allow_empty;
    }
    return length <= maximum_digits && decimal_digits_are_valid(text, length);
}

static bool paired_unit_field_is_valid(const char *value,
                                       const char *unit,
                                       const char *expected_unit)
{
    if (value == NULL || unit == NULL || expected_unit == NULL)
    {
        return false;
    }
    if (value[0] == '\0')
    {
        return unit[0] == '\0';
    }
    if (strcmp(expected_unit, "E|W") == 0)
    {
        return unit[1] == '\0' && (unit[0] == 'E' || unit[0] == 'W');
    }
    return strcmp(unit, expected_unit) == 0;
}

static bool optional_checksum_is_valid(char *line,
                                       gps_nmea_reject_reason_t *reject_reason)
{
    if (line == NULL || reject_reason == NULL || line[0] != '$')
    {
        return false;
    }
    char *separator = strchr(line, '*');
    if (separator == NULL)
    {
        return true;
    }
    if (strlen(separator) != 3U ||
        !hexadecimal_digit_is_valid(separator[1]) ||
        !hexadecimal_digit_is_valid(separator[2]))
    {
        *reject_reason = GPS_NMEA_REJECT_CHECKSUM_SYNTAX;
        return false;
    }
    uint8_t calculated = 0U;
    for (const char *cursor = line + 1; cursor < separator; ++cursor)
    {
        calculated ^= (uint8_t)*cursor;
    }
    const uint8_t expected =
        (uint8_t)((hexadecimal_digit_value(separator[1]) << 4U) |
                  hexadecimal_digit_value(separator[2]));
    if (calculated != expected)
    {
        *reject_reason = GPS_NMEA_REJECT_CHECKSUM_VALUE;
        return false;
    }
    *separator = '\0';
    return true;
}

static bool hexadecimal_digit_is_valid(char value)
{
    return (value >= '0' && value <= '9') ||
           (value >= 'A' && value <= 'F') ||
           (value >= 'a' && value <= 'f');
}

static bool all_coordinate_fields_empty(const char *latitude_text,
                                        const char *latitude_direction,
                                        const char *longitude_text,
                                        const char *longitude_direction)
{
    return latitude_text != NULL && latitude_text[0] == '\0' &&
           latitude_direction != NULL && latitude_direction[0] == '\0' &&
           longitude_text != NULL && longitude_text[0] == '\0' &&
           longitude_direction != NULL && longitude_direction[0] == '\0';
}

static bool target_sentence_has_formatter(const char *type,
                                          const char *formatter)
{
    if (type == NULL || formatter == NULL || strlen(type) != 6U ||
        type[0] != '$')
    {
        return false;
    }
    const bool talker_supported =
        (type[1] == 'G' &&
         (type[2] == 'P' || type[2] == 'N' || type[2] == 'L')) ||
        (type[1] == 'B' && type[2] == 'D');
    return talker_supported && memcmp(type + 3, formatter, 3U) == 0;
}

static bool target_sentence_is_rmc(const char *type)
{
    return target_sentence_has_formatter(type, "RMC");
}

static bool target_sentence_is_gga(const char *type)
{
    return target_sentence_has_formatter(type, "GGA");
}

static bool diagnostic_sentence_is_gsv(const char *type)
{
    return target_sentence_has_formatter(type, "GSV");
}

static bool diagnostic_sentence_is_gsa(const char *type)
{
    return target_sentence_has_formatter(type, "GSA");
}

static bool diagnostic_sentence_is_txt(const char *type)
{
    return target_sentence_has_formatter(type, "TXT");
}

static bool diagnostic_sentence_is_zda(const char *type)
{
    return target_sentence_has_formatter(type, "ZDA");
}

static bool line_targets_supported_sentence(const char *line, size_t length)
{
    if (line == NULL || length < 6U)
    {
        return false;
    }
    char type[7] = {0};
    memcpy(type, line, 6U);
    return target_sentence_is_rmc(type) || target_sentence_is_gga(type);
}

static bool line_targets_diagnostic_sentence(const char *line, size_t length)
{
    if (line == NULL || length < 6U)
    {
        return false;
    }
    char type[7] = {0};
    memcpy(type, line, 6U);
    return diagnostic_sentence_is_gsv(type) ||
           diagnostic_sentence_is_gsa(type) ||
           diagnostic_sentence_is_txt(type) ||
           diagnostic_sentence_is_zda(type);
}

static double normalize_coordinate(double coordinate)
{
    const double normalized = round(coordinate * 1000000.0) / 1000000.0;
    return normalized == 0.0 ? 0.0 : normalized;
}

static void clear_fix(gps_parser_owner_t *owner)
{
    owner->coordinates_valid = false;
    owner->latitude = 0.0;
    owner->longitude = 0.0;
    owner->last_fix_rx_ms = 0U;
}

static gps_parser_status_t parser_refresh(gps_parser_owner_t *owner,
                                          uint64_t now_ms)
{
    if (!owner->service_running)
    {
        clear_fix(owner);
        return GPS_PARSER_STATUS_OFF;
    }
    if (!owner->driver_available)
    {
        clear_fix(owner);
        return GPS_PARSER_STATUS_UNAVAILABLE;
    }
    if (!owner->coordinates_valid || now_ms < owner->last_fix_rx_ms ||
        now_ms - owner->last_fix_rx_ms >= GPS_SERVICE_FIX_FRESHNESS_MS)
    {
        clear_fix(owner);
        return GPS_PARSER_STATUS_SEARCHING;
    }
    return GPS_PARSER_STATUS_FIXED;
}

static uint32_t next_nonzero_sequence(uint32_t value)
{
    return value == UINT32_MAX ? 1U : value + 1U;
}

static bool sequence_is_after(uint32_t candidate, uint32_t baseline)
{
    return candidate != 0U &&
           (baseline == 0U || (int32_t)(candidate - baseline) > 0);
}

static gps_probe_decision_t typed_probe_decision(
    const gps_parser_owner_t *owner,
    uint32_t baseline_sequence,
    uint32_t baseline_malformed_sequence,
    gps_selftest_probe_kind_t kind,
    gps_selftest_context_t context,
    bool driver_available,
    bool cancelled,
    uint32_t elapsed_ms,
    uint64_t now_ms)
{
    if (!driver_available)
    {
        return GPS_PROBE_UNAVAILABLE;
    }
    if (cancelled)
    {
        return GPS_PROBE_CANCELLED;
    }
    if (kind == GPS_SELFTEST_PROBE_FIX)
    {
        if (elapsed_ms >= GPS_SERVICE_FIX_SELFTEST_OWNER_DEADLINE_MS)
        {
            return GPS_PROBE_TIMEOUT;
        }
        if (fix_is_eligible(owner, now_ms))
        {
            return GPS_PROBE_PASS;
        }
        if (context == GPS_SELFTEST_CONTEXT_INDOOR_CONFIRMED)
        {
            return GPS_PROBE_INDOOR_SKIP;
        }
        return GPS_PROBE_PENDING;
    }
    if (owner != NULL &&
        sequence_is_after(owner->nmea_frame_sequence, baseline_sequence))
    {
        return GPS_PROBE_PASS;
    }
    if (elapsed_ms >= GPS_SERVICE_SELFTEST_OWNER_DEADLINE_MS)
    {
        return owner != NULL &&
                       sequence_is_after(owner->malformed_target_sequence,
                                         baseline_malformed_sequence)
                   ? GPS_PROBE_MALFORMED
                   : GPS_PROBE_TIMEOUT;
    }
    return GPS_PROBE_PENDING;
}

static uint8_t hexadecimal_digit_value(char value)
{
    if (value >= '0' && value <= '9')
    {
        return (uint8_t)(value - '0');
    }
    if (value >= 'A' && value <= 'F')
    {
        return (uint8_t)(value - 'A' + 10);
    }
    return (uint8_t)(value - 'a' + 10);
}

static bool fix_is_eligible(const gps_parser_owner_t *owner,
                            uint64_t now_ms)
{
    if (owner == NULL || !owner->service_running || !owner->driver_available ||
        !owner->coordinates_valid || !isfinite(owner->latitude) ||
        !isfinite(owner->longitude) || owner->latitude < -90.0 ||
        owner->latitude > 90.0 || owner->longitude < -180.0 ||
        owner->longitude > 180.0 ||
        (owner->latitude == 0.0 && owner->longitude == 0.0) ||
        now_ms < owner->last_fix_rx_ms)
    {
        return false;
    }
    return now_ms - owner->last_fix_rx_ms < GPS_SERVICE_FIX_FRESHNESS_MS;
}

#ifndef GPS_SERVICE_TEST
static uint64_t monotonic_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000U;
}

static uint32_t next_update_sequence(void)
{
    uint32_t sequence = atomic_fetch_add(&s_update_sequence, 1U) + 1U;
    if (sequence == 0U)
    {
        sequence = atomic_fetch_add(&s_update_sequence, 1U) + 1U;
    }
    return sequence;
}

static watch_gps_status_t watch_status_from_parser(gps_parser_status_t status)
{
    switch (status)
    {
    case GPS_PARSER_STATUS_SEARCHING:
        return WATCH_GPS_STATUS_SEARCHING;
    case GPS_PARSER_STATUS_FIXED:
        return WATCH_GPS_STATUS_FIXED;
    case GPS_PARSER_STATUS_UNAVAILABLE:
        return WATCH_GPS_STATUS_UNAVAILABLE;
    case GPS_PARSER_STATUS_OFF:
    default:
        return WATCH_GPS_STATUS_OFF;
    }
}

static watch_gps_acquisition_state_t watch_acquisition_state(void)
{
    switch (s_session_state)
    {
    case GPS_SESSION_ACQUIRING:
        return WATCH_GPS_ACQUISITION_SEARCHING;
    case GPS_SESSION_TRACKING:
        return WATCH_GPS_ACQUISITION_TRACKING;
    case GPS_SESSION_BACKOFF:
        return WATCH_GPS_ACQUISITION_BACKOFF;
    case GPS_SESSION_UNAVAILABLE:
        return WATCH_GPS_ACQUISITION_UNAVAILABLE;
    case GPS_SESSION_STANDBY:
    default:
        return WATCH_GPS_ACQUISITION_STANDBY;
    }
}

static const char *status_error_code(gps_parser_status_t status,
                                     const char *driver_error_code)
{
    switch (status)
    {
    case GPS_PARSER_STATUS_FIXED:
        return "GPS_OK";
    case GPS_PARSER_STATUS_SEARCHING:
        return "GPS_SEARCHING";
    case GPS_PARSER_STATUS_UNAVAILABLE:
        return driver_error_code != NULL ? driver_error_code : "GPS_UNAVAILABLE";
    case GPS_PARSER_STATUS_OFF:
    default:
        return "GPS_OFF";
    }
}

static esp_err_t publish_parser_state(uint64_t now_ms,
                                      const char *driver_error_code,
                                      bool force)
{
    const gps_parser_status_t parser_status = parser_refresh(&s_parser, now_ms);
    time_service_snapshot_t time_snapshot = {0};
    (void)time_service_get_snapshot(&time_snapshot);
    const bool manual_request_available =
        manual_request_available_for_state(
            s_session_state,
            LEGBOT_CAP_GPS_CONTINUOUS_LOCATION != 0,
            time_snapshot.valid,
            now_ms,
            s_active_trigger_cooldown_deadline_ms);
    atomic_store(&s_manual_request_available,
                 manual_request_available);
    if (!force && s_state_published &&
        parser_status == s_last_published_status &&
        s_parser.nmea_sequence == s_last_published_nmea_sequence &&
        s_session_state == s_last_published_session_state &&
        s_session_purpose == s_last_published_session_purpose &&
        s_session_trigger == s_last_published_session_trigger &&
        s_auto_retry_deadline_ms == s_last_published_retry_deadline_ms &&
        time_snapshot.source == s_last_published_time_source &&
        time_snapshot.sample_monotonic_ms ==
            s_last_published_time_sample_ms &&
        manual_request_available ==
            s_last_published_manual_request_available)
    {
        return ESP_OK;
    }

    watch_gps_update_t update = {
        .status = watch_status_from_parser(parser_status),
        .acquisition_state = watch_acquisition_state(),
        .purpose = (watch_gps_purpose_t)s_session_purpose,
        .trigger = (watch_gps_trigger_t)s_session_trigger,
        .coordinates_valid = parser_status == GPS_PARSER_STATUS_FIXED,
        .manual_request_available = manual_request_available,
        .screen_opportunity_used = s_screen_opportunity_used,
        .next_retry_ms = s_auto_retry_deadline_ms,
        .manual_cooldown_deadline_ms =
            s_active_trigger_cooldown_deadline_ms,
        .updated_at_ms = now_ms,
        .update_sequence = next_update_sequence(),
        .session_generation = s_session_generation,
        .backoff_stage = s_backoff_stage,
    };
    if (time_snapshot.valid)
    {
        update.time_synchronized = true;
        update.time_source =
            (watch_time_source_t)time_snapshot.source;
        update.time_offset_ms = time_snapshot.utc_offset_ms;
#if !LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        update.manual_request_available = false;
#endif
    }
    if (update.coordinates_valid)
    {
        update.latitude = s_parser.latitude;
        update.longitude = s_parser.longitude;
        update.last_fix_rx_ms = s_parser.last_fix_rx_ms;
    }
    const char *error_code = status_error_code(parser_status, driver_error_code);
    const size_t error_length = strnlen(error_code, sizeof(update.error_code));
    if (error_length == 0U || error_length >= sizeof(update.error_code))
    {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(update.error_code, error_code, error_length + 1U);
    const esp_err_t error = state_service_publish_gps(&update, 0);
    if (error == ESP_OK)
    {
        s_state_published = true;
        s_last_published_status = parser_status;
        s_last_published_nmea_sequence = s_parser.nmea_sequence;
        s_last_published_session_state = s_session_state;
        s_last_published_session_purpose = s_session_purpose;
        s_last_published_session_trigger = s_session_trigger;
        s_last_published_retry_deadline_ms = s_auto_retry_deadline_ms;
        s_last_published_time_source = time_snapshot.source;
        s_last_published_time_sample_ms =
            time_snapshot.sample_monotonic_ms;
        s_last_published_manual_request_available =
            manual_request_available;
    }
    else
    {
        ESP_LOGW(TAG, "GPS typed 真值暂未保留，错误=0x%x", (unsigned)error);
    }
    return error;
}

static bool probe_cancel_requested(void)
{
    return s_probe_active &&
           atomic_load(&s_cancel_probe_generation) ==
               s_probe_request.probe_generation &&
           atomic_load(&s_cancel_run_id) == s_probe_request.run_id &&
           atomic_load(&s_cancel_request_id) == s_probe_request.request_id &&
           atomic_load(&s_cancel_probe_kind) == (unsigned)s_probe_request.kind;
}

static void log_fix_diagnostics(const char *stage)
{
    ESP_LOGI(TAG,
             "GPS fix %s诊断：目标帧=%lu，合法 NMEA=%lu，malformed=%lu，拒绝计数=%lu，上次拒绝=%s，RMC状态=%c，GGA质量=%u，解算卫星=%u，GSV帧(GP/BD/GL/GN)=%lu/%lu/%lu/%lu，可见卫星=%u/%u/%u/%u，跟踪卫星=%u/%u/%u/%u，最大C/N0=%u/%u/%u/%u，最强卫星ID=%u/%u/%u/%u，GSA模式=%u，GSA使用(GP/BD/GL/QZ/未知)=%u/%u/%u/%u/%u，GSA末帧系统=%u，DOP(P/H/V)=%.1f/%.1f/%.1f，启动报文序号=%lu，接收机配置=Arduino默认，driver=%d，坐标有效=%d，UART RX=%llu 字节，最长RX间隔=%llu ms，UART TX命令=%lu，ANTENNA OPEN=%lu，运行代次=%lu",
             stage != NULL ? stage : "",
             (unsigned long)s_parser.nmea_frame_sequence,
             (unsigned long)s_parser.nmea_sequence,
             (unsigned long)s_parser.malformed_target_sequence,
             (unsigned long)s_parser.rejection_count,
             nmea_reject_reason_name(s_parser.last_reject_reason),
             s_parser.last_rmc_status != '\0' ? s_parser.last_rmc_status : '?',
             (unsigned)s_parser.last_gga_quality,
             (unsigned)s_parser.last_gga_satellites_used,
             (unsigned long)s_parser.gps_gsv_sentence_count,
             (unsigned long)s_parser.beidou_gsv_sentence_count,
             (unsigned long)s_parser.glonass_gsv_sentence_count,
             (unsigned long)s_parser.multi_gsv_sentence_count,
             (unsigned)s_parser.gps_visible_satellites,
             (unsigned)s_parser.beidou_visible_satellites,
             (unsigned)s_parser.glonass_visible_satellites,
             (unsigned)s_parser.multi_visible_satellites,
             (unsigned)s_parser.gps_tracked_satellites,
             (unsigned)s_parser.beidou_tracked_satellites,
             (unsigned)s_parser.glonass_tracked_satellites,
             (unsigned)s_parser.multi_tracked_satellites,
             (unsigned)s_parser.gps_max_cn0,
             (unsigned)s_parser.beidou_max_cn0,
             (unsigned)s_parser.glonass_max_cn0,
             (unsigned)s_parser.multi_max_cn0,
             (unsigned)s_parser.gps_strongest_satellite_id,
             (unsigned)s_parser.beidou_strongest_satellite_id,
             (unsigned)s_parser.glonass_strongest_satellite_id,
             (unsigned)s_parser.multi_strongest_satellite_id,
             (unsigned)s_parser.last_gsa_fix_mode,
             (unsigned)s_parser.gsa_used_satellites[1],
             (unsigned)s_parser.gsa_used_satellites[4],
             (unsigned)s_parser.gsa_used_satellites[2],
             (unsigned)s_parser.gsa_used_satellites[5],
             (unsigned)s_parser.gsa_used_satellites[0],
             (unsigned)s_parser.last_gsa_system_id,
             s_parser.last_gsa_pdop,
             s_parser.last_gsa_hdop,
             s_parser.last_gsa_vdop,
             (unsigned long)s_parser.receiver_boot_sequence,
             (int)s_parser.driver_available,
             (int)s_parser.coordinates_valid,
             (unsigned long long)s_parser.uart_rx_bytes,
             (unsigned long long)s_parser.longest_uart_gap_ms,
             (unsigned long)s_parser.uart_tx_command_count,
             (unsigned long)s_parser.antenna_open_count,
             (unsigned long)s_run_generation);
    log_gsa_satellite_ids(1U, "GPS");
    log_gsa_satellite_ids(4U, "北斗");
    log_gsa_satellite_ids(2U, "GLONASS");
    log_gsa_satellite_ids(5U, "QZSS");
}

static void log_diagnostic_history(const char *reason)
{
    const uint8_t count = s_parser.diagnostic_history_count;
    ESP_LOGI(TAG,
             "GPS 原始历元快照开始：原因=%s，句数=%u",
             reason != NULL ? reason : "未指定",
             (unsigned)count);
    const uint8_t start =
        (uint8_t)((s_parser.diagnostic_history_next +
                   GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT - count) %
                  GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT);
    for (uint8_t offset = 0U; offset < count; ++offset)
    {
        const uint8_t index =
            (uint8_t)((start + offset) %
                      GPS_SERVICE_DIAGNOSTIC_HISTORY_COUNT);
        const gps_nmea_history_entry_t *entry =
            &s_parser.diagnostic_history[index];
        ESP_LOGI(TAG,
                 "GPS 原始历元：t=%llu ms，%s",
                 (unsigned long long)entry->received_at_ms,
                 entry->sentence);
    }
    ESP_LOGI(TAG,
             "GPS 原始历元快照结束：原因=%s",
             reason != NULL ? reason : "未指定");
}

static void log_gsa_satellite_ids(uint8_t system_id,
                                  const char *system_name)
{
    if (system_id >= GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT ||
        system_name == NULL ||
        s_parser.gsa_used_satellites[system_id] == 0U)
    {
        return;
    }
    char satellite_ids[48];
    format_gsa_satellite_ids(system_id,
                             satellite_ids,
                             sizeof(satellite_ids));
    ESP_LOGI(TAG,
             "GPS GSA 参与解算卫星：星系=%s，数量=%u，ID=%s",
             system_name,
             (unsigned)s_parser.gsa_used_satellites[system_id],
             satellite_ids);
}

static void format_gsa_satellite_ids(uint8_t system_id,
                                     char *buffer,
                                     size_t capacity)
{
    if (buffer == NULL || capacity == 0U)
    {
        return;
    }
    buffer[0] = '\0';
    if (system_id >= GPS_SERVICE_GSA_SYSTEM_SLOT_COUNT)
    {
        return;
    }
    size_t used = 0U;
    const uint8_t satellite_count =
        s_parser.gsa_used_satellites[system_id];
    for (uint8_t index = 0U;
         index < satellite_count &&
         index < GPS_SERVICE_GSA_SATELLITES_MAX;
         ++index)
    {
        const int written = snprintf(
            buffer + used,
            capacity - used,
            index == 0U ? "%u" : ",%u",
            (unsigned)s_parser.gsa_satellite_ids[system_id][index]);
        if (written < 0 || (size_t)written >= capacity - used)
        {
            buffer[capacity - 1U] = '\0';
            return;
        }
        used += (size_t)written;
    }
}

static bool session_is_active(void)
{
    return s_session_state == GPS_SESSION_ACQUIRING ||
           s_session_state == GPS_SESSION_TRACKING;
}

static esp_err_t acquire_gps_pm_lock(void)
{
    if (s_gps_pm_lock_held)
    {
        return ESP_OK;
    }
    if (s_gps_pm_lock == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t error = esp_pm_lock_acquire(s_gps_pm_lock);
    if (error == ESP_OK)
    {
        s_gps_pm_lock_held = true;
    }
    else
    {
        ESP_LOGE(TAG,
                 "GPS acquisition 电源管理锁获取失败，错误=0x%x",
                 (unsigned)error);
    }
    return error;
}

static void release_gps_pm_lock(void)
{
    if (!s_gps_pm_lock_held || s_gps_pm_lock == NULL)
    {
        return;
    }
    const esp_err_t error = esp_pm_lock_release(s_gps_pm_lock);
    if (error == ESP_OK)
    {
        s_gps_pm_lock_held = false;
    }
    else
    {
        ESP_LOGE(TAG,
                 "GPS acquisition 电源管理锁释放失败，错误=0x%x",
                 (unsigned)error);
    }
}

static void release_gps_radio_lock(void)
{
    if (!s_gps_radio_lock_held)
    {
        return;
    }
    const esp_err_t error =
        radio_power_arbiter_release(RADIO_POWER_OWNER_GPS);
    if (error == ESP_OK)
    {
        s_gps_radio_lock_held = false;
    }
    else
    {
        ESP_LOGE(TAG,
                 "GPS/LTE 射频互斥释放失败，错误=0x%x",
                 (unsigned)error);
    }
}

static void finish_gps_radio_shutdown(esp_err_t standby_error,
                                      esp_err_t deinit_error)
{
    if (standby_error == ESP_OK || deinit_error == ESP_OK)
    {
        release_gps_radio_lock();
        return;
    }
    if (s_gps_radio_lock_held)
    {
        ESP_LOGE(TAG,
                 "GPS 未确认 standby/deinit，保持射频互斥并阻止 LTE，standby=0x%x，deinit=0x%x",
                 (unsigned)standby_error,
                 (unsigned)deinit_error);
    }
}

static const char *session_state_name(gps_session_state_t state)
{
    switch (state)
    {
    case GPS_SESSION_STANDBY:
        return "待机";
    case GPS_SESSION_ACQUIRING:
        return "搜星";
    case GPS_SESSION_TRACKING:
        return "持续定位";
    case GPS_SESSION_BACKOFF:
        return "退避";
    case GPS_SESSION_UNAVAILABLE:
    default:
        return "不可用";
    }
}

static const char *trigger_name(gps_acquisition_trigger_t trigger)
{
    switch (trigger)
    {
    case GPS_TRIGGER_BOOT:
        return "启动";
    case GPS_TRIGGER_SCHEDULED:
        return "定时";
    case GPS_TRIGGER_SCREEN_WAKE:
        return "亮屏";
    case GPS_TRIGGER_MANUAL:
        return "手动";
    case GPS_TRIGGER_MOTION:
        return "运动";
    case GPS_TRIGGER_SELFTEST:
    default:
        return "自检";
    }
}

#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
static bool automatic_location_allowed(void)
{
    watch_state_snapshot_t state = {0};
    if (watch_state_snapshot(&state, 0) != ESP_OK)
    {
        return false;
    }
    return state.has_bound_exoskeleton && state.ble_connected &&
           state.gatt_ready && state.notify_ready &&
           state.device_state_available && state.dataReady &&
           state.status_fresh && !state.pending_control &&
           !state.modem_ready;
}
#endif

static esp_err_t start_acquisition(gps_acquisition_purpose_t purpose,
                                   gps_acquisition_trigger_t trigger,
                                   uint64_t now_ms)
{
    if (session_is_active())
    {
        return ESP_OK;
    }
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    if (purpose == GPS_PURPOSE_LOCATION && !automatic_location_allowed())
    {
        ESP_LOGI(TAG,
                 "GPS 运动定位因 BLE、支付或 LTE 前置不满足而延后，触发=%s",
                 trigger_name(trigger));
        return ESP_ERR_INVALID_STATE;
    }
#endif
    esp_err_t error =
        radio_power_arbiter_try_acquire(RADIO_POWER_OWNER_GPS);
    if (error != ESP_OK)
    {
        ESP_LOGI(TAG,
                 "GPS 搜星因 LTE 射频会话占用而延后，触发=%s",
                 trigger_name(trigger));
        return error;
    }
    s_gps_radio_lock_held = true;
    if (!l76kb_a58_bsp_is_started())
    {
        error = l76kb_a58_bsp_start();
    }
    if (error == ESP_OK)
    {
        error = l76kb_a58_bsp_set_standby();
    }
    if (error == ESP_OK)
    {
        error = l76kb_a58_bsp_flush_input();
    }
    if (error == ESP_OK)
    {
        error = acquire_gps_pm_lock();
    }
    if (error == ESP_OK)
    {
        error = l76kb_a58_bsp_set_active();
    }
    if (error != ESP_OK)
    {
        const esp_err_t standby_error = l76kb_a58_bsp_set_standby();
        esp_err_t deinit_error = ESP_FAIL;
        if (standby_error != ESP_OK)
        {
            deinit_error = l76kb_a58_bsp_deinit();
        }
        release_gps_pm_lock();
        finish_gps_radio_shutdown(standby_error, deinit_error);
        s_session_state = GPS_SESSION_UNAVAILABLE;
        parser_set_driver(&s_parser, true, false);
        ESP_LOGE(TAG,
                 "GPS 搜星会话启动失败，触发=%s，原因=%s",
                 trigger_name(trigger),
                 l76kb_a58_bsp_last_error_code());
        return error;
    }

    s_session_generation = next_nonzero_sequence(s_session_generation);
    s_session_state = GPS_SESSION_ACQUIRING;
    s_session_purpose = purpose;
    s_session_trigger = trigger;
    const uint32_t budget_ms =
        gps_acquisition_policy_session_budget_ms(
            s_initial_session_completed,
            trigger == GPS_TRIGGER_BOOT);
    s_session_deadline_ms = now_ms + budget_ms;
    s_session_nmea_deadline_ms =
        now_ms + GPS_SERVICE_NMEA_STARTUP_DEADLINE_MS;
    s_session_baseline_frame_sequence = s_parser.nmea_frame_sequence;
    s_parser.active_session_generation = s_session_generation;
    s_parser.time_candidate_valid = false;
    s_parser.time_sample_ready = false;
    s_scheduled_retry_merged = trigger == GPS_TRIGGER_SCHEDULED;
    s_next_location_prerequisite_check_ms = now_ms;
    parser_set_driver(&s_parser, true, true);
    ESP_LOGI(TAG,
             "GPS 搜星会话已开始，代次=%lu，触发=%s，用途=%u，预算=%lu ms，状态=%s",
             (unsigned long)s_session_generation,
             trigger_name(trigger),
             (unsigned)purpose,
             (unsigned long)budget_ms,
             session_state_name(s_session_state));
    return ESP_OK;
}

static void enter_backoff(uint64_t now_ms, bool advance_stage)
{
    release_gps_pm_lock();
    if (advance_stage)
    {
        s_backoff_stage =
            gps_acquisition_policy_next_backoff_stage(s_backoff_stage);
    }
    const uint64_t delay_ms =
        gps_acquisition_policy_backoff_delay_ms(s_backoff_stage);
    s_auto_retry_deadline_ms = now_ms + delay_ms;
    s_screen_opportunity_used = false;
    s_session_state = GPS_SESSION_BACKOFF;
    s_session_deadline_ms = 0U;
    s_session_nmea_deadline_ms = 0U;
    s_scheduled_retry_merged = false;
    ESP_LOGI(TAG,
             "GPS 已进入退避，阶段=%u，等待=%llu ms，下一截止=%llu ms",
             (unsigned)s_backoff_stage,
             (unsigned long long)delay_ms,
             (unsigned long long)s_auto_retry_deadline_ms);
}

static void finish_successful_acquisition(uint64_t now_ms,
                                          bool *driver_available,
                                          const char **driver_error_code)
{
    s_initial_session_completed = true;
    s_backoff_stage = 0U;
    s_auto_retry_deadline_ms = 0U;
    s_screen_opportunity_used = false;
    s_scheduled_retry_merged = false;
    s_session_deadline_ms = 0U;
    s_session_nmea_deadline_ms = 0U;
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    const esp_err_t standby_error = l76kb_a58_bsp_set_standby();
    release_gps_pm_lock();
    if (standby_error == ESP_OK)
    {
        release_gps_radio_lock();
        s_session_state = GPS_SESSION_STANDBY;
        if (s_session_purpose == GPS_PURPOSE_LOCATION)
        {
            s_auto_retry_deadline_ms =
                now_ms + GPS_SERVICE_MOTION_INTERVAL_MS;
        }
        parser_set_driver(&s_parser, true, true);
        ESP_LOGI(TAG,
                 "GPS 已获得有效定位并进入待机，代次=%lu，下次最早=%llu ms",
                 (unsigned long)s_session_generation,
                 (unsigned long long)s_auto_retry_deadline_ms);
    }
    else
    {
        s_session_state = GPS_SESSION_UNAVAILABLE;
        parser_set_driver(&s_parser, true, false);
        if (driver_available != NULL)
        {
            *driver_available = false;
        }
        if (driver_error_code != NULL)
        {
            *driver_error_code = l76kb_a58_bsp_last_error_code();
        }
        const esp_err_t deinit_error = l76kb_a58_bsp_deinit();
        finish_gps_radio_shutdown(standby_error, deinit_error);
        ESP_LOGE(TAG,
                 "GPS 定位成功但进入待机失败，已请求释放驱动资源，原因=%s",
                 l76kb_a58_bsp_last_error_code());
    }
#else
    const esp_err_t standby_error = l76kb_a58_bsp_set_standby();
    release_gps_pm_lock();
    if (standby_error == ESP_OK)
    {
        release_gps_radio_lock();
        s_session_state = GPS_SESSION_STANDBY;
        parser_set_driver(&s_parser, true, true);
        ESP_LOGI(TAG,
                 "GPS 已完成本次通电授时并进入待机，代次=%lu，时间=%llu ms",
                 (unsigned long)s_session_generation,
                 (unsigned long long)now_ms);
    }
    else
    {
        s_session_state = GPS_SESSION_UNAVAILABLE;
        parser_set_driver(&s_parser, true, false);
        if (driver_available != NULL)
        {
            *driver_available = false;
        }
        if (driver_error_code != NULL)
        {
            *driver_error_code = l76kb_a58_bsp_last_error_code();
        }
        const esp_err_t deinit_error = l76kb_a58_bsp_deinit();
        finish_gps_radio_shutdown(standby_error, deinit_error);
        ESP_LOGE(TAG,
                 "GPS 授时成功但进入待机失败，已请求释放驱动资源，原因=%s",
                 l76kb_a58_bsp_last_error_code());
    }
#endif
}

static void finish_failed_acquisition(uint64_t now_ms,
                                      const char *reason,
                                      bool unavailable,
                                      bool *driver_available,
                                      const char **driver_error_code)
{
    const bool recovering_after_tracking =
        s_session_state == GPS_SESSION_TRACKING;
    const bool preserve_existing_deadline =
        (s_session_trigger == GPS_TRIGGER_MANUAL ||
         s_session_trigger == GPS_TRIGGER_SCREEN_WAKE) &&
        !s_scheduled_retry_merged &&
        s_auto_retry_deadline_ms > now_ms;
    const esp_err_t standby_error = l76kb_a58_bsp_set_standby();
    esp_err_t deinit_error = ESP_FAIL;
    release_gps_pm_lock();
    if (standby_error != ESP_OK)
    {
        unavailable = true;
        if (driver_error_code != NULL)
        {
            *driver_error_code = l76kb_a58_bsp_last_error_code();
        }
        deinit_error = l76kb_a58_bsp_deinit();
        ESP_LOGE(TAG,
                 "GPS 搜星失败后进入待机失败，已请求释放驱动资源，原因=%s",
                 l76kb_a58_bsp_last_error_code());
    }
    finish_gps_radio_shutdown(standby_error, deinit_error);
    clear_fix(&s_parser);
    if (recovering_after_tracking)
    {
        s_initial_session_completed = true;
        s_backoff_stage = 0U;
        enter_backoff(now_ms, false);
    }
    else if (!s_initial_session_completed)
    {
        s_initial_session_completed = true;
        s_backoff_stage = 0U;
        enter_backoff(now_ms, false);
    }
    else if (preserve_existing_deadline)
    {
        s_session_state = GPS_SESSION_BACKOFF;
        s_session_deadline_ms = 0U;
        s_session_nmea_deadline_ms = 0U;
        s_scheduled_retry_merged = false;
    }
    else
    {
        enter_backoff(now_ms, true);
    }
    if (unavailable)
    {
        s_session_state = GPS_SESSION_UNAVAILABLE;
        parser_set_driver(&s_parser, true, false);
        if (driver_available != NULL)
        {
            *driver_available = false;
        }
    }
    ESP_LOGW(TAG,
             "GPS 搜星会话结束且未成功，代次=%lu，原因=%s，保留原截止=%d",
             (unsigned long)s_session_generation,
             reason != NULL ? reason : "未知",
             (int)preserve_existing_deadline);
}

static void finish_selftest_only_session(uint64_t now_ms)
{
    if (s_session_state != GPS_SESSION_ACQUIRING ||
        s_session_purpose != GPS_PURPOSE_SELFTEST)
    {
        return;
    }
    const esp_err_t standby_error = l76kb_a58_bsp_set_standby();
    release_gps_pm_lock();
    clear_fix(&s_parser);
    s_session_deadline_ms = 0U;
    s_session_nmea_deadline_ms = 0U;
    s_scheduled_retry_merged = false;
    if (standby_error != ESP_OK)
    {
        s_session_state = GPS_SESSION_UNAVAILABLE;
        parser_set_driver(&s_parser, true, false);
        const esp_err_t deinit_error = l76kb_a58_bsp_deinit();
        finish_gps_radio_shutdown(standby_error, deinit_error);
        return;
    }
    release_gps_radio_lock();
    parser_set_driver(&s_parser, true, true);
    s_session_state =
        s_auto_retry_deadline_ms > now_ms
            ? GPS_SESSION_BACKOFF
            : GPS_SESSION_STANDBY;
    ESP_LOGI(TAG,
             "GPS 自检独占观察已结束，接收机返回%s",
             s_session_state == GPS_SESSION_BACKOFF ? "原退避窗口" : "待机");
}

static void pause_automatic_acquisition(uint64_t now_ms,
                                        const char *reason,
                                        bool preserve_retry_deadline)
{
    if (!session_is_active() || s_session_purpose == GPS_PURPOSE_SELFTEST)
    {
        return;
    }
    const esp_err_t standby_error = l76kb_a58_bsp_set_standby();
    release_gps_pm_lock();
    clear_fix(&s_parser);
    s_session_deadline_ms = 0U;
    s_session_nmea_deadline_ms = 0U;
    s_scheduled_retry_merged = false;
    if (!preserve_retry_deadline)
    {
        s_auto_retry_deadline_ms = 0U;
    }
    if (standby_error == ESP_OK)
    {
        release_gps_radio_lock();
        s_session_state = preserve_retry_deadline &&
                                  s_auto_retry_deadline_ms > now_ms
                              ? GPS_SESSION_BACKOFF
                              : GPS_SESSION_STANDBY;
        parser_set_driver(&s_parser, true, true);
        ESP_LOGI(TAG,
                 "GPS 自动会话已暂停并进入待机，原因=%s",
                 reason != NULL ? reason : "未知");
        return;
    }
    s_session_state = GPS_SESSION_UNAVAILABLE;
    parser_set_driver(&s_parser, true, false);
    const esp_err_t deinit_error = l76kb_a58_bsp_deinit();
    finish_gps_radio_shutdown(standby_error, deinit_error);
    ESP_LOGE(TAG,
             "GPS 自动会话暂停失败，已请求释放驱动资源，原因=%s，驱动=%s",
             reason != NULL ? reason : "未知",
             l76kb_a58_bsp_last_error_code());
}

static void process_acquisition_request(gps_acquisition_purpose_t purpose,
                                        gps_acquisition_trigger_t trigger,
                                        uint64_t now_ms)
{
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    if (trigger == GPS_TRIGGER_MOTION)
    {
        s_last_motion_ms = now_ms;
        if (session_is_active() ||
            (s_auto_retry_deadline_ms != 0U &&
             now_ms < s_auto_retry_deadline_ms))
        {
            return;
        }
        (void)start_acquisition(GPS_PURPOSE_LOCATION,
                                GPS_TRIGGER_MOTION,
                                now_ms);
        return;
    }
#endif
#if !LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    time_service_snapshot_t time_snapshot = {0};
    if (purpose == GPS_PURPOSE_TIME_SYNC &&
        time_service_get_snapshot(&time_snapshot) == ESP_OK &&
        time_snapshot.valid)
    {
        return;
    }
#endif
    if (session_is_active())
    {
        if (trigger == GPS_TRIGGER_SCHEDULED)
        {
            s_scheduled_retry_merged = true;
        }
        if (purpose == GPS_PURPOSE_LOCATION ||
            (purpose == GPS_PURPOSE_TIME_SYNC &&
             s_session_purpose == GPS_PURPOSE_SELFTEST))
        {
            s_session_purpose = purpose;
        }
        return;
    }
    if ((trigger == GPS_TRIGGER_MANUAL ||
         trigger == GPS_TRIGGER_SCREEN_WAKE) &&
        now_ms < s_active_trigger_cooldown_deadline_ms)
    {
        return;
    }
    if (trigger == GPS_TRIGGER_SCREEN_WAKE)
    {
        bool screen_opportunity_allowed =
            s_session_state == GPS_SESSION_BACKOFF &&
            gps_acquisition_policy_screen_opportunity_allowed(
                s_backoff_stage,
                s_screen_opportunity_used,
                now_ms,
                s_active_trigger_cooldown_deadline_ms);
#if !LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        screen_opportunity_allowed = screen_opportunity_allowed ||
                                     s_session_state == GPS_SESSION_STANDBY;
#endif
        if (!screen_opportunity_allowed)
        {
            return;
        }
        s_screen_opportunity_used =
            s_session_state == GPS_SESSION_BACKOFF;
    }
    if (trigger == GPS_TRIGGER_MANUAL)
    {
        s_screen_opportunity_used = s_backoff_stage > 0U;
    }
    if (trigger == GPS_TRIGGER_MANUAL ||
        trigger == GPS_TRIGGER_SCREEN_WAKE)
    {
        s_active_trigger_cooldown_deadline_ms =
            now_ms + GPS_SERVICE_ACTIVE_TRIGGER_COOLDOWN_MS;
    }
    (void)start_acquisition(purpose, trigger, now_ms);
}

static void advance_session(uint64_t now_ms,
                            bool *driver_available,
                            const char **driver_error_code)
{
    if (driver_available == NULL || driver_error_code == NULL)
    {
        return;
    }
    if (!session_is_active())
    {
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        const bool motion_active =
            gps_acquisition_policy_motion_active(s_last_motion_ms, now_ms);
        if (s_auto_retry_deadline_ms != 0U &&
            now_ms >= s_auto_retry_deadline_ms)
        {
            if (!motion_active || !automatic_location_allowed())
            {
                s_auto_retry_deadline_ms = 0U;
                s_session_state = GPS_SESSION_STANDBY;
                return;
            }
            const esp_err_t error = start_acquisition(
                GPS_PURPOSE_LOCATION,
                GPS_TRIGGER_SCHEDULED,
                now_ms);
            *driver_available = error == ESP_OK ||
                                error == ESP_ERR_INVALID_STATE;
            *driver_error_code = l76kb_a58_bsp_last_error_code();
            if (error != ESP_OK && error != ESP_ERR_INVALID_STATE)
            {
                enter_backoff(now_ms, true);
                s_session_state = GPS_SESSION_UNAVAILABLE;
            }
            else if (error == ESP_ERR_INVALID_STATE)
            {
                s_auto_retry_deadline_ms = 0U;
                s_session_state = GPS_SESSION_STANDBY;
            }
        }
        else if (s_auto_retry_deadline_ms == 0U && motion_active &&
                 automatic_location_allowed())
        {
            const esp_err_t error = start_acquisition(
                GPS_PURPOSE_LOCATION,
                GPS_TRIGGER_MOTION,
                now_ms);
            *driver_available = error == ESP_OK ||
                                error == ESP_ERR_INVALID_STATE;
            *driver_error_code = l76kb_a58_bsp_last_error_code();
        }
#else
        if (s_auto_retry_deadline_ms != 0U &&
            now_ms >= s_auto_retry_deadline_ms)
        {
            const esp_err_t error = start_acquisition(
                GPS_PURPOSE_TIME_SYNC,
                GPS_TRIGGER_SCHEDULED,
                now_ms);
            *driver_available = error == ESP_OK;
            *driver_error_code = l76kb_a58_bsp_last_error_code();
            if (error != ESP_OK)
            {
                enter_backoff(now_ms, true);
                s_session_state = GPS_SESSION_UNAVAILABLE;
            }
        }
#endif
        return;
    }
    if (s_auto_retry_deadline_ms != 0U &&
        now_ms >= s_auto_retry_deadline_ms)
    {
        s_scheduled_retry_merged = true;
        if (s_session_purpose == GPS_PURPOSE_SELFTEST)
        {
            s_session_purpose =
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
                GPS_PURPOSE_LOCATION;
#else
                GPS_PURPOSE_TIME_SYNC;
#endif
            s_session_trigger = GPS_TRIGGER_SCHEDULED;
        }
    }
    if (!*driver_available)
    {
        finish_failed_acquisition(now_ms,
                                  "UART 不可用",
                                  true,
                                  driver_available,
                                  driver_error_code);
        return;
    }

#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
    if (s_session_purpose == GPS_PURPOSE_LOCATION)
    {
        if (!gps_acquisition_policy_motion_active(s_last_motion_ms, now_ms))
        {
            pause_automatic_acquisition(now_ms,
                                        "两分钟内无新运动证据",
                                        false);
            return;
        }
        if (now_ms >= s_next_location_prerequisite_check_ms)
        {
            s_next_location_prerequisite_check_ms = now_ms + 1000U;
            if (!automatic_location_allowed())
            {
                pause_automatic_acquisition(now_ms,
                                            "BLE、支付或 LTE 前置变化",
                                            true);
                return;
            }
        }
    }
    const bool fixed = fix_is_eligible(&s_parser, now_ms);
    if (fixed)
    {
        finish_successful_acquisition(now_ms,
                                      driver_available,
                                      driver_error_code);
        return;
    }
#else
    time_service_snapshot_t time_snapshot = {0};
    if (time_service_get_snapshot(&time_snapshot) == ESP_OK &&
        time_snapshot.valid)
    {
        finish_successful_acquisition(now_ms,
                                      driver_available,
                                      driver_error_code);
        return;
    }
#endif
    if (now_ms >= s_session_nmea_deadline_ms &&
        !sequence_is_after(s_parser.nmea_frame_sequence,
                           s_session_baseline_frame_sequence))
    {
        *driver_error_code = GPS_SERVICE_ERROR_NMEA_TIMEOUT;
        finish_failed_acquisition(now_ms,
                                  "30 秒内无合法 NMEA",
                                  true,
                                  driver_available,
                                  driver_error_code);
        return;
    }
    if (now_ms >= s_session_deadline_ms)
    {
        finish_failed_acquisition(now_ms,
                                  "搜星预算到期",
                                  false,
                                  driver_available,
                                  driver_error_code);
    }
}

static void submit_ready_time_sample(void)
{
    if (!s_parser.time_sample_ready ||
        s_parser.time_sample_sequence == s_submitted_time_sample_sequence)
    {
        return;
    }
    time_service_snapshot_t current_time = {0};
    if (time_service_get_snapshot(&current_time) == ESP_OK &&
        current_time.valid)
    {
        /*
         * 本次通电取得首个可信时间后保持单调偏移不变，避免 v2 持续
         * 定位时每秒更新 watch_state；后续云端样本仍由 time_service
         * 以更高优先级覆盖。
         */
        s_submitted_time_sample_sequence = s_parser.time_sample_sequence;
        return;
    }
    const esp_err_t error =
        time_service_submit_sample(TIME_SOURCE_GPS,
                                   s_parser.time_sample_utc_ms,
                                   s_parser.time_sample_rx_ms);
    s_submitted_time_sample_sequence = s_parser.time_sample_sequence;
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "GPS 连续双帧 UTC 已接受，UTC=%lld ms，接收=%llu ms，会话=%lu",
                 (long long)s_parser.time_sample_utc_ms,
                 (unsigned long long)s_parser.time_sample_rx_ms,
                 (unsigned long)s_parser.active_session_generation);
    }
    else if (error != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "GPS 时间样本被拒绝，错误=0x%x", (unsigned)error);
    }
}

static void process_command(const gps_command_t *command,
                            bool *driver_available,
                            uint64_t now_ms,
                            const char **driver_error_code)
{
    if (command == NULL || driver_available == NULL ||
        driver_error_code == NULL)
    {
        return;
    }
    if (command->type == GPS_COMMAND_STOP)
    {
        atomic_store(&s_stop_requested, true);
        return;
    }
    if (command->type == GPS_COMMAND_SELFTEST_CANCEL)
    {
        if (s_probe_active &&
            request_identity_matches(&command->request, &s_probe_request))
        {
            complete_probe(GPS_SELFTEST_OUTCOME_FAIL, "GPS_SELFTEST_CANCELLED");
        }
        return;
    }
    if (command->type == GPS_COMMAND_PAUSE_FOR_RADIO)
    {
        pause_automatic_acquisition(now_ms,
                                    "显示熄灭或 LTE 即将使用 RF",
                                    true);
        *driver_available =
            s_session_state != GPS_SESSION_UNAVAILABLE &&
            l76kb_a58_bsp_is_started();
        *driver_error_code = l76kb_a58_bsp_last_error_code();
        return;
    }
    if (command->type == GPS_COMMAND_ACQUIRE)
    {
        process_acquisition_request(command->purpose,
                                    command->trigger,
                                    now_ms);
        if (command->trigger == GPS_TRIGGER_MANUAL ||
            command->trigger == GPS_TRIGGER_SCREEN_WAKE)
        {
            atomic_store(&s_manual_request_available, false);
            atomic_store(&s_active_request_pending, false);
        }
        *driver_available =
            s_session_state != GPS_SESSION_UNAVAILABLE &&
            l76kb_a58_bsp_is_started() &&
            (!session_is_active() || l76kb_a58_bsp_is_active());
        *driver_error_code = l76kb_a58_bsp_last_error_code();
        return;
    }
    if (command->type != GPS_COMMAND_SELFTEST_BEGIN)
    {
        return;
    }
    if (s_probe_active)
    {
        emit_probe_result(&command->request,
                          GPS_SELFTEST_OUTCOME_FAIL,
                          "GPS_SELFTEST_BUSY");
        return;
    }
    s_probe_request = command->request;
    s_probe_active = true;
    s_probe_accepted_at_ms = now_ms;
    if (!session_is_active())
    {
        process_acquisition_request(GPS_PURPOSE_SELFTEST,
                                    GPS_TRIGGER_SELFTEST,
                                    now_ms);
        *driver_available =
            s_session_state != GPS_SESSION_UNAVAILABLE &&
            l76kb_a58_bsp_is_started() &&
            (!session_is_active() || l76kb_a58_bsp_is_active());
        *driver_error_code = l76kb_a58_bsp_last_error_code();
    }
    if (!*driver_available)
    {
        complete_probe(
            GPS_SELFTEST_OUTCOME_FAIL,
            command->request.kind == GPS_SELFTEST_PROBE_FIX
                ? "GPS_FIX_UNAVAILABLE"
                : (*driver_error_code != NULL
                       ? *driver_error_code
                       : "GPS_UNAVAILABLE"));
        return;
    }
    if (command->request.kind == GPS_SELFTEST_PROBE_FIX)
    {
        reset_receiver_diagnostics(&s_parser);
    }
    s_probe_baseline_frame_sequence = s_parser.nmea_frame_sequence;
    s_probe_baseline_malformed_sequence =
        s_parser.malformed_target_sequence;
    s_last_fix_progress_log_ms = now_ms;
}

static bool complete_probe(gps_selftest_outcome_t outcome,
                           const char *detail_code)
{
    if (!s_probe_active || detail_code == NULL || s_result_queue == NULL)
    {
        return false;
    }
    if (!emit_probe_result(&s_probe_request, outcome, detail_code))
    {
        return false;
    }
    const bool finish_selftest_session =
        s_session_purpose == GPS_PURPOSE_SELFTEST &&
        s_session_state == GPS_SESSION_ACQUIRING &&
        (s_probe_request.kind == GPS_SELFTEST_PROBE_FIX ||
         outcome != GPS_SELFTEST_OUTCOME_PASS);
    const uint32_t completed_generation = s_probe_request.probe_generation;
    s_probe_active = false;
    memset(&s_probe_request, 0, sizeof(s_probe_request));
    uint32_t expected_generation = completed_generation;
    (void)atomic_compare_exchange_strong(&s_cancel_probe_generation,
                                         &expected_generation,
                                         0U);
    if (finish_selftest_session)
    {
        finish_selftest_only_session(monotonic_ms());
    }
    /* generation=0 已使旧身份失效；保留 run/request，避免覆盖并发发布的新 cancel。 */
    return true;
}

static bool emit_probe_result(const gps_selftest_request_t *request,
                              gps_selftest_outcome_t outcome,
                              const char *detail_code)
{
    if (request == NULL || detail_code == NULL || s_result_queue == NULL)
    {
        return false;
    }
    gps_selftest_result_t result = {
        .run_id = request->run_id,
        .request_id = request->request_id,
        .probe_generation = request->probe_generation,
        .kind = request->kind,
        .outcome = outcome,
    };
    const size_t detail_length = strnlen(detail_code, sizeof(result.detail_code));
    if (detail_length == 0U || detail_length >= sizeof(result.detail_code))
    {
        detail_code = "DRV_GPS_SELFTEST_RESULT_INVALID";
    }
    memcpy(result.detail_code, detail_code, strlen(detail_code) + 1U);
    if (xQueueSend(s_result_queue, &result, 0) == pdTRUE)
    {
        return true;
    }

    /* 结果队列只承载终态；拥塞时淘汰最旧终态并保留最新完整身份。 */
    gps_selftest_result_t discarded = {0};
    const bool discarded_oldest =
        xQueueReceive(s_result_queue, &discarded, 0) == pdTRUE;
    if (xQueueSend(s_result_queue, &result, 0) == pdTRUE)
    {
        if (discarded_oldest)
        {
            ESP_LOGW(TAG,
                     "GPS 自检终态队列拥塞，已淘汰旧 request_id=%lu 并保留新 request_id=%lu",
                     (unsigned long)discarded.request_id,
                     (unsigned long)result.request_id);
        }
        return true;
    }
    ESP_LOGE(TAG, "GPS 自检终态保留失败，request_id=%lu",
             (unsigned long)result.request_id);
    return false;
}

static bool request_identity_matches(const gps_selftest_request_t *left,
                                     const gps_selftest_request_t *right)
{
    return left != NULL && right != NULL &&
           left->run_id == right->run_id &&
           left->request_id == right->request_id &&
           left->probe_generation == right->probe_generation &&
           left->kind == right->kind &&
           left->context == right->context;
}

static esp_err_t enqueue_command(gps_command_type_t type,
                                 const gps_selftest_request_t *request,
                                 TickType_t timeout_ticks)
{
    if (!atomic_load(&s_task_active) ||
        !atomic_load(&s_accepting_commands) ||
        s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    gps_command_t command = {.type = type};
    if (request != NULL)
    {
        command.request = *request;
    }
    if (xQueueSend(s_command_queue, &command, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    return atomic_load(&s_accepting_commands)
               ? ESP_OK
               : ESP_ERR_INVALID_STATE;
}

static esp_err_t enqueue_acquisition(gps_acquisition_purpose_t purpose,
                                     gps_acquisition_trigger_t trigger,
                                     TickType_t timeout_ticks)
{
    if (!atomic_load(&s_task_active) ||
        !atomic_load(&s_accepting_commands) ||
        s_command_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const gps_command_t command = {
        .type = GPS_COMMAND_ACQUIRE,
        .purpose = purpose,
        .trigger = trigger,
    };
    if (xQueueSend(s_command_queue, &command, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    return atomic_load(&s_accepting_commands)
               ? ESP_OK
               : ESP_ERR_INVALID_STATE;
}
#endif
