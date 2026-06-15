# lvglpp::core — foundation library mirroring rlvgl-core.
#
# PARITY: rlvgl/core (v0.2.0). The crate-level decomposition (widget,
# event, renderer, style, theme, animation, font, plugins) is the
# triangulation seam; each rlvgl module here gets a matching public
# header under include/lvglpp/core/ as it is implemented.

# ---------------------------------------------------------------------------
# Plugin slot options (CORE-07)
# ---------------------------------------------------------------------------
# Each slot mirrors a rlvgl `feature = "<name>"` and a
# `rlvgl/core/src/plugins/<name>.rs` module. All default OFF; consumers
# opt in. See docs/core-plugins/00-plugin-surface.md §5.1.
option(LVGLPP_CORE_PNG            "Enable PNG decoder plugin"            OFF)
option(LVGLPP_CORE_JPEG           "Enable JPEG decoder plugin"           OFF)
option(LVGLPP_CORE_GIF            "Enable GIF decoder plugin"            OFF)
option(LVGLPP_CORE_APNG           "Enable APNG decoder plugin"           OFF)
option(LVGLPP_CORE_QRCODE         "Enable QR code generator plugin"      OFF)
option(LVGLPP_CORE_CANVAS         "Enable embedded-canvas plugin"        OFF)
option(LVGLPP_CORE_FATFS          "Enable FAT filesystem plugin"         OFF)
option(LVGLPP_CORE_FONTDUE        "Enable fontdue host font plugin"      OFF)
option(LVGLPP_CORE_RLE            "Enable RLE (RLEC) decoder plugin"     OFF)  # CORE-07n
option(LVGLPP_CORE_LOTTIE         "Enable Lottie API surface plugin"     OFF)
option(LVGLPP_CORE_LOTTIE_BACKEND "Enable rlottie runtime backend"       OFF)
option(LVGLPP_CORE_PINYIN         "Enable pinyin helper plugin"          OFF)
option(LVGLPP_CORE_NES            "Enable NES emulator hook plugin"      OFF)
option(LVGLPP_CORE_DASH_LOTTIE    "Enable dash-lottie lite plugin"       OFF)

add_library(lvglpp_core)
add_library(lvglpp::core ALIAS lvglpp_core)

# Generate font_6x10_data.inc from the 713-byte bring-up font binary
# at configure time. Portable — no xxd/objcopy dependency.
set(LVGLPP_FONT_6X10_BIN "${CMAKE_CURRENT_SOURCE_DIR}/src/fonts/font_6x10.bin")
set(LVGLPP_FONT_6X10_INC "${CMAKE_CURRENT_BINARY_DIR}/generated/font_6x10_data.inc")
file(READ "${LVGLPP_FONT_6X10_BIN}" LVGLPP_FONT_6X10_HEX HEX)
string(REGEX MATCHALL "[0-9a-f][0-9a-f]" LVGLPP_FONT_6X10_BYTES "${LVGLPP_FONT_6X10_HEX}")
list(LENGTH LVGLPP_FONT_6X10_BYTES LVGLPP_FONT_6X10_LEN)
set(LVGLPP_FONT_6X10_BODY "")
foreach(_b ${LVGLPP_FONT_6X10_BYTES})
    string(APPEND LVGLPP_FONT_6X10_BODY "0x${_b}, ")
endforeach()
file(WRITE "${LVGLPP_FONT_6X10_INC}"
"// Auto-generated from src/fonts/font_6x10.bin at CMake configure time.\n"
"// DO NOT EDIT — regenerated whenever the .bin changes.\n"
"#pragma once\n"
"#include <array>\n"
"#include <cstdint>\n"
"namespace lvglpp::core::fonts::detail {\n"
"inline constexpr std::array<std::uint8_t, ${LVGLPP_FONT_6X10_LEN}> FONT_6X10_BYTES = { ${LVGLPP_FONT_6X10_BODY} };\n"
"}\n")

target_sources(lvglpp_core
    PRIVATE
        src/runtime.cpp
        src/object.cpp
        src/display.cpp
        src/timer.cpp
        src/draw.cpp
        src/fonts/font_6x10.cpp
)

# CORE-07n — RLE decoder plugin (docs/core-plugins/01-rle.md).
# Embedded-friendly; no external dependency.
if(LVGLPP_CORE_RLE)
    target_sources(lvglpp_core PRIVATE src/plugins/rle.cpp)
    target_compile_definitions(lvglpp_core PUBLIC LVGLPP_CORE_RLE=1)
endif()

target_include_directories(lvglpp_core
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

target_include_directories(lvglpp_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(lvglpp_core
    PUBLIC
        lvgl
        lvglpp::expected
    PRIVATE
        lvglpp_warnings
)

target_compile_features(lvglpp_core PUBLIC cxx_std_20)

if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
