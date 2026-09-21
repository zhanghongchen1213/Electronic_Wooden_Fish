/**
 * @file     power_interaction_policy.h
 * @brief    日常屏幕与运行态 PWR 纯策略接口
 * @details  定义可由 production-C host 测试直接驱动的 idle、短按释放、去抖和显示意图合同。
 * @author   ZHC
 * @date     2026-07-27
 */

#ifndef LEGBOT_POWER_INTERACTION_POLICY_H
#define LEGBOT_POWER_INTERACTION_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"

/** 常态屏幕默认自动熄屏时间。 */
#define POWER_SERVICE_IDLE_TIMEOUT_MS 5000U
/** 自动熄屏最短可选时间。 */
#define POWER_SERVICE_IDLE_TIMEOUT_5S_MS 5000U
/** 自动熄屏中间可选时间。 */
#define POWER_SERVICE_IDLE_TIMEOUT_15S_MS 15000U
/** 自动熄屏最长可选时间。 */
#define POWER_SERVICE_IDLE_TIMEOUT_30S_MS 30000U
/** BLE 搜索与连接页固定自动熄屏时间。 */
#define POWER_SERVICE_BLE_PAGE_IDLE_TIMEOUT_MS 30000U
/** 手环电量不高于该百分比时进入普通低电提醒。 */
#define POWER_WATCH_LOW_BATTERY_ENTER_PERCENT 30U
/** 普通低电后只有高于该百分比才退出提醒。 */
#define POWER_WATCH_LOW_BATTERY_EXIT_PERCENT 33U
/** 手环电量不高于该百分比时进入严重低电。 */
#define POWER_WATCH_CRITICAL_BATTERY_ENTER_PERCENT 15U
/** 严重低电后只有高于该百分比才退回普通低电。 */
#define POWER_WATCH_CRITICAL_BATTERY_EXIT_PERCENT 18U
/** 正常和普通低电使用的 CO5300 原始亮度。 */
#define POWER_DISPLAY_NORMAL_BRIGHTNESS 255U
/** 严重低电常态最大原始亮度，按满量程 30% 向下取整。 */
#define POWER_DISPLAY_CRITICAL_BRIGHTNESS_MAX 76U
/** 没有活动 deadline 时允许 power_task 无限期阻塞的返回值。 */
#define POWER_EVENT_WAIT_FOREVER_MS UINT32_MAX

typedef enum
{
    POWER_DISPLAY_INTENT_NONE = 0, /**< 当前采样不产生显示动作。 */
    POWER_DISPLAY_INTENT_WAKE,     /**< 请求 ui_task 恢复显示。 */
    POWER_DISPLAY_INTENT_SLEEP,    /**< 请求 ui_task 关闭显示。 */
    POWER_DISPLAY_INTENT_NEXT_PAGE, /**< 请求 ui_task 切到下一个常驻页面。 */
    POWER_DISPLAY_INTENT_STOP,     /**< 停止固定 power_task。 */
    POWER_DISPLAY_INTENT_COUNT,    /**< 显示与导航意图闭集边界。 */
} power_display_intent_t;

typedef struct
{
    uint64_t now_ms;        /**< 当前 boot 单调毫秒，可自然无符号回绕。 */
    bool pwr_valid;         /**< PWR 极性和去抖已有目标板证据，可采用 pressed。 */
    bool pwr_pressed;       /**< 已由硬件证据解释并去抖后的 PWR 按下事实。 */
    bool local_activity;    /**< 触摸或 BOOT0 语音入口产生的本地用户活动。 */
    bool wrist_raise_activity; /**< 已由 IMU 姿态分类器确认的一次抬腕活动。 */
    bool background_update; /**< BLE、telemetry、GPS 或模型刷新等后台更新。 */
    bool selftest_fact_valid; /**< 本轮 selftest_active 是否来自成功状态快照。 */
    bool selftest_active;   /**< 整机自检或单项重试正在运行。 */
    bool stop_requested;    /**< 服务框架要求 power_task 停止。 */
} power_interaction_input_t;

typedef struct
{
    uint64_t last_activity_ms; /**< 最近一次本地真实用户活动单调毫秒。 */
    uint32_t idle_timeout_ms;  /**< 当前采用的自动熄屏时间。 */
    bool ble_page_idle_timeout_active; /**< BLE 搜索与连接页是否采用固定 30 秒熄屏。 */
    bool display_on;           /**< power owner 期望的当前显示开关事实。 */
    bool selftest_active;      /**< 上一次采样是否处于自检或单项重试。 */
    bool selftest_entry_display_on; /**< 进入自检前的显示事实。 */
    bool selftest_user_activity; /**< 自检期间是否出现真实本地活动。 */
    bool selftest_wake_suppressed; /**< 自检强制亮屏失败后是否等待新用户事件。 */
    bool pwr_valid;            /**< 上一次采样是否具有可采用的 PWR 证据。 */
    bool pwr_pressed;          /**< 上一次已采用的稳定 PWR 状态。 */
    bool pwr_press_started_display_on; /**< 当前按压开始时屏幕是否已亮。 */
    bool pwr_release_suppressed; /**< 当前按压是否因自检边界不得在释放时触发动作。 */
    bool stopped;              /**< STOP 后阻止任何后续交互动作。 */
} power_interaction_policy_t;

