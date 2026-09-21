/**
 * @file     exoskeleton_scene_config.h
 * @brief    外骨骼场景与云端对接编译期配置
 * @details  集中维护 v1 至 v9 的场景配置矩阵，以及部署、接口、调度和重试参数。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_EXOSKELETON_SCENE_CONFIG_H
#define LEGBOT_EXOSKELETON_SCENE_CONFIG_H

/* 云端默认部署配置：只用于初始化完全空白的运行时配置。 */

/** 线上联调云 HTTPS 默认 Base URL。 */
#define CLOUD_DEFAULT_BASE_URL "https://watch-api.xianlitech.com"
/** 默认 APN；空字符串表示由模组自动选择。 */
#define CLOUD_DEFAULT_APN ""

/* 云端 API 路径配置：与 Base URL 拼接后形成最终请求地址。 */

/** 租赁授权查询 API 路径。 */
#define CLOUD_RENTAL_CHECK_PATH "/api/v1/iot/rental/check"
/** Telemetry 上报 API 路径。 */
#define CLOUD_TELEMETRY_PATH "/api/v1/iot/telemetry"

/* 云端固定相位与支付调度配置。 */

/** Telemetry 固定云窗口间隔，单位为毫秒，当前为三分钟。 */
#define CLOUD_TELEMETRY_INTERVAL_MS 180000U
/** 快速支付复查间隔，单位为毫秒。 */
#define CLOUD_PAYMENT_FAST_INTERVAL_MS 15000U
/** 快速窗口内包含初查的 HTTP 上限。 */
#define CLOUD_PAYMENT_FAST_HTTP_MAX 12U
/** 首个 UNPAID 后最后一个快速复查偏移，由间隔与 HTTP 上限派生。 */
#define CLOUD_PAYMENT_FAST_LAST_OFFSET_MS \
    (CLOUD_PAYMENT_FAST_INTERVAL_MS * (CLOUD_PAYMENT_FAST_HTTP_MAX - 1U))
/** 首个 UNPAID 后快速窗口结束偏移，边界不发 FAST 请求。 */
#define CLOUD_PAYMENT_FAST_WINDOW_MS 180000U
/** 慢速支付复查直接复用 Telemetry 固定云窗口。 */
#define CLOUD_PAYMENT_SLOW_INTERVAL_MS CLOUD_TELEMETRY_INTERVAL_MS

/* 支付网络暂错七档退避配置，保留五分钟的第四档。 */

/** 支付网络退避档位总数。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_COUNT 7U
/** 支付网络退避第一档，三十秒。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_1_MS 30000U
/** 支付网络退避第二档，一分钟。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_2_MS 60000U
/** 支付网络退避第三档，两分钟。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_3_MS 120000U
/** 支付网络退避第四档，五分钟。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_4_MS 300000U
/** 支付网络退避第五档，一小时。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_5_MS 3600000U
/** 支付网络退避第六档，两小时。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_6_MS 7200000U
/** 支付网络退避第七档及封顶值，四小时。 */
#define CLOUD_PAYMENT_BACKOFF_STEP_7_MS 14400000U

/* 云端 HTTPS 逻辑请求组配置；底层 attempt、连接与清理超时不在此处。 */

/** 逻辑请求初始 attempt 后允许的重试次数上限；Telemetry 取满，支付固定为零。 */
#define CLOUD_REQUEST_RETRY_MAX 2U
/** 逻辑请求第一次重试退避，单位为毫秒。 */
#define CLOUD_REQUEST_RETRY_FIRST_MS 5000U
/** 逻辑请求第二次重试退避，单位为毫秒。 */
#define CLOUD_REQUEST_RETRY_SECOND_MS 15000U
/** 云端 HTTPS 逻辑请求组的业务执行截止预算；失败清理由底层独立收尾。 */
#define CLOUD_REQUEST_GROUP_TIMEOUT_MS 80000U

/* 外骨骼版本场景能力配置。 */

/** 标准、健身、极限、下山四模式配置。 */
#define SCENE_CONFIG_FULL 1
/** 标准、健身、下山三模式配置。 */
#define SCENE_CONFIG_NO_EXTREME 2
/** 标准、健身两模式配置。 */
#define SCENE_CONFIG_STANDARD_FITNESS 3
/** 仅标准上报且禁止场景控制的单模式配置。 */
#define SCENE_CONFIG_STANDARD_ONLY 4
/** 标准、小碎步两模式配置；线协议仍使用 1 与 2。 */
#define SCENE_CONFIG_STANDARD_SMALL_STEP 5

