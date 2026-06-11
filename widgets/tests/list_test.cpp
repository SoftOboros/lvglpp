// list_test.cpp — WID-05 acceptance for List.
//
// PARITY: mirrors rlvgl/widgets/tests/list_event.rs (index/boundary
// selection), list_draw_selected.rs (selected item drawn with
// style.border_color), golden_list.rs (bg fill present).

#include "lvglpp/widgets/list.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace lw = lvglpp::widgets;

namespace {

struct RecordingRenderer : lc::Renderer {
    struct FillOp { lc::Rect rect; lc::Color color; };
    struct TextOp {
        std::int32_t x, y;
        std::string text;
        lc::Color color;
    };
    std::vector<FillOp> fills;
    std::vector<TextOp> texts;
    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back(FillOp{rect, color});
    }
    void draw_text(std::int32_t x, std::int32_t y, std::string_view t,
                   lc::Color color) override {
        texts.push_back(TextOp{x, y, std::string{t}, color});
    }
};

lw::List make_list() {
    lw::List list{lc::Rect{10, 20, 100, 64}};
    list.add_item("alpha");
    list.add_item("beta");
    list.add_item("gamma");
    return list;
}

// list_event.rs parity: tap row math incl. the y=row-boundary edge.
void test_selection_event() {
    auto list = make_list();
    assert(!list.selected().has_value());

    // Inside row 0.
    assert(list.handle_event(lc::Event{lc::event::PressRelease{15, 25}}));
    assert(list.selected().has_value() && *list.selected() == 0);

    // Exactly on the 16px boundary -> row 1 (list_event.rs edge).
    assert(list.handle_event(lc::Event{lc::event::PressRelease{15, 20 + 16}}));
    assert(*list.selected() == 1);

    // Outside x range: ignored, selection unchanged.
    assert(!list.handle_event(lc::Event{lc::event::PressRelease{200, 25}}));
    assert(*list.selected() == 1);

    // Below the populated rows: ignored.
    assert(!list.handle_event(lc::Event{lc::event::PressRelease{15, 20 + 3 * 16}}));
    assert(*list.selected() == 1);

    // Above the widget: ignored.
    assert(!list.handle_event(lc::Event{lc::event::PressRelease{15, 5}}));

    // Non-PressRelease events: ignored.
    assert(!list.handle_event(lc::Event{lc::event::PressDown{15, 25}}));
}

// golden_list.rs parity: default style emits the bounds bg fill;
// items draw at x+2, baseline y + i*16 + 16.
void test_draw_layout() {
    auto list = make_list();
    RecordingRenderer r;
    list.draw(r);
    assert(!r.fills.empty());
    assert(r.fills.front().rect == (lc::Rect{10, 20, 100, 64}));
    assert(r.texts.size() == 3);
    assert(r.texts[0].x == 12 && r.texts[0].y == 36);
    assert(r.texts[1].y == 52 && r.texts[2].y == 68);
    assert(r.texts[0].text == "alpha");
}

// list_draw_selected.rs parity: selected item text uses
// style.border_color; others use text_color.
void test_selected_color() {
    auto list = make_list();
    list.text_color         = lc::Color{1, 2, 3, 255};
    list.style.border_color = lc::Color{200, 100, 50, 255};
    assert(list.handle_event(lc::Event{lc::event::PressRelease{15, 40}}));
    assert(*list.selected() == 1);

    RecordingRenderer r;
    list.draw(r);
    assert(r.texts[0].color == (lc::Color{1, 2, 3, 255}));
    assert(r.texts[1].color == (lc::Color{200, 100, 50, 255}));
    assert(r.texts[2].color == (lc::Color{1, 2, 3, 255}));
}

}  // namespace

int main() {
    test_selection_event();
    test_draw_layout();
    test_selected_color();
    return 0;
}
