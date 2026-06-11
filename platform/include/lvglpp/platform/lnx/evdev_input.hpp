// evdev_input.hpp — generic Linux evdev input backend.
//
// PARITY: rlvgl/platform/src/linux_evdev.rs (v0.2.0 @ 79f730d) —
//         O_NONBLOCK reads of input_event records; single-touch
//         ABS_X/Y + BTN_TOUCH; multi-touch protocol B (ABS_MT_SLOT /
//         TRACKING_ID / POSITION_X/Y, 10 slots); SYN_REPORT commits
//         accumulated state to pointer events.
// LVGL:   lvgl/src/drivers/evdev — informative only.
// DELTA:  namespace `lnx`; emits core::Event pointer variants.
//
// docs/platform-linux/00-fbdev-evdev.md (PLAT-LNX) §5.1. Linux-only.

#ifndef LVGLPP_PLATFORM_LNX_EVDEV_INPUT_HPP
#define LVGLPP_PLATFORM_LNX_EVDEV_INPUT_HPP

#include <array>
#include <cstdint>
#include <optional>

#include "lvglpp/core/event.hpp"

namespace lvglpp::platform::lnx {

// Owns the evdev fd; closed in the destructor. Move-only.
class EvdevInput {
public:
    [[nodiscard]] static std::optional<EvdevInput>
    open(const char* path) noexcept;

    EvdevInput(const EvdevInput&)            = delete;
    EvdevInput& operator=(const EvdevInput&) = delete;
    EvdevInput(EvdevInput&& other) noexcept;
    EvdevInput& operator=(EvdevInput&&)      = delete;
    ~EvdevInput();

    // Non-blocking: drains available input_event records; returns
    // the next committed pointer event, or nullopt when the device
    // has nothing new this poll. (linux_evdev.rs poll() parity.)
    [[nodiscard]] std::optional<core::Event> poll() noexcept;

    // Testing seam (PLAT-LNX §12): feed one 24-byte input_event
    // record through the state machine exactly as if read from the
    // fd; returns a committed event when this record was a
    // SYN_REPORT that produced one. Lets host tests exercise the
    // protocol logic with synthetic streams and no /dev access.
    [[nodiscard]] std::optional<core::Event>
    feed_for_test(std::uint16_t type, std::uint16_t code,
                  std::int32_t value) noexcept;

    // Host-test constructor: state machine without a device.
    static EvdevInput make_detached_for_test() noexcept {
        return EvdevInput{};
    }

private:
    EvdevInput() = default;

    [[nodiscard]] std::optional<core::Event>
    apply(std::uint16_t type, std::uint16_t code,
          std::int32_t value) noexcept;

    int fd_ = -1;  // owns

    // Accumulated touch state (linux_evdev.rs parity).
    static constexpr std::size_t MT_SLOTS = 10;
    struct Slot {
        std::int32_t id = -1;  // -1 = empty (TRACKING_ID -1 lifts)
        std::int32_t x = 0, y = 0;
    };
    std::array<Slot, MT_SLOTS> slots_{};
    std::size_t  current_slot_ = 0;
    std::int32_t abs_x_ = 0, abs_y_ = 0;
    bool         touching_      = false;  // BTN_TOUCH state
    bool         touch_changed_ = false;  // edge pending SYN commit
    bool         moved_         = false;  // position pending commit
    bool         was_down_      = false;  // last committed contact
};

}  // namespace lvglpp::platform::lnx

#endif  // LVGLPP_PLATFORM_LNX_EVDEV_INPUT_HPP
