// font.hpp — bitmap and packed font value types.
//
// PARITY: rlvgl/core/src/bitmap_font.rs (v0.2.0 @ d99f793) — BitmapFont
//         (line 16) including draw_char (line 39), draw_str (line 70),
//         draw_str_y (line 83).
//         rlvgl/core/src/packed_font.rs (v0.2.0 @ d99f793) —
//         GlyphMetric (line 16), PackedFont (line 32).
// LVGL:   lvgl/src/font/lv_font.h — informative. LVGL's font system
//         is callback-based; lvglpp fonts are value-type views over
//         glyph data and call back into Renderer::fill_rect (bitmap)
//         or Renderer::draw_pixels (packed).
// DELTA:  rlvgl uses `&'static [u8]` and `&'static [GlyphMetric]`;
//         lvglpp uses std::span<const T> with no explicit `static`
//         lifetime requirement — the caller is responsible for
//         keeping the underlying storage alive (the `borrows`
//         ownership tag).
//
// This file implements docs/core-font/00-fonts.md (CORE-06).
// Field shapes are FROZEN under Standards Action.

#ifndef LVGLPP_CORE_FONT_HPP
#define LVGLPP_CORE_FONT_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"  // Color, Rect

namespace lvglpp::core {

// ---------------------------------------------------------------------------
// BitmapFont — concepts doc §5.1
// ---------------------------------------------------------------------------

// Fixed-width 1-bit-packed bitmap font. Glyphs cover ASCII
// 0x20..=0x7E, stored row-major MSB-first. Mirrors
// rlvgl/core/src/bitmap_font.rs:16.
//
// Ownership: BitmapFont is a non-owning view. The `data` span borrows
// glyph storage that lives elsewhere (typically a static array in a
// generated translation unit, or a memory-mapped flash region).
struct BitmapFont {
    std::uint8_t glyph_width  = 0;
    std::uint8_t glyph_height = 0;
    std::uint8_t scale        = 1;
    // borrows: glyph blob; lifetime is the application's. Mirrors
    // rlvgl's &'static [u8].
    std::span<const std::uint8_t> data{};

    // Scaled glyph dimensions in display pixels.
    [[nodiscard]] constexpr std::int32_t scaled_width() const noexcept {
        return static_cast<std::int32_t>(glyph_width) *
               static_cast<std::int32_t>(scale);
    }
    [[nodiscard]] constexpr std::int32_t scaled_height() const noexcept {
        return static_cast<std::int32_t>(glyph_height) *
               static_cast<std::int32_t>(scale);
    }

    // Render a single character at (x, y). Mirrors
    // rlvgl/core/src/bitmap_font.rs:39 — same indexing math, same
    // MSB-first bit order, same scale-replication via fill_rect.
    void draw_char(Renderer& renderer,
                   std::int32_t x,
                   std::int32_t y,
                   char32_t ch,
                   Color color) const {
        if (ch < 0x20U || ch > 0x7EU) return;  // non-printable
        const auto idx = static_cast<std::size_t>(
            static_cast<std::uint32_t>(ch) - 0x20U);
        const std::size_t bits_per_glyph =
            static_cast<std::size_t>(glyph_width) *
            static_cast<std::size_t>(glyph_height);
        const std::size_t bit_offset = idx * bits_per_glyph;
        const auto s = static_cast<std::int32_t>(scale);

        for (std::size_t row = 0; row < glyph_height; ++row) {
            for (std::size_t col = 0; col < glyph_width; ++col) {
                const std::size_t bit =
                    bit_offset + row * glyph_width + col;
                const std::size_t byte_idx = bit / 8U;
                const std::size_t bit_idx  = 7U - (bit % 8U);
                if (byte_idx < data.size() &&
                    ((data[byte_idx] >> bit_idx) & 1U) != 0U) {
                    renderer.fill_rect(
                        Rect{
                            x + static_cast<std::int32_t>(col) * s,
                            y + static_cast<std::int32_t>(row) * s,
                            s,
                            s,
                        },
                        color);
                }
            }
        }
    }

    // Render a string at (x, y) advancing along x. Mirrors
    // rlvgl/core/src/bitmap_font.rs:70.
    //
    // Args:
    //   text: borrows; UTF-8. Non-ASCII codepoints are skipped per
    //         draw_char's printable-range check.
    void draw_str(Renderer& renderer,
                  std::int32_t x,
                  std::int32_t y,
                  std::string_view text,
                  Color color) const {
        const auto advance = scaled_width() + static_cast<std::int32_t>(scale);
        std::int32_t cx = x;
        for (char ch : text) {
            // ASCII fast path; multi-byte UTF-8 codepoints are
            // skipped because draw_char's range check filters them.
            // BitmapFont is ASCII-only by design (use PackedFont for
            // Unicode).
            draw_char(renderer, cx, y, static_cast<char32_t>(static_cast<unsigned char>(ch)), color);
            cx += advance;
        }
    }

    // Render a string advancing along y (rotated displays). Mirrors
    // rlvgl/core/src/bitmap_font.rs:83.
    void draw_str_y(Renderer& renderer,
                    std::int32_t x,
                    std::int32_t y,
                    std::string_view text,
                    Color color) const {
        const auto advance = scaled_width() + static_cast<std::int32_t>(scale);
        std::int32_t cy = y;
        for (char ch : text) {
            draw_char(renderer, x, cy, static_cast<char32_t>(static_cast<unsigned char>(ch)), color);
            cy += advance;
        }
    }
};

// ---------------------------------------------------------------------------
// GlyphMetric + PackedFont — concepts doc §5.2
// ---------------------------------------------------------------------------

struct GlyphMetric {
    char32_t      ch           = 0;
    std::uint16_t width        = 0;
    std::uint16_t height       = 0;
    std::uint16_t advance_fp16 = 0;  // Q12.4 fixed-point pixel advance
    std::uint32_t offset       = 0;
    std::int16_t  ymin         = 0;

    constexpr bool operator==(const GlyphMetric&) const noexcept = default;
};

// Variable-width grayscale font. 8-bit glyphs (0=transparent,
// 255=opaque) with per-glyph metrics. Mirrors
// rlvgl/core/src/packed_font.rs:32.
//
// Ownership: non-owning view. Both spans borrow.
struct PackedFont {
    std::uint16_t height = 0;
    std::int16_t  ascent = 0;
    // borrows: sorted by codepoint for binary search.
    std::span<const GlyphMetric>   glyphs{};
    // borrows: raw 8-bpp grayscale glyph bitmap data.
    std::span<const std::uint8_t>  data{};

    // Look up a glyph by codepoint. Returns nullptr if the codepoint
    // is not in the font. Mirrors rlvgl's binary_search_by_key.
    [[nodiscard]] const GlyphMetric* glyph(char32_t ch) const noexcept {
        auto it = std::lower_bound(
            glyphs.begin(), glyphs.end(), ch,
            [](const GlyphMetric& g, char32_t c) noexcept {
                return g.ch < c;
            });
        if (it == glyphs.end() || it->ch != ch) return nullptr;
        return &*it;
    }
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_FONT_HPP
