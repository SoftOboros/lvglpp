// icon_strip_test.cpp — DEMO-05 acceptance for IconStrip.
//
// Mirrors the rlvgl icon_strip unit behavior: focus get/set, tap-index
// dispatch on PressRelease within a slot, disabled-slot gating, and that
// draw blits a decoded icon. Uses a hand-built RLEC blob so the test does
// not depend on the rlvgl submodule being present.

#include "lvglpp/app/disco_demo/icon_strip.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace lc = lvglpp::core;
namespace ad = lvglpp::app::disco_demo;

namespace {

struct RecordingRenderer : lc::Renderer {
    struct FillOp { lc::Rect rect; lc::Color color; };
    struct BlitOp { std::int32_t x, y; std::uint32_t w, h; };
    std::vector<FillOp> fills;
    std::vector<BlitOp> blits;
    void fill_rect(lc::Rect rect, lc::Color color) override {
        fills.push_back({rect, color});
    }
    void draw_text(std::int32_t, std::int32_t, std::string_view, lc::Color) override {}
    void draw_pixels(std::int32_t x, std::int32_t y, std::span<const lc::Color>,
                     std::uint32_t w, std::uint32_t h) override {
        blits.push_back({x, y, w, h});
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

// 2x2 all-red RLEC blob: palette[0]=red565, stream = idx0 + short-repeat 3.
std::vector<std::uint8_t> red_2x2() {
    std::vector<std::uint8_t> blob = {'R', 'L', 'E', 'C'};
    push_u16(blob, 2);  // width
    push_u16(blob, 2);  // height
    push_u16(blob, 1);  // palette_len
    push_u16(blob, 0xF800);  // red565
    const std::vector<std::uint8_t> stream = {0x00, 0x04};  // idx0, repeat 3
    push_u32(blob, static_cast<std::uint32_t>(stream.size()));
    for (auto b : stream) blob.push_back(b);
    return blob;
}

void test_focus_get_set() {
    ad::IconStrip strip{740, 60, 17, 10};
    assert(!strip.focused_slot().has_value());
    strip.set_focused_slot(2);
    assert(strip.focused_slot() == std::optional<std::size_t>{2});
    strip.set_focused_slot(std::nullopt);
    assert(!strip.focused_slot().has_value());
}

void test_tap_index() {
    const auto blob = red_2x2();
    ad::IconStrip strip{740, 60, 17, 10};
    std::optional<std::size_t> tapped;
    for (std::size_t i = 0; i < ad::SLOT_COUNT; ++i) {
        ad::IconSlot slot;
        slot.rle = std::span<const std::uint8_t>(blob.data(), blob.size());
        slot.enabled = true;
        slot.on_tap = [&tapped](std::size_t idx) { tapped = idx; };
        strip.set_slot(i, std::move(slot));
    }
    // step = 70; slot 1 occupies y in [82, 152); x must be >= 740.
    lc::Event ev{lc::event::PressRelease{745, 100}};
    assert(strip.handle_event(ev));
    assert(tapped == std::optional<std::size_t>{1});
}

void test_disabled_slot_ignored() {
    const auto blob = red_2x2();
    ad::IconStrip strip{740, 60, 17, 10};
    bool fired = false;
    ad::IconSlot slot;
    slot.rle = std::span<const std::uint8_t>(blob.data(), blob.size());
    slot.enabled = false;  // disabled
    slot.on_tap = [&fired](std::size_t) { fired = true; };
    strip.set_slot(0, std::move(slot));
    lc::Event ev{lc::event::PressRelease{745, 10}};
    assert(!strip.handle_event(ev));
    assert(!fired);
}

void test_non_pressrelease_ignored() {
    ad::IconStrip strip{740, 60, 17, 10};
    const lc::Event ev{lc::event::PointerDown{745, 100}};
    assert(!strip.handle_event(ev));
}

void test_draw_blits_and_focus_border() {
    const auto blob = red_2x2();
    RecordingRenderer r;
    ad::IconStrip strip{740, 60, 17, 10};
    ad::IconSlot slot;
    slot.rle = std::span<const std::uint8_t>(blob.data(), blob.size());
    slot.enabled = true;
    strip.set_slot(0, std::move(slot));
    strip.set_focused_slot(0);
    strip.draw(r);
    assert(r.blits.size() == 1);
    assert(r.blits[0].w == 2 && r.blits[0].h == 2);
    // Focus border = four straight edges.
    assert(r.fills.size() == 4);
}

}  // namespace

int main() {
    test_focus_get_set();
    test_tap_index();
    test_disabled_slot_ignored();
    test_non_pressrelease_ignored();
    test_draw_blits_and_focus_border();
    return 0;
}
