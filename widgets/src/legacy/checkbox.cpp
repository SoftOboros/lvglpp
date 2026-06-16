// checkbox.cpp — Checkbox::draw + handle_event.
//
// PARITY: rlvgl/widgets/src/checkbox.rs:51 / :86. Draw call
//         sequence frozen at WID-03 §5.1.

#include "lvglpp/widgets/legacy/checkbox.hpp"

#include <variant>

#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::widgets::legacy {

namespace {
constexpr std::int32_t kSquareSize = 10;
constexpr std::int32_t kInnerInset = 2;
constexpr std::int32_t kTextOffset = kSquareSize + 4;
}  // namespace

void Checkbox::draw(::lvglpp::core::Renderer& renderer) const {
    namespace lc = ::lvglpp::core;

    const std::uint8_t a = style.alpha;
    const std::uint8_t r = style.radius;

    // 1. Background per §5.1.
    lc::draw_widget_bg(renderer, bounds_, style);

    // 2. 10×10 box anchored at the bounds origin.
    const lc::Rect box_rect{bounds_.x, bounds_.y, kSquareSize, kSquareSize};
    lc::fill_rounded_rect(renderer, box_rect,
                          style.border_color.with_alpha(a), r);

    // 3. Check mark — 6×6 inset 2px on every side.
    if (checked_) {
        const lc::Rect inner{
            box_rect.x + kInnerInset,
            box_rect.y + kInnerInset,
            box_rect.width  - 2 * kInnerInset,
            box_rect.height - 2 * kInnerInset,
        };
        lc::fill_rounded_rect(renderer, inner,
                              check_color.with_alpha(a), r);
    }

    // 4. Label text right of the box; baseline at bottom-left.
    renderer.draw_text(
        bounds_.x + kTextOffset,
        bounds_.y + bounds_.height,
        text_,
        text_color.with_alpha(a));
}

bool Checkbox::handle_event(const ::lvglpp::core::Event& event) {
    namespace e = ::lvglpp::core::event;

    if (const auto* pr = std::get_if<e::PressRelease>(&event)) {
        if (inside_bounds(pr->x, pr->y)) {
            checked_ = !checked_;
            return true;
        }
    }
    return false;
}

}  // namespace lvglpp::widgets::legacy
