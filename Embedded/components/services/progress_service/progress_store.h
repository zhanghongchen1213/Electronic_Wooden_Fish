/**
 * @file     progress_store.h
 * @brief    高水位/轮次事务组的持久化存储适配接口。
 * @details  事务组以单一 namespace + 单次提交为事务边界（契约 §11 设备侧对齐）。
 *           固件侧由 progress_store_nvs.c 以 NVS 实现；主机测试由测试替身实现，
 *           记录提交次数与写集供恢复序列回归。业务代码只依赖本接口。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_PROGRESS_STORE_H
#define EWF_PROGRESS_STORE_H

#include <stdbool.h>

#include "esp_err.h"
#include "progress_transaction.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 存储加载结果状态。 */
typedef enum {
    EWF_PROGRESS_STORE_OK = 0, /**< 已完整读出持久化事务组。 */
    EWF_PROGRESS_STORE_EMPTY,  /**< 首次启动：schema 标记不存在，按零值初始化。 */
    EWF_PROGRESS_STORE_ERROR,  /**< 介质错误：调用方必须 fail-closed。 */
} ewf_progress_store_status_t;

/**
 * @brief 读取持久化事务组
 * @param tx 事务组输出；EMPTY 时输出按零值初始化的结果
 * @param status 加载状态输出
 * @return ESP_OK 已取得结论（含 EMPTY），其他值表示参数非法
 */
esp_err_t progress_store_load(ewf_progress_transaction_t *tx,
                              ewf_progress_store_status_t *status);

/**
 * @brief 以单次提交为事务边界写入完整事务组
 * @details 组内全部字段落在同一 namespace，一次提交；禁止拆分介质或多次提交。
 * @param tx 待写入事务组
 * @return ESP_OK 已提交，其他值表示参数非法或介质错误（busy/error 必须上抛）
 */
esp_err_t progress_store_save(const ewf_progress_transaction_t *tx);

#ifdef __cplusplus
}
#endif

#endif /* EWF_PROGRESS_STORE_H */
