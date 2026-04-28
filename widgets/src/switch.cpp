// switch.cpp — Switch::draw + handle_event.
//
// PARITY: rlvgl/widgets/src/switch.rs:46 / :72.

#include "lvglpp/widgets/switch.hpp"

#include <variant>

#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::widgets {

void Switch::draw(::lvglpp::core::Renderer& renderer) const {
    namespace lc = ::lvglpp::core;

    const std::uint8_t a = style.alpha;
    const std::uint8_t r = style.radius;

    // 1. Track background.
    lc::draw_widget_bg(renderer, bounds_, style);

    // 2. Knob — half the track width, anchored left or right by state.
    const std::int32_t knob_width = bounds_.width / 2;
    const lc::Rect knob_rect = on_
        ? lc::Rect{bounds_.x + bounds_.width - knob_width,
                   bounds_.y, knob_width, bounds_.height}
        : lc::Rect{bounds_.x, bounds_.y, knob_width, bounds_.height};
    lc::fill_rounded_rect(renderer, knob_rect,
                          knob_color.with_alpha(a), r);
}

bool Switch::handle_event(const ::lvglpp::core::Event& event) {
    namespace e = ::lvglpp::core::event;

    if (const auto* pr = std::get_if<e::PressRelease>(&event)) {
        if (inside_bounds(pr->x, pr->y)) {
            on_ = !on_;
            return true;
        }
    }
    return false;
}

}  // namespace lvglpp::widgets
