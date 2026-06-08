// dashboard_panel_test.cpp — DEMO-05 acceptance for DashboardPanel.
//
// Mirrors the rlvgl dashboard_panel unit behavior: show/hide + is_visible,
// bounds collapse when hidden, close-hit consume on PressRelease, and the
// word-wrap cases tested in dashboard_panel.rs (via the shared text_wrap
// helper).

#include "lvglpp/app/disco_demo/assets.hpp"
#include "lvglpp/app/disco_demo/dashboard_panel.hpp"
#include "lvglpp/app/disco_demo/detail/text_wrap.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace ad = lvglpp::app::disco_demo;

namespace {

struct CountRenderer : lc::Renderer {
    int fills = 0;
    void fill_rect(lc::Rect, lc::Color) override { ++fills; }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override {}
};

const lc::Rect kBounds{ad::PANEL_X, ad::PANEL_Y, ad::PANEL_WIDTH, ad::PANEL_HEIGHT};

void test_show_hide_and_bounds() {
    ad::DashboardPanel panel{kBounds, "Title", "Caption"};
    assert(!panel.is_visible());
    assert(panel.bounds() == (lc::Rect{0, 0, 0, 0}));  // hidden -> collapse

    panel.show();
    assert(panel.is_visible());
    assert(panel.bounds() == kBounds);

    panel.hide();
    assert(!panel.is_visible());
    assert(panel.bounds() == (lc::Rect{0, 0, 0, 0}));
}

void test_close_hit_consume() {
    ad::DashboardPanel panel{kBounds, "Title", "Caption"};

    // Hidden: event ignored.
    const lc::Event close{lc::event::PressRelease{
        kBounds.x + kBounds.width - 10, kBounds.y + 4}};
    assert(!panel.handle_event(close));

    panel.show();
    // Non-close hit (panel center) is not consumed.
    const lc::Event center{lc::event::PressRelease{
        kBounds.x + kBounds.width / 2, kBounds.y + kBounds.height / 2}};
    assert(!panel.handle_event(center));
    assert(panel.is_visible());

    // Top-right close square consumes and hides.
    assert(panel.handle_event(close));
    assert(!panel.is_visible());
}

void test_draw_visibility_gated() {
    ad::DashboardPanel panel{kBounds, "Title", "Caption"};
    CountRenderer hidden;
    panel.draw(hidden);
    assert(hidden.fills == 0);

    panel.show();
    CountRenderer shown;
    panel.draw(shown);
    assert(shown.fills > 0);
}

// wrap_text cases mirror dashboard_panel.rs:240-265.
void test_wrap_long_sentence() {
    const auto lines = ad::detail::wrap_text(
        "the quick brown fox jumps over the lazy dog", 12);
    for (const auto& l : lines) assert(l.size() <= 12);
    // Joining the wrapped lines with spaces reconstructs the word stream.
    std::string joined;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) joined.push_back(' ');
        joined += lines[i];
    }
    assert(joined == "the quick brown fox jumps over the lazy dog");
}

void test_wrap_preserves_newlines() {
    const auto lines = ad::detail::wrap_text("line one\nline two", 40);
    assert(lines.size() == 2);
    assert(lines[0] == "line one");
    assert(lines[1] == "line two");
}

void test_wrap_breaks_long_word() {
    const auto lines = ad::detail::wrap_text("supercalifragilistic", 5);
    const std::vector<std::string> want = {"super", "calif", "ragil", "istic"};
    assert(lines == want);
}

void test_wrap_zero_cols() {
    const auto lines = ad::detail::wrap_text("hello", 0);
    assert(lines.size() == 1 && lines[0].empty());
}

}  // namespace

int main() {
    test_show_hide_and_bounds();
    test_close_hit_consume();
    test_draw_visibility_gated();
    test_wrap_long_sentence();
    test_wrap_preserves_newlines();
    test_wrap_breaks_long_word();
    test_wrap_zero_cols();
    return 0;
}
