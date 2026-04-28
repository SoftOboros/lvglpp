// event_spec.hpp — wire-format event types for lvglpp::playit.
//
// PARITY: rlvgl/playit/src/command.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A — playit is the cross-language test harness, not LVGL
//         surface. The runtime widget-tree event lives in
//         lvglpp::core::Event (CORE-02 execution, pending).
// DELTA:  rlvgl uses Rust enums with tuple/struct payloads; lvglpp
//         uses std::variant + per-variant POD structs. Conversion to
//         lvglpp::core::Event lands with CORE-02 (acceptance §12).

#ifndef LVGLPP_PLAYIT_EVENT_SPEC_HPP
#define LVGLPP_PLAYIT_EVENT_SPEC_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>

namespace lvglpp::playit {

// Frozen by docs/core-event/00-event-surface.md §5.4 (Standards Action).
// Mirrors rlvgl/core/src/event.rs:4 — FT5336 controller hardware ceiling.
inline constexpr std::size_t MAX_TOUCH_POINTS = 5;

// Mirrors rlvgl/core/src/event.rs:8.
enum class TouchStateSpec : std::uint8_t {
    Down,
    Up,
    Contact,
};

// Mirrors rlvgl/playit/src/command.rs:74.
struct TouchPointSpec {
    std::uint8_t   id    = 0;
    TouchStateSpec state = TouchStateSpec::Up;
    std::int32_t   x     = 0;
    std::int32_t   y     = 0;

    constexpr bool operator==(const TouchPointSpec&) const noexcept = default;
};

// KeySpec — wire-format key identifier.
//
// PARITY: rlvgl/playit/src/command.rs:112.
//
// rlvgl uses a Rust enum; lvglpp folds the small set into a tag +
// payload because the C++ wire-side never needs to dispatch on the
// variants except in the to_event() seam. Variant identity is
// preserved by (kind, value) — see §5.3 of the CORE-02 chapter.
struct KeySpec {
    enum class Kind : std::uint8_t {
        Escape,
        Enter,
        Space,
        ArrowUp,
        ArrowDown,
        ArrowLeft,
        ArrowRight,
        Function,   // value = 1..=12
        Character,  // value = Unicode scalar value
        Other,      // value = opaque keycode
    };

    Kind          kind  = Kind::Other;
    std::uint32_t value = 0;

    constexpr bool operator==(const KeySpec&) const noexcept = default;
};

// Per-variant POD payloads. One type per variant so std::visit and
// pattern-style code dispatch correctly without a separate Kind
// discriminator. Each is an aggregate with defaulted equality.
namespace event_spec {

struct Tick {
    constexpr bool operator==(const Tick&) const noexcept = default;
};

struct PressRelease {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PressRelease&) const noexcept = default;
};

struct PressDown {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PressDown&) const noexcept = default;
};

struct PointerDown {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PointerDown&) const noexcept = default;
};

struct PointerUp {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PointerUp&) const noexcept = default;
};

struct PointerMove {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PointerMove&) const noexcept = default;
};

struct DoubleTap {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const DoubleTap&) const noexcept = default;
};

struct KeyDown {
    KeySpec key{};
    constexpr bool operator==(const KeyDown&) const noexcept = default;
};

struct KeyUp {
    KeySpec key{};
    constexpr bool operator==(const KeyUp&) const noexcept = default;
};

struct Touch {
    std::uint8_t                                 count = 0;
    std::array<TouchPointSpec, MAX_TOUCH_POINTS> points{};
    constexpr bool operator==(const Touch&) const noexcept = default;
};

}  // namespace event_spec

// EventSpec — sum type over the ten variants in
// docs/core-event/00-event-surface.md §5.1.
//
// Variant ordering matches the rlvgl source for parity with any
// future serialization that depends on tag indices.
using EventSpec = std::variant<
    event_spec::Tick,
    event_spec::PressRelease,
    event_spec::PressDown,
    event_spec::PointerDown,
    event_spec::PointerUp,
    event_spec::PointerMove,
    event_spec::DoubleTap,
    event_spec::KeyDown,
    event_spec::KeyUp,
    event_spec::Touch
>;

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_EVENT_SPEC_HPP
