# Story 1.1：services 组件只纳管 EWF 启动图中的服务。
# BLE/GPS/ML307R/cloud/voice/audio/ui_service/time/maintenance/radio_power 旧实现
# 已从默认启动图移除（目录一并清除），避免通过“保留但不编译”掩盖未审计代码。
idf_component_register(
    SRCS
        "legbot_services.c"
        "power_service/power_boot_policy.c"
        "power_service/power_service.c"
        "pvdf_input_service/pvdf_confirm_policy.c"
        "pvdf_input_service/pvdf_input_service.c"
        "tap_input_service/tap_input_policy.c"
        "tap_input_service/tap_input_service.c"
        "selftest_service/selftest_service.c"
        "state_service/state_service.c"
    INCLUDE_DIRS
        "."
        "power_service"
        "pvdf_input_service"
        "tap_input_service"
        "selftest_service"
        "state_service"
    REQUIRES
        BSP
        app_state
        esp_driver_gpio
        esp_timer
        freertos
        heap
        platform
)
