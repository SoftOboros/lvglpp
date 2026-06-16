// checkbox_test.cpp — WID-03 acceptance for Checkbox.

#include "lvglpp/widgets/legacy/checkbox.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets::legacy;

namespace {

struct RecordingRenderer : lc::Renderer {
    int fill_calls = 0;
    int blend_calls = 0;
    int text_calls = 0;
    void fill_rect(lc::Rect, lc::Color) override { ++fill_calls; }
    void blend_rect(lc::Rect, lc::Color) override { ++blend_calls; }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override { ++text_calls; }
};

void test_checkbox_defaults() {
    lw::Checkbox c{std::string{"Accept"}, lc::Rect{10, 10, 100, 20}};
    assert(c.text() == "Accept");
    assert(c.bounds() == (lc::Rect{10, 10, 100, 20}));
    assert(!c.is_checked());
}

void test_checkbox_press_release_inside_toggles() {
    lw::Checkbox c{std::string{"x"}, lc::Rect{10, 10, 100, 20}};
    lc::Event ev{lc::event::PressRelease{50, 15}};

    bool consumed = c.handle_event(ev);
    assert(consumed);
    assert(c.is_checked());

    consumed = c.handle_event(ev);
    assert(consumed);
    assert(!c.is_checked());
}

void test_checkbox_press_release_outside_ignored() {
    lw::Checkbox c{std::string{"x"}, lc::Rect{10, 10, 100, 20}};
    // Above the bounds.
    lc::Event ev{lc::event::PressRelease{50, 0}};
    assert(!c.handle_event(ev));
    assert(!c.is_checked());
}

void test_checkbox_other_events_ignored() {
    lw::Checkbox c{std::string{"x"}, lc::Rect{0, 0, 100, 100}};
    const lc::Event others[] = {
        lc::Event{lc::event::Tick{}},
        lc::Event{lc::event::PointerDown{10, 10}},
        lc::Event{lc::event::PointerUp{10, 10}},
        lc::Event{lc::event::PressDown{10, 10}},
        lc::Event{lc::event::DoubleTap{10, 10}},
        lc::Event{lc::event::KeyDown{lc::Key{lc::key::Enter{}}}},
    };
    for (const auto& e : others) {
        assert(!c.handle_event(e));
    }
    assert(!c.is_checked());
}

void test_checkbox_set_checked_programmatic() {
    lw::Checkbox c{std::string{"x"}, lc::Rect{0, 0, 100, 100}};
    c.set_checked(true);
    assert(c.is_checked());
    c.set_checked(false);
    assert(!c.is_checked());
}

// §5.1 draw sequence: bg + box + (optional check) + text.
void test_checkbox_draw_unchecked_emits_bg_box_text() {
    RecordingRenderer r;
    lw::Checkbox c{std::string{"x"}, lc::Rect{0, 0, 100, 30}};
    c.draw(r);
    // Default style: opaque white bg → 1 fill_rect for bg, 1 for box, 0 for check, 1 draw_text.
    assert(r.fill_calls == 2);
    assert(r.text_calls == 1);
}

void test_checkbox_draw_checked_adds_inner_fill() {
    RecordingRenderer r;
    lw::Checkbox c{std::string{"x"}, lc::Rect{0, 0, 100, 30}};
    c.set_checked(true);
    c.draw(r);
    // bg + box + check inner + text.
    assert(r.fill_calls == 3);
    assert(r.text_calls == 1);
}

}  // namespace

int main() {
    test_checkbox_defaults();
    test_checkbox_press_release_inside_toggles();
    test_checkbox_press_release_outside_ignored();
    test_checkbox_other_events_ignored();
    test_checkbox_set_checked_programmatic();
    test_checkbox_draw_unchecked_emits_bg_box_text();
    test_checkbox_draw_checked_adds_inner_fill();
    return 0;
}
