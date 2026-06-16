// label_test.cpp — WID-01 acceptance: Label::draw issues exactly the
// two-call sequence in docs/widgets-label/00-label.md §5.3:
//   1. fill_rect (or blend_rect) for the bg
//   2. draw_text at (bounds.x, bounds.y + bounds.height) with
//      text_color.with_alpha(style.alpha).

#include "lvglpp/widgets/legacy/label.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets::legacy;

namespace {

struct RecordingRenderer : lc::Renderer {
    struct FillOp  { lc::Rect rect; lc::Color color; };
    struct BlendOp { lc::Rect rect; lc::Color color; };
    struct TextOp  {
        std::int32_t x;
        std::int32_t y;
        std::string  text;  // copy because the view's lifetime ends with the call
        lc::Color    color;
    };

    std::vector<FillOp>  fills;
    std::vector<BlendOp> blends;
    std::vector<TextOp>  texts;

    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back(FillOp{rect, color});
    }
    void blend_rect(lc::Rect rect, lc::Color color) override {
        blends.push_back(BlendOp{rect, color});
    }
    void draw_text(std::int32_t x,
                   std::int32_t y,
                   std::string_view text,
                   lc::Color color) override {
        texts.push_back(TextOp{x, y, std::string{text}, color});
    }
};

void test_label_basic_construction_and_accessors() {
    lw::Label label{std::string{"hello"}, lc::Rect{1, 2, 30, 40}};
    assert(label.text() == "hello");
    assert(label.bounds() == (lc::Rect{1, 2, 30, 40}));

    // handle_event: Label never consumes per chapter §5.2.
    lc::Event tick{lc::event::Tick{}};
    assert(!label.handle_event(tick));

    label.set_text("world");
    assert(label.text() == "world");
}

void test_label_default_style_yields_opaque_white_bg() {
    // Default Style: bg_color = (255,255,255,255), alpha = 255 →
    // bg.with_alpha(alpha) = (255,255,255,255), opaque path → fill_rect.
    lw::Label label{std::string{"x"}, lc::Rect{10, 20, 50, 60}};
    RecordingRenderer r;
    label.draw(r);

    // Exactly one fill_rect for the bg, zero blend_rects (opaque).
    assert(r.fills.size()  == 1);
    assert(r.blends.empty());

    assert(r.fills[0].rect  == (lc::Rect{10, 20, 50, 60}));
    assert(r.fills[0].color == (lc::Color{255, 255, 255, 255}));

    // Exactly one draw_text call, baseline anchored at bottom-left.
    assert(r.texts.size() == 1);
    assert(r.texts[0].x    == 10);
    assert(r.texts[0].y    == 20 + 60);  // bounds.y + bounds.height
    assert(r.texts[0].text == "x");
    // text_color default (0,0,0,255) modulated by alpha 255 → (0,0,0,255).
    assert(r.texts[0].color == (lc::Color{0, 0, 0, 255}));
}

void test_label_translucent_bg_uses_blend_rect() {
    lw::Label label{std::string{"y"}, lc::Rect{0, 0, 10, 10}};
    label.style.alpha = 128;  // → bg.a = (255*128)/255 = 128, translucent path
    RecordingRenderer r;
    label.draw(r);

    // Translucent bg → blend_rect, no fill_rect for bg.
    assert(r.fills.empty());
    assert(r.blends.size() == 1);
    assert(r.blends[0].color.a == 128);
}

void test_label_zero_alpha_skips_bg_entirely() {
    lw::Label label{std::string{"z"}, lc::Rect{0, 0, 10, 10}};
    label.style.alpha = 0;  // → bg.a = 0, skip path
    RecordingRenderer r;
    label.draw(r);

    assert(r.fills.empty());
    assert(r.blends.empty());
    // draw_text still happens; text_color.with_alpha(0) → a=0 (caller's choice).
    assert(r.texts.size() == 1);
    assert(r.texts[0].color.a == 0);
}

void test_label_border_emits_four_fill_rects() {
    lw::Label label{std::string{"b"}, lc::Rect{0, 0, 100, 100}};
    label.style.border_width = 2;
    // border_color default (0,0,0,255), alpha 255 → opaque border.
    RecordingRenderer r;
    label.draw(r);

    // 1 bg fill + 4 border edges = 5 fills.
    assert(r.fills.size() == 5);
    // First fill is the bg (full rect).
    assert(r.fills[0].rect == (lc::Rect{0, 0, 100, 100}));
    // Remaining 4 are the edges from draw_border_straight.
    // Top, bottom, left, right (per the helper's emit order).
    assert(r.fills[1].rect == (lc::Rect{0, 0, 100, 2}));
    assert(r.fills[2].rect == (lc::Rect{0, 98, 100, 2}));
    assert(r.fills[3].rect == (lc::Rect{0, 2, 2, 96}));
    assert(r.fills[4].rect == (lc::Rect{98, 2, 2, 96}));
}

void test_label_text_alpha_modulated_by_style_alpha() {
    lw::Label label{std::string{"t"}, lc::Rect{0, 0, 10, 10}};
    label.style.alpha    = 128;
    label.text_color     = lc::Color{255, 0, 0, 200};  // semi-transparent red
    RecordingRenderer r;
    label.draw(r);

    // text_color.with_alpha(128): a = (200*128)/255 = 100 (truncating).
    assert(r.texts.size() == 1);
    assert(r.texts[0].color == (lc::Color{255, 0, 0,
                                          static_cast<std::uint8_t>((200U * 128U) / 255U)}));
}

}  // namespace

int main() {
    test_label_basic_construction_and_accessors();
    test_label_default_style_yields_opaque_white_bg();
    test_label_translucent_bg_uses_blend_rect();
    test_label_zero_alpha_skips_bg_entirely();
    test_label_border_emits_four_fill_rects();
    test_label_text_alpha_modulated_by_style_alpha();
    return 0;
}
