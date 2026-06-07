// font_6x10.hpp — bring-up bitmap font.
//
// PARITY: rlvgl/core/src/bitmap_font.rs:109 (FONT_6X10) and the
//         713-byte glyph blob at rlvgl/core/src/bitmap_font_6x10.bin
//         (v0.2.0 @ d99f793).
// LVGL:   N/A.
// DELTA:  rlvgl exposes `pub static FONT_6X10: BitmapFont`; lvglpp
//         exposes the same value via `lvglpp::core::fonts::FONT_6X10`
//         linked from src/fonts/font_6x10.cpp. The glyph data itself
//         is byte-for-byte identical (same .bin file copied into
//         core/src/fonts/font_6x10.bin and embedded by CMake-time
//         hex conversion).
//
// 6x10 ASCII (0x20..=0x7E) bitmap font, rendered at 2x scale by
// default (12x20 display pixels per glyph) — matches rlvgl's stub
// font ergonomics for bring-up.

#ifndef LVGLPP_CORE_FONTS_FONT_6X10_HPP
#define LVGLPP_CORE_FONTS_FONT_6X10_HPP

#include "lvglpp/core/font.hpp"

namespace lvglpp::core::fonts {

// Built-in 6x10 ASCII bitmap font (95 glyphs, 2x scale).
// borrows the static glyph blob held in src/fonts/font_6x10.cpp.
extern const BitmapFont FONT_6X10;

}  // namespace lvglpp::core::fonts

#endif  // LVGLPP_CORE_FONTS_FONT_6X10_HPP
