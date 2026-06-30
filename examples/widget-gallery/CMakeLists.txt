# examples/widget-gallery — WID-05/WID-06 conformance sim.
#
# Headless-only (no SDL): playit TCP + D dumps + ASCII capture over
# the List+Image composition. docs/widgets-list/00-list.md §12 /
# docs/widgets-image/00-image.md §12.

# One icon blob from the lvglpp-owned disco-demo asset set (same configure-time
# embed pattern as apps/disco-demo).
set(_gal_icon
    "${CMAKE_SOURCE_DIR}/examples/apps/disco-demo/assets/icons/settings.rle")
if(NOT EXISTS "${_gal_icon}")
    message(FATAL_ERROR "widget-gallery: missing ${_gal_icon} "
        "(lvglpp disco-demo assets not present?)")
endif()
file(READ "${_gal_icon}" _gal_hex HEX)
string(REGEX MATCHALL "[0-9a-f][0-9a-f]" _gal_bytes "${_gal_hex}")
list(LENGTH _gal_bytes _gal_len)
set(_gal_body "")
foreach(_b ${_gal_bytes})
    string(APPEND _gal_body "0x${_b}, ")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/gallery_assets.inc"
"// Auto-generated from settings.rle at CMake configure time.\n"
"#pragma once\n#include <array>\n#include <cstdint>\n"
"namespace lvglpp::gallery::detail {\n"
"inline constexpr std::array<std::uint8_t, ${_gal_len}> GALLERY_ICON_BYTES = { ${_gal_body} };\n"
"}\n")
# Namespace note: the scene reads gallery::detail relative to
# lvglpp::gallery.

add_executable(lvglpp_example_widget_gallery main.cpp)
target_include_directories(lvglpp_example_widget_gallery
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(lvglpp_example_widget_gallery
    PRIVATE
        lvglpp::core
        lvglpp::widgets
        lvglpp::playit
        lvglpp_warnings)
