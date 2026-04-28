// transport.hpp — byte-level I/O abstraction for playit.
//
// PARITY: rlvgl/playit/src/transport.rs:11 (PlayitTransport).
// LVGL:   N/A.
// DELTA:  rlvgl uses Option<u8>; lvglpp uses std::optional<uint8_t>.
//         Identical semantics — None == "no data right now".

#ifndef LVGLPP_PLAYIT_TRANSPORT_HPP
#define LVGLPP_PLAYIT_TRANSPORT_HPP

#include <cstdint>
#include <optional>
#include <span>

namespace lvglpp::playit {

class Transport {
public:
    Transport()                                = default;
    Transport(const Transport&)                = default;
    Transport(Transport&&) noexcept            = default;
    Transport& operator=(const Transport&)     = default;
    Transport& operator=(Transport&&) noexcept = default;
    virtual ~Transport()                       = default;

    // Non-blocking single-byte read. Returns std::nullopt when no
    // data is available. Implementations MUST NOT block.
    [[nodiscard]] virtual std::optional<std::uint8_t>
    read_byte() noexcept = 0;

    // Best-effort write of the full span. Implementations MAY block
    // briefly to flush but MUST NOT block indefinitely.
    virtual void write_bytes(std::span<const std::uint8_t> bytes) noexcept = 0;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_TRANSPORT_HPP
