// font_select_test.cpp — FONT-00 acceptance: Font handle extensions
// (builtin/glyph_metrics/is_anti_aliased/base_line) and cascade font
// selection (Style::set_text_font, Object::set_local_text_font, text_font).
//
// See docs/font/00-concepts.md §12.

#include "lvglpp/core/draw.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style_cascade.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::Runtime;
using lvglpp::core::BuiltinFont;
using lvglpp::core::Font;
using lvglpp::core::GlyphMetrics;
using lvglpp::core::Object;
using lvglpp::core::Screen;
namespace style = lvglpp::core::style;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// The default font is a built-in anti-aliased montserrat with real metrics.
void test_default_font() {
    Font def = Font::default_font();
    assert(!def.empty());
    assert(def.is_anti_aliased());      // LV_FONT_DEFAULT is 4-bit AA
    assert(def.line_height() > 0);
    assert(def.base_line() >= 0);

    GlyphMetrics m = def.glyph_metrics(0x4DU);  // 'M'
    assert(m.adv_w > 0U && m.box_w > 0U && m.box_h > 0U);
}

// builtin() returns the compiled-in font, or an empty Font for a size whose
// LV_FONT_MONTSERRAT_<n> flag is off.
void test_builtin() {
    Font f14 = Font::builtin(BuiltinFont::Montserrat14);
    assert(!f14.empty());                                  // on by default
    assert(f14.is_anti_aliased());
    assert(f14.borrow_raw() == Font::default_font().borrow_raw());  // == LV_FONT_DEFAULT

    // Montserrat12 is off in this lv_conf -> empty Font (caller falls back).
    Font f12 = Font::builtin(BuiltinFont::Montserrat12);
    assert(f12.empty());
}

// Empty Font: every accessor is a safe zero/false.
void test_empty_font() {
    Font empty;
    assert(empty.empty());
    assert(!empty.is_anti_aliased());
    assert(empty.line_height() == 0);
    assert(empty.base_line() == 0);
    assert(empty.glyph_advance(0x4DU) == 0U);
    assert(empty.glyph_metrics(0x4DU) == GlyphMetrics{});

    // Absent glyph in a real font also yields zero metrics.
    GlyphMetrics absent = Font::default_font().glyph_metrics(0x10FFFFU);
    assert(absent == GlyphMetrics{});
}

// A fresh object resolves to the cascade default font.
void test_default_resolution() {
    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());
    assert(obj.text_font().borrow_raw() == Font::default_font().borrow_raw());
}

// Object::set_local_text_font selects a font; text_font() resolves it back.
void test_local_font_selection() {
    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());

    Font f14 = Font::builtin(BuiltinFont::Montserrat14);
    obj.set_local_text_font(f14, style::Selector{style::Part::Main});
    assert(obj.text_font().borrow_raw() == f14.borrow_raw());

    // Empty Font is a no-op (leaves the existing selection).
    obj.set_local_text_font(Font{}, style::Selector{style::Part::Main});
    assert(obj.text_font().borrow_raw() == f14.borrow_raw());
}

// Style::set_text_font selects through a shared style; the Style outlives obj.
void test_style_font_selection() {
    Font f14 = Font::builtin(BuiltinFont::Montserrat14);

    style::Style s;
    s.set_text_font(f14);

    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());
    obj.add_style(s, style::Selector{style::Part::Main});
    assert(obj.text_font().borrow_raw() == f14.borrow_raw());
    // obj destroyed before s — honoring the outlives-objects rule.
}

// Font ops on an empty Object are no-ops.
void test_empty_object_safe() {
    Object o = Object::make(lvglpp::ObjectView{lv_screen_active()});
    Object moved = std::move(o);
    (void)moved;
    assert(o.empty());
    assert(o.text_font().empty());
    o.set_local_text_font(Font::default_font(), style::Selector{});  // no-op
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    static std::uint8_t draw_buf[100 * 20 * 4];
    lv_display_t* disp = lv_display_create(100, 100);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    test_default_font();
    test_builtin();
    test_empty_font();
    test_default_resolution();
    test_local_font_selection();
    test_style_font_selection();
    test_empty_object_safe();

    lv_display_delete(disp);
    return 0;
}