typedef struct
{
    uint64_t candidate_since_ms; /**< 当前候选状态首次出现的单调毫秒。 */
    uint32_t debounce_ms;        /**< 目标板实测提供的稳定窗口；测试值不代表板级阈值。 */
    bool stable_pressed;         /**< 最近已确认的稳定按下状态。 */
    bool candidate_pressed;      /**< 等待稳定窗口确认的候选状态。 */
    bool candidate_active;       /**< 当前是否正在观察候选状态。 */
} power_pwr_debounce_t;

typedef struct
{
    power_display_intent_t pending; /**< 队列拥塞时保留的显示或导航意图。 */
    uint32_t pending_count;          /**< 相对导航尚未成功入队的精确次数。 */
} power_display_intent_mailbox_t;

typedef struct
{
    watch_power_level_t level; /**< 当前已采用的闭集电量等级。 */
    uint8_t last_valid_percent; /**< 最近一次有效电量百分比。 */
    bool has_valid_sample;      /**< 是否已经取得过有效电量样本。 */
    bool last_sample_valid;     /**< 最近一次输入是否是 0-100 的有效样本。 */
} power_battery_level_policy_t;

typedef struct
{
    uint64_t deadline_ms; /**< 绝对 boot 单调毫秒截止点。 */
    bool active;          /**< true 表示本截止点参与最早时间选择。 */
} power_event_deadline_t;

typedef struct
{
    bool immediate_work;                    /**< 已有 latest 事实或未提交意图需立即处理。 */
    power_event_deadline_t idle;             /**< 亮屏自动熄屏截止点。 */
    power_event_deadline_t pwr_debounce;     /**< PWR 候选稳定窗口截止点。 */
    power_event_deadline_t wrist_raise;      /**< 抬腕重武装或候选分类截止点。 */
    power_event_deadline_t battery;          /**< CW2015 下一次采样截止点。 */
    power_event_deadline_t pending_delivery; /**< 未提交状态或显示意图的重试截止点。 */
} power_event_deadlines_t;

/**
 * @brief 初始化手环电量等级迟滞策略
 * @param policy 调用方拥有的电量策略状态
 */
void power_battery_level_policy_init(
    power_battery_level_policy_t *policy);

/**
 * @brief 推进一次手环电量等级采样
 * @details 无效或越界采样只更新有效性，保留最近百分比和已采用等级。
 * @param policy 调用方拥有的电量策略状态
 * @param valid 采样是否由 CW2015 owner 判定有效
 * @param percent 电量百分比，有效范围 0-100
 * @return 当前采用的闭集电量等级
 */
watch_power_level_t power_battery_level_policy_advance(
    power_battery_level_policy_t *policy,
    bool valid,
    uint8_t percent);

/**
 * @brief 选择当前电量和自检组合下的原始显示亮度
 * @details 自检始终使用正常亮度；非法等级在非自检下保守采用严重低电上限。
 * @param power_level power owner 已采用的闭集电量等级
 * @param selftest_active 整机自检或单项重试是否 active
 * @return CO5300 原始亮度值
 */
uint16_t power_display_brightness_for(
    watch_power_level_t power_level,
    bool selftest_active);

/**
 * @brief 选择用户亮度与当前电量、自检组合下的有效原始亮度
 * @details 自检固定满亮；严重低电将用户值限制到安全上限，其他状态保留用户值。
 * @param power_level power owner 已采用的闭集电量等级
 * @param selftest_active 整机自检或单项重试是否 active
 * @param user_brightness 用户选择的 CO5300 原始亮度
 * @return CO5300 有效原始亮度值
 */
uint16_t power_display_brightness_for_user(
    watch_power_level_t power_level,
    bool selftest_active,
    uint16_t user_brightness);

/**
 * @brief 初始化日常屏幕与 PWR 纯策略
 * @param policy 调用方拥有的策略状态
 * @param now_ms 初始化时的 boot 单调毫秒
 * @param display_on 初始化时的显示事实
 */
void power_interaction_policy_init(power_interaction_policy_t *policy,
                                   uint64_t now_ms,
                                   bool display_on);

/**
 * @brief 校验自动熄屏时间是否属于三档闭集
 * @param timeout_ms 待校验毫秒数
 * @return true 是 5/15/30 秒之一，false 非法
 */
bool power_interaction_policy_idle_timeout_valid(uint32_t timeout_ms);

/**
 * @brief 更新日常屏幕自动熄屏时间
 * @details 保留最近活动时间，使缩短后的截止线可在下一次推进立即生效。
 * @param policy 调用方拥有的策略状态
 * @param timeout_ms 5/15/30 秒之一
 * @return true 已采用，false 参数或档位非法
 */
