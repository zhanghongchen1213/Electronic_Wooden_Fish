/**
 * @file     sync_https_codec.c
 * @brief    契约 §6 JSON 编解码实现（固定字段手工编解码）。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_https_codec.h"

#include <stdio.h>
#include <string.h>

static const char *find_key(const char *json, const char *key)
{
    if (json == NULL || key == NULL) {
        return NULL;
    }
    char pattern[64];
    const int written =
        snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    if (written <= 0 || (size_t)written >= sizeof(pattern)) {
        return NULL;
    }
    return strstr(json, pattern);
}

static bool parse_u32_after_key(const char *json, const char *key,
                                uint32_t *out)
{
    const char *pos = find_key(json, key);
    if (pos == NULL || out == NULL) {
        return false;
    }
    pos = strchr(pos + 1, ':');
    if (pos == NULL) {
        return false;
    }
    ++pos;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
        ++pos;
    }
    unsigned long value = 0UL;
    if (sscanf(pos, "%lu", &value) != 1) {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

static bool parse_i32_after_key(const char *json, const char *key, int32_t *out)
{
    const char *pos = find_key(json, key);
    if (pos == NULL || out == NULL) {
        return false;
    }
    pos = strchr(pos + 1, ':');
    if (pos == NULL) {
        return false;
    }
    ++pos;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
        ++pos;
    }
    long value = 0L;
    if (sscanf(pos, "%ld", &value) != 1) {
        return false;
    }
    *out = (int32_t)value;
    return true;
}

static bool parse_bool_after_key(const char *json, const char *key, bool *out)
{
    const char *pos = find_key(json, key);
    if (pos == NULL || out == NULL) {
        return false;
    }
    pos = strchr(pos + 1, ':');
    if (pos == NULL) {
        return false;
    }
    ++pos;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
        ++pos;
    }
    if (strncmp(pos, "true", 4) == 0) {
        *out = true;
        return true;
    }
    if (strncmp(pos, "false", 5) == 0) {
        *out = false;
        return true;
    }
    return false;
}

static bool parse_string_after_key(const char *json, const char *key,
                                   char *out, size_t capacity)
{
    const char *pos = find_key(json, key);
    if (pos == NULL || out == NULL || capacity == 0U) {
        return false;
    }
    pos = strchr(pos + 1, ':');
    if (pos == NULL) {
        return false;
    }
    ++pos;
    while (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r') {
        ++pos;
    }
    if (*pos != '"') {
        return false;
    }
    ++pos;
    size_t i = 0U;
    while (pos[i] != '\0' && pos[i] != '"' && i + 1U < capacity) {
        out[i] = pos[i];
        ++i;
    }
    if (pos[i] != '"') {
        out[0] = '\0';
        return false;
    }
    out[i] = '\0';
    return true;
}

size_t ewf_sync_https_encode_request(const ewf_sync_report_request_t *request,
                                     char *buffer,
                                     size_t capacity)
{
    if (request == NULL || buffer == NULL || capacity < 64U) {
        return 0U;
    }
    const int written = snprintf(
        buffer,
        capacity,
        "{"
        "\"device_id\":\"%s\","
        "\"scripture_version\":\"%s\","
        "\"local_total\":%lu,"
        "\"acked_total\":%lu,"
        "\"round_id\":%lu,"
        "\"round_state\":\"%s\","
        "\"round_cursor\":%lu,"
        "\"pending_completion\":%s,"
        "\"applied_revision\":%lu,"
        "\"battery_percent\":%u,"
        "\"network_mode\":\"%s\","
        "\"audio_config_version\":%lu,"
        "\"firmware_version\":\"%s\","
        "\"action_id\":\"%s\""
        "}",
        request->device_id,
        request->scripture_version,
        (unsigned long)request->local_total,
        (unsigned long)request->acked_total,
        (unsigned long)request->round_id,
        request->round_state,
        (unsigned long)request->round_cursor,
        request->pending_completion ? "true" : "false",
        (unsigned long)request->applied_revision,
        (unsigned)request->battery_percent,
        request->network_mode,
        (unsigned long)request->audio_config_version,
        request->firmware_version,
        request->action_id);
    if (written <= 0 || (size_t)written >= capacity) {
        return 0U;
    }
    return (size_t)written;
}

bool ewf_sync_https_decode_response(const char *json,
                                    ewf_sync_report_response_t *out)
{
    if (json == NULL || out == NULL) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->brightness = 255U;
    if (!parse_i32_after_key(json, "code", &out->code)) {
        return false;
    }
    (void)parse_string_after_key(json, "message", out->message,
                                 sizeof(out->message));
    if (!parse_u32_after_key(json, "acked_total", &out->acked_total) ||
        !parse_u32_after_key(json, "round_id", &out->round_id) ||
        !parse_string_after_key(json, "round_state", out->round_state,
                                sizeof(out->round_state)) ||
        !parse_u32_after_key(json, "round_cursor", &out->round_cursor) ||
        !parse_bool_after_key(json, "pending_completion",
                              &out->pending_completion) ||
        !parse_u32_after_key(json, "command_revision",
                             &out->command_revision) ||
        !parse_u32_after_key(json, "snapshot_seq", &out->snapshot_seq)) {
        return false;
    }

    uint32_t volume = 0U;
    uint32_t timeout_s = 0U;
    char brightness_name[16];
    const bool has_volume = parse_u32_after_key(json, "volume", &volume);
    const bool has_timeout = parse_u32_after_key(json, "timeout", &timeout_s);
    const bool has_brightness =
        parse_string_after_key(json, "brightness", brightness_name,
                               sizeof(brightness_name));
    if (has_volume || has_timeout || has_brightness) {
        out->has_command_payload = true;
        if (has_volume && volume <= 100U) {
            out->volume = (uint8_t)volume;
        }
        if (has_timeout) {
            out->timeout_s = timeout_s;
        }
        if (has_brightness) {
            if (strcmp(brightness_name, "low") == 0) {
                out->brightness = 0U;
            } else if (strcmp(brightness_name, "mid") == 0) {
                out->brightness = 1U;
            } else if (strcmp(brightness_name, "high") == 0) {
                out->brightness = 2U;
            }
        }
    }
    return true;
}

void ewf_sync_https_redact_field(const char *label,
                                 const char *value,
                                 char *out,
                                 size_t capacity)
{
    if (out == NULL || capacity == 0U) {
        return;
    }
    const size_t len = value != NULL ? strlen(value) : 0U;
    (void)snprintf(out, capacity, "%s(len=%lu)",
                   label != NULL ? label : "field",
                   (unsigned long)len);
}
