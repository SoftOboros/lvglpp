// gesture.hpp — TapRecognizer + DoubleTapRecognizer + GesturePipeline.
//
// PARITY: rlvgl/platform/src/gesture.rs (v0.2.0 @ b178cbc) — same
//         duration constants, same FSM, same tick math.
//         rlvgl/examples/disco-sim/src/main.rs:158
//         (DiscoGesturePipeline) — canonical composition reference.
// LVGL:   N/A.
// DELTA:  rlvgl places these in `rlvgl-platform`. lvglpp places
//         them in `lvglpp::playit` because PLAYIT already houses
//         the EventPipeline trait they compose with. Functionally
//         equivalent.
//
// docs/playit-recognizer/00-tap-and-double-tap.md (PLAYIT-04a).

#ifndef LVGLPP_PLAYIT_GESTURE_HPP
#define LVGLPP_PLAYIT_GESTURE_HPP

#include <cstdint>
#include <optional>
#include <variant>

#include "lvglpp/core/event.hpp"
#include "lvglpp/playit/event_pipeline.hpp"

namespace lvglpp::playit {

// PLAYIT-04a §5.1 — frozen duration constants.
inline constexpr std::uint32_t SETTLE_MS               = 200;
inline constexpr std::uint32_t SHORT_PRESS_MAX_MS      = 250;
inline constexpr std::uint32_t DOUBLE_TAP_WINDOW_MS    = 400;
inline constexpr std::int32_t  DOUBLE_TAP_MAX_DISTANCE = 20;

// PLAYIT-04a §5.2 — ms_to_ticks(ms, hz) = ceil(ms*hz / 1000).
[[nodiscard]] constexpr std::uint8_t
ms_to_ticks(std::uint32_t ms, std::uint32_t frame_hz) noexcept {
    return static_cast<std::uint8_t>((ms * frame_hz + 999U) / 1000U);
}

// ---------------------------------------------------------------------------
// TapRecognizer — PLAYIT-04a §5.3 / §5.4
// ---------------------------------------------------------------------------

class TapRecognizer {
public:
    explicit TapRecognizer(std::uint32_t frame_hz = 60) noexcept
        : max_settle_{ms_to_ticks(SETTLE_MS, frame_hz)} {}

    // Process a raw input event. Returns a gesture event for the
    // next stage, or std::nullopt if the event was consumed.
    [[nodiscard]] std::optional<::lvglpp::core::Event>
    process(const ::lvglpp::core::Event& event) noexcept {
        namespace e = ::lvglpp::core::event;

        if (const auto* pd = std::get_if<e::PointerDown>(&event)) {
            pos_x_ = pd->x;
            pos_y_ = pd->y;
            switch (state_) {
                case State::Idle:
                    state_ = State::Down;
                    return ::lvglpp::core::Event{e::PressDown{pd->x, pd->y}};
                case State::Down:
                    return std::nullopt;  // drag
                case State::PendingRelease:
                    // Bounce — re-arm Down, suppress repeat PressDown.
                    state_  = State::Down;
                    settle_ = 0;
                    return std::nullopt;
            }
        }
        if (const auto* pu = std::get_if<e::PointerUp>(&event)) {
            pos_x_ = pu->x;
            pos_y_ = pu->y;
            switch (state_) {
                case State::Down:
                    state_  = State::PendingRelease;
                    settle_ = max_settle_;
                    return std::nullopt;
                case State::PendingRelease:
                    settle_ = max_settle_;
                    return std::nullopt;
                case State::Idle:
                    return std::nullopt;  // spurious
            }
        }
        if (const auto* pm = std::get_if<e::PointerMove>(&event)) {
            if (state_ == State::Down) {
                pos_x_ = pm->x;
                pos_y_ = pm->y;
            }
            // Pass through for widgets that want move tracking.
            return event;
        }
        // All other events pass through unchanged.
        return event;
    }

    // Advance the settle timer. Call once per frame. Returns the
    // deferred PressRelease when settle expires.
    [[nodiscard]] std::optional<::lvglpp::core::Event> tick() noexcept {
        namespace e = ::lvglpp::core::event;
        if (state_ == State::PendingRelease) {
            if (settle_ > 0) --settle_;
            if (settle_ == 0) {
                state_ = State::Idle;
                return ::lvglpp::core::Event{e::PressRelease{pos_x_, pos_y_}};
            }
        }
        return std::nullopt;
    }

private:
    enum class State : std::uint8_t {
        Idle,
        Down,
        PendingRelease,
    };

    State         state_      = State::Idle;
    std::int32_t  pos_x_      = 0;
    std::int32_t  pos_y_      = 0;
    std::uint8_t  settle_     = 0;
    std::uint8_t  max_settle_;
};

// ---------------------------------------------------------------------------
// DoubleTapRecognizer — PLAYIT-04a §5.5 / §5.6
// ---------------------------------------------------------------------------

class DoubleTapRecognizer {
public:
    explicit DoubleTapRecognizer(std::uint32_t frame_hz = 60) noexcept
        : short_press_max_ticks_{ms_to_ticks(SHORT_PRESS_MAX_MS, frame_hz)},
          window_ticks_{ms_to_ticks(DOUBLE_TAP_WINDOW_MS, frame_hz)} {}

