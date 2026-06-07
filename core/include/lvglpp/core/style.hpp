// style.hpp — appearance value types and the Theme abstract base.
//
// PARITY: rlvgl/core/src/style.rs (v0.2.0 @ d99f793) — Style (line 5),
//         StyleBuilder (line 33).
//         rlvgl/core/src/theme.rs (v0.2.0 @ d99f793) — Theme trait
//         (line 11), LightTheme (line 17), DarkTheme (line 27).
//         rlvgl/core/src/animation.rs:19, :102 — Easing, LoopMode.
// LVGL:   lvgl/src/misc/lv_style.h — informative. lvglpp::Style is
//         deliberately a smaller, fixed-field value type; richer
//         LVGL styling reaches into LVGL directly inside per-widget
//         translation units.
// DELTA:  rlvgl uses Rust enums with `match`; lvglpp uses
//         enum-class-with-payload (Step) tagged form for Easing and
//         LoopMode for parity with the rlvgl source's serialization
//         tag indices.
//
// This file implements docs/core-style/00-appearance.md (CORE-05).
// Variant sets and field shapes are FROZEN under Standards Action.

#ifndef LVGLPP_CORE_STYLE_HPP
#define LVGLPP_CORE_STYLE_HPP

#include <cstdint>

#include "lvglpp/core/widget.hpp"  // Color

namespace lvglpp::core {

// ---------------------------------------------------------------------------
// Style — concepts doc §5.1
// ---------------------------------------------------------------------------

struct Style {
    Color        bg_color     = Color{255, 255, 255, 255};
    Color        border_color = Color{0,   0,   0,   255};
    std::uint8_t border_width = 0;
    std::uint8_t alpha        = 255;
    std::uint8_t radius       = 0;

    constexpr bool operator==(const Style&) const noexcept = default;
};

// StyleBuilder — concepts doc §5.2.
//
// Returns reference for chained-call ergonomics (rlvgl uses `mut self`
// and consumes; the C++ form returns `StyleBuilder&` for the same
// fluent shape without the move dance).
class StyleBuilder {
public:
    constexpr StyleBuilder& bg_color(Color c) noexcept {
        style_.bg_color = c;
        return *this;
    }
    constexpr StyleBuilder& border_color(Color c) noexcept {
        style_.border_color = c;
        return *this;
    }
    constexpr StyleBuilder& border_width(std::uint8_t w) noexcept {
        style_.border_width = w;
        return *this;
    }
    constexpr StyleBuilder& alpha(std::uint8_t a) noexcept {
        style_.alpha = a;
        return *this;
    }
    constexpr StyleBuilder& radius(std::uint8_t r) noexcept {
        style_.radius = r;
        return *this;
    }

    // Returns: owns Style by value.
    [[nodiscard]] constexpr Style build() const noexcept {
        return style_;
    }

private:
    Style style_{};
};

// ---------------------------------------------------------------------------
// Theme — concepts doc §5.6
// ---------------------------------------------------------------------------

class Theme {
public:
    Theme()                                = default;
    Theme(const Theme&)                    = default;
    Theme(Theme&&) noexcept                = default;
    Theme& operator=(const Theme&)         = default;
    Theme& operator=(Theme&&) noexcept     = default;
    virtual ~Theme()                       = default;

    // Mutate the supplied style in place.
    // Args:
    //   style: borrows mut Style for the duration of the call.
    virtual void apply(Style& style) const = 0;
};

// Concrete defaults — bg/border swap to the theme's palette,
// other Style fields untouched. Mirrors rlvgl/core/src/theme.rs:17, :27.
class LightTheme final : public Theme {
public:
    void apply(Style& style) const override {
        style.bg_color     = Color{255, 255, 255, 255};
        style.border_color = Color{0,   0,   0,   255};
    }
};

class DarkTheme final : public Theme {
public:
    void apply(Style& style) const override {
        style.bg_color     = Color{0,   0,   0,   255};
        style.border_color = Color{255, 255, 255, 255};
    }
};

// ---------------------------------------------------------------------------
// Easing — concepts doc §5.3
// ---------------------------------------------------------------------------

// Easing curve. The Step variant carries an integer step count; all
// others are unit. The C++ form uses an enum class for the kind plus
// a uint8_t payload for Step; this satisfies "MAY use std::variant or
// a tagged form" per §12.
class Easing {
public:
    enum class Kind : std::uint8_t {
        Linear,         // default
        EaseIn,
        EaseOut,
        EaseInOut,
        EaseInCubic,
        EaseOutCubic,
        EaseInOutCubic,
        Bounce,
        Step,           // value = step count (0 → no quantize)
    };

