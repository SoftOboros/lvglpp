# lvglpp STM32H747I-DISCO example. PLAT-02 family.
#
# Gated on LVGLPP_PLATFORM_DISCO. Requires the arm-none-eabi
# cross-toolchain (cmake/toolchains/arm-none-eabi.cmake). Building
# this directory on a host toolchain is a configuration error.

if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm" OR NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    message(FATAL_ERROR
        "examples/stm32h747i-disco/ requires the arm-none-eabi toolchain.\n"
        "Configure with:\n"
        "  cmake -S . -B build-disco \\\n"
        "        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake \\\n"
        "        -DLVGLPP_PLATFORM_DISCO=ON \\\n"
        "        -DLVGLPP_BUILD_TESTS=OFF -DLVGLPP_BUILD_EXAMPLES=ON")
endif()

# Locate the linker script. CMAKE_CURRENT_SOURCE_DIR holds memory.ld
# + disco.ld; both are passed to the linker via -L + -T.
set(_disco_ld_dir "${CMAKE_CURRENT_SOURCE_DIR}")

# --- PLAT-02a smoke target ------------------------------------------
# Bare-metal: vector table + reset handler + a main() that loops on
# WFE. Validates the toolchain + linker seam end-to-end.

add_executable(lvglpp_stm32h747i_disco_smoke
    startup_stm32h747xi.cpp
    main_smoke.cpp
)

set_target_properties(lvglpp_stm32h747i_disco_smoke PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "lvglpp-stm32h747i-disco-smoke")

target_link_libraries(lvglpp_stm32h747i_disco_smoke
    PRIVATE
        lvglpp_warnings
        lvglpp_posture)

# Linker script + map file + nostartfiles (we provide our own
# Reset_Handler + vector table).
target_link_options(lvglpp_stm32h747i_disco_smoke PRIVATE
    "-T${_disco_ld_dir}/disco.ld"
    "-L${_disco_ld_dir}"
    "-nostartfiles"
    "-Wl,-Map=$<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_smoke>/lvglpp-stm32h747i-disco-smoke.map"
)

# Force a relink whenever a linker script changes.
set_target_properties(lvglpp_stm32h747i_disco_smoke PROPERTIES
    LINK_DEPENDS "${_disco_ld_dir}/disco.ld;${_disco_ld_dir}/memory.ld")

# Emit .bin / .hex + size summary post-build.
add_custom_command(TARGET lvglpp_stm32h747i_disco_smoke POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:lvglpp_stm32h747i_disco_smoke>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_smoke>/lvglpp-stm32h747i-disco-smoke.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:lvglpp_stm32h747i_disco_smoke>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_smoke>/lvglpp-stm32h747i-disco-smoke.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:lvglpp_stm32h747i_disco_smoke>
    VERBATIM)

# --- PLAT-02c sdram smoke target ------------------------------------
# Boots, runs the clock tree (PLAT-02b), muxes FMC pins, brings up
# bank-2 SDRAM at 0xD0000000 + MPU region 0, runs a 4 KiB memtest,
# leaves canaries 0xDEADBEEF / 0xCAFEBABE at 0xD0000000 / 0xD0000004
# so a probe-rs reader can confirm the chip is live.

add_executable(lvglpp_stm32h747i_disco_sdram
    startup_stm32h747xi.cpp
    disco/clocks.cpp
    disco/pinmux.cpp
    disco/sdram.cpp
    main_sdram.cpp
)

set_target_properties(lvglpp_stm32h747i_disco_sdram PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "lvglpp-stm32h747i-disco-sdram")

