// stdio_transport.hpp — host-only Transport reading stdin / writing stdout.
//
// PARITY: rlvgl/playit/src/transport.rs — same byte-level shape.
// LVGL:   N/A.
// DELTA:  Host-only. Embedded targets ship their own UART/USART
//         transport per board; this header is excluded under
//         LVGLPP_EMBEDDED_POSTURE.
//
// docs/playit-transport/00-transport-and-executor.md §5.3.

#ifndef LVGLPP_PLAYIT_STDIO_TRANSPORT_HPP
#define LVGLPP_PLAYIT_STDIO_TRANSPORT_HPP

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  error "lvglpp::playit::StdioTransport is host-only; do not include under LVGLPP_EMBEDDED_POSTURE."
#endif

#include <cstdint>
#include <optional>
#include <span>

#include "lvglpp/playit/transport.hpp"

namespace lvglpp::playit {

// Reads from stdin (set non-blocking on construction), writes to
// stdout. Single-instance — at most one StdioTransport may be alive
// at a time so the stdin flag manipulation is well-defined.
//
// Move-only: move-construct allowed (transfers ownership of the
// stdin-flag restoration), move-assign deleted.
class StdioTransport final : public Transport {
public:
    StdioTransport()  noexcept;
    ~StdioTransport() noexcept override;

    StdioTransport(const StdioTransport&)            = delete;
    StdioTransport& operator=(const StdioTransport&) = delete;
    StdioTransport(StdioTransport&& other) noexcept;
    StdioTransport& operator=(StdioTransport&&)      = delete;

    [[nodiscard]] std::optional<std::uint8_t> read_byte() noexcept override;
    void write_bytes(std::span<const std::uint8_t> bytes) noexcept override;

    // True once the upstream writer has closed stdin (`read()` returned
    // 0). Stays latched. Callers use this to exit a long-running event
    // loop cleanly: `cat fixture.txt | demo` should drain its commands
    // and terminate.
    [[nodiscard]] bool is_eof() const noexcept { return eof_; }

private:
    int  saved_flags_ = -1;
    bool owns_flags_  = false;
    bool eof_         = false;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_STDIO_TRANSPORT_HPP
