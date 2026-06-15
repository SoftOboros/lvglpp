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

std::int32_t Font::line_height() const noexcept {
    if (font_ == nullptr) {
        return 0;
    }
    return font_->line_height;
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
