// button_test.cpp — WID-02 acceptance: handle_event consumes
// PressRelease inside bounds (firing on_click) and ignores
// everything else.

#include "lvglpp/widgets/button.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets;

namespace {

struct RecordingRenderer : lc::Renderer {
    int fill_calls = 0;
    int text_calls = 0;
    void fill_rect(lc::Rect, lc::Color) override { ++fill_calls; }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override { ++text_calls; }
};

void test_button_basic_accessors() {
    lw::Button b{std::string{"Click me"}, lc::Rect{10, 20, 100, 30}};
    assert(b.text() == "Click me");
    assert(b.bounds() == (lc::Rect{10, 20, 100, 30}));
    b.set_text("New");
    assert(b.text() == "New");
}

void test_button_press_release_inside_fires_callback() {
    lw::Button b{std::string{"x"}, lc::Rect{10, 20, 100, 30}};
    int hits = 0;
    b.set_on_click([&](lw::Button&) { ++hits; });

    // Inside bounds.
    lc::Event ev{lc::event::PressRelease{50, 30}};
    bool consumed = b.handle_event(ev);
    assert(consumed);
    assert(hits == 1);
}

void test_button_press_release_outside_does_not_fire() {
    lw::Button b{std::string{"x"}, lc::Rect{10, 20, 100, 30}};
    int hits = 0;
    b.set_on_click([&](lw::Button&) { ++hits; });

    // Above the button.
    lc::Event ev{lc::event::PressRelease{50, 5}};
    bool consumed = b.handle_event(ev);
    assert(!consumed);
    assert(hits == 0);
}

void test_button_press_release_at_edges() {
    // bounds = {10, 20, 100, 30} → x ∈ [10, 110), y ∈ [20, 50).
    lw::Button b{std::string{"x"}, lc::Rect{10, 20, 100, 30}};
    int hits = 0;
    b.set_on_click([&](lw::Button&) { ++hits; });

    // Top-left corner — inside.
    lc::Event hit{lc::event::PressRelease{10, 20}};
    assert(b.handle_event(hit));

    // Just past right edge — outside (half-open interval).
    lc::Event miss_right{lc::event::PressRelease{110, 30}};
    assert(!b.handle_event(miss_right));

    // Just past bottom edge — outside.
    lc::Event miss_bottom{lc::event::PressRelease{50, 50}};
    assert(!b.handle_event(miss_bottom));

    // Only the first invocation was inside-bounds; one click fired.
    assert(hits == 1);
}

void test_button_ignores_other_events() {
    lw::Button b{std::string{"x"}, lc::Rect{0, 0, 100, 100}};
    int hits = 0;
    b.set_on_click([&](lw::Button&) { ++hits; });

    const lc::Event evs[] = {
        lc::Event{lc::event::Tick{}},
        lc::Event{lc::event::PointerDown{10, 10}},
        lc::Event{lc::event::PointerUp{10, 10}},
        lc::Event{lc::event::PointerMove{10, 10}},
        lc::Event{lc::event::PressDown{10, 10}},
        lc::Event{lc::event::DoubleTap{10, 10}},
        lc::Event{lc::event::KeyDown{lc::Key{lc::key::Enter{}}}},
        lc::Event{lc::event::KeyUp{lc::Key{lc::key::Enter{}}}},
    };
    for (const auto& e : evs) {
        assert(!b.handle_event(e));
    }
    assert(hits == 0);
}

void test_button_no_callback_still_consumes() {
    // No on_click registered; PressRelease inside still consumed.
    lw::Button b{std::string{"x"}, lc::Rect{0, 0, 100, 100}};
    lc::Event ev{lc::event::PressRelease{10, 10}};
    assert(b.handle_event(ev));
}

void test_button_draw_delegates_to_label() {
    // Same bounds + text → same renderer call count as a bare Label.
    RecordingRenderer rb;
    lw::Button b{std::string{"x"}, lc::Rect{0, 0, 50, 30}};
    b.draw(rb);

    RecordingRenderer rl;
    lvglpp::widgets::Label l{std::string{"x"}, lc::Rect{0, 0, 50, 30}};
    l.draw(rl);

    assert(rb.fill_calls == rl.fill_calls);
    assert(rb.text_calls == rl.text_calls);
}

}  // namespace

int main() {
    test_button_basic_accessors();
    test_button_press_release_inside_fires_callback();
    test_button_press_release_outside_does_not_fire();
    test_button_press_release_at_edges();
    test_button_ignores_other_events();
    test_button_no_callback_still_consumes();
    test_button_draw_delegates_to_label();
    return 0;
}
