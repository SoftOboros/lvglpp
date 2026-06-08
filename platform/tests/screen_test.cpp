// screen_test.cpp — DEMO-0S acceptance for the Screen display descriptor.

#include "lvglpp/platform/screen.hpp"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace lp = lvglpp::platform;

namespace {

// §5: make(800,480,Deg0) == landscape(800,480), Argb8888, frame_hz == 60.
void test_defaults() {
    const lp::Screen made = lp::Screen::make(800, 480, lp::Rotation::Deg0);
    const lp::Screen land = lp::Screen::landscape(800, 480);
    assert(made == land);
    assert(land.width == 800);
    assert(land.height == 480);
    assert(land.rotation == lp::Rotation::Deg0);
    assert(land.color_format == lp::ColorFormat::Argb8888);
    assert(land.frame_hz == 60);
    assert(lp::DEFAULT_FRAME_HZ == 60);
}

// §5: is_portrait true for Deg90/Deg270 only.
void test_is_portrait() {
    assert(!lp::is_portrait(lp::Rotation::Deg0));
    assert(lp::is_portrait(lp::Rotation::Deg90));
    assert(!lp::is_portrait(lp::Rotation::Deg180));
    assert(lp::is_portrait(lp::Rotation::Deg270));
}

// §5: builder methods return modified copies (value semantics).
void test_with_color_format() {
    const lp::Screen base = lp::Screen::make(480, 320, lp::Rotation::Deg90);
    const lp::Screen tinted = base.with_color_format(lp::ColorFormat::Rgb565);
    // Modified copy.
    assert(tinted.color_format == lp::ColorFormat::Rgb565);
    // Everything else preserved.
    assert(tinted.width == 480);
    assert(tinted.height == 320);
    assert(tinted.rotation == lp::Rotation::Deg90);
    assert(tinted.frame_hz == 60);
    // Original untouched (value semantics).
    assert(base.color_format == lp::ColorFormat::Argb8888);
}

void test_with_frame_hz() {
    const lp::Screen base = lp::Screen::landscape(800, 480);
    const lp::Screen fast = base.with_frame_hz(120);
    assert(fast.frame_hz == 120);
    assert(base.frame_hz == 60);  // original untouched.
    // frame_hz == 0 clamps to 1.
    assert(base.with_frame_hz(0).frame_hz == 1);
}

// operator== behaviour: any differing field compares unequal.
void test_operator_equals() {
    const lp::Screen a = lp::Screen::landscape(800, 480);
    assert(a == a);
    assert(a == lp::Screen::landscape(800, 480));
    assert(!(a == lp::Screen::landscape(801, 480)));
    assert(!(a == lp::Screen::make(800, 480, lp::Rotation::Deg90)));
    assert(!(a == a.with_color_format(lp::ColorFormat::Mono)));
    assert(!(a == a.with_frame_hz(30)));
}

// Trivially-copyable value type (DEMO-00 §5 / DEMO-0S §4).
static_assert(std::is_trivially_copyable_v<lp::Screen>);
// constexpr-usable end to end.
static_assert(lp::Screen::make(800, 480, lp::Rotation::Deg0) ==
              lp::Screen::landscape(800, 480));

}  // namespace

int main() {
    test_defaults();
    test_is_portrait();
    test_with_color_format();
    test_with_frame_hz();
    test_operator_equals();
    return 0;
}
