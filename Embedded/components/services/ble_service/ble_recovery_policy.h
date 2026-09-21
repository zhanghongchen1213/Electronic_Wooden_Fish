/**
 * @file     ble_recovery_policy.h
 * @brief    BLE 自动恢复、授权触发、扫描会话与自检暂停纯策略接口
 * @details  提供无任务、无队列和无全局状态的固定退避、链路授权触发、扫描模式、自检 discovery 暂停转换、饱和计数与 NimBLE 回收守卫。
 * @author   ZHC
 * @date     2026-07-24
 */

#ifndef LEGBOT_BLE_RECOVERY_POLICY_H
#define LEGBOT_BLE_RECOVERY_POLICY_H

#include <stdbool.h>
#include <stdint.h>

/** 亮屏自动重连固定退避档位数量。 */
#define BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT 5U
/** 自动重连策略可使用的最大退避档位数量。 */
#define BLE_RECONNECT_BACKOFF_COUNT BLE_RECONNECT_SCREEN_ON_BACKOFF_COUNT
/** 单次 BLE discovery 的固定窗口。 */
#define BLE_SCAN_WINDOW_DURATION_MS 5000U
/** 亮屏扫描间隔，NimBLE 单位为 0.625 ms，即 30 ms。 */
#define BLE_SCAN_SCREEN_ON_INTERVAL_UNITS 48U
/** 亮屏扫描窗口，NimBLE 单位为 0.625 ms，即 30 ms。 */
#define BLE_SCAN_SCREEN_ON_WINDOW_UNITS 48U
/** 亮屏或控制事务连接间隔下限，NimBLE 单位为 1.25 ms，即 30 ms。 */
#define BLE_CONNECTION_ACTIVE_INTERVAL_MIN_UNITS 24U
/** 亮屏或控制事务连接间隔上限，NimBLE 单位为 1.25 ms，即 50 ms。 */
#define BLE_CONNECTION_ACTIVE_INTERVAL_MAX_UNITS 40U
/** 亮屏或控制事务连接从机延迟。 */
#define BLE_CONNECTION_ACTIVE_LATENCY 0U
/** 亮屏或控制事务监督超时，NimBLE 单位为 10 ms，即 2 秒。 */
#define BLE_CONNECTION_ACTIVE_TIMEOUT_UNITS 200U
/** 熄屏稳定连接间隔下限，NimBLE 单位为 1.25 ms，即 90 ms。 */
#define BLE_CONNECTION_STABLE_INTERVAL_MIN_UNITS 72U
/** 熄屏稳定连接间隔上限，NimBLE 单位为 1.25 ms，即 100 ms。 */
#define BLE_CONNECTION_STABLE_INTERVAL_MAX_UNITS 80U
/** 熄屏稳定连接从机延迟。 */
#define BLE_CONNECTION_STABLE_LATENCY 4U
/** 熄屏稳定连接监督超时，NimBLE 单位为 10 ms，即 3 秒。 */
#define BLE_CONNECTION_STABLE_TIMEOUT_UNITS 300U

typedef struct
{
    uint8_t next_backoff_index; /**< 下一次调度使用的退避档位。 */
    uint8_t scheduled_backoff_index; /**< 当前 deadline 使用的退避档位。 */
    bool pending;               /**< 是否存在尚未消费的绝对 deadline。 */
    bool screen_on;             /**< 当前 deadline 是否按亮屏退避计算。 */
    uint32_t deadline_ms;       /**< 支持 uint32 回绕的绝对单调毫秒 deadline。 */
    uint32_t generation;        /**< deadline 所属恢复代次。 */
} ble_recovery_policy_t;

typedef struct
{
    bool allowed;           /**< 当前 discovery 模式和屏幕事实是否允许扫描。 */
    uint16_t interval_units;/**< 扫描间隔，单位为 0.625 ms。 */
    uint16_t window_units;  /**< 扫描窗口，单位为 0.625 ms。 */
    uint32_t duration_ms;   /**< 单次 discovery 持续时间，单位为毫秒。 */
} ble_scan_power_policy_t;

typedef enum
{
    BLE_CONNECTION_POWER_PROFILE_UNKNOWN = 0, /**< 尚未请求或无法确认的连接参数。 */
    BLE_CONNECTION_POWER_PROFILE_ACTIVE,      /**< 亮屏、解锁或控制事务使用的活跃参数。 */
    BLE_CONNECTION_POWER_PROFILE_STABLE,      /**< 熄屏且无控制事务使用的稳定参数。 */
} ble_connection_power_profile_t;

