// draw_helpers.hpp — panel header renderer + close-button hit test.
//
// PARITY: rlvgl/ui/src/draw_helpers.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (composite UI helper, not an LVGL primitive).
// DELTA:  free functions in lvglpp::ui; &str -> std::string_view. No behavioral delta.
//
// docs/disco-demo/02-ui-draw-helpers.md (DEMO-02).

#ifndef LVGLPP_UI_DRAW_HELPERS_HPP
#define LVGLPP_UI_DRAW_HELPERS_HPP

#include <cstdint>
#include <string_view>

#include "lvglpp/core/font.hpp"      // BitmapFont
#include "lvglpp/core/renderer.hpp"  // Renderer
#include "lvglpp/core/widget.hpp"    // Color, Rect

namespace lvglpp::ui {

// Standard panel padding. Mirrors rlvgl/ui/src/draw_helpers.rs:27
// (PANEL_PADDING). FROZEN — must equal the rlvgl value.
inline constexpr std::int32_t kPanelPadding = 20;

// Standard close-button hit area — a square the height of the title
// area. Mirrors rlvgl/ui/src/draw_helpers.rs:29 (CLOSE_SIZE). FROZEN.
inline constexpr std::int32_t kCloseSize = 48;

// Draw a standard panel header: accent bar, title, close "X", divider.
//
// Mirrors rlvgl/ui/src/draw_helpers.rs:41 byte-for-byte: accent bar
// 72x8 at +kPanelPadding (radius 4), title at
// +kPanelPadding,+kPanelPadding+20, close "X" near the right edge,
// divider at title_y + font.scaled_height() + 12.
//
// Args:
//   r:             borrows mut Renderer for the duration of the call;
//                  never retained (DEMO-00 §5).
//   bounds:        panel outer rectangle.
//   accent:        accent-bar color.
//   title:         borrows; header title text, must outlive the call.
//   font:          borrows; bitmap font for the title and close "X".
//   title_color / close_color / divider_color: styling.
// Returns:
//   the Y coordinate below the divider — callers render body content
//   starting there (div_y + 12).
std::int32_t draw_panel_header(core::Renderer& r,
                               core::Rect bounds,
                               core::Color accent,
                               std::string_view title,
                               const core::BitmapFont& font,
                               core::Color title_color,
                               core::Color close_color,
                               core::Color divider_color);

// Close-button hit test for panels using draw_panel_header. The hit
// area is a kCloseSize square anchored at the panel's top-right corner
// (inside padding). Mirrors rlvgl/ui/src/draw_helpers.rs:98.
//
// Args:
//   bounds: panel outer rectangle.
//   x, y:   pointer coordinates to test.
// Returns:
//   true iff (x, y) falls inside the close-button square.
[[nodiscard]] bool panel_close_hit(core::Rect bounds,
                                   std::int32_t x,
                                   std::int32_t y) noexcept;

}  // namespace lvglpp::ui

#endif  // LVGLPP_UI_DRAW_HELPERS_HPP
