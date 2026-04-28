cmake_minimum_required(VERSION 3.20)
project(lvglpp
    VERSION 0.1.0
    DESCRIPTION "Modern C++ wrapper around LVGL with strict and explicit ownership"
    LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Project-wide policy
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(LVGLPP_BUILD_TESTS    "Build lvglpp tests"     ON)
option(LVGLPP_BUILD_EXAMPLES "Build lvglpp examples"  ON)
option(LVGLPP_USE_RLVGL      "Expose rlvgl reference paths to consumers" OFF)

# PLAT-02 STM32H747I-DISCO target. Requires the arm-none-eabi
# cross-toolchain (cmake/toolchains/arm-none-eabi.cmake); see
# examples/stm32h747i-disco/CMakeLists.txt for the gate. Default OFF
# so host builds are unchanged.
option(LVGLPP_PLATFORM_DISCO "Build STM32H747I-DISCO targets" OFF)

# Mirror rlvgl's no_std + panic=abort posture. When ON:
#   - exceptions and RTTI are disabled across all lvglpp targets;
#   - LVGLPP_EMBEDDED_POSTURE is defined as a compile-time symbol;
#   - throwing constructors (e.g. Runtime{}) call std::abort() instead;
#   - host-only convenience APIs are unavailable.
# Cross-build / firmware targets MUST set this ON; host smoke tests
# leave it OFF to keep ergonomic C++ exception flow.
option(LVGLPP_EMBEDDED_POSTURE
    "Mirror rlvgl no_std + panic=abort: -fno-exceptions -fno-rtti, abort() on panic"
    OFF)

# ---------------------------------------------------------------------------
# Warnings — strict-and-explicit-ownership demands strict diagnostics
# ---------------------------------------------------------------------------
add_library(lvglpp_warnings INTERFACE)
target_compile_options(lvglpp_warnings INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Wold-style-cast
        -Woverloaded-virtual -Wconversion -Wsign-conversion
        -Wnull-dereference -Wdouble-promotion -Wformat=2
        -Wimplicit-fallthrough
    >
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:
        /W4 /permissive-
    >
)

# Embedded-posture interface: linked into every lvglpp module so the
# flag-set propagates without per-target boilerplate.
add_library(lvglpp_posture INTERFACE)
if(LVGLPP_EMBEDDED_POSTURE)
    target_compile_definitions(lvglpp_posture INTERFACE LVGLPP_EMBEDDED_POSTURE=1)
    target_compile_options(lvglpp_posture INTERFACE
        $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>:
            -fno-exceptions -fno-rtti
        >
    )
endif()

# ---------------------------------------------------------------------------
# Host vs. cross. The lvglpp module set + lvgl submodule + ctest are all
# host-targeted today; PLAT-02a's cross build is deliberately minimal
# (just the disco example links the warnings + posture INTERFACE libs).
# Subsequent PLAT-02 sub-phases will grow the cross surface as they
# need core/playit on-target.
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(LVGLPP_HOST_BUILD OFF)
else()
    set(LVGLPP_HOST_BUILD ON)
endif()

# ---------------------------------------------------------------------------
# LVGL submodule (the C library we wrap) — host-only today.
# ---------------------------------------------------------------------------
if(LVGLPP_HOST_BUILD)
    set(LVGLPP_LVGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lvgl"
        CACHE PATH "Path to the LVGL C source tree (default: top-level submodule)")

    if(NOT EXISTS "${LVGLPP_LVGL_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "LVGL submodule not initialized at ${LVGLPP_LVGL_DIR}.\n"
            "Run: git submodule update --init lvgl")
    endif()

    # Provide an lv_conf.h before adding LVGL. LVGL's CMake reads
    # LV_BUILD_CONF_PATH (absolute path to the config header) and
    # LV_BUILD_CONF_DIR (directory holding lv_conf.h, used with simple include).
    set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/lvglpp/lv_conf.h"
        CACHE FILEPATH "Path to lv_conf.h" FORCE)

    add_subdirectory(${LVGLPP_LVGL_DIR} ${CMAKE_BINARY_DIR}/_deps/lvgl EXCLUDE_FROM_ALL)
