# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/components
)

set(PROJECT_VER "0.1.0" CACHE STRING "电子木鱼固件版本" FORCE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(electronic_wooden_fish)

message(
    STATUS
    "电子木鱼固件：PROJECT_VER=${PROJECT_VER}，target=esp32s3，仅纳管 EWF 启动图（power/state/selftest）；"
    "BLE/GPS/ML307R/旧业务云/voice/audio 不在默认启动前提内"
)
