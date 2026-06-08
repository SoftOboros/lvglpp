// draw_helpers.cpp — panel header renderer + close-button hit test.
//
// PARITY: rlvgl/ui/src/draw_helpers.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (composite UI helper, not an LVGL primitive).
// DELTA:  free functions in lvglpp::ui; &str -> std::string_view. No behavioral delta.
//
// docs/disco-demo/02-ui-draw-helpers.md (DEMO-02).

#include "lvglpp/ui/draw_helpers.hpp"

#include "lvglpp/core/draw_helpers.hpp"  // fill_rounded_rect

namespace lvglpp::ui {

// Mirrors rlvgl/ui/src/draw_helpers.rs:41-93.
std::int32_t draw_panel_header(core::Renderer& r,
                               core::Rect bounds,
                               core::Color accent,
                               std::string_view title,
                               const core::BitmapFont& font,
                               core::Color title_color,
                               core::Color close_color,
                               core::Color divider_color) {
    // Accent bar.
    core::fill_rounded_rect(
        r,
        core::Rect{bounds.x + kPanelPadding, bounds.y + kPanelPadding, 72, 8},
        accent,
        4);

    // Title.
    const std::int32_t title_y = bounds.y + kPanelPadding + 20;
    font.draw_str(r, bounds.x + kPanelPadding, title_y, title, title_color);

    // Close button "X" — text at the right edge, hit area is a
    // kCloseSize square. BitmapFont chars are 6px wide at scale 1;
    // approximate position (mirrors draw_helpers.rs:75-78).
    const std::int32_t close_text_x =
        bounds.x + bounds.width - kPanelPadding - 12;
    const std::int32_t close_text_y = bounds.y + kPanelPadding;
    font.draw_str(r, close_text_x, close_text_y, "X", close_color);

    // Divider line.
    const std::int32_t div_y = title_y + font.scaled_height() + 12;
    r.fill_rect(
        core::Rect{bounds.x + kPanelPadding, div_y,
                   bounds.width - kPanelPadding * 2, 1},
        divider_color);

    return div_y + 12;  // body content starts here
}

// Mirrors rlvgl/ui/src/draw_helpers.rs:98-102.
bool panel_close_hit(core::Rect bounds, std::int32_t x,
                     std::int32_t y) noexcept {
    const std::int32_t cx = bounds.x + bounds.width - kPanelPadding - kCloseSize;
    const std::int32_t cy = bounds.y;
    return x >= cx && x < bounds.x + bounds.width && y >= cy &&
           y < cy + kCloseSize;
}

}  // namespace lvglpp::ui