target_include_directories(lvglpp_stm32h747i_disco_sdram
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(lvglpp_stm32h747i_disco_sdram
    PRIVATE lvglpp_warnings lvglpp_posture)

target_link_options(lvglpp_stm32h747i_disco_sdram PRIVATE
    "-T${_disco_ld_dir}/disco.ld"
    "-L${_disco_ld_dir}"
    "-nostartfiles"
    "-Wl,-Map=$<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_sdram>/lvglpp-stm32h747i-disco-sdram.map")

set_target_properties(lvglpp_stm32h747i_disco_sdram PROPERTIES
    LINK_DEPENDS "${_disco_ld_dir}/disco.ld;${_disco_ld_dir}/memory.ld")

add_custom_command(TARGET lvglpp_stm32h747i_disco_sdram POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:lvglpp_stm32h747i_disco_sdram>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_sdram>/lvglpp-stm32h747i-disco-sdram.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:lvglpp_stm32h747i_disco_sdram>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_sdram>/lvglpp-stm32h747i-disco-sdram.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:lvglpp_stm32h747i_disco_sdram>
    VERBATIM)

# --- PLAT-02d display first-pixels target ---------------------------
# Boots, runs the clock tree (PLAT-02b), muxes FMC pins, brings up
# SDRAM (PLAT-02c), then DSI + LTDC + NT35510 panel (PLAT-02d) in
# adapted command mode and fills the framebuffer at 0xD0000000 with a
# four-quadrant + 1px-border ARGB8888 pattern. Relays DSI_PSR /
# LTDC_CDSR / DSI_WISR to D3 SRAM at 0x38000320.. for probe-rs.
# PLAT-02f-1: ends in the playit serial pump on USART1/VCP. The
# playit parser/formatter sources compile directly into the target
# (06-touch-and-uart.md §5.4) until the module libraries grow a
# cross-build leg.

# PLAT-02e-3b: regenerate the bring-up font .inc for the cross
# build (same configure-time conversion as core/CMakeLists.txt —
# core's copy lands in the host build dir only).
set(_disco_font_bin "${CMAKE_SOURCE_DIR}/core/src/fonts/font_6x10.bin")
set(_disco_font_inc_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(READ "${_disco_font_bin}" _disco_font_hex HEX)
string(REGEX MATCHALL "[0-9a-f][0-9a-f]" _disco_font_bytes "${_disco_font_hex}")
list(LENGTH _disco_font_bytes _disco_font_len)
set(_disco_font_body "")
foreach(_b ${_disco_font_bytes})
    string(APPEND _disco_font_body "0x${_b}, ")
endforeach()
file(WRITE "${_disco_font_inc_dir}/font_6x10_data.inc"
"// Auto-generated from core/src/fonts/font_6x10.bin at CMake configure time.\n"
"// DO NOT EDIT — regenerated whenever the .bin changes.\n"
"#pragma once\n"
"#include <array>\n"
"#include <cstdint>\n"
"namespace lvglpp::core::fonts::detail {\n"
"inline constexpr std::array<std::uint8_t, ${_disco_font_len}> FONT_6X10_BYTES = { ${_disco_font_body} };\n"
"}\n")

add_executable(lvglpp_stm32h747i_disco_display
    startup_stm32h747xi.cpp
    disco/clocks.cpp
    disco/pinmux.cpp
    disco/sdram.cpp
    disco/display.cpp
    disco/dma2d.cpp
    disco/usart.cpp
    disco/renderer.cpp
    disco/cxx_shims.cpp
    main_display.cpp
    ${CMAKE_SOURCE_DIR}/playit/src/parser.cpp
    ${CMAKE_SOURCE_DIR}/playit/src/format.cpp
    ${CMAKE_SOURCE_DIR}/playit/src/dispatcher.cpp
    ${CMAKE_SOURCE_DIR}/widgets/src/container.cpp
    ${CMAKE_SOURCE_DIR}/widgets/src/button.cpp
    ${CMAKE_SOURCE_DIR}/widgets/src/label.cpp
    ${CMAKE_SOURCE_DIR}/core/src/fonts/font_6x10.cpp
)

set_target_properties(lvglpp_stm32h747i_disco_display PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "lvglpp-stm32h747i-disco-display")

target_include_directories(lvglpp_stm32h747i_disco_display
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/playit/include
        ${CMAKE_SOURCE_DIR}/core/include
        ${CMAKE_SOURCE_DIR}/widgets/include
        ${_disco_font_inc_dir})

target_link_libraries(lvglpp_stm32h747i_disco_display
    PRIVATE lvglpp_warnings lvglpp_posture)

target_link_options(lvglpp_stm32h747i_disco_display PRIVATE
    "-T${_disco_ld_dir}/disco.ld"
    "-L${_disco_ld_dir}"
    "-nostartfiles"
    "-Wl,-Map=$<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_display>/lvglpp-stm32h747i-disco-display.map")

set_target_properties(lvglpp_stm32h747i_disco_display PROPERTIES
    LINK_DEPENDS "${_disco_ld_dir}/disco.ld;${_disco_ld_dir}/memory.ld")

add_custom_command(TARGET lvglpp_stm32h747i_disco_display POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:lvglpp_stm32h747i_disco_display>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_display>/lvglpp-stm32h747i-disco-display.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:lvglpp_stm32h747i_disco_display>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_display>/lvglpp-stm32h747i-disco-display.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:lvglpp_stm32h747i_disco_display>
    VERBATIM)

# --- PLAT-02b clocks-and-pinmux smoke target ------------------------
# Boots, brings up the clock tree (SYSCLK=400, HCLK=200, PLL3_R≈32),
# muxes the FMC pins AF12+VeryHigh, writes breadcrumb 0xA11C_0007,
# loops on WFE. Halted-target acceptance per PLAT-02b §12.

add_executable(lvglpp_stm32h747i_disco_clocks
    startup_stm32h747xi.cpp
    disco/clocks.cpp
    disco/pinmux.cpp
    main_clocks.cpp
)

set_target_properties(lvglpp_stm32h747i_disco_clocks PROPERTIES
    SUFFIX ".elf"
    OUTPUT_NAME "lvglpp-stm32h747i-disco-clocks")

target_include_directories(lvglpp_stm32h747i_disco_clocks
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(lvglpp_stm32h747i_disco_clocks
    PRIVATE lvglpp_warnings lvglpp_posture)

target_link_options(lvglpp_stm32h747i_disco_clocks PRIVATE
    "-T${_disco_ld_dir}/disco.ld"
    "-L${_disco_ld_dir}"
    "-nostartfiles"
    "-Wl,-Map=$<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_clocks>/lvglpp-stm32h747i-disco-clocks.map")

set_target_properties(lvglpp_stm32h747i_disco_clocks PROPERTIES
    LINK_DEPENDS "${_disco_ld_dir}/disco.ld;${_disco_ld_dir}/memory.ld")

add_custom_command(TARGET lvglpp_stm32h747i_disco_clocks POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:lvglpp_stm32h747i_disco_clocks>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_clocks>/lvglpp-stm32h747i-disco-clocks.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:lvglpp_stm32h747i_disco_clocks>
            $<TARGET_FILE_DIR:lvglpp_stm32h747i_disco_clocks>/lvglpp-stm32h747i-disco-clocks.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:lvglpp_stm32h747i_disco_clocks>
    VERBATIM)
