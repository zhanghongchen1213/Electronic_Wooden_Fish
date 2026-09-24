/**
 * @file     device_settings_store.h
 * @brief    契约 §11.1 device 作用域设置存储适配接口。
 * @details  单一 namespace settings + schema_ver + 单次 commit；字段恰为
 *           volume/brightness/timeout/applied_revision。固件 NVS 实现；
 *           主机测试替身。是音量唯一可写持久化真源（收敛 Story 2.3 audio）。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_DEVICE_SETTINGS_STORE_H
#define EWF_DEVICE_SETTINGS_STORE_H

#include "device_settings_policy.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EWF_SETTINGS_STORE_OK = 0,    /**< 已完整读出。 */
    EWF_SETTINGS_STORE_EMPTY,     /**< 首启缺字段，调用方用默认并落盘。 */
    EWF_SETTINGS_STORE_ERROR,     /**< 介质错误，fail-closed。 */
} ewf_settings_store_status_t;

/**
 * @brief 读取设备设置；必要时一次性迁移旧 audio namespace 音量
 * @param record 输出记录
 * @param status 加载状态
 * @return ESP_OK 已取得结论（含 EMPTY），其他值表示参数非法
 */
esp_err_t ewf_device_settings_store_load(ewf_device_settings_record_t *record,
                                         ewf_settings_store_status_t *status);

/**
 * @brief 以单次 commit 写入完整设置记录
 * @param record 待写入记录
 * @return ESP_OK 已提交
 */
esp_err_t ewf_device_settings_store_save(
    const ewf_device_settings_record_t *record);

#ifdef __cplusplus
}
#endif

#endif /* EWF_DEVICE_SETTINGS_STORE_H */
