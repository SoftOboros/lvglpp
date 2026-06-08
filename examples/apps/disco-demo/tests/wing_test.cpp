// wing_test.cpp — DEMO-05 acceptance for Wing.
//
// Mirrors the rlvgl wing unit behavior: visibility toggle, collapse-to-zero
// bounds when hidden (the FROZEN DEMO contract — see wing.hpp DELTA),
// clear_region returning the paint-over rect for CLEAR_FRAMES after close,
// slot-tap dispatch that closes the wing, and focus get/set.

#include "lvglpp/app/disco_demo/wing.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace lc = lvglpp::core;
namespace ad = lvglpp::app::disco_demo;

namespace {

struct RecordingRenderer : lc::Renderer {
    int fills = 0;
    int blits = 0;
    void fill_rect(lc::Rect, lc::Color) override { ++fills; }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override {}
    void draw_pixels(std::int32_t, std::int32_t, std::span<const lc::Color>,
                     std::uint32_t, std::uint32_t) override {
        ++blits;
    }
};

void push_u16(std::vector<std::uint8_t>& v, std::uint16_t x) {
    v.push_back(static_cast<std::uint8_t>(x & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
}
void push_u32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    for (int i = 0; i < 4; ++i)
        v.push_back(static_cast<std::uint8_t>((x >> (8 * i)) & 0xFF));
}
std::vector<std::uint8_t> red_2x2() {
    std::vector<std::uint8_t> blob = {'R', 'L', 'E', 'C'};
    push_u16(blob, 2);
    push_u16(blob, 2);
    push_u16(blob, 1);
    push_u16(blob, 0xF800);
    const std::vector<std::uint8_t> stream = {0x00, 0x04};
    push_u32(blob, static_cast<std::uint32_t>(stream.size()));
    for (auto b : stream) blob.push_back(b);
    return blob;
}

using IconPair = std::pair<std::span<const std::uint8_t>, bool>;

std::vector<IconPair> two_slots(const std::vector<std::uint8_t>& blob) {
    const std::span<const std::uint8_t> s(blob.data(), blob.size());
    return {IconPair{s, true}, IconPair{s, true}};
}

void test_visibility_toggle_and_bounds() {
    const auto blob = red_2x2();
    const auto pairs = two_slots(blob);
    ad::Wing w{std::span<const IconPair>(pairs)};

    assert(!w.is_visible());
    assert(w.bounds() == (lc::Rect{0, 0, 0, 0}));  // collapse when hidden

    assert(w.toggle_visible());  // now visible
    assert(w.is_visible());
    const lc::Rect vb = w.bounds();
    assert(vb.width > 0 && vb.height > 0);

    assert(!w.toggle_visible());  // toggles to hidden (close)
    assert(!w.is_visible());
    assert(w.bounds() == (lc::Rect{0, 0, 0, 0}));
}

void test_clear_region_after_close() {
    const auto blob = red_2x2();
    const auto pairs = two_slots(blob);
    ad::Wing w{std::span<const IconPair>(pairs)};

    w.toggle_visible();  // visible
    w.close();           // arms CLEAR_FRAMES countdown
    // Exactly CLEAR_FRAMES (=3) non-null clear rects, then nullopt.
    for (std::uint8_t k = 0; k < ad::CLEAR_FRAMES; ++k) {
        assert(w.clear_region().has_value());
    }
    assert(!w.clear_region().has_value());
}

void test_tap_fires_and_closes() {
    const auto blob = red_2x2();
    const auto pairs = two_slots(blob);
    ad::Wing w{std::span<const IconPair>(pairs)};

    std::optional<std::size_t> tapped;
    w.slots_mut()[0]->on_tap = [&tapped](std::size_t i) { tapped = i; };

    // Hidden: event ignored.
    const lc::Event ev{lc::event::PressRelease{30, 40}};
    assert(!w.handle_event(ev));

    w.toggle_visible();
    assert(w.handle_event(ev));               // slot 0 hit (x<=70, y in [0,82))
    assert(tapped == std::optional<std::size_t>{0});
    assert(!w.is_visible());                  // tap closes the wing
}

void test_focus_get_set() {
    const auto blob = red_2x2();
    const auto pairs = two_slots(blob);
    ad::Wing w{std::span<const IconPair>(pairs)};
    assert(!w.focused_slot().has_value());
    w.set_focused_slot(1);
    assert(w.focused_slot() == std::optional<std::size_t>{1});
}

void test_draw_visibility_gated() {
    const auto blob = red_2x2();
    const auto pairs = two_slots(blob);
    ad::Wing w{std::span<const IconPair>(pairs)};

    RecordingRenderer hidden;
    w.draw(hidden);
    assert(hidden.fills == 0 && hidden.blits == 0);

    w.toggle_visible();
    RecordingRenderer shown;
    w.draw(shown);
    assert(shown.blits == 2);   // two decoded icons
    assert(shown.fills >= 5);   // bg fill + 4-edge border (at least)
}

}  // namespace

int main() {
    test_visibility_toggle_and_bounds();
    test_clear_region_after_close();
    test_tap_fires_and_closes();
    test_focus_get_set();
    test_draw_visibility_gated();
    return 0;
}
