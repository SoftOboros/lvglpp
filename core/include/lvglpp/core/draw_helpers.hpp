// draw_helpers.hpp — composition helpers built on top of Renderer.
//
// PARITY: rlvgl/core/src/draw.rs (v0.2.0 @ d99f793) — draw_widget_bg
//         (line 480), draw_border_straight (line 425).
// LVGL:   N/A.
// DELTA:  rlvgl's draw.rs is a 642-line module covering rounded
//         corners, anti-aliased borders, and arc math. lvglpp ships
//         a **minimal port** here covering only the radius==0 path
//         (axis-aligned bg + straight 1-px border). The
//         rounded-corner / anti-aliased path lands in CORE-04b once
//         the first widget that needs rounded corners arrives;
//         until then, callers MUST set Style::radius == 0 if they
//         want pixel-identical parity with rlvgl.
//
// This file is a CORE-04 sub-phase (CORE-04a). The full draw.rs
// port is governed by docs/core-renderer/ and a follow-up chapter
// docs/core-renderer/01-rounded-draw.md (not yet ratified).

#ifndef LVGLPP_CORE_DRAW_HELPERS_HPP
#define LVGLPP_CORE_DRAW_HELPERS_HPP

#include <cstdint>

#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"  // Color, Rect

namespace lvglpp::core {

namespace detail {

// Draw an axis-aligned rectangle border `width` pixels thick on the
// inside edge of `rect`. Mirrors the radius==0 code path of
// rlvgl/core/src/draw.rs:425 (draw_border_straight) — four
// fill_rect calls along the four edges.
inline void draw_border_straight(Renderer& renderer,
                                 Rect rect,
                                 Color color,
                                 std::uint8_t width) noexcept {
    if (width == 0) return;
    const auto w = static_cast<std::int32_t>(width);
    if (rect.width < 2 * w || rect.height < 2 * w) {
        // Border thicker than half the rect — fall back to a solid
        // fill of the whole rect. Same fallback rlvgl uses
        // implicitly via the four-rect overlap behaviour.
        renderer.fill_rect(rect, color);
        return;
    }
    // Top edge.
    renderer.fill_rect(Rect{rect.x, rect.y, rect.width, w}, color);
    // Bottom edge.
    renderer.fill_rect(
        Rect{rect.x, rect.y + rect.height - w, rect.width, w},
        color);
    // Left edge (between top + bottom).
    renderer.fill_rect(
        Rect{rect.x, rect.y + w, w, rect.height - 2 * w},
        color);
    // Right edge.
    renderer.fill_rect(
        Rect{rect.x + rect.width - w, rect.y + w, w, rect.height - 2 * w},
        color);
}

}  // namespace detail

// fill_rounded_rect — call-shape parity with
// `rlvgl/core/src/draw.rs:49`. The `radius` parameter is accepted
// but **ignored** until CORE-04b lands the actual corner-rounding
// math; current behaviour is `Renderer::fill_rect(rect, color)`
// regardless of radius. Widgets call this with `style.radius` so
// the rlvgl-equivalent draw shape lands today and the rounding
// becomes pixel-correct when CORE-04b ships.
inline void fill_rounded_rect(Renderer& renderer,
                              Rect rect,
                              Color color,
                              std::uint8_t /*radius*/) noexcept {
    // CORE-04a DELTA: rounded rendering deferred to CORE-04b.
    if (color.a == 255) {
        renderer.fill_rect(rect, color);
    } else {
        renderer.blend_rect(rect, color);
    }
}

// Background + border fill for a styled widget rectangle.
//
// Args:
//   renderer: borrows mut Renderer for the duration of the call.
//   rect:     widget bounds (typically bounds() from the widget).
//   style:    borrows Style — bg_color, border_color, alpha, radius,
//             border_width are read.
//
// Behaviour mirrors rlvgl/core/src/draw.rs:480 with the documented
// DELTA: when style.radius > 0 the call falls back to the radius==0
// path (sharp corners) and proceeds. A future CORE-04b sub-phase
// will route the radius>0 case through fill_rounded_rect /
// draw_rounded_border.
inline void draw_widget_bg(Renderer& renderer,
                           Rect rect,
                           const Style& style) noexcept {
    const Color bg = style.bg_color.with_alpha(style.alpha);
    if (bg.a != 0) {
        // CORE-04a DELTA: rounded path deferred — always axis-aligned.
        if (bg.a == 255) {
            renderer.fill_rect(rect, bg);
        } else {
            renderer.blend_rect(rect, bg);
        }
    }
    if (style.border_width > 0) {
        const Color border = style.border_color.with_alpha(style.alpha);
        if (border.a != 0) {
            detail::draw_border_straight(renderer, rect, border,
                                          style.border_width);
        }
    }
}

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_DRAW_HELPERS_HPP
