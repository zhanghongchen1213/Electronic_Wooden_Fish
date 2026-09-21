file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/generated/filelist.txt" SQUARELINE_GENERATED_SRCS)
list(REMOVE_ITEM SQUARELINE_GENERATED_SRCS "ui_events.c")
set(SQUARELINE_COMPONENT_SRCS "")
foreach(SQUARELINE_SOURCE IN LISTS SQUARELINE_GENERATED_SRCS)
    list(APPEND SQUARELINE_COMPONENT_SRCS "generated/${SQUARELINE_SOURCE}")
endforeach()

idf_component_register(
    SRCS
        "ui_navigation.c"
        "ui_selftest_summary.c"
        "bindings/control_ui_binding.c"
        "bindings/ui_manifest_projection.c"
        "bindings/ui_runtime_binding.c"
        "bindings/ui_runtime_events.c"
        "bindings/ui_touch_session.c"
        ${SQUARELINE_COMPONENT_SRCS}
    INCLUDE_DIRS
        "."
        "bindings"
        "compat"
        "generated"
    REQUIRES
        app_state
        heap
        log
        lvgl
        esp_timer
)
