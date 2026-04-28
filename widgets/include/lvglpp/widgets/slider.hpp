// slider.hpp — horizontal value-bound slider widget.
//
// PARITY: rlvgl/widgets/src/slider.rs (v0.2.0 @ b178cbc).
// LVGL:   lvgl/src/widgets/slider/lv_slider.h — informative.
// DELTA:  PressRelease-only (no drag-tracking — WID-04a deferred).
//         `style.radius` ignored until CORE-04b lands.
//
// docs/widgets-slider/00-slider.md (WID-04).

#ifndef LVGLPP_WIDGETS_SLIDER_HPP
#define LVGLPP_WIDGETS_SLIDER_HPP

#include <algorithm>
#include <cstdint>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets {

class Slider final : public ::lvglpp::core::Widget {
public:
    Slider(::lvglpp::core::Rect bounds,
           std::int32_t min,
           std::int32_t max) noexcept
        : bounds_{bounds},
          min_{min},
          max_{max},
          value_{min} {}

    [[nodiscard]] std::int32_t value() const noexcept { return value_; }
    [[nodiscard]] std::int32_t min()   const noexcept { return min_; }
    [[nodiscard]] std::int32_t max()   const noexcept { return max_; }

    // §5.2 — clamps to [min, max].
    void set_value(std::int32_t v) noexcept {
        value_ = std::clamp(v, min_, max_);
    }

    // Public — application code styles directly (parity with rlvgl).
    ::lvglpp::core::Style style{};
    ::lvglpp::core::Color knob_color{0, 0, 0, 255};

    // ---- Widget overrides --------------------------------------------------

    [[nodiscard]] ::lvglpp::core::Rect bounds() const override {
        return bounds_;
    }

    void draw(::lvglpp::core::Renderer& renderer) const override;

    [[nodiscard]] bool handle_event(
        const ::lvglpp::core::Event& event) override;

private:
    [[nodiscard]] std::int32_t position_from_value() const noexcept {
        const std::int32_t range = max_ - min_;
        if (range == 0) return bounds_.x;
        const float ratio =
            static_cast<float>(value_ - min_) / static_cast<float>(range);
        return bounds_.x +
               static_cast<std::int32_t>(ratio * static_cast<float>(bounds_.width));
    }

    ::lvglpp::core::Rect bounds_;
    std::int32_t         min_;
    std::int32_t         max_;
    std::int32_t         value_;
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_SLIDER_HPP
