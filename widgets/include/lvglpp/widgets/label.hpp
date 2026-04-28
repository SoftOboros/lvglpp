// label.hpp — basic text label widget.
//
// PARITY: rlvgl/widgets/src/label.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/widgets/label/lv_label.h — informative.
//         lvglpp::widgets::Label does not wrap lv_label; it sits at
//         the lvglpp::core::Widget layer and uses Renderer::draw_text
//         directly.
// DELTA:  rlvgl::Label::text() returns &str; lvglpp returns
//         std::string_view (`borrows` lifetime tied to Label).
//
// This file implements docs/widgets-label/00-label.md (WID-01).
// Field set, override set, and draw call sequence are FROZEN per
// the chapter (Standards Action for the draw sequence).

#ifndef LVGLPP_WIDGETS_LABEL_HPP
#define LVGLPP_WIDGETS_LABEL_HPP

#include <string>
#include <string_view>
#include <utility>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets {

// Label — single-line text display. Per WID-01 chapter §5.2 the
// widget does not consume input events.
//
// Ownership: Label owns its text buffer (std::string). The bounds,
// style, and text_color are value types. No raw pointers / no
// external lifetimes here.
class Label final : public ::lvglpp::core::Widget {
public:
    // Args:
    //   text:   owns std::string; consumed.
    //   bounds: position + size relative to parent.
    Label(std::string text, ::lvglpp::core::Rect bounds)
        : bounds_{bounds}, text_{std::move(text)} {}

    // Replace the displayed text.
    // Args:
    //   text: owns std::string; consumed.
    void set_text(std::string text) noexcept {
        text_ = std::move(text);
    }

    // Read-only view of the current text.
    // Returns: borrows; lifetime is the Label's.
    [[nodiscard]] std::string_view text() const noexcept {
        return text_;
    }

    // Public so application code can mutate directly (parity with
    // rlvgl::Label::style which is `pub`).
    ::lvglpp::core::Style style{};
    // Same — text_color is `pub` in rlvgl.
    ::lvglpp::core::Color text_color{0, 0, 0, 255};

    // ---- Widget overrides --------------------------------------------------

    [[nodiscard]] ::lvglpp::core::Rect bounds() const override {
        return bounds_;
    }

    void draw(::lvglpp::core::Renderer& renderer) const override;

    [[nodiscard]] bool handle_event(
        const ::lvglpp::core::Event& /*event*/) override {
        return false;  // Labels never consume input — chapter §5.2.
    }

private:
    ::lvglpp::core::Rect bounds_;
    std::string          text_;
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_LABEL_HPP