bool power_interaction_policy_set_idle_timeout_ms(
    power_interaction_policy_t *policy,
    uint32_t timeout_ms);

/**
 * @brief 设置 BLE 搜索与连接页固定熄屏状态
 * @details 页面可见时固定使用 30 秒；进入和离开时都从当前时刻重新计时。
 * @param policy 调用方拥有的策略状态
 * @param active true 使用 BLE 页固定 30 秒，false 恢复用户设置
 * @param now_ms 当前 boot 单调毫秒
 */
void power_interaction_policy_set_ble_page_active(
    power_interaction_policy_t *policy,
    bool active,
    uint64_t now_ms);

/**
 * @brief 用 ui_task 已确认的显示结果校正 owner 状态
 * @param policy 调用方拥有的策略状态
 * @param display_on ui_task 已确认的实际显示开关事实
 */
void power_interaction_policy_reconcile_display(
    power_interaction_policy_t *policy,
    bool display_on);

/**
 * @brief 记录自检强制亮屏的硬件失败
 * @details 仅在自检仍 active 且显示实际为关时抑制自动重投；新触摸、PWR
 *          短按释放或下一次自检会话可重新尝试一次。
 * @param policy 调用方拥有的策略状态
 */
void power_interaction_policy_note_display_failure(
    power_interaction_policy_t *policy);

/**
 * @brief 推进一次日常屏幕与 PWR 纯策略采样
 * @details 后台事实不重置 idle；屏亮时抬腕不延长 idle；短按只在释放后成立。
 * @param policy 调用方拥有的策略状态
 * @param input 当前稳定输入与单调时间
 * @return 本次应由 owner 执行的唯一动作
 */
power_display_intent_t power_interaction_policy_advance(
    power_interaction_policy_t *policy,
    const power_interaction_input_t *input);

/**
 * @brief 初始化不解释 GPIO 极性的 PWR 去抖器
 * @param debounce 调用方拥有的去抖状态
 * @param initial_pressed 已由板级证据解释的初始按下状态
 * @param debounce_ms 目标板实测确认的稳定窗口
 */
void power_pwr_debounce_init(power_pwr_debounce_t *debounce,
                             bool initial_pressed,
                             uint32_t debounce_ms);

/**
 * @brief 推进一次已解释极性的 PWR 原始按下采样
 * @details 本函数只做时间稳定过滤，不读取 GPIO，也不猜测高低电平含义。
 * @param debounce 调用方拥有的去抖状态
 * @param raw_pressed 已由板级证据解释极性的瞬时按下状态
 * @param now_ms 当前 boot 单调毫秒
 * @param changed 输出稳定状态本轮是否变化
 * @return 当前稳定按下状态
 */
bool power_pwr_debounce_advance(power_pwr_debounce_t *debounce,
                                bool raw_pressed,
                                uint64_t now_ms,
                                bool *changed);

/**
 * @brief 合并一个最新显示或常驻页导航意图
 * @details 显示转换保留 latest 值；连续 NEXT_PAGE 精确累计次数。显示转换与常驻页导航由调用方使用独立保留槽，避免互相覆盖。
 * @param mailbox 调用方拥有的有界意图保留槽
 * @param intent 最新策略动作，非 WAKE/SLEEP/NEXT_PAGE 会被忽略
 */
void power_display_intent_mailbox_offer(
    power_display_intent_mailbox_t *mailbox,
    power_display_intent_t intent);

/**
 * @brief 读取当前待提交的显示或常驻页导航意图
 * @param mailbox 调用方拥有的有界意图保留槽
 * @return 当前待提交 WAKE/SLEEP/NEXT_PAGE，无待办时返回 NONE
 */
power_display_intent_t power_display_intent_mailbox_peek(
    const power_display_intent_mailbox_t *mailbox);

/**
 * @brief 在 UI 队列成功接受当前意图后清空保留槽
 * @param mailbox 调用方拥有的有界意图保留槽
 */
void power_display_intent_mailbox_complete(
    power_display_intent_mailbox_t *mailbox);

/**
 * @brief 根据一组绝对 deadline 计算下一次队列阻塞毫秒数
 * @details 任一已到期或 immediate 工作返回 0；无活动 deadline 返回
 *          POWER_EVENT_WAIT_FOREVER_MS，不得用固定周期轮询替代。
 * @param now_ms 当前 boot 单调毫秒
 * @param deadlines power owner 当前的活动截止点集合
 * @return 0 立即处理，POWER_EVENT_WAIT_FOREVER_MS 无限期阻塞，其他值为最早剩余毫秒
 */
uint32_t power_event_deadlines_wait_ms(
    uint64_t now_ms,
    const power_event_deadlines_t *deadlines);

#endif /* LEGBOT_POWER_INTERACTION_POLICY_H */
