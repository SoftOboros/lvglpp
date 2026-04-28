// event_pipeline.hpp — gesture-routing seam for parsed events.
//
// PARITY: rlvgl/playit/src/executor.rs:56 (EventPipeline trait),
//         :64 (NullPipeline). v0.2.0 @ 79f730d.
// LVGL:   N/A.
// DELTA:  rlvgl returns `(Option<Event>, Option<Event>)`; lvglpp
//         returns a small POD `PipelineOutput` carrying the same
//         pair of optional events. Same semantics, more explicit
//         shape.
//
// The pipeline sits between the parsed Command (PLAYIT-01) and the
// widget tree (CORE-03 / future). It transforms one input event into
// zero, one, or two output events — the second slot exists so
// gesture recognisers can fan out (e.g. PressDown raw event into
// PressDown debounced + an internal timer event).
//
// PLAYIT-02 ratifies the surface; concrete pipelines (e.g. the
// rlvgl-style PressRelease debouncer + DoubleTap recogniser) land
// as follow-up sub-phases.

#ifndef LVGLPP_PLAYIT_EVENT_PIPELINE_HPP
#define LVGLPP_PLAYIT_EVENT_PIPELINE_HPP

#include <optional>

#include "lvglpp/core/event.hpp"

namespace lvglpp::playit {

// Output of a single pipeline step. Both fields default to nullopt;
// a recogniser that consumes input without producing output returns
// `{nullopt, nullopt}`.
struct PipelineOutput {
    std::optional<::lvglpp::core::Event> primary;
    std::optional<::lvglpp::core::Event> secondary;

    bool operator==(const PipelineOutput&) const noexcept = default;
};

// Abstract base for an event-routing pipeline. Concrete pipelines
// implement `process` (per-event) and `tick` (per-frame timer
// advance).
//
// Ownership: the pipeline holds its own internal state; callers
// borrow the EventPipeline by mutable reference for the duration of
// a process()/tick() call. Pipelines MUST be safe to call repeatedly
// across frames.
class EventPipeline {
public:
    EventPipeline()                                    = default;
    EventPipeline(const EventPipeline&)                = default;
    EventPipeline(EventPipeline&&) noexcept            = default;
    EventPipeline& operator=(const EventPipeline&)     = default;
    EventPipeline& operator=(EventPipeline&&) noexcept = default;
    virtual ~EventPipeline()                           = default;

    // Transform one input event into zero, one, or two output events.
    // Args:
    //   event: borrows Event for the duration of the call.
    [[nodiscard]] virtual PipelineOutput
    process(const ::lvglpp::core::Event& event) = 0;

    // Advance internal timers. Called once per frame.
    [[nodiscard]] virtual PipelineOutput tick() = 0;
};

// No-op pipeline that passes events through unchanged. Mirrors
// rlvgl/playit/src/executor.rs:64. Useful as a default in tests and
// as the leaf pipeline when no recogniser is needed.
class NullPipeline final : public EventPipeline {
public:
    [[nodiscard]] PipelineOutput
    process(const ::lvglpp::core::Event& event) override {
        return PipelineOutput{event, std::nullopt};
    }

    [[nodiscard]] PipelineOutput tick() override {
        return PipelineOutput{std::nullopt, std::nullopt};
    }
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_EVENT_PIPELINE_HPP
