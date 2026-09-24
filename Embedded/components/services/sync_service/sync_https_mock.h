/**
 * @file     sync_https_mock.h
 * @brief    表驱动 HTTPS mock（主机默认；固件可编译开关）。
 * @details  覆盖成功、重复提交 no-op、低于高水位 no-op、冲突 20003；
 *           不依赖真实 Spring Boot。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_SYNC_HTTPS_MOCK_H
#define EWF_SYNC_HTTPS_MOCK_H

#include <stddef.h>
#include <stdint.h>

#include "sync_https_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EWF_SYNC_MOCK_SUCCESS = 0,       /**< 成功确认并抬升 acked（不附带待应用命令）。 */
    EWF_SYNC_MOCK_SUCCESS_WITH_COMMAND, /**< 成功确认并附带 command_revision+1。 */
    EWF_SYNC_MOCK_IDEMPOTENT_REPEAT, /**< 重复提交同结果 no-op。 */
    EWF_SYNC_MOCK_BELOW_WATERMARK,   /**< 低于/等于已确认高水位 no-op。 */
    EWF_SYNC_MOCK_CONFLICT_20003,    /**< 可恢复冲突。 */
    EWF_SYNC_MOCK_PENDING_20004,     /**< 可选：完成未确认。 */
    EWF_SYNC_MOCK_VERSION_20005,     /**< 可选：经文版本不一致。 */
    EWF_SYNC_MOCK_TRANSPORT_ERROR,   /**< 传输失败。 */
} ewf_sync_mock_scenario_t;

/**
 * @brief 选择下一 mock 场景（测试可注入）
 * @param scenario 场景
 */
void ewf_sync_https_mock_set_scenario(ewf_sync_mock_scenario_t scenario);

/**
 * @brief 读取当前场景
 */
ewf_sync_mock_scenario_t ewf_sync_https_mock_get_scenario(void);

/**
 * @brief 按场景生成响应 JSON
 * @param request 请求
 * @param buffer 输出
 * @param capacity 容量
 * @param http_status 输出 HTTP 状态（业务错误亦为 200）
 * @return 写入长度；传输错误返回 0
 */
size_t ewf_sync_https_mock_build_response(
    const ewf_sync_report_request_t *request,
    char *buffer,
    size_t capacity,
    uint16_t *http_status);

#ifdef __cplusplus
}
#endif

#endif /* EWF_SYNC_HTTPS_MOCK_H */
