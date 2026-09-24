# Story 1.1：services 组件只纳管 EWF 启动图中的服务。
# BLE/GPS/ML307R/cloud/voice/audio/time/maintenance/radio_power 旧实现已移除。
# Story 3.1 重新登记 EWF ui_service（独占 ui_task），与 legbot 旧 UI 服务不同。
idf_component_register(
    SRCS
        "legbot_services.c"
        "power_service/power_boot_policy.c"
        "power_service/power_auto_mode_policy.c"
        "power_service/power_core_path_policy.c"
        "power_service/power_service.c"
        "progress_service/progress_transaction.c"
        "progress_service/progress_store_nvs.c"
        "progress_service/today_bucket_policy.c"
        "progress_service/today_bucket_store_nvs.c"
        "progress_service/fault_gate_policy.c"
        "progress_service/progress_service.c"
        "feedback_service/feedback_policy.c"
        "feedback_service/audio_volume_store_nvs.c"
        "feedback_service/feedback_audio_bsp.c"
        "feedback_service/feedback_service.c"
        "device_nav_service/device_nav_policy.c"
        "device_nav_service/device_settings_policy.c"
        "device_nav_service/device_settings_store_nvs.c"
        "device_nav_service/device_nav_service.c"
        "sync_service/sync_window_policy.c"
        "sync_service/sync_response_policy.c"
        "sync_service/sync_https_codec.c"
        "sync_service/sync_https_mock.c"
        "sync_service/sync_service.c"
        "pvdf_input_service/pvdf_confirm_policy.c"
        "pvdf_input_service/pvdf_input_service.c"
        "tap_input_service/tap_input_policy.c"
        "tap_input_service/tap_input_service.c"
        "selftest_service/selftest_service.c"
        "state_service/state_service.c"
        "ui_service/ui_service.c"
    INCLUDE_DIRS
        "."
        "power_service"
        "progress_service"
        "feedback_service"
        "device_nav_service"
        "sync_service"
        "pvdf_input_service"
        "tap_input_service"
        "selftest_service"
        "state_service"
        "ui_service"
    REQUIRES
        BSP
        app_state
        esp_driver_gpio
        esp_timer
        freertos
        heap
        nvs_flash
        platform
        spiffs
        ui
        lvgl
)

# canonical 生成头（ewf_scripture_canonical.h）位于仓库 docs 契约目录；
# 只读包含不复制，保证 scripture 版本与可消费总数的单一真源。
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../docs/contracts/canonical/generated"
)
