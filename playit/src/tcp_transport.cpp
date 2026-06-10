// tcp_transport.cpp — loopback TCP transport implementation.
//
// PARITY: rlvgl/playit/src/tcp.rs (v0.2.0 @ 79f730d) — see header.

#include "lvglpp/playit/tcp_transport.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <utility>

namespace lvglpp::playit {

namespace {

[[nodiscard]] bool set_nonblocking(int fd) noexcept {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    return flags >= 0 && ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

}  // namespace

std::optional<TcpServerTransport>
TcpServerTransport::bind_loopback(std::uint16_t port) noexcept {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return std::nullopt;

    const int one = 1;
    (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port        = htons(port);
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&addr),
               sizeof(addr)) < 0 ||
        ::listen(fd, 1) < 0 || !set_nonblocking(fd)) {
        ::close(fd);
        return std::nullopt;
    }

    // Resolve the OS-assigned port for the PLAYIT_READY line.
    sockaddr_in bound{};
    socklen_t   len = sizeof(bound);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &len) < 0) {
        ::close(fd);
        return std::nullopt;
    }
    return TcpServerTransport{fd, ntohs(bound.sin_port)};
}

TcpServerTransport::TcpServerTransport(TcpServerTransport&& other) noexcept
    : listener_fd_{std::exchange(other.listener_fd_, -1)},
      stream_fd_{std::exchange(other.stream_fd_, -1)},
      port_{other.port_},
      read_buf_{std::move(other.read_buf_)},
      write_buf_{std::move(other.write_buf_)} {}

TcpServerTransport::~TcpServerTransport() {
    if (stream_fd_ >= 0) ::close(stream_fd_);
    if (listener_fd_ >= 0) ::close(listener_fd_);
}

void TcpServerTransport::poll_accept() noexcept {
    if (stream_fd_ >= 0 || listener_fd_ < 0) return;
    const int fd = ::accept(listener_fd_, nullptr, nullptr);
    if (fd < 0) return;  // EAGAIN/EWOULDBLOCK included — no client yet.
    if (!set_nonblocking(fd)) {
        ::close(fd);
        return;
    }
    stream_fd_ = fd;
}

void TcpServerTransport::drop_stream() noexcept {
    if (stream_fd_ >= 0) ::close(stream_fd_);
    stream_fd_ = -1;
    read_buf_.clear();
    write_buf_.clear();
}

void TcpServerTransport::flush_write_buf() noexcept {
    poll_accept();
    if (stream_fd_ < 0) return;
    while (!write_buf_.empty()) {
        // deque is not contiguous; flush in bounded chunks.
        std::array<std::uint8_t, 512> chunk{};
        std::size_t n = 0;
        for (std::uint8_t b : write_buf_) {
            if (n == chunk.size()) break;
            chunk[n++] = b;
        }
        const ssize_t written = ::send(stream_fd_, chunk.data(), n, 0);
        if (written > 0) {
            write_buf_.erase(write_buf_.begin(),
                             write_buf_.begin() + written);
            continue;
        }
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
        drop_stream();  // 0 = peer closed; other errors fatal (tcp.rs)
        return;
    }
}

void TcpServerTransport::fill_read_buf() noexcept {
    poll_accept();
    if (stream_fd_ < 0) return;
    std::array<std::uint8_t, 512> chunk{};
    for (;;) {
        const ssize_t got =
            ::recv(stream_fd_, chunk.data(), chunk.size(), 0);
        if (got > 0) {
            read_buf_.insert(read_buf_.end(), chunk.data(),
                             chunk.data() + got);
            if (static_cast<std::size_t>(got) < chunk.size()) return;
            continue;
        }
        if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
        drop_stream();  // 0 = orderly shutdown; mirror tcp.rs
        return;
    }
}

std::optional<std::uint8_t> TcpServerTransport::read_byte() noexcept {
    if (read_buf_.empty()) fill_read_buf();
    if (read_buf_.empty()) return std::nullopt;
    const std::uint8_t b = read_buf_.front();
    read_buf_.pop_front();
    return b;
}

void TcpServerTransport::write_bytes(
    std::span<const std::uint8_t> bytes) noexcept {
    write_buf_.insert(write_buf_.end(), bytes.begin(), bytes.end());
    flush_write_buf();
}

}  // namespace lvglpp::playit
