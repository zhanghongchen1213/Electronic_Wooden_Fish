/**
 * @file     modem_service.c
 * @brief    ML307R 网络生命周期服务实现
 * @details  固定 modem_task 串行驱动唯一 AT reader、bring-up、SIM/驻网/PDP 查询与有界软恢复。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "modem_service.h"

#include <limits.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "app_state.h"
#include "at_core.h"
#include "bsp_resources.h"
#include "config_service.h"
#include "exoskeleton_scene_config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ml307r_bsp.h"
#include "modem_recovery_policy.h"
#include "modem_selftest_policy.h"
#include "radio_power_arbiter.h"
#include "state_service.h"

static const char *TAG = "SVC_MODEM";

/** 联网后读取 ML307R 实际运行配置的临时诊断开关，改为 0 可完全取消。 */
#define MODEM_RUNTIME_CONFIG_DEBUG 0
/** owner 启动阶段等待 MATREADY 的有界时间。 */
#define MODEM_MATREADY_WAIT_MS 3000U
/** 普通 AT 命令绝对截止预算。 */
#define MODEM_AT_COMMAND_TIMEOUT_MS 3000U
/** CFUN=4/1 射频迁移从 UART 真正发送后的响应预算。 */
#define MODEM_CFUN_TRANSITION_TIMEOUT_MS 15000U
/** 无业务会话时 CFUN 迁移包含排队和重同步的总预算。 */
#define MODEM_CFUN_BUSINESS_TIMEOUT_MS 20000U
/** 连续失败后尝试 CFUN/MREBOOT 的门限。 */
#define MODEM_SOFT_RECOVERY_THRESHOLD 3U
/** modem STOP 专用命令。 */
#define MODEM_COMMAND_STOP 1U
/** 唤醒处于 CFUN=4 事件等待的 modem owner。 */
#define MODEM_COMMAND_WAKE 2U
/** HTTPS 请求队列深度。 */
#define MODEM_HTTPS_REQUEST_QUEUE_DEPTH 2U
/** HTTPS 完成队列深度。 */
#define MODEM_HTTPS_RESPONSE_QUEUE_DEPTH 6U
/** modem 自检 request/result 队列深度。 */
#define MODEM_SELFTEST_QUEUE_DEPTH 1U
/** STOP 时原生 HTTP TERM/DEL best-effort 清理预算。 */
#define MODEM_STOP_CLEANUP_MS 1250U
/** PDP 拨号 OK 后等待异步 MIPCALL 地址的预算。 */
#define MODEM_PDP_URC_WAIT_MS 3000U
/** 在线期重新确认 SIM READY 的有界周期。 */
#define MODEM_SIM_HEALTH_CHECK_MS 60000U
/** 单次后台、手动或支付联网搜索窗口。 */
#define MODEM_CONNECT_ATTEMPT_MS 60000U
/** 维护页显式联网持有 MANUAL lease 的有界窗口。 */
#define MODEM_MANUAL_LEASE_MS 60000U
/** 未驻网时重新采集信号与注册事实的间隔。 */
#define MODEM_NETWORK_PROBE_INTERVAL_MS 5000U
/** 固定 bring-up 表中 AT+CSQ 对应的网络重采起点。 */
#define MODEM_NETWORK_PROBE_FIRST_STEP 11U
/** 维护页手动联网原子触发位。 */
#define MODEM_CONNECT_TRIGGER_MANUAL_BIT (1U << MODEM_CONNECT_TRIGGER_MANUAL)
/** 支付联网原子触发位。 */
#define MODEM_CONNECT_TRIGGER_PAYMENT_BIT (1U << MODEM_CONNECT_TRIGGER_PAYMENT)
/** Telemetry 云窗口原子触发位。 */
#define MODEM_CONNECT_TRIGGER_TELEMETRY_BIT (1U << MODEM_CONNECT_TRIGGER_TELEMETRY)
/** 支付 RF lease 位。 */
#define MODEM_NETWORK_LEASE_PAYMENT_BIT (1U << MODEM_NETWORK_LEASE_PAYMENT)
/** Telemetry RF lease 位。 */
#define MODEM_NETWORK_LEASE_TELEMETRY_BIT (1U << MODEM_NETWORK_LEASE_TELEMETRY)
/** 维护页 RF lease 位。 */
#define MODEM_NETWORK_LEASE_MANUAL_BIT (1U << MODEM_NETWORK_LEASE_MANUAL)
/** owner 诊断命令与来源标签容量。 */
#define MODEM_DIAGNOSTIC_LABEL_CAPACITY 48U
/** active 加请求队列全部终态所需的 HTTPS outbox 深度。 */
#define MODEM_HTTPS_TERMINAL_BACKLOG_DEPTH (MODEM_HTTPS_REQUEST_QUEUE_DEPTH + 1U)

typedef struct
{
    const char *command;         /**< 固定 AT 命令。 */
    const char *expected_prefix; /**< 可选响应前缀。 */
} modem_init_command_t;

#if MODEM_RUNTIME_CONFIG_DEBUG
typedef struct
{
    const char *name;            /**< 便于日志检索的中文配置名称。 */
    const char *command;         /**< 只读 AT 查询命令。 */
    const char *expected_prefix; /**< 可选响应前缀。 */
} modem_runtime_config_command_t;
#endif

typedef struct
{
    uint32_t request_id;       /**< 调用方非零请求 ID。 */
    TickType_t enqueued_ticks; /**< 请求进入 owner 队列的单调时刻。 */
} modem_selftest_request_t;

typedef enum
{
    MODEM_CONNECT_ORIGIN_AUTOMATIC = 0, /**< 指数退避到期后的自动联网。 */
    MODEM_CONNECT_ORIGIN_MANUAL,      /**< 维护页用户显式联网。 */
    MODEM_CONNECT_ORIGIN_PAYMENT,     /**< 支付验证显式联网。 */
    MODEM_CONNECT_ORIGIN_TELEMETRY,   /**< 三分钟 telemetry 云窗口。 */
} modem_connect_origin_t;

/** ML307R 射频功能级别的 owner 规范真值。 */
typedef enum
{
    MODEM_RADIO_MODE_UNKNOWN = 0,       /**< 尚未通过 CFUN? 取得真值。 */
    MODEM_RADIO_MODE_FULL,              /**< CFUN=1，允许 SIM/驻网/PDP。 */
    MODEM_RADIO_MODE_RF_OFF,            /**< CFUN=4，发射与接收已关闭。 */
    MODEM_RADIO_MODE_TRANSITION_TO_FULL, /**< 正在进入 CFUN=1。 */
    MODEM_RADIO_MODE_TRANSITION_TO_RF_OFF, /**< 正在进入 CFUN=4。 */
} modem_radio_mode_t;

/** 射频迁移后通过基础 AT、关闭回显与 CFUN? 读取真实状态。 */
typedef enum
{
    MODEM_RADIO_VERIFY_IDLE = 0, /**< 当前没有射频对账事务。 */
    MODEM_RADIO_VERIFY_BASIC_AT, /**< 等待基础 AT 终态。 */
    MODEM_RADIO_VERIFY_ECHO_OFF, /**< 等待 ATE0 终态。 */
    MODEM_RADIO_VERIFY_MODE,     /**< 等待 CFUN? 真值。 */
} modem_radio_verify_stage_t;

typedef struct
{
    modem_service_snapshot_t snapshot;                                                   /**< owner 私有网络真值。 */
    size_t init_step;                                                                    /**< 当前顺序 bring-up 步骤。 */
    uint32_t next_request_id;                                                            /**< AT 请求单调 ID。 */
    uint32_t active_request_id;                                                          /**< 当前生命周期 AT 请求。 */
    TickType_t active_started_ticks;                                                     /**< 当前生命周期 AT 提交时刻。 */
    uint32_t active_rx_baseline;                                                         /**< 当前生命周期 AT 提交前的全局 RX 累计值。 */
    char active_command[MODEM_DIAGNOSTIC_LABEL_CAPACITY];                               /**< 当前生命周期 AT 的脱敏诊断命令。 */
    char active_origin[MODEM_DIAGNOSTIC_LABEL_CAPACITY];                                /**< 当前生命周期 AT 的状态机来源。 */
    uint32_t failure_count;                                                              /**< 连续失败次数。 */
    TickType_t next_action_ticks;                                                        /**< 状态机下一动作绝对时间。 */
    modem_recovery_policy_t recovery_cooldown;                                           /**< 自动恢复失败后的指数退避状态。 */
    bool waiting_result;                                                                 /**< 是否等待当前 AT 完成。 */
    bool matready_seen;                                                                  /**< 是否观察到 MATREADY。 */
    bool startup_gate_pending;                                                           /**< 首条 AT 是否仍受 IO10 启动门禁约束。 */
    TickType_t startup_gate_ticks;                                                       /**< 首条 AT 允许提交的最早绝对 tick。 */
    bool startup_radio_sync_complete;                                                    /**< 启动 AT/ATE0/CFUN? 真值同步是否已收口。 */
    bool connect_attempt_active;                                                         /**< 当前是否处于 60 秒联网会话。 */
    TickType_t connect_attempt_deadline_ticks;                                           /**< 当前联网会话的绝对截止。 */
    uint32_t connect_session_id;                                                         /**< 联网会话单调诊断 ID。 */
    modem_connect_origin_t connect_origin;                                               /**< 当前联网会话触发来源。 */
    uint32_t pending_connect_trigger_mask;                                               /**< 自检或在途 AT 期间保留的显式触发位。 */
    modem_radio_mode_t radio_mode;                                                       /**< 当前已确认或在途的射频功能级别。 */
    modem_radio_mode_t radio_verify_target;                                              /**< 当前 CFUN? 需要确认的目标模式。 */
    modem_radio_verify_stage_t radio_verify_stage;                                       /**< 当前射频对账子阶段。 */
    bool active_is_radio_verify;                                                         /**< 当前 AT 是否为射频迁移后 CFUN? 对账。 */
    bool radio_sleep_disabled;                                                           /**< 本次启动是否因迁移异常禁用射频休眠。 */
    uint8_t radio_wake_retry_count;                                                      /**< 当前会话 CFUN=1 对账重试次数。 */
    bool active_is_backoff_cfun4;                                                        /**< 当前 AT 是否是进入退避前的 CFUN=4。 */
    bool active_is_radio_wake_cfun1;                                                     /**< 当前 AT 是否是退避唤醒的 CFUN=1。 */
    bool active_cfun4_requires_no_demand;                                                /**< 当前 CFUN=4 是否由无网络需求事务提交。 */
    bool radio_backoff_pending;                                                          /**< 需等待当前 AT 终态后执行低功耗退避。 */
    bool pending_backoff_requires_no_demand;                                             /**< 延迟退避是否必须在终态时再次确认仍无需求。 */
    char pending_backoff_error[MODEM_ERROR_CAPACITY];                                   /**< 等待 CFUN=4 收尾的真实失败原因。 */
    bool background_suspended_for_selftest;                                              /**< 后台联网与恢复是否正被整机自检暂停。 */
    bool stop_requested;                                                                 /**< STOP 已接受。 */
    uint32_t last_published_sequence;                                                    /**< 最近提交 state_task 的序号。 */
    uint32_t selftest_request_id;                                                        /**< 当前 owner 自检 ID。 */
    TickType_t selftest_started_ticks;                                                   /**< 当前 owner 自检接受时刻。 */
    TickType_t selftest_deadline_ticks;                                                  /**< 自检 owner 绝对截止。 */
    bool selftest_pending;                                                               /**< owner 正在执行自检查询序列。 */
    modem_selftest_phase_t selftest_phase;                                               /**< 独立于普通 bring-up 的自检阶段。 */
    modem_selftest_phase_t active_selftest_phase;                                        /**< 当前在途 AT 锁存的自检阶段。 */
    bool selftest_pdp_attempted;                                                         /**< 当前自检是否已主动尝试 PDP 激活。 */
    bool selftest_at_confirmed;                                                          /**< 基础 AT 门槛是否已成功。 */
    uint8_t selftest_at_attempts;                                                        /**< 基础 AT 已发起尝试次数。 */
    uint32_t selftest_at_rx_total;                                                       /**< 基础 AT 尝试累计 RX 字节数。 */
    uint8_t baud_probe_index;                                                            /**< 当前波特率候选索引。 */
    uint8_t baud_probe_attempts;                                                         /**< 已探测的候选数量。 */
    char selftest_terminal_reason[40];                                                   /**< 基础 AT 失败的精确终态原因。 */
    bool active_is_apn;                                                                  /**< 当前生命周期 AT 是 APN 配置而非 init_step。 */
    bool active_is_pdp_dial;                                                             /**< 当前生命周期 AT 是 PDP 拨号。 */
    bool active_is_recovery;                                                             /**< 当前 AT 是软恢复命令。 */
    bool active_is_health_cpin;                                                          /**< 当前 AT 是在线期 SIM 健康检查。 */
#if MODEM_RUNTIME_CONFIG_DEBUG
    bool runtime_config_pending;                                                         /**< 当前在线代次是否仍需采集配置。 */
    bool runtime_config_active;                                                          /**< 配置快照是否已经开始且尚未结束。 */
    bool active_is_runtime_config;                                                       /**< 当前在途 AT 是否属于配置快照。 */
    bool runtime_config_restart_pending;                                                 /**< 当前查询结束后是否因 PDP 代次变化重采。 */
    bool runtime_config_restart_requires_recovery;                                       /**< 重采前是否必须先进入既有网络恢复。 */
    size_t runtime_config_step;                                                          /**< 当前配置查询序号。 */
    uint32_t runtime_config_round;                                                       /**< 配置快照轮次。 */
#endif
    uint8_t recovery_stage;                                                              /**< 射频对账与 MREBOOT 有界恢复阶段。 */
    bool apn_applied;                                                                    /**< 当前 bring-up 已应用一次合法非空 APN。 */
    bool pdp_waiting_urc;                                                                /**< PDP 拨号 OK 后等待有效地址 URC。 */
    TickType_t pdp_urc_deadline_ticks;                                                   /**< PDP 地址 URC 绝对截止。 */
    TickType_t next_sim_check_ticks;                                                     /**< 下一次在线 SIM 检查绝对时间。 */
    ml307r_https_response_t pending_https_responses[MODEM_HTTPS_TERMINAL_BACKLOG_DEPTH]; /**< 待重投 HTTPS 终态 FIFO。 */
    size_t pending_https_response_count;                                                 /**< HTTPS 终态 FIFO 当前数量。 */
    bool pending_selftest_result_valid;                                                  /**< 自检结果队列满时保留终态。 */
    modem_selftest_result_t pending_selftest_result;                                     /**< 待重投自检终态。 */
} modem_owner_t;

typedef struct
{
    modem_owner_t owner;                              /**< modem 单 owner 私有状态。 */
    ml307r_https_post_request_t https_request;         /**< HTTPS 请求工作区。 */
    ml307r_https_response_t https_response;            /**< HTTPS 响应工作区。 */
    legbot_at_result_t at_result;                      /**< AT 结果工作区。 */
    legbot_at_urc_t urc;                               /**< URC 工作区。 */
#if MODEM_RUNTIME_CONFIG_DEBUG
    char runtime_config_log_response[
        LEGBOT_AT_RESPONSE_MAX_LENGTH * 2U + 1U];      /**< 运行配置日志转义缓冲区。 */
#endif
} modem_external_workspace_t;

/** 固定 bring-up 命令序列，严格单 in-flight。 */
static const modem_init_command_t s_init_commands[] = {
    {"AT", ""},
    {"ATE0", ""},
    {"AT+CMEE=2", ""},
    {"ATI", ""},
    {"AT+CGMI", ""},
    {"AT+CGMM", ""},
    {"AT+CGMR", ""},
    {"AT+CFUN?", "+CFUN:"},
    {"AT+CGSN", ""},
    {"AT+CGSN=1", ""},
    {"AT+CPIN?", "+CPIN:"},
    {"AT+CSQ", "+CSQ:"},
    {"AT+CESQ", "+CESQ:"},
    {"AT+CEREG?", "+CEREG:"},
    {"AT+CREG?", "+CREG:"},
    {"AT+CGREG?", "+CGREG:"},
    {"AT+COPS?", "+COPS:"},
    {"AT+MIPCALL?", "+MIPCALL:"},
};

_Static_assert(MODEM_NETWORK_PROBE_FIRST_STEP <
                   sizeof(s_init_commands) / sizeof(s_init_commands[0]),
               "Network probe start must reference an init command");

#if MODEM_RUNTIME_CONFIG_DEBUG
/** 联网空闲期只读配置快照，按睡眠、网络节能、注册和 PDP 顺序采集。 */
static const modem_runtime_config_command_t s_runtime_config_commands[] = {
    {"模块身份", "ATI", ""},
    {"固件版本", "AT+CGMR", ""},
    {"功能级别", "AT+CFUN?", "+CFUN:"},
    {"模块睡眠模式", "AT+MLPMCFG=\"sleepmode\"", "+MLPMCFG:"},
    {"延迟睡眠时间", "AT+MLPMCFG=\"delaysleep\"", "+MLPMCFG:"},
    {"睡眠指示配置", "AT+MLPMCFG=\"sleepind\"", "+MLPMCFG:"},
    {"PSM 配置", "AT+CPSMS?", "+CPSMS:"},
    {"eDRX 配置", "AT+CEDRXS?", "+CEDRXS:"},
    {"eDRX 网络协商结果", "AT+CEDRXRDP", "+CEDRXRDP:"},
    {"信令连接状态", "AT+CSCON?", "+CSCON:"},
    {"SIM 状态", "AT+CPIN?", "+CPIN:"},
    {"基础信号质量", "AT+CSQ", "+CSQ:"},
    {"扩展信号质量", "AT+CESQ", "+CESQ:"},
    {"EPS 注册状态", "AT+CEREG?", "+CEREG:"},
    {"当前运营商", "AT+COPS?", "+COPS:"},
    {"分组域附着状态", "AT+CGATT?", "+CGATT:"},
    {"PDP 上下文定义", "AT+CGDCONT?", "+CGDCONT:"},
    {"PDP 上下文激活状态", "AT+CGACT?", "+CGACT:"},
    {"ML307R PDP 地址", "AT+MIPCALL?", "+MIPCALL:"},
};
#endif

/** ML307R 波特率探测候选列表，首项保持项目固定默认值。 */
static const uint32_t s_baud_probe_candidates[MODEM_BAUD_PROBE_CANDIDATE_COUNT] = {
    LEGBOT_BSP_ML307R_UART_BAUD,  /* 115200 */
    921600U,
    460800U,
    230400U,
    57600U,
    38400U,
    19200U,
    9600U,
};

_Static_assert(
    sizeof(s_baud_probe_candidates) / sizeof(s_baud_probe_candidates[0]) ==
        MODEM_BAUD_PROBE_CANDIDATE_COUNT,
    "Modem baud probe candidate count mismatch");

_Static_assert(MODEM_SELFTEST_OWNER_TIMEOUT_MS <
                   MODEM_SELFTEST_RESULT_DEADLINE_MS,
               "Modem owner deadline must leave result scheduling margin");
_Static_assert(MODEM_SELFTEST_RESULT_DEADLINE_MS <
                   MODEM_SELFTEST_TIMEOUT_MS,
               "Modem result deadline must leave framework scheduling margin");
_Static_assert(MODEM_SELFTEST_AT_WORST_CASE_MS <
                   MODEM_SELFTEST_OWNER_TIMEOUT_MS,
               "Modem basic AT gate must fit inside owner deadline");
_Static_assert(MODEM_SELFTEST_BAUD_PROBE_TIMEOUT_MS *
                       MODEM_BAUD_PROBE_CANDIDATE_COUNT <
                   MODEM_SELFTEST_OWNER_TIMEOUT_MS,
               "Baud probe must fit inside owner deadline");

/** modem STOP 私有队列。 */
static QueueHandle_t s_control_queue;
/** cloud consumer 到 modem owner 的 typed HTTPS 请求队列。 */
static QueueHandle_t s_https_request_queue;
/** modem owner 到 cloud consumer 的 typed HTTPS 完成队列。 */
static QueueHandle_t s_https_response_queue;
/** selftest_task 到 modem owner 的 typed 请求队列。 */
static QueueHandle_t s_selftest_request_queue;
/** modem owner 到 selftest_task 的 typed 结果队列。 */
static QueueHandle_t s_selftest_result_queue;
/** modem task 独占的 PSRAM 常驻工作区，不在 cache-off 或 ISR 路径访问。 */
static modem_external_workspace_t *s_external_workspace;
/** modem owner 私有状态在 PSRAM 工作区中的稳定别名。 */
#define s_owner (s_external_workspace->owner)
/** HTTPS 请求工作区在 PSRAM 工作区中的稳定别名。 */
#define s_https_request_scratch (s_external_workspace->https_request)
/** HTTPS 响应工作区在 PSRAM 工作区中的稳定别名。 */
#define s_https_response_scratch (s_external_workspace->https_response)
/** modem prepare 阶段用于清理自检队列的工作区。 */
static modem_selftest_result_t s_selftest_scratch;
/** AT 结果工作区在 PSRAM 工作区中的稳定别名。 */
#define s_at_result_scratch (s_external_workspace->at_result)
/** URC 工作区在 PSRAM 工作区中的稳定别名。 */
#define s_urc_scratch (s_external_workspace->urc)
#if MODEM_RUNTIME_CONFIG_DEBUG
/** 运行配置日志缓冲区在 PSRAM 工作区中的稳定别名。 */
#define s_runtime_config_log_response \
    (s_external_workspace->runtime_config_log_response)
#endif
/** 服务是否已 prepare。 */
static atomic_bool s_prepared;
/** 跨 modem task 重启保持单调的 state update 序号。 */
static atomic_uint s_modem_update_sequence;
/** 已被调用方占用的非零自检请求 ID。 */
static atomic_uint s_claimed_selftest_request_id;
/** 调用方请求取消的非零自检请求 ID。 */
static atomic_uint s_selftest_cancel_request_id;
/** 跨任务合并维护页与支付显式联网请求的原子触发位。 */
static atomic_uint s_connect_trigger_mask;
/** 跨任务按位合并的 RF lease。 */
static atomic_uint s_network_lease_mask;
/** 维护页 MANUAL lease 的绝对截止 tick；是否持有只由 MANUAL lease 位判定。 */
static atomic_uint s_manual_lease_deadline_ticks;
/** cloud owner 请求取消的 transport 请求 ID。 */
static atomic_uint s_https_cancel_request_id;
/** LTE demand 发布与射频 owner 释放共用的 SMP 临界区。 */
static portMUX_TYPE s_radio_demand_lock = portMUX_INITIALIZER_UNLOCKED;
/** LTE RF 会话期间禁止自动 light sleep 的最小作用域锁。 */
static esp_pm_lock_handle_t s_modem_pm_lock;
/** LTE RF 状态未确认关闭前固定最高 CPU 频率的锁。 */
static esp_pm_lock_handle_t s_modem_cpu_max_lock;
/** LTE RF 电源管理锁是否已由 modem owner 持有。 */
static bool s_modem_pm_lock_held;
/** LTE RF 最高 CPU 频率锁是否已由 modem owner 持有。 */
static bool s_modem_cpu_max_lock_held;

bool modem_service_init_command_is_connectivity_gate(
    const char *command)
{
    return command == NULL ||
           strcmp(command, "AT") == 0 ||
           strcmp(command, "AT+CPIN?") == 0 ||
           strcmp(command, "AT+CSQ") == 0 ||
           strcmp(command, "AT+MIPCALL?") == 0;
}

const char *modem_selftest_phase_command(modem_selftest_phase_t phase)
{
    switch (phase)
    {
    case MODEM_SELFTEST_PHASE_CFUN_WAKE:
        return "AT+CFUN=1";
    case MODEM_SELFTEST_PHASE_CFUN_VERIFY:
        return "AT+CFUN?";
    case MODEM_SELFTEST_PHASE_BAUD_PROBE:
    case MODEM_SELFTEST_PHASE_BASIC_AT:
        return "AT";
    case MODEM_SELFTEST_PHASE_SIM:
        return "AT+CPIN?";
    case MODEM_SELFTEST_PHASE_SIGNAL:
        return "AT+CSQ";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CEREG:
        return "AT+CEREG?";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CREG:
        return "AT+CREG?";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CGREG:
        return "AT+CGREG?";
    case MODEM_SELFTEST_PHASE_PDP_QUERY:
        return "AT+MIPCALL?";
    case MODEM_SELFTEST_PHASE_PDP_ACTIVATE:
        return "AT+MIPCALL=1,1";
    case MODEM_SELFTEST_PHASE_IDLE:
    case MODEM_SELFTEST_PHASE_PDP_WAIT:
    case MODEM_SELFTEST_PHASE_DONE:
    default:
        return NULL;
    }
}

bool modem_selftest_phase_accepts_pdp_urc(
    modem_selftest_phase_t phase)
{
    return phase == MODEM_SELFTEST_PHASE_PDP_WAIT;
}

const char *modem_selftest_classify_snapshot(
    const modem_service_snapshot_t *snapshot,
    bool deadline,
    bool pdp_attempted,
    modem_selftest_outcome_t *outcome)
{
    if (snapshot == NULL || outcome == NULL)
    {
        return "MODEM_SELFTEST_FAILED";
    }
    *outcome = MODEM_SELFTEST_FAIL;
    if (snapshot->sim_status != MODEM_SIM_READY)
    {
        return snapshot->sim_status == MODEM_SIM_MISSING
                   ? "MODEM_SIM_MISSING"
                   : "MODEM_SIM_NOT_READY";
    }
    if (snapshot->signal_status != MODEM_SIGNAL_PRESENT)
    {
        return snapshot->signal_status == MODEM_SIGNAL_NONE
                   ? "MODEM_NO_SIGNAL"
                   : "MODEM_SIGNAL_UNKNOWN";
    }
    if (snapshot->registration != MODEM_REGISTRATION_HOME &&
        snapshot->registration != MODEM_REGISTRATION_ROAMING)
    {
        return "MODEM_NOT_REGISTERED";
    }
    if (snapshot->pdp_status != MODEM_PDP_UP)
    {
        return deadline && !pdp_attempted
                   ? "MODEM_SELFTEST_TIMEOUT"
                   : "MODEM_PDP_FAILED";
    }
    *outcome = MODEM_SELFTEST_PASS;
    return "MODEM_SELFTEST_OK";
}

