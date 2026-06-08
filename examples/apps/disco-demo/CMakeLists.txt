# lvglpp::app_disco_demo — disco-demo composite-widget app module (DEMO-05).
#
# PARITY: rlvgl crate rlvgl-app-disco-demo
#         (rlvgl/examples/apps/disco-demo). Platform-independent: the four
#         composite widgets + the consume-only icon-asset pipeline.
# DELTA:  icon byte arrays are generated at configure time from the rlvgl
#         `.rle` assets (mirrors core/'s font_6x10.bin -> .inc pattern).
#
# docs/disco-demo/05-composite-widgets.md.

# ---------------------------------------------------------------------------
# Icon-asset pipeline (DEMO-05 §3) — generate disco_demo_assets.inc from the
# rlvgl `.rle` files at configure time. Consume-only; FAIL LOUDLY if absent.
# ---------------------------------------------------------------------------
set(LVGLPP_DISCO_DEMO_ICON_DIR
    "${CMAKE_SOURCE_DIR}/rlvgl/examples/stm32h747i-disco/assets/icons")

# (symbol|relative-path) for each icon named in assets.rs:18-45.
set(_lvglpp_disco_icons
    "ICON_SETTINGS|settings.rle"
    "ICON_FILE|file.rle"
    "ICON_INFO|info.rle"
    "ICON_AUDIO_48|48/audio48.rle"
    "ICON_CAMERA_48|48/camera48.rle"
    "ICON_MONITOR_48|48/monitor48.rle"
    "ICON_GLOBE_48|48/globe48.rle"
    "ICON_BUG_48|48/bug48.rle"
    "ICON_CPU_48|48/cpu48.rle"
    "ICON_PLAY_48|48/play48.rle"
)

set(_lvglpp_disco_inc
    "${CMAKE_CURRENT_BINARY_DIR}/generated/disco_demo_assets.inc")
set(_lvglpp_disco_body "")
foreach(_entry ${_lvglpp_disco_icons})
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _sym)
    list(GET _parts 1 _rel)
    set(_path "${LVGLPP_DISCO_DEMO_ICON_DIR}/${_rel}")
    if(NOT EXISTS "${_path}")
        message(FATAL_ERROR
            "DEMO-05 icon asset missing: ${_path}\n"
            "Initialize the rlvgl submodule: git submodule update --init rlvgl")
    endif()
    file(READ "${_path}" _hex HEX)
    string(REGEX MATCHALL "[0-9a-f][0-9a-f]" _bytes "${_hex}")
    list(LENGTH _bytes _len)
    if(_len EQUAL 0)
        message(FATAL_ERROR "DEMO-05 icon asset is empty: ${_path}")
    endif()
    set(_arr "")
    foreach(_b ${_bytes})
        string(APPEND _arr "0x${_b}, ")
    endforeach()
    string(APPEND _lvglpp_disco_body
        "inline constexpr std::array<std::uint8_t, ${_len}> ${_sym}_BYTES = { ${_arr} };\n")
endforeach()

file(WRITE "${_lvglpp_disco_inc}"
"// Auto-generated from rlvgl .rle icon assets at CMake configure time.\n"
"// DO NOT EDIT — regenerated whenever the assets change.\n"
"#pragma once\n"
"#include <array>\n"
"#include <cstdint>\n"
"namespace lvglpp::app::disco_demo::detail {\n"
"${_lvglpp_disco_body}"
"}\n")

# ---------------------------------------------------------------------------
# Static library target.
# ---------------------------------------------------------------------------
add_library(lvglpp_app_disco_demo STATIC)
add_library(lvglpp::app_disco_demo ALIAS lvglpp_app_disco_demo)

target_sources(lvglpp_app_disco_demo
    PRIVATE
        src/assets.cpp
        src/icon_strip.cpp
        src/wing.cpp
        src/dashboard_panel.cpp
        src/hotspot.cpp
        src/disco_controller.cpp
)

target_include_directories(lvglpp_app_disco_demo
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

target_link_libraries(lvglpp_app_disco_demo
    PUBLIC
        lvglpp::core
        lvglpp::ui
        lvglpp::widgets
        lvglpp::platform  # DEMO-06: DiscoController::make takes platform::Screen.
    PRIVATE
        lvglpp_warnings
)

# Embedded-posture parity, matching the top-level module wiring.
if(TARGET lvglpp_posture)
    target_link_libraries(lvglpp_app_disco_demo PUBLIC lvglpp_posture)
endif()

target_compile_features(lvglpp_app_disco_demo PUBLIC cxx_std_20)

if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