typedef struct
{
    uint16_t interval_min_units; /**< 连接间隔下限，单位为 1.25 ms。 */
    uint16_t interval_max_units; /**< 连接间隔上限，单位为 1.25 ms。 */
    uint16_t latency;            /**< 从机延迟。 */
    uint16_t timeout_units;      /**< 监督超时，单位为 10 ms。 */
} ble_connection_parameter_policy_t;

typedef struct
{
    bool stop_issued;       /**< 当前 host 周期是否已发出 stop。 */
    bool deinit_attempted;  /**< 当前 host 周期是否已尝试 deinit。 */
    int32_t terminal_error; /**< 首个非零终端回收错误。 */
} ble_nimble_cleanup_guard_t;

typedef enum
{
    BLE_DISCOVERY_PAUSE_NO_CHANGE = 0, /**< 当前暂停状态无需变化。 */
    BLE_DISCOVERY_PAUSE_ENTER,         /**< 首次进入自检暂停态。 */
    BLE_DISCOVERY_PAUSE_EXIT,          /**< 首次退出自检暂停态。 */
} ble_discovery_pause_transition_t;

typedef enum
{
    BLE_DISCOVERY_MODE_IDLE = 0,       /**< 未绑定且候选页未开启，不扫描。 */
    BLE_DISCOVERY_MODE_BINDING_SESSION,/**< 未绑定候选页会话，仅发布候选。 */
    BLE_DISCOVERY_MODE_BOUND_RECONNECT,/**< 已绑定，仅定向发现绑定身份。 */
} ble_discovery_mode_t;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 查询固定产品退避延迟
     * @param attempt 从零开始的恢复尝试序号
     * @return 1000/2000/5000/10000/30000 ms，超出后保持 30000 ms
     */
    uint32_t ble_recovery_backoff_ms(uint32_t attempt);

    /**
     * @brief 按最新屏幕事实查询自动重连退避
     * @param attempt 从零开始的恢复尝试序号
     * @param screen_on true 使用亮屏 1/2/5/10/30 秒，false 表示熄屏暂停自动重连
     * @return 亮屏对应退避毫秒，熄屏返回 0
     */
    uint32_t ble_recovery_backoff_ms_for_screen(uint32_t attempt,
                                                bool screen_on);

    /**
     * @brief 重置恢复策略，使下一次断链从 1 秒档开始
     * @param policy 策略状态
     */
    void ble_recovery_policy_reset(ble_recovery_policy_t *policy);

    /**
     * @brief 按当前档位调度一次绝对 deadline 并推进下一档
     * @param policy 策略状态
     * @param now_ms 当前 uint32 单调毫秒
     * @param generation deadline 所属恢复代次
     */
    void ble_recovery_policy_schedule(ble_recovery_policy_t *policy,
                                      uint32_t now_ms,
                                      uint32_t generation);

    /**
     * @brief 按最新屏幕事实调度一次绝对 deadline 并推进下一档
     * @param policy 策略状态
     * @param now_ms 当前 uint32 单调毫秒
     * @param generation deadline 所属恢复代次
     * @param screen_on 当前是否亮屏；熄屏时取消 pending deadline
     */
    void ble_recovery_policy_schedule_for_screen(
        ble_recovery_policy_t *policy,
        uint32_t now_ms,
        uint32_t generation,
        bool screen_on);

    /**
     * @brief 屏幕状态变化时重定向或取消尚未消费的 deadline
     * @param policy 策略状态
     * @param now_ms 当前 uint32 单调毫秒
     * @param screen_on 最新是否亮屏
     * @return true 表示 pending deadline 已按亮屏重算或因熄屏取消
     */
    bool ble_recovery_policy_retarget_screen(
        ble_recovery_policy_t *policy,
        uint32_t now_ms,
        bool screen_on);

    /**
     * @brief 取消 pending deadline，保留当前退避档位
     * @param policy 策略状态
     */
    void ble_recovery_policy_cancel(ble_recovery_policy_t *policy);

    /**
     * @brief 在 deadline 到达时按正确代次单次消费启动权
     * @param policy 策略状态
     * @param now_ms 当前 uint32 单调毫秒
     * @param generation 当前恢复代次
     * @return true 仅在首次到期且代次匹配时返回
     */
    bool ble_recovery_policy_take_due(ble_recovery_policy_t *policy,
                                      uint32_t now_ms,
                                      uint32_t generation);

    /**
     * @brief 判断当前链路是否应调度一次自动授权
     * @param link_control_ready 当前链路是否已满足完整控制准入
     * @param has_binding 是否存在已提交绑定
     * @param binding_matches 当前链路身份是否匹配已提交绑定
     * @param link_generation 当前非零物理链路代次
     * @param attempt_generation 已完成自动授权尝试的链路代次
     * @return true 仅当前链路首次完整就绪且绑定匹配时返回
     */
    bool ble_authorization_should_schedule(
        bool link_control_ready,
        bool has_binding,
        bool binding_matches,
        uint32_t link_generation,
        uint32_t attempt_generation);

    /**
     * @brief 对 uint32 诊断计数执行饱和加一
     * @param value 当前计数
     * @return 加一后的计数，UINT32_MAX 保持饱和
     */
    uint32_t ble_recovery_saturating_increment(uint32_t value);

    /**
     * @brief 根据自检运行事实计算 discovery 暂停状态转换
     * @param suspended 当前是否已暂停
     * @param selftest_running 当前是否正在执行完整自检或单项重试
     * @return 无变化、首次进入或首次退出
     */
    ble_discovery_pause_transition_t ble_discovery_pause_transition(
        bool suspended,
        bool selftest_running);

    /**
     * @brief 根据绑定事实和候选页会话解析唯一 discovery 模式
     * @param has_binding 是否存在已提交绑定
     * @param binding_session_active 未绑定候选页扫描会话是否活动
     * @return 空闲、候选会话或已绑定定向恢复模式
     */
    ble_discovery_mode_t ble_discovery_resolve_mode(
        bool has_binding,
        bool binding_session_active);

    /**
     * @brief 判断 discovery 模式是否允许发布维护候选
     * @param mode 当前 discovery 模式
     * @return true 仅候选页会话模式可发布
     */
    bool ble_discovery_mode_publishes_candidates(
        ble_discovery_mode_t mode);

    /**
     * @brief 解析当前 discovery 模式和屏幕事实对应的扫描参数
     * @param mode 当前 discovery 模式
     * @param screen_on 当前是否亮屏
     * @return 是否允许以及 interval/window/duration 固定合同
     */
    ble_scan_power_policy_t ble_scan_power_policy_resolve(
        ble_discovery_mode_t mode,
        bool screen_on);

    /**
     * @brief 解析连接当前应使用的功耗档位
     * @param screen_on 当前是否亮屏
     * @param control_active 是否正在执行解锁或普通控制事务
     * @return 活跃或熄屏稳定档位
     */
    ble_connection_power_profile_t ble_connection_power_profile_resolve(
        bool screen_on,
        bool control_active);

    /**
     * @brief 查询连接功耗档位对应的 NimBLE 参数
     * @param profile 活跃或稳定档位
     * @return 固定 interval/latency/timeout 参数；未知档位返回全零
     */
    ble_connection_parameter_policy_t ble_connection_parameter_policy(
        ble_connection_power_profile_t profile);

    /**
     * @brief 重置 NimBLE host 单周期回收守卫
     * @param guard 回收守卫状态
     */
    void ble_nimble_cleanup_guard_reset(ble_nimble_cleanup_guard_t *guard);

    /**
     * @brief 单次取得 host stop 执行权
     * @param guard 回收守卫状态
     * @return true 仅在无终端错误且首次调用时返回
     */
    bool ble_nimble_cleanup_begin_stop(ble_nimble_cleanup_guard_t *guard);

    /**
     * @brief 单次取得 port deinit 执行权
     * @param guard 回收守卫状态
     * @return true 仅在 stop 已发出、无终端错误且首次调用时返回
     */
    bool ble_nimble_cleanup_begin_deinit(ble_nimble_cleanup_guard_t *guard);

    /**
     * @brief 锁存首个非零 host 回收终端错误
     * @param guard 回收守卫状态
     * @param error 非零错误码
     */
    void ble_nimble_cleanup_record_error(ble_nimble_cleanup_guard_t *guard,
                                         int32_t error);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BLE_RECOVERY_POLICY_H */
