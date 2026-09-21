idf_build_get_property(target IDF_TARGET)
idf_build_get_property(python PYTHON)

if(${target} STREQUAL "linux")
    return()
endif()

set(IDF_ESP_PM_DIR "$ENV{IDF_PATH}/components/esp_pm")
set(PATCHED_PM_IMPL "${CMAKE_CURRENT_BINARY_DIR}/pm_impl.c")

execute_process(
    COMMAND
        ${python}
        ${CMAKE_CURRENT_LIST_DIR}/generate_v554_pm_impl.py
        --source ${IDF_ESP_PM_DIR}/pm_impl.c
        --output ${PATCHED_PM_IMPL}
    RESULT_VARIABLE patch_result
    OUTPUT_VARIABLE patch_output
    ERROR_VARIABLE patch_error
)
if(NOT patch_result EQUAL 0)
    message(FATAL_ERROR
        "ESP-IDF v5.5.4 light sleep 修复回移失败：${patch_error}${patch_output}")
endif()

set(priv_requires esp_system esp_driver_gpio esp_timer)
set(srcs
    "${IDF_ESP_PM_DIR}/pm_locks.c"
    "${IDF_ESP_PM_DIR}/pm_trace.c"
    "${PATCHED_PM_IMPL}"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "${IDF_ESP_PM_DIR}/include"
    PRIV_REQUIRES "${priv_requires}"
    LDFRAGMENTS "${IDF_ESP_PM_DIR}/linker.lf"
)

message(STATUS
    "已启用 ESP-IDF v5.5.4 light sleep 修复回移：拒绝休眠不补 tick，超时 2 tick 内有界补偿")
