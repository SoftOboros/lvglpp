// image_test.cpp — WID-06 acceptance for Image.
//
// PARITY: mirrors rlvgl/widgets/tests/golden_image.rs — a 2x2
// [red, green, blue, white] buffer reproduces exactly at the widget
// origin through the default per-pixel draw_pixels path.

#include "lvglpp/widgets/image.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>


namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets;

namespace {

// Minimal 8x8 memory surface driven via the Renderer default
// draw_pixels -> fill_rect path.
struct MemorySurface : lc::Renderer {
    std::array<lc::Color, 64> px{};
    void fill_rect(lc::Rect rect, lc::Color color) override {
        for (std::int32_t y = rect.y; y < rect.y + rect.height; ++y) {
            for (std::int32_t x = rect.x; x < rect.x + rect.width; ++x) {
                if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                    px[static_cast<std::size_t>(y) * 8 +
                       static_cast<std::size_t>(x)] = color;
                }
            }
        }
    }
    void draw_text(std::int32_t, std::int32_t, std::string_view,
                   lc::Color) override {}
};

void test_golden_2x2() {
    const std::array<lc::Color, 4> pixels{
        lc::Color{255, 0, 0, 255}, lc::Color{0, 255, 0, 255},
        lc::Color{0, 0, 255, 255}, lc::Color{255, 255, 255, 255}};
    lw::Image img{lc::Rect{3, 4, 2, 2}, 2, 2,
                  std::span<const lc::Color>{pixels}};
    img.style.alpha = 0;  // suppress the bg fill; isolate the blit

    MemorySurface surf;
    img.draw(surf);
    assert((surf.px[4 * 8 + 3] == pixels[0]));
    assert((surf.px[4 * 8 + 4] == pixels[1]));
    assert((surf.px[5 * 8 + 3] == pixels[2]));
    assert((surf.px[5 * 8 + 4] == pixels[3]));
    // A neighbour stays untouched.
    assert((surf.px[0] == lc::Color{}));
}

void test_passive_and_accessors() {
    const std::array<lc::Color, 1> pixels{lc::Color{9, 9, 9, 255}};
    lw::Image img{lc::Rect{0, 0, 1, 1}, 1, 1,
                  std::span<const lc::Color>{pixels}};
    assert(img.width() == 1 && img.height() == 1);
    assert(img.pixels().size() == 1);
    assert(!img.handle_event(lc::Event{lc::event::PressRelease{0, 0}}));
}

}  // namespace

int main() {
    test_golden_2x2();
    test_passive_and_accessors();
    return 0;
}
