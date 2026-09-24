file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/generated/filelist.txt" SQUARELINE_GENERATED_SRCS)
list(REMOVE_ITEM SQUARELINE_GENERATED_SRCS "ui_events.c")
set(SQUARELINE_COMPONENT_SRCS "")
foreach(SQUARELINE_SOURCE IN LISTS SQUARELINE_GENERATED_SRCS)
    list(APPEND SQUARELINE_COMPONENT_SRCS "generated/${SQUARELINE_SOURCE}")
endforeach()

idf_component_register(
    SRCS
        "bindings/ui_shell_policy.c"
        "bindings/ui_manifest_projection.c"
        "bindings/ui_runtime_binding.c"
        "bindings/ui_runtime_events.c"
        "bindings/ui_muyu_belt_policy.c"
        "bindings/ui_scripture_display_expand.c"
        "bindings/ui_jingwen_stream_policy.c"
        "bindings/ui_jingwen_gesture_policy.c"
        "bindings/ui_jingwen_projection.c"
        "bindings/ui_tap_rings_policy.c"
        "bindings/ui_muyu_projection.c"
        "bindings/ui_muyu_done_policy.c"
        "bindings/ui_tongji_stats_policy.c"
        "bindings/ui_tongji_projection.c"
        "bindings/ui_shezhi_settings_policy.c"
        "bindings/ui_shezhi_projection.c"
        "generated/ui_events.c"
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

# canonical 经文头只读 include（字带唯一经文源）。
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../docs/contracts/canonical/generated"
)
