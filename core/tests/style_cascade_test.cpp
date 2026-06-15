// style_cascade_test.cpp — LPAR-07 acceptance: RAII Style over lv_style_t,
// Part/State selectors, Object::add_style + local-style setters, and the
// Theme handle.
//
// See docs/core-style/01-style-cascade-theme.md §12.

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style_cascade.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Object;
using lvglpp::core::ObjectState;
using lvglpp::core::Screen;
namespace style = lvglpp::core::style;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// Selector packs (Part, State) into part | state exactly as LVGL expects.
void test_selector_packing() {
    const style::Selector main_default;  // default-constructed == 0
    assert(main_default.raw() == (LV_PART_MAIN | LV_STATE_DEFAULT));

    const style::Selector knob_pressed{style::Part::Knob, ObjectState::Pressed};
    assert(knob_pressed.raw() ==
           (static_cast<lv_style_selector_t>(LV_PART_KNOB) |
            static_cast<lv_style_selector_t>(LV_STATE_PRESSED)));

    const style::Selector state_only{ObjectState::Checked};
    assert(state_only.raw() == static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
}

// A shared Style attached then removed; the Style outlives the object, which
// is the load-bearing ownership rule (§5.1).
void test_shared_style_lifetime() {
    style::Style boxed;
    boxed.set_bg_color(lv_color_hex(0x2080ff))
        .set_bg_opa(LV_OPA_COVER)
        .set_border_color(lv_color_hex(0x000000))
        .set_border_width(2)
        .set_radius(6)
        .set_pad_all(4);
    assert(boxed.borrow_raw() != nullptr);

    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());

    obj.add_style(boxed, style::Selector{style::Part::Main});
    obj.remove_style(boxed, style::Selector{style::Part::Main});
    obj.remove_all_styles();
    // `obj` (and `screen`) are destroyed at scope exit, before `boxed` —
    // honoring "the Style outlives every object it is added to".
}

// reset() clears properties in place without moving the storage, so a Style
// added to a live object can be re-themed safely.
void test_style_reset_stable_address() {
    style::Style s;
    lv_style_t* before = s.borrow_raw();
    s.set_radius(10);
    s.reset();
    s.set_radius(2);
    assert(s.borrow_raw() == before);  // address stable across reset()
}

// Per-object local overrides are copied in — no Style lifetime hazard.
void test_local_overrides() {
    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());

    obj.set_local_bg_color(lv_color_hex(0xff0000), style::Selector{style::Part::Main});
    obj.set_local_bg_opa(LV_OPA_50, style::Selector{});
    obj.set_local_text_color(lv_color_hex(0xffffff), style::Selector{});
    obj.set_local_border_width(1, style::Selector{style::Part::Main, ObjectState::Pressed});
    obj.set_local_radius(8, style::Selector{});
    obj.set_local_pad_all(3, style::Selector{});
}

// Theme handle: default_init returns a live (LVGL-owned) theme; empty handles
// are safe no-ops.
void test_theme(lv_display_t* disp) {
    style::Theme theme = style::Theme::default_init(
        disp, lv_color_hex(0x0000ff), lv_color_hex(0x00ff00), /*dark=*/false,
        lv_font_get_default());
    assert(!theme.empty());
    assert(theme.borrow_raw() != nullptr);
    theme.bind_to_display(disp);

    Screen screen = Screen::make();
    style::Theme::apply_to(screen.view());
    style::Theme from = style::Theme::from(screen.view());
    static_cast<void>(from);  // may be empty depending on apply order; just safe.

    style::Theme empty_theme;
    assert(empty_theme.empty());
    empty_theme.bind_to_display(disp);  // no-op
    empty_theme.set_parent(theme);      // no-op
    style::Theme::apply_to(ObjectView{nullptr});  // no-op
}

// Style ops on an empty (moved-from) Object are no-ops.
void test_empty_object_safe() {
    Object o = Object::make(ObjectView{lv_screen_active()});
    Object moved = std::move(o);
    (void)moved;
    assert(o.empty());

    style::Style s;
    o.add_style(s, style::Selector{});            // no-op, must not crash
    o.remove_style(s, style::Selector{});         // no-op
    o.remove_all_styles();                        // no-op
    o.set_local_radius(2, style::Selector{});     // no-op
    o.set_local_bg_color(lv_color_hex(0), style::Selector{});  // no-op
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

    test_selector_packing();
    test_shared_style_lifetime();
    test_style_reset_stable_address();
    test_local_overrides();
    test_theme(disp);
    test_empty_object_safe();

    lv_display_delete(disp);
    return 0;
}
