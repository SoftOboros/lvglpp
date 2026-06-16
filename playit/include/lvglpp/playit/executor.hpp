// executor.hpp — line accumulator + dispatch loop driving a Transport.
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.0 @ 79f730d) — the
//         per-frame poll() shape (read bytes / accumulate lines /
//         dispatch / format / write).
// LVGL:   N/A.
// DELTA:  rlvgl's PlayitExecutor owns transport + dispatcher +
//         recorder + dump-state. lvglpp splits these: Executor is a
//         thin pump that holds Transport& + any dispatcher. Recorder
//         (PLAYIT-06) lives behind the dispatcher.
//
//         LVGLPP-WRAP-0N: Executor is dispatcher-AGNOSTIC. The constructor
//         is a template that type-erases the dispatcher to a function
//         pointer (no virtual, embedded-safe), so it pumps either the
//         hand-rolled WidgetNode Dispatcher or the lv_obj ObjDispatcher —
//         any type with `Response dispatch(const Command&)`.
//
// docs/playit-transport/00-transport-and-executor.md (PLAYIT-07).

#ifndef LVGLPP_PLAYIT_EXECUTOR_HPP
#define LVGLPP_PLAYIT_EXECUTOR_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "lvglpp/playit/command.hpp"
#include "lvglpp/playit/event_recorder.hpp"
#include "lvglpp/playit/framebuffer.hpp"
#include "lvglpp/playit/response.hpp"
#include "lvglpp/playit/transport.hpp"

namespace lvglpp::playit {

// Concepts doc §5.2 / §5.4 defaults.
inline constexpr std::size_t LINE_BUF_BYTES   = 256;
inline constexpr std::size_t POLL_MAX_BYTES   = 4096;
inline constexpr std::size_t RESPONSE_BUF_BYTES = 128;

class Executor {
public:
    // Args:
    //   transport:  borrows for the executor's lifetime.
    //   dispatcher: borrows for the executor's lifetime. Any type exposing
    //               `Response dispatch(const Command&)` (Dispatcher or
    //               ObjDispatcher). Type-erased to a function pointer.
    template <class Dispatch>
    Executor(Transport& transport, Dispatch& dispatcher) noexcept
        : transport_{&transport},
          dispatcher_{&dispatcher},
          dispatch_fn_{[](void* d, const Command& cmd) noexcept -> Response {
              return static_cast<Dispatch*>(d)->dispatch(cmd);
          }} {}

    // Attach an EventRecorder. While set, the Executor intercepts
    // RS / RE / RD commands (handling them at the Executor level
    // per PLAYIT-06 §5.4) and records EventSpec from Inject /
    // InjectTagged commands when the recorder is running.
    //
    // Args:
    //   recorder: borrows for the executor's lifetime, or nullptr
    //             to detach.
    void set_recorder(EventRecorder* recorder) noexcept {
        recorder_ = recorder;
    }

    // Attach a FramebufferReader (PLAYIT-07a). While set, the
    // Executor intercepts `D` (DumpPixels) commands: queue on
    // accept (`DUMP:queued`), then emit one frame per observed
    // present-count change from poll() — mirrors rlvgl
    // executor.rs::emit_dump_if_ready. Without a reader, `D` falls
    // through to the Dispatcher (not-implemented error, the
    // pre-07a behaviour).
    //
    // Args:
    //   reader: borrows for the executor's lifetime, or nullptr to
    //           detach (cancels any queued dump).
    void set_framebuffer_reader(FramebufferReader* reader) noexcept {
        fb_ = reader;
        if (reader == nullptr) dump_.reset();
    }

    // Drain currently-available bytes. For each newline-terminated
    // line, parse + dispatch + format + write the response.
    // Returns the number of complete lines processed this call.
    // noexcept: malformed input is returned as Response::Error.
    std::size_t poll() noexcept;

private:
    // In-progress framebuffer dump (mirrors rlvgl's DumpState).
    struct DumpState {
        DumpSpec      spec{};
        std::uint8_t  remaining = 0;
        std::uint32_t last_present_seen = 0;
    };

    void dispatch_line() noexcept;
    void dump_recording() noexcept;
    void emit_dump_if_ready() noexcept;

    // Dispatch through the type-erased dispatcher.
    [[nodiscard]] Response dispatch(const Command& cmd) noexcept {
        return dispatch_fn_(dispatcher_, cmd);
    }

    // borrows: caller-owned.
    Transport*         transport_;
    void*              dispatcher_;  // borrows: the concrete dispatcher.
    Response (*dispatch_fn_)(void*, const Command&) noexcept;  // trampoline.
    EventRecorder*     recorder_ = nullptr;
    FramebufferReader* fb_       = nullptr;

    // owns: queued dump request, present-gated (PLAYIT-07a).
    std::optional<DumpState> dump_{};

    // owns: per-line accumulator. Bytes are appended until '\n'
    // arrives; CRLF tolerance strips a trailing '\r'.
    std::array<char, LINE_BUF_BYTES> line_buf_{};
    std::size_t                      line_len_  = 0;
    bool                             overflowed_ = false;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_EXECUTOR_HPP
