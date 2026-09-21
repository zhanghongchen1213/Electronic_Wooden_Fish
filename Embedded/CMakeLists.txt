# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/components
)

# 手环固件版本唯一维护入口；发布新镜像前手动更新该值
# 非景区版本 v1.x.x
# 景区版本 v2.x.x
set(LEGBOT_DEFAULT_PROJECT_VER "v1.0.0")
set(_legbot_effective_project_ver "${LEGBOT_DEFAULT_PROJECT_VER}")
if(DEFINED LEGBOT_PROJECT_VER_OVERRIDE AND
   NOT "${LEGBOT_PROJECT_VER_OVERRIDE}" STREQUAL "")
    set(_legbot_effective_project_ver "${LEGBOT_PROJECT_VER_OVERRIDE}")
endif()
if(NOT "${_legbot_effective_project_ver}" MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
        "手环固件版本必须是 vX.Y.Z，实际为 ${_legbot_effective_project_ver}")
endif()
# PROJECT_VER 是 ESP-IDF 消费的最终值；FORCE 防止复用构建目录时沿用旧 profile。
set(PROJECT_VER "${_legbot_effective_project_ver}"
    CACHE STRING "手环固件版本" FORCE)

set(_legbot_scenic_mode_override_defined FALSE)
set(_legbot_scenic_mode_override_value "")
if(DEFINED SCENIC_AREA_MANAGEMENT_DEBUG)
    set(_legbot_scenic_mode_override_defined TRUE)
    set(_legbot_scenic_mode_override_value "${SCENIC_AREA_MANAGEMENT_DEBUG}")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/resolve_firmware_profile.cmake)
legbot_resolve_scenic_area_management_mode(
    "${PROJECT_VER}"
    "${_legbot_scenic_mode_override_defined}"
    "${_legbot_scenic_mode_override_value}"
    _legbot_scenic_mode
    _legbot_scenic_mode_source
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
idf_build_set_property(
    COMPILE_DEFINITIONS
    "SCENIC_AREA_MANAGEMENT_DEBUG=${_legbot_scenic_mode}"
    APPEND
)
idf_build_set_property(COMPILE_DEFINITIONS "LEGBOT_CAP_BLE=1" APPEND)
idf_build_set_property(
    COMPILE_DEFINITIONS
    "LEGBOT_CAP_GPS_TIME=${_legbot_scenic_mode}"
    APPEND
)
idf_build_set_property(
    COMPILE_DEFINITIONS
    "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION=${_legbot_scenic_mode}"
    APPEND
)
idf_build_set_property(
    COMPILE_DEFINITIONS
    "LEGBOT_CAP_MODEM=${_legbot_scenic_mode}"
    APPEND
)
idf_build_set_property(
    COMPILE_DEFINITIONS
    "LEGBOT_CAP_CLOUD=${_legbot_scenic_mode}"
    APPEND
)
foreach(_legbot_exoskeleton_version RANGE 1 9)
    set(_legbot_scene_config_name
        "WATCH_EXOSKELETON_V${_legbot_exoskeleton_version}_SCENE_CONFIG")
    if(DEFINED ${_legbot_scene_config_name})
        idf_build_set_property(
            COMPILE_DEFINITIONS
            "${_legbot_scene_config_name}=${${_legbot_scene_config_name}}"
            APPEND
        )
    endif()
endforeach()
project(legbot_watch)

message(
    STATUS
    "固件编译模式：PROJECT_VER=${PROJECT_VER}，来源=${_legbot_scenic_mode_source}，"
    "SCENIC_AREA_MANAGEMENT_DEBUG=${_legbot_scenic_mode}，GPS授时=${_legbot_scenic_mode}，"
    "GPS持续定位=${_legbot_scenic_mode}，4G=${_legbot_scenic_mode}，云端=${_legbot_scenic_mode}"
)
