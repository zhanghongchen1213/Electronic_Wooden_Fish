/**
 * @file     test_sync_https_codec.c
 * @brief    §6 JSON 编解码、mock 四类响应与脱敏主机测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_https_codec.h"
#include "sync_https_mock.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static ewf_sync_report_request_t sample_request(void)
{
    ewf_sync_report_request_t req;
    memset(&req, 0, sizeof(req));
    strcpy(req.device_id, "dev-1");
    strcpy(req.scripture_version, "HS-1.0.0");
    req.local_total = 20U;
    req.acked_total = 10U;
    req.round_id = 1U;
    strcpy(req.round_state, "in_progress");
    req.round_cursor = 5U;
    req.pending_completion = false;
    req.applied_revision = 2U;
    req.battery_percent = 80U;
    strcpy(req.network_mode, "connected");
    req.audio_config_version = 1U;
    strcpy(req.firmware_version, "1.0.0");
    strcpy(req.action_id, "act-1");
    return req;
}

static void test_encode_has_fourteen_fields(void)
{
    const ewf_sync_report_request_t req = sample_request();
    char buffer[1024];
    const size_t len =
        ewf_sync_https_encode_request(&req, buffer, sizeof(buffer));
    assert(len > 0U);
    assert(strstr(buffer, "device_id") != NULL);
    assert(strstr(buffer, "scripture_version") != NULL);
    assert(strstr(buffer, "local_total") != NULL);
    assert(strstr(buffer, "acked_total") != NULL);
    assert(strstr(buffer, "round_id") != NULL);
    assert(strstr(buffer, "round_state") != NULL);
    assert(strstr(buffer, "round_cursor") != NULL);
    assert(strstr(buffer, "pending_completion") != NULL);
    assert(strstr(buffer, "applied_revision") != NULL);
    assert(strstr(buffer, "battery_percent") != NULL);
    assert(strstr(buffer, "network_mode") != NULL);
    assert(strstr(buffer, "audio_config_version") != NULL);
    assert(strstr(buffer, "firmware_version") != NULL);
    assert(strstr(buffer, "action_id") != NULL);
    assert(strstr(buffer, "delta") == NULL);
    assert(strstr(buffer, "high_watermark") == NULL);
}

static void test_mock_four_scenarios(void)
{
    const ewf_sync_report_request_t req = sample_request();
    char buffer[2048];
    uint16_t http = 0U;
    ewf_sync_report_response_t resp;

    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_SUCCESS);
    assert(ewf_sync_https_mock_build_response(&req, buffer, sizeof(buffer),
                                              &http) > 0U);
    assert(http == 200U);
    assert(ewf_sync_https_decode_response(buffer, &resp));
    assert(resp.code == 0);
    assert(resp.acked_total == 20U);

    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_IDEMPOTENT_REPEAT);
    assert(ewf_sync_https_mock_build_response(&req, buffer, sizeof(buffer),
                                              &http) > 0U);
    assert(ewf_sync_https_decode_response(buffer, &resp));
    assert(resp.code == 0);

    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_BELOW_WATERMARK);
    assert(ewf_sync_https_mock_build_response(&req, buffer, sizeof(buffer),
                                              &http) > 0U);
    assert(ewf_sync_https_decode_response(buffer, &resp));
    assert(resp.acked_total >= req.acked_total);

    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_CONFLICT_20003);
    assert(ewf_sync_https_mock_build_response(&req, buffer, sizeof(buffer),
                                              &http) > 0U);
    assert(ewf_sync_https_decode_response(buffer, &resp));
    assert(resp.code == 20003);
}

static void test_redact_no_secrets(void)
{
    char out[64];
    ewf_sync_https_redact_field("token", "super-secret-token", out, sizeof(out));
    assert(strstr(out, "super-secret-token") == NULL);
    assert(strstr(out, "len=") != NULL);
    ewf_sync_https_redact_field("url", "https://example.com/path?k=1", out,
                                sizeof(out));
    assert(strstr(out, "https://") == NULL);
}

int main(void)
{
    test_encode_has_fourteen_fields();
    test_mock_four_scenarios();
    test_redact_no_secrets();
    printf("PASS test_sync_https_codec\n");
    return 0;
}
