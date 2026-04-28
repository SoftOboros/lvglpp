// event_recorder.hpp — fixed-capacity timed event recorder.
//
// PARITY: rlvgl/playit/src/recorder.rs (v0.2.0 @ 79f730d) — after
//         PLAYIT-06a (tick-delta) this is a byte-for-byte parity
//         port: same Entry shape, same fill-and-stop semantics,
//         same saturating delta computation.
// LVGL:   N/A.
//
// docs/playit-recorder/00-event-recorder.md (PLAYIT-06) +
// docs/playit-recorder/01-tick-delta.md (PLAYIT-06a, supersedes
// the v1 monotonic-seq form).

#ifndef LVGLPP_PLAYIT_EVENT_RECORDER_HPP
#define LVGLPP_PLAYIT_EVENT_RECORDER_HPP

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "lvglpp/playit/event_spec.hpp"

namespace lvglpp::playit {

class EventRecorder {
public:
    // §5.1 — fixed capacity (mirrors rlvgl's default-N=256).
    static constexpr std::size_t CAPACITY = 256;

    // PLAYIT-06a §5.1: tick_delta (uint16_t saturating). Replaces
    // the v1 monotonic-seq form.
    struct Entry {
        std::uint16_t tick_delta;  // ticks since previous record (0 for first)
        EventSpec     spec;
    };

    // Begin recording. Clears the buffer and resets tick counters
    // (rlvgl `start()` semantics — see PLAYIT-06a §5.4).
    void start() noexcept {
        running_         = true;
        len_             = 0;
        tick_counter_    = 0;
        last_event_tick_ = 0;
    }

    // Stop recording. The buffer is preserved for the dump path.
    void stop() noexcept { running_ = false; }

    [[nodiscard]] bool        running() const noexcept { return running_; }
    [[nodiscard]] bool        is_full() const noexcept { return len_ >= CAPACITY; }
    [[nodiscard]] std::size_t size()    const noexcept { return len_; }
    [[nodiscard]] bool        empty()   const noexcept { return len_ == 0; }

    // Advance the tick counter — call once per main-loop frame.
    // No-op while stopped (rlvgl semantics).
    void tick() noexcept {
        if (running_) {
            // Wrapping add (rlvgl's `wrapping_add`).
            tick_counter_ = static_cast<std::uint32_t>(tick_counter_ + 1U);
        }
    }

    // Record an event. No-op while stopped or while full;
    // fill auto-stops (PLAYIT-06a §5.4).
    void record(const EventSpec& spec) noexcept {
        if (!running_) return;
        if (len_ >= CAPACITY) {
            running_ = false;
            return;
        }

        // Saturating delta (PLAYIT-06a §5.3). The `tick_counter_ <
        // last_event_tick_` branch covers the u32-wrap case (rlvgl
        // uses `saturating_sub`, which yields 0 when the result
        // would underflow).
        const std::uint32_t delta_u32 =
            (tick_counter_ >= last_event_tick_)
                ? (tick_counter_ - last_event_tick_)
                : 0U;
        const std::uint16_t delta_u16 =
            (delta_u32 > static_cast<std::uint32_t>(UINT16_MAX))
                ? static_cast<std::uint16_t>(UINT16_MAX)
                : static_cast<std::uint16_t>(delta_u32);

        entries_[len_] = Entry{delta_u16, spec};
        ++len_;
        last_event_tick_ = tick_counter_;

        if (len_ >= CAPACITY) {
            running_ = false;
        }
    }

    // Visit each entry in chronological order.
    template <class F>
    void for_each(F&& f) const noexcept(noexcept(f(std::declval<const Entry&>()))) {
        for (std::size_t i = 0; i < len_; ++i) {
            f(entries_[i]);
        }
    }

private:
    std::array<Entry, CAPACITY> entries_{};
    std::size_t                 len_             = 0;
    std::uint32_t               tick_counter_    = 0;
    std::uint32_t               last_event_tick_ = 0;
    bool                        running_         = false;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_EVENT_RECORDER_HPP
