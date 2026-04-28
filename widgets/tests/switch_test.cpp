// switch_test.cpp — WID-03 acceptance for Switch.

#include "lvglpp/widgets/switch.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets;

namespace {

struct RecordingRenderer : lc::Renderer {
    struct FillOp { lc::Rect rect; lc::Color color; };
    std::vector<FillOp> fills;
    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back(FillOp{rect, color});
    }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override {}
};

void test_switch_defaults() {
    lw::Switch s{lc::Rect{0, 0, 40, 20}};
    assert(s.bounds() == (lc::Rect{0, 0, 40, 20}));
    assert(!s.is_on());
}

void test_switch_press_release_toggles() {
    lw::Switch s{lc::Rect{0, 0, 40, 20}};
    lc::Event ev{lc::event::PressRelease{5, 5}};

    assert(s.handle_event(ev));
    assert(s.is_on());
    assert(s.handle_event(ev));
    assert(!s.is_on());
}

void test_switch_outside_ignored() {
    lw::Switch s{lc::Rect{0, 0, 40, 20}};
    lc::Event ev{lc::event::PressRelease{100, 100}};
    assert(!s.handle_event(ev));
    assert(!s.is_on());
}

void test_switch_other_events_ignored() {
    lw::Switch s{lc::Rect{0, 0, 40, 20}};
    const lc::Event others[] = {
        lc::Event{lc::event::PointerDown{5, 5}},
        lc::Event{lc::event::PointerUp{5, 5}},
        lc::Event{lc::event::Tick{}},
    };
    for (const auto& e : others) assert(!s.handle_event(e));
    assert(!s.is_on());
}

// §5.2 draw: track bg + knob (left when off, right when on).
void test_switch_draw_off_knob_on_left() {
    RecordingRenderer r;
    lw::Switch s{lc::Rect{0, 0, 40, 20}};
    s.draw(r);
    // bg fill + knob fill = 2.
    assert(r.fills.size() == 2);
    // Knob is the second fill: left half.
    assert(r.fills[1].rect.x      == 0);
    assert(r.fills[1].rect.width  == 20);  // bounds.width / 2
    assert(r.fills[1].rect.height == 20);
}

void test_switch_draw_on_knob_on_right() {
    RecordingRenderer r;
    lw::Switch s{lc::Rect{0, 0, 40, 20}};
    s.set_on(true);
    s.draw(r);
    assert(r.fills.size() == 2);
    // Knob anchored on right: x = bounds.x + bounds.width - knob_width.
    assert(r.fills[1].rect.x      == 20);
    assert(r.fills[1].rect.width  == 20);
}

}  // namespace

int main() {
    test_switch_defaults();
    test_switch_press_release_toggles();
    test_switch_outside_ignored();
    test_switch_other_events_ignored();
    test_switch_draw_off_knob_on_left();
    test_switch_draw_on_knob_on_right();
    return 0;
}