    constexpr Easing() noexcept = default;
    constexpr explicit Easing(Kind k, std::uint8_t step_n = 0) noexcept
        : kind_{k}, step_n_{step_n} {}

    [[nodiscard]] constexpr Kind         kind()   const noexcept { return kind_; }
    [[nodiscard]] constexpr std::uint8_t step_n() const noexcept { return step_n_; }

    constexpr bool operator==(const Easing&) const noexcept = default;

    // Apply the easing curve to linear progress t in [0, 1].
    // Mirrors rlvgl/core/src/animation.rs:45 — same polynomial
    // expressions, no <cmath> dependency, no transcendentals.
    [[nodiscard]] constexpr float apply(float t) const noexcept {
        // Clamp t into [0, 1] without <algorithm> for freestanding
        // posture purity.
        if (t < 0.0F) t = 0.0F;
        if (t > 1.0F) t = 1.0F;

        switch (kind_) {
            case Kind::Linear:
                return t;
            case Kind::EaseIn:
                return t * t;
            case Kind::EaseOut: {
                const float inv = 1.0F - t;
                return 1.0F - inv * inv;
            }
            case Kind::EaseInOut:
                return t * t * (3.0F - 2.0F * t);
            case Kind::EaseInCubic:
                return t * t * t;
            case Kind::EaseOutCubic: {
                const float inv = 1.0F - t;
                return 1.0F - inv * inv * inv;
            }
            case Kind::EaseInOutCubic:
                if (t < 0.5F) {
                    return 4.0F * t * t * t;
                } else {
                    const float inv = -2.0F * t + 2.0F;
                    return 1.0F - inv * inv * inv / 2.0F;
                }
            case Kind::Bounce: {
                float bt = 1.0F - t;
                float out;
                if (bt < 1.0F / 2.75F) {
                    out = 7.5625F * bt * bt;
                } else if (bt < 2.0F / 2.75F) {
                    bt -= 1.5F / 2.75F;
                    out = 7.5625F * bt * bt + 0.75F;
                } else if (bt < 2.5F / 2.75F) {
                    bt -= 2.25F / 2.75F;
                    out = 7.5625F * bt * bt + 0.9375F;
                } else {
                    bt -= 2.625F / 2.75F;
                    out = 7.5625F * bt * bt + 0.984375F;
                }
                return 1.0F - out;
            }
            case Kind::Step:
                if (step_n_ == 0) return t;
                {
                    const auto steps = static_cast<float>(step_n_);
                    const auto idx   = static_cast<std::uint32_t>(t * steps);
                    return static_cast<float>(idx) / steps;
                }
        }
        return t;  // unreachable — defensive
    }

private:
    Kind         kind_   = Kind::Linear;
    std::uint8_t step_n_ = 0;
};

// ---------------------------------------------------------------------------
// LoopMode — concepts doc §5.4
// ---------------------------------------------------------------------------

class LoopMode {
public:
    enum class Kind : std::uint8_t {
        Once,      // default
        Repeat,    // count = 0 → infinite
        PingPong,  // count = 0 → infinite round-trips
    };

    constexpr LoopMode() noexcept = default;
    constexpr explicit LoopMode(Kind k, std::uint16_t count = 0) noexcept
        : kind_{k}, count_{count} {}

    [[nodiscard]] constexpr Kind          kind()  const noexcept { return kind_; }
    [[nodiscard]] constexpr std::uint16_t count() const noexcept { return count_; }

    constexpr bool operator==(const LoopMode&) const noexcept = default;

private:
    Kind          kind_  = Kind::Once;
    std::uint16_t count_ = 0;
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_STYLE_HPP
