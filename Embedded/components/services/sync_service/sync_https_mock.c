/**
 * @file     sync_https_mock.c
 * @brief    HTTPS mock 表驱动实现。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_https_mock.h"

#include <stdio.h>
#include <string.h>

static ewf_sync_mock_scenario_t s_scenario = EWF_SYNC_MOCK_SUCCESS;
static uint32_t s_cloud_acked = 0U;

void ewf_sync_https_mock_set_scenario(ewf_sync_mock_scenario_t scenario)
{
    s_scenario = scenario;
}

ewf_sync_mock_scenario_t ewf_sync_https_mock_get_scenario(void)
{
    return s_scenario;
}

size_t ewf_sync_https_mock_build_response(
    const ewf_sync_report_request_t *request,
    char *buffer,
    size_t capacity,
    uint16_t *http_status)
{
    if (request == NULL || buffer == NULL || capacity == 0U) {
        return 0U;
    }
    if (http_status != NULL) {
        *http_status = 200U;
    }

    if (s_scenario == EWF_SYNC_MOCK_TRANSPORT_ERROR) {
        return 0U;
    }

    int32_t code = 0;
    const char *message = "ok";
    uint32_t acked = request->local_total;
    uint32_t command_revision = request->applied_revision;
    uint8_t volume = 50U;
    const char *brightness = "mid";
    uint32_t timeout_s = 15U;

    switch (s_scenario) {
    case EWF_SYNC_MOCK_SUCCESS:
        if (request->local_total > s_cloud_acked) {
            s_cloud_acked = request->local_total;
        }
        acked = s_cloud_acked;
        /* 成功确认不附带待应用命令，避免默认 mock 路径反复改写设置。 */
        command_revision = request->applied_revision;
        break;
    case EWF_SYNC_MOCK_SUCCESS_WITH_COMMAND:
        if (request->local_total > s_cloud_acked) {
            s_cloud_acked = request->local_total;
        }
        acked = s_cloud_acked;
        command_revision = request->applied_revision + 1U;
        break;
    case EWF_SYNC_MOCK_IDEMPOTENT_REPEAT:
        acked = request->acked_total;
        if (request->local_total > s_cloud_acked) {
            s_cloud_acked = request->local_total;
            acked = s_cloud_acked;
        }
        break;
    case EWF_SYNC_MOCK_BELOW_WATERMARK:
        if (s_cloud_acked < request->acked_total) {
            s_cloud_acked = request->acked_total;
        }
        acked = s_cloud_acked;
        break;
    case EWF_SYNC_MOCK_CONFLICT_20003:
        code = 20003;
        message = "device_reset_conflict";
        acked = s_cloud_acked > 0U ? s_cloud_acked : request->acked_total + 10U;
        break;
    case EWF_SYNC_MOCK_PENDING_20004:
        code = 20004;
        message = "pending_completion";
        acked = request->local_total;
        break;
    case EWF_SYNC_MOCK_VERSION_20005:
        code = 20005;
        message = "scripture_version_mismatch";
        acked = request->acked_total;
        break;
    default:
        break;
    }

    const int written = snprintf(
        buffer,
        capacity,
        "{"
        "\"code\":%ld,"
        "\"message\":\"%s\","
        "\"data\":{"
        "\"acked_total\":%lu,"
        "\"round_id\":%lu,"
        "\"round_state\":\"%s\","
        "\"round_cursor\":%lu,"
        "\"pending_completion\":%s,"
        "\"command_revision\":%lu,"
        "\"snapshot_seq\":1,"
        "\"volume\":%u,"
        "\"brightness\":\"%s\","
        "\"timeout\":%lu"
        "}}",
        (long)code,
        message,
        (unsigned long)acked,
        (unsigned long)request->round_id,
        request->round_state,
        (unsigned long)request->round_cursor,
        request->pending_completion ? "true" : "false",
        (unsigned long)command_revision,
        (unsigned)volume,
        brightness,
        (unsigned long)timeout_s);
    if (written <= 0 || (size_t)written >= capacity) {
        return 0U;
    }
    return (size_t)written;
}
