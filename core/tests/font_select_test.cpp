// font_select_test.cpp - FONT-00 acceptance adapted to lvglpp LvFontView.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/draw_lvgl.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style_lvgl.hpp"

#include <array>
#include <cassert>

namespace {

struct Fixture {
    std::array<std::uint8_t, 80 * 80 * 4> draw_buffer{};
    lvglpp::LvDisplay display;
    lvglpp::LvObject screen;

    Fixture()
        : display{lvglpp::LvDisplay::make(80, 80)},
          screen{lvglpp::LvObject::make_screen()} {
        assert(!display.empty());
        assert(!screen.empty());
        display.set_default();
        display.set_buffers(draw_buffer.data(),
                            nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_screen_load(screen.borrow_raw());
    }
};

void test_default_font() {
    const auto font = lvglpp::LvFontView::default_font();
    assert(!font.empty());
    assert(font.is_anti_aliased());
    assert(font.line_height() > 0);
    assert(font.base_line() >= 0);

    const auto metrics = font.glyph_metrics(U'M');
    assert(metrics.advance_width > 0U);
    assert(metrics.box_width > 0U);
    assert(metrics.box_height > 0U);
}

void test_builtin_font_selection() {
    const auto font14 = lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat14);
    assert(!font14.empty());
    assert(font14.is_anti_aliased());
    assert(font14.borrow_raw() == lvglpp::LvFontView::default_font().borrow_raw());

    const auto font12 = lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat12);
#if LV_FONT_MONTSERRAT_12
    assert(!font12.empty());
#else
    assert(font12.empty());
#endif
}

void test_empty_font() {
    const lvglpp::LvFontView empty{nullptr};
    assert(empty.empty());
    assert(!empty.is_anti_aliased());
    assert(empty.line_height() == 0);
    assert(empty.base_line() == 0);
    assert(empty.glyph_width(U'M') == 0U);
    assert(empty.glyph_metrics(U'M') == lvglpp::GlyphMetrics{});

    const auto absent = lvglpp::LvFontView::default_font().glyph_metrics(
        static_cast<char32_t>(0x10FFFFU));
    assert(absent == lvglpp::GlyphMetrics{});
}

void test_default_resolution(Fixture& fixture) {
    auto object = lvglpp::LvObject::make_child(fixture.screen.borrow());
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           lvglpp::LvFontView::default_font().borrow_raw());
}

void test_local_font_selection(Fixture& fixture) {
    auto object = lvglpp::LvObject::make_child(fixture.screen.borrow());
    const auto font14 = lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat14);
    assert(!font14.empty());

    lvglpp::set_local_text_font(object.borrow(),
                                font14.borrow_raw(),
                                lvglpp::StyleSelector{});
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           font14.borrow_raw());

    lvglpp::set_local_text_font(
        object.borrow(), nullptr, lvglpp::StyleSelector{});
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           font14.borrow_raw());
}

void test_style_font_selection(Fixture& fixture) {
    const auto font14 = lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat14);
    assert(!font14.empty());

    lvglpp::LvStyle style;
    style.set_text_font(font14.borrow_raw());

    auto object = lvglpp::LvObject::make_child(fixture.screen.borrow());
    lvglpp::add_style(object.borrow(), style.borrow(), lvglpp::StyleSelector{});
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           font14.borrow_raw());
}

void test_empty_object_safe() {
    const lvglpp::ObjectView empty{nullptr};
    assert(lvglpp::resolved_text_font(empty, lvglpp::StylePart::Main) == nullptr);
    lvglpp::set_local_text_font(
        empty,
        lvglpp::LvFontView::default_font().borrow_raw(),
        lvglpp::StyleSelector{});
}

}  // namespace

int main() {
    auto runtime = lvglpp::Runtime::try_make();
    assert(runtime.has_value());

    Fixture fixture;
    test_default_font();
    test_builtin_font_selection();
    test_empty_font();
    test_default_resolution(fixture);
    test_local_font_selection(fixture);
    test_style_font_selection(fixture);
    test_empty_object_safe();

    return 0;
}
