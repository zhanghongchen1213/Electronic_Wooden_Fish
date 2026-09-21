# 根据固件版本和可选覆盖值解析景区管理编译模式。
function(
    legbot_resolve_scenic_area_management_mode
    project_version
    override_defined
    override_value
    output_mode
    output_source
)
    if(NOT "${project_version}" MATCHES
       "^v(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
        message(
            FATAL_ERROR
            "PROJECT_VER 必须是 v<major>.<minor>.<patch>，当前值=${project_version}"
        )
    endif()
    set(_project_major "${CMAKE_MATCH_1}")

    if(override_defined AND NOT "${override_value}" STREQUAL "")
        if(NOT "${override_value}" MATCHES "^[01]$")
            message(
                FATAL_ERROR
                "SCENIC_AREA_MANAGEMENT_DEBUG 仅允许 0、1 或空值，当前值=${override_value}"
            )
        endif()
        if(_project_major STREQUAL "1" AND "${override_value}" STREQUAL "1")
            message(
                FATAL_ERROR
                "PROJECT_VER=${project_version} 禁止启用 4G/GPS/cloud 景区能力；v1.x.x 必须保持 "
                "SCENIC_AREA_MANAGEMENT_DEBUG=0"
            )
        endif()
        set(_resolved_mode "${override_value}")
        set(_resolved_source "显式覆盖")
    else()
        if(_project_major STREQUAL "1")
            set(_resolved_mode "0")
        elseif(_project_major STREQUAL "2")
            set(_resolved_mode "1")
        else()
            message(
                FATAL_ERROR
                "PROJECT_VER=${project_version} 尚未定义景区管理模式默认值；"
                "请显式设置 -DSCENIC_AREA_MANAGEMENT_DEBUG=0 或 1"
            )
        endif()
        set(_resolved_source "PROJECT_VER v${_project_major} 默认值")
    endif()

    set("${output_mode}" "${_resolved_mode}" PARENT_SCOPE)
    set("${output_source}" "${_resolved_source}" PARENT_SCOPE)
endfunction()
