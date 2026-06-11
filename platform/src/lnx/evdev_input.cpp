// evdev_input.cpp — evdev input state machine + device IO.
//
// PARITY: rlvgl/platform/src/linux_evdev.rs (v0.2.0 @ 79f730d).
// The protocol state machine (apply) is portable C++ so host tests
// can exercise it with synthetic streams (PLAT-LNX §12); only the
// fd open/read path is Linux-gated.

#include "lvglpp/platform/lnx/evdev_input.hpp"

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#endif

namespace lvglpp::platform::lnx {

namespace {

// Linux UAPI constants (input-event-codes.h), restated so the state
// machine compiles portably for host tests. Values are frozen ABI.
constexpr std::uint16_t EV_SYN             = 0x00;
constexpr std::uint16_t EV_KEY             = 0x01;
constexpr std::uint16_t EV_ABS             = 0x03;
constexpr std::uint16_t SYN_REPORT         = 0x00;
constexpr std::uint16_t BTN_TOUCH          = 0x14A;
constexpr std::uint16_t ABS_X              = 0x00;
constexpr std::uint16_t ABS_Y              = 0x01;
constexpr std::uint16_t ABS_MT_SLOT        = 0x2F;
constexpr std::uint16_t ABS_MT_POSITION_X  = 0x35;
constexpr std::uint16_t ABS_MT_POSITION_Y  = 0x36;
constexpr std::uint16_t ABS_MT_TRACKING_ID = 0x39;

}  // namespace

std::optional<EvdevInput> EvdevInput::open(const char* path) noexcept {
#ifdef __linux__
    const int fd = ::open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return std::nullopt;
    EvdevInput in;
    in.fd_ = fd;
    return in;
#else
    (void)path;
    return std::nullopt;  // PLAT-LNX is Linux-only at runtime.
#endif
}

EvdevInput::EvdevInput(EvdevInput&& other) noexcept
    : fd_{other.fd_}, slots_{other.slots_},
      current_slot_{other.current_slot_}, abs_x_{other.abs_x_},
      abs_y_{other.abs_y_}, touching_{other.touching_},
      touch_changed_{other.touch_changed_}, moved_{other.moved_},
      was_down_{other.was_down_} {
    other.fd_ = -1;
}

EvdevInput::~EvdevInput() {
#ifdef __linux__
    if (fd_ >= 0) ::close(fd_);
#endif
}

std::optional<core::Event>
EvdevInput::apply(std::uint16_t type, std::uint16_t code,
                  std::int32_t value) noexcept {
    switch (type) {
        case EV_ABS:
            switch (code) {
                case ABS_X: abs_x_ = value; moved_ = true; break;
                case ABS_Y: abs_y_ = value; moved_ = true; break;
                case ABS_MT_SLOT:
                    if (value >= 0 &&
                        static_cast<std::size_t>(value) < MT_SLOTS) {
                        current_slot_ = static_cast<std::size_t>(value);
                    }
                    break;
                case ABS_MT_TRACKING_ID:
                    slots_[current_slot_].id = value;  // -1 lifts
                    touch_changed_ = true;
                    break;
                case ABS_MT_POSITION_X:
                    slots_[current_slot_].x = value;
                    if (current_slot_ == 0) { abs_x_ = value; moved_ = true; }
                    break;
                case ABS_MT_POSITION_Y:
                    slots_[current_slot_].y = value;
                    if (current_slot_ == 0) { abs_y_ = value; moved_ = true; }
                    break;
                default: break;
            }
            break;
        case EV_KEY:
            if (code == BTN_TOUCH) {
                touching_      = value != 0;
                touch_changed_ = true;
            }
            break;
        case EV_SYN:
            if (code == SYN_REPORT) {
                const bool down = touching_ || slots_[0].id >= 0;
                const bool moved = moved_;
                moved_ = false;
                touch_changed_ = false;
                if (down && !was_down_) {
                    was_down_ = true;
                    return core::Event{core::event::PointerDown{abs_x_, abs_y_}};
                }
                if (!down && was_down_) {
                    was_down_ = false;
                    return core::Event{core::event::PointerUp{abs_x_, abs_y_}};
                }
                if (down && moved) {
                    return core::Event{core::event::PointerMove{abs_x_, abs_y_}};
                }
            }
            break;
        default: break;
    }
    return std::nullopt;
}

std::optional<core::Event>
EvdevInput::feed_for_test(std::uint16_t type, std::uint16_t code,
                          std::int32_t value) noexcept {
    return apply(type, code, value);
}

std::optional<core::Event> EvdevInput::poll() noexcept {
#ifdef __linux__
    if (fd_ < 0) return std::nullopt;
    // 24-byte input_event on 64-bit: timeval(16) + type(2) + code(2)
    // + value(4) (linux_evdev.rs parity).
    struct RawEvent {
        std::uint64_t tv_sec;
        std::uint64_t tv_usec;
        std::uint16_t type;
        std::uint16_t code;
        std::int32_t  value;
    };
    RawEvent ev;
    while (::read(fd_, &ev, sizeof(ev)) == static_cast<ssize_t>(sizeof(ev))) {
        if (auto out = apply(ev.type, ev.code, ev.value)) return out;
    }
    return std::nullopt;
#else
    return std::nullopt;
#endif
}

}  // namespace lvglpp::platform::lnx
