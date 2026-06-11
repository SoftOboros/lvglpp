// evdev_test.cpp — PLAT-LNX §12 acceptance: the evdev protocol
// state machine driven by synthetic input_event streams.
//
// PARITY: exercises the linux_evdev.rs semantics (single-touch
// BTN_TOUCH + ABS_X/Y; MT protocol B slot 0; SYN_REPORT commits).

#include "lvglpp/platform/lnx/evdev_input.hpp"

#include <cassert>
#include <variant>

namespace lc = lvglpp::core;
using lvglpp::platform::lnx::EvdevInput;

namespace {

// UAPI values (frozen ABI), restated for the synthetic streams.
constexpr std::uint16_t EV_SYN = 0, EV_KEY = 1, EV_ABS = 3;
constexpr std::uint16_t SYN_REPORT = 0, BTN_TOUCH = 0x14A;
constexpr std::uint16_t ABS_X = 0, ABS_Y = 1;
constexpr std::uint16_t ABS_MT_SLOT = 0x2F, ABS_MT_POSITION_X = 0x35,
                        ABS_MT_POSITION_Y = 0x36,
                        ABS_MT_TRACKING_ID = 0x39;

void test_single_touch_cycle() {
    auto in = EvdevInput::make_detached_for_test();

    // Down at (100, 200).
    assert(!in.feed_for_test(EV_ABS, ABS_X, 100).has_value());
    assert(!in.feed_for_test(EV_ABS, ABS_Y, 200).has_value());
    assert(!in.feed_for_test(EV_KEY, BTN_TOUCH, 1).has_value());
    auto ev = in.feed_for_test(EV_SYN, SYN_REPORT, 0);
    assert(ev.has_value());
    const auto* down = std::get_if<lc::event::PointerDown>(&*ev);
    assert(down != nullptr && down->x == 100 && down->y == 200);

    // Drag to (110, 210).
    assert(!in.feed_for_test(EV_ABS, ABS_X, 110).has_value());
    assert(!in.feed_for_test(EV_ABS, ABS_Y, 210).has_value());
    ev = in.feed_for_test(EV_SYN, SYN_REPORT, 0);
    assert(ev.has_value());
    const auto* move = std::get_if<lc::event::PointerMove>(&*ev);
    assert(move != nullptr && move->x == 110 && move->y == 210);

    // Quiet frame: no event.
    assert(!in.feed_for_test(EV_SYN, SYN_REPORT, 0).has_value());

    // Up.
    assert(!in.feed_for_test(EV_KEY, BTN_TOUCH, 0).has_value());
    ev = in.feed_for_test(EV_SYN, SYN_REPORT, 0);
    assert(ev.has_value());
    assert(std::get_if<lc::event::PointerUp>(&*ev) != nullptr);
}

void test_mt_protocol_b_slot0() {
    auto in = EvdevInput::make_detached_for_test();

    // Contact on slot 0 via tracking id, position, SYN.
    assert(!in.feed_for_test(EV_ABS, ABS_MT_SLOT, 0).has_value());
    assert(!in.feed_for_test(EV_ABS, ABS_MT_TRACKING_ID, 7).has_value());
    assert(!in.feed_for_test(EV_ABS, ABS_MT_POSITION_X, 31).has_value());
    assert(!in.feed_for_test(EV_ABS, ABS_MT_POSITION_Y, 42).has_value());
    auto ev = in.feed_for_test(EV_SYN, SYN_REPORT, 0);
    assert(ev.has_value());
    const auto* down = std::get_if<lc::event::PointerDown>(&*ev);
    assert(down != nullptr && down->x == 31 && down->y == 42);

    // Lift: tracking id -1.
    assert(!in.feed_for_test(EV_ABS, ABS_MT_TRACKING_ID, -1).has_value());
    ev = in.feed_for_test(EV_SYN, SYN_REPORT, 0);
    assert(ev.has_value());
    assert(std::get_if<lc::event::PointerUp>(&*ev) != nullptr);
}

}  // namespace

int main() {
    test_single_touch_cycle();
    test_mt_protocol_b_slot0();
    return 0;
}
