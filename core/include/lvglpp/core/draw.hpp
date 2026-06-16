// draw.hpp — non-owning Font handle over lv_font_t + thin draw-helper
// wrappers over the LVGL draw layer (LPAR-08, v1 scope).
//
// PARITY: rlvgl/core/src/font.rs + rlvgl/core/src/draw.rs (v0.2.4 @ 343f596).
//         rlvgl re-implements glyph metrics and draw primitives because it
//         has no LVGL draw engine; lvglpp has one, so the ownership story
//         (Font observes a static font; draw helpers borrow a layer for the
//         duration of the call) matches but the rasterization is LVGL's.
// LVGL:   lvgl/src/font/lv_font.h (lv_font_t, lv_font_get_glyph_dsc,
//         lv_font_get_default), lvgl/src/draw/lv_draw_rect.h,
//         lvgl/src/draw/lv_draw_label.h, lvgl/src/draw/lv_draw.h (lv_layer_t).
// DELTA:  Supersedes CORE-04 Renderer, CORE-04a draw_widget_bg/
//         draw_border_straight, and the CORE-06 BitmapFont/PackedFont/
//         FONT_6X10 bring-up path (see docs/core-draw/00-text-draw-image-mask.md
//         §10). That supersession is documentary; this file does not delete
//         the older headers. Image descriptors / masks (concepts §3) are
//         deferred past this v1 scope.

#ifndef LVGLPP_CORE_DRAW_HPP
#define LVGLPP_CORE_DRAW_HPP

#include <cstdint>

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// FONT-00 — names LVGL's built-in anti-aliased lv_font_montserrat_* sizes.
// Specification Required (local enum; rlvgl has no built-in size enum). A
// size whose LV_FONT_MONTSERRAT_<n> build flag is off yields an empty Font
// from Font::builtin (the caller falls back to Font::default_font). See
// docs/font/00-concepts.md §3, §6.
enum class BuiltinFont : std::uint8_t {
    Montserrat12,
    Montserrat14,
    Montserrat16,
    Montserrat18,
    Montserrat24,
    Montserrat28,
    Montserrat48,
};

// FONT-00 — the layout-relevant subset of lv_font_glyph_dsc_t (§3). All zero
// for an empty Font or an absent glyph.
struct GlyphMetrics {
    std::uint32_t adv_w = 0;  // advance width (pen movement) in pixels.
    std::uint32_t box_w = 0;  // glyph bounding-box width.
    std::uint32_t box_h = 0;  // glyph bounding-box height.
    std::int32_t  ofs_x = 0;  // bounding-box x offset.
    std::int32_t  ofs_y = 0;  // bounding-box y offset.

    constexpr bool operator==(const GlyphMetrics&) const noexcept = default;
};

// Non-owning handle over an lv_font_t. LVGL fonts are typically static or
// otherwise externally owned (concepts §5.2), so Font observes; it never
// frees. Glyph metrics come straight from lv_font_get_glyph_dsc.
//
// Ownership: observes a const lv_font_t. The pointed-to font is external
// (static storage or owned by a font backend) and MUST outlive any Font
// view of it. A default-constructed / null Font is empty and every accessor
// returns 0.
class Font {
public:
    // Empty font: observes nothing; all accessors return 0.
    constexpr Font() noexcept = default;

    // Args:
    //   font: observes; external/static, must outlive this view. May be
    //         nullptr, which yields an empty Font.
    explicit constexpr Font(const lv_font_t* font) noexcept : font_{font} {}

    // The LVGL default font (LV_FONT_DEFAULT, e.g. lv_font_montserrat_14)
    // via lv_font_get_default(). Returns: observes a static font.
    [[nodiscard]] static Font default_font() noexcept;

    // FONT-00: a built-in anti-aliased montserrat font. Returns an empty Font
    // if that size's LV_FONT_MONTSERRAT_<n> build flag is disabled (the caller
    // falls back to default_font()). Returns: observes a static font.
    [[nodiscard]] static Font builtin(BuiltinFont which) noexcept;

    // observes: valid only while the underlying font is alive (external).
    [[nodiscard]] constexpr const lv_font_t* borrow_raw() const noexcept { return font_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return font_ == nullptr; }

    // Advance width (pen movement) for `codepoint` in pixels, via
    // lv_font_get_glyph_dsc's adv_w field. No kerning (no next codepoint).
    // Returns 0 for an empty font or a glyph the font does not contain.
    [[nodiscard]] std::uint32_t glyph_advance(std::uint32_t codepoint) const noexcept;

    // Line height in pixels (the lv_font_t::line_height field). Returns 0
    // for an empty font.
    [[nodiscard]] std::int32_t line_height() const noexcept;

    // FONT-00: base line measured from the bottom of the line height
    // (lv_font_t::base_line). Returns 0 for an empty font.
    [[nodiscard]] std::int32_t base_line() const noexcept;

    // FONT-00: full layout metrics for `codepoint` via lv_font_get_glyph_dsc
    // (no kerning). All-zero GlyphMetrics for an empty font or an absent glyph.
    [[nodiscard]] GlyphMetrics glyph_metrics(std::uint32_t codepoint) const noexcept;

    // FONT-00: whether a representative glyph rasterizes as multi-bit AA
    // coverage (A2/A3/A4/A8) rather than 1-bit (A1). This is a query on the
    // font asset; lvglpp adds no rasterization (§6). Uses 'M' (U+004D) as the
    // probe glyph, falling back to the font's first available glyph. Returns
    // false for an empty font or a font with no probeable glyph.
    [[nodiscard]] bool is_anti_aliased() const noexcept;

private:
    // observes: const lv_font_t owned externally (static storage or a font
    // backend). Never freed by this class. nullptr => empty.
    const lv_font_t* font_ = nullptr;
};

// Thin free-function wrappers over the LVGL draw layer, used by widgets that
// custom-draw (Canvas, Scale, Chart — concepts §5.3). Each borrows an
// lv_layer_t draw context for the duration of the call only.
//
// NOTE: runtime coverage of these helpers needs a live lv_layer_t (only
// available inside an LVGL draw event), which is out of scope for a host
// unit test. They are exercised by the first custom-draw widget; the unit
// test for this phase only proves they compile and link.
namespace draw {

// Fill `coords` with a solid `color` at opacity `opa` via lv_draw_rect.
// Args:
//   layer:  borrows the draw context for this call only; must be non-null.
//   coords: borrows; the area to fill (LVGL copies it).
void fill_rect(lv_layer_t* layer, const lv_area_t& coords, lv_color_t color,
               lv_opa_t opa) noexcept;

// Draw NUL-terminated `text` inside `coords` in `font`'s glyphs via
// lv_draw_label.
// Args:
//   layer:  borrows the draw context for this call only; must be non-null.
//   coords: borrows; the bounding area (LVGL copies it).
//   text:   borrows; must stay valid until LVGL renders the draw task.
//   font:   observes; its lv_font_t must outlive the draw.
void label(lv_layer_t* layer, const lv_area_t& coords, const char* text,
           const Font& font, lv_color_t color) noexcept;

}  // namespace draw

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_DRAW_HPP
