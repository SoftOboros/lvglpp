// font_registry_test.cpp — FONT-05 acceptance: FontId + heap-free
// FontRegistry (register/lookup/apply, default-id resolution, over-capacity
// failure, observe-only lifetime).
//
// See docs/font/05-font-registry.md §12.

#include "lvglpp/core/draw.hpp"
#include "lvglpp/core/font_registry.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style_cascade.hpp"

#include <cassert>
#include <cstdint>

using lvglpp::Runtime;
using lvglpp::core::BuiltinFont;
using lvglpp::core::Font;
using lvglpp::core::FontId;
using lvglpp::core::FontRegistry;
using lvglpp::core::Object;
using lvglpp::core::Screen;
namespace style = lvglpp::core::style;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// FontId mirrors rlvgl: default id is 0, value-comparable.
void test_font_id() {
    static_assert(FontId{}.value() == 0U);
    static_assert(FontId::default_id() == FontId{});
    static_assert(FontId{7} == FontId{7});
    static_assert(!(FontId{7} == FontId{8}));
}

// register/lookup round-trips; default id always resolves to default_font().
void test_register_lookup() {
    FontRegistry<> reg;
    assert(reg.count() == 0U);
    assert(FontRegistry<>::capacity() == 16U);

    // Default id resolves even with no registration.
    assert(reg.lookup(FontId::default_id()).borrow_raw() == Font::default_font().borrow_raw());

    const FontId body{1};
    Font f14 = Font::builtin(BuiltinFont::Montserrat14);
    assert(reg.register_font(body, f14));
    assert(reg.count() == 1U);
    assert(reg.lookup(body).borrow_raw() == f14.borrow_raw());

    // Re-registering the same id replaces in place (no new slot).
    assert(reg.register_font(body, Font::default_font()));
    assert(reg.count() == 1U);
    assert(reg.lookup(body).borrow_raw() == Font::default_font().borrow_raw());

    // An unregistered, non-default id resolves to an empty Font.
    assert(reg.lookup(FontId{99}).empty());
}

// Over-capacity registration fails visibly (no silent drop).
void test_over_capacity() {
    FontRegistry<2> reg;
    assert(reg.register_font(FontId{1}, Font::default_font()));
    assert(reg.register_font(FontId{2}, Font::default_font()));
    assert(reg.count() == 2U);
    // Full and id is new -> false.
    assert(!reg.register_font(FontId{3}, Font::default_font()));
    assert(reg.count() == 2U);
    // But replacing an existing id still works when full.
    assert(reg.register_font(FontId{1}, Font::builtin(BuiltinFont::Montserrat14)));
    assert(reg.count() == 2U);
}

// apply() resolves an id and feeds it to the object's cascade font.
void test_apply() {
    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());

    FontRegistry<> reg;
    const FontId body{1};
    Font f14 = Font::builtin(BuiltinFont::Montserrat14);
    assert(reg.register_font(body, f14));

    assert(reg.apply(obj, body, style::Selector{style::Part::Main}));
    assert(obj.text_font().borrow_raw() == f14.borrow_raw());

    // Applying an unregistered id resolves empty -> returns false, no change.
    assert(!reg.apply(obj, FontId{42}, style::Selector{style::Part::Main}));
    assert(obj.text_font().borrow_raw() == f14.borrow_raw());

    // Applying the default id always works.
    assert(reg.apply(obj, FontId::default_id(), style::Selector{style::Part::Main}));
    assert(obj.text_font().borrow_raw() == Font::default_font().borrow_raw());
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

    test_font_id();
    test_register_lookup();
    test_over_capacity();
    test_apply();

    lv_display_delete(disp);
    return 0;
}
