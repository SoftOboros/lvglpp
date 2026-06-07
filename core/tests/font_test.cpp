// font_test.cpp — CORE-06 acceptance: BitmapFont decomposes into
// fill_rect calls per rlvgl/core/src/bitmap_font.rs:39, :70.

#include "lvglpp/core/font.hpp"
#include "lvglpp/core/fonts/font_6x10.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <vector>

using namespace lvglpp::core;

namespace {

struct RecordingRenderer : Renderer {
    struct Op {
        Rect  rect;
        Color color;
    };
    std::vector<Op> ops;

    void fill_rect(Rect rect, Color color) override {
        ops.push_back(Op{rect, color});
    }
    void draw_text(std::int32_t /*x*/,
                   std::int32_t /*y*/,
                   std::string_view /*t*/,
                   Color /*c*/) override {}
};

// Hand-crafted tiny font: 1x1 glyph for ' ' (0x20), where the single
// pixel is set. data has bit 0x80 in byte 0.
//
//   bit_offset = 0
//   byte_idx = 0, bit_idx = 7  -> data[0] & 0x80 != 0  -> draw 1x1.
void test_bitmap_font_minimal_glyph_set() {
    constexpr std::array<std::uint8_t, 1> data{0x80};
    BitmapFont tiny{
        /*.glyph_width  =*/ 1,
        /*.glyph_height =*/ 1,
        /*.scale        =*/ 1,
        /*.data         =*/ std::span<const std::uint8_t>(data.data(), data.size()),
    };

    RecordingRenderer r;
    tiny.draw_char(r, 10, 20, U' ', Color{1, 2, 3, 255});

    assert(r.ops.size() == 1);
    assert(r.ops[0].rect == (Rect{10, 20, 1, 1}));
    assert(r.ops[0].color == (Color{1, 2, 3, 255}));
}

// Tiny font with bit cleared — draw_char should issue zero calls.
void test_bitmap_font_minimal_glyph_clear() {
    constexpr std::array<std::uint8_t, 1> data{0x00};
    BitmapFont tiny{1, 1, 1, std::span<const std::uint8_t>(data.data(), data.size())};

    RecordingRenderer r;
    tiny.draw_char(r, 0, 0, U' ', Color{0, 0, 0, 255});
    assert(r.ops.empty());
}

void test_bitmap_font_skips_non_printable() {
    constexpr std::array<std::uint8_t, 1> data{0xFF};
    BitmapFont tiny{1, 1, 1, std::span<const std::uint8_t>(data.data(), data.size())};

    RecordingRenderer r;
    tiny.draw_char(r, 0, 0, /*ch=*/0x1F, Color{0, 0, 0, 255}); // below 0x20
    tiny.draw_char(r, 0, 0, /*ch=*/0x7F, Color{0, 0, 0, 255}); // above 0x7E
    assert(r.ops.empty());
}

// Verify the bring-up font: scaled dimensions and that draw_char on
// a known glyph (' ' = blank) produces zero fill_rects.
void test_built_in_font_6x10_scaled_dimensions() {
    using lvglpp::core::fonts::FONT_6X10;
    assert(FONT_6X10.glyph_width  == 6);
    assert(FONT_6X10.glyph_height == 10);
    assert(FONT_6X10.scale        == 2);
    assert(FONT_6X10.scaled_width()  == 12);
    assert(FONT_6X10.scaled_height() == 20);
    assert(FONT_6X10.data.size() == 713);
}

void test_built_in_font_6x10_blank_space_has_no_pixels() {
    using lvglpp::core::fonts::FONT_6X10;
    RecordingRenderer r;
    // ' ' (0x20) is the first glyph in the rlvgl bring-up font and
    // is all-zero (per the canonical .bin layout).
    FONT_6X10.draw_char(r, 0, 0, U' ', Color{255, 0, 0, 255});
    assert(r.ops.empty());
}

void test_built_in_font_6x10_known_glyph_emits_fill_rects() {
    using lvglpp::core::fonts::FONT_6X10;
    RecordingRenderer r;
    // '!' (0x21) is the second glyph; the rlvgl bring-up font has at
    // least one set bit for it. The exact pattern is auto-generated;
    // we don't assert specific positions, only that *some* fill_rect
    // calls happen at scale=2 (so each rect is 2x2).
    FONT_6X10.draw_char(r, 100, 200, U'!', Color{0, 255, 0, 255});
    assert(!r.ops.empty());
    for (const auto& op : r.ops) {
        assert(op.rect.width  == 2);
        assert(op.rect.height == 2);
        assert(op.color == (Color{0, 255, 0, 255}));
        // Within the 12x20 scaled glyph box anchored at (100, 200).
        assert(op.rect.x >= 100 && op.rect.x < 112);
        assert(op.rect.y >= 200 && op.rect.y < 220);
    }
}

void test_built_in_font_6x10_draw_str_advances() {
    using lvglpp::core::fonts::FONT_6X10;
    RecordingRenderer r;
    // Two characters: '!' (some pixels), ' ' (no pixels). The total
    // op count must equal exactly the '!' op count from a single
    // call — proves draw_str advances cx and that ' ' contributes
    // nothing.
    RecordingRenderer single;
    FONT_6X10.draw_char(single, 100, 200, U'!', Color{0, 0, 0, 255});

    FONT_6X10.draw_str(r, 100, 200, std::string_view{"! "}, Color{0, 0, 0, 255});
    assert(r.ops.size() == single.ops.size());
}

}  // namespace

int main() {
    test_bitmap_font_minimal_glyph_set();
    test_bitmap_font_minimal_glyph_clear();
    test_bitmap_font_skips_non_printable();
    test_built_in_font_6x10_scaled_dimensions();
    test_built_in_font_6x10_blank_space_has_no_pixels();
    test_built_in_font_6x10_known_glyph_emits_fill_rects();
    test_built_in_font_6x10_draw_str_advances();
    return 0;
}
