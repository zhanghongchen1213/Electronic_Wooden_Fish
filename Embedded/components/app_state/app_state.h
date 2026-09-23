/**
 * @file     app_state.h
 * @brief    手环运行状态接口
 * @details  定义全局运行状态字段、授权与普通控制载荷及状态快照，供服务间共享可观测状态。
 * @author   ZHC
 * @date     2026-08-17
 */

#ifndef LEGBOT_APP_STATE_H
#define LEGBOT_APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "exoskeleton_scene_config.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 状态快照中稳定诊断码的固定容量，包含字符串终止符。 */
#define WATCH_STATE_ERROR_CODE_CAPACITY 40U
/** 解锁会话证明只允许存在于当前进程 RAM，不得进入 NVS。 */
#define WATCH_UNLOCK_SESSION_RAM_ONLY 1U
/** 授权与普通控制共享唯一 pending 槽。 */
#define WATCH_CONTROL_SINGLE_PENDING 1U
/** 单个逻辑普通控制允许的最大 attempt 数。 */
#define WATCH_CONTROL_MAX_ATTEMPTS 3U
/** 设备 watch_id 容量，包含字符串终止符。 */
#define WATCH_STATE_WATCH_ID_CAPACITY 18U
/** BLE 绑定规范 MAC 容量，包含字符串终止符。 */
#define WATCH_BLE_MAC_CAPACITY 18U
/** 外骨骼状态超过该本地单调毫秒数即视为不同步。 */
#define STATUS_STALE_MS 5000U
/** 外骨骼版本字符串容量，包含结尾 NUL。 */
#define WATCH_EXOSKELETON_VERSION_CAPACITY 17U
/** 外骨骼协议统一允许的最低档位。 */
#define WATCH_EXOSKELETON_GEAR_MIN 1U
/** 支持极限模式型号允许的最高档位。 */
#define WATCH_EXOSKELETON_GEAR_MAX 15U
/** 不支持极限模式型号允许的最高档位。 */
#define WATCH_EXOSKELETON_NON_EXTREME_GEAR_MAX 10U
/** 归一化场景业务值的下界。 */
#define WATCH_EXOSKELETON_SCENE_MODE_MIN 1U
/** 归一化场景业务值的上界。 */
#define WATCH_EXOSKELETON_SCENE_MODE_MAX 5U
/** 小碎步归一化场景业务值；仅在 BLE 边界转换为线值 2。 */
#define WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP 5U
/** 自检结果 detail code 的固定容量，包含字符串终止符。 */
#define SELFTEST_DETAIL_CODE_CAPACITY 48U
/** modem IMEI/SN 有界容量，包含字符串终止符。 */
#define WATCH_MODEM_IDENTITY_CAPACITY 24U
/** modem 运营商有界容量，包含字符串终止符。 */
#define WATCH_MODEM_OPERATOR_CAPACITY 32U
/** modem IPv4/IPv6 有界容量，包含字符串终止符。 */
#define WATCH_MODEM_IP_CAPACITY 48U
/** GPS 坐标定位真值的新鲜窗口，达到该边界即失效。 */
#define WATCH_GPS_FIX_FRESHNESS_MS 10000U
/** JSON double 可安全精确表达的最大服务器 UTC 毫秒。 */
#define WATCH_CLOUD_SERVER_TIME_MAX_MS 9007199254740991LL
/** 外骨骼电量不高于该百分比时进入低电提醒。 */
#define WATCH_EXOSKELETON_LOW_BATTERY_ENTER_PERCENT 20U
/** 外骨骼低电后只有高于该百分比才退出提醒。 */
#define WATCH_EXOSKELETON_LOW_BATTERY_EXIT_PERCENT 23U

    typedef enum
    {
        SELFTEST_ITEM_BSP_RESOURCE_TABLE = 0, /**< BSP 资源表引脚、方向、所有权与保留脚一致性项。 */
        SELFTEST_ITEM_PWR_BOOT_INPUT,         /**< PWR_INT/BOOT0 只读输入边界与原始电平项。 */
        SELFTEST_ITEM_RESET_OBSERVATION,      /**< EN/RESET_N 复位原因的软件观察项。 */
        SELFTEST_ITEM_INIT_STAGES,            /**< 板级初始化阶段的顺序与返回值项。 */
        SELFTEST_ITEM_DISABLED_RESOURCES,     /**< 未纳管/禁用资源与禁用原因项。 */
        SELFTEST_ITEM_POWER_MATRIX_PENDING,   /**< 三种供电与 LTC2954 时序的待样机验证项。 */
        SELFTEST_ITEM_PVDF_INPUT,             /**< PVDF 候选输入二次确认策略与误触发项。 */
        SELFTEST_ITEM_COUNT                   /**< 当前启用的完整自检项数量。 */
    } selftest_item_id_t;

    typedef enum
    {
        SELFTEST_RUN_IDLE = 0, /**< 未运行或尚无历史摘要。 */
        SELFTEST_RUN_RUNNING,  /**< 正在串行执行自检项。 */
        SELFTEST_RUN_FINISHED, /**< 已生成完整终态摘要。 */
        SELFTEST_RUN_CANCELLED /**< 收到 typed STOP 后有界取消。 */
    } selftest_run_state_t;

    typedef enum
    {
        SELFTEST_OUTCOME_NOT_RUN = 0, /**< 本次尚未执行到该项。 */
        SELFTEST_OUTCOME_PASS,        /**< 项目通过固定判定。 */
        SELFTEST_OUTCOME_FAIL,        /**< 项目未通过或超时。 */
        SELFTEST_OUTCOME_SKIP,        /**< 后续能力未注册或环境前置不满足。 */
        SELFTEST_OUTCOME_INCONCLUSIVE /**< 观察窗口结束但没有形成硬件失败证据。 */
    } selftest_outcome_t;

    typedef enum
    {
        SELFTEST_REASON_NONE = 0,         /**< 无失败或跳过原因。 */
        SELFTEST_REASON_NOT_REGISTERED,   /**< 后续扩展项尚未注册。 */
        SELFTEST_REASON_PREREQUISITE,     /**< 显式环境前置不满足。 */
        SELFTEST_REASON_TIMEOUT,          /**< 单项超过固定验收时间。 */
        SELFTEST_REASON_CANCELLED,        /**< 运行被 typed STOP 取消。 */
        SELFTEST_REASON_DEVICE_MISSING,   /**< 应存在的硬件未应答。 */
        SELFTEST_REASON_BUS_BUSY,         /**< 共享总线有界等待超时。 */
        SELFTEST_REASON_READ_FAILED,      /**< 底层读取失败。 */
        SELFTEST_REASON_WRITE_FAILED,     /**< 底层写入失败。 */
        SELFTEST_REASON_RESOURCE_MISSING, /**< typed 资源不存在。 */
        SELFTEST_REASON_RESOURCE_CORRUPT, /**< typed 资源损坏或格式错误。 */
        SELFTEST_REASON_BUSY,             /**< 底层服务忙或命令未接受。 */
        SELFTEST_REASON_DRIVER_FAILED,    /**< 底层硬件或驱动失败。 */
        SELFTEST_REASON_USER_REJECTED,    /**< 测试人员明确判定失败。 */
        SELFTEST_REASON_HANDLER_MISSING,  /**< Story 1.5 必须项缺少 handler。 */
        SELFTEST_REASON_INVALID_RESULT,   /**< handler 返回不允许的终态。 */
        SELFTEST_REASON_WINDOW_EXPIRED    /**< 观察窗口结束，需继续搜索或复测。 */
    } selftest_reason_t;

    /**
     * @brief 自检项证据类别
     * @details 设计输入与软件编译成功都不能升级为 hardware_verified；只有附有板级回执时才允许该类别。
     */
    typedef enum
    {
        SELFTEST_EVIDENCE_DESIGN_INPUT = 0,  /**< 结论直接来自硬件事实源，本固件未观测。 */
        SELFTEST_EVIDENCE_SOFTWARE_OBSERVED, /**< 结论来自本次运行的软件观测。 */
        SELFTEST_EVIDENCE_HARDWARE_PENDING,  /**< 需要实板/仪器回执，当前缺失。 */
        SELFTEST_EVIDENCE_HARDWARE_VERIFIED, /**< 已附可追溯的板级回执。 */
        SELFTEST_EVIDENCE_FAILED,            /**< 已发现与硬件事实源冲突的证据。 */
        SELFTEST_EVIDENCE_COUNT              /**< 证据类别数量，不是有效类别。 */
    } selftest_evidence_t;

    typedef struct
    {
        selftest_item_id_t item_id;                      /**< 稳定自检项 ID。 */
        selftest_outcome_t outcome;                      /**< pass/fail/skip 终态。 */
        selftest_reason_t reason;                        /**< typed 原因。 */
        selftest_evidence_t evidence;                    /**< 本次终态的证据类别。 */
        uint32_t elapsed_ms;                             /**< 单调时钟实测耗时。 */
        char detail_code[SELFTEST_DETAIL_CODE_CAPACITY]; /**< 定长稳定 detail code。 */
    } selftest_item_result_t;

    typedef struct
    {
        uint32_t run_id;                                              /**< 启动周期内单调递增的运行 ID。 */
        selftest_run_state_t state;                                   /**< 运行状态。 */
        selftest_item_id_t current_item;                              /**< 当前执行项，非 running 时为 SELFTEST_ITEM_COUNT。 */
        TickType_t started_at_ticks;                                  /**< 本次运行开始 tick。 */
        TickType_t finished_at_ticks;                                 /**< 本次运行结束 tick。 */
        uint8_t pass_count;                                           /**< pass 项数。 */
        uint8_t fail_count;                                           /**< fail 项数。 */
        uint8_t skip_count;                                           /**< skip 项数。 */
        uint8_t inconclusive_count;                                   /**< 未完成、需复测项数。 */
        uint8_t completed_count;                                      /**< 已固化终态的项数。 */
        bool retry_active;                                            /**< true 表示正在重试一个既有失败项。 */
        selftest_item_id_t retry_item;                                /**< 最近重试项；尚无重试时为 SELFTEST_ITEM_COUNT。 */
        TickType_t retry_started_at_ticks;                            /**< 最近单项重试开始 tick。 */
        TickType_t retry_finished_at_ticks;                           /**< 最近单项重试结束 tick，进行中时为 0。 */
        selftest_item_result_t selftest_results[SELFTEST_ITEM_COUNT]; /**< 完整有界结果列表。 */
    } watch_selftest_summary_t;

    typedef struct
    {
        uint32_t run_id;               /**< 新运行 ID。 */
        selftest_item_id_t first_item; /**< 新运行的首项 ID。 */
        TickType_t started_at_ticks;   /**< 新运行开始 tick。 */
    } watch_selftest_begin_update_t;

    typedef struct
    {
        uint32_t run_id;               /**< 结果所属运行 ID。 */
        selftest_item_result_t result; /**< 定长单项终态结果。 */
        selftest_item_id_t next_item;  /**< 下一项 ID，全部完成时为 SELFTEST_ITEM_COUNT。 */
    } watch_selftest_item_update_t;

    typedef struct
    {
        uint32_t run_id;              /**< 结束的运行 ID。 */
        selftest_run_state_t state;   /**< 仅允许 FINISHED 或 CANCELLED。 */
        TickType_t finished_at_ticks; /**< 运行结束 tick。 */
    } watch_selftest_finish_update_t;

    typedef struct
    {
        uint32_t run_id;               /**< 被重试完整结果的运行 ID。 */
        selftest_item_id_t item_id;    /**< 必须是当前结果中的失败项。 */
        TickType_t started_at_ticks;   /**< 单项重试开始 tick。 */
    } watch_selftest_retry_begin_update_t;

    typedef struct
    {
        uint32_t run_id;               /**< 被重试完整结果的运行 ID。 */
        selftest_item_result_t result; /**< 被重试项的最新终态。 */
        TickType_t finished_at_ticks;  /**< 单项重试结束 tick。 */
    } watch_selftest_retry_finish_update_t;

    typedef enum
    {
        WATCH_STATE_FIELD_BOOT_READY = 0, /**< 启动流程是否完成基础初始化。 */
        WATCH_STATE_FIELD_BLE_CONNECTED,  /**< 保留的旧 BLE 字段 ID，调用时拒绝并要求 typed BLE 更新。 */
        WATCH_STATE_FIELD_MODEM_READY,    /**< 蜂窝模组是否可用。 */
        WATCH_STATE_FIELD_GPS_FIX,        /**< GPS 是否已有有效定位。 */
    } watch_state_field_t;

    typedef struct
    {
        watch_state_field_t field; /**< 需要更新的状态字段。 */
        uint32_t value;            /**< 字段新值，布尔字段使用 0/非 0 表示。 */
    } watch_state_update_t;

    /**
     * @brief 统一敲击 gate 的状态 owner 更新
     * @details 完成遮罩与故障锁定只允许由 state_service 通过单调序号发布，
     *          tap_input_service 只能从不可变快照读取，不能传入自造状态。
     */
    typedef struct
    {
        uint32_t update_sequence; /**< owner 单调更新序号，必须从 1 开始递增。 */
        bool completed;            /**< 是否处于完成遮罩。 */
        bool fault_locked;         /**< 是否处于故障锁定。 */
    } watch_tap_gate_update_t;

    typedef enum
    {
        WATCH_POWER_LEVEL_NORMAL = 0,             /**< 电量正常，不限制运行资源。 */
        WATCH_POWER_LEVEL_LOW_BATTERY_WARN,       /**< 普通低电提醒，不限制运行资源。 */
        WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY,   /**< 严重低电，抑制非必要输出并限制常态亮度。 */
        WATCH_POWER_LEVEL_COUNT                   /**< 电量等级数量，不是有效产品状态。 */
    } watch_power_level_t;

    typedef struct
    {
        bool valid;                    /**< 本次电量读取是否有效。 */
        uint8_t percent;               /**< 有效时的 0-100 电量百分比。 */
        watch_power_level_t power_level; /**< power owner 已采用的闭集电量等级。 */
        TickType_t attempted_at_ticks; /**< 本次读取完成的系统 tick。 */
        const char *power_error_code;  /**< POWER_ 前缀稳定状态码。 */
        const char *driver_error_code; /**< DRV_CW2015_ 前缀稳定驱动码。 */
    } watch_battery_update_t;

    typedef enum
    {
        WATCH_SCREEN_STATE_ON = 0, /**< AMOLED 正常显示并允许必要 flush。 */
        WATCH_SCREEN_STATE_OFF,    /**< AMOLED 熄屏但运行时与业务服务继续。 */
        WATCH_SCREEN_STATE_COUNT   /**< 屏幕状态数量，不是有效产品状态。 */
    } watch_screen_state_t;

    typedef enum
    {
        WATCH_POWER_ERROR_NONE = 0,           /**< 当前显示与触摸交互无错误。 */
        WATCH_POWER_ERROR_DISPLAY_FAILED,     /**< 显示开关、亮度或 flush 失败。 */
        WATCH_POWER_ERROR_TOUCH_FAILED,       /**< 触摸读取或恢复失败。 */
        WATCH_POWER_ERROR_PWR_INPUT_UNVERIFIED, /**< PWR 有效电平或去抖仍缺少目标板证据。 */
        WATCH_POWER_ERROR_COUNT               /**< 本机电源错误数量，不是有效错误。 */
    } watch_power_error_t;

    typedef struct
    {
        watch_screen_state_t screen_state; /**< 正交于电量等级的显示开关事实。 */
        uint64_t last_local_activity_ms;   /**< 最近真实本地用户活动单调毫秒。 */
        uint32_t transition_sequence;      /**< power owner 单调显示转换序号。 */
        watch_power_error_t error;         /**< 最近显示、触摸或 PWR 输入诊断。 */
    } watch_power_update_t;

    typedef struct
    {
        watch_power_level_t power_level; /**< 当前手环电量等级。 */
        watch_screen_state_t screen_state; /**< 当前正交显示事实。 */
        uint8_t battery_percent;         /**< 最近一次有效手环电量百分比。 */
        bool battery_has_valid_sample;   /**< 当前启动周期是否取得过有效电量。 */
        bool selftest_active;            /**< 整机自检或单项重试是否 active。 */
        uint32_t screen_transition_sequence; /**< 最近成功采用的显示转换序号。 */
    } watch_power_snapshot_t;

    typedef enum
    {
        WATCH_AUDIO_STATE_IDLE = 0,  /**< audio_task 空闲且尚未准备资源。 */
        WATCH_AUDIO_STATE_READY,     /**< 固定资源已准备，可接受播放。 */
        WATCH_AUDIO_STATE_PLAYING,   /**< 正在播放固定资源。 */
        WATCH_AUDIO_STATE_COMPLETED, /**< 最近一次播放完整结束。 */
        WATCH_AUDIO_STATE_STOPPED,   /**< 最近一次播放被 typed stop 有界中断。 */
        WATCH_AUDIO_STATE_FAILED,    /**< 最近一次资源或硬件操作失败。 */
    } watch_audio_state_t;

    typedef enum
    {
        WATCH_AUDIO_ERROR_NONE = 0,         /**< 当前没有音频错误。 */
        WATCH_AUDIO_ERROR_NOT_READY,        /**< 音频资源或服务尚未准备。 */
        WATCH_AUDIO_ERROR_RESOURCE_MISSING, /**< 固定资源文件不存在。 */
        WATCH_AUDIO_ERROR_CORRUPT,          /**< WAV 结构损坏或截断。 */
        WATCH_AUDIO_ERROR_UNSUPPORTED,      /**< WAV 参数不受支持。 */
        WATCH_AUDIO_ERROR_IO_FAILED,        /**< SPIFFS 或文件 I/O 失败。 */
        WATCH_AUDIO_ERROR_BUSY,             /**< 播放冲突或 typed 队列满。 */
        WATCH_AUDIO_ERROR_CODEC_FAILED,     /**< ES8311 配置或清理失败。 */
        WATCH_AUDIO_ERROR_I2S_FAILED,       /**< I2S0 启停或写入失败。 */
        WATCH_AUDIO_ERROR_PA_FAILED,        /**< NS4150 安全电平控制失败。 */
    } watch_audio_error_t;

    typedef struct
    {
        bool resource_available;     /**< 固定 selftest_ok 是否通过资源探测。 */
        watch_audio_state_t state;   /**< 单播放状态机状态。 */
        watch_audio_error_t error;   /**< 独立于播放状态的稳定错误原因。 */
        TickType_t updated_at_ticks; /**< 本次音频状态生成时刻。 */
    } watch_audio_update_t;

    typedef struct
    {
        bool resource_available;     /**< 固定 selftest_ok 是否可用。 */
        watch_audio_state_t state;   /**< 当前或最近一次音频播放状态。 */
        watch_audio_error_t error;   /**< 当前或最近一次音频错误原因。 */
        TickType_t updated_at_ticks; /**< 最近音频状态生成时刻。 */
        uint32_t update_sequence;    /**< 每次成功 apply 后递增的音频状态序号。 */
    } watch_audio_snapshot_t;

    typedef enum
    {
        WATCH_CONFIG_VALUE_UNCONFIGURED = 0, /**< 配置值缺失。 */
        WATCH_CONFIG_VALUE_CONFIGURED,       /**< 配置值存在且有效。 */
        WATCH_CONFIG_VALUE_INVALID,          /**< 配置值存在但非法。 */
    } watch_config_value_status_t;

    typedef enum
    {
        WATCH_CLOUD_UNCONFIGURED = 0, /**< 云链路配置未完成。 */
        WATCH_CLOUD_CONFIGURED,       /**< 云链路请求前置配置已通过。 */
        WATCH_CLOUD_INVALID,          /**< 至少一个云配置事实非法。 */
    } watch_cloud_config_status_t;

    typedef enum
    {
        WATCH_APN_AUTO = 0,   /**< APN 由运营商自动配置。 */
        WATCH_APN_CONFIGURED, /**< 非空 APN 已配置。 */
        WATCH_APN_INVALID,    /**< 非空 APN 非法。 */
    } watch_apn_mode_t;

    typedef enum
    {
        WATCH_CONFIG_SCHEMA_VALID = 0, /**< schema 当前可解释。 */
        WATCH_CONFIG_SCHEMA_CORRUPT,   /**< schema 类型或版本损坏。 */
    } watch_config_schema_status_t;

    typedef struct
    {
        uint32_t revision;                                      /**< 配置 owner 单调修订号，旧更新不得覆盖新真值。 */
        char watch_id[WATCH_STATE_WATCH_ID_CAPACITY];           /**< 工厂 base MAC 设备身份。 */
        bool cloud_configured;                                  /**< 云请求前置配置是否全部通过。 */
        watch_cloud_config_status_t cloud_status;               /**< configured/unconfigured/invalid 产品状态。 */
        watch_config_value_status_t base_url_status;            /**< base URL 脱敏状态。 */
        watch_apn_mode_t apn_mode;                              /**< APN auto/configured/invalid 状态。 */
        watch_config_value_status_t token_status;               /**< token 脱敏状态。 */
        watch_config_value_status_t certificate_status;         /**< 证书策略脱敏状态。 */
        watch_config_schema_status_t schema_status;             /**< 配置 schema 状态。 */
        char cloud_error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 最近 CLOUD_ 稳定配置错误。 */
    } watch_config_update_t;

    typedef enum
    {
        WATCH_MODEM_LIFECYCLE_BOOTING = 0,    /**< 模组启动或等待同步。 */
        WATCH_MODEM_LIFECYCLE_SIM_CHECK,      /**< SIM/模组信息检查。 */
        WATCH_MODEM_LIFECYCLE_REGISTERING,    /**< 网络驻网。 */
        WATCH_MODEM_LIFECYCLE_PDP_READY,      /**< PDP/IP 已验证。 */
        WATCH_MODEM_LIFECYCLE_HTTPS_READY,    /**< HTTPS transport 可用。 */
        WATCH_MODEM_LIFECYCLE_REQUEST_ACTIVE, /**< 单请求活动。 */
        WATCH_MODEM_LIFECYCLE_RECOVERING,     /**< 有界恢复。 */
        WATCH_MODEM_LIFECYCLE_BACKOFF,        /**< 射频已关闭并等待自动或显式唤醒。 */
    } watch_modem_lifecycle_t;

    typedef enum
    {
        WATCH_MODEM_SIM_UNKNOWN = 0, /**< SIM 尚未检查。 */
        WATCH_MODEM_SIM_MISSING,     /**< 未插 SIM。 */
        WATCH_MODEM_SIM_NOT_READY,   /**< SIM 未 READY。 */
        WATCH_MODEM_SIM_READY,       /**< SIM READY。 */
    } watch_modem_sim_status_t;

    typedef enum
    {
        WATCH_MODEM_SIGNAL_UNKNOWN = 0, /**< 信号未知。 */
        WATCH_MODEM_SIGNAL_NONE,        /**< 无信号。 */
        WATCH_MODEM_SIGNAL_PRESENT,     /**< 原始信号有效。 */
    } watch_modem_signal_status_t;

    typedef enum
    {
        WATCH_MODEM_REGISTRATION_UNKNOWN = 0, /**< 注册未知。 */
        WATCH_MODEM_REGISTRATION_SEARCHING,   /**< 未驻网或搜索中。 */
        WATCH_MODEM_REGISTRATION_HOME,        /**< 本地驻网。 */
        WATCH_MODEM_REGISTRATION_ROAMING,     /**< 漫游驻网。 */
        WATCH_MODEM_REGISTRATION_DENIED,      /**< 注册拒绝。 */
    } watch_modem_registration_status_t;

    typedef enum
    {
        WATCH_MODEM_PDP_DOWN = 0, /**< PDP/IP 不可用。 */
        WATCH_MODEM_PDP_UP,       /**< PDP/IP 已验证。 */
    } watch_modem_pdp_status_t;

    typedef enum
    {
        WATCH_MODEM_REQUEST_IDLE = 0, /**< 无 HTTPS 请求。 */
        WATCH_MODEM_REQUEST_ACTIVE,   /**< 请求执行中。 */
        WATCH_MODEM_REQUEST_FAILED,   /**< 最近请求失败。 */
        WATCH_MODEM_REQUEST_OK,       /**< 最近请求成功。 */
    } watch_modem_request_status_t;

    typedef struct
    {
        uint32_t modem_update_sequence;                    /**< modem owner 单调更新序号。 */
        watch_modem_lifecycle_t lifecycle;                 /**< 网络生命周期。 */
        watch_modem_sim_status_t sim_status;               /**< SIM 状态。 */
        watch_modem_signal_status_t signal_status;         /**< 信号状态。 */
        watch_modem_registration_status_t registration;    /**< 驻网状态。 */
        watch_modem_pdp_status_t pdp_status;               /**< PDP/IP 状态。 */
        watch_modem_request_status_t request_status;       /**< HTTPS 请求状态。 */
        int16_t csq_rssi;                                  /**< 原始 CSQ RSSI。 */
        int16_t csq_ber;                                   /**< 原始 CSQ BER。 */
        int16_t cesq_rsrq;                                 /**< 原始 CESQ RSRQ，unknown 使用 255。 */
        int16_t cesq_rsrp;                                 /**< 原始 CESQ RSRP，unknown 使用 255。 */
        TickType_t updated_at_ticks;                       /**< 最近更新 tick。 */
        TickType_t last_success_ticks;                     /**< 最近成功 tick。 */
        TickType_t last_failure_ticks;                     /**< 最近失败 tick。 */
        uint32_t recovery_count;                           /**< 恢复饱和计数。 */
        uint32_t line_overflow_count;                      /**< AT 超长行饱和计数。 */
        uint32_t queue_overflow_count;                     /**< AT/result queue 溢出饱和计数。 */
        uint32_t urc_overflow_count;                       /**< URC ring 溢出饱和计数。 */
        char imei[WATCH_MODEM_IDENTITY_CAPACITY];          /**< 有界 IMEI 诊断。 */
        char serial_number[WATCH_MODEM_IDENTITY_CAPACITY]; /**< 有界 SN 诊断。 */
        char operator_name[WATCH_MODEM_OPERATOR_CAPACITY]; /**< 有界运营商。 */
        char ip_address[WATCH_MODEM_IP_CAPACITY];          /**< 有界 IP。 */
        char last_error[WATCH_STATE_ERROR_CODE_CAPACITY];  /**< 最近 MODEM_ 错误。 */
    } watch_modem_update_t;

    typedef watch_modem_update_t watch_modem_snapshot_t;

    typedef enum
    {
        WATCH_GPS_STATUS_OFF = 0,     /**< GPS owner 尚未启动或已经停止。 */
        WATCH_GPS_STATUS_SEARCHING,   /**< 已收到受支持 NMEA，但当前没有新鲜定位。 */
        WATCH_GPS_STATUS_FIXED,       /**< 当前保存的是新鲜且完整的定位坐标。 */
        WATCH_GPS_STATUS_UNAVAILABLE, /**< 驱动、硬件策略或资源不可用。 */
    } watch_gps_status_t;

    typedef enum
    {
        WATCH_GPS_ACQUISITION_STANDBY = 0, /**< WAKE 低电平且没有活动会话。 */
        WATCH_GPS_ACQUISITION_SEARCHING,   /**< 正在执行有界搜星会话。 */
        WATCH_GPS_ACQUISITION_TRACKING,    /**< v2 已定位并持续跟踪。 */
        WATCH_GPS_ACQUISITION_BACKOFF,     /**< 等待自动重试截止时间。 */
        WATCH_GPS_ACQUISITION_UNAVAILABLE, /**< 最近一次驱动或 UART 不可用。 */
    } watch_gps_acquisition_state_t;

    typedef enum
    {
        WATCH_GPS_PURPOSE_TIME_SYNC = 0, /**< 当前会话用于取得可信 GPS 时间。 */
        WATCH_GPS_PURPOSE_LOCATION,      /**< 当前会话用于取得或恢复持续定位。 */
        WATCH_GPS_PURPOSE_SELFTEST,      /**< 当前会话仅由整机自检观察持有。 */
    } watch_gps_purpose_t;

    typedef enum
    {
        WATCH_GPS_TRIGGER_BOOT = 0,    /**< 启动流程触发的首次自动搜星。 */
        WATCH_GPS_TRIGGER_SCHEDULED,   /**< 自动退避截止触发的重试。 */
        WATCH_GPS_TRIGGER_SCREEN_WAKE, /**< 真实熄屏到亮屏触发的机会搜索。 */
        WATCH_GPS_TRIGGER_MANUAL,      /**< 维护页手动触发的搜索。 */
        WATCH_GPS_TRIGGER_SELFTEST,    /**< 整机自检触发的观察会话。 */
        WATCH_GPS_TRIGGER_MOTION,      /**< QMI 或 BLE 步数触发的运动定位。 */
    } watch_gps_trigger_t;

    typedef enum
    {
        WATCH_TIME_SOURCE_NONE = 0, /**< 尚无可信时间。 */
        WATCH_TIME_SOURCE_GPS,      /**< GPS RMC 临时时间。 */
        WATCH_TIME_SOURCE_CLOUD,    /**< 云端最高优先级时间。 */
    } watch_time_source_t;

    typedef struct
    {
        watch_gps_status_t status;                        /**< GPS 产品状态。 */
        watch_gps_acquisition_state_t acquisition_state;  /**< 单一 owner 搜星、跟踪或退避状态。 */
        watch_gps_purpose_t purpose;                       /**< 当前或最近一次搜星会话用途。 */
        watch_gps_trigger_t trigger;                       /**< 当前或最近一次搜星会话触发原因。 */
        bool coordinates_valid;                           /**< 纬度、经度与最近定位时间是否组成有效原子事实。 */
        bool manual_request_available;                    /**< 维护页当前是否允许主动搜星。 */
        bool screen_opportunity_used;                     /**< 当前长退避窗口亮屏机会是否已消耗。 */
        double latitude;                                  /**< 有效时的 WGS-84 十进制度纬度，范围 -90 到 90。 */
        double longitude;                                 /**< 有效时的 WGS-84 十进制度经度，范围 -180 到 180。 */
        uint64_t last_fix_rx_ms;                          /**< 最近有效定位句接收时的本地单调毫秒。 */
        uint64_t next_retry_ms;                           /**< 自动退避截止单调毫秒，无计划时为 0。 */
        uint64_t manual_cooldown_deadline_ms;             /**< 主动请求冷却截止单调毫秒。 */
        uint64_t updated_at_ms;                           /**< 本次完整 GPS 真值生成时的本地单调毫秒。 */
        uint32_t update_sequence;                         /**< GPS owner 启动周期内单调递增的更新序号。 */
        uint32_t session_generation;                      /**< 当前或最近搜星会话代次。 */
        uint8_t backoff_stage;                            /**< 0=10 分钟、1=30 分钟、2=60 分钟。 */
        watch_time_source_t time_source;                  /**< 当前统一时间来源。 */
        bool time_synchronized;                           /**< 当前启动周期是否已有可信时间。 */
        int64_t time_offset_ms;                           /**< 当前 UTC 与启动单调毫秒偏移。 */
        char error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< GPS_ 或 DRV_ 前缀稳定诊断码。 */
    } watch_gps_update_t;

    typedef enum
    {
        WATCH_BLE_TRANSACTION_UNBOUND = 0,        /**< 未绑定且尚无待提交事务。 */
        WATCH_BLE_TRANSACTION_SCANNING,           /**< 正在有界扫描目标 Service。 */
        WATCH_BLE_TRANSACTION_SELECTING,          /**< 已发布多候选，等待 typed 选择。 */
        WATCH_BLE_TRANSACTION_CONNECTING,         /**< 已使用内部 peer address 发起连接。 */
        WATCH_BLE_TRANSACTION_PENDING_VALIDATION, /**< 已连接，等待链路诊断与完整首帧证据。 */
        WATCH_BLE_TRANSACTION_BOUND_READY,        /**< 绑定已提交，identity 事实可用。 */
        WATCH_BLE_TRANSACTION_CLEARING,           /**< 正在 fail-closed 清除绑定。 */
        WATCH_BLE_TRANSACTION_ERROR,              /**< 最近一次 BLE 事务显式失败。 */
    } watch_ble_transaction_t;

    /** 外骨骼固件版本对应的产品型号。 */
    typedef enum
    {
        WATCH_EXOSKELETON_MODEL_UNKNOWN = 0, /**< 版本缺失、损坏或不符合已知格式。 */
        WATCH_EXOSKELETON_MODEL_MINI,        /**< Mini-v1.x.x 型号。 */
        WATCH_EXOSKELETON_MODEL_PRO,         /**< Pro-v2.x.x 型号。 */
        WATCH_EXOSKELETON_MODEL_MAX,         /**< Max-v3.x.x 型号。 */
        WATCH_EXOSKELETON_MODEL_MINI_Y,      /**< MiniY-v4.x.x 型号。 */
        WATCH_EXOSKELETON_MODEL_PRO_Y,       /**< ProY-v5.x.x 型号。 */
        WATCH_EXOSKELETON_MODEL_MAX_Y,       /**< MaxY-v6.x.x 型号。 */
        WATCH_EXOSKELETON_MODEL_OLD_A,       /**< OldA-v7.x.x 单模式型号。 */
        WATCH_EXOSKELETON_MODEL_OLD_C,       /**< OldC-v8.x.x 单模式型号。 */
        WATCH_EXOSKELETON_MODEL_CHILD,       /**< Child-v9.x.x 型号。 */
    } watch_exoskeleton_model_t;

    typedef struct
    {
        float current_mode;                               /**< 当前运动模式原始有效 float 值。 */
        float steps;                                      /**< 当前累计步数原始有效 float 值。 */
        float step_frequency;                             /**< 当前步频原始有效 float 值。 */
        float left_gear;                                  /**< 当前左档位原始有效 float 值。 */
        float right_gear;                                 /**< 当前右档位原始有效 float 值。 */
        float balance_factor;                             /**< 当前平衡因子原始有效 float 值。 */
        uint8_t motor_enable;                             /**< 当前助力开关原始有效值。 */
        uint8_t battery_level;                            /**< 当前外骨骼电量百分比。 */
        uint8_t scene_mode;                               /**< 归一化场景模式：1=标准、2=健身、3=极限、4=下山、5=小碎步。 */
        watch_exoskeleton_scene_config_t scene_config;    /**< 设备上报的场景能力配置。 */
        uint8_t firmware_update_flag;                     /**< 当前固件升级标志原始值。 */
        int8_t left_motor_position;                       /**< 当前左电机位置有符号角度。 */
        int8_t right_motor_position;                      /**< 当前右电机位置有符号角度。 */
        uint8_t standby_time;                             /**< 当前待机时间原始值。 */
        uint8_t temp_alarm;                               /**< 当前温控报警原始值。 */
        char version[WATCH_EXOSKELETON_VERSION_CAPACITY]; /**< 安全终止的外骨骼版本。 */
    } watch_exoskeleton_state_t;

    typedef struct
    {
        uint32_t valid_frame_count;                            /**< 完整有效协议帧饱和计数。 */
        uint32_t invalid_frame_count;                          /**< 完整无效协议帧饱和计数。 */
        uint32_t discarded_byte_count;                         /**< 包头同步前丢弃字节饱和计数。 */
        uint32_t version_sanitized_count;                      /**< 版本字符串隔离饱和计数。 */
        uint32_t reconnect_attempt_count;                      /**< 产品自动重连 procedure 启动饱和计数。 */
        uint32_t reconnect_success_count;                      /**< 同身份端到端恢复成功饱和计数。 */
        char last_error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 最近非 OK 的 BLE_ 稳定错误码。 */
    } watch_ble_diagnostics_t;

    typedef enum
    {
        WATCH_RENTAL_PHASE_WAITING_AUTH = 0, /**< 当前会话尚无租赁授权证明。 */
        WATCH_RENTAL_PHASE_QUERYING,         /**< 云端租赁查询正在进行。 */
        WATCH_RENTAL_PHASE_UNLOCK_WRITING,   /**< 一次性 BLE 解锁写入正在进行。 */
        WATCH_RENTAL_PHASE_CONFIRMING,       /**< 写入成功，等待下一帧有效状态证据。 */
        WATCH_RENTAL_PHASE_UNLOCKED,         /**< 当前外骨骼上电会话已解锁。 */
        WATCH_RENTAL_PHASE_BLOCKED,          /**< 最近授权或解锁流程已 fail-closed。 */
        WATCH_RENTAL_PHASE_WAITING_PAYMENT,  /**< 已确认未支付，保持当前身份等待后续复查。 */
        WATCH_RENTAL_PHASE_RETRY_WAIT,       /**< 暂态云错误，保持当前身份等待退避。 */
    } watch_rental_phase_t;

    typedef enum
    {
        WATCH_CONTROL_BLOCK_NONE = 0,        /**< 当前没有产品控制阻塞原因。 */
        WATCH_CONTROL_BLOCK_UNBOUND,         /**< 尚未绑定外骨骼。 */
        WATCH_CONTROL_BLOCK_BLE_LINK,        /**< BLE/GATT/Notify 链路未就绪。 */
        WATCH_CONTROL_BLOCK_DATA_NOT_READY,  /**< 尚无当前链路有效状态。 */
        WATCH_CONTROL_BLOCK_STATUS_STALE,    /**< 最近状态已经过期。 */
        WATCH_CONTROL_BLOCK_BUSY,            /**< 已有授权、写入或确认事务。 */
        WATCH_CONTROL_BLOCK_UNPAID,          /**< 租赁押金尚未支付。 */
        WATCH_CONTROL_BLOCK_DISABLED,        /**< 外骨骼已被云端禁用。 */
        WATCH_CONTROL_BLOCK_UNKNOWN_DEVICE,  /**< 云端未登记当前外骨骼。 */
        WATCH_CONTROL_BLOCK_CLOUD,           /**< 云端、TLS、HTTP 或响应协议失败。 */
        WATCH_CONTROL_BLOCK_BLE_WRITE,       /**< 一次性 BLE 解锁写入失败。 */
        WATCH_CONTROL_BLOCK_CONFIRM_TIMEOUT, /**< 5 秒状态证据窗口超时。 */
        WATCH_CONTROL_BLOCK_RETRY_EXHAUSTED, /**< 普通控制三次写入均未获合格回显，需真实重连。 */
    } watch_control_block_reason_t;

    typedef enum
    {
        WATCH_RENTAL_RESULT_NONE = 0,        /**< 尚无租赁查询终态。 */
        WATCH_RENTAL_RESULT_PAID,            /**< 云端确认已支付。 */
        WATCH_RENTAL_RESULT_UNPAID,          /**< 云端确认未支付。 */
        WATCH_RENTAL_RESULT_DISABLED,        /**< 云端确认设备禁用。 */
        WATCH_RENTAL_RESULT_UNKNOWN_DEVICE,  /**< 云端未登记设备。 */
        WATCH_RENTAL_RESULT_REQUEST_INVALID, /**< 请求合同被云端拒绝。 */
        WATCH_RENTAL_RESULT_SERVER_ERROR,    /**< 云端服务内部错误。 */
        WATCH_RENTAL_RESULT_TOKEN_INVALID,   /**< 设备 token 无效。 */
        WATCH_RENTAL_RESULT_HTTP_ERROR,      /**< 其他 HTTP 非 2xx 错误。 */
        WATCH_RENTAL_RESULT_TRANSPORT_ERROR, /**< 网络或 modem transport 错误。 */
        WATCH_RENTAL_RESULT_TLS_ERROR,       /**< 证书或 TLS 校验失败。 */
        WATCH_RENTAL_RESULT_TIMEOUT,         /**< 云请求或解锁确认超时。 */
        WATCH_RENTAL_RESULT_PARSE_ERROR,     /**< 云响应合同解析失败。 */
        WATCH_RENTAL_RESULT_CONFIG_ERROR,    /**< 云配置或证书前置无效。 */
    } watch_rental_result_t;

    typedef enum
    {
        WATCH_CONTROL_KIND_NONE = 0,     /**< 没有普通控制目标。 */
        WATCH_CONTROL_KIND_MOTOR_ENABLE, /**< 助力开关控制，目标仅允许 0 或 1。 */
        WATCH_CONTROL_KIND_GEAR,         /**< 统一档位控制，左右协议字段原子写入同一 1-15 目标。 */
        WATCH_CONTROL_KIND_SCENE_MODE,   /**< 场景模式控制，通用业务目标允许 1 至 5，型号能力由 control_gate 复核。 */
        WATCH_CONTROL_KIND_POWEROFF,     /**< 一次性关机控制，目标固定为 1。 */
        WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY, /**< 内部 10/11 档跨界控制，同时覆盖档位与标准/极限模式。 */
    } watch_control_kind_t;

    typedef enum
    {
        WATCH_CONTROL_PHASE_IDLE = 0,              /**< 当前没有普通控制写入。 */
        WATCH_CONTROL_PHASE_WRITING,               /**< 初始有响应 GATT 写入正在进行。 */
        WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED, /**< 写 procedure 成功但尚无设备状态回显。 */
        WATCH_CONTROL_PHASE_POWEROFF_PENDING,      /**< 关机写 procedure 成功，等待离线或在线证据。 */
    } watch_control_phase_t;

    typedef enum
    {
        WATCH_CONTROL_RESULT_NONE = 0,              /**< 尚无普通控制终态。 */
        WATCH_CONTROL_RESULT_NO_OP,                 /**< 目标与协议等价状态一致，未写 BLE。 */
        WATCH_CONTROL_RESULT_BLOCKED,               /**< 产品或 owner 复核未通过。 */
        WATCH_CONTROL_RESULT_WRITE_REJECTED,        /**< NimBLE 初始调用或 callback 拒绝写入。 */
        WATCH_CONTROL_RESULT_SUBMITTED_UNCONFIRMED, /**< 写 procedure 已被接受但未证明设备应用。 */
        WATCH_CONTROL_RESULT_APPLIED,               /**< 新鲜后续状态帧已证明普通控制目标生效。 */
        WATCH_CONTROL_RESULT_RETRYING,              /**< 前一普通控制 attempt 失败，下一 attempt 待写入。 */
        WATCH_CONTROL_RESULT_RETRY_EXHAUSTED,       /**< 三次普通控制 attempt 均失败并进入重连锁止。 */
        WATCH_CONTROL_RESULT_POWEROFF_TERMINAL,     /**< 关机产品结果已固化到独立结果枚举。 */
    } watch_control_result_t;

    typedef enum
    {
        WATCH_POWEROFF_RESULT_NONE = 0,           /**< 尚无关机产品终态。 */
        WATCH_POWEROFF_RESULT_ASSUMED_OFF,        /**< 结果窗口内观察到断开或状态失鲜，预计已关机。 */
        WATCH_POWEROFF_RESULT_TIMEOUT_UNKNOWN,    /**< 15 秒内无法证明设备离线或在线。 */
        WATCH_POWEROFF_RESULT_FAILED_RECONNECTED, /**< 同一设备仍在线且无关机或重启证据。 */
    } watch_poweroff_result_t;

    typedef struct
    {
        uint32_t cloud_update_sequence;                   /**< cloud_task 单调诊断更新序号。 */
        uint32_t request_id;                              /**< 最近租赁请求 ID。 */
        watch_rental_result_t result;                     /**< 最近云查询本地终态。 */
        uint16_t http_status;                             /**< 最近 HTTP 状态，transport 前失败为 0。 */
        char error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 脱敏 CLOUD_ 稳定码。 */
    } watch_cloud_update_t;

    typedef enum
    {
        WATCH_TELEMETRY_RESULT_NEVER_SENT = 0,   /**< 当前 boot 尚无 telemetry 终态。 */
        WATCH_TELEMETRY_RESULT_ACCEPTED,         /**< 云端严格接受最近一次 telemetry。 */
        WATCH_TELEMETRY_RESULT_HTTP_ERROR,       /**< 最近一次 telemetry 收到非 2xx。 */
        WATCH_TELEMETRY_RESULT_TRANSPORT_ERROR,  /**< 最近一次 telemetry transport 失败。 */
        WATCH_TELEMETRY_RESULT_RESPONSE_INVALID, /**< 最近一次 2xx 响应合同无效。 */
    } watch_telemetry_result_t;

    typedef struct
    {
        uint32_t telemetry_update_sequence;               /**< cloud_task 单调 telemetry 诊断序号。 */
        uint32_t request_id;                              /**< 最近 telemetry transport 请求 ID。 */
        uint32_t binding_generation;                      /**< telemetry 请求捕获的 BLE 物理链路代次。 */
        watch_telemetry_result_t result;                  /**< 最近 telemetry 有界终态。 */
        uint16_t http_status;                             /**< 最近 HTTP 状态，transport 前失败为 0。 */
        uint64_t last_attempt_monotonic_ms;               /**< 本次终态处理的 boot 单调毫秒。 */
        int64_t last_success_server_time_ms;              /**< 本终态携带的新成功服务器 UTC 毫秒。 */
        int64_t cloud_offset_ms;                          /**< 本终态成功建立的 boot 云时间偏移。 */
        bool last_success_server_time_valid;              /**< 本终态是否携带新成功服务器时间。 */
        bool cloud_time_synchronized;                     /**< 本终态是否已建立当前 boot RAM 云时间。 */
        char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY];     /**< telemetry 请求捕获的规范绑定 MAC。 */
        char error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 有界 CLOUD_ 稳定诊断码。 */
    } watch_telemetry_update_t;

    /** 当前绑定仍匹配时执行的短小、非阻塞 telemetry 提交动作。 */
    typedef esp_err_t (*watch_state_binding_action_t)(void *context);

    typedef struct
    {
        uint64_t control_ack_deadline_ms;                         /**< 普通控制 callback 成功后的产品 ACK 截止毫秒。 */
        uint64_t poweroff_deadline_ms;                            /**< 关机写成功后等待产品结果的截止毫秒。 */
        uint32_t control_update_sequence;                         /**< control_gate 单调更新序号。 */
        uint32_t request_id;                                      /**< 当前控制更新所属的非零请求 ID；无事务身份时为 0。 */
        uint32_t link_generation;                                 /**< 当前控制更新绑定的 BLE 物理链路代次。 */
        watch_rental_phase_t rental_phase;                        /**< 首次授权与解锁阶段。 */
        watch_control_kind_t control_kind;                        /**< 普通控制类型；无普通控制时为 NONE。 */
        watch_control_phase_t control_phase;                      /**< 独立普通控制操作阶段。 */
        watch_control_result_t control_last_result;               /**< 最近普通控制有界终态。 */
        watch_poweroff_result_t poweroff_result;                  /**< 一次性关机的独立产品终态。 */
        watch_control_block_reason_t block_reason;                /**< 当前控制阻塞原因。 */
        watch_rental_result_t last_rental_result;                 /**< 最近租赁查询的有界终态。 */
        char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY];             /**< 当前控制更新绑定的规范外骨骼 MAC。 */
        char control_error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 脱敏 CLOUD_/BLE_ 稳定码。 */
        uint8_t control_target;                                   /**< 普通控制目标；由 control_kind 决定范围。 */
        uint8_t control_attempt;                                  /**< 当前逻辑普通控制的 attempt，活动或终态范围为 1-3。 */
        bool control_locked_out;                                  /**< 三次普通控制失败后的零写入重连锁止。 */
        bool pending_control;                                     /**< 是否存在授权、写入或确认事务。 */
        bool unlock_session_valid;                                /**< 当前外骨骼上电会话 RAM-only 解锁证明。 */
    } watch_control_update_t;

    typedef struct
    {
        bool has_bound_exoskeleton;                           /**< 是否存在已提交绑定事实。 */
        char bound_exoskeleton_mac[WATCH_BLE_MAC_CAPACITY];   /**< 已提交规范 MAC。 */
        bool ble_connected;                                   /**< BLE 物理链路是否已连接。 */
        uint32_t link_generation;                             /**< 当前 BLE 物理链路代次；未知或断连时为 0。 */
        watch_ble_transaction_t transaction;                  /**< 绑定事务状态。 */
        bool gatt_ready;                                      /**< 目标 Service、Characteristic 与 CCCD 是否唯一确认。 */
        bool mtu_ready;                                       /**< 当前连接是否已记录实际 ATT MTU。 */
        uint16_t negotiated_mtu;                              /**< 当前连接实际 ATT MTU，仅用于诊断。 */
        bool notify_ready;                                    /**< 远端 CCCD write callback 是否已确认成功。 */
        bool device_state_available;                          /**< 是否保留同一身份最近有效产品状态。 */
        bool dataReady;                                       /**< 当前物理链路是否已收到完整有效状态帧。 */
        bool status_fresh;                                    /**< 最近有效状态是否仍处于 5 秒新鲜窗口。 */
        uint64_t last_status_rx_ms;                           /**< 最近有效状态帧的本地单调接收毫秒。 */
        watch_exoskeleton_state_t exoskeleton_state;          /**< 按值复制的完整产品状态。 */
        bool link_control_ready;                              /**< 是否具备链路控制准入证明。 */
        bool clear_pending_control;                           /**< 是否仅发出清除 pending 控制的意图。 */
        char ble_error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 最近 BLE_ 前缀稳定错误码。 */
        watch_ble_diagnostics_t diagnostics;                  /**< BLE owner 按值发布的累计诊断事实。 */
        bool diagnostics_reset;                               /**< 是否显式请求在安全未绑定状态重置累计诊断。 */
    } watch_ble_update_t;

    typedef struct
    {
        bool boot_ready;                                                 /**< 启动流程是否已就绪。 */
        uint32_t config_revision;                                        /**< 最近成功应用的配置修订号。 */
        char watch_id[WATCH_STATE_WATCH_ID_CAPACITY];                    /**< 工厂 base MAC 设备身份。 */
        bool cloud_configured;                                           /**< 云请求前置配置是否全部通过。 */
        watch_cloud_config_status_t cloud_status;                        /**< 云配置产品状态。 */
        watch_config_value_status_t base_url_status;                     /**< base URL 脱敏状态。 */
        watch_apn_mode_t apn_mode;                                       /**< APN 脱敏模式。 */
        watch_config_value_status_t token_status;                        /**< token 脱敏状态。 */
        watch_config_value_status_t certificate_status;                  /**< 证书策略脱敏状态。 */
        watch_config_schema_status_t config_schema_status;               /**< 配置 schema 状态。 */
        char cloud_error_code[WATCH_STATE_ERROR_CODE_CAPACITY];          /**< 最近 CLOUD_ 配置错误。 */
        bool ble_connected;                                              /**< BLE 是否已连接。 */
        uint32_t ble_link_generation;                                    /**< 当前 BLE 物理链路代次，未知或断连时为 0。 */
        bool has_bound_exoskeleton;                                      /**< 是否存在已提交外骨骼绑定。 */
        char bound_exoskeleton_mac[WATCH_BLE_MAC_CAPACITY];              /**< 已提交规范外骨骼 MAC。 */
        watch_exoskeleton_model_t exoskeleton_model;                     /**< 当前绑定设备的缓存型号，普通断连时保留。 */
        watch_ble_transaction_t ble_transaction;                         /**< 当前 BLE 绑定事务状态。 */
        bool gatt_ready;                                                 /**< GATT 结构准入是否完成。 */
        bool mtu_ready;                                                  /**< 是否已记录实际 ATT MTU。 */
        uint16_t negotiated_mtu;                                         /**< 实际 ATT MTU，仅用于诊断。 */
        bool notify_ready;                                               /**< Notify CCCD 是否已由 peer 确认开启。 */
        bool device_state_available;                                     /**< 是否保留同一身份最近有效产品状态。 */
        bool dataReady;                                                  /**< 当前物理链路是否已收到有效状态帧。 */
        bool status_fresh;                                               /**< 最近有效状态是否仍处于新鲜窗口。 */
        uint64_t last_status_rx_ms;                                      /**< 最近有效状态帧的本地单调接收毫秒。 */
        watch_exoskeleton_state_t exoskeleton_state;                     /**< 外骨骼只读产品状态快照。 */
        bool exoskeleton_low_battery;                                    /**< 当前绑定身份隔离的外骨骼低电迟滞事实。 */
        bool link_control_ready;                                         /**< 链路控制准入证明是否有效。 */
        uint32_t control_update_sequence;                                /**< control_gate 最近成功应用的更新序号。 */
        uint32_t control_request_id;                                     /**< 最近 control 更新所属请求 ID，无事务身份时为 0。 */
        char control_exoskeleton_mac[WATCH_BLE_MAC_CAPACITY];            /**< 最近 control 更新绑定的规范外骨骼 MAC。 */
        uint32_t control_link_generation;                                /**< 最近 control 更新绑定的 BLE 链路代次。 */
        watch_rental_phase_t rental_phase;                               /**< 当前首次授权与解锁阶段。 */
        watch_control_kind_t control_kind;                               /**< 当前或最近普通控制类型。 */
        uint8_t control_target;                                          /**< 当前或最近普通控制目标。 */
        watch_control_phase_t control_phase;                             /**< 当前普通控制操作阶段。 */
        watch_control_result_t control_last_result;                      /**< 最近普通控制有界终态。 */
        uint8_t control_attempt;                                         /**< 当前或最近普通控制 attempt，范围 0-3。 */
        uint64_t control_ack_deadline_ms;                                /**< 普通控制产品 ACK 截止毫秒。 */
        bool control_locked_out;                                         /**< 普通控制是否因三次失败等待真实重连。 */
        watch_poweroff_result_t poweroff_result;                         /**< 最近一次关机产品终态。 */
        uint64_t poweroff_deadline_ms;                                   /**< 关机产品结果截止毫秒。 */
        watch_control_block_reason_t control_block_reason;               /**< 当前控制阻塞原因。 */
        watch_rental_result_t last_rental_result;                        /**< 最近租赁查询有界终态。 */
        bool unlock_session_valid;                                       /**< 当前外骨骼上电会话解锁证明是否有效。 */
        bool pending_control;                                            /**< 是否存在由后续 control_gate 拥有的待处理控制。 */
        char control_error_code[WATCH_STATE_ERROR_CODE_CAPACITY];        /**< 最近 CLOUD_/BLE_ 控制稳定码。 */
        uint32_t cloud_update_sequence;                                  /**< cloud_task 最近诊断更新序号。 */
        uint32_t last_cloud_request_id;                                  /**< 最近租赁云请求 ID。 */
        watch_rental_result_t last_cloud_result;                         /**< 最近云查询本地终态。 */
        uint16_t last_cloud_http_status;                                 /**< 最近 HTTP 状态。 */
        char rental_cloud_error_code[WATCH_STATE_ERROR_CODE_CAPACITY];   /**< 最近租赁 CLOUD_ 诊断码。 */
        uint32_t telemetry_update_sequence;                              /**< telemetry 最近成功应用的诊断序号。 */
        uint32_t last_telemetry_request_id;                              /**< 最近 telemetry transport 请求 ID。 */
        watch_telemetry_result_t last_telemetry_result;                  /**< 最近 telemetry 有界终态。 */
        uint16_t last_telemetry_http_status;                             /**< 最近 telemetry HTTP 状态。 */
        uint64_t last_telemetry_attempt_monotonic_ms;                    /**< 最近 telemetry 尝试的 boot 单调毫秒。 */
        int64_t last_telemetry_success_server_time_ms;                   /**< 最近成功 telemetry 的服务器 UTC 毫秒。 */
        int64_t cloud_time_offset_ms;                                    /**< 当前 boot 最近成功建立的云时间偏移。 */
        bool last_telemetry_success_server_time_valid;                   /**< 是否存在当前 boot 成功服务器时间。 */
        bool cloud_time_synchronized;                                    /**< 当前 boot 是否已有服务器时间证据。 */
        watch_time_source_t time_source;                                  /**< 当前统一时间来源。 */
        bool time_synchronized;                                           /**< 当前 boot 是否具有 GPS 或云端可信时间。 */
        int64_t time_offset_ms;                                            /**< 当前统一 UTC 与单调毫秒偏移。 */
        char telemetry_error_code[WATCH_STATE_ERROR_CODE_CAPACITY];      /**< 最近 telemetry CLOUD_ 诊断码。 */
        char ble_error_code[WATCH_STATE_ERROR_CODE_CAPACITY];            /**< 最近 BLE_ 前缀稳定错误码。 */
        watch_ble_diagnostics_t ble_diagnostics;                         /**< BLE 累计诊断与专用最近错误。 */
        watch_modem_snapshot_t modem;                                    /**< modem typed 生命周期与脱敏诊断。 */
        bool modem_ready;                                                /**< HTTPS transport 当前是否可接受请求。 */
        watch_gps_update_t gps;                                          /**< GPS owner 发布的完整定位真值。 */
        bool gps_fix;                                                    /**< 由 gps.status 派生的兼容只读定位标志。 */
        uint8_t battery_percent;                                         /**< 最近一次有效电量百分比，范围 0-100。 */
        bool battery_valid;                                              /**< 最近一次读取是否有效。 */
        bool battery_has_valid_sample;                                   /**< 当前启动周期是否取得过有效电量样本。 */
        watch_power_level_t power_level;                                 /**< power owner 发布的闭集手环电量等级。 */
        TickType_t battery_last_valid_ticks;                             /**< 最近有效电量的系统 tick。 */
        TickType_t battery_last_attempt_ticks;                           /**< 最近读取尝试的系统 tick。 */
        char battery_error_code[WATCH_STATE_ERROR_CODE_CAPACITY];        /**< 最近 POWER_ 前缀稳定状态码。 */
        char battery_driver_error_code[WATCH_STATE_ERROR_CODE_CAPACITY]; /**< 最近 DRV_CW2015_ 前缀驱动码。 */
        watch_screen_state_t screen_state;                               /**< 独立于电池功耗等级的显示事实。 */
        uint64_t last_local_activity_ms;                                 /**< 最近本地真实用户活动单调毫秒。 */
        uint32_t screen_transition_sequence;                             /**< 最近成功采用的显示转换序号。 */
        watch_power_error_t power_error;                                 /**< 最近显示、触摸或本机关机错误。 */
        bool audio_resource_available;                                   /**< 固定 selftest_ok 是否可用。 */
        watch_audio_state_t audio_state;                                 /**< 当前或最近一次音频播放状态。 */
        watch_audio_error_t audio_error;                                 /**< 当前或最近一次音频错误原因。 */
        TickType_t audio_updated_at_ticks;                               /**< 最近音频状态更新时间。 */
        uint32_t audio_update_sequence;                                  /**< 每次成功 apply 后递增的音频状态序号。 */
        uint32_t tap_gate_update_sequence;                               /**< 统一敲击 gate owner 最近更新序号。 */
        bool tap_completed;                                               /**< 完成遮罩事实，由 state_service owner 更新。 */
        bool tap_fault_locked;                                            /**< 故障锁定事实，由 state_service owner 更新。 */
        watch_selftest_summary_t selftest;                               /**< 当前启动周期的最近一次自检摘要。 */
    } watch_state_snapshot_t;

    /**
     * @brief 初始化全局运行状态存储
     * @return ESP_OK 成功
     *         ESP_ERR_NO_MEM 互斥锁创建失败
     */
    esp_err_t watch_state_init(void);

    /**
     * @brief 应用一条运行状态更新
     * @param update 状态更新描述
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 状态模块尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效或字段非法
     *         ESP_ERR_TIMEOUT 获取互斥锁超时
     */
    esp_err_t watch_state_apply_update(const watch_state_update_t *update, TickType_t timeout_ticks);

    /**
     * @brief 原子应用统一敲击 gate owner 更新
     * @param update 完成遮罩与故障锁定的类型化更新
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功；ESP_ERR_INVALID_STATE/ARG；ESP_ERR_TIMEOUT
     */
    esp_err_t watch_state_apply_tap_gate_update(
        const watch_tap_gate_update_t *update,
        TickType_t timeout_ticks);

    /**
     * @brief 原子应用一条类型化手环电量更新
     * @param update 电量有效性、最近尝试时间和诊断上下文
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 状态模块尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         ESP_ERR_TIMEOUT 获取互斥锁超时
     */
    esp_err_t watch_state_apply_battery_update(const watch_battery_update_t *update,
                                               TickType_t timeout_ticks);

    /**
     * @brief 原子应用正交屏幕与本机电源状态更新
     * @param update 显示开关、最近活动、单调序号和 typed 错误
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示未初始化、参数、旧序号或互斥超时
     */
    esp_err_t watch_state_apply_power_update(const watch_power_update_t *update,
                                             TickType_t timeout_ticks);

    /**
     * @brief 原子应用一条类型化音频状态更新
     * @param update 资源可用性、播放状态、错误原因和更新时间
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 状态模块尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         ESP_ERR_TIMEOUT 获取互斥锁超时
     */
    esp_err_t watch_state_apply_audio_update(const watch_audio_update_t *update,
                                             TickType_t timeout_ticks);

    /**
     * @brief 纯校验一条身份与云配置更新的字段组合和稳定错误码
     * @param update 按值身份、脱敏配置状态、修订号和 CLOUD_ 错误
     * @return ESP_OK 组合一致，ESP_ERR_INVALID_ARG 表示字段、状态或错误码不一致
     */
    esp_err_t watch_config_update_validate(const watch_config_update_t *update);

    /**
     * @brief 原子应用一条无凭据类型化身份与云配置更新
     * @param update 按值身份、脱敏配置状态、修订号和 CLOUD_ 错误
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示未初始化、参数非法、旧修订或互斥超时
     */
    esp_err_t watch_state_apply_config_update(const watch_config_update_t *update,
                                              TickType_t timeout_ticks);

    /**
     * @brief 以 fail-closed 优先级保留待重投的最新配置真值
     * @param slot 待重投的完整配置值槽
     * @param dirty 是否存在尚未应用的最新真值
     * @param latest 新配置真值
     * @return ESP_OK 已保存或保留更安全真值，ESP_ERR_INVALID_ARG 表示参数非法
     */
    esp_err_t watch_config_update_store_latest(watch_config_update_t *slot,
                                               bool *dirty,
                                               const watch_config_update_t *latest);

    /**
     * @brief 原子应用一条类型化 BLE 链路与外骨骼产品状态更新
     * @param update 已提交身份、链路、产品快照、新鲜度和稳定诊断
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示未初始化、参数非法或互斥超时
     */
    esp_err_t watch_state_apply_ble_update(const watch_ble_update_t *update,
                                           TickType_t timeout_ticks);

    /**
     * @brief 原子应用 control_gate 拥有的授权、pending 与 RAM-only 会话证明
     * @param update 单调序号、阶段、阻塞原因、租赁终态和稳定码
     * @param timeout_ticks 等待状态 mutex 的有界 tick
     * @return ESP_OK 成功，其他值表示参数、旧序号、状态或互斥超时
     */
    esp_err_t watch_state_apply_control_update(const watch_control_update_t *update,
                                               TickType_t timeout_ticks);

    /**
     * @brief 原子应用 cloud_task 拥有的租赁查询诊断
     * @param update 请求 ID、HTTP 状态、本地终态与 CLOUD_ 稳定码
     * @param timeout_ticks 等待状态 mutex 的有界 tick
     * @return ESP_OK 成功，其他值表示参数、旧序号、状态或互斥超时
     */
    esp_err_t watch_state_apply_cloud_update(const watch_cloud_update_t *update,
                                             TickType_t timeout_ticks);

    /**
     * @brief 在状态互斥区内校验绑定身份并执行一次 telemetry 提交动作
     * @details action 必须短小且非阻塞；绑定更新在 action 返回前不能跨越该提交边界。
     * @param exoskeleton_mac telemetry 请求捕获的规范绑定 MAC
     * @param binding_generation telemetry 请求捕获的 BLE 物理链路代次，可为断连态 0
     * @param action 身份匹配时执行的一次性短动作
     * @param context 传给 action 的调用方上下文
     * @param timeout_ticks 等待状态 mutex 的有界 tick
     * @return ESP_OK 提交成功，其他值表示参数、身份变化、动作失败或互斥超时
     */
    esp_err_t watch_state_run_if_binding_current(
        const char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY],
        uint32_t binding_generation,
        watch_state_binding_action_t action,
        void *context,
        TickType_t timeout_ticks);

    /**
     * @brief 原子应用 cloud_task 拥有的独立 telemetry 诊断
     * @param update 请求、结果、时间证据与 CLOUD_ 稳定码
     * @param timeout_ticks 等待状态 mutex 的有界 tick
     * @return ESP_OK 成功，其他值表示参数、旧序号、状态或互斥超时
     */
    esp_err_t watch_state_apply_telemetry_update(
        const watch_telemetry_update_t *update,
        TickType_t timeout_ticks);

    /**
     * @brief 纯校验独立 telemetry 诊断更新
     * @param update 请求、结果、HTTP、时间证据与稳定码
     * @return ESP_OK 组合一致，ESP_ERR_INVALID_ARG 表示矛盾或越界
     */
    esp_err_t watch_telemetry_update_validate(
        const watch_telemetry_update_t *update);

    /**
     * @brief 应用 modem owner 发布的 typed 最新真值
     * @param update 生命周期、SIM、信号、驻网、PDP、请求和脱敏诊断
     * @param timeout_ticks 等待状态 mutex 的有界 tick
     * @return ESP_OK 成功，其他值表示参数、状态或互斥超时
     */
    esp_err_t watch_state_apply_modem_update(const watch_modem_update_t *update,
                                             TickType_t timeout_ticks);

    /**
     * @brief 纯校验 GPS owner 发布的完整定位真值
     * @param update GPS 状态、可空坐标、新鲜度、序号和稳定诊断码
     * @return ESP_OK 组合一致，ESP_ERR_INVALID_ARG 表示矛盾或毒化更新
     */
    esp_err_t watch_gps_update_validate(const watch_gps_update_t *update);

    /**
     * @brief 原子应用 GPS owner 发布的完整定位真值
     * @param update GPS 状态、可空坐标、新鲜度、序号和稳定诊断码
     * @param timeout_ticks 等待状态 mutex 的有界 tick
     * @return ESP_OK 成功，其他值表示未初始化、参数、旧序号或互斥超时
     */
    esp_err_t watch_state_apply_gps_update(const watch_gps_update_t *update,
                                           TickType_t timeout_ticks);

    /**
     * @brief 按单调序号保留 GPS latest-value 完整真值
     * @param slot 待应用的完整 GPS 值槽
     * @param dirty 是否存在待应用真值
     * @param latest 新 GPS 完整真值
     * @return ESP_OK 已替换，ESP_ERR_INVALID_STATE 表示非更新序号，其他值表示参数非法
     */
    esp_err_t watch_gps_update_store_latest(watch_gps_update_t *slot,
                                            bool *dirty,
                                            const watch_gps_update_t *latest);

    /**
     * @brief 纯校验 modem owner 发布的 typed 最新真值
     * @param update 生命周期、SIM、信号、驻网、PDP、请求和脱敏诊断
     * @return ESP_OK 组合一致，ESP_ERR_INVALID_ARG 表示毒化或矛盾更新
     */
    esp_err_t watch_modem_update_validate(const watch_modem_update_t *update);

    /**
     * @brief 判断 modem typed 快照是否已建立真实 4G 网络连接
     * @details 只依据 SIM、驻网、PDP 和有效 IP 事实，不把 HTTPS transport 可接单状态混入网络连接语义。
     * @param snapshot 已通过状态校验的 modem typed 快照
     * @return true 表示 4G 网络已连接，false 表示未连接或快照为空
     */
    bool watch_modem_network_connected(
        const watch_modem_snapshot_t *snapshot);

    /**
     * @brief 以完整按值替换待重投的最新 BLE 真值
     * @details 仅管理调用方拥有的值槽与 dirty 标志，不访问全局 watch_state 或队列。
     * @param slot 待重投的完整 BLE 值槽
     * @param dirty 是否存在尚未确认入队的最新真值
     * @param latest 覆盖旧值的最新完整真值
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 表示指针无效
     */
    esp_err_t watch_ble_update_store_latest(watch_ble_update_t *slot,
                                            bool *dirty,
                                            const watch_ble_update_t *latest);

    /**
     * @brief 获取不含完整自检摘要的小型音频状态快照
     * @param snapshot 音频状态、错误、时间和更新序号输出
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 状态模块尚未初始化
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_TIMEOUT 获取互斥锁超时
     */
    esp_err_t watch_state_audio_snapshot(watch_audio_snapshot_t *snapshot,
                                         TickType_t timeout_ticks);

    /**
     * @brief 获取电量、屏幕与自检正交事实的小型只读快照
     * @param snapshot 电量等级、最近有效百分比、屏幕和自检 active 输出
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示未初始化、参数非法或互斥超时
     */
    esp_err_t watch_state_power_snapshot(watch_power_snapshot_t *snapshot,
                                         TickType_t timeout_ticks);

    /**
     * @brief 原子开始一次有界自检摘要
     * @param update 运行 ID、首项和开始 tick
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示未初始化、参数非法或互斥超时
     */
    esp_err_t watch_state_apply_selftest_begin(const watch_selftest_begin_update_t *update,
                                               TickType_t timeout_ticks);

    /**
     * @brief 原子累积一个自检单项终态
     * @param update 运行 ID、定长结果和下一项
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示顺序/参数非法或互斥超时
     */
    esp_err_t watch_state_apply_selftest_item(const watch_selftest_item_update_t *update,
                                              TickType_t timeout_ticks);

    /**
     * @brief 原子固化自检完成或取消终态
     * @param update 运行 ID、终态和结束 tick
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功，其他值表示顺序/参数非法或互斥超时
     */
    esp_err_t watch_state_apply_selftest_finish(const watch_selftest_finish_update_t *update,
                                                TickType_t timeout_ticks);

    /**
     * @brief 标记一个既有失败项进入单项重试
     * @param update 原运行 ID、失败项 ID 和重试开始 tick
     * @param timeout_ticks 等待状态互斥锁的有界 tick 数
     * @return ESP_OK 已进入重试，其他值表示参数、状态或超时错误
     */
    esp_err_t watch_state_apply_selftest_retry_begin(
        const watch_selftest_retry_begin_update_t *update,
        TickType_t timeout_ticks);

    /**
     * @brief 用单项重试终态覆盖该失败项并保留其他完整结果
     * @param update 原运行 ID、最新单项终态和结束 tick
     * @param timeout_ticks 等待状态互斥锁的有界 tick 数
     * @return ESP_OK 已固化重试结果，其他值表示参数、状态或超时错误
     */
    esp_err_t watch_state_apply_selftest_retry_finish(
        const watch_selftest_retry_finish_update_t *update,
        TickType_t timeout_ticks);

    /**
     * @brief 获取当前运行状态快照
     * @param snapshot 输出状态快照
     * @param timeout_ticks 等待状态互斥锁的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE 状态模块尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         ESP_ERR_TIMEOUT 获取互斥锁超时
     */
    esp_err_t watch_state_snapshot(watch_state_snapshot_t *snapshot, TickType_t timeout_ticks);

    /**
     * @brief 由严格、区分大小写的 vM.m.p 固件版本识别外骨骼型号
     * @details 接受 v1 至 v9 及与主版本匹配的型号前缀兼容别名。
     * @param version 安全终止且不带额外后缀的固件版本字符串
     * @return 已识别型号；空串、损坏或不匹配时返回 UNKNOWN
     */
    watch_exoskeleton_model_t watch_exoskeleton_model_from_version(
        const char *version);

    /**
     * @brief 查询型号的编译期场景配置
     * @param model 已识别或 UNKNOWN 型号
     * @return v1 至 v9 的矩阵配置；UNKNOWN 返回 INVALID
     */
    watch_exoskeleton_scene_config_t watch_exoskeleton_model_scene_config(
        watch_exoskeleton_model_t model);
    uint8_t watch_exoskeleton_scene_config_max_gear(
        watch_exoskeleton_scene_config_t config);
    bool watch_exoskeleton_scene_mode_from_protocol_config(
        watch_exoskeleton_scene_config_t config,
        uint8_t protocol_scene_mode,
        uint8_t *scene_mode);
    bool watch_exoskeleton_scene_config_supports_mode(
        watch_exoskeleton_scene_config_t config,
        uint8_t scene_mode);
    bool watch_exoskeleton_scene_config_allows_control(
        watch_exoskeleton_scene_config_t config,
        uint8_t scene_mode);

    /**
     * @brief 获取型号允许的最高档位
     * @param model 已识别或 UNKNOWN 型号
     * @return FULL 返回 15，其他有效配置返回 10，UNKNOWN 返回 0
     */
    uint8_t watch_exoskeleton_model_max_gear(
        watch_exoskeleton_model_t model);

    /**
     * @brief 判断型号是否支持极限模式
     * @param model 已识别或 UNKNOWN 型号
     * @return true 表示支持模式 3 和 11 至 15 档
     */
    bool watch_exoskeleton_model_supports_extreme(
        watch_exoskeleton_model_t model);

    /**
     * @brief 判断型号是否显示并允许场景模式切换
     * @param model 已识别或 UNKNOWN 型号
     * @return FULL、NO_EXTREME、STANDARD_FITNESS、STANDARD_SMALL_STEP 返回 true，其他返回 false
     */
    bool watch_exoskeleton_model_has_mode_switch(
        watch_exoskeleton_model_t model);

    /**
     * @brief 判断型号上报是否兼容指定归一化场景模式
     * @param model 已识别或 UNKNOWN 型号
     * @param scene_mode 归一化场景模式业务目标 1 至 5
     * @return true 表示该模式可作为状态真值；STANDARD_ONLY 仅兼容模式 1
     */
    bool watch_exoskeleton_model_supports_scene_mode(
        watch_exoskeleton_model_t model,
        uint8_t scene_mode);

    /**
     * @brief 判断型号是否允许主动下发指定场景模式
     * @details 与上报兼容门禁独立；STANDARD_ONLY 可接收模式 1，但任何场景目标均返回 false。
     * @param model 已识别或 UNKNOWN 型号
     * @param scene_mode 归一化场景模式业务目标 1 至 5
     * @return true 表示允许生成场景控制，false 表示必须零下发
     */
    bool watch_exoskeleton_model_allows_scene_control(
        watch_exoskeleton_model_t model,
        uint8_t scene_mode);

    /**
     * @brief 将归一化场景模式转换为设备线协议枚举
     * @details 三模式配置使用线值 1=标准、2=健身、3=下山；四模式使用线值 1～4；标准健身使用业务值/线值 1/2；标准小碎步使用业务值 1/5 并转换为线值 1/2；STANDARD_ONLY 一律拒绝下发。
     * @param model 外骨骼型号
     * @param scene_mode 归一化场景模式
     * @param protocol_scene_mode 输出的设备线协议枚举
     * @return true 转换成功，false 表示型号不支持目标或参数无效
     */
    bool watch_exoskeleton_scene_mode_to_protocol(
        watch_exoskeleton_model_t model,
        uint8_t scene_mode,
        uint8_t *protocol_scene_mode);

    /**
     * @brief 将设备线协议场景枚举转换为归一化场景模式
     * @details 用于把型号相关的状态包枚举统一为 UI、语音和控制门禁共享的逻辑值。
     * @param model 外骨骼型号
     * @param protocol_scene_mode 设备状态包中的线协议枚举
     * @param scene_mode 输出的归一化场景模式
     * @return true 转换成功，false 表示线协议枚举与型号不兼容或参数无效
     */
    bool watch_exoskeleton_scene_mode_from_protocol(
        watch_exoskeleton_model_t model,
        uint8_t protocol_scene_mode,
        uint8_t *scene_mode);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_APP_STATE_H */
