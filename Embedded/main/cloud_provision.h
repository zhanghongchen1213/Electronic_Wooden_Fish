/**
 * @file     cloud_provision.h
 * @brief    线上联调云配置初始化接口
 * @details  仅为空白配置写入无 CA 的默认 HTTPS 参数，已有配置保持不变。
 * @author   ZHC
 * @date     2026-07-20
 */

#ifndef LEGBOT_CLOUD_PROVISION_H
#define LEGBOT_CLOUD_PROVISION_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 为空白配置初始化默认线上联调参数
     * @details 已有、部分或损坏配置均保持不变，不会按 URL 内容识别或覆盖人工配置。
     * @return ESP_OK 表示已完成或无需修改，其他值表示快照读取或 NVS 提交失败
     */
    esp_err_t cloud_provision_apply(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CLOUD_PROVISION_H */
