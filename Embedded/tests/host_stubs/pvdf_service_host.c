/**
 * @file     pvdf_service_host.c
 * @brief    统一敲击服务主机测试的平台替身。
 * @details  只替代 FreeRTOS/硬件边界，业务代码仍使用真实生产实现。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "../../components/services/pvdf_input_service/pvdf_input_service.c"

/* 将 ADC 策略终态交给真实 production 转投函数，不重写转换或 submit。 */
void host_pvdf_terminal(ewf_pvdf_event_kind_t kind, uint32_t at_ms)
{
    const ewf_pvdf_event_t event = {.kind = kind, .at_ms = at_ms};
    publish_terminal(&event, at_ms);
}

void host_pvdf_reset_sequence(void)
{
    s_candidate_sequence = 0U;
}
