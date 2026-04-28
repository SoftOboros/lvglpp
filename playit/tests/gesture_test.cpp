// gesture_test.cpp — PLAYIT-04a acceptance: TapRecognizer +
// DoubleTapRecognizer + GesturePipeline. Mirrors the rlvgl tests
// at rlvgl/platform/src/gesture.rs:299-448.

#include "lvglpp/playit/gesture.hpp"

#include "lvglpp/core/event.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace lc = lvglpp::core;
namespace lp = lvglpp::playit;

namespace {

template <class Alt, class Variant>
const Alt* as(const Variant& v) noexcept {
    return std::get_if<Alt>(&v);
}

// ---- ms_to_ticks parity ---------------------------------------------

void test_ms_to_ticks_scales_with_frame_rate() {
    // ceil(200 * 6 / 1000) = 2
    assert(lp::ms_to_ticks(lp::SETTLE_MS, 6)  == 2);
    // ceil(200 * 30 / 1000) = 6
    assert(lp::ms_to_ticks(lp::SETTLE_MS, 30) == 6);
    // ceil(200 * 60 / 1000) = 12
    assert(lp::ms_to_ticks(lp::SETTLE_MS, 60) == 12);
}

// ---- TapRecognizer ---------------------------------------------------

void test_tap_produces_press_down_then_release() {
    lp::TapRecognizer tap{30};

    // PointerDown → PressDown immediately.
    auto out = tap.process(lc::Event{lc::event::PointerDown{100, 200}});
    assert(out.has_value());
    auto* pd = as<lc::event::PressDown>(*out);
    assert(pd && pd->x == 100 && pd->y == 200);

    // PointerUp → queued, no output yet.
    out = tap.process(lc::Event{lc::event::PointerUp{100, 200}});
    assert(!out.has_value());

    // Tick through settle (SETTLE_MS=200 at 30Hz = 6 ticks).
    for (int i = 0; i < 5; ++i) {
        assert(!tap.tick().has_value());
    }
    auto release = tap.tick();
    assert(release.has_value());
    auto* pr = as<lc::event::PressRelease>(*release);
    assert(pr && pr->x == 100 && pr->y == 200);

    // Subsequent ticks idle.
    assert(!tap.tick().has_value());
}

void test_tap_bounce_suppressed() {
    lp::TapRecognizer tap{30};
    (void)tap.process(lc::Event{lc::event::PointerDown{10, 20}});
    (void)tap.process(lc::Event{lc::event::PointerUp{10, 20}});

    // Bounce: PointerDown during settle.
    auto out = tap.process(lc::Event{lc::event::PointerDown{10, 20}});
    assert(!out.has_value());

    (void)tap.process(lc::Event{lc::event::PointerUp{10, 20}});

    // Settle.
    const auto settle_ticks = lp::ms_to_ticks(lp::SETTLE_MS, 30);
    for (int i = 0; i < settle_ticks - 1; ++i) {
        assert(!tap.tick().has_value());
    }
    auto release = tap.tick();
    assert(release.has_value());
    auto* pr = as<lc::event::PressRelease>(*release);
    assert(pr && pr->x == 10 && pr->y == 20);
}

void test_tap_non_pointer_events_pass_through() {
    lp::TapRecognizer tap{30};
    auto out = tap.process(lc::Event{lc::event::Tick{}});
    assert(out.has_value());
    assert(std::holds_alternative<lc::event::Tick>(*out));
}

void test_tap_pointer_move_passes_through() {
    lp::TapRecognizer tap{30};
    auto out = tap.process(lc::Event{lc::event::PointerMove{5, 6}});
    assert(out.has_value());
    auto* pm = as<lc::event::PointerMove>(*out);
    assert(pm && pm->x == 5 && pm->y == 6);
}

// ---- DoubleTapRecognizer --------------------------------------------

// Helper: simulate a complete short tap through the double-tap recognizer.
std::vector<lc::Event> short_tap(lp::DoubleTapRecognizer& dtap,
                                  std::int32_t x, std::int32_t y,
                                  std::uint8_t hold_ticks) {
    std::vector<lc::Event> out;
    auto pair = dtap.process(lc::Event{lc::event::PressDown{x, y}});
    if (pair.primary)   out.push_back(*pair.primary);
    if (pair.secondary) out.push_back(*pair.secondary);
    for (std::uint8_t i = 0; i < hold_ticks; ++i) {
        if (auto e = dtap.tick()) out.push_back(*e);
    }
    pair = dtap.process(lc::Event{lc::event::PressRelease{x, y}});
    if (pair.primary)   out.push_back(*pair.primary);
    if (pair.secondary) out.push_back(*pair.secondary);
    return out;
}

bool contains_double_tap(const std::vector<lc::Event>& evs,
                          std::int32_t x, std::int32_t y) noexcept {
    for (const auto& e : evs) {
        if (auto* dt = as<lc::event::DoubleTap>(e)) {
            if (dt->x == x && dt->y == y) return true;
        }
    }
    return false;
}

bool contains_press_release(const std::vector<lc::Event>& evs,
                             std::int32_t x, std::int32_t y) noexcept {
    for (const auto& e : evs) {
        if (auto* pr = as<lc::event::PressRelease>(e)) {
            if (pr->x == x && pr->y == y) return true;
        }
    }
    return false;
}

void test_double_tap_emits_double_tap_event() {
    lp::DoubleTapRecognizer dtap{30};

    // First short tap — buffered, no PressRelease.
    auto first = short_tap(dtap, 100, 200, 2);
    assert(first.size() == 1);
    auto* pd = as<lc::event::PressDown>(first[0]);
    assert(pd && pd->x == 100 && pd->y == 200);

    // Small gap.
    for (int i = 0; i < 3; ++i) {
        assert(!dtap.tick().has_value());
    }

    // Second short tap at same position → DoubleTap.
    auto second = short_tap(dtap, 100, 200, 2);
    assert(contains_double_tap(second, 100, 200));
}

void test_single_tap_emits_after_timeout() {
    lp::DoubleTapRecognizer dtap{30};
    auto events = short_tap(dtap, 50, 60, 1);
    assert(events.size() == 1);  // only PressDown
    assert(as<lc::event::PressDown>(events[0]) != nullptr);

    const auto window_ticks = lp::ms_to_ticks(lp::DOUBLE_TAP_WINDOW_MS, 30);
    bool released = false;
    for (std::uint8_t i = 0; i < window_ticks; ++i) {
        if (auto e = dtap.tick()) {
            auto* pr = as<lc::event::PressRelease>(*e);
            assert(pr && pr->x == 50 && pr->y == 60);
            released = true;
        }
    }
    assert(released);
}

void test_long_press_passes_through() {
    lp::DoubleTapRecognizer dtap{30};
    const auto long_hold =
        static_cast<std::uint8_t>(lp::ms_to_ticks(lp::SHORT_PRESS_MAX_MS, 30) + 5);
    auto events = short_tap(dtap, 100, 200, long_hold);
    // PressDown + PressRelease both came through (not buffered).
    bool saw_pd = false, saw_pr = false;
    for (const auto& e : events) {
        if (as<lc::event::PressDown>(e))    saw_pd = true;
        if (auto* pr = as<lc::event::PressRelease>(e)) {
            if (pr->x == 100 && pr->y == 200) saw_pr = true;
        }
    }
    assert(saw_pd && saw_pr);
}

void test_double_tap_distance_rejection() {
    lp::DoubleTapRecognizer dtap{30};
    (void)short_tap(dtap, 10, 10, 1);
    // Second tap far away — no DoubleTap, but the buffered first
    // PressRelease emerges.
    const auto far = lp::DOUBLE_TAP_MAX_DISTANCE + 10;
    auto events = short_tap(dtap, 10 + far, 10, 1);
    bool has_double = false;
    for (const auto& e : events) {
        if (as<lc::event::DoubleTap>(e)) has_double = true;
    }
    assert(!has_double);
    assert(contains_press_release(events, 10, 10));
}

// ---- GesturePipeline composition ------------------------------------

void test_gesture_pipeline_full_tap_to_press_release() {
    lp::GesturePipeline pipe{30};

    // PointerDown → PressDown immediately. With DoubleTap on the
    // far side, PressDown passes through unchanged.
    auto out = pipe.process(lc::Event{lc::event::PointerDown{50, 50}});
    assert(out.primary && as<lc::event::PressDown>(*out.primary));

    // PointerUp → queued in TapRecognizer.
    out = pipe.process(lc::Event{lc::event::PointerUp{50, 50}});
    assert(!out.primary && !out.secondary);

    // Tick through settle (6 ticks at 30Hz). The Tap fires
    // PressRelease. DoubleTap suppresses it (buffers as Armed).
    int ticks_with_output = 0;
    for (int i = 0; i < 7; ++i) {
        auto t = pipe.tick();
        if (t.primary || t.secondary) ++ticks_with_output;
    }
    // No output yet — the buffered first PressRelease is held by
    // DoubleTap waiting for a possible second tap.
    assert(ticks_with_output == 0);

    // Tick through the double-tap window (12 ticks at 30Hz).
    bool released = false;
    const auto window_ticks = lp::ms_to_ticks(lp::DOUBLE_TAP_WINDOW_MS, 30);
    for (std::uint8_t i = 0; i < window_ticks; ++i) {
        auto t = pipe.tick();
        if (t.primary && as<lc::event::PressRelease>(*t.primary)) {
            released = true;
        }
    }
    assert(released);
}

void test_gesture_pipeline_two_taps_yield_double_tap() {
    lp::GesturePipeline pipe{30};

    // First tap: PointerDown / PointerUp / settle.
    (void)pipe.process(lc::Event{lc::event::PointerDown{100, 100}});
    (void)pipe.process(lc::Event{lc::event::PointerUp{100, 100}});
    const auto settle_ticks = lp::ms_to_ticks(lp::SETTLE_MS, 30);
    for (std::uint8_t i = 0; i < settle_ticks; ++i) (void)pipe.tick();

    // Small gap — within the double-tap window.
    for (int i = 0; i < 2; ++i) (void)pipe.tick();

    // Second tap.
    (void)pipe.process(lc::Event{lc::event::PointerDown{100, 100}});
    (void)pipe.process(lc::Event{lc::event::PointerUp{100, 100}});
    bool double_tap_seen = false;
    for (std::uint8_t i = 0; i < settle_ticks + 4; ++i) {
        auto t = pipe.tick();
        if (t.primary && as<lc::event::DoubleTap>(*t.primary)) {
            double_tap_seen = true;
        }
        if (t.secondary && as<lc::event::DoubleTap>(*t.secondary)) {
            double_tap_seen = true;
        }
    }
    assert(double_tap_seen);
}

}  // namespace

int main() {
    test_ms_to_ticks_scales_with_frame_rate();
    test_tap_produces_press_down_then_release();
    test_tap_bounce_suppressed();
    test_tap_non_pointer_events_pass_through();
    test_tap_pointer_move_passes_through();
    test_double_tap_emits_double_tap_event();
    test_single_tap_emits_after_timeout();
    test_long_press_passes_through();
    test_double_tap_distance_rejection();
    test_gesture_pipeline_full_tap_to_press_release();
    test_gesture_pipeline_two_taps_yield_double_tap();
    return 0;
}
