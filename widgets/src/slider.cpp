// slider.cpp — Slider::draw + handle_event.
//
// PARITY: rlvgl/widgets/src/slider.rs:59 / :95.

#include "lvglpp/widgets/slider.hpp"

#include <variant>

#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::widgets {

namespace {
constexpr std::int32_t kTrackHeight = 4;
constexpr std::int32_t kKnobSize    = 10;
}  // namespace

void Slider::draw(::lvglpp::core::Renderer& renderer) const {
    namespace lc = ::lvglpp::core;

    const std::uint8_t a = style.alpha;
    const std::uint8_t r = style.radius;

    // 1. Background.
    lc::draw_widget_bg(renderer, bounds_, style);

    // 2. Track — 4px tall, vertically centred. Pill shape when
    //    style.radius > 0 (CORE-04b will land actual rounding).
    const std::int32_t track_y = bounds_.y + (bounds_.height - kTrackHeight) / 2;
    const lc::Rect     track_rect{bounds_.x, track_y, bounds_.width, kTrackHeight};
    const std::uint8_t track_radius =
        (r > 0) ? static_cast<std::uint8_t>(kTrackHeight / 2) : 0;
    lc::fill_rounded_rect(renderer, track_rect,
                          style.border_color.with_alpha(a),
                          track_radius);

    // 3. Knob — 10×10 centred on position_from_value().
    const std::int32_t knob_x = position_from_value();
    const lc::Rect     knob_rect{
        knob_x - kKnobSize / 2,
        bounds_.y + (bounds_.height - kKnobSize) / 2,
        kKnobSize,
        kKnobSize,
    };
    const std::uint8_t knob_radius =
        (r > 0) ? static_cast<std::uint8_t>(kKnobSize / 2) : 0;
    lc::fill_rounded_rect(renderer, knob_rect,
                          knob_color.with_alpha(a),
                          knob_radius);
}

bool Slider::handle_event(const ::lvglpp::core::Event& event) {
    namespace e = ::lvglpp::core::event;

    const auto* pr = std::get_if<e::PressRelease>(&event);
    if (pr == nullptr) return false;

    if (pr->y < bounds_.y || pr->y >= bounds_.y + bounds_.height) {
        return false;
    }
    if (pr->x < bounds_.x || pr->x >= bounds_.x + bounds_.width) {
        return false;
    }

    const std::int32_t relative = pr->x - bounds_.x;
    const float        ratio    =
        static_cast<float>(relative) / static_cast<float>(bounds_.width);
    const std::int32_t new_value = min_ +
        static_cast<std::int32_t>(static_cast<float>(max_ - min_) * ratio);
    set_value(new_value);
    return true;
}

}  // namespace lvglpp::widgets
