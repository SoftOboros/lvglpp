// draw.cpp — Font handle + draw-helper implementation (LPAR-08, v1 scope).
//
// PARITY: rlvgl/core/src/font.rs + rlvgl/core/src/draw.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/font/lv_font.c (lv_font_get_glyph_dsc, lv_font_get_default),
//         lvgl/src/draw/lv_draw_rect.c, lvgl/src/draw/lv_draw_label.c.
// DELTA:  see draw.hpp — wraps the LVGL draw engine; supersedes CORE-04/04a/06.

#include "lvglpp/core/draw.hpp"

namespace lvglpp::core {

Font Font::default_font() noexcept {
    // SAFETY: lv_font_get_default returns LV_FONT_DEFAULT, a const lv_font_t
    //   in static storage (e.g. lv_font_montserrat_14). It is external and
    //   never freed; observing it is safe for the program lifetime.
    return Font{lv_font_get_default()};
}

std::uint32_t Font::glyph_advance(std::uint32_t codepoint) const noexcept {
    if (font_ == nullptr) {
        return 0U;
    }
    lv_font_glyph_dsc_t dsc{};
    // letter_next = 0: no kerning against a following glyph.
    if (!lv_font_get_glyph_dsc(font_, &dsc, codepoint, 0U)) {
        return 0U;  // glyph absent from this font
    }
    return static_cast<std::uint32_t>(dsc.adv_w);
}

Font Font::builtin(BuiltinFont which) noexcept {
    // Each arm references a built-in only when its LV_FONT_MONTSERRAT_<n> flag
    // is enabled; otherwise the symbol is not declared, so the arm yields an
    // empty Font and the caller falls back to default_font() (FONT-00 §5.3).
    switch (which) {
        case BuiltinFont::Montserrat12:
#if LV_FONT_MONTSERRAT_12
            return Font{&lv_font_montserrat_12};
#else
            break;
#endif
        case BuiltinFont::Montserrat14:
#if LV_FONT_MONTSERRAT_14
            return Font{&lv_font_montserrat_14};
#else
            break;
#endif
        case BuiltinFont::Montserrat16:
#if LV_FONT_MONTSERRAT_16
            return Font{&lv_font_montserrat_16};
#else
            break;
#endif
        case BuiltinFont::Montserrat18:
#if LV_FONT_MONTSERRAT_18
            return Font{&lv_font_montserrat_18};
#else
            break;
#endif
        case BuiltinFont::Montserrat24:
#if LV_FONT_MONTSERRAT_24
            return Font{&lv_font_montserrat_24};
#else
            break;
#endif
        case BuiltinFont::Montserrat28:
#if LV_FONT_MONTSERRAT_28
            return Font{&lv_font_montserrat_28};
#else
            break;
#endif
        case BuiltinFont::Montserrat48:
#if LV_FONT_MONTSERRAT_48
            return Font{&lv_font_montserrat_48};
#else
            break;
#endif
    }
    return Font{};  // size not compiled in -> empty (caller falls back).
}

std::int32_t Font::line_height() const noexcept {
    if (font_ == nullptr) {
        return 0;
    }
    return font_->line_height;
}

std::int32_t Font::base_line() const noexcept {
    if (font_ == nullptr) {
        return 0;
    }
    return font_->base_line;
}

GlyphMetrics Font::glyph_metrics(std::uint32_t codepoint) const noexcept {
    if (font_ == nullptr) {
        return GlyphMetrics{};
    }
    lv_font_glyph_dsc_t dsc{};
    if (!lv_font_get_glyph_dsc(font_, &dsc, codepoint, 0U)) {
        return GlyphMetrics{};  // glyph absent from this font
    }
    return GlyphMetrics{
        static_cast<std::uint32_t>(dsc.adv_w),
        static_cast<std::uint32_t>(dsc.box_w),
        static_cast<std::uint32_t>(dsc.box_h),
        static_cast<std::int32_t>(dsc.ofs_x),
        static_cast<std::int32_t>(dsc.ofs_y),
    };
}

bool Font::is_anti_aliased() const noexcept {
    if (font_ == nullptr) {
        return false;
    }
    // Probe a few common glyphs; a font is AA if a resolved glyph uses a
    // multi-bit coverage format (A2/A3/A4/A8) rather than 1-bit A1.
    const std::uint32_t probes[] = {0x4DU /*'M'*/, 0x41U /*'A'*/, 0x30U /*'0'*/,
                                    0x20U /*space*/};
    for (std::uint32_t cp : probes) {
        lv_font_glyph_dsc_t dsc{};
        if (lv_font_get_glyph_dsc(font_, &dsc, cp, 0U)) {
            switch (dsc.format) {
                case LV_FONT_GLYPH_FORMAT_A2:
                case LV_FONT_GLYPH_FORMAT_A3:
                case LV_FONT_GLYPH_FORMAT_A4:
                case LV_FONT_GLYPH_FORMAT_A8:
                    return true;
                default:
                    return false;  // A1 or a non-coverage format -> not AA
            }
        }
    }
    return false;  // no probeable glyph
}

namespace draw {

void fill_rect(lv_layer_t* layer, const lv_area_t& coords, lv_color_t color,
               lv_opa_t opa) noexcept {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.bg_opa   = opa;
    // SAFETY: layer borrows a live draw context owned by the LVGL draw
    //   pipeline; valid for this call only. coords is copied by LVGL.
    lv_draw_rect(layer, &dsc, &coords);
}

void label(lv_layer_t* layer, const lv_area_t& coords, const char* text,
           const Font& font, lv_color_t color) noexcept {
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text  = text;   // borrows: must outlive the draw task (concepts §5.3).
    dsc.font  = font.borrow_raw();  // observes: external font.
    dsc.color = color;
    // SAFETY: layer borrows a live draw context owned by the LVGL draw
    //   pipeline; valid for this call only. coords is copied by LVGL.
    lv_draw_label(layer, &dsc, &coords);
}

}  // namespace draw

}  // namespace lvglpp::core
