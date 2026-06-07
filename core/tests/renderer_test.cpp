// renderer_test.cpp — CORE-04 acceptance: default blend_rect /
// draw_pixels decompose into fill_rect calls per
// rlvgl/core/src/renderer.rs:25, :33.

#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <vector>

using namespace lvglpp::core;

namespace {

// Test-only renderer that records every fill_rect call.
struct RecordingRenderer : Renderer {
    struct Op {
        Rect  rect;
        Color color;
        bool operator==(const Op&) const noexcept = default;
    };
    std::vector<Op> ops;

    void fill_rect(Rect rect, Color color) override {
        ops.push_back(Op{rect, color});
    }

    void draw_text(std::int32_t /*x*/,
                   std::int32_t /*y*/,
                   std::string_view /*text*/,
                   Color /*color*/) override {
        // Not exercised by the default-impl tests; intentionally empty.
    }
};

void test_blend_rect_default_calls_fill_rect() {
    RecordingRenderer r;
    Rect  rect{1, 2, 30, 40};
    Color color{50, 60, 70, 80};

    r.blend_rect(rect, color);
    assert(r.ops.size() == 1);
    assert(r.ops[0].rect  == rect);
    assert(r.ops[0].color == color);
}

void test_draw_pixels_default_decomposes_to_fill_rect() {
    RecordingRenderer r;

    // 2x2 grid of distinct colors.
    std::array<Color, 4> pixels{
        Color{1, 1, 1, 255},
        Color{2, 2, 2, 255},
        Color{3, 3, 3, 255},
        Color{4, 4, 4, 255},
    };
    r.draw_pixels(/*x=*/10, /*y=*/20,
                  std::span<const Color>{pixels.data(), pixels.size()},
                  /*width=*/2, /*height=*/2);

    assert(r.ops.size() == 4);

    // Row-major, top-to-bottom, left-to-right:
    //  pixels[0] → (10, 20)
    //  pixels[1] → (11, 20)
    //  pixels[2] → (10, 21)
    //  pixels[3] → (11, 21)
    assert(r.ops[0].rect == (Rect{10, 20, 1, 1}));
    assert(r.ops[0].color == pixels[0]);
    assert(r.ops[1].rect == (Rect{11, 20, 1, 1}));
    assert(r.ops[1].color == pixels[1]);
    assert(r.ops[2].rect == (Rect{10, 21, 1, 1}));
    assert(r.ops[2].color == pixels[2]);
    assert(r.ops[3].rect == (Rect{11, 21, 1, 1}));
    assert(r.ops[3].color == pixels[3]);
}

void test_draw_pixels_short_buffer_skips_oob() {
    RecordingRenderer r;
    // 2x2 destination requested, but only 2 pixels supplied — the
    // remaining 2 indices are out of bounds and MUST be skipped per
    // rlvgl/core/src/renderer.rs:36 (`if let Some(&c) =
    // pixels.get(idx)`).
    std::array<Color, 2> pixels{Color{9, 9, 9, 9}, Color{8, 8, 8, 8}};
    r.draw_pixels(0, 0,
                  std::span<const Color>{pixels.data(), pixels.size()},
                  2, 2);
    assert(r.ops.size() == 2);
}

}  // namespace

int main() {
    test_blend_rect_default_calls_fill_rect();
    test_draw_pixels_default_decomposes_to_fill_rect();
    test_draw_pixels_short_buffer_skips_oob();
    return 0;
}
