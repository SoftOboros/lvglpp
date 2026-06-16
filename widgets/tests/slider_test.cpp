// slider_test.cpp — WID-04 acceptance for Slider.

#include "lvglpp/widgets/legacy/slider.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets::legacy;

namespace {

struct RecordingRenderer : lc::Renderer {
    int fill_calls = 0;
    void fill_rect(lc::Rect, lc::Color) override { ++fill_calls; }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override {}
};

void test_slider_defaults() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    assert(s.value() == 0);     // initialised to min
    assert(s.min()   == 0);
    assert(s.max()   == 100);
}

void test_slider_set_value_clamps() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    s.set_value(50);   assert(s.value() == 50);
    s.set_value(-10);  assert(s.value() == 0);
    s.set_value(999);  assert(s.value() == 100);
}

// §5.5 — tap at bounds.x sets value to ~min, tap at bounds.x +
// bounds.width - 1 sets value to ~max.
void test_slider_tap_at_left_sets_min() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    s.set_value(50);
    lc::Event ev{lc::event::PressRelease{0, 10}};
    assert(s.handle_event(ev));
    assert(s.value() == 0);
}

void test_slider_tap_at_right_sets_near_max() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    lc::Event ev{lc::event::PressRelease{99, 10}};
    assert(s.handle_event(ev));
    // ratio = 99/100 → value = 0 + 100*0.99 = 99 (truncated).
    assert(s.value() == 99);
}

void test_slider_tap_at_middle_sets_half() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    lc::Event ev{lc::event::PressRelease{50, 10}};
    assert(s.handle_event(ev));
    assert(s.value() == 50);
}

void test_slider_tap_outside_y_ignored() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    s.set_value(7);
    lc::Event ev{lc::event::PressRelease{50, 100}};  // far below
    assert(!s.handle_event(ev));
    assert(s.value() == 7);  // unchanged
}

void test_slider_tap_outside_x_ignored() {
    lw::Slider s{lc::Rect{10, 0, 100, 20}, 0, 100};
    s.set_value(7);
    lc::Event ev{lc::event::PressRelease{5, 10}};  // before bounds.x
    assert(!s.handle_event(ev));
    assert(s.value() == 7);
}

void test_slider_other_events_ignored() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    const lc::Event others[] = {
        lc::Event{lc::event::PointerDown{50, 10}},
        lc::Event{lc::event::PointerUp{50, 10}},
        lc::Event{lc::event::Tick{}},
    };
    for (const auto& e : others) assert(!s.handle_event(e));
}

void test_slider_negative_range() {
    lw::Slider s{lc::Rect{0, 0, 100, 20}, -50, 50};
    lc::Event mid{lc::event::PressRelease{50, 10}};
    assert(s.handle_event(mid));
    assert(s.value() == 0);  // 0.5 ratio over [-50, 50] → 0
}

void test_slider_degenerate_range_handled() {
    // min == max (range == 0). position_from_value() returns
    // bounds.x; handle_event still updates value via clamp.
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 5, 5};
    assert(s.value() == 5);
    lc::Event ev{lc::event::PressRelease{50, 10}};
    assert(s.handle_event(ev));
    assert(s.value() == 5);  // clamped
}

// §5.4 draw: bg + track + knob = 3 fill_rects (default style:
// opaque white bg → fill_rect; alpha 255 borders → fill_rect).
void test_slider_draw_emits_three_fills() {
    RecordingRenderer r;
    lw::Slider s{lc::Rect{0, 0, 100, 20}, 0, 100};
    s.draw(r);
    assert(r.fill_calls == 3);
}

}  // namespace

int main() {
    test_slider_defaults();
    test_slider_set_value_clamps();
    test_slider_tap_at_left_sets_min();
    test_slider_tap_at_right_sets_near_max();
    test_slider_tap_at_middle_sets_half();
    test_slider_tap_outside_y_ignored();
    test_slider_tap_outside_x_ignored();
    test_slider_other_events_ignored();
    test_slider_negative_range();
    test_slider_degenerate_range_handled();
    test_slider_draw_emits_three_fills();
    return 0;
}
