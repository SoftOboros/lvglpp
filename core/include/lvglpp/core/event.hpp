// event.hpp — runtime event surface for the lvglpp widget tree.
//
// PARITY: rlvgl/core/src/event.rs (v0.2.0 @ d99f793) — Event variant
//         set, TouchState, TouchPoint, Key, MAX_TOUCH_POINTS.
// LVGL:   lvgl/src/core/lv_obj_event.h — informative only.
//         lvglpp::core::Event does NOT shadow LVGL's lv_event_t; see
//         docs/core-event/00-event-surface.md §10 ("Reconciliation
//         vs. adjacent primitives").
// DELTA:  rlvgl uses a Rust enum with payload variants; lvglpp uses
//         std::variant + per-variant aggregate structs. The Rust
//         `enum` `match` exhaustiveness translates to std::visit +
//         constexpr-if dispatch.
//
// This file implements docs/core-event/00-event-surface.md (CORE-02).
// Variant sets and MAX_TOUCH_POINTS are FROZEN under Standards Action
// — adding a value requires a concepts-doc amendment first per
// CLAUDE.md § "Frozen enumerations" + § "Cross-language change
// ordering".

#ifndef LVGLPP_CORE_EVENT_HPP
#define LVGLPP_CORE_EVENT_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>

namespace lvglpp::core {

// Frozen by docs/core-event/00-event-surface.md §5.4 (Standards
// Action). Mirrors rlvgl/core/src/event.rs:4 — FT5336 controller
// hardware ceiling.
inline constexpr std::size_t MAX_TOUCH_POINTS = 5;

// Per-point event flag. Mirrors rlvgl/core/src/event.rs:8 (§5.2).
enum class TouchState : std::uint8_t {
    Down,
    Up,
    Contact,
};

// A single touch contact point within a frame. Mirrors
// rlvgl/core/src/event.rs:19 (§3 of the chapter — fields documented
// there).
//
// Field ordering matches the rlvgl source: id / x / y / state. C++
// alignment may add padding after `id`; the layout is logical, not
// byte-stable (concepts doc §4 rules out memcpy transmission).
struct TouchPoint {
    std::uint8_t id    = 0;
    std::int32_t x     = 0;
    std::int32_t y     = 0;
    TouchState   state = TouchState::Up;

    constexpr bool operator==(const TouchPoint&) const noexcept = default;
};

// Key variant payloads. Named keys (Escape, Enter, …) are empty
// aggregates so std::visit dispatches by type rather than by a
// secondary discriminator. Parametric keys carry their value
// directly.
//
// Mirrors rlvgl/core/src/event.rs:118 (§5.3).
namespace key {

struct Escape     { constexpr bool operator==(const Escape&) const noexcept = default; };
struct Enter      { constexpr bool operator==(const Enter&) const noexcept = default; };
struct Space      { constexpr bool operator==(const Space&) const noexcept = default; };
struct ArrowUp    { constexpr bool operator==(const ArrowUp&) const noexcept = default; };
struct ArrowDown  { constexpr bool operator==(const ArrowDown&) const noexcept = default; };
struct ArrowLeft  { constexpr bool operator==(const ArrowLeft&) const noexcept = default; };
struct ArrowRight { constexpr bool operator==(const ArrowRight&) const noexcept = default; };

// Function key index. rlvgl's source documents 1..=12 in prose; the
// type holds the full uint8_t range to mirror the Rust source's
// `u8` width without lossy clamping at the type level. Validate at
// construction sites if the function-key range matters there.
struct Function {
    std::uint8_t n = 0;
    constexpr bool operator==(const Function&) const noexcept = default;
};

// Printable character key. Carries a Unicode scalar value (U+0000
// through U+10FFFF excluding the surrogate range U+D800..=U+DFFF —
// not enforced at construction; the playit parser only emits ASCII
// today).
struct Character {
    std::uint32_t codepoint = 0;
    constexpr bool operator==(const Character&) const noexcept = default;
};

// Catch-all for keys outside the named / Function / Character set.
// `code` is opaque; consumers MUST NOT assume any encoding.
struct Other {
    std::uint32_t code = 0;
    constexpr bool operator==(const Other&) const noexcept = default;
};

}  // namespace key

// Sum type over key variants. Default-constructs to Escape (the
// first variant), parity with std::variant default semantics; in
// practice every Key value is constructed explicitly from the input
// path.
using Key = std::variant<
    key::Escape,
    key::Enter,
    key::Space,
    key::ArrowUp,
    key::ArrowDown,
    key::ArrowLeft,
    key::ArrowRight,
    key::Function,
    key::Character,
    key::Other
>;

// Event variant payloads. One aggregate per variant so std::visit
// dispatches by type. Field shapes mirror rlvgl/core/src/event.rs:43
// (§5.1) field-for-field.
//
// Coordinate fields are int32_t in landscape pixel coordinates per
// concepts doc §5.5; off-screen values are valid for hit-test edges.
namespace event {

struct Tick {
    constexpr bool operator==(const Tick&) const noexcept = default;
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

struct PressDown {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PressDown&) const noexcept = default;
};

struct PressRelease {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const PressRelease&) const noexcept = default;
};

struct DoubleTap {
    std::int32_t x = 0;
    std::int32_t y = 0;
    constexpr bool operator==(const DoubleTap&) const noexcept = default;
};

struct KeyDown {
    Key key{};
    constexpr bool operator==(const KeyDown&) const noexcept = default;
};

struct KeyUp {
    Key key{};
    constexpr bool operator==(const KeyUp&) const noexcept = default;
};

// Multi-touch frame. Only `points[0..count]` are meaningful; the
// trailing slots are zero-initialised TouchPoints (state = Up).
struct Touch {
    std::uint8_t                            count  = 0;
    std::array<TouchPoint, MAX_TOUCH_POINTS> points{};
    constexpr bool operator==(const Touch&) const noexcept = default;
};

}  // namespace event

// Event sum type. Variant ordering matches rlvgl/core/src/event.rs:43
// for parity with any future serialization that depends on tag
// indices.
using Event = std::variant<
    event::Tick,
    event::PointerDown,
    event::PointerUp,
    event::PointerMove,
    event::PressDown,
    event::PressRelease,
    event::DoubleTap,
    event::KeyDown,
    event::KeyUp,
    event::Touch
>;

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_EVENT_HPP
