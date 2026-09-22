/**
 * @file     pvdf_confirm_policy.h
 * @brief    PVDF 唤醒与 ADC 二次确认的纯逻辑策略
 * @details  不含任何 ESP-IDF 依赖，可由主机测试直接编译运行。策略只接收唤醒事件时刻与
 *           ADC 采样结果，产出 typed 结论：只有确认窗口内正向峰值严格超过确认阈值才产出
 *           **候选**有效输入，越界、采样失败与上电盲窗内的事件一律拒绝。
 *           策略不读 ADC、不打印、不阻塞、不推进任何正式累计。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_PVDF_CONFIRM_POLICY_H
#define EWF_PVDF_CONFIRM_POLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 以下四个常量全部是设计值、未冻结，必须按硬件开放项 HW-OI-008 的样机实测结果重新冻结。
 */

/** 确认阈值（mV）：锚定 PVDF 前端文档的比较器设计阈值 148.1 mV（3.3 V × 47 kΩ ÷ (1 MΩ + 47 kΩ)）取整。 */
#define EWF_PVDF_CONFIRM_THRESHOLD_MV 148
/** 确认窗口（ms）：前端 ADC 隔离滤波 τ = 100 µs 远短于窗口；20 次/秒的最小间隔为 50 ms，窗口必须显著小于 50 ms。 */
#define EWF_PVDF_CONFIRM_WINDOW_MS 20U
/** 采样间隔（ms）：与窗口共同决定单个确认内的有界采样次数。 */
#define EWF_PVDF_CONFIRM_SAMPLE_INTERVAL_MS 1U
/** 上电盲窗（ms）：严格大于比较器阈值节点 5τ ≈ 22.4 ms 的上电建立时间，屏蔽 TLV7042 上电高阻伪高电平。 */
#define EWF_PVDF_SETTLING_BLIND_MS 25U