/* 唯一可编辑矩阵：构建参数可在包含本头文件前覆盖任一行。 */
#ifndef WATCH_EXOSKELETON_V1_SCENE_CONFIG
/** v1 Mini 默认使用标准、健身、下山三模式。 */
#define WATCH_EXOSKELETON_V1_SCENE_CONFIG SCENE_CONFIG_NO_EXTREME
#endif
#ifndef WATCH_EXOSKELETON_V2_SCENE_CONFIG
/** v2 Pro 默认使用完整四模式。 */
#define WATCH_EXOSKELETON_V2_SCENE_CONFIG SCENE_CONFIG_FULL
#endif
#ifndef WATCH_EXOSKELETON_V3_SCENE_CONFIG
/** v3 Max 默认使用完整四模式。 */
#define WATCH_EXOSKELETON_V3_SCENE_CONFIG SCENE_CONFIG_FULL
#endif
#ifndef WATCH_EXOSKELETON_V4_SCENE_CONFIG
/** v4 MiniY 默认使用标准、健身、下山三模式。 */
#define WATCH_EXOSKELETON_V4_SCENE_CONFIG SCENE_CONFIG_NO_EXTREME
#endif
#ifndef WATCH_EXOSKELETON_V5_SCENE_CONFIG
/** v5 ProY 默认使用完整四模式。 */
#define WATCH_EXOSKELETON_V5_SCENE_CONFIG SCENE_CONFIG_FULL
#endif
#ifndef WATCH_EXOSKELETON_V6_SCENE_CONFIG
/** v6 MaxY 默认使用完整四模式。 */
#define WATCH_EXOSKELETON_V6_SCENE_CONFIG SCENE_CONFIG_FULL
#endif
#ifndef WATCH_EXOSKELETON_V7_SCENE_CONFIG
/** v7 OldA 默认使用标准与小碎步两模式。 */
#define WATCH_EXOSKELETON_V7_SCENE_CONFIG SCENE_CONFIG_STANDARD_SMALL_STEP
#endif
#ifndef WATCH_EXOSKELETON_V8_SCENE_CONFIG
/** v8 OldC 默认使用标准与小碎步两模式。 */
#define WATCH_EXOSKELETON_V8_SCENE_CONFIG SCENE_CONFIG_STANDARD_SMALL_STEP
#endif
#ifndef WATCH_EXOSKELETON_V9_SCENE_CONFIG
/** v9 Child 默认使用标准、健身、下山三模式。 */
#define WATCH_EXOSKELETON_V9_SCENE_CONFIG SCENE_CONFIG_NO_EXTREME
#endif

/** 判断一个编译期场景配置值是否属于闭集。 */
#define WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(value)                 \
    ((value) == SCENE_CONFIG_FULL ||                                  \
     (value) == SCENE_CONFIG_NO_EXTREME ||                            \
     (value) == SCENE_CONFIG_STANDARD_FITNESS ||                      \
     (value) == SCENE_CONFIG_STANDARD_ONLY ||                         \
     (value) == SCENE_CONFIG_STANDARD_SMALL_STEP)

#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V1_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V1_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V2_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V2_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V3_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V3_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V4_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V4_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V5_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V5_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V6_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V6_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V7_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V7_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V8_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V8_SCENE_CONFIG is invalid"
#endif
#if !WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(WATCH_EXOSKELETON_V9_SCENE_CONFIG)
#error "WATCH_EXOSKELETON_V9_SCENE_CONFIG is invalid"
#endif

/** 外骨骼型号对应的场景能力配置。 */
typedef enum
{
    WATCH_EXOSKELETON_SCENE_CONFIG_INVALID = 0, /**< 未知型号或非法配置。 */
    WATCH_EXOSKELETON_SCENE_CONFIG_FULL = SCENE_CONFIG_FULL, /**< 完整四模式。 */
    WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME = SCENE_CONFIG_NO_EXTREME, /**< 无极限三模式。 */
    WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_FITNESS = SCENE_CONFIG_STANDARD_FITNESS, /**< 标准与健身两模式。 */
    WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_ONLY = SCENE_CONFIG_STANDARD_ONLY, /**< 仅标准上报且禁止场景控制。 */
    WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP = SCENE_CONFIG_STANDARD_SMALL_STEP, /**< 标准与小碎步两模式。 */
} watch_exoskeleton_scene_config_t;

#endif /* LEGBOT_EXOSKELETON_SCENE_CONFIG_H */
