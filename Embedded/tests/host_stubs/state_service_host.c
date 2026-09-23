/**
 * @file     state_service_host.c
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "../../components/services/state_service/state_service.c"

/* 只控制任务句柄并驱动真实 apply switch；不替换生产 gate 或状态存储。 */
void host_state_owner_start(void)
{
    atomic_store(&s_owner_task, xTaskGetCurrentTaskHandle());
}

void host_state_owner_stop(void)
{
    atomic_store(&s_owner_task, NULL);
}

esp_err_t host_state_apply_one(void)
{
    state_service_update_t update = {0};
    if (xQueueReceive(legbot_state_service_queue(), &update, 0) != pdTRUE)
    {
        return ESP_ERR_NOT_FOUND;
    }
    return apply_update(&update);
}
