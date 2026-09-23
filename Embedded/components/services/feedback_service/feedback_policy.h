/**
 * @file     feedback_policy.h
 * @brief    统一反馈服务的纯逻辑策略。
 * @details  覆盖音量校验与持久化记录打包、高速连击限速合并决策、RGB 节流决策、
 *           按通道的故障降级状态机与故障日志限频。本模块不依赖 ESP-IDF，
 *           可在主机测试中独立验证；播放链与持久化介质由 feedback_audio 与
 *           audio_volume_store 承接，反馈服务（feedback_service）是唯一 owner。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_FEEDBACK_POLICY_H
#define EWF_FEEDBACK_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 音量取值域下界（契约 §9.1：integer 0-100，0 等价静音）。 */
#define EWF_FEEDBACK_VOLUME_MIN 0U
/** 音量取值域上界（契约 §9.1）。 */
#define EWF_FEEDBACK_VOLUME_MAX 100U
/** 音量首次启动默认值（FR-E-005：默认 50/100）。 */
#define EWF_FEEDBACK_VOLUME_DEFAULT 50U
/** 音量持久化 schema 版本；删除或改义字段必须递增并一次性重写（契约 §11）。 */
#define EWF_FEEDBACK_VOLUME_SCHEMA_VERSION 1U
/**
 * @brief 音频最小重触发间隔（毫秒）
 * @details 表现类参数：1 秒 20 次连击时按该间隔合并成可辨识节奏（AD-12），
 *          默认 80 ms 落在样机定标建议区间 60-100 ms 内，待样机定标，不伪装成已验证听感。
 */
#define EWF_FEEDBACK_AUDIO_MIN_INTERVAL_MS 80U
/**
 * @brief RGB 灯效最小更新间隔（毫秒）
 * @details 表现类参数：高速连击期间 RGB 保持低打扰节流，不逐事件全亮度堆叠，
 *          待样机定标。
 */
#define EWF_FEEDBACK_RGB_MIN_INTERVAL_MS 100U
/**
 * @brief 同一通道故障日志的最小发布间隔（毫秒）
 * @details 低打扰约束：同一故障连续发生时限频发布，不重试风暴、不反复刷错；
 *          该间隔只约束日志与事实发布频率，不改变故障事实本身。
 */
#define EWF_FEEDBACK_FAULT_LOG_MIN_INTERVAL_MS 2000U
/**
 * @brief 设备音频资源配置结构版本（契约 §2）
 * @details 设备所有、单调递增、与音量设置相互独立：以构建期常量实现，
 *          资源/配置结构变化时人工递增；不随音量调节变化。真实上报由
 *          Story 2.5 活动窗口客户端消费，本 Story 只建立事实源。
 */
#define EWF_FEEDBACK_AUDIO_CONFIG_VERSION 1U

/**
 * @brief 音量持久化记录（契约 §11.1 device 作用域 volume 首块基线）
 * @details 单一 namespace + schema_version + 单次提交为事务边界；
 *          Story 2.4 落设置存储时必须收敛到同一真源，不得建立第二个音量持久化位。
 */
typedef struct {
    uint32_t schema_version; /**< 音量记录 schema 版本。 */
    uint8_t volume;          /**< 持久化音量，取值域 0-100。 */
} ewf_feedback_volume_record_t;

/** 音量校验结果闭集。 */
typedef enum {
    EWF_FEEDBACK_VOLUME_OK = 0,       /**< 音量在 0-100 取值域内。 */
    EWF_FEEDBACK_VOLUME_NULL,         /**< 输出指针为 NULL。 */
    EWF_FEEDBACK_VOLUME_OUT_OF_RANGE, /**< 音量越界，保持原值并拒绝写入。 */
} ewf_feedback_volume_result_t;

/** 持久化记录校验结果闭集。 */
typedef enum {
    EWF_FEEDBACK_RECORD_OK = 0, /**< 记录完整且可解释。 */
    EWF_FEEDBACK_RECORD_NULL,   /**< 参数为 NULL。 */
    EWF_FEEDBACK_RECORD_SCHEMA, /**< schema 版本不被当前固件解释。 */
    EWF_FEEDBACK_RECORD_VOLUME, /**< 音量字段越界。 */
} ewf_feedback_record_result_t;

/** 音频输出决策闭集。 */
typedef enum {
    EWF_FEEDBACK_AUDIO_PLAY = 0,   /**< 立即播放：距上次输出已超过最小重触发间隔。 */
    EWF_FEEDBACK_AUDIO_MERGE_SKIP, /**< 合并跳过：间隔内合并为既有节奏，不叠加输出。 */
} ewf_feedback_audio_decision_t;

/** RGB 输出决策闭集。 */
typedef enum {
    EWF_FEEDBACK_RGB_UPDATE = 0, /**< 允许更新灯效。 */
    EWF_FEEDBACK_RGB_SKIP,       /**< 节流窗口内跳过本次灯效更新。 */
} ewf_feedback_rgb_decision_t;

/**
 * @brief 反馈策略状态（owner 私有，主机测试可直接驱动）
 * @details 时刻一律使用毫秒单调时钟；跨回绕比较使用无符号差值语义。
 */