bool modem_selftest_apply_terminal_state(
    modem_service_snapshot_t *snapshot,
    modem_selftest_outcome_t outcome)
{
    if (snapshot == NULL)
    {
        return false;
    }
    const bool connected =
        outcome == MODEM_SELFTEST_PASS &&
        snapshot->sim_status == MODEM_SIM_READY &&
        snapshot->signal_status == MODEM_SIGNAL_PRESENT &&
        (snapshot->registration == MODEM_REGISTRATION_HOME ||
         snapshot->registration == MODEM_REGISTRATION_ROAMING) &&
        snapshot->pdp_status == MODEM_PDP_UP &&
        snapshot->ip_address[0] != '\0';
    if (connected)
    {
        snapshot->lifecycle = MODEM_LIFECYCLE_HTTPS_READY;
        snapshot->network_status = MODEM_NETWORK_ONLINE;
        return true;
    }
    snapshot->lifecycle = MODEM_LIFECYCLE_BOOTING;
    snapshot->network_status = MODEM_NETWORK_BOOTING;
    snapshot->pdp_status = MODEM_PDP_DOWN;
    snapshot->ip_address[0] = '\0';
    return false;
}

const char *modem_selftest_classify_at_failure(
    at_core_result_status_t status,
    uint32_t rx_bytes)
{
    if (status == AT_CORE_RESULT_MODEM_RESTARTED)
    {
        return NULL;
    }
    if (status == AT_CORE_RESULT_UART_ERROR)
    {
        return "MODEM_UART_ERROR";
    }
    if (status == AT_CORE_RESULT_ERROR ||
        status == AT_CORE_RESULT_CME_ERROR)
    {
        return "MODEM_AT_REJECTED";
    }
    return rx_bytes == 0U
               ? "MODEM_AT_NO_RESPONSE"
               : "MODEM_AT_RESPONSE_TIMEOUT";
}

static void secure_clear(void *buffer, size_t length);
static void saturating_increment(uint32_t *value);
static bool tick_reached(TickType_t now_ticks, TickType_t deadline_ticks);
static const char *modem_at_status_text(at_core_result_status_t status);
static const char *diagnostic_command_origin(const char *command, bool sensitive);
static void set_error(modem_service_snapshot_t *snapshot, const char *code);
static void touch_snapshot(modem_service_snapshot_t *snapshot, TickType_t now_ticks);
static bool copy_bounded(char *destination, size_t capacity, const char *source);
static bool valid_ip_address(const char *text);
static bool valid_ipv4_address(const char *text);
static bool valid_ipv6_address(const char *text);
static int parse_registration_status(const char *line);
static void parse_cpin(modem_service_snapshot_t *snapshot, const char *response);
static void parse_csq(modem_service_snapshot_t *snapshot, const char *response);
static void parse_cesq(modem_service_snapshot_t *snapshot, const char *response);
static void parse_registration(modem_service_snapshot_t *snapshot, const char *response);
static bool registration_is_usable(modem_registration_status_t registration);
static void apply_registration_response(
    modem_service_snapshot_t *snapshot,
    const char *response,
    modem_registration_source_t source);
static void parse_operator(modem_service_snapshot_t *snapshot, const char *response);
static void parse_mipcall(modem_service_snapshot_t *snapshot, const char *response);
static void parse_identity(modem_service_snapshot_t *snapshot,
                           const char *response,
                           bool serial_number);
static const char *selftest_expected_prefix(modem_selftest_phase_t phase);
static const char *selftest_phase_failure_reason(modem_selftest_phase_t phase);
static void reset_selftest_after_restart(TickType_t now_ticks);
static void process_selftest_at_result(const legbot_at_result_t *result,
                                       TickType_t now_ticks,
                                       modem_selftest_phase_t phase);
static void advance_selftest(TickType_t now_ticks);
static uint32_t next_request_id(void);
static esp_err_t submit_command(const char *command,
                                const char *expected_prefix,
                                TickType_t now_ticks,
                                bool sensitive);
static esp_err_t submit_configured_apn(TickType_t now_ticks);
static void process_at_result(const legbot_at_result_t *result, TickType_t now_ticks);
static void process_urc(const legbot_at_urc_t *urc, TickType_t now_ticks);
static const char *connect_origin_name(modem_connect_origin_t origin);
static void begin_connect_attempt(modem_connect_origin_t origin,
                                  TickType_t now_ticks);
static void defer_or_begin_connectivity_retry(const char *error,
                                              TickType_t now_ticks);
static void begin_radio_backoff(const char *error, TickType_t now_ticks);
static bool begin_no_demand_radio_backoff(const char *error,
                                          TickType_t now_ticks);
static void begin_radio_backoff_committed(const char *error,
                                          TickType_t now_ticks,
                                          bool no_demand_commit);
static void finalize_radio_backoff(modem_radio_mode_t confirmed_mode,
                                   TickType_t now_ticks);
static modem_radio_mode_t parse_radio_mode(const char *response);
static esp_err_t begin_radio_mode_reconciliation(
    modem_radio_mode_t target,
    TickType_t now_ticks);
static void handle_radio_mode_verification(
    const legbot_at_result_t *result,
    TickType_t now_ticks);
static void process_explicit_connect_request(TickType_t now_ticks);
static bool network_demand_active_locked(void);
static bool network_demand_active(void);
static uint32_t pending_connect_trigger_snapshot(void);
static uint32_t take_pending_connect_triggers(void);
static bool stop_connect_attempt_if_no_demand(TickType_t now_ticks);
static void expire_manual_network_lease(TickType_t now_ticks);
static void wake_modem_owner(void);
static bool owner_can_wait_for_work(void);
static void release_lte_radio_if_safe(void);
static esp_err_t sync_modem_pm_lock(void);
static void release_modem_pm_lock(void);
static void enter_recovery(const char *error, TickType_t now_ticks);
static void advance_lifecycle(TickType_t now_ticks);
#if MODEM_RUNTIME_CONFIG_DEBUG
static void invalidate_runtime_config(const char *reason);
static void format_runtime_config_response(const legbot_at_result_t *result);
static void process_runtime_config_result(const legbot_at_result_t *result,
                                          TickType_t now_ticks);
static bool advance_runtime_config(TickType_t now_ticks);
#endif
static void final_publish(TickType_t now_ticks);
static void process_https_cancel_request(TickType_t now_ticks);
static void process_https_request(TickType_t now_ticks);
static void process_https_response(TickType_t now_ticks);
static void publish_https_terminal(const ml307r_https_post_request_t *request,
                                   ml307r_https_error_t error,
                                   const char *code,
                                   TickType_t now_ticks);
static void retry_pending_terminals(void);
static void drain_https_requests(TickType_t now_ticks);
static void reset_typed_queues(void);
static void queue_https_response(const ml307r_https_response_t *response);
static void publish_snapshot(void);
static void process_selftest_request(TickType_t now_ticks);
static void process_selftest_completion(TickType_t now_ticks);
static void set_selftest_terminal_reason(const char *reason);
static esp_err_t prepare_external_workspace(void);

