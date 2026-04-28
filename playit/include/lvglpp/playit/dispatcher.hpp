// dispatcher.hpp — routes parsed playit Commands into a WidgetNode tree.
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.0 @ 79f730d) — the
//         per-command dispatch logic. lvglpp's Dispatcher is the
//         minimal core of that executor: command → Response, no
//         transport, no recorder (PLAYIT-06), no framebuffer dump
//         (PLAYIT-05).
// LVGL:   N/A.
// DELTA:  rlvgl's executor owns the transport + the recorder + the
//         status snapshot; lvglpp splits these. Dispatcher only
//         knows about (root, status_snapshot); transport and
//         recorder land in their own sub-phases.
//
// This file implements docs/playit-tagged/00-tagged-queries.md
// (PLAYIT-04).

#ifndef LVGLPP_PLAYIT_DISPATCHER_HPP
#define LVGLPP_PLAYIT_DISPATCHER_HPP

#include <variant>

#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/playit/command.hpp"
#include "lvglpp/playit/conversion.hpp"
#include "lvglpp/playit/response.hpp"

namespace lvglpp::playit {

class Dispatcher {
public:
    // Args:
    //   root: borrows mut WidgetNode for the dispatcher's lifetime.
    //         Caller MUST keep the tree alive at least as long as
    //         this Dispatcher.
    explicit Dispatcher(::lvglpp::core::WidgetNode& root) noexcept
        : root_{&root} {}

    // Replace the StatusData snapshot returned for `Status` commands.
    void set_status_snapshot(StatusData snapshot) noexcept {
        status_ = snapshot;
    }

    // Process one parsed command. noexcept: any internal failure
    // produces a Response::Error rather than throwing.
    [[nodiscard]] Response dispatch(const Command& cmd) noexcept;

private:
    // borrows: caller-owned WidgetNode tree.
    ::lvglpp::core::WidgetNode* root_;
    StatusData                  status_{};
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_DISPATCHER_HPP
