/**
 * @file     log_service.h
 * @brief    关键日志服务公共接口
 * @details  声明日志服务在服务仲裁层中的固定服务 ID，后续关键事件环形日志通过该接口接入统一服务框架。
 * @author   ZHC
 * @date     2026-07-09
 */

#ifndef LEGBOT_LOG_SERVICE_H
#define LEGBOT_LOG_SERVICE_H

#include "legbot_services.h"

/** 日志服务在统一服务表中的固定 ID。 */
#define LEGBOT_LOG_SERVICE_ID LEGBOT_SERVICE_LOG

#endif /* LEGBOT_LOG_SERVICE_H */
