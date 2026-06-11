# examples/linux-fbdev-smoke — PLAT-LNX §5.3 console smoke.
# Gated on LVGLPP_PLATFORM_LINUX_FBDEV (Linux-only, enforced in
# platform/CMakeLists.txt).

# Reuse the gallery icon embed.
set(_smoke_icon
    "${CMAKE_SOURCE_DIR}/rlvgl/examples/stm32h747i-disco/assets/icons/settings.rle")
file(READ "${_smoke_icon}" _smoke_hex HEX)
string(REGEX MATCHALL "[0-9a-f][0-9a-f]" _smoke_bytes "${_smoke_hex}")
list(LENGTH _smoke_bytes _smoke_len)
set(_smoke_body "")
foreach(_b ${_smoke_bytes})
    string(APPEND _smoke_body "0x${_b}, ")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/gallery_assets.inc"
"#pragma once\n#include <array>\n#include <cstdint>\n"
"namespace lvglpp::gallery::detail {\n"
"inline constexpr std::array<std::uint8_t, ${_smoke_len}> GALLERY_ICON_BYTES = { ${_smoke_body} };\n"
"}\n")

add_executable(lvglpp_example_linux_fbdev_smoke main.cpp)
target_include_directories(lvglpp_example_linux_fbdev_smoke
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(lvglpp_example_linux_fbdev_smoke
    PRIVATE
        lvglpp::core
        lvglpp::widgets
        lvglpp::platform
        lvglpp_warnings)
