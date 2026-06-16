// container_test.cpp — DEMO-01 acceptance for Container.
//
// PARITY: rlvgl/widgets/src/container.rs. Mirrors the bounds-echo,
// passive-event, and draw-via-draw_widget_bg behavior.

#include "lvglpp/widgets/legacy/container.hpp"

#include <cassert>
#include <cstdint>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets::legacy;

namespace {

struct RecordingRenderer : lc::Renderer {
    struct FillOp { lc::Rect rect; lc::Color color; };
    std::vector<FillOp> fills;
    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back(FillOp{rect, color});
    }
    void draw_text(std::int32_t, std::int32_t, std::string_view,
                   lc::Color) override {}
};

void test_container_bounds_echo() {
    lw::Container c{lc::Rect{12, 34, 200, 100}};
    assert(c.bounds() == (lc::Rect{12, 34, 200, 100}));
}

// Containers are passive: every event variant returns false and nothing
// mutates.
void test_container_passive_events() {
    lw::Container c{lc::Rect{0, 0, 40, 20}};
    const lc::Event events[] = {
        lc::Event{lc::event::PressRelease{5, 5}},
        lc::Event{lc::event::PressDown{5, 5}},
        lc::Event{lc::event::PointerDown{5, 5}},
        lc::Event{lc::event::PointerMove{5, 5}},
        lc::Event{lc::event::PointerUp{5, 5}},
        lc::Event{lc::event::DoubleTap{5, 5}},
        lc::Event{lc::event::KeyDown{lc::key::Enter{}}},
        lc::Event{lc::event::Tick{}},
    };
    for (const auto& e : events) assert(!c.handle_event(e));
}

// Default Style: opaque white bg, no border -> a single fill of bounds.
void test_container_draw_default_bg() {
    lw::Container c{lc::Rect{0, 0, 40, 20}};
    RecordingRenderer r;
    c.draw(r);

    assert(r.fills.size() == 1);
    assert(r.fills[0].rect == (lc::Rect{0, 0, 40, 20}));
    assert(r.fills[0].color == (lc::Color{255, 255, 255, 255}));
}

// Border adds the four edge fills (draw_border_straight) in the border
// color, on top of the background fill.
void test_container_draw_with_border() {
    lw::Container c{lc::Rect{0, 0, 40, 20}};
    c.style.bg_color     = lc::Color{10, 20, 30, 255};
    c.style.border_color = lc::Color{200, 0, 0, 255};
    c.style.border_width = 2;

    RecordingRenderer r;
    c.draw(r);

    // 1 background + 4 border edges.
    assert(r.fills.size() == 5);
    assert(r.fills[0].color == (lc::Color{10, 20, 30, 255}));

    bool saw_border = false;
    for (const auto& f : r.fills) {
        if (f.color == (lc::Color{200, 0, 0, 255})) saw_border = true;
    }
    assert(saw_border);
}

// Fully transparent background with no border draws nothing.
void test_container_draw_transparent_noop() {
    lw::Container c{lc::Rect{0, 0, 40, 20}};
    c.style.bg_color = lc::Color{0, 0, 0, 0};  // alpha 0
    c.style.alpha    = 255;
    c.style.border_width = 0;

    RecordingRenderer r;
    c.draw(r);
    assert(r.fills.empty());
}

}  // namespace

int main() {
    test_container_bounds_echo();
    test_container_passive_events();
    test_container_draw_default_bg();
    test_container_draw_with_border();
    test_container_draw_transparent_noop();
    return 0;
}