esp_err_t modem_service_prepare(void)
{
    if (!at_core_is_initialized())
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t uart_ready_wait_ms = 0U;
    const esp_err_t ready_error =
        ml307r_bsp_uart_ready_wait_ms(&uart_ready_wait_ms);
    if (ready_error != ESP_OK)
    {
        return ready_error;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_prepared, &expected, true))
    {
        return ESP_OK;
    }
    esp_err_t error = prepare_external_workspace();
    if (error == ESP_OK)
    {
        error = ml307r_https_transport_prepare();
    }
    if (error != ESP_OK)
    {
        atomic_store(&s_prepared, false);
        return error;
    }
    error = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,
                                         0,
                                         "modem_rf",
                                         &s_modem_pm_lock);
    if (error != ESP_OK)
    {
        atomic_store(&s_prepared, false);
        return error;
    }
    error = esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX,
                               0,
                               "modem_cpu",
                               &s_modem_cpu_max_lock);
    if (error != ESP_OK)
    {
        (void)esp_pm_lock_delete(s_modem_pm_lock);
        s_modem_pm_lock = NULL;
        atomic_store(&s_prepared, false);
        return error;
    }
    s_modem_pm_lock_held = false;
    s_modem_cpu_max_lock_held = false;
    error = radio_power_arbiter_try_acquire(RADIO_POWER_OWNER_LTE);
    if (error == ESP_OK)
    {
        error = sync_modem_pm_lock();
    }
    if (error != ESP_OK)
    {
        (void)radio_power_arbiter_release(RADIO_POWER_OWNER_LTE);
        release_modem_pm_lock();
        (void)esp_pm_lock_delete(s_modem_cpu_max_lock);
        s_modem_cpu_max_lock = NULL;
        (void)esp_pm_lock_delete(s_modem_pm_lock);
        s_modem_pm_lock = NULL;
        atomic_store(&s_prepared, false);
        return error;
    }
    if (s_control_queue == NULL)
    {
        s_control_queue = xQueueCreate(2U, sizeof(uint32_t));
    }
    if (s_https_request_queue == NULL)
    {
        /* HTTPS 按值载荷只在 cloud/modem task 间传递，使用 PSRAM 避免挤占 PHY/I2S 内存。 */
        s_https_request_queue = xQueueCreateWithCaps(
            MODEM_HTTPS_REQUEST_QUEUE_DEPTH,
            sizeof(ml307r_https_post_request_t),
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (s_https_response_queue == NULL)
    {
        s_https_response_queue = xQueueCreateWithCaps(
            MODEM_HTTPS_RESPONSE_QUEUE_DEPTH,
            sizeof(ml307r_https_response_t),
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (s_selftest_request_queue == NULL)
    {
        s_selftest_request_queue = xQueueCreate(MODEM_SELFTEST_QUEUE_DEPTH,
                                                sizeof(modem_selftest_request_t));
    }
    if (s_selftest_result_queue == NULL)
    {
        s_selftest_result_queue = xQueueCreate(MODEM_SELFTEST_QUEUE_DEPTH,
                                               sizeof(modem_selftest_result_t));
    }
    if (s_control_queue == NULL || s_https_request_queue == NULL ||
        s_https_response_queue == NULL || s_selftest_request_queue == NULL ||
        s_selftest_result_queue == NULL)
    {
        release_modem_pm_lock();
        (void)esp_pm_lock_delete(s_modem_cpu_max_lock);
        s_modem_cpu_max_lock = NULL;
        (void)esp_pm_lock_delete(s_modem_pm_lock);
        s_modem_pm_lock = NULL;
        (void)radio_power_arbiter_release(RADIO_POWER_OWNER_LTE);
        atomic_store(&s_prepared, false);
        return ESP_ERR_NO_MEM;
    }
    reset_typed_queues();
    secure_clear(&s_at_result_scratch, sizeof(s_at_result_scratch));
    secure_clear(&s_urc_scratch, sizeof(s_urc_scratch));
    memset(&s_owner, 0, sizeof(s_owner));
    s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
    s_owner.radio_verify_target = MODEM_RADIO_MODE_UNKNOWN;
    s_owner.radio_verify_stage = MODEM_RADIO_VERIFY_IDLE;
    atomic_store(&s_claimed_selftest_request_id, 0U);
    atomic_store(&s_selftest_cancel_request_id, 0U);
    atomic_store(&s_connect_trigger_mask, 0U);
    atomic_store(&s_network_lease_mask, 0U);
    atomic_store(&s_manual_lease_deadline_ticks, 0U);
    atomic_store(&s_https_cancel_request_id, 0U);
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BOOTING;
    s_owner.snapshot.network_status = MODEM_NETWORK_BOOTING;
    s_owner.snapshot.csq_rssi = 99;
    s_owner.snapshot.csq_ber = 99;
    s_owner.snapshot.cesq_rsrq = 255;
    s_owner.snapshot.cesq_rsrp = 255;
#if MODEM_RUNTIME_CONFIG_DEBUG
    s_owner.runtime_config_pending = true;
#endif
    set_error(&s_owner.snapshot, "MODEM_BOOTING");
    const uint32_t startup_wait_ms =
        uart_ready_wait_ms > MODEM_MATREADY_WAIT_MS
            ? uart_ready_wait_ms
            : MODEM_MATREADY_WAIT_MS;
    s_owner.startup_gate_pending = true;
    s_owner.startup_gate_ticks =
        xTaskGetTickCount() + pdMS_TO_TICKS(startup_wait_ms);
    s_owner.next_action_ticks = s_owner.startup_gate_ticks;
    ESP_LOGI(TAG,
             "4G 后台启动门禁已建立，IO10 剩余等待=%lu ms，MATREADY 观察下限=%u ms，首条 AT 最早等待=%lu ms",
             (unsigned long)uart_ready_wait_ms,
             (unsigned)MODEM_MATREADY_WAIT_MS,
             (unsigned long)startup_wait_ms);
    return ESP_OK;
}

void modem_service_cancel_prepared_run(void)
{
    atomic_store(&s_prepared, false);
    at_core_abort_all(false);
    drain_https_requests(xTaskGetTickCount());
    secure_clear(&s_owner, sizeof(s_owner));
    taskENTER_CRITICAL(&s_radio_demand_lock);
    atomic_store(&s_claimed_selftest_request_id, 0U);
    atomic_store(&s_selftest_cancel_request_id, 0U);
    atomic_store(&s_connect_trigger_mask, 0U);
    atomic_store(&s_network_lease_mask, 0U);
    atomic_store(&s_manual_lease_deadline_ticks, 0U);
    atomic_store(&s_https_cancel_request_id, 0U);
    (void)radio_power_arbiter_release(RADIO_POWER_OWNER_LTE);
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    release_modem_pm_lock();
    if (s_modem_pm_lock != NULL)
    {
        (void)esp_pm_lock_delete(s_modem_pm_lock);
        s_modem_pm_lock = NULL;
    }
    if (s_modem_cpu_max_lock != NULL)
    {
        (void)esp_pm_lock_delete(s_modem_cpu_max_lock);
        s_modem_cpu_max_lock = NULL;
    }
}

esp_err_t modem_service_request_stop(TickType_t timeout_ticks)
{
    if (!atomic_load(&s_prepared) || s_control_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t command = MODEM_COMMAND_STOP;
    return xQueueSend(s_control_queue, &command, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t modem_service_request_connect(
    modem_connect_trigger_t trigger)
{
    if (trigger < MODEM_CONNECT_TRIGGER_MANUAL ||
        trigger > MODEM_CONNECT_TRIGGER_TELEMETRY)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t trigger_bit =
        trigger == MODEM_CONNECT_TRIGGER_PAYMENT
            ? MODEM_CONNECT_TRIGGER_PAYMENT_BIT
            : (trigger == MODEM_CONNECT_TRIGGER_TELEMETRY
                   ? MODEM_CONNECT_TRIGGER_TELEMETRY_BIT
                   : MODEM_CONNECT_TRIGGER_MANUAL_BIT);
    const TickType_t now_ticks = xTaskGetTickCount();
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const esp_err_t radio_error =
        atomic_load(&s_prepared)
            ? radio_power_arbiter_try_acquire(RADIO_POWER_OWNER_LTE)
            : ESP_ERR_INVALID_STATE;
    if (radio_error == ESP_OK)
    {
        (void)atomic_fetch_or(&s_connect_trigger_mask, trigger_bit);
        if (trigger == MODEM_CONNECT_TRIGGER_MANUAL)
        {
            (void)atomic_fetch_or(&s_network_lease_mask,
                                  MODEM_NETWORK_LEASE_MANUAL_BIT);
            atomic_store(&s_manual_lease_deadline_ticks,
                         now_ticks + pdMS_TO_TICKS(MODEM_MANUAL_LEASE_MS));
        }
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (radio_error != ESP_OK)
    {
        return radio_error;
    }
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_network_acquire(modem_network_lease_t lease)
{
    if (lease < MODEM_NETWORK_LEASE_PAYMENT ||
        lease > MODEM_NETWORK_LEASE_MANUAL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t lease_bit = 1U << (uint32_t)lease;
    const uint32_t trigger_bit =
        lease == MODEM_NETWORK_LEASE_PAYMENT
            ? MODEM_CONNECT_TRIGGER_PAYMENT_BIT
            : (lease == MODEM_NETWORK_LEASE_TELEMETRY
                   ? MODEM_CONNECT_TRIGGER_TELEMETRY_BIT
                   : MODEM_CONNECT_TRIGGER_MANUAL_BIT);
    const TickType_t now_ticks = xTaskGetTickCount();
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const esp_err_t radio_error =
        atomic_load(&s_prepared)
            ? radio_power_arbiter_try_acquire(RADIO_POWER_OWNER_LTE)
            : ESP_ERR_INVALID_STATE;
    if (radio_error == ESP_OK)
    {
        (void)atomic_fetch_or(&s_network_lease_mask, lease_bit);
        (void)atomic_fetch_or(&s_connect_trigger_mask, trigger_bit);
        if (lease == MODEM_NETWORK_LEASE_MANUAL)
        {
            atomic_store(&s_manual_lease_deadline_ticks,
                         now_ticks + pdMS_TO_TICKS(MODEM_MANUAL_LEASE_MS));
        }
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (radio_error != ESP_OK)
    {
        return radio_error;
    }
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_network_release(modem_network_lease_t lease)
{
    if (lease < MODEM_NETWORK_LEASE_PAYMENT ||
        lease > MODEM_NETWORK_LEASE_MANUAL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t lease_bit = 1U << (uint32_t)lease;
    const uint32_t trigger_bit =
        lease == MODEM_NETWORK_LEASE_PAYMENT
            ? MODEM_CONNECT_TRIGGER_PAYMENT_BIT
            : (lease == MODEM_NETWORK_LEASE_TELEMETRY
                   ? MODEM_CONNECT_TRIGGER_TELEMETRY_BIT
                   : MODEM_CONNECT_TRIGGER_MANUAL_BIT);
    taskENTER_CRITICAL(&s_radio_demand_lock);
    if (atomic_load(&s_prepared))
    {
        (void)atomic_fetch_and(&s_network_lease_mask, ~lease_bit);
        (void)atomic_fetch_and(&s_connect_trigger_mask, ~trigger_bit);
        s_owner.pending_connect_trigger_mask &= ~trigger_bit;
        if (lease == MODEM_NETWORK_LEASE_MANUAL)
        {
            atomic_store(&s_manual_lease_deadline_ticks, 0U);
        }
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_https_cancel(uint32_t request_id)
{
    if (request_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_https_cancel_request_id, request_id);
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_run(void)
{
    if (!atomic_load(&s_prepared) || s_control_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG, "modem_task 已接管唯一 UART2 AT reader");
    while (!s_owner.stop_requested)
    {
        (void)sync_modem_pm_lock();
        TickType_t now_ticks = xTaskGetTickCount();
        uint32_t control = 0U;
        if (xQueueReceive(s_control_queue, &control, 0) == pdTRUE &&
            control == MODEM_COMMAND_STOP)
        {
#if MODEM_RUNTIME_CONFIG_DEBUG
            invalidate_runtime_config("modem_task 收到停止请求");
#endif
            s_owner.stop_requested = true;
            break;
        }
        expire_manual_network_lease(now_ticks);
        if (owner_can_wait_for_work())
        {
            release_lte_radio_if_safe();
            (void)sync_modem_pm_lock();
            TickType_t wait_ticks = portMAX_DELAY;
            if (s_owner.radio_mode != MODEM_RADIO_MODE_RF_OFF)
            {
                wait_ticks = tick_reached(now_ticks,
                                          s_owner.next_action_ticks)
                                 ? 0U
                                 : s_owner.next_action_ticks - now_ticks;
            }
            if (wait_ticks != 0U &&
                xQueueReceive(s_control_queue,
                              &control,
                              wait_ticks) == pdTRUE)
            {
                if (control == MODEM_COMMAND_STOP)
                {
                    s_owner.stop_requested = true;
                    break;
                }
                continue;
            }
            now_ticks = xTaskGetTickCount();
        }
        esp_err_t poll_error = at_core_owner_poll(
            now_ticks,
            pdMS_TO_TICKS(MODEM_OWNER_POLL_MS));
        if (poll_error != ESP_OK)
        {
            if (s_owner.selftest_pending)
            {
                at_core_abort_all(false);
                s_owner.waiting_result = false;
                s_owner.active_selftest_phase =
                    MODEM_SELFTEST_PHASE_IDLE;
                set_selftest_terminal_reason("MODEM_UART_ERROR");
                ESP_LOGE(TAG,
                         "4G 自检 UART 驱动错误，请求=%lu，尝试=%u",
                         (unsigned long)s_owner.selftest_request_id,
                         (unsigned)s_owner.selftest_at_attempts);
            }
#if MODEM_RUNTIME_CONFIG_DEBUG
            else if (s_owner.active_is_runtime_config &&
                     s_owner.waiting_result)
            {
                ESP_LOGW(TAG,
                         "ML307R 运行配置遇到 UART 读取错误，保留当前终态等待逐项记录，轮次=%lu，序号=%u，命令=%s",
                         (unsigned long)s_owner.runtime_config_round,
                         (unsigned)(s_owner.runtime_config_step + 1U),
                         s_owner.active_command);
            }
#endif
            else
            {
                enter_recovery("MODEM_UART_READ", now_ticks);
            }
        }
        while (at_core_receive_urc(&s_urc_scratch, 0) == ESP_OK)
        {
            if (!ml307r_https_transport_handle_urc(
                    &s_urc_scratch, now_ticks))
            {
                process_urc(&s_urc_scratch, now_ticks);
            }
            secure_clear(&s_urc_scratch, sizeof(s_urc_scratch));
        }
        while (at_core_receive_result(&s_at_result_scratch, 0) ==
               ESP_OK)
        {
            if (!ml307r_https_transport_handle_at_result(
                    &s_at_result_scratch, now_ticks))
            {
                process_at_result(&s_at_result_scratch, now_ticks);
            }
            secure_clear(&s_at_result_scratch,
                         sizeof(s_at_result_scratch));
        }
        process_https_cancel_request(now_ticks);
        process_https_request(now_ticks);
        process_selftest_request(now_ticks);
        (void)ml307r_https_transport_step(now_ticks);
        process_https_response(now_ticks);
        process_selftest_completion(now_ticks);
        retry_pending_terminals();
        process_explicit_connect_request(now_ticks);
        if (sync_modem_pm_lock() == ESP_OK)
        {
            advance_lifecycle(now_ticks);
        }
        (void)sync_modem_pm_lock();
        publish_snapshot();
    }
    ml307r_https_transport_cancel(false);
    drain_https_requests(xTaskGetTickCount());
    retry_pending_terminals();
    const TickType_t cleanup_started_at = xTaskGetTickCount();
    while (xTaskGetTickCount() - cleanup_started_at <
           pdMS_TO_TICKS(MODEM_STOP_CLEANUP_MS))
    {
        const TickType_t cleanup_now = xTaskGetTickCount();
        (void)at_core_owner_poll(cleanup_now, pdMS_TO_TICKS(MODEM_OWNER_POLL_MS));
        while (at_core_receive_result(&s_at_result_scratch, 0) ==
               ESP_OK)
        {
            (void)ml307r_https_transport_handle_at_result(
                &s_at_result_scratch, cleanup_now);
            secure_clear(&s_at_result_scratch,
                         sizeof(s_at_result_scratch));
        }
        (void)ml307r_https_transport_step(cleanup_now);
        process_https_response(cleanup_now);
        retry_pending_terminals();
        if (!ml307r_https_transport_active())
        {
            break;
        }
    }
    process_https_response(xTaskGetTickCount());
    drain_https_requests(xTaskGetTickCount());
    retry_pending_terminals();
    at_core_abort_all(false);
    final_publish(xTaskGetTickCount());
    publish_snapshot();
    atomic_store(&s_prepared, false);
    secure_clear(&s_owner, sizeof(s_owner));
    secure_clear(&s_at_result_scratch, sizeof(s_at_result_scratch));
    secure_clear(&s_urc_scratch, sizeof(s_urc_scratch));
    taskENTER_CRITICAL(&s_radio_demand_lock);
    atomic_store(&s_claimed_selftest_request_id, 0U);
    atomic_store(&s_selftest_cancel_request_id, 0U);
    atomic_store(&s_connect_trigger_mask, 0U);
    atomic_store(&s_network_lease_mask, 0U);
    atomic_store(&s_manual_lease_deadline_ticks, 0U);
    atomic_store(&s_https_cancel_request_id, 0U);
    (void)radio_power_arbiter_release(RADIO_POWER_OWNER_LTE);
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    release_modem_pm_lock();
    if (s_modem_pm_lock != NULL)
    {
        const esp_err_t lock_error = esp_pm_lock_delete(s_modem_pm_lock);
        if (lock_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "modem_task 删除 RF 电源管理锁失败，错误=0x%x",
                     (unsigned)lock_error);
        }
        else
        {
            s_modem_pm_lock = NULL;
        }
    }
    if (s_modem_cpu_max_lock != NULL)
    {
        const esp_err_t lock_error =
            esp_pm_lock_delete(s_modem_cpu_max_lock);
        if (lock_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "modem_task 删除 RF 最高频率锁失败，错误=0x%x",
                     (unsigned)lock_error);
        }
        else
        {
            s_modem_cpu_max_lock = NULL;
        }
    }
    ESP_LOGI(TAG, "modem_task 已有界停止");
    return ESP_OK;
}

esp_err_t modem_service_apply_urc(modem_service_snapshot_t *snapshot,
                                  const char *line,
                                  TickType_t now_ticks)
{
    if (snapshot == NULL || line == NULL ||
        strnlen(line, LEGBOT_AT_URC_MAX_LENGTH) >= LEGBOT_AT_URC_MAX_LENGTH)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strcmp(line, "+MATREADY") == 0)
    {
        snapshot->lifecycle = MODEM_LIFECYCLE_BOOTING;
        snapshot->network_status = MODEM_NETWORK_BOOTING;
        snapshot->sim_status = MODEM_SIM_UNKNOWN;
        snapshot->signal_status = MODEM_SIGNAL_UNKNOWN;
        snapshot->registration = MODEM_REGISTRATION_UNKNOWN;
        snapshot->registration_source = MODEM_REGISTRATION_SOURCE_NONE;
        snapshot->pdp_status = MODEM_PDP_DOWN;
        snapshot->ip_address[0] = '\0';
        set_error(snapshot, "MODEM_RESTARTED");
        touch_snapshot(snapshot, now_ticks);
        return ESP_OK;
    }
    if (strncmp(line, "+CEREG:", 7U) == 0 ||
        strncmp(line, "+CREG:", 6U) == 0 ||
        strncmp(line, "+CGREG:", 7U) == 0)
    {
        apply_registration_response(
            snapshot,
            line,
            strncmp(line, "+CEREG:", 7U) == 0
                ? MODEM_REGISTRATION_SOURCE_CEREG
                : (strncmp(line, "+CREG:", 6U) == 0
                       ? MODEM_REGISTRATION_SOURCE_CREG
                       : MODEM_REGISTRATION_SOURCE_CGREG));
        if (!registration_is_usable(snapshot->registration))
        {
            snapshot->pdp_status = MODEM_PDP_DOWN;
            snapshot->ip_address[0] = '\0';
        }
        touch_snapshot(snapshot, now_ticks);
        return ESP_OK;
    }
    if (strncmp(line, "+CPIN:", 6U) == 0)
    {
        parse_cpin(snapshot, line);
        if (snapshot->sim_status != MODEM_SIM_READY)
        {
            snapshot->registration = MODEM_REGISTRATION_UNKNOWN;
            snapshot->registration_source = MODEM_REGISTRATION_SOURCE_NONE;
            snapshot->pdp_status = MODEM_PDP_DOWN;
            snapshot->ip_address[0] = '\0';
        }
        touch_snapshot(snapshot, now_ticks);
        return ESP_OK;
    }
    if (strncmp(line, "+MIPCALL:", 9U) == 0)
    {
        parse_mipcall(snapshot, line);
        touch_snapshot(snapshot, now_ticks);
        return ESP_OK;
    }
    if (strncmp(line, "+MHTTPURC:", strlen("+MHTTPURC:")) == 0)
    {
        touch_snapshot(snapshot, now_ticks);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t modem_service_https_post_async(const ml307r_https_post_request_t *request,
                                         TickType_t timeout_ticks)
{
    if (!atomic_load(&s_prepared) || s_https_request_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (request == NULL || request->request_id == 0U ||
        request->deadline_ticks == 0U ||
        request->max_retries > CLOUD_REQUEST_RETRY_MAX ||
        tick_reached(xTaskGetTickCount(), request->deadline_ticks))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xQueueSend(s_https_request_queue, request, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_https_receive(ml307r_https_response_t *response,
                                      TickType_t timeout_ticks)
{
    if (!atomic_load(&s_prepared) || s_https_response_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return xQueueReceive(s_https_response_queue, response, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t modem_service_selftest_begin(uint32_t request_id)
{
    if (request_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared) || s_selftest_request_queue == NULL ||
        s_selftest_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    unsigned int expected_request_id = 0U;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const esp_err_t radio_error =
        atomic_load(&s_prepared)
            ? radio_power_arbiter_try_acquire(RADIO_POWER_OWNER_LTE)
            : ESP_ERR_INVALID_STATE;
    const bool request_claimed =
        radio_error == ESP_OK &&
        atomic_compare_exchange_strong(&s_claimed_selftest_request_id,
                                       &expected_request_id,
                                       request_id);
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (radio_error == ESP_ERR_INVALID_STATE &&
        !atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (radio_error != ESP_OK)
    {
        return ESP_ERR_NOT_FINISHED;
    }
    if (!request_claimed)
    {
        return ESP_ERR_TIMEOUT;
    }
    const modem_selftest_request_t request = {
        .request_id = request_id,
        .enqueued_ticks = xTaskGetTickCount(),
    };
    if (xQueueSend(s_selftest_request_queue, &request, 0) != pdTRUE)
    {
        taskENTER_CRITICAL(&s_radio_demand_lock);
        expected_request_id = request_id;
        (void)atomic_compare_exchange_strong(
            &s_claimed_selftest_request_id,
            &expected_request_id,
            0U);
        taskEXIT_CRITICAL(&s_radio_demand_lock);
        wake_modem_owner();
        return ESP_ERR_TIMEOUT;
    }
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_selftest_cancel(uint32_t request_id)
{
    if (request_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (atomic_load(&s_claimed_selftest_request_id) != request_id)
    {
        return ESP_ERR_NOT_FOUND;
    }
    atomic_store(&s_selftest_cancel_request_id, request_id);
    if (atomic_load(&s_claimed_selftest_request_id) != request_id)
    {
        unsigned int expected_cancel_id = request_id;
        (void)atomic_compare_exchange_strong(
            &s_selftest_cancel_request_id,
            &expected_cancel_id,
            0U);
        return ESP_ERR_NOT_FOUND;
    }
    wake_modem_owner();
    return ESP_OK;
}

esp_err_t modem_service_selftest_receive(modem_selftest_result_t *result,
                                         TickType_t timeout_ticks)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_prepared) || s_selftest_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueReceive(s_selftest_result_queue, result, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

static esp_err_t prepare_external_workspace(void)
{
    if (s_external_workspace != NULL)
    {
        return ESP_OK;
    }
    s_external_workspace = heap_caps_calloc(
        1U,
        sizeof(*s_external_workspace),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_external_workspace == NULL)
    {
        ESP_LOGE(TAG,
                 "modem owner PSRAM 常驻工作区申请失败，字节=%lu",
                 (unsigned long)sizeof(*s_external_workspace));
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG,
             "modem owner PSRAM 常驻工作区已就绪，字节=%lu",
             (unsigned long)sizeof(*s_external_workspace));
    return ESP_OK;
}

static void secure_clear(void *buffer, size_t length)
{
    volatile uint8_t *bytes = (volatile uint8_t *)buffer;
    while (length-- > 0U)
    {
        *bytes++ = 0U;
    }
}

static void saturating_increment(uint32_t *value)
{
    if (*value < UINT32_MAX)
    {
        ++(*value);
    }
}

static bool tick_reached(TickType_t now_ticks, TickType_t deadline_ticks)
{
    return (int32_t)(now_ticks - deadline_ticks) >= 0;
}

static const char *modem_at_status_text(at_core_result_status_t status)
{
    switch (status)
    {
    case AT_CORE_RESULT_OK:
        return "OK";
    case AT_CORE_RESULT_ERROR:
        return "ERROR";
    case AT_CORE_RESULT_CME_ERROR:
        return "CME_ERROR";
    case AT_CORE_RESULT_TIMEOUT:
        return "TIMEOUT";
    case AT_CORE_RESULT_CANCELLED:
        return "CANCELLED";
    case AT_CORE_RESULT_UART_ERROR:
        return "UART_ERROR";
    case AT_CORE_RESULT_MODEM_RESTARTED:
        return "MODEM_RESTARTED";
    case AT_CORE_RESULT_RESPONSE_OVERFLOW:
        return "RESPONSE_OVERFLOW";
    case AT_CORE_RESULT_PROMPT:
        return "PROMPT";
    default:
        return "UNKNOWN";
    }
}

static const char *diagnostic_command_origin(const char *command, bool sensitive)
{
    if (s_owner.selftest_pending)
    {
        return "4G自检";
    }
    if (s_owner.active_is_radio_verify)
    {
        return "射频对账";
    }
    if (s_owner.active_is_backoff_cfun4)
    {
        return "射频休眠";
    }
    if (s_owner.active_is_radio_wake_cfun1)
    {
        return "射频唤醒";
    }
#if MODEM_RUNTIME_CONFIG_DEBUG
    if (s_owner.active_is_runtime_config)
    {
        return "联网配置诊断";
    }
#endif
    if (strcmp(command, "AT+CFUN=4") == 0 ||
        strcmp(command, "AT+CFUN=1") == 0 ||
        strcmp(command, "AT+MREBOOT=0") == 0)
    {
        return "软恢复";
    }
    if (strcmp(command, "AT+MIPCALL=1,1") == 0)
    {
        return "PDP拨号";
    }
    if (sensitive)
    {
        return "敏感配置";
    }
    if (strcmp(command, "AT+CPIN?") == 0 &&
        (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_HTTPS_READY ||
         s_owner.init_step >=
             sizeof(s_init_commands) / sizeof(s_init_commands[0])))
    {
        return "SIM健康检查";
    }
    return "自动联网";
}

static void set_error(modem_service_snapshot_t *snapshot, const char *code)
{
    memset(snapshot->last_error, 0, sizeof(snapshot->last_error));
    size_t length = strnlen(code, sizeof(snapshot->last_error));
    if (length >= sizeof(snapshot->last_error) || strncmp(code, "MODEM_", 6U) != 0)
    {
        code = "MODEM_INTERNAL";
        length = strlen(code);
    }
    memcpy(snapshot->last_error, code, length);
}

static void touch_snapshot(modem_service_snapshot_t *snapshot, TickType_t now_ticks)
{
    uint32_t sequence = 0U;
    do
    {
        sequence = atomic_fetch_add(&s_modem_update_sequence, 1U) + 1U;
    } while (sequence == 0U);
    snapshot->sequence = sequence;
    snapshot->updated_at_ticks = now_ticks;
}

static bool copy_bounded(char *destination, size_t capacity, const char *source)
{
    size_t length = 0U;
    while (length < capacity && source[length] != '\0')
    {
        ++length;
    }
    if (length == 0U || length >= capacity)
    {
        destination[0] = '\0';
        return false;
    }
    memcpy(destination, source, length + 1U);
    return true;
}

static bool valid_ip_address(const char *text)
{
    return valid_ipv4_address(text) || valid_ipv6_address(text);
}

static bool valid_ipv4_address(const char *text)
{
    unsigned octets = 0U;
    const char *cursor = text;
    while (*cursor != '\0')
    {
        unsigned value = 0U;
        unsigned digits = 0U;
        while (*cursor >= '0' && *cursor <= '9')
        {
            value = value * 10U + (unsigned)(*cursor - '0');
            ++digits;
            ++cursor;
        }
        if (digits == 0U || digits > 3U || value > 255U)
        {
            return false;
        }
        ++octets;
        if (*cursor == '\0')
        {
            break;
        }
        if (*cursor != '.' || octets >= 4U)
        {
            return false;
        }
        ++cursor;
    }
    return octets == 4U && strcmp(text, "0.0.0.0") != 0;
}

static bool valid_ipv6_address(const char *text)
{
    unsigned groups = 0U;
    bool compressed = false;
    const char *cursor = text;
    if (*cursor == '\0')
    {
        return false;
    }
    while (*cursor != '\0')
    {
        if (*cursor == ':')
        {
            if (cursor[1] != ':' || compressed)
            {
                return false;
            }
            compressed = true;
            cursor += 2;
            if (*cursor == '\0')
            {
                break;
            }
            continue;
        }
        unsigned digits = 0U;
        while ((*cursor >= '0' && *cursor <= '9') ||
               (*cursor >= 'a' && *cursor <= 'f') ||
               (*cursor >= 'A' && *cursor <= 'F'))
        {
            ++digits;
            ++cursor;
        }
        if (digits == 0U || digits > 4U || ++groups > 8U)
        {
            return false;
        }
        if (*cursor == '\0')
        {
            break;
        }
        if (*cursor != ':')
        {
            return false;
        }
        if (cursor[1] == ':')
        {
            if (compressed)
            {
                return false;
            }
            compressed = true;
            cursor += 2;
        }
        else
        {
            ++cursor;
        }
    }
    return groups >= 1U && (compressed ? groups < 8U : groups == 8U) &&
           strcmp(text, "::") != 0;
}

static int parse_registration_status(const char *line)
{
    const char *cursor = strchr(line, ':');
    if (cursor == NULL)
    {
        return -1;
    }
    ++cursor;
    while (*cursor == ' ')
    {
        ++cursor;
    }
    char *end = NULL;
    const long first = strtol(cursor, &end, 10);
    if (end == cursor || first < 0L || first > 5L)
    {
        return -1;
    }
    while (*end == ' ')
    {
        ++end;
    }
    if (*end == ',')
    {
        const char *second = end + 1;
        while (*second == ' ')
        {
            ++second;
        }
        if (*second >= '0' && *second <= '9')
        {
            char *second_end = NULL;
            const long parsed = strtol(second, &second_end, 10);
            if (second_end != second && parsed >= 0L && parsed <= 5L &&
                (*second_end == '\0' || *second_end == ',' || *second_end == '\r' ||
                 *second_end == '\n'))
            {
                return (int)parsed;
            }
        }
    }
    return (int)first;
}

static void parse_cpin(modem_service_snapshot_t *snapshot, const char *response)
{
    const char *prefix = strstr(response, "+CPIN:");
    const char *value = prefix == NULL ? response : prefix + strlen("+CPIN:");
    while (*value == ' ' || *value == '\t')
    {
        ++value;
    }
    char state[24] = {0};
    size_t length = 0U;
    while (value[length] != '\0' && value[length] != '\r' && value[length] != '\n' &&
           length + 1U < sizeof(state))
    {
        state[length] = value[length];
        ++length;
    }
    while (length > 0U && (state[length - 1U] == ' ' || state[length - 1U] == '\t'))
    {
        state[--length] = '\0';
    }
    if (strcmp(state, "READY") == 0)
    {
        snapshot->sim_status = MODEM_SIM_READY;
        if (snapshot->pdp_status != MODEM_PDP_UP)
        {
            snapshot->lifecycle = MODEM_LIFECYCLE_REGISTERING;
            snapshot->network_status = MODEM_NETWORK_NOT_REGISTERED;
            set_error(snapshot, "MODEM_NOT_REGISTERED");
        }
    }
    else if (strcmp(state, "NOT INSERTED") == 0 ||
             strcmp(state, "SIM ABSENT") == 0)
    {
        snapshot->sim_status = MODEM_SIM_MISSING;
        snapshot->network_status = MODEM_NETWORK_SIM_MISSING;
        set_error(snapshot, "MODEM_SIM_MISSING");
    }
    else
    {
        snapshot->sim_status = MODEM_SIM_NOT_READY;
        snapshot->network_status = MODEM_NETWORK_SIM_NOT_READY;
        set_error(snapshot, "MODEM_SIM_NOT_READY");
    }
}

static void parse_csq(modem_service_snapshot_t *snapshot, const char *response)
{
    const char *prefix = strstr(response, "+CSQ:");
    int rssi = 99;
    int ber = 99;
    if (prefix == NULL || sscanf(prefix, "+CSQ: %d,%d", &rssi, &ber) != 2)
    {
        snapshot->signal_status = MODEM_SIGNAL_UNKNOWN;
        snapshot->network_status = MODEM_NETWORK_NO_SIGNAL;
        set_error(snapshot, "MODEM_SIGNAL_UNKNOWN");
        return;
    }
    snapshot->csq_rssi = (int16_t)rssi;
    snapshot->csq_ber = (int16_t)ber;
    if (!((rssi >= 0 && rssi <= 31) || rssi == 99) ||
        !((ber >= 0 && ber <= 7) || ber == 99))
    {
        snapshot->signal_status = MODEM_SIGNAL_UNKNOWN;
        snapshot->network_status = MODEM_NETWORK_NO_SIGNAL;
        set_error(snapshot, "MODEM_SIGNAL_INVALID");
    }
    else if (rssi == 99 || rssi == 0)
    {
        snapshot->signal_status = rssi == 99 ? MODEM_SIGNAL_UNKNOWN : MODEM_SIGNAL_NONE;
        snapshot->network_status = MODEM_NETWORK_NO_SIGNAL;
        set_error(snapshot, rssi == 99 ? "MODEM_SIGNAL_UNKNOWN" : "MODEM_NO_SIGNAL");
    }
    else
    {
        snapshot->signal_status = MODEM_SIGNAL_PRESENT;
    }
}

static void parse_cesq(modem_service_snapshot_t *snapshot, const char *response)
{
    const char *prefix = strstr(response, "+CESQ:");
    int rxlev = 255;
    int ber = 255;
    int rscp = 255;
    int ecno = 255;
    int rsrq = 255;
    int rsrp = 255;
    if (prefix == NULL ||
        sscanf(prefix, "+CESQ: %d,%d,%d,%d,%d,%d",
               &rxlev, &ber, &rscp, &ecno, &rsrq, &rsrp) != 6 ||
        !((rsrq >= 0 && rsrq <= 34) || rsrq == 255) ||
        !((rsrp >= 0 && rsrp <= 97) || rsrp == 255))
    {
        snapshot->cesq_rsrq = 255;
        snapshot->cesq_rsrp = 255;
        return;
    }
    snapshot->cesq_rsrq = (int16_t)rsrq;
    snapshot->cesq_rsrp = (int16_t)rsrp;
    if (snapshot->signal_status == MODEM_SIGNAL_UNKNOWN &&
        (rsrq != 255 || rsrp != 255))
    {
        snapshot->signal_status = MODEM_SIGNAL_PRESENT;
    }
}

static void parse_registration(modem_service_snapshot_t *snapshot, const char *response)
{
    const int status = parse_registration_status(response);
    if (status == 1)
    {
        snapshot->registration = MODEM_REGISTRATION_HOME;
    }
    else if (status == 5)
    {
        snapshot->registration = MODEM_REGISTRATION_ROAMING;
    }
    else if (status == 3)
    {
        snapshot->registration = MODEM_REGISTRATION_DENIED;
        snapshot->network_status = MODEM_NETWORK_NOT_REGISTERED;
        set_error(snapshot, "MODEM_REGISTRATION_DENIED");
    }
    else
    {
        snapshot->registration = MODEM_REGISTRATION_SEARCHING;
        snapshot->network_status = MODEM_NETWORK_NOT_REGISTERED;
        set_error(snapshot, "MODEM_NOT_REGISTERED");
    }
}

static bool registration_is_usable(modem_registration_status_t registration)
{
    return registration == MODEM_REGISTRATION_HOME ||
           registration == MODEM_REGISTRATION_ROAMING;
}

static void apply_registration_response(
    modem_service_snapshot_t *snapshot,
    const char *response,
    modem_registration_source_t source)
{
    const modem_registration_status_t previous = snapshot->registration;
    const modem_registration_source_t previous_source =
        snapshot->registration_source;
    const modem_network_status_t previous_network = snapshot->network_status;
    char previous_error[MODEM_ERROR_CAPACITY] = {0};
    memcpy(previous_error, snapshot->last_error, sizeof(previous_error));
    parse_registration(snapshot, response);
    /* 可用的高优先级注册事实只允许同源或更强来源更新。 */
    if (registration_is_usable(previous) &&
        source < previous_source)
    {
        snapshot->registration = previous;
        snapshot->registration_source = previous_source;
        snapshot->network_status = previous_network;
        memcpy(snapshot->last_error, previous_error,
               sizeof(snapshot->last_error));
        return;
    }
    snapshot->registration_source = source;
}

static void parse_operator(modem_service_snapshot_t *snapshot, const char *response)
{
    const char *first_quote = strchr(response, '"');
    const char *second_quote = first_quote == NULL ? NULL : strchr(first_quote + 1, '"');
    if (first_quote == NULL || second_quote == NULL || second_quote == first_quote + 1 ||
        (size_t)(second_quote - first_quote) >= sizeof(snapshot->operator_name))
    {
        snapshot->operator_name[0] = '\0';
        set_error(snapshot, "MODEM_OPERATOR_READ");
        return;
    }
    const size_t length = (size_t)(second_quote - first_quote - 1);
    memcpy(snapshot->operator_name, first_quote + 1, length);
    snapshot->operator_name[length] = '\0';
}

static void parse_mipcall(modem_service_snapshot_t *snapshot, const char *response)
{
    const char *prefix = strstr(response, "+MIPCALL:");
    if (prefix == NULL)
    {
        snapshot->pdp_status = MODEM_PDP_DOWN;
        snapshot->ip_address[0] = '\0';
        snapshot->network_status = MODEM_NETWORK_PDP_FAILED;
        set_error(snapshot, "MODEM_PDP_FAILED");
        return;
    }
    const char *cursor = prefix + strlen("+MIPCALL:");
    while (*cursor == ' ' || *cursor == '\t')
    {
        ++cursor;
    }
    char *end = NULL;
    const long cid = strtol(cursor, &end, 10);
    if (end == cursor || *end != ',')
    {
        snapshot->pdp_status = MODEM_PDP_DOWN;
        snapshot->ip_address[0] = '\0';
        snapshot->network_status = MODEM_NETWORK_PDP_FAILED;
        set_error(snapshot, "MODEM_PDP_FAILED");
        return;
    }
    cursor = end + 1;
    const long active = strtol(cursor, &end, 10);
    if (end == cursor || *end != ',' || cid != 1L || active != 1L)
    {
        snapshot->pdp_status = MODEM_PDP_DOWN;
        snapshot->ip_address[0] = '\0';
        snapshot->network_status = MODEM_NETWORK_PDP_FAILED;
        set_error(snapshot, "MODEM_PDP_FAILED");
        return;
    }
    const char *ip = end + 1;
    char value[MODEM_IP_CAPACITY] = {0};
    size_t length = 0U;
    if (*ip == '"')
    {
        ++ip;
    }
    while (ip[length] != '\0' && ip[length] != '"' && ip[length] != '\r' &&
           ip[length] != '\n' && ip[length] != ',' &&
           length + 1U < sizeof(value))
    {
        value[length] = ip[length];
        ++length;
    }
    const char *tail = ip + length;
    if (*tail == '"')
    {
        ++tail;
    }
    while (*tail == ' ' || *tail == '\t' || *tail == '\r' || *tail == '\n')
    {
        ++tail;
    }
    if (*tail != '\0' || !valid_ip_address(value) ||
        !copy_bounded(snapshot->ip_address, sizeof(snapshot->ip_address), value))
    {
        snapshot->pdp_status = MODEM_PDP_DOWN;
        snapshot->network_status = MODEM_NETWORK_PDP_FAILED;
        set_error(snapshot, "MODEM_PDP_INVALID_IP");
        return;
    }
    snapshot->pdp_status = MODEM_PDP_UP;
    snapshot->lifecycle = MODEM_LIFECYCLE_PDP_READY;
    snapshot->network_status = MODEM_NETWORK_ONLINE;
    set_error(snapshot, "MODEM_OK");
}

static void parse_identity(modem_service_snapshot_t *snapshot,
                           const char *response,
                           bool serial_number)
{
    char value[MODEM_IDENTITY_CAPACITY] = {0};
    size_t length = 0U;
    for (const char *cursor = response; *cursor != '\0' && length + 1U < sizeof(value); ++cursor)
    {
        if (*cursor >= '0' && *cursor <= '9')
        {
            value[length++] = *cursor;
        }
    }
    if (length < 8U)
    {
        return;
    }
    (void)copy_bounded(serial_number ? snapshot->serial_number : snapshot->imei,
                       MODEM_IDENTITY_CAPACITY,
                       value);
}

static uint32_t next_request_id(void)
{
    do
    {
        ++s_owner.next_request_id;
    } while (s_owner.next_request_id == 0U);
    return s_owner.next_request_id;
}

static const char *selftest_expected_prefix(modem_selftest_phase_t phase)
{
    switch (phase)
    {
    case MODEM_SELFTEST_PHASE_SIM:
        return "+CPIN:";
    case MODEM_SELFTEST_PHASE_SIGNAL:
        return "+CSQ:";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CEREG:
        return "+CEREG:";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CREG:
        return "+CREG:";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CGREG:
        return "+CGREG:";
    case MODEM_SELFTEST_PHASE_PDP_QUERY:
    case MODEM_SELFTEST_PHASE_PDP_ACTIVATE:
        return "+MIPCALL:";
    case MODEM_SELFTEST_PHASE_CFUN_VERIFY:
        return "+CFUN:";
    case MODEM_SELFTEST_PHASE_IDLE:
    case MODEM_SELFTEST_PHASE_CFUN_WAKE:
    case MODEM_SELFTEST_PHASE_BAUD_PROBE:
    case MODEM_SELFTEST_PHASE_BASIC_AT:
    case MODEM_SELFTEST_PHASE_PDP_WAIT:
    case MODEM_SELFTEST_PHASE_DONE:
    default:
        return "";
    }
}

static const char *selftest_phase_failure_reason(modem_selftest_phase_t phase)
{
    switch (phase)
    {
    case MODEM_SELFTEST_PHASE_CFUN_WAKE:
    case MODEM_SELFTEST_PHASE_CFUN_VERIFY:
        return "MODEM_CFUN_WAKE_FAILED";
    case MODEM_SELFTEST_PHASE_SIM:
        return "MODEM_SIM_NOT_READY";
    case MODEM_SELFTEST_PHASE_SIGNAL:
        return "MODEM_SIGNAL_UNKNOWN";
    case MODEM_SELFTEST_PHASE_REGISTRATION_CEREG:
    case MODEM_SELFTEST_PHASE_REGISTRATION_CREG:
    case MODEM_SELFTEST_PHASE_REGISTRATION_CGREG:
        return "MODEM_NOT_REGISTERED";
    case MODEM_SELFTEST_PHASE_PDP_QUERY:
    case MODEM_SELFTEST_PHASE_PDP_ACTIVATE:
    case MODEM_SELFTEST_PHASE_PDP_WAIT:
        return "MODEM_PDP_FAILED";
    case MODEM_SELFTEST_PHASE_IDLE:
    case MODEM_SELFTEST_PHASE_BAUD_PROBE:
    case MODEM_SELFTEST_PHASE_BASIC_AT:
    case MODEM_SELFTEST_PHASE_DONE:
    default:
        return "MODEM_SELFTEST_FAILED";
    }
}

static void reset_selftest_after_restart(TickType_t now_ticks)
{
    s_owner.selftest_phase = MODEM_SELFTEST_PHASE_BAUD_PROBE;
    s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    s_owner.selftest_pdp_attempted = false;
    s_owner.selftest_at_confirmed = false;
    s_owner.selftest_at_attempts = 0U;
    s_owner.selftest_at_rx_total = 0U;
    s_owner.baud_probe_index = 0U;
    s_owner.baud_probe_attempts = 0U;
    s_owner.selftest_terminal_reason[0] = '\0';
    s_owner.matready_seen = true;
    s_owner.pdp_waiting_urc = false;
    s_owner.next_action_ticks =
        now_ticks + pdMS_TO_TICKS(MODEM_MATREADY_WAIT_MS);
}

static esp_err_t submit_command(const char *command,
                                const char *expected_prefix,
                                TickType_t now_ticks,
                                bool sensitive)
{
    const bool cfun_transition =
        strcmp(command, "AT+CFUN=1") == 0 ||
        strcmp(command, "AT+CFUN=4") == 0;
    const uint32_t response_timeout_ms =
        cfun_transition
            ? MODEM_CFUN_TRANSITION_TIMEOUT_MS
            : (s_owner.selftest_pending
                   ? (s_owner.selftest_phase ==
                              MODEM_SELFTEST_PHASE_BAUD_PROBE
                          ? MODEM_SELFTEST_BAUD_PROBE_TIMEOUT_MS
                          : (s_owner.selftest_phase ==
                                     MODEM_SELFTEST_PHASE_BASIC_AT
                                 ? MODEM_SELFTEST_AT_TIMEOUT_MS
                                 : MODEM_AT_COMMAND_TIMEOUT_MS))
                   : MODEM_AT_COMMAND_TIMEOUT_MS);
    TickType_t business_deadline =
        now_ticks + pdMS_TO_TICKS(cfun_transition
                                      ? MODEM_CFUN_BUSINESS_TIMEOUT_MS
                                      : response_timeout_ms);
    if (s_owner.selftest_pending && s_owner.selftest_deadline_ticks != 0U)
    {
        business_deadline = s_owner.selftest_deadline_ticks;
    }
    else if (s_owner.connect_attempt_active &&
             s_owner.connect_attempt_deadline_ticks != 0U)
    {
        business_deadline = s_owner.connect_attempt_deadline_ticks;
    }
    legbot_at_transaction_t transaction = {
        .request_id = next_request_id(),
        .phase = AT_CORE_PHASE_COMMAND,
        .deadline_ticks = business_deadline,
        .response_timeout_ticks =
            pdMS_TO_TICKS(response_timeout_ms),
        .sensitive = sensitive,
    };
    if (!copy_bounded(transaction.command, sizeof(transaction.command), command))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    if (expected_prefix[0] != '\0' &&
        !copy_bounded(transaction.expected_prefix,
                      sizeof(transaction.expected_prefix),
                      expected_prefix))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    esp_err_t err = at_core_submit(&transaction, 0);
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    if (err == ESP_OK)
    {
        s_owner.active_request_id = transaction.request_id;
        s_owner.active_started_ticks = now_ticks;
        s_owner.active_rx_baseline = diagnostics.rx_byte_count;
        (void)copy_bounded(
            s_owner.active_command,
            sizeof(s_owner.active_command),
            sensitive ? "<敏感命令>" : command);
        (void)copy_bounded(
            s_owner.active_origin,
            sizeof(s_owner.active_origin),
            diagnostic_command_origin(command, sensitive));
        s_owner.waiting_result = true;
        s_owner.active_selftest_phase =
            s_owner.selftest_pending
                ? s_owner.selftest_phase
                : MODEM_SELFTEST_PHASE_IDLE;
        if (s_owner.selftest_pending &&
            (s_owner.selftest_phase == MODEM_SELFTEST_PHASE_BAUD_PROBE ||
             s_owner.selftest_phase == MODEM_SELFTEST_PHASE_BASIC_AT) &&
            strcmp(command, "AT") == 0 &&
            s_owner.selftest_at_attempts < UINT8_MAX)
        {
            ++s_owner.selftest_at_attempts;
        }
        ESP_LOGI(TAG,
                 "AT owner 已提交，来源=%s，请求=%lu，命令=%s，响应预算=%lu ms，业务截止 tick=%lu，生命周期=%u，网络=%u，init=%u，失败计数=%lu，恢复阶段=%u，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu",
                 s_owner.active_origin,
                 (unsigned long)s_owner.active_request_id,
                 s_owner.active_command,
                 (unsigned long)response_timeout_ms,
                 (unsigned long)business_deadline,
                 (unsigned)s_owner.snapshot.lifecycle,
                 (unsigned)s_owner.snapshot.network_status,
                 (unsigned)s_owner.init_step,
                 (unsigned long)s_owner.failure_count,
                 (unsigned)s_owner.recovery_stage,
                 (unsigned long)diagnostics.rx_byte_count,
                 (unsigned long)diagnostics.tx_byte_count,
                 (unsigned long)diagnostics.last_rx_ticks);
    }
    else
    {
        ESP_LOGE(TAG,
                 "AT owner 提交失败，来源=%s，命令=%s，错误=0x%x，生命周期=%u，网络=%u，init=%u，失败计数=%lu，恢复阶段=%u，全局 RX=%lu，核心错误=%s",
                 diagnostic_command_origin(command, sensitive),
                 sensitive ? "<敏感命令>" : command,
                 (unsigned)err,
                 (unsigned)s_owner.snapshot.lifecycle,
                 (unsigned)s_owner.snapshot.network_status,
                 (unsigned)s_owner.init_step,
                 (unsigned long)s_owner.failure_count,
                 (unsigned)s_owner.recovery_stage,
                 (unsigned long)diagnostics.rx_byte_count,
                 diagnostics.last_error);
    }
    secure_clear(&transaction, sizeof(transaction));
    return err;
}

static modem_radio_mode_t parse_radio_mode(const char *response)
{
    if (response == NULL)
    {
        return MODEM_RADIO_MODE_UNKNOWN;
    }
    const char *prefix = strstr(response, "+CFUN:");
    int mode = -1;
    if (prefix == NULL || sscanf(prefix, "+CFUN: %d", &mode) != 1)
    {
        return MODEM_RADIO_MODE_UNKNOWN;
    }
    return mode == 1
               ? MODEM_RADIO_MODE_FULL
               : (mode == 4
                      ? MODEM_RADIO_MODE_RF_OFF
                      : MODEM_RADIO_MODE_UNKNOWN);
}

static esp_err_t begin_radio_mode_reconciliation(
    modem_radio_mode_t target,
    TickType_t now_ticks)
{
    if (target != MODEM_RADIO_MODE_UNKNOWN &&
        target != MODEM_RADIO_MODE_FULL &&
        target != MODEM_RADIO_MODE_RF_OFF)
    {
        return ESP_ERR_INVALID_ARG;
    }
    s_owner.active_is_radio_verify = true;
    s_owner.radio_verify_target = target;
    s_owner.radio_verify_stage = MODEM_RADIO_VERIFY_BASIC_AT;
    const esp_err_t error =
        submit_command("AT", "", now_ticks, false);
    if (error != ESP_OK)
    {
        s_owner.active_is_radio_verify = false;
        s_owner.radio_verify_target = MODEM_RADIO_MODE_UNKNOWN;
        s_owner.radio_verify_stage = MODEM_RADIO_VERIFY_IDLE;
    }
    return error;
}

static void handle_radio_mode_verification(
    const legbot_at_result_t *result,
    TickType_t now_ticks)
{
    const modem_radio_mode_t target = s_owner.radio_verify_target;
    const modem_radio_verify_stage_t stage =
        s_owner.radio_verify_stage;
    if (stage == MODEM_RADIO_VERIFY_BASIC_AT &&
        result->status == AT_CORE_RESULT_OK)
    {
        s_owner.radio_verify_stage = MODEM_RADIO_VERIFY_ECHO_OFF;
        if (submit_command("ATE0", "", now_ticks, false) == ESP_OK)
        {
            return;
        }
    }
    else if (stage == MODEM_RADIO_VERIFY_ECHO_OFF &&
             result->status == AT_CORE_RESULT_OK)
    {
        s_owner.radio_verify_stage = MODEM_RADIO_VERIFY_MODE;
        if (submit_command("AT+CFUN?", "+CFUN:", now_ticks, false) == ESP_OK)
        {
            return;
        }
    }
    const modem_radio_mode_t actual =
        stage == MODEM_RADIO_VERIFY_MODE &&
                result->status == AT_CORE_RESULT_OK
            ? parse_radio_mode(result->response)
            : MODEM_RADIO_MODE_UNKNOWN;
    s_owner.active_is_radio_verify = false;
    s_owner.radio_verify_target = MODEM_RADIO_MODE_UNKNOWN;
    s_owner.radio_verify_stage = MODEM_RADIO_VERIFY_IDLE;
    if (target == MODEM_RADIO_MODE_UNKNOWN)
    {
        s_owner.startup_radio_sync_complete = true;
        if (actual == MODEM_RADIO_MODE_RF_OFF)
        {
            finalize_radio_backoff(MODEM_RADIO_MODE_RF_OFF, now_ticks);
            return;
        }
        if (actual == MODEM_RADIO_MODE_FULL)
        {
            s_owner.radio_mode = MODEM_RADIO_MODE_FULL;
            if (network_demand_active())
            {
                s_owner.next_action_ticks = now_ticks;
                touch_snapshot(&s_owner.snapshot, now_ticks);
                ESP_LOGI(TAG,
                         "4G 启动射频对账为 CFUN=1，已有业务 lease，保持全功能并等待联网会话");
                return;
            }
            if (begin_no_demand_radio_backoff("MODEM_IDLE", now_ticks))
            {
                ESP_LOGI(TAG,
                         "4G 启动射频对账为 CFUN=1，当前无业务 lease，开始进入 CFUN=4");
                return;
            }
            if (network_demand_active())
            {
                s_owner.next_action_ticks = now_ticks;
                touch_snapshot(&s_owner.snapshot, now_ticks);
                return;
            }
        }
    }
    if (target != MODEM_RADIO_MODE_UNKNOWN && actual == target)
    {
        s_owner.radio_mode = actual;
        if (actual == MODEM_RADIO_MODE_RF_OFF)
        {
            finalize_radio_backoff(MODEM_RADIO_MODE_RF_OFF, now_ticks);
            return;
        }
        s_owner.radio_wake_retry_count = 0U;
        s_owner.matready_seen = false;
        s_owner.init_step = 0U;
        s_owner.next_action_ticks =
            now_ticks + pdMS_TO_TICKS(MODEM_MATREADY_WAIT_MS);
        touch_snapshot(&s_owner.snapshot, now_ticks);
        ESP_LOGI(TAG,
                 "4G 射频对账成功，会话=%lu，CFUN=%u，等待 %u ms 后开始联网探测",
                 (unsigned long)s_owner.connect_session_id,
                 actual == MODEM_RADIO_MODE_FULL ? 1U : 4U,
                 MODEM_MATREADY_WAIT_MS);
        return;
    }
    if (target == MODEM_RADIO_MODE_FULL &&
        actual == MODEM_RADIO_MODE_RF_OFF &&
        s_owner.radio_wake_retry_count == 0U)
    {
        s_owner.radio_wake_retry_count = 1U;
        s_owner.radio_mode = MODEM_RADIO_MODE_TRANSITION_TO_FULL;
        s_owner.active_is_radio_wake_cfun1 = true;
        if (submit_command("AT+CFUN=1", "", now_ticks, false) == ESP_OK)
        {
            ESP_LOGW(TAG,
                     "4G 射频对账仍为 CFUN=4，执行唯一一次有界唤醒重试");
            return;
        }
        s_owner.active_is_radio_wake_cfun1 = false;
    }
    s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
    s_owner.radio_sleep_disabled = true;
    ESP_LOGW(TAG,
             "4G 射频对账失败，目标=%u，实际=%u，阶段=%u，AT 状态=%u，本次启动禁用射频休眠",
             (unsigned)target,
             (unsigned)actual,
             (unsigned)stage,
             (unsigned)result->status);
    if (target == MODEM_RADIO_MODE_RF_OFF)
    {
        finalize_radio_backoff(actual, now_ticks);
    }
    else
    {
        begin_radio_backoff("MODEM_CFUN_VERIFY_FAILED", now_ticks);
    }
}

static esp_err_t submit_configured_apn(TickType_t now_ticks)
{
    config_service_snapshot_t config = {0};
    if (config_service_snapshot(&config, pdMS_TO_TICKS(20)) != ESP_OK ||
        config.apn_mode != CONFIG_SERVICE_APN_CONFIGURED ||
        config_service_cloud_preflight(&config) != CLOUD_ERROR_NONE)
    {
        return ESP_ERR_NOT_FOUND;
    }
    config_service_cloud_credentials_t credentials = {0};
    esp_err_t err = config_service_cloud_credentials_copy(&credentials,
                                                          pdMS_TO_TICKS(20));
    if (err != ESP_OK)
    {
        secure_clear(&credentials, sizeof(credentials));
        return err;
    }
    char command[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U] = {0};
    const int written = snprintf(command,
                                 sizeof(command),
                                 "AT+CGDCONT=1,\"IPV4V6\",\"%s\"",
                                 credentials.apn);
    secure_clear(&credentials, sizeof(credentials));
    if (written <= 0 || (size_t)written >= sizeof(command))
    {
        secure_clear(command, sizeof(command));
        return ESP_ERR_INVALID_SIZE;
    }
    err = submit_command(command, "", now_ticks, true);
    if (err == ESP_OK)
    {
        s_owner.active_is_apn = true;
    }
    secure_clear(command, sizeof(command));
    return err;
}

static void process_selftest_at_result(const legbot_at_result_t *result,
                                       TickType_t now_ticks,
                                       modem_selftest_phase_t phase)
{
    const char *command = modem_selftest_phase_command(phase);
    ESP_LOGI(TAG,
             "4G 自检阶段完成，请求=%lu，阶段=%u，命令=%s，运行时波特率=%lu，尝试=%u，状态=%u，RX 增量=%lu，MATREADY=%u，SIM=%u，信号=%u，注册=%u，PDP=%u",
             (unsigned long)s_owner.selftest_request_id,
             (unsigned)phase,
             command != NULL ? command : "无",
             (unsigned long)at_core_baud_rate(),
             (unsigned)s_owner.selftest_at_attempts,
             (unsigned)result->status,
             (unsigned long)result->rx_byte_count,
             s_owner.matready_seen ? 1U : 0U,
             (unsigned)s_owner.snapshot.sim_status,
             (unsigned)s_owner.snapshot.signal_status,
             (unsigned)s_owner.snapshot.registration,
             (unsigned)s_owner.snapshot.pdp_status);

    if (result->status == AT_CORE_RESULT_MODEM_RESTARTED)
    {
        reset_selftest_after_restart(now_ticks);
        return;
    }
    if (phase == MODEM_SELFTEST_PHASE_CFUN_WAKE)
    {
        s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
        s_owner.selftest_phase = MODEM_SELFTEST_PHASE_CFUN_VERIFY;
        s_owner.next_action_ticks = now_ticks + 1U;
        ESP_LOGI(TAG,
                 "4G 自检 CFUN=1 已取得终态，请求=%lu，状态=%u，继续执行 CFUN? 对账",
                 (unsigned long)s_owner.selftest_request_id,
                 (unsigned)result->status);
        return;
    }
    if (phase == MODEM_SELFTEST_PHASE_CFUN_VERIFY)
    {
        const modem_radio_mode_t actual =
            result->status == AT_CORE_RESULT_OK
                ? parse_radio_mode(result->response)
                : MODEM_RADIO_MODE_UNKNOWN;
        if (actual == MODEM_RADIO_MODE_FULL)
        {
            s_owner.radio_mode = MODEM_RADIO_MODE_FULL;
            s_owner.radio_wake_retry_count = 0U;
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_BASIC_AT;
            s_owner.next_action_ticks =
                now_ticks + pdMS_TO_TICKS(MODEM_MATREADY_WAIT_MS);
            ESP_LOGI(TAG,
                     "4G 自检射频对账成功，请求=%lu，CFUN=1",
                     (unsigned long)s_owner.selftest_request_id);
            return;
        }
        if (actual == MODEM_RADIO_MODE_RF_OFF &&
            s_owner.radio_wake_retry_count == 0U)
        {
            s_owner.radio_wake_retry_count = 1U;
            s_owner.radio_mode = MODEM_RADIO_MODE_TRANSITION_TO_FULL;
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_CFUN_WAKE;
            s_owner.next_action_ticks = now_ticks + 1U;
            ESP_LOGW(TAG,
                     "4G 自检射频对账仍为 CFUN=4，请求=%lu，执行唯一一次有界唤醒重试",
                     (unsigned long)s_owner.selftest_request_id);
            return;
        }
        s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
        s_owner.radio_sleep_disabled = true;
        set_selftest_terminal_reason("MODEM_CFUN_WAKE_FAILED");
        return;
    }
    if (phase == MODEM_SELFTEST_PHASE_BAUD_PROBE)
    {
        if (UINT32_MAX - s_owner.selftest_at_rx_total <
            result->rx_byte_count)
        {
            s_owner.selftest_at_rx_total = UINT32_MAX;
        }
        else
        {
            s_owner.selftest_at_rx_total += result->rx_byte_count;
        }
        if (result->status == AT_CORE_RESULT_OK)
        {
            s_owner.selftest_at_confirmed = true;
            s_owner.baud_probe_attempts = 0U;
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_BASIC_AT;
            s_owner.next_action_ticks = now_ticks + 1U;
            ESP_LOGI(TAG,
                     "ML307R 波特率探测成功，波特率=%lu，尝试=%u",
                     (unsigned long)at_core_baud_rate(),
                     (unsigned)s_owner.selftest_at_attempts);
            return;
        }
        if (result->status != AT_CORE_RESULT_TIMEOUT ||
            s_owner.baud_probe_attempts >= MODEM_BAUD_PROBE_CANDIDATE_COUNT)
        {
            set_selftest_terminal_reason(
                modem_selftest_classify_at_failure(
                    result->status,
                    s_owner.selftest_at_rx_total));
            return;
        }
        /* 切换到下一个候选波特率 */
        const uint8_t next_index =
            (uint8_t)((s_owner.baud_probe_index + 1U) %
                      MODEM_BAUD_PROBE_CANDIDATE_COUNT);
        const uint32_t next_baud = s_baud_probe_candidates[next_index];
        esp_err_t baud_err = at_core_update_baud_rate(next_baud);
        if (baud_err != ESP_OK)
        {
            set_selftest_terminal_reason("MODEM_UART_ERROR");
            return;
        }
        s_owner.baud_probe_index = next_index;
        ++s_owner.baud_probe_attempts;
        s_owner.next_action_ticks =
            now_ticks + pdMS_TO_TICKS(MODEM_BAUD_PROBE_INTERVAL_MS);
        ESP_LOGI(TAG,
                 "ML307R 切换到候选波特率=%lu，进度=%u/%u",
                 (unsigned long)next_baud,
                 (unsigned)s_owner.baud_probe_attempts,
                 (unsigned)MODEM_BAUD_PROBE_CANDIDATE_COUNT);
        return;
    }
    if (phase == MODEM_SELFTEST_PHASE_BASIC_AT)
    {
        if (UINT32_MAX - s_owner.selftest_at_rx_total <
            result->rx_byte_count)
        {
            s_owner.selftest_at_rx_total = UINT32_MAX;
        }
        else
        {
            s_owner.selftest_at_rx_total += result->rx_byte_count;
        }
        if (result->status == AT_CORE_RESULT_OK)
        {
            s_owner.selftest_at_confirmed = true;
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_SIM;
            s_owner.next_action_ticks = now_ticks + 1U;
            ESP_LOGI(TAG,
                     "ML307R 固定 115200 基础 AT 已确认，尝试=%u",
                     (unsigned)s_owner.selftest_at_attempts);
            return;
        }
        if (result->status != AT_CORE_RESULT_TIMEOUT ||
            s_owner.selftest_at_attempts >= MODEM_SELFTEST_AT_MAX_ATTEMPTS)
        {
            set_selftest_terminal_reason(
                modem_selftest_classify_at_failure(
                    result->status,
                    s_owner.selftest_at_rx_total));
            return;
        }
        s_owner.next_action_ticks =
            now_ticks +
            pdMS_TO_TICKS(MODEM_SELFTEST_AT_RETRY_INTERVAL_MS);
        return;
    }
    if (result->status == AT_CORE_RESULT_UART_ERROR)
    {
        set_selftest_terminal_reason("MODEM_UART_ERROR");
        return;
    }
    if (result->status != AT_CORE_RESULT_OK)
    {
        if (phase == MODEM_SELFTEST_PHASE_PDP_ACTIVATE &&
            s_owner.snapshot.pdp_status == MODEM_PDP_UP)
        {
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_DONE;
            s_owner.next_action_ticks = now_ticks + 1U;
            return;
        }
        if (phase == MODEM_SELFTEST_PHASE_REGISTRATION_CEREG)
        {
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_REGISTRATION_CREG;
            s_owner.next_action_ticks = now_ticks + 1U;
            return;
        }
        if (phase == MODEM_SELFTEST_PHASE_REGISTRATION_CREG)
        {
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_REGISTRATION_CGREG;
            s_owner.next_action_ticks = now_ticks + 1U;
            return;
        }
        set_selftest_terminal_reason(selftest_phase_failure_reason(phase));
        return;
    }

    switch (phase)
    {
    case MODEM_SELFTEST_PHASE_SIM:
        parse_cpin(&s_owner.snapshot, result->response);
        if (s_owner.snapshot.sim_status != MODEM_SIM_READY)
        {
            set_selftest_terminal_reason(
                s_owner.snapshot.sim_status == MODEM_SIM_MISSING
                    ? "MODEM_SIM_MISSING"
                    : "MODEM_SIM_NOT_READY");
            break;
        }
        s_owner.selftest_phase = MODEM_SELFTEST_PHASE_SIGNAL;
        s_owner.next_action_ticks = now_ticks + 1U;
        break;
    case MODEM_SELFTEST_PHASE_SIGNAL:
        parse_csq(&s_owner.snapshot, result->response);
        if (s_owner.snapshot.signal_status != MODEM_SIGNAL_PRESENT)
        {
            set_selftest_terminal_reason(
                s_owner.snapshot.signal_status == MODEM_SIGNAL_NONE
                    ? "MODEM_NO_SIGNAL"
                    : "MODEM_SIGNAL_UNKNOWN");
            break;
        }
        s_owner.selftest_phase = MODEM_SELFTEST_PHASE_REGISTRATION_CEREG;
        s_owner.next_action_ticks = now_ticks + 1U;
        break;
    case MODEM_SELFTEST_PHASE_REGISTRATION_CEREG:
        apply_registration_response(&s_owner.snapshot,
                                    result->response,
                                    MODEM_REGISTRATION_SOURCE_CEREG);
        s_owner.selftest_phase =
            registration_is_usable(s_owner.snapshot.registration)
                ? MODEM_SELFTEST_PHASE_PDP_QUERY
                : MODEM_SELFTEST_PHASE_REGISTRATION_CREG;
        s_owner.next_action_ticks = now_ticks + 1U;
        break;
    case MODEM_SELFTEST_PHASE_REGISTRATION_CREG:
        apply_registration_response(&s_owner.snapshot,
                                    result->response,
                                    MODEM_REGISTRATION_SOURCE_CREG);
        s_owner.selftest_phase =
            registration_is_usable(s_owner.snapshot.registration)
                ? MODEM_SELFTEST_PHASE_PDP_QUERY
                : MODEM_SELFTEST_PHASE_REGISTRATION_CGREG;
        s_owner.next_action_ticks = now_ticks + 1U;
        break;
    case MODEM_SELFTEST_PHASE_REGISTRATION_CGREG:
        apply_registration_response(&s_owner.snapshot,
                                    result->response,
                                    MODEM_REGISTRATION_SOURCE_CGREG);
        if (!registration_is_usable(s_owner.snapshot.registration))
        {
            set_selftest_terminal_reason("MODEM_NOT_REGISTERED");
            break;
        }
        s_owner.selftest_phase = MODEM_SELFTEST_PHASE_PDP_QUERY;
        s_owner.next_action_ticks = now_ticks + 1U;
        break;
    case MODEM_SELFTEST_PHASE_PDP_QUERY:
        parse_mipcall(&s_owner.snapshot, result->response);
        s_owner.selftest_phase =
            s_owner.snapshot.pdp_status == MODEM_PDP_UP
                ? MODEM_SELFTEST_PHASE_DONE
                : MODEM_SELFTEST_PHASE_PDP_ACTIVATE;
        s_owner.next_action_ticks = now_ticks + 1U;
        break;
    case MODEM_SELFTEST_PHASE_PDP_ACTIVATE:
        parse_mipcall(&s_owner.snapshot, result->response);
        if (s_owner.snapshot.pdp_status == MODEM_PDP_UP)
        {
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_DONE;
            s_owner.next_action_ticks = now_ticks + 1U;
            break;
        }
        s_owner.selftest_phase = MODEM_SELFTEST_PHASE_PDP_WAIT;
        s_owner.pdp_waiting_urc = true;
        s_owner.pdp_urc_deadline_ticks =
            now_ticks + pdMS_TO_TICKS(MODEM_PDP_URC_WAIT_MS);
        s_owner.next_action_ticks = s_owner.pdp_urc_deadline_ticks;
        break;
    case MODEM_SELFTEST_PHASE_IDLE:
    case MODEM_SELFTEST_PHASE_CFUN_VERIFY:
    case MODEM_SELFTEST_PHASE_BASIC_AT:
    case MODEM_SELFTEST_PHASE_PDP_WAIT:
    case MODEM_SELFTEST_PHASE_DONE:
    default:
        set_selftest_terminal_reason("MODEM_SELFTEST_FAILED");
        break;
    }
    touch_snapshot(&s_owner.snapshot, now_ticks);
}

static void advance_selftest(TickType_t now_ticks)
{
    if (s_owner.selftest_terminal_reason[0] != '\0')
    {
        return;
    }
    if (s_owner.selftest_phase == MODEM_SELFTEST_PHASE_PDP_WAIT)
    {
        s_owner.pdp_waiting_urc = false;
        if (s_owner.snapshot.pdp_status == MODEM_PDP_UP)
        {
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_DONE;
        }
        else
        {
            set_selftest_terminal_reason("MODEM_PDP_FAILED");
        }
        return;
    }
    const char *command =
        modem_selftest_phase_command(s_owner.selftest_phase);
    if (command == NULL)
    {
        if (s_owner.selftest_phase != MODEM_SELFTEST_PHASE_DONE)
        {
            set_selftest_terminal_reason("MODEM_SELFTEST_FAILED");
        }
        return;
    }
    if (submit_command(command,
                       selftest_expected_prefix(s_owner.selftest_phase),
                       now_ticks,
                       false) != ESP_OK)
    {
        set_selftest_terminal_reason("MODEM_AT_QUEUE_FULL");
        return;
    }
    if (s_owner.selftest_phase == MODEM_SELFTEST_PHASE_PDP_ACTIVATE)
    {
        s_owner.selftest_pdp_attempted = true;
    }
}

#if MODEM_RUNTIME_CONFIG_DEBUG
static void invalidate_runtime_config(const char *reason)
{
    const bool snapshot_started = s_owner.runtime_config_active;
    const bool snapshot_completed = !s_owner.runtime_config_pending;
    if (snapshot_started || snapshot_completed)
    {
        ESP_LOGW(TAG,
                 "ML307R 运行配置快照已作废，轮次=%lu，已完成=%u/%u，原因=%s",
                 (unsigned long)s_owner.runtime_config_round,
                 (unsigned)s_owner.runtime_config_step,
                 (unsigned)(sizeof(s_runtime_config_commands) /
                            sizeof(s_runtime_config_commands[0])),
                 reason != NULL ? reason : "在线代次变化");
    }
    s_owner.runtime_config_pending = true;
    s_owner.runtime_config_active = false;
    s_owner.active_is_runtime_config = false;
    s_owner.runtime_config_restart_pending = false;
    s_owner.runtime_config_restart_requires_recovery = false;
    s_owner.runtime_config_step = 0U;
}

static void format_runtime_config_response(const legbot_at_result_t *result)
{
    const size_t input_length =
        result->response_length <= LEGBOT_AT_RESPONSE_MAX_LENGTH
            ? result->response_length
            : LEGBOT_AT_RESPONSE_MAX_LENGTH;
    size_t output_length = 0U;
    for (size_t index = 0U; index < input_length; ++index)
    {
        const char value = result->response[index];
        if (value == '\r' || value == '\n')
        {
            s_runtime_config_log_response[output_length++] = '\\';
            s_runtime_config_log_response[output_length++] =
                value == '\r' ? 'r' : 'n';
        }
        else
        {
            s_runtime_config_log_response[output_length++] = value;
        }
    }
    s_runtime_config_log_response[output_length] = '\0';
}

static void process_runtime_config_result(const legbot_at_result_t *result,
                                          TickType_t now_ticks)
{
    const size_t command_count =
        sizeof(s_runtime_config_commands) /
        sizeof(s_runtime_config_commands[0]);
    const size_t step = s_owner.runtime_config_step;
    s_owner.active_is_runtime_config = false;
    if (!s_owner.runtime_config_active || step >= command_count)
    {
        ESP_LOGW(TAG,
                 "ML307R 运行配置收到失配终态，轮次=%lu，序号=%u，状态=%s(%u)，RX=%lu",
                 (unsigned long)s_owner.runtime_config_round,
                 (unsigned)step,
                 modem_at_status_text(result->status),
                 (unsigned)result->status,
                 (unsigned long)result->rx_byte_count);
        invalidate_runtime_config("诊断状态失配");
        enter_recovery("MODEM_AT_FAILED", now_ticks);
        return;
    }

    const modem_runtime_config_command_t *item =
        &s_runtime_config_commands[step];
    bool queried_pdp_lost = false;
    if (result->status == AT_CORE_RESULT_OK &&
        strcmp(item->command, "AT+MIPCALL?") == 0)
    {
        const bool previous_pdp_up =
            s_owner.snapshot.pdp_status == MODEM_PDP_UP;
        char previous_ip_address[MODEM_IP_CAPACITY] = {0};
        (void)copy_bounded(previous_ip_address,
                           sizeof(previous_ip_address),
                           s_owner.snapshot.ip_address);
        parse_mipcall(&s_owner.snapshot, result->response);
        queried_pdp_lost =
            previous_pdp_up &&
            s_owner.snapshot.pdp_status != MODEM_PDP_UP;
        if (previous_pdp_up &&
            s_owner.snapshot.pdp_status == MODEM_PDP_UP &&
            strcmp(previous_ip_address,
                   s_owner.snapshot.ip_address) != 0)
        {
            s_owner.runtime_config_restart_pending = true;
            ESP_LOGW(TAG,
                     "ML307R 运行配置查询发现 PDP 有效地址变化，本条记录后重采，轮次=%lu，原地址=%s，新地址=%s",
                     (unsigned long)s_owner.runtime_config_round,
                     previous_ip_address,
                     s_owner.snapshot.ip_address);
        }
    }
    format_runtime_config_response(result);
    if (result->status == AT_CORE_RESULT_OK)
    {
        ESP_LOGI(TAG,
                 "ML307R 运行配置，轮次=%lu，序号=%u/%u，名称=%s，命令=%s，状态=%s(%u)，RX=%lu，响应长度=%u，原始响应(转义)=%s",
                 (unsigned long)s_owner.runtime_config_round,
                 (unsigned)(step + 1U),
                 (unsigned)command_count,
                 item->name,
                 item->command,
                 modem_at_status_text(result->status),
                 (unsigned)result->status,
                 (unsigned long)result->rx_byte_count,
                 (unsigned)result->response_length,
                 s_runtime_config_log_response);
    }
    else
    {
        ESP_LOGW(TAG,
                 "ML307R 运行配置，轮次=%lu，序号=%u/%u，名称=%s，命令=%s，状态=%s(%u)，RX=%lu，响应长度=%u，原始响应(转义)=%s",
                 (unsigned long)s_owner.runtime_config_round,
                 (unsigned)(step + 1U),
                 (unsigned)command_count,
                 item->name,
                 item->command,
                 modem_at_status_text(result->status),
                 (unsigned)result->status,
                 (unsigned long)result->rx_byte_count,
                 (unsigned)result->response_length,
                 s_runtime_config_log_response);
    }

    if (queried_pdp_lost)
    {
        ESP_LOGW(TAG,
                 "ML307R 运行配置查询确认 PDP 已丢失，终止快照并开始有界联网会话，轮次=%lu",
                 (unsigned long)s_owner.runtime_config_round);
        invalidate_runtime_config("PDP 丢失");
        begin_connect_attempt(MODEM_CONNECT_ORIGIN_AUTOMATIC,
                              now_ticks);
        return;
    }

    if (result->status == AT_CORE_RESULT_OK ||
        result->status == AT_CORE_RESULT_ERROR ||
        result->status == AT_CORE_RESULT_CME_ERROR ||
        result->status == AT_CORE_RESULT_RESPONSE_OVERFLOW)
    {
        if (s_owner.runtime_config_restart_pending)
        {
            const bool recovery_required =
                s_owner.runtime_config_restart_requires_recovery;
            const bool registration_lost =
                s_owner.snapshot.registration != MODEM_REGISTRATION_HOME &&
                s_owner.snapshot.registration != MODEM_REGISTRATION_ROAMING;
            invalidate_runtime_config("在线代次发生变化");
            if (recovery_required)
            {
                ESP_LOGW(TAG,
                         "ML307R 运行配置确认网络代次失效，开始有界联网会话，原因=%s",
                         registration_lost
                             ? "MODEM_NOT_REGISTERED"
                             : "MODEM_PDP_FAILED");
                begin_connect_attempt(MODEM_CONNECT_ORIGIN_AUTOMATIC,
                                      now_ticks);
            }
            else
            {
                s_owner.next_action_ticks = now_ticks + 1U;
            }
            return;
        }
        ++s_owner.runtime_config_step;
        s_owner.next_action_ticks = now_ticks + 1U;
        if (s_owner.runtime_config_step >= command_count)
        {
            s_owner.runtime_config_active = false;
            s_owner.runtime_config_pending = false;
            ESP_LOGI(TAG,
                     "ML307R 运行配置快照完成，轮次=%lu，查询数=%u，PDP=%u，生命周期=%u",
                     (unsigned long)s_owner.runtime_config_round,
                     (unsigned)command_count,
                     (unsigned)s_owner.snapshot.pdp_status,
                     (unsigned)s_owner.snapshot.lifecycle);
        }
        return;
    }

    ESP_LOGW(TAG,
             "ML307R 运行配置快照中断，轮次=%lu，序号=%u/%u，命令=%s，状态=%s(%u)，RX=%lu",
             (unsigned long)s_owner.runtime_config_round,
             (unsigned)(step + 1U),
             (unsigned)command_count,
             item->command,
             modem_at_status_text(result->status),
             (unsigned)result->status,
             (unsigned long)result->rx_byte_count);
    enter_recovery(result->status == AT_CORE_RESULT_MODEM_RESTARTED
                       ? "MODEM_RESTARTED"
                       : "MODEM_AT_FAILED",
                   now_ticks);
}

static bool advance_runtime_config(TickType_t now_ticks)
{
    if (!s_owner.runtime_config_pending ||
        s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_HTTPS_READY ||
        s_owner.snapshot.pdp_status != MODEM_PDP_UP ||
        s_owner.selftest_pending ||
        ml307r_https_transport_active() ||
        s_owner.pending_https_response_count > 0U ||
        s_owner.pending_selftest_result_valid ||
        s_owner.waiting_result)
    {
        return false;
    }

    const size_t command_count =
        sizeof(s_runtime_config_commands) /
        sizeof(s_runtime_config_commands[0]);
    if (!s_owner.runtime_config_active)
    {
        s_owner.runtime_config_active = true;
        s_owner.runtime_config_step = 0U;
        saturating_increment(&s_owner.runtime_config_round);
        ESP_LOGI(TAG,
                 "ML307R 运行配置快照开始，轮次=%lu，查询数=%u，PDP=%u，生命周期=%u",
                 (unsigned long)s_owner.runtime_config_round,
                 (unsigned)command_count,
                 (unsigned)s_owner.snapshot.pdp_status,
                 (unsigned)s_owner.snapshot.lifecycle);
    }
    if (s_owner.runtime_config_step >= command_count)
    {
        s_owner.runtime_config_active = false;
        s_owner.runtime_config_pending = false;
        return false;
    }

    const modem_runtime_config_command_t *item =
        &s_runtime_config_commands[s_owner.runtime_config_step];
    s_owner.active_is_runtime_config = true;
    if (submit_command(item->command,
                       item->expected_prefix,
                       now_ticks,
                       false) == ESP_OK)
    {
        return true;
    }

    s_owner.active_is_runtime_config = false;
    ESP_LOGW(TAG,
             "ML307R 运行配置提交繁忙，保持联网真值并延后重试，轮次=%lu，序号=%u/%u，名称=%s，命令=%s",
             (unsigned long)s_owner.runtime_config_round,
             (unsigned)(s_owner.runtime_config_step + 1U),
             (unsigned)command_count,
             item->name,
             item->command);
    s_owner.next_action_ticks = now_ticks + pdMS_TO_TICKS(100U);
    return true;
}
#endif

static void process_at_result(const legbot_at_result_t *result, TickType_t now_ticks)
{
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    if (!s_owner.waiting_result || result->request_id != s_owner.active_request_id)
    {
        ESP_LOGW(TAG,
                 "丢弃非当前 AT 终态，结果请求=%lu，owner 请求=%lu，等待=%u，状态=%s(%u)，事务 RX=%lu，全局 RX=%lu，最后 RX tick=%lu",
                 (unsigned long)result->request_id,
                 (unsigned long)s_owner.active_request_id,
                 s_owner.waiting_result ? 1U : 0U,
                 modem_at_status_text(result->status),
                 (unsigned)result->status,
                 (unsigned long)result->rx_byte_count,
                 (unsigned long)diagnostics.rx_byte_count,
                 (unsigned long)diagnostics.last_rx_ticks);
        return;
    }
    const uint32_t elapsed_ms =
        s_owner.active_started_ticks == 0U
            ? 0U
            : pdTICKS_TO_MS(now_ticks - s_owner.active_started_ticks);
    const uint32_t owner_rx_delta =
        diagnostics.rx_byte_count >= s_owner.active_rx_baseline
            ? diagnostics.rx_byte_count - s_owner.active_rx_baseline
            : 0U;
    if (result->status == AT_CORE_RESULT_OK)
    {
        ESP_LOGI(TAG,
                 "AT owner 终态，来源=%s，请求=%lu，命令=%s，状态=%s(%u)，耗时=%lu ms，事务 RX=%lu，owner 期间 RX=%lu，响应=%u，全局 RX=%lu，最后 RX tick=%lu，失败计数=%lu，恢复阶段=%u，生命周期=%u，网络=%u",
                 s_owner.active_origin,
                 (unsigned long)result->request_id,
                 s_owner.active_command,
                 modem_at_status_text(result->status),
                 (unsigned)result->status,
                 (unsigned long)elapsed_ms,
                 (unsigned long)result->rx_byte_count,
                 (unsigned long)owner_rx_delta,
                 (unsigned)result->response_length,
                 (unsigned long)diagnostics.rx_byte_count,
                 (unsigned long)diagnostics.last_rx_ticks,
                 (unsigned long)s_owner.failure_count,
                 (unsigned)s_owner.recovery_stage,
                 (unsigned)s_owner.snapshot.lifecycle,
                 (unsigned)s_owner.snapshot.network_status);
    }
    else
    {
        ESP_LOGW(TAG,
                 "AT owner 终态，来源=%s，请求=%lu，命令=%s，状态=%s(%u)，耗时=%lu ms，事务 RX=%lu，owner 期间 RX=%lu，响应=%u，全局 RX=%lu，最后 RX tick=%lu，失败计数=%lu，恢复阶段=%u，生命周期=%u，网络=%u，核心错误=%s",
                 s_owner.active_origin,
                 (unsigned long)result->request_id,
                 s_owner.active_command,
                 modem_at_status_text(result->status),
                 (unsigned)result->status,
                 (unsigned long)elapsed_ms,
                 (unsigned long)result->rx_byte_count,
                 (unsigned long)owner_rx_delta,
                 (unsigned)result->response_length,
                 (unsigned long)diagnostics.rx_byte_count,
                 (unsigned long)diagnostics.last_rx_ticks,
                 (unsigned long)s_owner.failure_count,
                 (unsigned)s_owner.recovery_stage,
                 (unsigned)s_owner.snapshot.lifecycle,
                 (unsigned)s_owner.snapshot.network_status,
                 diagnostics.last_error);
    }
    const modem_selftest_phase_t active_selftest_phase =
        s_owner.active_selftest_phase;
    s_owner.waiting_result = false;
    s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    if (s_owner.selftest_pending)
    {
        process_selftest_at_result(result,
                                   now_ticks,
                                   active_selftest_phase);
        return;
    }
    if (s_owner.active_is_radio_verify)
    {
        s_owner.radio_backoff_pending = false;
        s_owner.pending_backoff_requires_no_demand = false;
        handle_radio_mode_verification(result, now_ticks);
        return;
    }
    if (s_owner.active_is_backoff_cfun4)
    {
        s_owner.radio_backoff_pending = false;
        s_owner.pending_backoff_requires_no_demand = false;
        s_owner.active_is_backoff_cfun4 = false;
        if (begin_radio_mode_reconciliation(MODEM_RADIO_MODE_RF_OFF,
                                            now_ticks) == ESP_OK)
        {
            return;
        }
        s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
        s_owner.radio_sleep_disabled = true;
        finalize_radio_backoff(MODEM_RADIO_MODE_UNKNOWN, now_ticks);
        return;
    }
    if (s_owner.active_is_radio_wake_cfun1)
    {
        s_owner.radio_backoff_pending = false;
        s_owner.pending_backoff_requires_no_demand = false;
        s_owner.active_is_radio_wake_cfun1 = false;
        if (begin_radio_mode_reconciliation(MODEM_RADIO_MODE_FULL,
                                            now_ticks) != ESP_OK)
        {
            s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
            s_owner.radio_sleep_disabled = true;
            begin_radio_backoff("MODEM_CFUN_VERIFY_FAILED", now_ticks);
        }
        return;
    }
    if (s_owner.radio_backoff_pending)
    {
        char pending_error[MODEM_ERROR_CAPACITY] = {0};
        (void)copy_bounded(pending_error,
                           sizeof(pending_error),
                           s_owner.pending_backoff_error);
        const bool no_demand_required =
            s_owner.pending_backoff_requires_no_demand;
        s_owner.radio_backoff_pending = false;
        s_owner.pending_backoff_requires_no_demand = false;
        if (no_demand_required)
        {
            (void)begin_no_demand_radio_backoff(
                pending_error[0] == '\0'
                    ? "MODEM_CONNECT_CANCELLED"
                    : pending_error,
                now_ticks);
        }
        else
        {
            begin_radio_backoff(pending_error[0] == '\0'
                                    ? "MODEM_CONNECT_FAILED"
                                    : pending_error,
                                now_ticks);
        }
        return;
    }
#if MODEM_RUNTIME_CONFIG_DEBUG
    if (s_owner.active_is_runtime_config)
    {
        process_runtime_config_result(result, now_ticks);
        return;
    }
#endif
    if (s_owner.active_is_recovery)
    {
        s_owner.active_is_recovery = false;
        if (result->status != AT_CORE_RESULT_OK)
        {
            s_owner.recovery_stage = 0U;
            enter_recovery("MODEM_RECOVERY_FAILED", now_ticks);
            return;
        }
        if (s_owner.recovery_stage < 3U)
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_RECOVERING;
            s_owner.next_action_ticks = now_ticks + pdMS_TO_TICKS(100U);
            touch_snapshot(&s_owner.snapshot, now_ticks);
            return;
        }
        s_owner.failure_count = 0U;
        s_owner.recovery_stage = 0U;
        s_owner.matready_seen = false;
        s_owner.init_step = 0U;
        s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BOOTING;
        s_owner.snapshot.network_status = MODEM_NETWORK_BOOTING;
        s_owner.next_action_ticks = now_ticks + pdMS_TO_TICKS(MODEM_MATREADY_WAIT_MS);
        touch_snapshot(&s_owner.snapshot, now_ticks);
        return;
    }
    if (result->status != AT_CORE_RESULT_OK)
    {
        if (s_owner.init_step <
                sizeof(s_init_commands) / sizeof(s_init_commands[0]) &&
            !modem_service_init_command_is_connectivity_gate(
                s_init_commands[s_owner.init_step].command) &&
            (result->status == AT_CORE_RESULT_ERROR ||
             result->status == AT_CORE_RESULT_CME_ERROR ||
             result->status == AT_CORE_RESULT_TIMEOUT ||
             result->status == AT_CORE_RESULT_RESPONSE_OVERFLOW))
        {
            ESP_LOGW(TAG,
                     "自动联网可选 AT 未通过，继续读取联网硬事实，命令=%s，状态=%u，RX=%lu",
                     s_init_commands[s_owner.init_step].command,
                     (unsigned)result->status,
                     (unsigned long)result->rx_byte_count);
            ++s_owner.init_step;
            s_owner.next_action_ticks = now_ticks + 1U;
            touch_snapshot(&s_owner.snapshot, now_ticks);
            return;
        }
        if (!s_owner.active_is_apn &&
            s_owner.init_step <
                sizeof(s_init_commands) / sizeof(s_init_commands[0]))
        {
            ESP_LOGW(TAG,
                     "自动联网关键 AT 失败并进入恢复，命令=%s，状态=%u，RX=%lu",
                     s_init_commands[s_owner.init_step].command,
                     (unsigned)result->status,
                     (unsigned long)result->rx_byte_count);
        }
        else if (s_owner.active_is_apn)
        {
            ESP_LOGW(TAG,
                     "自动联网 APN 配置 AT 失败并进入恢复，状态=%u，RX=%lu",
                     (unsigned)result->status,
                     (unsigned long)result->rx_byte_count);
        }
        enter_recovery(result->status == AT_CORE_RESULT_MODEM_RESTARTED
                           ? "MODEM_RESTARTED"
                           : "MODEM_AT_FAILED",
                       now_ticks);
        return;
    }
    if (s_owner.active_is_health_cpin)
    {
        s_owner.active_is_health_cpin = false;
        parse_cpin(&s_owner.snapshot, result->response);
        if (s_owner.snapshot.sim_status != MODEM_SIM_READY)
        {
            begin_radio_backoff(
                s_owner.snapshot.sim_status == MODEM_SIM_MISSING
                    ? "MODEM_SIM_MISSING"
                    : "MODEM_SIM_NOT_READY",
                now_ticks);
        }
        else
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_HTTPS_READY;
            s_owner.next_sim_check_ticks = now_ticks +
                                           pdMS_TO_TICKS(MODEM_SIM_HEALTH_CHECK_MS);
            touch_snapshot(&s_owner.snapshot, now_ticks);
        }
        return;
    }
    if (s_owner.active_is_apn)
    {
        s_owner.active_is_apn = false;
        s_owner.apn_applied = true;
        touch_snapshot(&s_owner.snapshot, now_ticks);
        return;
    }
    if (s_owner.active_is_pdp_dial)
    {
        s_owner.active_is_pdp_dial = false;
        if (s_owner.snapshot.pdp_status == MODEM_PDP_UP)
        {
            s_owner.pdp_waiting_urc = false;
            s_owner.failure_count = 0U;
            s_owner.next_action_ticks = now_ticks + 1U;
            touch_snapshot(&s_owner.snapshot, now_ticks);
            return;
        }
        s_owner.pdp_waiting_urc = true;
        s_owner.pdp_urc_deadline_ticks = now_ticks +
                                         pdMS_TO_TICKS(MODEM_PDP_URC_WAIT_MS);
        s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_REGISTERING;
        set_error(&s_owner.snapshot, "MODEM_PDP_WAITING");
        touch_snapshot(&s_owner.snapshot, now_ticks);
        return;
    }
    if (s_owner.init_step < sizeof(s_init_commands) / sizeof(s_init_commands[0]))
    {
        const char *command = s_init_commands[s_owner.init_step].command;
        if (strcmp(command, "AT+CGSN") == 0)
        {
            parse_identity(&s_owner.snapshot, result->response, false);
        }
        else if (strcmp(command, "AT+CGSN=1") == 0)
        {
            parse_identity(&s_owner.snapshot, result->response, true);
        }
        else if (strcmp(command, "AT+CPIN?") == 0)
        {
            parse_cpin(&s_owner.snapshot, result->response);
        }
        else if (strcmp(command, "AT+CSQ") == 0)
        {
            parse_csq(&s_owner.snapshot, result->response);
        }
        else if (strcmp(command, "AT+CESQ") == 0)
        {
            parse_cesq(&s_owner.snapshot, result->response);
        }
        else if (strcmp(command, "AT+CEREG?") == 0 ||
                 strcmp(command, "AT+CREG?") == 0 ||
                 strcmp(command, "AT+CGREG?") == 0)
        {
            apply_registration_response(
                &s_owner.snapshot,
                result->response,
                strcmp(command, "AT+CEREG?") == 0
                    ? MODEM_REGISTRATION_SOURCE_CEREG
                    : (strcmp(command, "AT+CREG?") == 0
                           ? MODEM_REGISTRATION_SOURCE_CREG
                           : MODEM_REGISTRATION_SOURCE_CGREG));
        }
        else if (strcmp(command, "AT+COPS?") == 0)
        {
            parse_operator(&s_owner.snapshot, result->response);
        }
        else if (strcmp(command, "AT+MIPCALL?") == 0)
        {
            parse_mipcall(&s_owner.snapshot, result->response);
            if (s_owner.snapshot.pdp_status == MODEM_PDP_UP)
            {
                s_owner.failure_count = 0U;
                s_owner.next_action_ticks = now_ticks + 1U;
            }
        }
        ++s_owner.init_step;
    }
    touch_snapshot(&s_owner.snapshot, now_ticks);
}

static void process_urc(const legbot_at_urc_t *urc, TickType_t now_ticks)
{
    const bool backoff_cfun4_active =
        s_owner.active_is_backoff_cfun4;
    const bool selftest_cfun_transition_active =
        s_owner.selftest_pending && s_owner.waiting_result &&
        s_owner.active_selftest_phase == MODEM_SELFTEST_PHASE_CFUN_WAKE;
    const bool backoff_cfun_transition_active =
        backoff_cfun4_active || s_owner.active_is_radio_wake_cfun1 ||
        s_owner.active_is_radio_verify || selftest_cfun_transition_active;
    const bool idle_backoff =
        s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_BACKOFF &&
        !backoff_cfun_transition_active;
    const modem_network_status_t backoff_network_status =
        s_owner.snapshot.network_status;
#if MODEM_RUNTIME_CONFIG_DEBUG
    const bool previous_pdp_up =
        s_owner.snapshot.pdp_status == MODEM_PDP_UP;
    char previous_ip_address[MODEM_IP_CAPACITY] = {0};
    if (previous_pdp_up && strncmp(urc->line, "+MIPCALL:", 9U) == 0)
    {
        (void)copy_bounded(previous_ip_address,
                           sizeof(previous_ip_address),
                           s_owner.snapshot.ip_address);
    }
#endif
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    if (strcmp(urc->line, "+MATREADY") == 0)
    {
        ESP_LOGW(TAG,
                 "modem owner 收到 MATREADY，URC 序列=%lu，等待 AT=%u，来源=%s，请求=%lu，命令=%s，自检=%u，失败计数=%lu，恢复阶段=%u，全局 RX=%lu，最后 RX tick=%lu",
                 (unsigned long)urc->sequence,
                 s_owner.waiting_result ? 1U : 0U,
                 s_owner.active_origin[0] == '\0'
                     ? "无"
                     : s_owner.active_origin,
                 (unsigned long)s_owner.active_request_id,
                 s_owner.active_command[0] == '\0'
                     ? "无"
                     : s_owner.active_command,
                 s_owner.selftest_pending ? 1U : 0U,
                 (unsigned long)s_owner.failure_count,
                 (unsigned)s_owner.recovery_stage,
                 (unsigned long)diagnostics.rx_byte_count,
                 (unsigned long)diagnostics.last_rx_ticks);
    }
    else
    {
        ESP_LOGI(TAG,
                 "modem owner 收到 URC，序列=%lu，内容=%s，等待 AT=%u，自检=%u，生命周期=%u，网络=%u，全局 RX=%lu",
                 (unsigned long)urc->sequence,
                 urc->line,
                 s_owner.waiting_result ? 1U : 0U,
                 s_owner.selftest_pending ? 1U : 0U,
                 (unsigned)s_owner.snapshot.lifecycle,
                 (unsigned)s_owner.snapshot.network_status,
                 (unsigned long)diagnostics.rx_byte_count);
    }
    if (strcmp(urc->line, "+MATREADY") == 0)
    {
#if MODEM_RUNTIME_CONFIG_DEBUG
        const bool runtime_config_terminal_pending =
            s_owner.active_is_runtime_config && s_owner.waiting_result;
        if (runtime_config_terminal_pending)
        {
            s_owner.runtime_config_restart_pending = true;
            s_owner.runtime_config_restart_requires_recovery = false;
        }
        else
        {
            invalidate_runtime_config("收到 MATREADY");
        }
#endif
        ml307r_https_transport_cancel(true);
        s_owner.matready_seen = true;
        s_owner.init_step = 0U;
        s_owner.apn_applied = false;
#if MODEM_RUNTIME_CONFIG_DEBUG
        if (!runtime_config_terminal_pending &&
            !backoff_cfun_transition_active)
        {
            s_owner.waiting_result = false;
        }
#else
        if (!backoff_cfun_transition_active)
        {
            s_owner.waiting_result = false;
        }
#endif
        s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
        if (!backoff_cfun_transition_active)
        {
            at_core_abort_all(true);
        }
        if (s_owner.selftest_pending)
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BOOTING;
            s_owner.snapshot.network_status = MODEM_NETWORK_BOOTING;
            (void)modem_service_apply_urc(&s_owner.snapshot,
                                          urc->line,
                                          now_ticks);
            reset_selftest_after_restart(now_ticks);
            touch_snapshot(&s_owner.snapshot, now_ticks);
            ESP_LOGI(TAG,
                     "4G 自检观察到 MATREADY，请求=%lu，等待 %u ms 后重新执行固定 115200 基础 AT",
                     (unsigned long)s_owner.selftest_request_id,
                     MODEM_MATREADY_WAIT_MS);
            return;
        }
    }
    if (modem_service_apply_urc(&s_owner.snapshot, urc->line, now_ticks) != ESP_OK)
    {
        return;
    }
    if (s_owner.selftest_pending)
    {
        if (strncmp(urc->line, "+MIPCALL:", 9U) == 0 &&
            s_owner.snapshot.pdp_status == MODEM_PDP_UP &&
            modem_selftest_phase_accepts_pdp_urc(
                s_owner.selftest_phase))
        {
            s_owner.pdp_waiting_urc = false;
            s_owner.selftest_phase = MODEM_SELFTEST_PHASE_DONE;
            s_owner.next_action_ticks = now_ticks + 1U;
        }
        return;
    }
    if (backoff_cfun_transition_active)
    {
        /* CFUN 过渡期间的网络 URC 只作诊断，最终状态由在途命令终态决定。 */
        if (backoff_cfun4_active)
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_RECOVERING;
        }
        s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
        s_owner.snapshot.ip_address[0] = '\0';
        touch_snapshot(&s_owner.snapshot, now_ticks);
        return;
    }
    if (idle_backoff)
    {
        if (strcmp(urc->line, "+MATREADY") == 0)
        {
            begin_radio_backoff("MODEM_RESTARTED", now_ticks);
        }
        else
        {
            /* 延迟网络 URC 不得把已关闭射频的退避态重新投影为在线。 */
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BACKOFF;
            s_owner.snapshot.network_status = backoff_network_status;
            s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
            s_owner.snapshot.ip_address[0] = '\0';
            touch_snapshot(&s_owner.snapshot, now_ticks);
        }
        return;
    }
    if (strncmp(urc->line, "+CPIN:", 6U) == 0)
    {
        if (s_owner.snapshot.sim_status != MODEM_SIM_READY)
        {
            ml307r_https_transport_cancel(false);
            const char *error =
                s_owner.snapshot.sim_status == MODEM_SIM_MISSING
                    ? "MODEM_SIM_MISSING"
                    : "MODEM_SIM_NOT_READY";
            if (s_owner.waiting_result)
            {
                s_owner.radio_backoff_pending = true;
                s_owner.pending_backoff_requires_no_demand = false;
                (void)copy_bounded(s_owner.pending_backoff_error,
                                   sizeof(s_owner.pending_backoff_error),
                                   error);
                set_error(&s_owner.snapshot, error);
                touch_snapshot(&s_owner.snapshot, now_ticks);
            }
            else
            {
                begin_radio_backoff(error, now_ticks);
            }
        }
        else
        {
            s_owner.next_sim_check_ticks = now_ticks +
                                           pdMS_TO_TICKS(MODEM_SIM_HEALTH_CHECK_MS);
        }
        return;
    }
    if (strcmp(urc->line, "+MATREADY") == 0)
    {
        s_owner.next_action_ticks = now_ticks;
        return;
    }
    const bool registration_lost =
        (strncmp(urc->line, "+CEREG:", 7U) == 0 ||
         strncmp(urc->line, "+CREG:", 6U) == 0 ||
         strncmp(urc->line, "+CGREG:", 7U) == 0) &&
        s_owner.snapshot.registration != MODEM_REGISTRATION_HOME &&
        s_owner.snapshot.registration != MODEM_REGISTRATION_ROAMING;
    const bool pdp_lost = strncmp(urc->line, "+MIPCALL:", 9U) == 0 &&
                          s_owner.snapshot.pdp_status != MODEM_PDP_UP;
#if MODEM_RUNTIME_CONFIG_DEBUG
    const bool pdp_replaced =
        strncmp(urc->line, "+MIPCALL:", 9U) == 0 &&
        previous_pdp_up &&
        s_owner.snapshot.pdp_status == MODEM_PDP_UP &&
        strcmp(previous_ip_address, s_owner.snapshot.ip_address) != 0;
    if (registration_lost || pdp_lost)
    {
        if (s_owner.active_is_runtime_config && s_owner.waiting_result)
        {
            s_owner.runtime_config_restart_pending = true;
            s_owner.runtime_config_restart_requires_recovery = true;
            at_core_abort_all(false);
        }
        else
        {
            invalidate_runtime_config(registration_lost
                                          ? "网络注册丢失"
                                          : "PDP 丢失");
        }
    }
    if (pdp_replaced)
    {
        if (s_owner.active_is_runtime_config && s_owner.waiting_result)
        {
            s_owner.runtime_config_restart_pending = true;
            ESP_LOGW(TAG,
                     "ML307R 运行配置检测到 PDP 有效地址变化，当前命令完成后重采，轮次=%lu，原地址=%s，新地址=%s",
                     (unsigned long)s_owner.runtime_config_round,
                     previous_ip_address,
                     s_owner.snapshot.ip_address);
        }
        else
        {
            invalidate_runtime_config("PDP 有效地址发生变化");
        }
    }
#endif
    if ((registration_lost || pdp_lost) && ml307r_https_transport_active())
    {
        ml307r_https_transport_cancel(false);
    }
    if (s_owner.snapshot.pdp_status == MODEM_PDP_UP)
    {
        s_owner.pdp_waiting_urc = false;
        s_owner.failure_count = 0U;
        s_owner.next_action_ticks = now_ticks + 1U;
    }
    else if ((registration_lost || pdp_lost) &&
             network_demand_active() &&
             !s_owner.waiting_result &&
             !s_owner.selftest_pending &&
             !s_owner.connect_attempt_active &&
             s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_RECOVERING &&
             s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_BACKOFF)
    {
        s_owner.pdp_waiting_urc = false;
        begin_connect_attempt(MODEM_CONNECT_ORIGIN_AUTOMATIC,
                              now_ticks);
    }
}

static const char *connect_origin_name(modem_connect_origin_t origin)
{
    switch (origin)
    {
    case MODEM_CONNECT_ORIGIN_AUTOMATIC:
        return "自动";
    case MODEM_CONNECT_ORIGIN_MANUAL:
        return "维护页";
    case MODEM_CONNECT_ORIGIN_PAYMENT:
        return "支付";
    case MODEM_CONNECT_ORIGIN_TELEMETRY:
        return "遥测";
    default:
        return "未知";
    }
}

static void begin_connect_attempt(modem_connect_origin_t origin,
                                  TickType_t now_ticks)
{
    s_owner.startup_radio_sync_complete = true;
    modem_recovery_policy_cancel(&s_owner.recovery_cooldown);
    if (s_owner.connect_session_id < UINT32_MAX)
    {
        ++s_owner.connect_session_id;
    }
    else
    {
        s_owner.connect_session_id = 1U;
    }
    s_owner.connect_origin = origin;
    s_owner.connect_attempt_active = true;
    s_owner.connect_attempt_deadline_ticks =
        now_ticks + pdMS_TO_TICKS(MODEM_CONNECT_ATTEMPT_MS);
    s_owner.init_step = 0U;
    s_owner.apn_applied = false;
    s_owner.pdp_waiting_urc = false;
    s_owner.next_sim_check_ticks = 0U;
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BOOTING;
    s_owner.snapshot.network_status = MODEM_NETWORK_BOOTING;
    s_owner.snapshot.signal_status = MODEM_SIGNAL_UNKNOWN;
    s_owner.snapshot.registration = MODEM_REGISTRATION_UNKNOWN;
    s_owner.snapshot.registration_source = MODEM_REGISTRATION_SOURCE_NONE;
    s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
    s_owner.snapshot.ip_address[0] = '\0';
    set_error(&s_owner.snapshot, "MODEM_CONNECTING");
    touch_snapshot(&s_owner.snapshot, now_ticks);
    ESP_LOGI(TAG,
             "4G 联网会话开始，会话=%lu，来源=%s，窗口=%u ms，退避下一档=%u，射频模式=%u，休眠禁用=%u",
             (unsigned long)s_owner.connect_session_id,
             connect_origin_name(origin),
             MODEM_CONNECT_ATTEMPT_MS,
             (unsigned)(s_owner.recovery_cooldown.next_delay_index == 0U
                            ? 0U
                            : s_owner.recovery_cooldown.next_delay_index - 1U),
             (unsigned)s_owner.radio_mode,
             s_owner.radio_sleep_disabled ? 1U : 0U);
    s_owner.radio_wake_retry_count = 0U;
    if (s_owner.radio_mode == MODEM_RADIO_MODE_FULL)
    {
        s_owner.next_action_ticks = now_ticks;
        return;
    }
    if (s_owner.radio_mode == MODEM_RADIO_MODE_UNKNOWN)
    {
        if (begin_radio_mode_reconciliation(MODEM_RADIO_MODE_FULL,
                                            now_ticks) == ESP_OK)
        {
            return;
        }
        s_owner.radio_sleep_disabled = true;
        begin_radio_backoff("MODEM_CFUN_QUERY_FAILED", now_ticks);
        return;
    }
    s_owner.radio_mode = MODEM_RADIO_MODE_TRANSITION_TO_FULL;
    s_owner.active_is_radio_wake_cfun1 = true;
    if (submit_command("AT+CFUN=1", "", now_ticks, false) == ESP_OK)
    {
        return;
    }
    s_owner.active_is_radio_wake_cfun1 = false;
    s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
    s_owner.radio_sleep_disabled = true;
    begin_radio_backoff("MODEM_CFUN_WAKE_FAILED", now_ticks);
}

static void defer_or_begin_connectivity_retry(const char *error,
                                              TickType_t now_ticks)
{
    if (!s_owner.connect_attempt_active)
    {
        s_owner.connect_attempt_active = true;
        s_owner.connect_attempt_deadline_ticks =
            now_ticks + pdMS_TO_TICKS(MODEM_CONNECT_ATTEMPT_MS);
    }
    if (tick_reached(now_ticks, s_owner.connect_attempt_deadline_ticks))
    {
        begin_radio_backoff(error, now_ticks);
        return;
    }
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_REGISTERING;
    if (strcmp(error, "MODEM_NO_SIGNAL") == 0 ||
        strcmp(error, "MODEM_SIGNAL_UNKNOWN") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_NO_SIGNAL;
    }
    else if (strcmp(error, "MODEM_PDP_FAILED") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_PDP_FAILED;
    }
    else
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_NOT_REGISTERED;
    }
    set_error(&s_owner.snapshot, error);
    s_owner.init_step = MODEM_NETWORK_PROBE_FIRST_STEP;
    s_owner.next_action_ticks =
        now_ticks + pdMS_TO_TICKS(MODEM_NETWORK_PROBE_INTERVAL_MS);
    touch_snapshot(&s_owner.snapshot, now_ticks);
    ESP_LOGI(TAG,
             "4G 联网会话继续搜索，会话=%lu，原因=%s，剩余=%lu ms，下次探测=%u ms",
             (unsigned long)s_owner.connect_session_id,
             error,
             (unsigned long)pdTICKS_TO_MS(
                 s_owner.connect_attempt_deadline_ticks - now_ticks),
             MODEM_NETWORK_PROBE_INTERVAL_MS);
}

static void begin_radio_backoff(const char *error, TickType_t now_ticks)
{
    begin_radio_backoff_committed(error, now_ticks, false);
}

static bool begin_no_demand_radio_backoff(const char *error,
                                          TickType_t now_ticks)
{
    if (s_owner.radio_sleep_disabled)
    {
        return false;
    }
    bool committed = false;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    if (!network_demand_active_locked() &&
        !s_owner.active_is_backoff_cfun4 &&
        !s_owner.active_is_radio_wake_cfun1)
    {
        s_owner.active_is_backoff_cfun4 = true;
        s_owner.active_cfun4_requires_no_demand = true;
        modem_recovery_policy_cancel(&s_owner.recovery_cooldown);
        committed = true;
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (committed)
    {
        begin_radio_backoff_committed(error, now_ticks, true);
    }
    return committed;
}

static void begin_radio_backoff_committed(const char *error,
                                          TickType_t now_ticks,
                                          bool no_demand_commit)
{
#if MODEM_RUNTIME_CONFIG_DEBUG
    invalidate_runtime_config(error);
#endif
    if (s_owner.radio_sleep_disabled)
    {
        s_owner.connect_attempt_active = false;
        s_owner.connect_attempt_deadline_ticks = 0U;
        s_owner.radio_backoff_pending = false;
        s_owner.pending_backoff_requires_no_demand = false;
        s_owner.active_is_backoff_cfun4 = false;
        s_owner.active_is_radio_wake_cfun1 = false;
        s_owner.active_cfun4_requires_no_demand = false;
        if (no_demand_commit &&
            s_owner.snapshot.pdp_status == MODEM_PDP_UP)
        {
            s_owner.next_action_ticks = portMAX_DELAY;
            ESP_LOGW(TAG,
                     "4G 射频休眠已禁用，无业务时保持 CFUN=1 与当前 PDP");
            return;
        }
        s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_RECOVERING;
        s_owner.snapshot.network_status = MODEM_NETWORK_RECOVERING;
        set_error(&s_owner.snapshot, error);
        modem_recovery_policy_schedule(&s_owner.recovery_cooldown,
                                       now_ticks);
        s_owner.next_action_ticks =
            s_owner.recovery_cooldown.retry_at_ticks;
        touch_snapshot(&s_owner.snapshot, now_ticks);
        ESP_LOGW(TAG,
                 "4G 射频休眠已禁用，保持全功能恢复路径，原因=%s，等待=%lu ms",
                 error,
                 (unsigned long)s_owner.recovery_cooldown.scheduled_delay_ms);
        return;
    }
    s_owner.connect_attempt_active = false;
    s_owner.connect_attempt_deadline_ticks = 0U;
    s_owner.radio_backoff_pending = false;
    s_owner.pending_backoff_requires_no_demand = false;
    s_owner.waiting_result = false;
    s_owner.active_is_apn = false;
    s_owner.active_is_pdp_dial = false;
    s_owner.active_is_recovery = false;
    s_owner.active_is_health_cpin = false;
    s_owner.active_is_backoff_cfun4 = no_demand_commit;
    s_owner.active_is_radio_wake_cfun1 = false;
    s_owner.active_cfun4_requires_no_demand = no_demand_commit;
    s_owner.pdp_waiting_urc = false;
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_RECOVERING;
    s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
    s_owner.snapshot.ip_address[0] = '\0';
    set_error(&s_owner.snapshot, error);
    (void)copy_bounded(s_owner.pending_backoff_error,
                       sizeof(s_owner.pending_backoff_error),
                       error);
    touch_snapshot(&s_owner.snapshot, now_ticks);
    if (!no_demand_commit)
    {
        s_owner.active_is_backoff_cfun4 = true;
    }
    s_owner.radio_mode = MODEM_RADIO_MODE_TRANSITION_TO_RF_OFF;
    if (submit_command("AT+CFUN=4", "", now_ticks, false) == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "4G 联网会话失败，准备关闭射频，会话=%lu，原因=%s",
                 (unsigned long)s_owner.connect_session_id,
                 error);
        return;
    }
    s_owner.active_is_backoff_cfun4 = false;
    s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
    s_owner.radio_sleep_disabled = true;
    finalize_radio_backoff(MODEM_RADIO_MODE_UNKNOWN, now_ticks);
}

static void finalize_radio_backoff(modem_radio_mode_t confirmed_mode,
                                   TickType_t now_ticks)
{
    const bool rf_off_confirmed =
        confirmed_mode == MODEM_RADIO_MODE_RF_OFF;
    bool demand_active = false;
    bool released = false;
    bool resume_after_no_demand_cfun4 = false;
    uint8_t scheduled_backoff_step = 0U;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const bool no_demand_commit =
        s_owner.active_cfun4_requires_no_demand;
    s_owner.active_is_backoff_cfun4 = false;
    s_owner.active_is_radio_wake_cfun1 = false;
    s_owner.active_cfun4_requires_no_demand = false;
    s_owner.radio_mode = confirmed_mode;
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BACKOFF;
    if (s_owner.snapshot.network_status == MODEM_NETWORK_BOOTING ||
        s_owner.snapshot.network_status == MODEM_NETWORK_ONLINE)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_RECOVERING;
    }
    s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
    s_owner.snapshot.ip_address[0] = '\0';
    demand_active = network_demand_active_locked();
    resume_after_no_demand_cfun4 = demand_active && no_demand_commit;
    if (!demand_active)
    {
        if (rf_off_confirmed)
        {
            modem_recovery_policy_reset(&s_owner.recovery_cooldown);
            s_owner.next_action_ticks = portMAX_DELAY;
        }
        else
        {
            modem_recovery_policy_schedule(&s_owner.recovery_cooldown,
                                           now_ticks);
            s_owner.next_action_ticks =
                s_owner.recovery_cooldown.retry_at_ticks;
        }
        if (rf_off_confirmed &&
            radio_power_arbiter_owner() == RADIO_POWER_OWNER_LTE)
        {
            released =
                radio_power_arbiter_release(RADIO_POWER_OWNER_LTE) == ESP_OK;
        }
    }
    else if (resume_after_no_demand_cfun4)
    {
        modem_recovery_policy_cancel(&s_owner.recovery_cooldown);
        s_owner.next_action_ticks = now_ticks;
    }
    else
    {
        /* 调度会推进 next_delay_index，先保留本次实际使用的 1 基档位。 */
        scheduled_backoff_step =
            s_owner.recovery_cooldown.next_delay_index <
                    MODEM_RECOVERY_BACKOFF_STEP_COUNT
                ? s_owner.recovery_cooldown.next_delay_index + 1U
                : MODEM_RECOVERY_BACKOFF_STEP_COUNT;
        modem_recovery_policy_schedule(&s_owner.recovery_cooldown,
                                       now_ticks);
        s_owner.next_action_ticks =
            s_owner.recovery_cooldown.retry_at_ticks;
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    touch_snapshot(&s_owner.snapshot, now_ticks);
    if (!demand_active)
    {
        if (rf_off_confirmed)
        {
            ESP_LOGI(TAG,
                     "4G 已在无 RF lease 状态进入 CFUN=4，射频关闭确认=1，互斥释放=%u",
                     released ? 1U : 0U);
        }
        else
        {
            ESP_LOGW(TAG,
                     "4G 的 CFUN=4 尚未确认，保持 LTE 互斥并转入全功能可靠性恢复");
        }
        return;
    }
    if (resume_after_no_demand_cfun4)
    {
        ESP_LOGI(TAG,
                 "无需求 CFUN=4 提交后收到新 RF lease，跳过网络退避并立即恢复射频");
        return;
    }
    ESP_LOGW(TAG,
             "4G 已进入低功耗退避，原因=%s，射频关闭确认=%u，档位=%u，等待=%lu ms，下次 tick=%lu",
             s_owner.pending_backoff_error[0] == '\0'
                 ? "MODEM_CONNECT_FAILED"
                 : s_owner.pending_backoff_error,
             rf_off_confirmed ? 1U : 0U,
             (unsigned)scheduled_backoff_step,
             (unsigned long)s_owner.recovery_cooldown.scheduled_delay_ms,
             (unsigned long)s_owner.recovery_cooldown.retry_at_ticks);
}

static void process_explicit_connect_request(TickType_t now_ticks)
{
    (void)now_ticks;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    s_owner.pending_connect_trigger_mask |=
        atomic_exchange(&s_connect_trigger_mask, 0U);
    const uint32_t lease_mask = atomic_load(&s_network_lease_mask);
    if ((lease_mask & MODEM_NETWORK_LEASE_PAYMENT_BIT) == 0U)
    {
        s_owner.pending_connect_trigger_mask &=
            ~MODEM_CONNECT_TRIGGER_PAYMENT_BIT;
    }
    if ((lease_mask & MODEM_NETWORK_LEASE_TELEMETRY_BIT) == 0U)
    {
        s_owner.pending_connect_trigger_mask &=
            ~MODEM_CONNECT_TRIGGER_TELEMETRY_BIT;
    }
    if ((lease_mask & MODEM_NETWORK_LEASE_MANUAL_BIT) == 0U)
    {
        s_owner.pending_connect_trigger_mask &=
            ~MODEM_CONNECT_TRIGGER_MANUAL_BIT;
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
}

static bool network_demand_active_locked(void)
{
    return s_owner.selftest_pending ||
           atomic_load(&s_claimed_selftest_request_id) != 0U ||
           atomic_load(&s_network_lease_mask) != 0U ||
           s_owner.pending_connect_trigger_mask != 0U ||
           atomic_load(&s_connect_trigger_mask) != 0U;
}

static bool network_demand_active(void)
{
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const bool active = network_demand_active_locked();
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    return active;
}

static uint32_t pending_connect_trigger_snapshot(void)
{
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const uint32_t trigger_mask = s_owner.pending_connect_trigger_mask;
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    return trigger_mask;
}

static uint32_t take_pending_connect_triggers(void)
{
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const uint32_t trigger_mask = s_owner.pending_connect_trigger_mask;
    s_owner.pending_connect_trigger_mask = 0U;
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    return trigger_mask;
}

static bool stop_connect_attempt_if_no_demand(TickType_t now_ticks)
{
    bool abort_active_command = false;
    bool begin_radio_off = false;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    if (s_owner.connect_attempt_active &&
        !network_demand_active_locked())
    {
        s_owner.connect_attempt_active = false;
        s_owner.connect_attempt_deadline_ticks = 0U;
        if (s_owner.waiting_result)
        {
            s_owner.radio_backoff_pending = true;
            s_owner.pending_backoff_requires_no_demand = true;
            abort_active_command = true;
        }
        else
        {
            begin_radio_off = true;
        }
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (abort_active_command)
    {
        (void)copy_bounded(s_owner.pending_backoff_error,
                           sizeof(s_owner.pending_backoff_error),
                           "MODEM_CONNECT_CANCELLED");
        at_core_abort_all(false);
        ESP_LOGI(TAG,
                 "最后一个 RF lease 已释放，中止在途联网 AT 并等待终态后关闭射频");
    }
    else if (begin_radio_off)
    {
        (void)begin_no_demand_radio_backoff("MODEM_CONNECT_CANCELLED",
                                            now_ticks);
        ESP_LOGI(TAG,
                 "最后一个 RF lease 已释放，立即结束联网窗口并关闭射频");
    }
    return abort_active_command || begin_radio_off;
}

static void expire_manual_network_lease(TickType_t now_ticks)
{
    const uint32_t lease_mask = atomic_load(&s_network_lease_mask);
    if ((lease_mask & MODEM_NETWORK_LEASE_MANUAL_BIT) == 0U)
    {
        return;
    }
    const TickType_t deadline =
        (TickType_t)atomic_load(&s_manual_lease_deadline_ticks);
    if (!tick_reached(now_ticks, deadline))
    {
        return;
    }
    bool expired = false;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    const TickType_t confirmed_deadline =
        (TickType_t)atomic_load(&s_manual_lease_deadline_ticks);
    if ((atomic_load(&s_network_lease_mask) &
         MODEM_NETWORK_LEASE_MANUAL_BIT) != 0U &&
        tick_reached(now_ticks, confirmed_deadline))
    {
        (void)atomic_fetch_and(&s_network_lease_mask,
                               ~MODEM_NETWORK_LEASE_MANUAL_BIT);
        (void)atomic_fetch_and(&s_connect_trigger_mask,
                               ~MODEM_CONNECT_TRIGGER_MANUAL_BIT);
        s_owner.pending_connect_trigger_mask &=
            ~MODEM_CONNECT_TRIGGER_MANUAL_BIT;
        atomic_store(&s_manual_lease_deadline_ticks, 0U);
        expired = true;
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (expired)
    {
        ESP_LOGI(TAG,
                 "维护页 4G 手动联网窗口已到期，释放 MANUAL RF lease");
    }
}

static void wake_modem_owner(void)
{
    if (s_control_queue == NULL)
    {
        return;
    }
    const uint32_t command = MODEM_COMMAND_WAKE;
    /* 队列已满表示 STOP 或另一条 WAKE 已足以唤醒 owner。 */
    (void)xQueueSend(s_control_queue, &command, 0);
}

static bool owner_can_wait_for_work(void)
{
    const bool idle_radio_state =
        s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_BACKOFF ||
        (s_owner.radio_sleep_disabled &&
         s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_HTTPS_READY &&
         s_owner.snapshot.pdp_status == MODEM_PDP_UP);
    return idle_radio_state &&
           !network_demand_active() &&
           !s_owner.connect_attempt_active &&
           !s_owner.waiting_result &&
           !s_owner.radio_backoff_pending &&
           !s_owner.active_is_backoff_cfun4 &&
           !s_owner.active_is_radio_wake_cfun1 &&
           !ml307r_https_transport_active() &&
           s_owner.pending_https_response_count == 0U &&
           !s_owner.pending_selftest_result_valid &&
           atomic_load(&s_claimed_selftest_request_id) == 0U &&
           (s_https_request_queue == NULL ||
            uxQueueMessagesWaiting(s_https_request_queue) == 0U) &&
           (s_selftest_request_queue == NULL ||
            uxQueueMessagesWaiting(s_selftest_request_queue) == 0U);
}

static void release_lte_radio_if_safe(void)
{
    bool released = false;
    const bool transport_active = ml307r_https_transport_active();
    taskENTER_CRITICAL(&s_radio_demand_lock);
    if (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_BACKOFF &&
        s_owner.radio_mode == MODEM_RADIO_MODE_RF_OFF &&
        !network_demand_active_locked() &&
        !s_owner.waiting_result && !transport_active &&
        radio_power_arbiter_owner() == RADIO_POWER_OWNER_LTE)
    {
        released =
            radio_power_arbiter_release(RADIO_POWER_OWNER_LTE) == ESP_OK;
    }
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (released)
    {
        ESP_LOGI(TAG, "LTE 已确认 CFUN=4 并释放 GPS/LTE 射频互斥");
    }
}

static esp_err_t sync_modem_pm_lock(void)
{
    const bool should_hold =
        network_demand_active() || s_owner.connect_attempt_active ||
        s_owner.waiting_result || s_owner.radio_backoff_pending ||
        s_owner.active_is_backoff_cfun4 ||
        s_owner.active_is_radio_wake_cfun1 ||
        s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_BACKOFF ||
        s_owner.radio_mode != MODEM_RADIO_MODE_RF_OFF ||
        radio_power_arbiter_owner() == RADIO_POWER_OWNER_LTE ||
        ml307r_https_transport_active();
    if (!should_hold)
    {
        release_modem_pm_lock();
        return ESP_OK;
    }
    if (s_modem_pm_lock_held && s_modem_cpu_max_lock_held)
    {
        return ESP_OK;
    }
    if (s_modem_pm_lock == NULL || s_modem_cpu_max_lock == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t error = ESP_OK;
    if (!s_modem_pm_lock_held)
    {
        error = esp_pm_lock_acquire(s_modem_pm_lock);
        if (error == ESP_OK)
        {
            s_modem_pm_lock_held = true;
        }
    }
    if (error == ESP_OK && !s_modem_cpu_max_lock_held)
    {
        error = esp_pm_lock_acquire(s_modem_cpu_max_lock);
        if (error == ESP_OK)
        {
            s_modem_cpu_max_lock_held = true;
        }
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "LTE RF 电源管理锁获取失败，暂停本轮联网推进，轻睡锁=%u，最高频率锁=%u，错误=0x%x",
                 s_modem_pm_lock_held ? 1U : 0U,
                 s_modem_cpu_max_lock_held ? 1U : 0U,
                 (unsigned)error);
    }
    return error;
}

static void release_modem_pm_lock(void)
{
    if (s_modem_pm_lock_held && s_modem_pm_lock != NULL)
    {
        const esp_err_t error = esp_pm_lock_release(s_modem_pm_lock);
        if (error == ESP_OK)
        {
            s_modem_pm_lock_held = false;
        }
        else
        {
            ESP_LOGE(TAG,
                     "LTE RF 轻睡锁释放失败，错误=0x%x",
                     (unsigned)error);
            return;
        }
    }
    if (s_modem_cpu_max_lock_held && s_modem_cpu_max_lock != NULL)
    {
        const esp_err_t error = esp_pm_lock_release(s_modem_cpu_max_lock);
        if (error == ESP_OK)
        {
            s_modem_cpu_max_lock_held = false;
        }
        else
        {
            ESP_LOGE(TAG,
                     "LTE RF 最高频率锁释放失败，错误=0x%x",
                     (unsigned)error);
        }
    }
}

static void enter_recovery(const char *error, TickType_t now_ticks)
{
    const modem_lifecycle_t previous_lifecycle = s_owner.snapshot.lifecycle;
    const modem_network_status_t previous_network =
        s_owner.snapshot.network_status;
    const uint32_t previous_failure_count = s_owner.failure_count;
    const uint8_t previous_recovery_stage = s_owner.recovery_stage;
    if (s_owner.connect_attempt_active)
    {
        if (s_owner.waiting_result)
        {
            s_owner.radio_backoff_pending = true;
            s_owner.pending_backoff_requires_no_demand = false;
            (void)copy_bounded(s_owner.pending_backoff_error,
                               sizeof(s_owner.pending_backoff_error),
                               error);
            at_core_abort_all(false);
        }
        else
        {
            begin_radio_backoff(error, now_ticks);
        }
        return;
    }
#if MODEM_RUNTIME_CONFIG_DEBUG
    invalidate_runtime_config(error);
#endif
    s_owner.waiting_result = false;
    s_owner.radio_backoff_pending = false;
    s_owner.pending_backoff_requires_no_demand = false;
    s_owner.active_is_apn = false;
    s_owner.active_is_pdp_dial = false;
    s_owner.active_is_recovery = false;
    s_owner.active_is_health_cpin = false;
    s_owner.active_is_backoff_cfun4 = false;
    s_owner.active_is_radio_wake_cfun1 = false;
    s_owner.pdp_waiting_urc = false;
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_RECOVERING;
    if (strcmp(error, "MODEM_SIM_MISSING") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_SIM_MISSING;
    }
    else if (strcmp(error, "MODEM_SIM_NOT_READY") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_SIM_NOT_READY;
    }
    else if (strcmp(error, "MODEM_NO_SIGNAL") == 0 ||
             strcmp(error, "MODEM_SIGNAL_UNKNOWN") == 0 ||
             strcmp(error, "MODEM_SIGNAL_INVALID") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_NO_SIGNAL;
    }
    else if (strcmp(error, "MODEM_NOT_REGISTERED") == 0 ||
             strcmp(error, "MODEM_REGISTRATION_DENIED") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_NOT_REGISTERED;
    }
    else if (strcmp(error, "MODEM_PDP_FAILED") == 0 ||
             strcmp(error, "MODEM_PDP_INVALID_IP") == 0)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_PDP_FAILED;
    }
    else
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_RECOVERING;
    }
    s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
    s_owner.snapshot.ip_address[0] = '\0';
    set_error(&s_owner.snapshot, error);
    saturating_increment(&s_owner.snapshot.recovery_count);
    saturating_increment(&s_owner.failure_count);
    modem_recovery_policy_schedule(&s_owner.recovery_cooldown, now_ticks);
    s_owner.next_action_ticks = s_owner.recovery_cooldown.retry_at_ticks;
    touch_snapshot(&s_owner.snapshot, now_ticks);
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    ESP_LOGW(TAG,
             "进入 4G 恢复冷却，原因=%s，触发来源=%s，触发命令=%s，原生命周期=%u，原网络=%u，失败计数=%lu->%lu，恢复阶段=%u，冷却=%lu ms，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu，AT 超时=%lu，重同步=%lu，核心错误=%s",
             error,
             s_owner.active_origin[0] == '\0'
                 ? "无"
                 : s_owner.active_origin,
             s_owner.active_command[0] == '\0'
                 ? "无"
                 : s_owner.active_command,
             (unsigned)previous_lifecycle,
             (unsigned)previous_network,
             (unsigned long)previous_failure_count,
             (unsigned long)s_owner.failure_count,
             (unsigned)previous_recovery_stage,
             (unsigned long)s_owner.recovery_cooldown.scheduled_delay_ms,
             (unsigned long)diagnostics.rx_byte_count,
             (unsigned long)diagnostics.tx_byte_count,
             (unsigned long)diagnostics.last_rx_ticks,
             (unsigned long)diagnostics.timeout_count,
             (unsigned long)diagnostics.resync_count,
             diagnostics.last_error);
}

static void advance_lifecycle(TickType_t now_ticks)
{
    uint32_t pending_connect_triggers =
        pending_connect_trigger_snapshot();
    if (ml307r_https_transport_active() && s_owner.snapshot.pdp_status == MODEM_PDP_UP)
    {
        if (s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_REQUEST_ACTIVE)
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_REQUEST_ACTIVE;
            touch_snapshot(&s_owner.snapshot, now_ticks);
        }
        return;
    }
    if (!s_owner.selftest_pending &&
        stop_connect_attempt_if_no_demand(now_ticks))
    {
        return;
    }
    if (!s_owner.selftest_pending &&
        s_owner.connect_attempt_active &&
        (pending_connect_triggers != 0U ||
         tick_reached(now_ticks,
                      s_owner.connect_attempt_deadline_ticks)))
    {
        watch_power_snapshot_t power = {0};
        if (watch_state_power_snapshot(&power, 0) == ESP_OK &&
            !power.selftest_active)
        {
            pending_connect_triggers = take_pending_connect_triggers();
            if (pending_connect_triggers != 0U)
            {
                const modem_connect_origin_t origin =
                    (pending_connect_triggers &
                     MODEM_CONNECT_TRIGGER_PAYMENT_BIT) != 0U
                        ? MODEM_CONNECT_ORIGIN_PAYMENT
                        : ((pending_connect_triggers &
                            MODEM_CONNECT_TRIGGER_TELEMETRY_BIT) != 0U
                               ? MODEM_CONNECT_ORIGIN_TELEMETRY
                               : MODEM_CONNECT_ORIGIN_MANUAL);
                s_owner.connect_origin = origin;
                ESP_LOGI(TAG,
                         "4G 显式联网请求已合并，会话=%lu，来源=%s，保持原始绝对截止，不延长联网窗口",
                         (unsigned long)s_owner.connect_session_id,
                         connect_origin_name(origin));
            }
            if (tick_reached(now_ticks,
                             s_owner.connect_attempt_deadline_ticks))
            {
                if (s_owner.waiting_result)
                {
                    if (!s_owner.radio_backoff_pending)
                    {
                        s_owner.radio_backoff_pending = true;
                        s_owner.pending_backoff_requires_no_demand = false;
                        (void)copy_bounded(
                            s_owner.pending_backoff_error,
                            sizeof(s_owner.pending_backoff_error),
                            "MODEM_CONNECT_TIMEOUT");
                        at_core_abort_all(false);
                    }
                }
                else
                {
                    begin_radio_backoff("MODEM_CONNECT_TIMEOUT",
                                        now_ticks);
                }
                return;
            }
        }
    }
    if (s_owner.waiting_result)
    {
        return;
    }
    if (s_owner.active_is_backoff_cfun4 ||
        s_owner.active_is_radio_wake_cfun1)
    {
        /* 等待 CFUN 及后续对账终态，避免插入第二条命令。 */
        return;
    }
    if (!s_owner.selftest_pending)
    {
        watch_power_snapshot_t power = {0};
        if (watch_state_power_snapshot(&power, 0) != ESP_OK)
        {
            /* 无法取得整机自检事实时保守暂停本轮后台推进，下一轮重新读取。 */
            return;
        }
        const bool suspended =
            modem_recovery_policy_background_suspended_by_selftest(
                power.selftest_active,
                s_owner.selftest_pending);
        if (suspended)
        {
            if (!s_owner.background_suspended_for_selftest)
            {
                s_owner.background_suspended_for_selftest = true;
                ESP_LOGI(TAG,
                         "整机自检进行中，4G 后台联网与自动恢复已暂停，自检网络检查保持可用");
            }
            return;
        }
        if (s_owner.background_suspended_for_selftest)
        {
            s_owner.background_suspended_for_selftest = false;
            ESP_LOGI(TAG, "整机自检已结束，4G 后台联网与自动恢复已恢复");
        }
    }
    if (s_owner.startup_gate_pending)
    {
        if (!tick_reached(now_ticks, s_owner.startup_gate_ticks))
        {
            return;
        }
        s_owner.startup_gate_pending = false;
        ESP_LOGI(TAG,
                 "4G 后台启动门禁已到期，允许提交首条 AT，当前 tick=%lu",
                 (unsigned long)now_ticks);
    }
    if (!s_owner.selftest_pending &&
        (pending_connect_triggers =
             pending_connect_trigger_snapshot()) != 0U)
    {
        const bool telemetry_only =
            (pending_connect_triggers &
             MODEM_CONNECT_TRIGGER_TELEMETRY_BIT) != 0U &&
            (pending_connect_triggers &
             (MODEM_CONNECT_TRIGGER_PAYMENT_BIT |
              MODEM_CONNECT_TRIGGER_MANUAL_BIT)) == 0U;
        if (telemetry_only && s_owner.recovery_cooldown.pending &&
            !tick_reached(
                now_ticks,
                s_owner.recovery_cooldown.retry_at_ticks))
        {
            /* 普通 telemetry 不越过 modem 退避；lease 超时由 cloud owner 释放。 */
            return;
        }
        pending_connect_triggers = take_pending_connect_triggers();
        if (pending_connect_triggers == 0U)
        {
            return;
        }
        const modem_connect_origin_t origin =
            (pending_connect_triggers &
             MODEM_CONNECT_TRIGGER_PAYMENT_BIT) != 0U
                ? MODEM_CONNECT_ORIGIN_PAYMENT
                : ((pending_connect_triggers &
                    MODEM_CONNECT_TRIGGER_TELEMETRY_BIT) != 0U
                       ? MODEM_CONNECT_ORIGIN_TELEMETRY
                       : MODEM_CONNECT_ORIGIN_MANUAL);
        const bool online =
            s_owner.snapshot.pdp_status == MODEM_PDP_UP &&
            (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_HTTPS_READY ||
             s_owner.snapshot.lifecycle ==
                 MODEM_LIFECYCLE_REQUEST_ACTIVE);
        if (online)
        {
            ESP_LOGI(TAG,
                     "4G 显式联网请求无需执行，来源=%s，当前已联网",
                     connect_origin_name(origin));
        }
        else
        {
            s_owner.failure_count = 0U;
            s_owner.recovery_stage = 0U;
            begin_connect_attempt(origin, now_ticks);
            return;
        }
    }
    if (!s_owner.selftest_pending &&
        !s_owner.startup_radio_sync_complete &&
        !s_owner.connect_attempt_active &&
        !s_owner.waiting_result)
    {
        s_owner.startup_radio_sync_complete = true;
        if (begin_radio_mode_reconciliation(MODEM_RADIO_MODE_UNKNOWN,
                                            now_ticks) != ESP_OK)
        {
            s_owner.radio_mode = MODEM_RADIO_MODE_UNKNOWN;
            s_owner.radio_sleep_disabled = true;
            begin_radio_backoff("MODEM_CFUN_QUERY_FAILED", now_ticks);
        }
        return;
    }
    if (!s_owner.selftest_pending &&
        !network_demand_active() &&
        !s_owner.connect_attempt_active &&
        !s_owner.waiting_result &&
        !ml307r_https_transport_active())
    {
        if (s_owner.radio_sleep_disabled)
        {
            const bool online =
                s_owner.radio_mode == MODEM_RADIO_MODE_FULL &&
                s_owner.snapshot.pdp_status == MODEM_PDP_UP;
            if (online)
            {
                return;
            }
            if (s_owner.recovery_cooldown.pending &&
                !modem_recovery_policy_take_due(
                    &s_owner.recovery_cooldown,
                    now_ticks))
            {
                return;
            }
            begin_connect_attempt(MODEM_CONNECT_ORIGIN_AUTOMATIC,
                                  now_ticks);
            return;
        }
        if (s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_BACKOFF &&
            !s_owner.active_is_backoff_cfun4)
        {
            (void)begin_no_demand_radio_backoff("MODEM_IDLE",
                                                now_ticks);
        }
        else if (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_BACKOFF &&
                 s_owner.radio_mode == MODEM_RADIO_MODE_RF_OFF)
        {
            release_lte_radio_if_safe();
        }
        else if (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_BACKOFF &&
                 tick_reached(now_ticks, s_owner.next_action_ticks))
        {
            (void)begin_no_demand_radio_backoff(
                "MODEM_IDLE_CFUN4_RETRY",
                now_ticks);
        }
        return;
    }
    if (s_owner.recovery_cooldown.pending)
    {
        if (s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_RECOVERING &&
            s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_BACKOFF)
        {
            modem_recovery_policy_cancel(&s_owner.recovery_cooldown);
        }
        else if (!modem_recovery_policy_take_due(
                     &s_owner.recovery_cooldown,
                     now_ticks))
        {
            return;
        }
        if (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_BACKOFF)
        {
            begin_connect_attempt(MODEM_CONNECT_ORIGIN_AUTOMATIC,
                                  now_ticks);
            return;
        }
    }
    if (!tick_reached(now_ticks, s_owner.next_action_ticks))
    {
        return;
    }
    if (s_owner.selftest_pending)
    {
        advance_selftest(now_ticks);
        return;
    }
    if (s_owner.pdp_waiting_urc)
    {
        if (!tick_reached(now_ticks, s_owner.pdp_urc_deadline_ticks))
        {
            return;
        }
        s_owner.pdp_waiting_urc = false;
        defer_or_begin_connectivity_retry("MODEM_PDP_FAILED",
                                          now_ticks);
        return;
    }
    if (s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_RECOVERING)
    {
        if (s_owner.failure_count >= MODEM_SOFT_RECOVERY_THRESHOLD)
        {
            const char *recovery = "AT+MREBOOT=0";
            s_owner.recovery_stage = 2U;
            at_core_diagnostics_t diagnostics = {0};
            (void)at_core_diagnostics_snapshot(&diagnostics);
            ESP_LOGW(TAG,
                     "准备执行 4G 软恢复，命令=%s，失败计数=%lu，恢复阶段=%u，生命周期=%u，网络=%u，全局 RX=%lu，最后 RX tick=%lu，AT 超时=%lu，重同步=%lu",
                     recovery,
                     (unsigned long)s_owner.failure_count,
                     (unsigned)s_owner.recovery_stage,
                     (unsigned)s_owner.snapshot.lifecycle,
                     (unsigned)s_owner.snapshot.network_status,
                     (unsigned long)diagnostics.rx_byte_count,
                     (unsigned long)diagnostics.last_rx_ticks,
                     (unsigned long)diagnostics.timeout_count,
                     (unsigned long)diagnostics.resync_count);
            if (submit_command(recovery, "", now_ticks, false) != ESP_OK)
            {
                enter_recovery("MODEM_RECOVERY_BUSY", now_ticks);
            }
            else
            {
                if (s_owner.recovery_stage < 3U)
                {
                    ++s_owner.recovery_stage;
                }
                s_owner.active_is_recovery = true;
            }
            return;
        }
        s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BOOTING;
        s_owner.snapshot.network_status = MODEM_NETWORK_BOOTING;
        s_owner.init_step = 0U;
        s_owner.apn_applied = false;
    }
    if (s_owner.init_step < sizeof(s_init_commands) / sizeof(s_init_commands[0]))
    {
        const modem_init_command_t *step = &s_init_commands[s_owner.init_step];
        if (strcmp(step->command, "AT+CPIN?") == 0)
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_SIM_CHECK;
            touch_snapshot(&s_owner.snapshot, now_ticks);
        }
        else if (strcmp(step->command, "AT+CEREG?") == 0)
        {
            s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_REGISTERING;
            touch_snapshot(&s_owner.snapshot, now_ticks);
        }
        if (strcmp(step->command, "AT+MIPCALL?") == 0 && !s_owner.apn_applied &&
            submit_configured_apn(now_ticks) == ESP_OK)
        {
            return;
        }
        if (submit_command(step->command, step->expected_prefix, now_ticks, false) != ESP_OK)
        {
            enter_recovery("MODEM_AT_QUEUE_FULL", now_ticks);
        }
        return;
    }
    if (s_owner.snapshot.sim_status != MODEM_SIM_READY)
    {
        begin_radio_backoff(
            s_owner.snapshot.sim_status == MODEM_SIM_MISSING
                ? "MODEM_SIM_MISSING"
                : "MODEM_SIM_NOT_READY",
            now_ticks);
        return;
    }
    if (s_owner.snapshot.signal_status != MODEM_SIGNAL_PRESENT)
    {
        defer_or_begin_connectivity_retry(
            s_owner.snapshot.signal_status == MODEM_SIGNAL_NONE
                ? "MODEM_NO_SIGNAL"
                : "MODEM_SIGNAL_UNKNOWN",
            now_ticks);
        return;
    }
    if (s_owner.snapshot.registration != MODEM_REGISTRATION_HOME &&
        s_owner.snapshot.registration != MODEM_REGISTRATION_ROAMING)
    {
        if (s_owner.snapshot.registration ==
            MODEM_REGISTRATION_DENIED)
        {
            begin_radio_backoff("MODEM_REGISTRATION_DENIED",
                                now_ticks);
        }
        else
        {
            defer_or_begin_connectivity_retry(
                "MODEM_NOT_REGISTERED",
                now_ticks);
        }
        return;
    }
    if (s_owner.snapshot.pdp_status != MODEM_PDP_UP)
    {
        if (submit_command("AT+MIPCALL=1,1", "+MIPCALL:", now_ticks, false) != ESP_OK)
        {
            defer_or_begin_connectivity_retry("MODEM_PDP_FAILED",
                                              now_ticks);
        }
        else
        {
            s_owner.active_is_pdp_dial = true;
            if (s_owner.selftest_pending)
            {
                s_owner.selftest_pdp_attempted = true;
            }
        }
        return;
    }
    if (s_owner.next_sim_check_ticks == 0U)
    {
        s_owner.next_sim_check_ticks = now_ticks +
                                       pdMS_TO_TICKS(MODEM_SIM_HEALTH_CHECK_MS);
    }
    else if (tick_reached(now_ticks, s_owner.next_sim_check_ticks))
    {
        if (submit_command("AT+CPIN?", "+CPIN:", now_ticks, false) != ESP_OK)
        {
            enter_recovery("MODEM_AT_QUEUE_FULL", now_ticks);
        }
        else
        {
            s_owner.active_is_health_cpin = true;
        }
        return;
    }
#if MODEM_RUNTIME_CONFIG_DEBUG
    if (advance_runtime_config(now_ticks))
    {
        return;
    }
#endif
    const bool became_https_ready =
        s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_HTTPS_READY;
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_HTTPS_READY;
    if (s_owner.snapshot.network_status != MODEM_NETWORK_REQUEST_FAILED &&
        s_owner.snapshot.network_status != MODEM_NETWORK_REPORTING_OK)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_ONLINE;
        set_error(&s_owner.snapshot, "MODEM_OK");
    }
    s_owner.failure_count = 0U;
    s_owner.recovery_stage = 0U;
    s_owner.connect_attempt_active = false;
    s_owner.connect_attempt_deadline_ticks = 0U;
    s_owner.radio_mode = MODEM_RADIO_MODE_FULL;
    modem_recovery_policy_reset(&s_owner.recovery_cooldown);
    if (became_https_ready)
    {
        ESP_LOGI(TAG,
                 "自动联网已就绪，SIM=%u，信号=%u，注册=%u，PDP=%u",
                 (unsigned)s_owner.snapshot.sim_status,
                 (unsigned)s_owner.snapshot.signal_status,
                 (unsigned)s_owner.snapshot.registration,
                 (unsigned)s_owner.snapshot.pdp_status);
    }
    touch_snapshot(&s_owner.snapshot, now_ticks);
    s_owner.next_action_ticks = now_ticks + pdMS_TO_TICKS(1000U);
}

static void final_publish(TickType_t now_ticks)
{
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_RECOVERING;
    s_owner.snapshot.network_status = MODEM_NETWORK_RECOVERING;
    s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
    s_owner.snapshot.ip_address[0] = '\0';
    set_error(&s_owner.snapshot, "MODEM_STOPPED");
    touch_snapshot(&s_owner.snapshot, now_ticks);
}

static void process_https_cancel_request(TickType_t now_ticks)
{
    const uint32_t request_id = atomic_load(&s_https_cancel_request_id);
    if (request_id == 0U ||
        !ml307r_https_transport_cancel_request(request_id))
    {
        return;
    }
    atomic_store(&s_https_cancel_request_id, 0U);
    s_owner.snapshot.network_status = MODEM_NETWORK_REQUEST_FAILED;
    s_owner.snapshot.last_failure_ticks = now_ticks;
    set_error(&s_owner.snapshot, "MODEM_HTTPS_CANCELLED");
    touch_snapshot(&s_owner.snapshot, now_ticks);
    ESP_LOGI(TAG,
             "HTTPS 活动请求已按身份取消，请求=%lu",
             (unsigned long)request_id);
}

static void process_https_request(TickType_t now_ticks)
{
    if (s_owner.selftest_pending || ml307r_https_transport_active() ||
        s_owner.waiting_result ||
        s_owner.pending_https_response_count > 0U ||
        s_https_request_queue == NULL)
    {
        return;
    }
    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    if (xQueueReceive(s_https_request_queue, &s_https_request_scratch, 0) != pdTRUE)
    {
        return;
    }
    const uint32_t cancel_request_id =
        atomic_load(&s_https_cancel_request_id);
    if (cancel_request_id == s_https_request_scratch.request_id)
    {
        atomic_store(&s_https_cancel_request_id, 0U);
        publish_https_terminal(&s_https_request_scratch,
                               ML307R_HTTPS_ERROR_CANCELLED,
                               "MODEM_HTTPS_CANCELLED",
                               now_ticks);
        secure_clear(&s_https_request_scratch,
                     sizeof(s_https_request_scratch));
        return;
    }
    if (tick_reached(now_ticks, s_https_request_scratch.deadline_ticks))
    {
        publish_https_terminal(&s_https_request_scratch,
                               ML307R_HTTPS_ERROR_ATTEMPT_TIMEOUT,
                               "MODEM_HTTPS_TIMEOUT", now_ticks);
        secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
        return;
    }
    if (s_owner.snapshot.lifecycle != MODEM_LIFECYCLE_HTTPS_READY)
    {
        if (xQueueSend(s_https_request_queue, &s_https_request_scratch, 0) != pdTRUE)
        {
            publish_https_terminal(&s_https_request_scratch,
                                   ML307R_HTTPS_ERROR_QUEUE,
                                   "MODEM_HTTPS_QUEUE", now_ticks);
        }
        secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
        return;
    }
    esp_err_t err = ml307r_https_transport_start(&s_https_request_scratch, now_ticks);
    if (err == ESP_OK)
    {
        s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_REQUEST_ACTIVE;
        s_owner.snapshot.network_status = MODEM_NETWORK_ONLINE;
        set_error(&s_owner.snapshot, "MODEM_OK");
        touch_snapshot(&s_owner.snapshot, now_ticks);
        secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
        return;
    }
    publish_https_terminal(&s_https_request_scratch,
                           err == ESP_ERR_INVALID_STATE
                               ? ML307R_HTTPS_ERROR_CONFIG
                               : ML307R_HTTPS_ERROR_INVALID_REQUEST,
                           err == ESP_ERR_INVALID_STATE
                               ? "MODEM_HTTPS_CONFIG"
                               : "MODEM_HTTPS_INVALID",
                           now_ticks);
    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    s_owner.snapshot.network_status = MODEM_NETWORK_REQUEST_FAILED;
    s_owner.snapshot.last_failure_ticks = now_ticks;
    set_error(&s_owner.snapshot, "MODEM_HTTPS_START");
    touch_snapshot(&s_owner.snapshot, now_ticks);
}

static void process_https_response(TickType_t now_ticks)
{
    secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
    if (ml307r_https_transport_take_response(&s_https_response_scratch) != ESP_OK)
    {
        return;
    }
    if (s_https_response_scratch.error == ML307R_HTTPS_ERROR_NONE)
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_REPORTING_OK;
        s_owner.snapshot.last_success_ticks = now_ticks;
        set_error(&s_owner.snapshot, "MODEM_OK");
    }
    else
    {
        s_owner.snapshot.network_status = MODEM_NETWORK_REQUEST_FAILED;
        s_owner.snapshot.last_failure_ticks = now_ticks;
        set_error(&s_owner.snapshot,
                  strncmp(s_https_response_scratch.error_code, "MODEM_", 6U) == 0
                      ? s_https_response_scratch.error_code
                      : "MODEM_HTTPS_FAILED");
    }
    const bool at_unresponsive =
        s_https_response_scratch.error ==
        ML307R_HTTPS_ERROR_AT_UNRESPONSIVE;
    s_owner.snapshot.lifecycle = s_owner.snapshot.pdp_status == MODEM_PDP_UP
                                     ? MODEM_LIFECYCLE_HTTPS_READY
                                     : MODEM_LIFECYCLE_RECOVERING;
    touch_snapshot(&s_owner.snapshot, now_ticks);
    queue_https_response(&s_https_response_scratch);
    secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
    if (at_unresponsive)
    {
        ESP_LOGW(TAG, "HTTPS 结束后 AT 通道无响应，作废 PDP 并进入自动恢复");
        enter_recovery("MODEM_AT_FAILED", now_ticks);
    }
}

static void publish_https_terminal(const ml307r_https_post_request_t *request,
                                   ml307r_https_error_t error,
                                   const char *code,
                                   TickType_t now_ticks)
{
    secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
    s_https_response_scratch.request_id = request->request_id;
    s_https_response_scratch.error = error;
    (void)copy_bounded(s_https_response_scratch.error_code,
                       sizeof(s_https_response_scratch.error_code),
                       code);
    queue_https_response(&s_https_response_scratch);
    s_owner.snapshot.network_status = MODEM_NETWORK_REQUEST_FAILED;
    s_owner.snapshot.last_failure_ticks = now_ticks;
    touch_snapshot(&s_owner.snapshot, now_ticks);
    secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
}

static void retry_pending_terminals(void)
{
    while (s_owner.pending_https_response_count > 0U &&
           s_https_response_queue != NULL)
    {
        if (xQueueSend(s_https_response_queue,
                       &s_owner.pending_https_responses[0],
                       0) != pdTRUE)
        {
            break;
        }
        secure_clear(&s_owner.pending_https_responses[0],
                     sizeof(s_owner.pending_https_responses[0]));
        --s_owner.pending_https_response_count;
        if (s_owner.pending_https_response_count > 0U)
        {
            memmove(&s_owner.pending_https_responses[0],
                    &s_owner.pending_https_responses[1],
                    s_owner.pending_https_response_count *
                        sizeof(s_owner.pending_https_responses[0]));
            secure_clear(
                &s_owner.pending_https_responses[s_owner.pending_https_response_count],
                sizeof(s_owner.pending_https_responses[0]));
        }
    }
    if (s_owner.pending_selftest_result_valid && s_selftest_result_queue != NULL &&
        xQueueSend(s_selftest_result_queue, &s_owner.pending_selftest_result, 0) == pdTRUE)
    {
        secure_clear(&s_owner.pending_selftest_result,
                     sizeof(s_owner.pending_selftest_result));
        s_owner.pending_selftest_result_valid = false;
    }
}

static void queue_https_response(const ml307r_https_response_t *response)
{
    if (s_https_response_queue != NULL &&
        xQueueSend(s_https_response_queue, response, 0) == pdTRUE)
    {
        return;
    }
    if (s_owner.pending_https_response_count < MODEM_HTTPS_TERMINAL_BACKLOG_DEPTH)
    {
        s_owner.pending_https_responses[s_owner.pending_https_response_count++] = *response;
    }
    else
    {
        /* accepted 数量受 active+request queue 上限约束，触发表示内部合同破坏。 */
        ESP_LOGE(TAG, "HTTPS 终态 outbox 已满，请求=%lu",
                 (unsigned long)response->request_id);
    }
    set_error(&s_owner.snapshot, "MODEM_RESULT_QUEUE_FULL");
}

static void drain_https_requests(TickType_t now_ticks)
{
    if (s_https_request_queue == NULL)
    {
        return;
    }
    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    while (xQueueReceive(s_https_request_queue, &s_https_request_scratch, 0) == pdTRUE)
    {
        publish_https_terminal(&s_https_request_scratch,
                               ML307R_HTTPS_ERROR_CANCELLED,
                               "MODEM_HTTPS_CANCELLED", now_ticks);
        secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    }
    (void)xQueueReset(s_https_request_queue);
    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    for (uint32_t index = 0U; index < MODEM_HTTPS_REQUEST_QUEUE_DEPTH; ++index)
    {
        (void)xQueueSend(s_https_request_queue, &s_https_request_scratch, 0);
    }
    (void)xQueueReset(s_https_request_queue);
    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
}

static void reset_typed_queues(void)
{
    (void)xQueueReset(s_control_queue);
    (void)xQueueReset(s_selftest_request_queue);

    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    while (xQueueReceive(s_https_request_queue, &s_https_request_scratch, 0) == pdTRUE)
    {
        secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    }
    for (uint32_t index = 0U; index < MODEM_HTTPS_REQUEST_QUEUE_DEPTH; ++index)
    {
        (void)xQueueSend(s_https_request_queue, &s_https_request_scratch, 0);
    }
    (void)xQueueReset(s_https_request_queue);

    secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
    while (xQueueReceive(s_https_response_queue, &s_https_response_scratch, 0) == pdTRUE)
    {
        secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
    }
    for (uint32_t index = 0U; index < MODEM_HTTPS_RESPONSE_QUEUE_DEPTH; ++index)
    {
        (void)xQueueSend(s_https_response_queue, &s_https_response_scratch, 0);
    }
    (void)xQueueReset(s_https_response_queue);

    secure_clear(&s_selftest_scratch, sizeof(s_selftest_scratch));
    while (xQueueReceive(s_selftest_result_queue, &s_selftest_scratch, 0) == pdTRUE)
    {
        secure_clear(&s_selftest_scratch, sizeof(s_selftest_scratch));
    }
    for (uint32_t index = 0U; index < MODEM_SELFTEST_QUEUE_DEPTH; ++index)
    {
        (void)xQueueSend(s_selftest_result_queue, &s_selftest_scratch, 0);
    }
    (void)xQueueReset(s_selftest_result_queue);
    secure_clear(&s_https_request_scratch, sizeof(s_https_request_scratch));
    secure_clear(&s_https_response_scratch, sizeof(s_https_response_scratch));
    secure_clear(&s_selftest_scratch, sizeof(s_selftest_scratch));
}

static void publish_snapshot(void)
{
    if (s_owner.snapshot.sequence == 0U ||
        s_owner.snapshot.sequence == s_owner.last_published_sequence)
    {
        return;
    }
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    watch_modem_update_t update = {
        .modem_update_sequence = s_owner.snapshot.sequence,
        .lifecycle = (watch_modem_lifecycle_t)s_owner.snapshot.lifecycle,
        .sim_status = (watch_modem_sim_status_t)s_owner.snapshot.sim_status,
        .signal_status = (watch_modem_signal_status_t)s_owner.snapshot.signal_status,
        .registration = (watch_modem_registration_status_t)s_owner.snapshot.registration,
        .pdp_status = (watch_modem_pdp_status_t)s_owner.snapshot.pdp_status,
        .request_status = s_owner.snapshot.lifecycle == MODEM_LIFECYCLE_REQUEST_ACTIVE
                              ? WATCH_MODEM_REQUEST_ACTIVE
                              : (s_owner.snapshot.network_status == MODEM_NETWORK_REQUEST_FAILED
                                     ? WATCH_MODEM_REQUEST_FAILED
                                     : (s_owner.snapshot.network_status == MODEM_NETWORK_REPORTING_OK
                                            ? WATCH_MODEM_REQUEST_OK
                                            : WATCH_MODEM_REQUEST_IDLE)),
        .csq_rssi = s_owner.snapshot.csq_rssi,
        .csq_ber = s_owner.snapshot.csq_ber,
        .cesq_rsrq = s_owner.snapshot.cesq_rsrq,
        .cesq_rsrp = s_owner.snapshot.cesq_rsrp,
        .updated_at_ticks = s_owner.snapshot.updated_at_ticks,
        .last_success_ticks = s_owner.snapshot.last_success_ticks,
        .last_failure_ticks = s_owner.snapshot.last_failure_ticks,
        .recovery_count = s_owner.snapshot.recovery_count,
        .line_overflow_count = diagnostics.line_overflow_count,
        .queue_overflow_count = diagnostics.command_queue_full_count +
                                diagnostics.result_queue_full_count,
        .urc_overflow_count = diagnostics.urc_overflow_count,
    };
    memcpy(update.imei, s_owner.snapshot.imei, sizeof(update.imei));
    memcpy(update.serial_number, s_owner.snapshot.serial_number,
           sizeof(update.serial_number));
    memcpy(update.operator_name, s_owner.snapshot.operator_name,
           sizeof(update.operator_name));
    memcpy(update.ip_address, s_owner.snapshot.ip_address,
           sizeof(update.ip_address));
    memcpy(update.last_error, s_owner.snapshot.last_error,
           sizeof(update.last_error));
    if (state_service_publish_modem(&update, 0) == ESP_OK)
    {
        s_owner.last_published_sequence = s_owner.snapshot.sequence;
    }
    else
    {
        ESP_LOGW(TAG, "modem typed 最新真值暂未被 state_task 接受");
    }
}

static void process_selftest_request(TickType_t now_ticks)
{
    if (s_owner.selftest_pending || s_owner.waiting_result ||
        s_owner.active_is_backoff_cfun4 ||
        s_owner.active_is_radio_wake_cfun1 ||
        s_owner.pending_selftest_result_valid || s_selftest_request_queue == NULL)
    {
        return;
    }
    modem_selftest_request_t request = {0};
    if (xQueueReceive(s_selftest_request_queue, &request, 0) != pdTRUE)
    {
        return;
    }
    if (atomic_load(&s_selftest_cancel_request_id) == request.request_id)
    {
        unsigned int expected_cancel_id = request.request_id;
        (void)atomic_compare_exchange_strong(
            &s_selftest_cancel_request_id,
            &expected_cancel_id,
            0U);
        unsigned int expected_request_id = request.request_id;
        (void)atomic_compare_exchange_strong(
            &s_claimed_selftest_request_id,
            &expected_request_id,
            0U);
        ESP_LOGI(TAG,
                 "4G 自检请求在 owner 接受前已取消，请求=%lu",
                 (unsigned long)request.request_id);
        return;
    }
    if (ml307r_https_transport_active())
    {
        modem_selftest_result_t busy = {
            .request_id = request.request_id,
            .outcome = MODEM_SELFTEST_SKIP,
        };
        (void)copy_bounded(busy.reason, sizeof(busy.reason), "MODEM_HTTPS_BUSY");
        if (xQueueSend(s_selftest_result_queue, &busy, 0) != pdTRUE)
        {
            s_owner.pending_selftest_result = busy;
            s_owner.pending_selftest_result_valid = true;
            set_error(&s_owner.snapshot, "MODEM_SELFTEST_RESULT_FULL");
        }
        secure_clear(&busy, sizeof(busy));
        unsigned int expected_request_id = request.request_id;
        (void)atomic_compare_exchange_strong(
            &s_claimed_selftest_request_id,
            &expected_request_id,
            0U);
        return;
    }
    const modem_lifecycle_t previous_lifecycle = s_owner.snapshot.lifecycle;
    const modem_network_status_t previous_network =
        s_owner.snapshot.network_status;
    const uint32_t previous_failure_count = s_owner.failure_count;
    const uint8_t previous_recovery_stage = s_owner.recovery_stage;
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    s_owner.selftest_pending = true;
    s_owner.selftest_request_id = request.request_id;
    s_owner.selftest_started_ticks = request.enqueued_ticks;
    s_owner.selftest_deadline_ticks =
        request.enqueued_ticks +
        pdMS_TO_TICKS(MODEM_SELFTEST_OWNER_TIMEOUT_MS);
    s_owner.selftest_phase =
        s_owner.radio_mode == MODEM_RADIO_MODE_RF_OFF
            ? MODEM_SELFTEST_PHASE_CFUN_WAKE
            : (s_owner.radio_mode == MODEM_RADIO_MODE_FULL
                   ? MODEM_SELFTEST_PHASE_BASIC_AT
                   : MODEM_SELFTEST_PHASE_CFUN_VERIFY);
    s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    s_owner.selftest_pdp_attempted = false;
    s_owner.selftest_at_confirmed = false;
    s_owner.selftest_at_attempts = 0U;
    s_owner.selftest_at_rx_total = 0U;
    s_owner.radio_wake_retry_count = 0U;
    s_owner.selftest_terminal_reason[0] = '\0';
    s_owner.matready_seen = false;
    s_owner.active_is_apn = false;
    s_owner.active_is_pdp_dial = false;
    s_owner.active_is_recovery = false;
    s_owner.active_is_health_cpin = false;
    s_owner.pdp_waiting_urc = false;
    s_owner.background_suspended_for_selftest = false;
    modem_recovery_policy_cancel(&s_owner.recovery_cooldown);
    s_owner.snapshot.sim_status = MODEM_SIM_UNKNOWN;
    s_owner.snapshot.signal_status = MODEM_SIGNAL_UNKNOWN;
    s_owner.snapshot.registration = MODEM_REGISTRATION_UNKNOWN;
    s_owner.snapshot.registration_source = MODEM_REGISTRATION_SOURCE_NONE;
    s_owner.snapshot.pdp_status = MODEM_PDP_DOWN;
    s_owner.snapshot.ip_address[0] = '\0';
    s_owner.snapshot.lifecycle = MODEM_LIFECYCLE_BOOTING;
    s_owner.snapshot.network_status = MODEM_NETWORK_BOOTING;
    s_owner.next_action_ticks =
        s_owner.radio_mode == MODEM_RADIO_MODE_RF_OFF
            ? now_ticks
            : now_ticks + pdMS_TO_TICKS(MODEM_MATREADY_WAIT_MS);
    touch_snapshot(&s_owner.snapshot, now_ticks);
    ESP_LOGI(TAG,
             "已接受 4G 自检请求，请求=%lu，运行时波特率=%lu，owner 预算=%u ms（先等 %u ms MATREADY），接受前生命周期=%u，网络=%u，失败计数=%lu，恢复阶段=%u，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu，AT 超时=%lu，重同步=%lu，核心错误=%s",
             (unsigned long)request.request_id,
             (unsigned long)at_core_baud_rate(),
             MODEM_SELFTEST_OWNER_TIMEOUT_MS,
             MODEM_MATREADY_WAIT_MS,
             (unsigned)previous_lifecycle,
             (unsigned)previous_network,
             (unsigned long)previous_failure_count,
             (unsigned)previous_recovery_stage,
             (unsigned long)diagnostics.rx_byte_count,
             (unsigned long)diagnostics.tx_byte_count,
             (unsigned long)diagnostics.last_rx_ticks,
             (unsigned long)diagnostics.timeout_count,
             (unsigned long)diagnostics.resync_count,
             diagnostics.last_error);
}

static void process_selftest_completion(TickType_t now_ticks)
{
    if (!s_owner.selftest_pending || s_selftest_result_queue == NULL)
    {
        return;
    }
    const bool deadline = tick_reached(now_ticks, s_owner.selftest_deadline_ticks);
    const bool cancelled =
        atomic_load(&s_selftest_cancel_request_id) ==
        s_owner.selftest_request_id;
    if (cancelled && s_owner.waiting_result)
    {
        at_core_abort_all(false);
        s_owner.waiting_result = false;
        s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    }
    if (cancelled && s_owner.selftest_terminal_reason[0] == '\0')
    {
        set_selftest_terminal_reason("MODEM_SELFTEST_CANCELLED");
    }
    if (deadline && s_owner.waiting_result)
    {
        at_core_abort_all(false);
        s_owner.waiting_result = false;
        s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    }
    if (deadline && !s_owner.selftest_at_confirmed &&
        s_owner.selftest_terminal_reason[0] == '\0')
    {
        set_selftest_terminal_reason(
            s_owner.selftest_at_rx_total == 0U &&
                    !s_owner.matready_seen
                ? "MODEM_AT_NO_RESPONSE"
                : "MODEM_AT_RESPONSE_TIMEOUT");
    }
    const bool sequence_done =
        s_owner.selftest_phase == MODEM_SELFTEST_PHASE_DONE;
    const bool terminal_failure =
        s_owner.selftest_terminal_reason[0] != '\0';
    if (!deadline && !terminal_failure && !sequence_done)
    {
        return;
    }
    modem_selftest_result_t result = {
        .request_id = s_owner.selftest_request_id,
        .outcome = MODEM_SELFTEST_FAIL,
    };
    const char *reason = s_owner.selftest_terminal_reason[0] != '\0'
                             ? s_owner.selftest_terminal_reason
                             : modem_selftest_classify_snapshot(
                                   &s_owner.snapshot,
                                   deadline,
                                   s_owner.selftest_pdp_attempted,
                                   &result.outcome);
    (void)copy_bounded(result.reason, sizeof(result.reason), reason);
    at_core_diagnostics_t diagnostics = {0};
    (void)at_core_diagnostics_snapshot(&diagnostics);
    ESP_LOGI(TAG,
             "4G 自检终态，请求=%lu，结果=%u，原因=%s，运行时波特率=%lu，SIM=%u，信号=%u，注册=%u，PDP=%u，耗时=%lu ms，截止=%u，取消=%u，失败计数=%lu，恢复阶段=%u，全局 RX=%lu，驱动 TX=%lu，最后 RX tick=%lu，AT 事务=%lu，AT 超时=%lu，重同步=%lu，读取错误=%lu，核心错误=%s",
             (unsigned long)result.request_id,
             (unsigned)result.outcome,
             result.reason,
             (unsigned long)at_core_baud_rate(),
             (unsigned)s_owner.snapshot.sim_status,
             (unsigned)s_owner.snapshot.signal_status,
             (unsigned)s_owner.snapshot.registration,
             (unsigned)s_owner.snapshot.pdp_status,
             (unsigned long)pdTICKS_TO_MS(now_ticks -
                                          s_owner.selftest_started_ticks),
             deadline ? 1U : 0U,
             cancelled ? 1U : 0U,
             (unsigned long)s_owner.failure_count,
             (unsigned)s_owner.recovery_stage,
             (unsigned long)diagnostics.rx_byte_count,
             (unsigned long)diagnostics.tx_byte_count,
             (unsigned long)diagnostics.last_rx_ticks,
             (unsigned long)diagnostics.transaction_count,
             (unsigned long)diagnostics.timeout_count,
             (unsigned long)diagnostics.resync_count,
             (unsigned long)diagnostics.rx_error_count,
             diagnostics.last_error);
    const bool result_queued =
        xQueueSend(s_selftest_result_queue, &result, 0) == pdTRUE;
    if (!result_queued)
    {
        s_owner.pending_selftest_result = result;
        s_owner.pending_selftest_result_valid = true;
        set_error(&s_owner.snapshot, "MODEM_SELFTEST_RESULT_FULL");
    }
    s_owner.selftest_pending = false;
    s_owner.selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    s_owner.active_selftest_phase = MODEM_SELFTEST_PHASE_IDLE;
    s_owner.selftest_request_id = 0U;
    s_owner.selftest_started_ticks = 0U;
    s_owner.selftest_deadline_ticks = 0U;
#if MODEM_RUNTIME_CONFIG_DEBUG
    const bool selftest_pdp_reactivated =
        s_owner.selftest_pdp_attempted;
#endif
    s_owner.selftest_pdp_attempted = false;
    s_owner.selftest_at_confirmed = false;
    s_owner.selftest_at_attempts = 0U;
    s_owner.selftest_at_rx_total = 0U;
    s_owner.baud_probe_index = 0U;
    s_owner.baud_probe_attempts = 0U;
    s_owner.selftest_terminal_reason[0] = '\0';
    s_owner.matready_seen = false;
    const bool network_retained =
        modem_selftest_apply_terminal_state(&s_owner.snapshot,
                                            result.outcome);
    if (network_retained)
    {
        s_owner.radio_mode = MODEM_RADIO_MODE_FULL;
        s_owner.connect_attempt_active = false;
        s_owner.connect_attempt_deadline_ticks = 0U;
        modem_recovery_policy_reset(&s_owner.recovery_cooldown);
    }
#if MODEM_RUNTIME_CONFIG_DEBUG
    if (!network_retained || selftest_pdp_reactivated)
    {
        invalidate_runtime_config(
            network_retained
                ? "4G 自检重新激活 PDP"
                : "4G 自检未保留联网状态");
    }
#endif
    s_owner.init_step =
        network_retained
            ? sizeof(s_init_commands) / sizeof(s_init_commands[0])
            : 0U;
    s_owner.active_is_apn = false;
    s_owner.active_is_pdp_dial = false;
    s_owner.active_is_recovery = false;
    s_owner.active_is_health_cpin = false;
#if MODEM_RUNTIME_CONFIG_DEBUG
    s_owner.active_is_runtime_config = false;
#endif
    s_owner.apn_applied = false;
    s_owner.pdp_waiting_urc = false;
    s_owner.next_sim_check_ticks =
        network_retained
            ? now_ticks + pdMS_TO_TICKS(MODEM_SIM_HEALTH_CHECK_MS)
            : 0U;
    if (network_retained && result_queued)
    {
        set_error(&s_owner.snapshot, "MODEM_OK");
    }
    ESP_LOGI(TAG,
             "4G 自检收尾，网络保留=%u，结果已入队=%u，下一生命周期=%u，下一网络=%u，init=%u，下一 SIM 检查=%lu，失败计数=%lu，恢复阶段=%u",
             network_retained ? 1U : 0U,
             result_queued ? 1U : 0U,
             (unsigned)s_owner.snapshot.lifecycle,
             (unsigned)s_owner.snapshot.network_status,
             (unsigned)s_owner.init_step,
             (unsigned long)s_owner.next_sim_check_ticks,
             (unsigned long)s_owner.failure_count,
             (unsigned)s_owner.recovery_stage);
    s_owner.next_action_ticks = now_ticks + 1U;
    touch_snapshot(&s_owner.snapshot, now_ticks);
    unsigned int expected_cancel_id = result.request_id;
    bool demand_after_selftest = false;
    taskENTER_CRITICAL(&s_radio_demand_lock);
    (void)atomic_compare_exchange_strong(
        &s_selftest_cancel_request_id,
        &expected_cancel_id,
        0U);
    unsigned int expected_request_id = result.request_id;
    (void)atomic_compare_exchange_strong(
        &s_claimed_selftest_request_id,
        &expected_request_id,
        0U);
    demand_after_selftest = network_demand_active_locked();
    taskEXIT_CRITICAL(&s_radio_demand_lock);
    if (!network_retained)
    {
        if (demand_after_selftest)
        {
            modem_recovery_policy_cancel(&s_owner.recovery_cooldown);
            s_owner.next_action_ticks = now_ticks;
            ESP_LOGI(TAG,
                     "4G 自检未保留网络但已有 RF lease，跳过 CFUN=4 并立即恢复业务联网");
        }
        else
        {
            (void)begin_no_demand_radio_backoff(result.reason,
                                                now_ticks);
        }
    }
}

static void set_selftest_terminal_reason(const char *reason)
{
    if (reason == NULL)
    {
        return;
    }
    (void)copy_bounded(s_owner.selftest_terminal_reason,
                       sizeof(s_owner.selftest_terminal_reason),
                       reason);
}
