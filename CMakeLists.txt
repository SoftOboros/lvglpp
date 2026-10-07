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

# Preserve the minimal DISCO/Generic build by default. Consumers can opt
# into the wrapper libraries with their own freestanding configuration.
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(_lvglpp_libraries_default OFF)
else()
    set(_lvglpp_libraries_default ON)
endif()
option(LVGLPP_BUILD_LIBRARIES "Build LVGL and the lvglpp wrapper libraries"
    ${_lvglpp_libraries_default})

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
# Generic board examples retain their minimal default. Library builds
# are independently selectable, including with a cross toolchain.
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Generic")
    set(LVGLPP_HOST_BUILD OFF)
else()
    set(LVGLPP_HOST_BUILD ON)
endif()

if(CMAKE_CROSSCOMPILING AND LVGLPP_BUILD_LIBRARIES AND NOT LVGLPP_EMBEDDED_POSTURE)
    message(FATAL_ERROR
        "Cross-built lvglpp libraries require LVGLPP_EMBEDDED_POSTURE=ON.")
endif()

# ---------------------------------------------------------------------------
# LVGL submodule (the C library we wrap).
# ---------------------------------------------------------------------------
if(LVGLPP_BUILD_LIBRARIES)
    set(LVGLPP_LVGL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lvgl"
        CACHE PATH "Path to the LVGL C source tree (default: top-level submodule)")

    if(NOT EXISTS "${LVGLPP_LVGL_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "LVGL submodule not initialized at ${LVGLPP_LVGL_DIR}.\n"
            "Run: git submodule update --init lvgl")
    endif()

    # Honor upstream configuration inputs from either -D or a parent
    # project. A non-cache fallback avoids pinning the bundled header in
    # the cache, so later reconfigures can select LV_BUILD_CONF_DIR too.
    # Declare typed entries first: upstream's older CMake policy scope
    # would otherwise remove a normal variable when creating its cache entry.
    set(LV_BUILD_CONF_PATH "${LV_BUILD_CONF_PATH}" CACHE FILEPATH "Path to a consumer lv_conf.h")
    set(LV_BUILD_CONF_DIR "${LV_BUILD_CONF_DIR}" CACHE PATH "Directory containing a consumer lv_conf.h")
    if(NOT LV_BUILD_CONF_PATH AND NOT LV_BUILD_CONF_DIR AND NOT LV_BUILD_USE_KCONFIG)
        set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/lvglpp/lv_conf.h")
    endif()

    # A library-only build must not pull in upstream demos/examples
    # through LVGL's public link interface. Explicit consumer settings
    # still take precedence over these defaults.
    if(NOT LVGLPP_BUILD_EXAMPLES)
        foreach(_lvglpp_upstream_option CONFIG_LV_BUILD_EXAMPLES CONFIG_LV_BUILD_DEMOS)
            if(NOT DEFINED ${_lvglpp_upstream_option})
                set(${_lvglpp_upstream_option} OFF)
            endif()
            set(${_lvglpp_upstream_option} "${${_lvglpp_upstream_option}}"
                CACHE BOOL "Build upstream LVGL examples/demos")
        endforeach()
    endif()

    add_subdirectory("${LVGLPP_LVGL_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/lvgl" EXCLUDE_FROM_ALL)
endif()

# Enable testing before adding modules so that per-module tests/
# subdirectories can register ctest entries via add_test(). Cross
# builds skip ctest entirely. Do not rewrite consumer cache preferences.
if(CMAKE_CROSSCOMPILING OR NOT LVGLPP_BUILD_LIBRARIES)
    set(LVGLPP_BUILD_TESTS OFF)
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
# Per-module libraries — selectable independently of tests and examples.
# ---------------------------------------------------------------------------
if(LVGLPP_BUILD_LIBRARIES)
    # CORE-07n §5.3: in-repo example consumers (disco-demo icons,
    # widget gallery) force the RLE plugin on; the bare library
    # build stays plugin-free.
    if(LVGLPP_BUILD_EXAMPLES)
        set(LVGLPP_CORE_RLE ON)
    endif()
    add_subdirectory(core)
    add_subdirectory(widgets)
    add_subdirectory(ui)
    add_subdirectory(platform)
    add_subdirectory(playit)
    add_subdirectory(i18n)

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
    # DEMO-02: lvglpp_ui is now a compiled STATIC library (was INTERFACE),
    # so posture flags apply to its own translation units via PUBLIC.
    target_link_libraries(lvglpp_ui        PUBLIC    lvglpp_posture)
    target_link_libraries(lvglpp_i18n      PUBLIC    lvglpp_posture)

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
