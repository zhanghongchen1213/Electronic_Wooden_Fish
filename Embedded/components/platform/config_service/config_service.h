/**
 * @file     config_service.h
 * @brief    设备身份与本地配置唯一所有者公共接口
 * @details  定义 BLE 绑定、云配置 schema、可选 CA、脱敏快照、请求预检与窄凭据复制边界。
 * @author   ZHC
 * @date     2026-07-13
 */

#ifndef LEGBOT_CONFIG_SERVICE_H
#define LEGBOT_CONFIG_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 配置 schema NVS namespace。 */
#define CONFIG_SERVICE_CONFIG_NAMESPACE "cfg"
/** 云配置 NVS namespace。 */
#define CONFIG_SERVICE_CLOUD_NAMESPACE "cloud"
/** BLE 绑定配置 NVS namespace。 */
#define CONFIG_SERVICE_BLE_NAMESPACE "ble"
/** 云时间派生值 NVS namespace。 */
#define CONFIG_SERVICE_TIME_NAMESPACE "time"
/** UI 偏好 NVS namespace。 */
#define CONFIG_SERVICE_UI_NAMESPACE "ui"
/** 运行诊断 NVS namespace。 */
#define CONFIG_SERVICE_DIAG_NAMESPACE "diag"
/** 配置 schema 版本 key。 */
#define CONFIG_SERVICE_SCHEMA_VERSION_KEY "schema_version"
/** cfg namespace 中已激活云配置代次 key。 */
#define CONFIG_SERVICE_CONFIG_EPOCH_KEY "config_epoch"
/** cloud namespace 中候选云配置代次 key。 */
#define CONFIG_SERVICE_CLOUD_EPOCH_KEY "config_epoch"
/** 云 base URL key。 */
#define CONFIG_SERVICE_CLOUD_BASE_URL_KEY "base_url"
/** 云 APN key。 */
#define CONFIG_SERVICE_CLOUD_APN_KEY "apn"
/** 云 token 受控覆盖 key。 */
#define CONFIG_SERVICE_CLOUD_TOKEN_KEY "token"
/** 云 CA 引用 key。 */
#define CONFIG_SERVICE_CLOUD_CA_REF_KEY "ca_ref"
/** 云 CA PEM key。 */
#define CONFIG_SERVICE_CLOUD_CA_PEM_KEY "ca_pem"
/** BLE 绑定 MAC 配置 key。 */
#define CONFIG_SERVICE_BOUND_EXO_KEY "bound_exo_mac"
/** 当前 boot 可重新派生但允许持久化诊断的云时间偏移 key。 */
#define CONFIG_SERVICE_CLOUD_OFFSET_KEY "cloud_offset_ms"
/** 触觉反馈偏好 key。 */
#define CONFIG_SERVICE_UI_HAPTICS_KEY "haptics"
/** 点击音效偏好 key。 */
#define CONFIG_SERVICE_UI_CLICK_AUDIO_KEY "click_audio"
/** 屏幕亮度档位偏好 key。 */
#define CONFIG_SERVICE_UI_BRIGHTNESS_KEY "brightness"
/** 自动熄屏档位偏好 key。 */
#define CONFIG_SERVICE_UI_SCREEN_TIMEOUT_KEY "screen_off"
/** 抬腕亮屏偏好 key。 */
#define CONFIG_SERVICE_UI_RAISE_WAKE_KEY "raise_wake"
/** 低亮度对应的 CO5300 原始值。 */
#define CONFIG_SERVICE_UI_BRIGHTNESS_LOW_RAW 64U
/** 中亮度对应的 CO5300 原始值。 */
#define CONFIG_SERVICE_UI_BRIGHTNESS_MEDIUM_RAW 160U
/** 高亮度对应的 CO5300 原始值。 */
#define CONFIG_SERVICE_UI_BRIGHTNESS_HIGH_RAW 255U
/** 出厂默认启用按钮触觉反馈。 */
#define CONFIG_SERVICE_UI_DEFAULT_HAPTICS_ENABLED true
/** 出厂默认关闭按钮点击音效。 */
#define CONFIG_SERVICE_UI_DEFAULT_CLICK_AUDIO_ENABLED false
/** 出厂默认关闭抬腕亮屏。 */
#define CONFIG_SERVICE_UI_DEFAULT_RAISE_TO_WAKE_ENABLED false
/** 出厂默认使用中亮度档位。 */
#define CONFIG_SERVICE_UI_DEFAULT_BRIGHTNESS CONFIG_SERVICE_UI_BRIGHTNESS_MEDIUM
/** 出厂默认使用 5 秒自动熄屏档位。 */
#define CONFIG_SERVICE_UI_DEFAULT_SCREEN_TIMEOUT CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S
/** diag namespace 中的整机自检运行中 dirty marker key。 */
#define CONFIG_SERVICE_SELFTEST_RUNNING_KEY "selftest_run"
/** 当前配置 schema 版本。 */
#define CONFIG_SERVICE_SCHEMA_VERSION 1U
/** 规范 MAC 容量，包含字符串终止符。 */
#define CONFIG_SERVICE_MAC_CAPACITY 18U
/** 设备 watch_id 容量，包含字符串终止符。 */
#define CONFIG_SERVICE_WATCH_ID_CAPACITY 18U
/** 云 base URL 最大容量，包含字符串终止符。 */
#define CONFIG_SERVICE_BASE_URL_CAPACITY 192U
/** APN 最大容量，包含字符串终止符。 */
#define CONFIG_SERVICE_APN_CAPACITY 64U
/** token 最大容量，包含字符串终止符。 */
#define CONFIG_SERVICE_TOKEN_CAPACITY 32U
/** CA 引用最大容量，包含字符串终止符。 */
#define CONFIG_SERVICE_CA_REF_CAPACITY 64U
/** CA PEM 最大容量，包含字符串终止符。 */
#define CONFIG_SERVICE_CA_PEM_CAPACITY 2048U
/** 固定 token 脱敏文本容量，包含字符串终止符。 */
#define CONFIG_SERVICE_TOKEN_MASK_CAPACITY 24U
/** 固定 token 脱敏文本。 */
#define CONFIG_SERVICE_TOKEN_MASK "xianli_freedom_****"
/** 云配置缺失时的稳定产品状态。 */
#define CONFIG_SERVICE_CLOUD_UNCONFIGURED_STATE "cloud_unconfigured"

    typedef enum
    {
        CONFIG_SERVICE_BINDING_UNBOUND = 0, /**< key 不存在，这是正常未绑定状态。 */
        CONFIG_SERVICE_BINDING_LOADED,      /**< 已载入通过校验的规范 MAC。 */
        CONFIG_SERVICE_BINDING_CORRUPT,     /**< key 存在但长度或格式损坏，已拒绝使用。 */
    } config_service_binding_status_t;

    typedef enum
    {
        CONFIG_SERVICE_VALUE_UNCONFIGURED = 0, /**< 配置缺失。 */
        CONFIG_SERVICE_VALUE_CONFIGURED,       /**< 配置存在且有效。 */
        CONFIG_SERVICE_VALUE_INVALID,          /**< 配置存在但类型、长度或内容非法。 */
    } config_service_value_status_t;

    typedef enum
    {
        CONFIG_SERVICE_APN_AUTO = 0,   /**< APN 缺失或空，使用运营商自动配置。 */
        CONFIG_SERVICE_APN_CONFIGURED, /**< 非空 APN 已通过校验。 */
        CONFIG_SERVICE_APN_INVALID,    /**< 非空 APN 类型、长度或字符非法。 */
    } config_service_apn_mode_t;

    typedef enum
    {
        CONFIG_SERVICE_SCHEMA_VALID = 0, /**< 当前 schema 可解释，缺失按空 v1 迁移。 */
        CONFIG_SERVICE_SCHEMA_CORRUPT,   /**< schema 类型错误或版本未知。 */
    } config_service_schema_status_t;

    typedef enum
    {
        CLOUD_ERROR_NONE = 0,        /**< 云配置预检通过。 */
        CLOUD_WATCH_ID_UNAVAILABLE,  /**< 芯片身份不可用。 */
        CLOUD_SCHEMA_CORRUPT,        /**< 配置 schema 损坏或未知。 */
        CLOUD_BASE_URL_UNCONFIGURED, /**< 云 base URL 未配置。 */
        CLOUD_BASE_URL_INVALID,      /**< 云 base URL 非 HTTPS 或 host 非法。 */
        CLOUD_APN_INVALID,           /**< 非空 APN 非法。 */
        CLOUD_TOKEN_UNCONFIGURED,    /**< token 未配置。 */
        CLOUD_TOKEN_INVALID,         /**< token 覆盖不符合 v1 合同。 */
        CLOUD_CERT_UNCONFIGURED,     /**< 兼容保留的 CA 未配置状态码，当前预检不返回。 */
        CLOUD_CERT_INVALID,          /**< CA 引用或 PEM 非法。 */
    } cloud_config_error_t;

    typedef struct
    {
        config_service_binding_status_t status;                  /**< BLE 绑定加载状态。 */
        bool has_bound_exoskeleton;                              /**< 是否存在已提交绑定。 */
        char bound_exoskeleton_mac[CONFIG_SERVICE_MAC_CAPACITY]; /**< 已提交规范 MAC。 */
        char watch_id[CONFIG_SERVICE_WATCH_ID_CAPACITY];         /**< 工厂 base MAC 设备身份。 */
        uint32_t config_revision;                                /**< 启动加载及每次完整云配置提交后的单调修订号。 */
        uint16_t schema_version;                                 /**< 当前解释后的配置 schema 版本。 */
        uint16_t config_epoch;                                   /**< cfg/cloud 两 namespace 已激活的一致配置代次。 */
        config_service_schema_status_t schema_status;            /**< schema 完整性状态。 */
        config_service_value_status_t cloud_base_url_status;     /**< base URL 脱敏状态。 */
        config_service_apn_mode_t apn_mode;                      /**< APN auto/configured/invalid 状态。 */
        config_service_value_status_t token_status;              /**< token 配置状态。 */
        config_service_value_status_t certificate_status;        /**< 可选 CA 策略状态。 */
        bool cloud_configured;                                   /**< 云请求必要配置是否通过。 */
        cloud_config_error_t cloud_error;                        /**< 最近确定的 CLOUD_ 配置错误。 */
    } config_service_snapshot_t;

    typedef struct
    {
        char base_url[CONFIG_SERVICE_BASE_URL_CAPACITY]; /**< HTTPS base URL 原值。 */
        char apn[CONFIG_SERVICE_APN_CAPACITY];           /**< APN 原值，空表示 auto。 */
        char token[CONFIG_SERVICE_TOKEN_CAPACITY];       /**< v1 token 原值。 */
        char ca_ref[CONFIG_SERVICE_CA_REF_CAPACITY];     /**< CA 引用原值。 */
        char ca_pem[CONFIG_SERVICE_CA_PEM_CAPACITY];     /**< CA PEM 原值。 */
    } config_service_cloud_credentials_t;

    typedef enum
    {
        CONFIG_SERVICE_UI_BRIGHTNESS_LOW = 0, /**< 低亮度档位。 */
        CONFIG_SERVICE_UI_BRIGHTNESS_MEDIUM,  /**< 中亮度档位。 */
        CONFIG_SERVICE_UI_BRIGHTNESS_HIGH,    /**< 高亮度档位。 */
        CONFIG_SERVICE_UI_BRIGHTNESS_COUNT,   /**< 亮度档位数量边界。 */
    } config_service_ui_brightness_t;

    typedef enum
    {
        CONFIG_SERVICE_UI_SCREEN_TIMEOUT_5S = 0, /**< 5 秒自动熄屏；沿用旧 15 秒档位序号。 */
        CONFIG_SERVICE_UI_SCREEN_TIMEOUT_15S,    /**< 15 秒自动熄屏；沿用旧 30 秒档位序号。 */
        CONFIG_SERVICE_UI_SCREEN_TIMEOUT_30S,    /**< 30 秒自动熄屏；沿用旧 60 秒档位序号。 */
        CONFIG_SERVICE_UI_SCREEN_TIMEOUT_COUNT,   /**< 自动熄屏档位数量边界。 */
    } config_service_ui_screen_timeout_t;

    typedef struct
    {
        bool haptics_enabled;                                  /**< 启用按钮触觉反馈。 */
        bool click_audio_enabled;                              /**< 启用按钮点击音效。 */
        bool raise_to_wake_enabled;                            /**< 启用抬腕亮屏。 */
        config_service_ui_brightness_t brightness;             /**< 用户选择的屏幕亮度档位。 */
        config_service_ui_screen_timeout_t screen_timeout;     /**< 用户选择的自动熄屏档位。 */
    } config_service_ui_preferences_t;

    /** 配置成功提交后向上层协调器发布无凭据快照的回调。 */
    typedef esp_err_t (*config_service_change_callback_t)(
        const config_service_snapshot_t *snapshot,
        void *context);

    /**
     * @brief 初始化配置 owner 并从 NVS 加载身份、云配置和 BLE 绑定
     * @return ESP_OK 已初始化，其他值表示真实 MAC、NVS I/O 或内存错误
     */
    esp_err_t config_service_init(void);

    /**
     * @brief 读取已提交的无凭据配置快照
     * @param snapshot 快照输出
     * @param timeout_ticks 等待配置互斥锁的有界 tick 数
     * @return ESP_OK 成功，其他值表示未初始化、参数无效或超时
     */
    esp_err_t config_service_snapshot(config_service_snapshot_t *snapshot,
                                      TickType_t timeout_ticks);

    /**
     * @brief 注册或清除配置成功提交后的无凭据真值回调
     * @details 回调在配置 mutex 释放后同步调用，不得长期阻塞；传入 NULL 可清除注册。
     * @param callback 上层 typed-state 发布回调，NULL 表示清除
     * @param context 原样传回回调的上下文
     * @return ESP_OK 成功，其他值表示未初始化或互斥超时
     */
    esp_err_t config_service_set_change_callback(
        config_service_change_callback_t callback,
        void *context);

    /**
     * @brief 重试发布配置 owner 保留的最新待通知无凭据真值
     * @return ESP_OK 无待通知或已被上层接受，其他值表示未初始化、未注册或暂时繁忙
     */
    esp_err_t config_service_retry_pending_change(void);

    /**
     * @brief 使用 set+commit 原子边界保存外骨骼绑定 MAC
     * @param mac 17 字符冒号 MAC，接受小写并规范化为大写
     * @return ESP_OK 已提交并更新快照，其他值表示参数、NVS set 或 commit 失败
     */
    esp_err_t config_service_set_bound_exoskeleton_mac(
        const char mac[CONFIG_SERVICE_MAC_CAPACITY]);

    /**
     * @brief 仅清除 BLE 绑定 key 并提交
     * @return ESP_OK 已清除或原本未绑定，其他值表示真实 NVS erase/commit 失败
     */
    esp_err_t config_service_clear_bound_exoskeleton_mac(void);

    /**
     * @brief 以 set/erase + commit 边界写入完整云配置
     * @details 空 base URL 清除配置，空 APN 表示 auto，空 token 恢复受控固件值；CA 可不配置，引用与 PEM 只能二选一。
     * @param credentials 待提交的有界完整配置
     * @return ESP_OK 完整提交后 RAM 真值已更新，其他值表示参数、NVS set/erase 或 commit 失败
     */
    esp_err_t config_service_set_cloud_credentials(
        const config_service_cloud_credentials_t *credentials);

    /**
     * @brief 根据脱敏配置事实执行纯云请求前置检查
     * @param snapshot 无凭据配置事实
     * @return 稳定 CLOUD_ 错误；CLOUD_ERROR_NONE 表示配置层允许进入 transport
     */
    cloud_config_error_t config_service_cloud_preflight(
        const config_service_snapshot_t *snapshot);

    /**
     * @brief 获取稳定 CLOUD_ 错误码文本
     * @param error typed 云配置错误
     * @return 静态只读稳定码，非法枚举返回 CLOUD_CONFIG_UNKNOWN
     */
    const char *config_service_cloud_error_code(cloud_config_error_t error);

    /**
     * @brief 为唯一 cloud consumer 有界复制当前有效凭据
     * @details 仅在预检通过时输出；调用方使用后必须及时清零整个输出结构。
     * @param credentials 凭据输出
     * @param timeout_ticks 等待配置互斥锁的有界 tick 数
     * @return ESP_OK 成功，其他值表示配置未就绪、参数无效或超时
     */
    esp_err_t config_service_cloud_credentials_copy(
        config_service_cloud_credentials_t *credentials,
        TickType_t timeout_ticks);

    /**
     * @brief 从 config owner 的 RAM 缓存读取 i64 云时间派生偏移
     * @details 缓存由内部栈启动流程从 NVS 加载；调用方不得据此证明当前 boot 已完成时间同步。
     * @param offset_ms i64 偏移输出；key 不存在时输出 0
     * @param timeout_ticks 等待配置 mutex 的有界 tick
     * @return ESP_OK 已读取，ESP_ERR_NVS_NOT_FOUND 表示尚无值，其他值表示状态、参数、互斥或 NVS 错误
     */
    esp_err_t config_service_cloud_offset_read(int64_t *offset_ms,
                                               TickType_t timeout_ticks);

    /**
     * @brief 以 set+commit 事务保存 i64 云时间派生偏移
     * @param offset_ms server_time_ms 减当前 boot 单调毫秒的派生偏移
     * @param timeout_ticks 等待配置 mutex 的有界 tick
     * @return ESP_OK 已提交，其他值表示状态、互斥或 NVS 事务错误
     */
    esp_err_t config_service_cloud_offset_write(int64_t offset_ms,
                                                TickType_t timeout_ticks);

    /**
     * @brief 读取持久化 UI 偏好
     * @details key 不存在时使用“振动开启、声音与抬腕关闭、中亮度、5 秒”；类型损坏时保留同一组安全默认并返回错误。
     * @param preferences UI 偏好输出
     * @param timeout_ticks 等待配置 mutex 的有界 tick
     * @return ESP_OK 已读取或应用默认值，其他值表示状态、参数、互斥或 NVS 错误
     */
    esp_err_t config_service_ui_preferences_read(
        config_service_ui_preferences_t *preferences,
        TickType_t timeout_ticks);

    /**
     * @brief 以单次 NVS commit 保存完整 UI 偏好
     * @param preferences 待保存的完整 UI 偏好
     * @param timeout_ticks 等待配置 mutex 的有界 tick
     * @return ESP_OK 已持久化，其他值表示状态、参数、互斥或 NVS 错误
     */
    esp_err_t config_service_ui_preferences_write(
        const config_service_ui_preferences_t *preferences,
        TickType_t timeout_ticks);

    /**
     * @brief 读取整机自检运行中 dirty marker
     * @details key 不存在时输出 false；该标记仅表示上次运行未发布完整终态。
     * @param running 运行中标记输出
     * @param timeout_ticks 等待配置 mutex 的有界 tick
     * @return ESP_OK 已读取或应用默认值，其他值表示状态、参数、互斥或 NVS 错误
     */
    esp_err_t config_service_selftest_running_read(bool *running,
                                                   TickType_t timeout_ticks);

    /**
     * @brief 持久化整机自检运行中 dirty marker
     * @param running true 表示整机自检已进入运行，false 表示完整终态已发布
     * @param timeout_ticks 等待配置 mutex 的有界 tick
     * @return ESP_OK 已提交，其他值表示状态、互斥或 NVS 事务错误
     */
    esp_err_t config_service_selftest_running_write(bool running,
                                                    TickType_t timeout_ticks);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CONFIG_SERVICE_H */
