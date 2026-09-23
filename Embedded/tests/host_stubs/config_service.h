/**
 * @file     config_service.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_CONFIG_SERVICE_H
#define EWF_HOST_CONFIG_SERVICE_H
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
esp_err_t config_service_retry_pending_change(void);
esp_err_t config_service_cloud_offset_write(int64_t offset, TickType_t timeout);
#endif
