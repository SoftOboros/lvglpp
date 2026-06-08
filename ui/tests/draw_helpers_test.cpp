// draw_helpers_test.cpp — DEMO-02 acceptance for the UI draw helpers.
//
// Mirrors the rlvgl CountRenderer test (draw_helpers.rs #[cfg(test)])
// plus the close-hit boundary cases required by
// docs/disco-demo/02-ui-draw-helpers.md §5.

#include "lvglpp/ui/draw_helpers.hpp"

#include <cassert>
#include <cstdint>
#include <string_view>
#include <vector>

#include "lvglpp/core/fonts/font_6x10.hpp"

namespace lc = lvglpp::core;
namespace lu = lvglpp::ui;

namespace {

// Records fill_rect calls; draw_text is a no-op (BitmapFont renders via
// fill_rect, so the title/"X" glyphs show up as recorded fills).
struct RecordingRenderer : lc::Renderer {
    struct FillOp {
        lc::Rect rect;
        lc::Color color;
    };
    std::vector<FillOp> fills;
    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back(FillOp{rect, color});
    }
    void draw_text(std::int32_t, std::int32_t, std::string_view,
                   lc::Color) override {}
};

// §5: panel_close_hit on the corner, just-inside, and just-outside
// cases. bounds {0,0,300,200} -> close square cx = 0+300-20-48 = 232,
// cy = 0, extent x in [232,300), y in [0,48).
void test_panel_close_hit_boundaries() {
    const lc::Rect bounds{0, 0, 300, 200};

    // Top-left corner of the close square — first inside pixel.
    assert(lu::panel_close_hit(bounds, 232, 0));
    // Just inside the far corner.
    assert(lu::panel_close_hit(bounds, 299, 47));

    // Just outside on the left edge.
    assert(!lu::panel_close_hit(bounds, 231, 0));
    // Just outside on the right edge (x == bounds.x + width).
    assert(!lu::panel_close_hit(bounds, 300, 0));
    // Just outside on the bottom edge (y == cy + CLOSE_SIZE).
    assert(!lu::panel_close_hit(bounds, 232, 48));
    // Opposite (top-left) corner of the panel.
    assert(!lu::panel_close_hit(bounds, 0, 0));
}

// §5: draw_panel_header returns the body-start y from the frozen
// formula and emits the accent bar (first fill) + divider (last fill).
void test_draw_panel_header_golden() {
    RecordingRenderer r;
    const lc::Rect bounds{0, 0, 300, 200};
    const lc::Color accent{0x33, 0x99, 0xFF, 0xFF};
    const lc::Color title_color{0xFF, 0xFF, 0xFF, 0xFF};
    const lc::Color close_color{0xFF, 0x00, 0x00, 0xFF};
    const lc::Color divider_color{0x80, 0x80, 0x80, 0xFF};

    const std::int32_t body_y = lu::draw_panel_header(
        r, bounds, accent, "Panel", lc::fonts::FONT_6X10, title_color,
        close_color, divider_color);

    // FONT_6X10 is 6x10 @ scale 2 -> scaled_height() == 20.
    // title_y = 0 + 20 + 20 = 40
    // div_y   = 40 + 20 + 12 = 72
    // return  = 72 + 12 = 84
    assert(lc::fonts::FONT_6X10.scaled_height() == 20);
    assert(body_y == 84);

    // Something was drawn (accent bar + title glyphs + "X" + divider).
    assert(!r.fills.empty());

    // First fill is the accent bar: 72x8 @ +kPanelPadding, radius
    // collapses to a plain fill_rect (CORE-04a).
    const lc::Rect accent_rect{0 + lu::kPanelPadding, 0 + lu::kPanelPadding, 72,
                               8};
    assert(r.fills.front().rect == accent_rect);
    assert(r.fills.front().color == accent);

    // Last fill is the divider: width-2*padding x 1 @ div_y.
    const lc::Rect divider_rect{0 + lu::kPanelPadding, 72,
                                300 - lu::kPanelPadding * 2, 1};
    assert(r.fills.back().rect == divider_rect);
    assert(r.fills.back().color == divider_color);
}

// Constants must equal the rlvgl PANEL_PADDING / CLOSE_SIZE.
void test_constants_match_rlvgl() {
    static_assert(lu::kPanelPadding == 20, "PANEL_PADDING parity");
    static_assert(lu::kCloseSize == 48, "CLOSE_SIZE parity");
}

}  // namespace

int main() {
    test_panel_close_hit_boundaries();
    test_draw_panel_header_golden();
    test_constants_match_rlvgl();
    return 0;
}
