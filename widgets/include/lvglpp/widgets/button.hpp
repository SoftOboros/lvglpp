// button.hpp — interactive button widget.
//
// PARITY: rlvgl/widgets/src/button.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/widgets/button/lv_button.h — informative.
// DELTA:  rlvgl uses Box<dyn FnMut(&mut Button)> for the click
//         handler; lvglpp uses std::function<void(Button&)>. Both
//         are heap-erased mutable closures.
//
// This file implements docs/widgets-button/00-button.md (WID-02).

#ifndef LVGLPP_WIDGETS_BUTTON_HPP
#define LVGLPP_WIDGETS_BUTTON_HPP

#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"
#include "lvglpp/widgets/label.hpp"

namespace lvglpp::widgets {

// Clickable button. Composes a Label for visual rendering and adds a
// click callback fired on `PressRelease` within bounds.
class Button final : public ::lvglpp::core::Widget {
public:
    using ClickHandler = std::function<void(Button&)>;

    // Args:
    //   text:   owns std::string (consumed into the inner Label).
    //   bounds: position + size relative to parent.
    Button(std::string text, ::lvglpp::core::Rect bounds)
        : bounds_{bounds}, label_{std::move(text), bounds} {}

    void set_text(std::string text) noexcept {
        label_.set_text(std::move(text));
    }

    [[nodiscard]] std::string_view text() const noexcept {
        return label_.text();
    }

    [[nodiscard]] ::lvglpp::core::Style&
    style() noexcept { return label_.style; }
    [[nodiscard]] const ::lvglpp::core::Style&
    style() const noexcept { return label_.style; }

    [[nodiscard]] ::lvglpp::core::Color&
    text_color() noexcept { return label_.text_color; }
    [[nodiscard]] const ::lvglpp::core::Color&
    text_color() const noexcept { return label_.text_color; }

    // Register the click callback. Args:
    //   handler: owns std::function — consumed via move.
    void set_on_click(ClickHandler handler) noexcept {
        on_click_ = std::move(handler);
    }

    // ---- Widget overrides --------------------------------------------------

    [[nodiscard]] ::lvglpp::core::Rect bounds() const override {
        return bounds_;
    }

    void draw(::lvglpp::core::Renderer& renderer) const override {
        label_.draw(renderer);
    }

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
    Label                label_;
    ClickHandler         on_click_{};
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_BUTTON_HPP
