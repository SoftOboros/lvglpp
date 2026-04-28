// conversion.hpp — playit-spec → core-event conversions.
//
// PARITY: rlvgl/playit/src/command.rs (v0.2.0 @ b178cbc) — the
//         `KeySpec::to_key`, `EventSpec::to_event`,
//         `TouchStateSpec::to_core` impls.
// LVGL:   N/A — these conversions live entirely on the lvglpp side.
// DELTA:  rlvgl uses methods (`spec.to_event()`); lvglpp uses free
//         functions (`to_event(spec)`) so playit's wire-format types
//         can be defined without a forward dependency on
//         lvglpp::core.
//
// All conversions are pure value transformations, allocation-free,
// noexcept, and constexpr where the std::variant ABI permits.

#ifndef LVGLPP_PLAYIT_CONVERSION_HPP
#define LVGLPP_PLAYIT_CONVERSION_HPP

#include <cstdint>
#include <type_traits>
#include <variant>

#include "lvglpp/core/event.hpp"
#include "lvglpp/playit/command.hpp"
#include "lvglpp/playit/event_spec.hpp"

namespace lvglpp::playit {

// MAX_TOUCH_POINTS must agree across the language pair AND the
// playit/core pair. Concepts doc §5.4 freezes this constant under
// Standards Action; the static_assert is the local enforcement.
static_assert(::lvglpp::playit::MAX_TOUCH_POINTS ==
              ::lvglpp::core::MAX_TOUCH_POINTS,
              "playit and core disagree on MAX_TOUCH_POINTS — see "
              "docs/core-event/00-event-surface.md §5.4");

// TouchStateSpec → core::TouchState.
[[nodiscard]] constexpr ::lvglpp::core::TouchState
to_core(TouchStateSpec s) noexcept {
    using lvglpp::core::TouchState;
    switch (s) {
        case TouchStateSpec::Down:    return TouchState::Down;
        case TouchStateSpec::Up:      return TouchState::Up;
        case TouchStateSpec::Contact: return TouchState::Contact;
    }
    // Unreachable in well-formed input; pick a defined value rather
    // than std::unreachable() so embedded posture (no exceptions)
    // does not invoke UB on a malformed enum.
    return TouchState::Up;
}

// TouchPointSpec → core::TouchPoint.
[[nodiscard]] constexpr ::lvglpp::core::TouchPoint
to_core(const TouchPointSpec& p) noexcept {
    return ::lvglpp::core::TouchPoint{
        p.id,
        p.x,
        p.y,
        to_core(p.state),
    };
}

// KeySpec → core::Key. Exhaustive over KeySpec::Kind; concepts doc
// §5.3 freezes the variant set so no default branch is needed except
// as a safe fallback for forward-incompatible producers.
[[nodiscard]] inline ::lvglpp::core::Key
to_key(const KeySpec& s) noexcept {
    using lvglpp::core::Key;
    namespace k = lvglpp::core::key;

    switch (s.kind) {
        case KeySpec::Kind::Escape:     return Key{k::Escape{}};
        case KeySpec::Kind::Enter:      return Key{k::Enter{}};
        case KeySpec::Kind::Space:      return Key{k::Space{}};
        case KeySpec::Kind::ArrowUp:    return Key{k::ArrowUp{}};
        case KeySpec::Kind::ArrowDown:  return Key{k::ArrowDown{}};
        case KeySpec::Kind::ArrowLeft:  return Key{k::ArrowLeft{}};
        case KeySpec::Kind::ArrowRight: return Key{k::ArrowRight{}};
        case KeySpec::Kind::Function:
            return Key{k::Function{static_cast<std::uint8_t>(s.value & 0xFFU)}};
        case KeySpec::Kind::Character:
            return Key{k::Character{s.value}};
        case KeySpec::Kind::Other:
            return Key{k::Other{s.value}};
    }
    return Key{k::Other{s.value}};
}

// EventSpec → core::Event. Exhaustive over the variant set in
// concepts doc §5.1.
[[nodiscard]] inline ::lvglpp::core::Event
to_event(const EventSpec& spec) noexcept {
    using lvglpp::core::Event;
    namespace e = lvglpp::core::event;

    return std::visit([](const auto& payload) -> Event {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, event_spec::Tick>) {
            return Event{e::Tick{}};
        } else if constexpr (std::is_same_v<T, event_spec::PressRelease>) {
            return Event{e::PressRelease{payload.x, payload.y}};
        } else if constexpr (std::is_same_v<T, event_spec::PressDown>) {
            return Event{e::PressDown{payload.x, payload.y}};
        } else if constexpr (std::is_same_v<T, event_spec::PointerDown>) {
            return Event{e::PointerDown{payload.x, payload.y}};
        } else if constexpr (std::is_same_v<T, event_spec::PointerUp>) {
            return Event{e::PointerUp{payload.x, payload.y}};
        } else if constexpr (std::is_same_v<T, event_spec::PointerMove>) {
            return Event{e::PointerMove{payload.x, payload.y}};
        } else if constexpr (std::is_same_v<T, event_spec::DoubleTap>) {
            return Event{e::DoubleTap{payload.x, payload.y}};
        } else if constexpr (std::is_same_v<T, event_spec::KeyDown>) {
            return Event{e::KeyDown{to_key(payload.key)}};
        } else if constexpr (std::is_same_v<T, event_spec::KeyUp>) {
            return Event{e::KeyUp{to_key(payload.key)}};
        } else if constexpr (std::is_same_v<T, event_spec::Touch>) {
            e::Touch out{};
            out.count = payload.count;
            for (std::size_t i = 0; i < ::lvglpp::core::MAX_TOUCH_POINTS; ++i) {
                out.points[i] = to_core(payload.points[i]);
            }
            return Event{out};
        } else {
            // Unreachable: the static_assert below catches any new
            // variant that escapes this dispatch. Its presence keeps
            // a future variant addition from silently compiling.
            static_assert(sizeof(T) == 0,
                "to_event(): unhandled EventSpec variant — concepts "
                "doc §5.1 added a variant without updating this seam");
        }
    }, spec);
}

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_CONVERSION_HPP
