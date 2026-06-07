// widget_test.cpp — CORE-03 acceptance: Rect, Color helpers, Widget
// abstract base. Mirrors the exact computation shapes in
// rlvgl/core/src/widget.rs:14, :27, :45, :52.

#include "lvglpp/core/widget.hpp"
#include "lvglpp/core/event.hpp"

#include <cassert>
#include <cstdint>
#include <optional>

using namespace lvglpp::core;

namespace {

void test_rect_defaults_and_equality() {
    Rect a{};
    assert(a.x == 0 && a.y == 0 && a.width == 0 && a.height == 0);
    Rect b{10, 20, 30, 40};
    Rect c{10, 20, 30, 40};
    assert(b == c);
    assert(!(a == b));
}

void test_color_to_argb8888() {
    // (255, 128, 64, 200) → A=200,R=255,G=128,B=64
    Color c{255, 128, 64, 200};
    const std::uint32_t expected =
        (static_cast<std::uint32_t>(200) << 24) |
        (static_cast<std::uint32_t>(255) << 16) |
        (static_cast<std::uint32_t>(128) << 8)  |
        (static_cast<std::uint32_t>(64));
    assert(c.to_argb8888() == expected);
}

void test_color_with_alpha() {
    // rlvgl: a=128, opacity=200 → (128*200)/255 = 100 (truncating).
    Color c{10, 20, 30, 128};
    const Color blended = c.with_alpha(200);
    assert(blended.r == 10 && blended.g == 20 && blended.b == 30);
    assert(blended.a == static_cast<std::uint8_t>((128U * 200U) / 255U));
}

// Concrete widget subclass to exercise the abstract base.
struct StubWidget : Widget {
    Rect bounds_value{};
    int  draw_calls   = 0;
    int  event_calls  = 0;
    bool consume      = false;

    [[nodiscard]] Rect bounds() const override { return bounds_value; }

    void draw(Renderer& /*r*/) const override {
        const_cast<StubWidget*>(this)->draw_calls += 1;
    }

    [[nodiscard]] bool handle_event(const Event& /*e*/) override {
        ++event_calls;
        return consume;
    }
};

void test_widget_default_clear_region_is_nullopt() {
    StubWidget w;
    assert(!w.clear_region().has_value());
}

void test_widget_handle_event_consume_flag() {
    StubWidget w;
    w.consume = false;
    Event tick{event::Tick{}};
    assert(!w.handle_event(tick));

    w.consume = true;
    assert(w.handle_event(tick));
    assert(w.event_calls == 2);
}

void test_widget_bounds_pure_virtual_via_subclass() {
    StubWidget w;
    w.bounds_value = Rect{5, 10, 100, 200};
    Rect b = w.bounds();
    assert(b.width == 100 && b.height == 200);
}

}  // namespace

int main() {
    test_rect_defaults_and_equality();
    test_color_to_argb8888();
    test_color_with_alpha();
    test_widget_default_clear_region_is_nullopt();
    test_widget_handle_event_consume_flag();
    test_widget_bounds_pure_virtual_via_subclass();
    return 0;
}
