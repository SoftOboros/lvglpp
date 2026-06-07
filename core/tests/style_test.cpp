// style_test.cpp — CORE-05 acceptance: Style defaults, StyleBuilder
// chain, Theme + Light/Dark, Easing math parity, LoopMode shape.

#include "lvglpp/core/style.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

using namespace lvglpp::core;

namespace {

// -------------------------------------------------------------------
// Style defaults and StyleBuilder chain
// -------------------------------------------------------------------

void test_style_defaults() {
    Style s{};
    assert(s.bg_color     == (Color{255, 255, 255, 255}));
    assert(s.border_color == (Color{0,   0,   0,   255}));
    assert(s.border_width == 0);
    assert(s.alpha        == 255);
    assert(s.radius       == 0);
}

void test_style_builder_chain() {
    StyleBuilder b;
    Style s = b.bg_color(Color{10, 20, 30, 200})
              .border_color(Color{40, 50, 60, 200})
              .border_width(3)
              .alpha(128)
              .radius(7)
              .build();
    assert(s.bg_color     == (Color{10, 20, 30, 200}));
    assert(s.border_color == (Color{40, 50, 60, 200}));
    assert(s.border_width == 3);
    assert(s.alpha        == 128);
    assert(s.radius       == 7);
}

// -------------------------------------------------------------------
// Theme: LightTheme / DarkTheme
// -------------------------------------------------------------------

void test_themes_parity_with_rlvgl() {
    Style s{};
    LightTheme light;
    light.apply(s);
    assert(s.bg_color     == (Color{255, 255, 255, 255}));
    assert(s.border_color == (Color{0,   0,   0,   255}));

    Style s2{};
    DarkTheme dark;
    dark.apply(s2);
    assert(s2.bg_color     == (Color{0,   0,   0,   255}));
    assert(s2.border_color == (Color{255, 255, 255, 255}));
}

// -------------------------------------------------------------------
// Easing math parity (concepts doc §5.5 — IEEE-754-equivalent).
// -------------------------------------------------------------------

bool approx(float a, float b) noexcept {
    // Single-precision rounding tolerance. The expressions are pure
    // multiplication/subtraction with no transcendentals, so they
    // should match bit-exactly on the same target — but we use a
    // small epsilon to tolerate compile-time constant-folding
    // ordering differences across optimizers.
    return std::fabs(a - b) <= 1e-6F * (1.0F + std::fabs(b));
}

void test_easing_endpoints_are_zero_and_one() {
    // Every curve except Bounce/Step should map 0→0 and 1→1.
    const Easing::Kind curves[] = {
        Easing::Kind::Linear,
        Easing::Kind::EaseIn,
        Easing::Kind::EaseOut,
        Easing::Kind::EaseInOut,
        Easing::Kind::EaseInCubic,
        Easing::Kind::EaseOutCubic,
        Easing::Kind::EaseInOutCubic,
    };
    for (auto k : curves) {
        Easing e{k};
        assert(approx(e.apply(0.0F), 0.0F));
        assert(approx(e.apply(1.0F), 1.0F));
    }
}

void test_easing_linear_is_identity() {
    Easing e{Easing::Kind::Linear};
    assert(approx(e.apply(0.25F), 0.25F));
    assert(approx(e.apply(0.5F),  0.5F));
    assert(approx(e.apply(0.75F), 0.75F));
}

void test_easing_easein_is_quadratic() {
    Easing e{Easing::Kind::EaseIn};
    assert(approx(e.apply(0.5F), 0.25F));   // 0.5^2
    assert(approx(e.apply(0.25F), 0.0625F));
}

void test_easing_easeinout_at_half() {
    // 3t² − 2t³ at t=0.5 → 3*0.25 - 2*0.125 = 0.75 - 0.25 = 0.5
    Easing e{Easing::Kind::EaseInOut};
    assert(approx(e.apply(0.5F), 0.5F));
}

void test_easing_easeinoutcubic_at_quarter_and_three_quarters() {
    Easing e{Easing::Kind::EaseInOutCubic};
    // t=0.25 (< 0.5) → 4*t³ = 4*0.015625 = 0.0625
    assert(approx(e.apply(0.25F), 0.0625F));
    // t=0.75 → 1 - (-2*0.75 + 2)³/2 = 1 - 0.5³/2 = 1 - 0.0625 = 0.9375
    assert(approx(e.apply(0.75F), 0.9375F));
}

void test_easing_step_quantizes() {
    Easing e4{Easing::Kind::Step, /*step_n=*/4};
    // Step(4): t in [0,0.25) → 0; [0.25, 0.5) → 0.25; etc.
    assert(approx(e4.apply(0.0F),  0.0F));
    assert(approx(e4.apply(0.2F),  0.0F));
    assert(approx(e4.apply(0.3F),  0.25F));
    assert(approx(e4.apply(0.6F),  0.5F));
    assert(approx(e4.apply(0.9F),  0.75F));
    assert(approx(e4.apply(1.0F),  1.0F));
}

void test_easing_step_zero_is_identity() {
    // Step(0) → identity (concepts doc §5.5).
    Easing e0{Easing::Kind::Step, 0};
    assert(approx(e0.apply(0.42F), 0.42F));
}

void test_easing_clamps_input() {
    Easing e{Easing::Kind::Linear};
    assert(approx(e.apply(-1.0F), 0.0F));  // clamped low
    assert(approx(e.apply( 5.0F), 1.0F));  // clamped high
}

// -------------------------------------------------------------------
// LoopMode shape
// -------------------------------------------------------------------

void test_loop_mode_defaults() {
    LoopMode m;
    assert(m.kind()  == LoopMode::Kind::Once);
    assert(m.count() == 0);
}

void test_loop_mode_explicit() {
    LoopMode r{LoopMode::Kind::Repeat, 3};
    assert(r.kind()  == LoopMode::Kind::Repeat);
    assert(r.count() == 3);

    LoopMode pp{LoopMode::Kind::PingPong, 0};
    assert(pp.kind()  == LoopMode::Kind::PingPong);
    assert(pp.count() == 0);  // 0 = infinite per §5.4.
}

}  // namespace

int main() {
    test_style_defaults();
    test_style_builder_chain();
    test_themes_parity_with_rlvgl();
    test_easing_endpoints_are_zero_and_one();
    test_easing_linear_is_identity();
    test_easing_easein_is_quadratic();
    test_easing_easeinout_at_half();
    test_easing_easeinoutcubic_at_quarter_and_three_quarters();
    test_easing_step_quantizes();
    test_easing_step_zero_is_identity();
    test_easing_clamps_input();
    test_loop_mode_defaults();
    test_loop_mode_explicit();
    return 0;
}
