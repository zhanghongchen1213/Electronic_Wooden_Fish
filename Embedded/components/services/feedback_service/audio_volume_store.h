/**
 * @file     audio_volume_store.h
 * @brief    音量持久化存储适配接口。
 * @details  音量记录以单一 namespace + schema_version + 单次提交为事务边界
 *           （契约 §11.1 device 作用域设置存储的首块，nvs_set_* 在 commit 前不落盘）。
 *           固件侧由 audio_volume_store_nvs.c 以 NVS 实现；主机测试由测试替身实现，
 *           记录提交与写集供恢复序列回归。业务代码只依赖本接口。
 *           Story 2.4 落设置存储时必须收敛到同一真源，不得建立第二个音量持久化位。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_AUDIO_VOLUME_STORE_H
#define EWF_AUDIO_VOLUME_STORE_H

#include <stdbool.h>

#include "esp_err.h"
#include "feedback_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 存储加载结果状态。 */
typedef enum {
    EWF_AUDIO_STORE_OK = 0,  /**< 已完整读出持久化音量记录。 */
    EWF_AUDIO_STORE_EMPTY,   /**< 首次启动：schema 标记不存在，按默认 50 初始化。 */
    EWF_AUDIO_STORE_ERROR,   /**< 介质错误：调用方必须 fail-closed（报错 + 保持内存值）。 */
} ewf_audio_store_status_t;

/**
 * @brief 读取持久化音量记录
 * @param record 记录输出；EMPTY 时输出按零值初始化的结果
 * @param status 加载状态输出
 * @return ESP_OK 已取得结论（含 EMPTY），其他值表示参数非法
 */
esp_err_t ewf_audio_volume_store_load(ewf_feedback_volume_record_t *record,
                                      ewf_audio_store_status_t *status);

/**
 * @brief 以单次提交为事务边界写入完整音量记录
 * @details 组内全部字段落在同一 namespace，一次提交；禁止拆分介质或多次提交。
 * @param record 待写入记录
 * @return ESP_OK 已提交，其他值表示参数非法或介质错误（busy/error 必须上抛）
 */
esp_err_t ewf_audio_volume_store_save(const ewf_feedback_volume_record_t *record);

#ifdef __cplusplus
}
#endif

#endif /* EWF_AUDIO_VOLUME_STORE_H */
