/**
 * @file     sync_config.h
 * @brief    同步客户端传输与身份配置（编译期/可注入）。
 * @details  默认 mock；真机 Air780 需显式切换。URL/Token 日志脱敏。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_SYNC_CONFIG_H
#define EWF_SYNC_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EWF_SYNC_TRANSPORT_MOCK
#define EWF_SYNC_TRANSPORT_MOCK 1
#endif

#ifndef EWF_SYNC_TRANSPORT_AIR780
#define EWF_SYNC_TRANSPORT_AIR780 0
#endif

#if EWF_SYNC_TRANSPORT_MOCK && EWF_SYNC_TRANSPORT_AIR780
#error "EWF_SYNC_TRANSPORT_MOCK 与 EWF_SYNC_TRANSPORT_AIR780 互斥"
#endif

#ifndef EWF_SYNC_HTTPS_URL
#define EWF_SYNC_HTTPS_URL "https://example.invalid/api/v1/sync/report"
#endif

#ifndef EWF_SYNC_DEVICE_TOKEN
#define EWF_SYNC_DEVICE_TOKEN ""
#endif

#ifndef EWF_SYNC_DEVICE_ID_DEFAULT
#define EWF_SYNC_DEVICE_ID_DEFAULT "ewf-unconfigured"
#endif

#ifndef EWF_SYNC_FIRMWARE_VERSION_DEFAULT
#define EWF_SYNC_FIRMWARE_VERSION_DEFAULT "0.0.0-dev"
#endif

#ifndef EWF_SYNC_AUDIO_CONFIG_VERSION_DEFAULT
#define EWF_SYNC_AUDIO_CONFIG_VERSION_DEFAULT 1U
#endif

#ifdef __cplusplus
}
#endif

#endif /* EWF_SYNC_CONFIG_H */