/** 单个确认窗口内允许的最大采样次数，保证采样有界。 */
#define EWF_PVDF_CONFIRM_MAX_SAMPLES \
    (EWF_PVDF_CONFIRM_WINDOW_MS / EWF_PVDF_CONFIRM_SAMPLE_INTERVAL_MS)

    /** 一次唤醒事件的 typed 结论。 */
    typedef enum
    {
        EWF_PVDF_EVENT_NONE = 0,                 /**< 尚未形成结论。 */
        EWF_PVDF_EVENT_CANDIDATE_TAP,            /**< 候选有效输入：窗口内正向峰值严格超过确认阈值。 */
        EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD, /**< 窗口内正向峰值未超过确认阈值。 */
        EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE,    /**< ADC 读数触量程边界或读取超时。 */
        EWF_PVDF_EVENT_REJECTED_BLIND_WINDOW,    /**< 上电盲窗内的唤醒事件。 */
        EWF_PVDF_EVENT_REJECTED_SAMPLE_FAILED,   /**< 窗口内采样未取得任何有效读数。 */
        EWF_PVDF_EVENT_COUNT                     /**< 结论数量，不是有效结论。 */
    } ewf_pvdf_event_kind_t;

    /** 单次 ADC 采样的 typed 状态。 */
    typedef enum
    {
        EWF_PVDF_SAMPLE_VALID = 0,    /**< 采样成功且读数落在该衰减档量程内。 */
        EWF_PVDF_SAMPLE_OUT_OF_RANGE, /**< 读数触量程边界或读取返回超时。 */
        EWF_PVDF_SAMPLE_FAILED,       /**< 其它读取失败，本次未取得有效读数。 */
    } ewf_pvdf_sample_status_t;

    /** 一次唤醒事件的 typed 结论载荷。 */
    typedef struct
    {
        ewf_pvdf_event_kind_t kind;  /**< typed 结论。 */
        int32_t peak_millivolt;      /**< 本次确认观测到的最大正向读数（mV）。 */
        int32_t margin_millivolt;    /**< 候选时的确认裕量 = 峰值 − 阈值；非候选恒为 0。 */
        uint32_t at_ms;              /**< 结论形成时刻的单调毫秒。 */
        uint32_t sample_count;       /**< 本次确认实际投喂的采样次数，有界。 */
    } ewf_pvdf_event_t;

    /** 策略状态；全部字段由策略自身维护，调用方只读。 */
    typedef struct
    {
        bool blind_window_open;           /**< 上电盲窗是否仍未结束。 */
        uint32_t blind_deadline_ms;       /**< 盲窗结束的单调毫秒。 */
        bool confirm_in_flight;           /**< 是否存在一个未出结论的在飞确认。 */
        uint32_t confirm_started_ms;      /**< 在飞确认的触发时刻单调毫秒。 */
        uint32_t confirm_deadline_ms;     /**< 在飞确认的窗口截止单调毫秒。 */
        int32_t peak_millivolt;           /**< 在飞确认窗口内观测到的最大正向读数。 */
        uint32_t sample_count;            /**< 在飞确认已投喂的采样次数。 */
        bool out_of_range_seen;           /**< 在飞确认窗口内是否出现过越界读数。 */
        bool sample_failed_seen;          /**< 在飞确认窗口内是否出现过读取失败。 */
        uint32_t wake_event_count;        /**< 已交接的唤醒事件数。 */
        uint32_t candidate_count;         /**< 已产出的候选有效输入数。 */
        uint32_t rejected_counts[EWF_PVDF_EVENT_COUNT]; /**< 按 typed 结论分类的拒绝计数。 */
        ewf_pvdf_event_kind_t last_kind;  /**< 最近一次结论；尚无结论时为 NONE。 */
        int32_t last_margin_millivolt;    /**< 最近一次候选的确认裕量（mV）。 */
    } ewf_pvdf_confirm_policy_t;

    /** 可复现自检样本的类别。 */
    typedef enum
    {
        EWF_PVDF_SELFCHECK_SINGLE_TAP = 0,      /**< 单次敲击：1 个正峰超阈。 */
        EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, /**< 1 秒 20 次连续敲击：20 个等间隔正峰。 */
        EWF_PVDF_SELFCHECK_CARRY_VIBRATION,     /**< 携带/取放振动：幅值持续低于阈值。 */
        EWF_PVDF_SELFCHECK_OUT_OF_RANGE,        /**< 超量程样本：读数触量程边界。 */
        EWF_PVDF_SELFCHECK_BLIND_WINDOW,        /**< 上电盲窗内样本，附盲窗结束后的对照敲击。 */
        EWF_PVDF_SELFCHECK_SAMPLE_FAILURE,      /**< 采样失败样本：未取得任何有效读数。 */
        EWF_PVDF_SELFCHECK_SCENARIO_COUNT       /**< 样本类别数量，不是有效类别。 */
    } ewf_pvdf_selfcheck_scenario_t;

    /** 单个样本类别的编译期固定判定契约。 */
    typedef struct
    {
        ewf_pvdf_selfcheck_scenario_t scenario; /**< 样本类别，同时也是重放结果的下标。 */
        const char *name;                       /**< 稳定 ASCII 名称。 */
        bool is_tap_scenario;                   /**< true 表示该类别本应产出候选，否则产出候选即误触发。 */
        uint32_t expected_wake_events;          /**< 该类别的合成唤醒事件数。 */
        uint32_t expected_candidates;           /**< 该类别的期望候选数。 */
        uint32_t expected_rejections;           /**< 该类别的期望拒绝数。 */
        ewf_pvdf_event_kind_t expected_kind;    /**< 该类别必须至少出现一次的 typed 结论。 */
    } ewf_pvdf_selfcheck_case_t;

    /** 样本表重放后的确定性计数结果。 */
    typedef struct
    {
        uint32_t wake_event_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];     /**< 各类别的唤醒事件数。 */
        uint32_t candidate_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];      /**< 各类别的候选产出数。 */
        uint32_t rejection_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];      /**< 各类别的拒绝数。 */
        uint32_t false_trigger_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];  /**< 非敲击类别产出候选的误触发数。 */
        uint32_t below_threshold_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];/**< 各类别的未超阈拒绝数。 */
        uint32_t out_of_range_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];   /**< 各类别的越界拒绝数。 */
        uint32_t blind_window_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];   /**< 各类别的盲窗拒绝数。 */
        uint32_t sample_failed_count[EWF_PVDF_SELFCHECK_SCENARIO_COUNT];  /**< 各类别的采样失败拒绝数。 */
        ewf_pvdf_event_kind_t last_kind[EWF_PVDF_SELFCHECK_SCENARIO_COUNT]; /**< 各类别最近一次结论。 */
        uint32_t total_candidates;      /**< 全部类别的候选数合计。 */
        uint32_t total_false_triggers;  /**< 全部类别的误触发数合计。 */
    } ewf_pvdf_selfcheck_counts_t;

    /**
     * @brief 复位策略状态
     * @param policy 策略实例，NULL 时安全返回
     */
    void ewf_pvdf_confirm_policy_reset(ewf_pvdf_confirm_policy_t *policy);

    /**
     * @brief 结束上电盲窗并进入运行态
     * @details 比较器阈值节点需要 5τ ≈ 22.4 ms 建立，且 TLV7042 上电复位期间输出高阻会被
     *          10 kΩ 上拉拉成伪高电平；本调用之后的 EWF_PVDF_SETTLING_BLIND_MS 内仍有盲窗。
     * @param policy 策略实例，NULL 时安全返回
     * @param now_ms 当前单调毫秒
     */
    void ewf_pvdf_confirm_policy_arm_runtime(ewf_pvdf_confirm_policy_t *policy,
                                             uint32_t now_ms);

    /**
     * @brief 交接一次比较器唤醒事件
     * @details 盲窗内直接拒绝；已有在飞确认时按 AD-7"可合并事件可丢最新"合并丢弃，
     *          不建立第二套等待队列、不排队堆积、不延长窗口。
     * @param policy 策略实例，NULL 时返回 false
     * @param now_ms 唤醒事件的单调毫秒
     * @param event 结论输出，可为 NULL
     * @return true 表示已形成终态结论并写入 event
     */
    bool ewf_pvdf_confirm_policy_on_wake_event(ewf_pvdf_confirm_policy_t *policy,
                                               uint32_t now_ms,
                                               ewf_pvdf_event_t *event);

    /**
     * @brief 向在飞确认投喂一次 ADC 采样结果
     * @details 越界读数立即终止本次确认且不重试；采样次数达到 EWF_PVDF_CONFIRM_MAX_SAMPLES
     *          或到达窗口截止时刻时给出终态结论。没有在飞确认时直接忽略。
     * @param policy 策略实例，NULL 时返回 false
     * @param millivolt 采样毫伏值；status 非 VALID 时忽略
     * @param status 本次采样的 typed 状态
     * @param now_ms 采样时刻的单调毫秒
     * @param event 结论输出，可为 NULL
     * @return true 表示本次采样形成了终态结论
     */
    bool ewf_pvdf_confirm_policy_on_sample(ewf_pvdf_confirm_policy_t *policy,
                                           int32_t millivolt,
                                           ewf_pvdf_sample_status_t status,
                                           uint32_t now_ms,
                                           ewf_pvdf_event_t *event);

    /**
     * @brief 有界推进：到达窗口截止时刻时结清在飞确认
     * @param policy 策略实例，NULL 时返回 false
     * @param now_ms 当前单调毫秒
     * @param event 结论输出，可为 NULL
     * @return true 表示本次调用结清了在飞确认
     */
    bool ewf_pvdf_confirm_policy_on_deadline(ewf_pvdf_confirm_policy_t *policy,
                                             uint32_t now_ms,
                                             ewf_pvdf_event_t *event);

    /**
     * @brief 查询在飞确认的窗口截止时刻
     * @param policy 策略实例
     * @return 截止单调毫秒；没有在飞确认或 policy 为 NULL 时返回 0
     */
    uint32_t ewf_pvdf_confirm_policy_deadline_ms(
        const ewf_pvdf_confirm_policy_t *policy);

    /**
     * @brief 查询上电盲窗是否仍未结束
     * @param policy 策略实例
     * @param now_ms 当前单调毫秒
     * @return true 表示仍在盲窗内
     */
    bool ewf_pvdf_confirm_policy_blind_window_open(
        const ewf_pvdf_confirm_policy_t *policy,
        uint32_t now_ms);

    /**
     * @brief 获取 typed 结论的稳定 ASCII 名称
     * @param kind typed 结论
     * @return 稳定名称；未知结论返回 "NONE"
     */
    const char *ewf_pvdf_event_name(ewf_pvdf_event_kind_t kind);

    /**
     * @brief 获取自检样本类别数量
     * @return 样本类别数量
     */
    size_t ewf_pvdf_selfcheck_case_count(void);

    /**
     * @brief 获取编译期固定的自检样本判定契约表
     * @param count 输出类别数量，可为 NULL
     * @return 固定契约表；始终非 NULL
     */
    const ewf_pvdf_selfcheck_case_t *ewf_pvdf_selfcheck_cases(size_t *count);

    /**
     * @brief 用同一张样本表重放策略，产出确定性计数
     * @details 固件自检与主机测试共用本函数；样本为编译期固定、无随机数的合成激励，
     *          因此"可复现"是字节级可复现。
     * @param scratch 调用方提供的策略实例，可为 NULL（内部丢弃重放状态）
     * @param out_counts 重放计数输出，可为 NULL
     */
    void ewf_pvdf_selfcheck_replay(ewf_pvdf_confirm_policy_t *scratch,
                                   ewf_pvdf_selfcheck_counts_t *out_counts);

    /**
     * @brief 判定重放计数是否与编译期期望契约完全一致
     * @param counts 重放计数
     * @return true 表示全部类别一致且误触发为零，false 表示存在偏差或 counts 为空
     */
    bool ewf_pvdf_selfcheck_expectations_met(
        const ewf_pvdf_selfcheck_counts_t *counts);

#ifdef __cplusplus
}
#endif

#endif /* EWF_PVDF_CONFIRM_POLICY_H */
