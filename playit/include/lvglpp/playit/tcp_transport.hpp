// tcp_transport.hpp — loopback TCP transport for simulator automation.
//
// PARITY: rlvgl/playit/src/tcp.rs (TcpServerTransport, v0.2.0 @
//         79f730d) — single non-blocking loopback client, FIFO
//         write buffer, drop-stream-and-relisten on error/EOF.
// LVGL:   N/A.
// DELTA:  POSIX sockets instead of std::net; no Windows support
//         (host sims run on POSIX today).
//
// Host-only (same CMake gate as StdioTransport — pulls <sys/socket.h>).
// docs/disco-demo/07-sim-automation.md §5.4.

#ifndef LVGLPP_PLAYIT_TCP_TRANSPORT_HPP
#define LVGLPP_PLAYIT_TCP_TRANSPORT_HPP

#include <cstdint>
#include <deque>
#include <optional>
#include <span>

#include "lvglpp/playit/transport.hpp"

namespace lvglpp::playit {

// Single-client loopback TCP transport for playit automation.
//
// Ownership: owns the listener and (when connected) client socket
// fds; both closed in the destructor. Move-only.
class TcpServerTransport final : public Transport {
public:
    // Bind a loopback listener on `port` (0 = OS-assigned). Returns
    // std::nullopt on bind failure.
    [[nodiscard]] static std::optional<TcpServerTransport>
    bind_loopback(std::uint16_t port) noexcept;

    TcpServerTransport(const TcpServerTransport&)            = delete;
    TcpServerTransport& operator=(const TcpServerTransport&) = delete;
    TcpServerTransport(TcpServerTransport&& other) noexcept;
    TcpServerTransport& operator=(TcpServerTransport&&)      = delete;
    ~TcpServerTransport() override;

    // The bound port (resolves OS-assigned when constructed with 0).
    [[nodiscard]] std::uint16_t local_port() const noexcept {
        return port_;
    }

    // Transport: non-blocking read; accepts a pending client first.
    [[nodiscard]] std::optional<std::uint8_t> read_byte() noexcept override;

    // Transport: buffer then best-effort flush (mirrors tcp.rs
    // write_bytes + flush_write_buf).
    void write_bytes(std::span<const std::uint8_t> bytes) noexcept override;

private:
    TcpServerTransport(int listener_fd, std::uint16_t port) noexcept
        : listener_fd_{listener_fd}, port_{port} {}

    void poll_accept() noexcept;
    void drop_stream() noexcept;
    void flush_write_buf() noexcept;
    void fill_read_buf() noexcept;

    int listener_fd_ = -1;  // owns: closed in destructor.
    int stream_fd_   = -1;  // owns: current client, -1 = none.
    std::uint16_t port_ = 0;
    std::deque<std::uint8_t> read_buf_;
    std::deque<std::uint8_t> write_buf_;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_TCP_TRANSPORT_HPP
