// container.cpp — Container::draw.
//
// PARITY: rlvgl/widgets/src/container.rs:30 (draw -> draw_widget_bg).

#include "lvglpp/widgets/legacy/container.hpp"

#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::widgets::legacy {

void Container::draw(::lvglpp::core::Renderer& renderer) const {
    // Background + border only; no foreground content. Mirrors
    // container.rs, which draws solely via draw_widget_bg.
    ::lvglpp::core::draw_widget_bg(renderer, bounds_, style);
}

}  // namespace lvglpp::widgets::legacy
