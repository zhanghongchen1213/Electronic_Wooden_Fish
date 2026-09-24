/**
 * @file     device_settings_policy.h
 * @brief    契约 §11.1 设备设置字段校验与默认真值的纯逻辑。
 * @details  brightness 冻结为 low|mid|high（不得写入 medium）；timeout 为 5|15|30；
 *           volume 0–100；applied_revision 单调不减。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_DEVICE_SETTINGS_POLICY_H
#define EWF_DEVICE_SETTINGS_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 设备设置 schema 版本。 */
#define EWF_DEVICE_SETTINGS_SCHEMA_VERSION 1U
/** 默认音量。 */
#define EWF_DEVICE_SETTINGS_VOLUME_DEFAULT 50U
/** 默认熄屏秒数。 */
#define EWF_DEVICE_SETTINGS_TIMEOUT_DEFAULT 15U
/** 默认 applied_revision。 */
#define EWF_DEVICE_SETTINGS_APPLIED_REVISION_DEFAULT 0U

/** brightness 原始档映射到 CO5300 0–255；样机定标前 hardware_pending。 */
#define EWF_BRIGHTNESS_RAW_LOW 64U
#define EWF_BRIGHTNESS_RAW_MID 160U
#define EWF_BRIGHTNESS_RAW_HIGH 255U

typedef enum {
    EWF_BRIGHTNESS_LOW = 0,
    EWF_BRIGHTNESS_MID,
    EWF_BRIGHTNESS_HIGH,
    EWF_BRIGHTNESS_COUNT
} ewf_brightness_t;

typedef enum {
    EWF_SYNC_STATUS_IDLE = 0, /**< 尚无立即同步请求。 */
    EWF_SYNC_STATUS_PENDING,  /**< 已请求，等待通信客户端。 */
    EWF_SYNC_STATUS_BUSY,     /**< 通信客户端活动中（Story 2.5）。 */
    EWF_SYNC_STATUS_OK,       /**< 客户端回报成功。 */
    EWF_SYNC_STATUS_FAIL,     /**< 客户端回报失败或本地无客户端。 */
    EWF_SYNC_STATUS_COUNT
} ewf_sync_status_t;

typedef struct {
    uint32_t schema_version;   /**< 存储 schema。 */
    uint8_t volume;            /**< 0–100。 */
    ewf_brightness_t brightness;
    uint32_t timeout_s;        /**< 5/15/30。 */
    uint32_t applied_revision; /**< 本地高水位，单调不减。 */
} ewf_device_settings_record_t;

typedef enum {
    EWF_SETTINGS_OK = 0,
    EWF_SETTINGS_NULL,
    EWF_SETTINGS_VOLUME,
    EWF_SETTINGS_BRIGHTNESS,
    EWF_SETTINGS_TIMEOUT,
    EWF_SETTINGS_REVISION,
    EWF_SETTINGS_SCHEMA,
} ewf_settings_result_t;

/**
 * @brief 填充契约默认设置记录
 */
void ewf_device_settings_defaults(ewf_device_settings_record_t *record);

/**
 * @brief 校验完整设置记录
 */
ewf_settings_result_t ewf_device_settings_validate(
    const ewf_device_settings_record_t *record);

/**
 * @brief 校验音量域
 */
bool ewf_device_settings_volume_valid(uint8_t volume);

/**
 * @brief 校验 timeout 枚举
 */
bool ewf_device_settings_timeout_valid(uint32_t timeout_s);

/**
 * @brief 按 wire 名解析亮度；只接受 low|mid|high
 * @param name wire 名
 * @param out 输出枚举
 * @return true 解析成功
 */
bool ewf_device_settings_brightness_parse(const char *name,
                                          ewf_brightness_t *out);

/**
 * @brief 亮度枚举转 wire 名
 */
const char *ewf_device_settings_brightness_name(ewf_brightness_t value);

/**
 * @brief 亮度映射到 CO5300 原始档（hardware_pending）
 */
uint16_t ewf_device_settings_brightness_raw(ewf_brightness_t value);

/**
 * @brief 校验 applied_revision 相对旧值是否单调不减
 */
bool ewf_device_settings_revision_ok(uint32_t previous, uint32_t next);

#ifdef __cplusplus
}
#endif

#endif /* EWF_DEVICE_SETTINGS_POLICY_H */
