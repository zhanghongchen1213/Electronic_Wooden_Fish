idf_component_register(
    SRCS "contract_checks.c"
    INCLUDE_DIRS "."
    REQUIRES
        BSP
        app_state
        control_gate
        services
        ui
)

