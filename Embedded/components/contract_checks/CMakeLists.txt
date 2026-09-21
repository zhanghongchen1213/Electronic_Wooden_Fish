idf_component_register(
    SRCS "contract_checks.c"
    INCLUDE_DIRS "."
    REQUIRES
        BSP
        platform
        app_state
        services
        ui
)
