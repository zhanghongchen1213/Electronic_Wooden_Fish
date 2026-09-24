/**
 * @file     sync_https_codec.h
 * @brief    契约 §6 HTTPS JSON 请求/响应编解码。
 * @details  请求恰 14 个 snake_case 字段；响应解析 {code,message,data} 信封。
 *           不得发明 delta/high_watermark 等未授权 wire 名。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_SYNC_HTTPS_CODEC_H
#define EWF_SYNC_HTTPS_CODEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** device_id 容量。 */
#define EWF_SYNC_DEVICE_ID_CAPACITY 64U
/** scripture_version 容量。 */
#define EWF_SYNC_SCRIPTURE_VERSION_CAPACITY 32U
/** firmware_version 容量。 */
#define EWF_SYNC_FIRMWARE_VERSION_CAPACITY 32U
/** action_id 容量。 */
#define EWF_SYNC_ACTION_ID_CAPACITY 64U
/** network_mode wire 容量。 */
#define EWF_SYNC_NETWORK_MODE_CAPACITY 16U
/** round_state wire 容量。 */
#define EWF_SYNC_ROUND_STATE_CAPACITY 16U
/** message 容量。 */
#define EWF_SYNC_MESSAGE_CAPACITY 96U

typedef struct {
    char device_id[EWF_SYNC_DEVICE_ID_CAPACITY];
    char scripture_version[EWF_SYNC_SCRIPTURE_VERSION_CAPACITY];
    uint32_t local_total;
    uint32_t acked_total;
    uint32_t round_id;
    char round_state[EWF_SYNC_ROUND_STATE_CAPACITY];
    uint32_t round_cursor;
    bool pending_completion;
    uint32_t applied_revision;
    uint8_t battery_percent;
    char network_mode[EWF_SYNC_NETWORK_MODE_CAPACITY];
    uint32_t audio_config_version;
    char firmware_version[EWF_SYNC_FIRMWARE_VERSION_CAPACITY];
    char action_id[EWF_SYNC_ACTION_ID_CAPACITY];
} ewf_sync_report_request_t;

typedef struct {
    int32_t code;
    char message[EWF_SYNC_MESSAGE_CAPACITY];
    uint32_t acked_total;
    uint32_t round_id;
    char round_state[EWF_SYNC_ROUND_STATE_CAPACITY];
    uint32_t round_cursor;
    bool pending_completion;
    uint32_t command_revision;
    uint32_t snapshot_seq;
    uint8_t volume;
    uint8_t brightness; /**< 0=low 1=mid 2=high；非法时保持 255。 */
    uint32_t timeout_s;
    bool has_command_payload;
} ewf_sync_report_response_t;

/**
 * @brief 编码 §6.1 请求 JSON
 * @param request 请求
 * @param buffer 输出缓冲
 * @param capacity 容量
 * @return 写入长度（不含 NUL）；失败返回 0
 */
size_t ewf_sync_https_encode_request(const ewf_sync_report_request_t *request,
                                     char *buffer,
                                     size_t capacity);

/**
 * @brief 解码信封 + §6.2 响应
 * @param json 响应正文
 * @param out 输出
 * @return true 解析成功
 */
bool ewf_sync_https_decode_response(const char *json,
                                    ewf_sync_report_response_t *out);

/**
 * @brief 脱敏日志片段：只返回长度描述，永不回显 token/URL/正文
 * @param label 字段标签
 * @param value 原值（可空）
 * @param out 输出缓冲
 * @param capacity 容量
 */
void ewf_sync_https_redact_field(const char *label,
                                 const char *value,
                                 char *out,
                                 size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* EWF_SYNC_HTTPS_CODEC_H */