typedef struct {
    uint32_t last_audio_ms;       /**< 上次音频输出开始时刻；has_audio 为 false 时无意义。 */
    bool has_audio;               /**< 当前启动周期是否已有音频输出历史。 */
    uint32_t last_rgb_ms;         /**< 上次 RGB 输出时刻；has_rgb 为 false 时无意义。 */
    bool has_rgb;                 /**< 当前启动周期是否已有 RGB 输出历史。 */
    bool audio_fault_active;      /**< 音频通道是否处于故障降级状态。 */
    bool rgb_fault_active;        /**< RGB 通道是否处于故障降级状态。 */
    uint32_t last_audio_fault_ms; /**< 上次音频故障日志发布时刻；未发布过时无意义。 */
    bool has_audio_fault_log;     /**< 当前启动周期是否已发布过音频故障日志。 */
    uint32_t last_rgb_fault_ms;   /**< 上次 RGB 故障日志发布时刻；未发布过时无意义。 */
    bool has_rgb_fault_log;       /**< 当前启动周期是否已发布过 RGB 故障日志。 */
    uint32_t audio_fault_count;   /**< 音频通道累计故障饱和计数（诊断观测用）。 */
    uint32_t rgb_fault_count;     /**< RGB 通道累计故障饱和计数（诊断观测用）。 */
} ewf_feedback_policy_state_t;

/**
 * @brief 校验音量取值域
 * @param volume 待校验音量
 * @return true 音量恰为整数 0-100，false 越界
 */
bool ewf_feedback_volume_valid(uint8_t volume);

/**
 * @brief 校验并打包音量持久化记录
 * @details 越界写入被拒绝（AC 2）；记录以单次提交为事务边界落盘。
 * @param volume 待写入音量
 * @param record 打包输出
 * @return EWF_FEEDBACK_VOLUME_OK 已打包，其他值表示参数或取值域拒绝
 */
ewf_feedback_volume_result_t ewf_feedback_volume_record_pack(
    uint8_t volume,
    ewf_feedback_volume_record_t *record);

/**
 * @brief 校验持久化恢复记录
 * @param record 从介质读出的记录
 * @return EWF_FEEDBACK_RECORD_OK 可采用，其他值表示 schema 或字段损坏
 */
ewf_feedback_record_result_t ewf_feedback_volume_record_validate(
    const ewf_feedback_volume_record_t *record);

/**
 * @brief 初始化反馈策略状态
 * @param state 策略状态输出
 */
void ewf_feedback_policy_init(ewf_feedback_policy_state_t *state);

/**
 * @brief 计算音频输出决策
 * @details 策略只作用于音频反馈通道，绝不触及计数、游标、持久化与同步路径
 *          （AD-12：高速连击保留全部计数）。
 * @param state 策略状态
 * @param now_ms 事件到达时刻（单调毫秒）
 * @return EWF_FEEDBACK_AUDIO_PLAY 立即播放，EWF_FEEDBACK_AUDIO_MERGE_SKIP 合并跳过
 */
ewf_feedback_audio_decision_t ewf_feedback_audio_decide(
    const ewf_feedback_policy_state_t *state,
    uint32_t now_ms);

/**
 * @brief 计算 RGB 灯效决策
 * @param state 策略状态
 * @param now_ms 事件到达时刻（单调毫秒）
 * @return EWF_FEEDBACK_RGB_UPDATE 允许更新，EWF_FEEDBACK_RGB_SKIP 节流跳过
 */
ewf_feedback_rgb_decision_t ewf_feedback_rgb_decide(
    const ewf_feedback_policy_state_t *state,
    uint32_t now_ms);

/**
 * @brief 登记一次音频输出结果并推进故障降级状态机
 * @details 成功即清除故障态（事实自动回到正常态）并重置限频；失败进入故障态，
 *          是否允许发布故障日志由返回值给出（同通道限频，低打扰）。
 * @param state 策略状态
 * @param now_ms 结果登记时刻（单调毫秒）
 * @param output_ms 本次输出开始时刻，用于推进最小间隔基准
 * @param succeeded 本次音频输出是否成功
 * @return true 允许发布故障日志（首 fault 或距上次发布超过限频间隔），false 限频静默
 */
bool ewf_feedback_note_audio_result(ewf_feedback_policy_state_t *state,
                                    uint32_t now_ms,
                                    uint32_t output_ms,
                                    bool succeeded);

/**
 * @brief 登记一次 RGB 输出结果并推进故障降级状态机
 * @param state 策略状态
 * @param now_ms 结果登记时刻（单调毫秒）
 * @param output_ms 本次输出开始时刻，用于推进节流窗口基准
 * @param succeeded 本次 RGB 输出是否成功
 * @return true 允许发布故障日志，false 限频静默
 */
bool ewf_feedback_note_rgb_result(ewf_feedback_policy_state_t *state,
                                  uint32_t now_ms,
                                  uint32_t output_ms,
                                  bool succeeded);

#ifdef __cplusplus
}
#endif

#endif /* EWF_FEEDBACK_POLICY_H */
