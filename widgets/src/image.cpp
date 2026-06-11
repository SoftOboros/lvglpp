// image.cpp — Image draw.
//
// PARITY: rlvgl/widgets/src/image.rs:36–44 (v0.2.0 @ 79f730d) —
//         draw_widget_bg then renderer.draw_pixels; the Renderer
//         decides the emission strategy (per-pixel default, DMA2D
//         or bulk-copy overrides).
//
// docs/widgets-image/00-image.md §5.2.

#include "lvglpp/widgets/image.hpp"

#include "lvglpp/core/draw_helpers.hpp"

namespace lvglpp::widgets {

void Image::draw(::lvglpp::core::Renderer& renderer) const {
    ::lvglpp::core::draw_widget_bg(renderer, bounds_, style);
    renderer.draw_pixels(bounds_.x, bounds_.y, pixels_,
                         static_cast<std::uint32_t>(width_),
                         static_cast<std::uint32_t>(height_));
}

}  // namespace lvglpp::widgets
