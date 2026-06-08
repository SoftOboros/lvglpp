// event_window_test.cpp — DEMO-03 acceptance for EventWindow.

#include "lvglpp/ui/event_window.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "lvglpp/core/fonts/font_6x10.hpp"

namespace lc = lvglpp::core;
namespace lu = lvglpp::ui;

namespace {

struct RecordingRenderer : lc::Renderer {
    struct FillOp {
        lc::Rect rect;
        lc::Color color;
    };
    std::vector<FillOp> fills;
    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back(FillOp{rect, color});
    }
    void draw_text(std::int32_t, std::int32_t, std::string_view,
                   lc::Color) override {}
};

lc::Event tick() { return lc::Event{lc::event::Tick{}}; }

// Expected geometry for FONT_6X10 (scaled_height 20 -> line_h 24,
// window_h = kMaxLines * 24 + 24 = 264).
constexpr std::int32_t kLineH = 20 + 4;
constexpr std::int32_t kWindowH =
    static_cast<std::int32_t>(lu::kMaxLines) * kLineH + 24;

// Builder centers, honors width, applies colors.
void test_builder_centering_width_colors() {
    const lc::Color bg{10, 20, 30, 255};
    const lc::Color border{40, 50, 60, 255};
    lu::EventWindow w = lu::EventWindowBuilder(lc::fonts::FONT_6X10)
                            .width(300)
                            .bg_color(bg)
                            .border_color(border)
                            .center(800, 480)
                            .build();

    // Reveal it so bounds() reports the panel rect (collapses when hidden).
    w.set_enabled(true);
    w.push_event("hi");
    assert(w.is_visible());

    const lc::Rect expected{(800 - 300) / 2, (480 - kWindowH) / 2, 300,
                            kWindowH};
    assert(w.bounds() == expected);

    RecordingRenderer r;
    w.draw(r);
    // First fill is the background (fill_rounded_rect -> fill_rect at a=255).
    assert(!r.fills.empty());
    assert(r.fills[0].rect == expected);
    assert(r.fills[0].color == bg);
    // Second fill is the top border edge.
    assert(r.fills.size() >= 2);
    assert(r.fills[1].color == border);
}

// push_event appends, caps at kMaxLines (oldest dropped), sets visible.
void test_push_event_cap_and_visible() {
    lu::EventWindow w = lu::EventWindowBuilder(lc::fonts::FONT_6X10).build();
    assert(!w.is_visible());
    w.set_enabled(true);
    for (std::size_t i = 0; i < lu::kMaxLines + 5; ++i) {
        w.push_event("e" + std::to_string(i));
    }
    assert(w.entry_count() == lu::kMaxLines);
    assert(w.is_visible());
}

// Disabled window drops events.
void test_disabled_drops_events() {
    lu::EventWindow w = lu::EventWindowBuilder(lc::fonts::FONT_6X10).build();
    w.push_event("dropped");  // enabled defaults false
    assert(w.entry_count() == 0);
    assert(!w.is_visible());
}

// Ticks past expire_ticks expire entries; emptying hides and arms
// clear_region for kClearFrames, then nullopt.
void test_tick_expiry_hide_and_clear_region() {
    lu::EventWindow w =
        lu::EventWindowBuilder(lc::fonts::FONT_6X10).expire_ticks(3).build();
    w.set_enabled(true);
    w.push_event("transient");
    assert(w.is_visible());
    assert(w.entry_count() == 1);

    const lc::Rect panel = w.bounds();  // full rect while visible
    assert(panel.width > 0 && panel.height > 0);

    // age 0 -> 1 -> 2 -> 3 (>= expire_ticks): removed on the 3rd tick.
    auto e = tick();
    assert(!w.handle_event(e));  // age 1
    assert(w.entry_count() == 1);
    assert(!w.handle_event(e));  // age 2
    assert(w.entry_count() == 1);
    assert(!w.handle_event(e));  // age 3 -> expired
    assert(w.entry_count() == 0);
    assert(!w.is_visible());

    // bounds() now collapses to zero, but clear_region paints the full
    // panel rect for kClearFrames frames.
    assert(w.bounds() == lc::Rect{});
    for (std::uint8_t i = 0; i < lu::kClearFrames; ++i) {
        std::optional<lc::Rect> cr = w.clear_region();
        assert(cr.has_value());
        assert(*cr == panel);
    }
    assert(!w.clear_region().has_value());
}

// bounds() is zero when hidden.
void test_bounds_zero_when_hidden() {
    lu::EventWindow w = lu::EventWindowBuilder(lc::fonts::FONT_6X10)
                            .center(800, 480)
                            .build();
    assert(!w.is_visible());
    assert(w.bounds() == lc::Rect{});
}

// Pointer / key events are non-consuming, visible or not.
void test_non_consuming_pointer_key() {
    lu::EventWindow w = lu::EventWindowBuilder(lc::fonts::FONT_6X10).build();
    w.set_enabled(true);
    w.push_event("visible now");
    assert(w.is_visible());

    lc::Event press{lc::event::PressRelease{5, 5}};
    lc::Event key{lc::event::KeyDown{lc::Key{lc::key::Enter{}}}};
    assert(!w.handle_event(press));
    assert(!w.handle_event(key));
    // Still visible; events were ignored, not consumed.
    assert(w.is_visible());
}

}  // namespace

int main() {
    test_builder_centering_width_colors();
    test_push_event_cap_and_visible();
    test_disabled_drops_events();
    test_tick_expiry_hide_and_clear_region();
    test_bounds_zero_when_hidden();
    test_non_consuming_pointer_key();
    return 0;
}
