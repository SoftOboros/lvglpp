// executor.hpp — line accumulator + dispatch loop driving a Transport.
//
// PARITY: rlvgl/playit/src/executor.rs (v0.2.0 @ 79f730d) — the
//         per-frame poll() shape (read bytes / accumulate lines /
//         dispatch / format / write).
// LVGL:   N/A.
// DELTA:  rlvgl's PlayitExecutor owns transport + dispatcher +
//         recorder + dump-state. lvglpp splits these: Executor is a
//         thin pump that holds Transport& + Dispatcher&. Recorder
//         (PLAYIT-06) lives behind the Dispatcher.
//
// docs/playit-transport/00-transport-and-executor.md (PLAYIT-07).

#ifndef LVGLPP_PLAYIT_EXECUTOR_HPP
#define LVGLPP_PLAYIT_EXECUTOR_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/event_recorder.hpp"
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
    //   dispatcher: borrows for the executor's lifetime.
    Executor(Transport& transport, Dispatcher& dispatcher) noexcept
        : transport_{&transport}, dispatcher_{&dispatcher} {}

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

    // Drain currently-available bytes. For each newline-terminated
    // line, parse + dispatch + format + write the response.
    // Returns the number of complete lines processed this call.
    // noexcept: malformed input is returned as Response::Error.
    std::size_t poll() noexcept;

private:
    void dispatch_line() noexcept;
    void dump_recording() noexcept;

    // borrows: caller-owned.
    Transport*     transport_;
    Dispatcher*    dispatcher_;
    EventRecorder* recorder_   = nullptr;

    // owns: per-line accumulator. Bytes are appended until '\n'
    // arrives; CRLF tolerance strips a trailing '\r'.
    std::array<char, LINE_BUF_BYTES> line_buf_{};
    std::size_t                      line_len_  = 0;
    bool                             overflowed_ = false;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_EXECUTOR_HPP
