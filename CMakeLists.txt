cmake_minimum_required(VERSION 3.16)

project(sceneswitch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external/fmt)
add_subdirectory(external/tomlplusplus)
add_subdirectory(external/CLI11)

set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)
add_subdirectory(external/spdlog)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED xcb)
pkg_check_modules(XCB REQUIRED xcb-randr)
pkg_check_modules(XCB_EWMH REQUIRED xcb-ewmh)

add_executable(sceneswitch src/main.cpp)

target_link_libraries(sceneswitch
    PRIVATE
        fmt::fmt
        spdlog::spdlog
        tomlplusplus::tomlplusplus
        ${XCB_LIBRARIES}
        ${XCB_EWMH_LIBRARIES}
)

target_include_directories(sceneswitch PRIVATE ${XCB_INCLUDE_DIRS} ${XCB_EWMH_INCLUDE_DIRS})
target_compile_options(sceneswitch PRIVATE ${XCB_CFLAGS_OTHER} ${XCB_EWMH_CFLAGS_OTHER})