    // Process a gesture event from the upstream stage. Returns up
    // to two events to dispatch.
    [[nodiscard]] PipelineOutput
    process(const ::lvglpp::core::Event& event) noexcept {
        namespace e = ::lvglpp::core::event;

        if (const auto* pd = std::get_if<e::PressDown>(&event)) {
            (void)pd;
            down_tick_ = tick_counter_;
            return PipelineOutput{event, std::nullopt};
        }
        if (const auto* pr = std::get_if<e::PressRelease>(&event)) {
            const std::uint8_t hold_ticks =
                static_cast<std::uint8_t>(tick_counter_ - down_tick_);
            const bool is_short = hold_ticks <= short_press_max_ticks_;

            switch (state_) {
                case State::Idle:
                    if (is_short) {
                        state_      = State::Armed;
                        armed_x_    = pr->x;
                        armed_y_    = pr->y;
                        countdown_  = window_ticks_;
                        return PipelineOutput{std::nullopt, std::nullopt};
                    }
                    // Long press — pass through immediately.
                    return PipelineOutput{event, std::nullopt};

                case State::Armed: {
                    const std::int32_t dx = (armed_x_ - pr->x);
                    const std::int32_t dy = (armed_y_ - pr->y);
                    const std::int32_t dist =
                        (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);

                    if (is_short && dist <= DOUBLE_TAP_MAX_DISTANCE) {
                        state_     = State::Idle;
                        countdown_ = 0;
                        return PipelineOutput{
                            ::lvglpp::core::Event{
                                e::DoubleTap{pr->x, pr->y}},
                            std::nullopt};
                    }
                    // Out of range — emit the buffered first tap.
                    ::lvglpp::core::Event first{
                        e::PressRelease{armed_x_, armed_y_}};
                    if (is_short) {
                        // Re-arm with the new position.
                        armed_x_   = pr->x;
                        armed_y_   = pr->y;
                        countdown_ = window_ticks_;
                        return PipelineOutput{first, std::nullopt};
                    }
                    // Long press — emit both and go idle.
                    state_     = State::Idle;
                    countdown_ = 0;
                    return PipelineOutput{first, event};
                }
            }
        }
        // All other events pass through unchanged.
        return PipelineOutput{event, std::nullopt};
    }

    // Advance the double-tap window. Call once per frame.
    [[nodiscard]] std::optional<::lvglpp::core::Event> tick() noexcept {
        namespace e = ::lvglpp::core::event;
        tick_counter_ = static_cast<std::uint8_t>(tick_counter_ + 1U);

        if (state_ == State::Armed) {
            if (countdown_ > 0) --countdown_;
            if (countdown_ == 0) {
                state_ = State::Idle;
                return ::lvglpp::core::Event{
                    e::PressRelease{armed_x_, armed_y_}};
            }
        }
        return std::nullopt;
    }

private:
    enum class State : std::uint8_t { Idle, Armed };

    State         state_     = State::Idle;
    std::int32_t  armed_x_   = 0;
    std::int32_t  armed_y_   = 0;
    std::uint8_t  countdown_ = 0;
    std::uint8_t  down_tick_ = 0;
    std::uint8_t  tick_counter_ = 0;
    std::uint8_t  short_press_max_ticks_;
    std::uint8_t  window_ticks_;
};

// ---------------------------------------------------------------------------
// GesturePipeline — PLAYIT-04a §5.7
// ---------------------------------------------------------------------------

// Concrete EventPipeline composing TapRecognizer ∘ DoubleTapRecognizer.
// Mirrors rlvgl/examples/disco-sim/src/main.rs:158
// (DiscoGesturePipeline) — same composition order, same
// push_output policy.
class GesturePipeline final : public EventPipeline {
public:
    explicit GesturePipeline(std::uint32_t frame_hz = 60) noexcept
        : tap_{frame_hz}, double_tap_{frame_hz} {}

    [[nodiscard]] PipelineOutput
    process(const ::lvglpp::core::Event& event) noexcept override {
        auto tap_out = tap_.process(event);
        if (!tap_out.has_value()) {
            return PipelineOutput{std::nullopt, std::nullopt};
        }
        return double_tap_.process(*tap_out);
    }

    [[nodiscard]] PipelineOutput tick() noexcept override {
        PipelineOutput outputs{};

        if (auto tap_out = tap_.tick()) {
            auto pair = double_tap_.process(*tap_out);
            push_output(outputs, pair.primary);
            push_output(outputs, pair.secondary);
        }
        if (auto dtap_out = double_tap_.tick()) {
            push_output(outputs, dtap_out);
        }
        return outputs;
    }

    // Exposed for tests + applications that want to drive the
    // recogniser stages directly.
    [[nodiscard]] TapRecognizer&        tap()        noexcept { return tap_; }
    [[nodiscard]] DoubleTapRecognizer&  double_tap() noexcept { return double_tap_; }

private:
    static void push_output(
        PipelineOutput& outputs,
        const std::optional<::lvglpp::core::Event>& event) noexcept {
        if (!event.has_value()) return;
        if (!outputs.primary.has_value()) {
            outputs.primary = event;
        } else if (!outputs.secondary.has_value()) {
            outputs.secondary = event;
        }
        // If both slots are full we silently drop — mirrors
        // rlvgl/examples/disco-sim/src/main.rs:165.
    }

    TapRecognizer       tap_;
    DoubleTapRecognizer double_tap_;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_GESTURE_HPP
