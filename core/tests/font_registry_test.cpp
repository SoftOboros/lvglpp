// font_registry_test.cpp - FONT-05 acceptance for FontId and FontRegistry.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/font_registry.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

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

void test_font_id() {
    static_assert(lvglpp::FontId{}.value() == 0U);
    static_assert(lvglpp::FontId::default_id() == lvglpp::FontId{});
    static_assert(lvglpp::FontId{7} == lvglpp::FontId{7});
    static_assert(!(lvglpp::FontId{7} == lvglpp::FontId{8}));
}

void test_register_lookup() {
    lvglpp::FontRegistry<> registry;
    assert(registry.count() == 0U);
    assert(lvglpp::FontRegistry<>::capacity() == 16U);

    assert(registry.lookup(lvglpp::FontId::default_id()).borrow_raw() ==
           lvglpp::LvFontView::default_font().borrow_raw());

    const lvglpp::FontId body{1};
    const auto font14 = lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat14);
    assert(registry.register_font(body, font14));
    assert(registry.count() == 1U);
    assert(registry.lookup(body).borrow_raw() == font14.borrow_raw());

    assert(registry.register_font(body, lvglpp::LvFontView::default_font()));
    assert(registry.count() == 1U);
    assert(registry.lookup(body).borrow_raw() ==
           lvglpp::LvFontView::default_font().borrow_raw());

    assert(registry.lookup(lvglpp::FontId{99}).empty());
}

void test_over_capacity() {
    lvglpp::FontRegistry<2> registry;
    assert(registry.register_font(
        lvglpp::FontId{1}, lvglpp::LvFontView::default_font()));
    assert(registry.register_font(
        lvglpp::FontId{2}, lvglpp::LvFontView::default_font()));
    assert(registry.count() == 2U);
    assert(!registry.register_font(
        lvglpp::FontId{3}, lvglpp::LvFontView::default_font()));
    assert(registry.count() == 2U);
    assert(registry.register_font(
        lvglpp::FontId{1},
        lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat14)));
    assert(registry.count() == 2U);
}

void test_apply(Fixture& fixture) {
    auto object = lvglpp::LvObject::make_child(fixture.screen.borrow());
    lvglpp::FontRegistry<> registry;

    const lvglpp::FontId body{1};
    const auto font14 = lvglpp::LvFontView::builtin(lvglpp::BuiltinFont::Montserrat14);
    assert(registry.register_font(body, font14));

    assert(registry.apply(object.borrow(), body, lvglpp::StyleSelector{}));
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           font14.borrow_raw());

    assert(!registry.apply(object.borrow(), lvglpp::FontId{42}, lvglpp::StyleSelector{}));
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           font14.borrow_raw());

    assert(registry.apply(
        object.borrow(), lvglpp::FontId::default_id(), lvglpp::StyleSelector{}));
    assert(lvglpp::resolved_text_font(object.borrow(), lvglpp::StylePart::Main) ==
           lvglpp::LvFontView::default_font().borrow_raw());
}

}  // namespace

int main() {
    auto runtime = lvglpp::Runtime::try_make();
    assert(runtime.has_value());

    Fixture fixture;
    test_font_id();
    test_register_lookup();
    test_over_capacity();
    test_apply(fixture);

    return 0;
}
