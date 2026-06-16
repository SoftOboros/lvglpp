// label.cpp — Label::draw implementation.
//
// PARITY: rlvgl/widgets/src/label.rs:46 — exact two-call sequence.
//
// docs/widgets-label/00-label.md §5.3 freezes this body under
// Standards Action; reordering or augmenting the calls requires a
// chapter amendment.

#include "lvglpp/widgets/legacy/label.hpp"

#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::widgets::legacy {

void Label::draw(::lvglpp::core::Renderer& renderer) const {
    // 1. Background + border via the CORE-04a helper. Per the
    //    helper's DELTA, rounded corners are deferred — Label
    //    callers wanting pixel-identical parity with rlvgl MUST set
    //    style.radius == 0 today.
    ::lvglpp::core::draw_widget_bg(renderer, bounds_, style);

    // 2. Text baseline anchored at the bottom-left of `bounds`,
    //    color modulated by the widget-level alpha. Mirrors
    //    rlvgl/widgets/src/label.rs:48.
    renderer.draw_text(
        bounds_.x,
        bounds_.y + bounds_.height,
        text_,
        text_color.with_alpha(style.alpha));
}

}  // namespace lvglpp::widgets::legacy