endif()

# Enable testing before adding modules so that per-module tests/
# subdirectories can register ctest entries via add_test(). Cross
# builds skip ctest entirely.
if(NOT LVGLPP_HOST_BUILD)
    set(LVGLPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif()
if(LVGLPP_BUILD_TESTS)
    enable_testing()
endif()

# ---------------------------------------------------------------------------
# rlvgl reference (Rust crate; not built here, only referenced for parity work)
# ---------------------------------------------------------------------------
set(LVGLPP_RLVGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rlvgl"
    CACHE PATH "Path to the rlvgl Rust source tree (parallel reference)")

if(LVGLPP_USE_RLVGL AND NOT EXISTS "${LVGLPP_RLVGL_DIR}/Cargo.toml")
    message(WARNING
        "rlvgl submodule not initialized at ${LVGLPP_RLVGL_DIR}.\n"
        "Run: git submodule update --init rlvgl")
endif()

# ---------------------------------------------------------------------------
# Top-level umbrella include path (lvglpp/lvglpp.hpp meta-umbrella +
# lvglpp/std/expected.hpp seam header)
# ---------------------------------------------------------------------------
add_library(lvglpp_umbrella_includes INTERFACE)
target_include_directories(lvglpp_umbrella_includes
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# Vendored expected polyfill — selects between <expected> and the
# vendored fallback at compile time via the seam header above.
# ---------------------------------------------------------------------------
add_library(lvglpp_expected INTERFACE)
add_library(lvglpp::expected ALIAS lvglpp_expected)
target_include_directories(lvglpp_expected
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/lvglpp_expected/include>
        $<INSTALL_INTERFACE:include>
)
# The seam header lives under include/lvglpp/std/, so it ships with the
# umbrella include path. Modules pull both via lvglpp::expected and
# their own include/.
target_link_libraries(lvglpp_expected
    INTERFACE
        lvglpp_umbrella_includes
)

# ---------------------------------------------------------------------------
# Per-module libraries — host-targeted today (each depends transitively on
# lvgl). PLAT-02a's cross build skips them; the smoke target only links
# lvglpp_warnings + lvglpp_posture. Subsequent sub-phases will land
# embedded-clean cross paths for the modules that need them on-target.
# ---------------------------------------------------------------------------
if(LVGLPP_HOST_BUILD)
    add_subdirectory(core)
    add_subdirectory(widgets)
    add_subdirectory(ui)
    add_subdirectory(platform)
    add_subdirectory(playit)

    # Aggregate INTERFACE alias — the surface a typical consumer links.
    add_library(lvglpp INTERFACE)
    add_library(lvglpp::lvglpp ALIAS lvglpp)
    target_link_libraries(lvglpp
        INTERFACE
            lvglpp::core
            lvglpp::widgets
            lvglpp::ui
            lvglpp::platform
            lvglpp::playit
            lvglpp_umbrella_includes
    )
    target_compile_features(lvglpp INTERFACE cxx_std_20)

    # Wire posture into every module. The compiled libs use PUBLIC; the
    # INTERFACE-only umbrellas use INTERFACE. lvglpp_platform switches
    # shape based on whether any backend is selected (see
    # platform/CMakeLists.txt) — branch the link accordingly.
    target_link_libraries(lvglpp_core      PUBLIC    lvglpp_posture)
    target_link_libraries(lvglpp_playit    PUBLIC    lvglpp_posture)
    target_link_libraries(lvglpp_widgets   PUBLIC    lvglpp_posture)
    target_link_libraries(lvglpp_ui        INTERFACE lvglpp_posture)

    get_target_property(_lvglpp_platform_type lvglpp_platform TYPE)
    if(_lvglpp_platform_type STREQUAL "INTERFACE_LIBRARY")
        target_link_libraries(lvglpp_platform INTERFACE lvglpp_posture)
    else()
        target_link_libraries(lvglpp_platform PUBLIC    lvglpp_posture)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests / examples
# ---------------------------------------------------------------------------
if(LVGLPP_BUILD_TESTS AND LVGLPP_HOST_BUILD)
    add_subdirectory(tests)
endif()

if(LVGLPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
