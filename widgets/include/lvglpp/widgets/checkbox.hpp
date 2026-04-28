// checkbox.hpp — binary checkbox widget with label.
//
// PARITY: rlvgl/widgets/src/checkbox.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/widgets/checkbox/lv_checkbox.h — informative.
// DELTA:  rlvgl's `text()` returns &str; lvglpp returns
//         std::string_view (`borrows`). `style.radius` is
//         currently ignored by the draw helpers (CORE-04a shim);
//         CORE-04b will land actual rounded corners.
//
// docs/widgets-toggles/00-checkbox-and-switch.md (WID-03 §5.1).

#ifndef LVGLPP_WIDGETS_CHECKBOX_HPP
#define LVGLPP_WIDGETS_CHECKBOX_HPP

#include <string>
#include <string_view>
#include <utility>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets {

class Checkbox final : public ::lvglpp::core::Widget {
public:
    Checkbox(std::string text, ::lvglpp::core::Rect bounds)
        : bounds_{bounds}, text_{std::move(text)} {}

    void set_text(std::string text) noexcept { text_ = std::move(text); }
    [[nodiscard]] std::string_view text() const noexcept { return text_; }

    [[nodiscard]] bool is_checked() const noexcept { return checked_; }
    void set_checked(bool value) noexcept { checked_ = value; }

    // Public — application code styles directly (parity with rlvgl
    // `pub` fields).
    ::lvglpp::core::Style style{};
    ::lvglpp::core::Color text_color{0, 0, 0, 255};
    ::lvglpp::core::Color check_color{0, 0, 0, 255};

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
    std::string          text_;
    bool                 checked_ = false;
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_CHECKBOX_HPP
