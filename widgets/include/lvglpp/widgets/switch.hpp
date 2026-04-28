// switch.hpp — binary on/off switch with sliding knob.
//
// PARITY: rlvgl/widgets/src/switch.rs (v0.2.0 @ b178cbc).
// LVGL:   lvgl/src/widgets/switch/lv_switch.h — informative.
// DELTA:  `style.radius` ignored by the CORE-04a shim until
//         CORE-04b lands. Knob slide is non-animated.
//
// docs/widgets-toggles/00-checkbox-and-switch.md (WID-03 §5.2).

#ifndef LVGLPP_WIDGETS_SWITCH_HPP
#define LVGLPP_WIDGETS_SWITCH_HPP

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets {

class Switch final : public ::lvglpp::core::Widget {
public:
    explicit Switch(::lvglpp::core::Rect bounds) noexcept
        : bounds_{bounds} {}

    [[nodiscard]] bool is_on() const noexcept { return on_; }
    void set_on(bool value) noexcept { on_ = value; }

    // Public — application code styles directly.
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
    [[nodiscard]] bool inside_bounds(std::int32_t x, std::int32_t y) const noexcept {
        return x >= bounds_.x &&
               x <  bounds_.x + bounds_.width &&
               y >= bounds_.y &&
               y <  bounds_.y + bounds_.height;
    }

    ::lvglpp::core::Rect bounds_;
    bool                 on_ = false;
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_SWITCH_HPP
