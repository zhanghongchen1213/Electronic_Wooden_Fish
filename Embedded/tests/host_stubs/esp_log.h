/**
 * @file     esp_log.h
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_HOST_ESP_LOG_H
#define EWF_HOST_ESP_LOG_H
#include <stdio.h>
#define ESP_LOGE(tag, ...) ((void)(tag), (void)fprintf(stderr, __VA_ARGS__))
#define ESP_LOGW(tag, ...) ((void)(tag), (void)fprintf(stderr, __VA_ARGS__))
#define ESP_LOGI(tag, ...) ((void)(tag), (void)fprintf(stderr, __VA_ARGS__))
#define ESP_LOGD(tag, ...) ((void)(tag), (void)fprintf(stderr, __VA_ARGS__))
#endif
