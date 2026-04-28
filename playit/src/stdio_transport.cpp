// stdio_transport.cpp — non-blocking stdin / blocking-best-effort stdout.

#include "lvglpp/playit/stdio_transport.hpp"

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

namespace lvglpp::playit {

namespace {

// Single-instance flag — at most one StdioTransport alive at a time.
std::atomic<bool> g_stdio_alive{false};

bool acquire_stdio_slot() noexcept {
    bool expected = false;
    return g_stdio_alive.compare_exchange_strong(expected, true);
}

void release_stdio_slot() noexcept {
    g_stdio_alive.store(false);
}

}  // namespace

StdioTransport::StdioTransport() noexcept {
    if (!acquire_stdio_slot()) {
        // Programmer error: another StdioTransport already exists.
        // Embedded posture is unreachable here (the header
        // #errors); host posture treats double-construction as a
        // hard programmer error.
        std::abort();
    }
    saved_flags_ = ::fcntl(STDIN_FILENO, F_GETFL, 0);
    if (saved_flags_ < 0) {
        // stdin isn't a fd — leave alone, accept blocking reads.
        owns_flags_ = false;
        return;
    }
    if (::fcntl(STDIN_FILENO, F_SETFL, saved_flags_ | O_NONBLOCK) == 0) {
        owns_flags_ = true;
    }
}

StdioTransport::StdioTransport(StdioTransport&& other) noexcept
    : saved_flags_{other.saved_flags_},
      owns_flags_{other.owns_flags_},
      eof_{other.eof_} {
    other.saved_flags_ = -1;
    other.owns_flags_  = false;
    other.eof_         = false;
}

StdioTransport::~StdioTransport() noexcept {
    if (owns_flags_ && saved_flags_ >= 0) {
        (void)::fcntl(STDIN_FILENO, F_SETFL, saved_flags_);
    }
    if (owns_flags_) {
        // Only the original instance releases the singleton slot.
        release_stdio_slot();
    } else if (saved_flags_ != -1) {
        // Edge case: constructor acquired the slot but couldn't get
        // stdin flags — still release the slot.
        release_stdio_slot();
    }
}

std::optional<std::uint8_t> StdioTransport::read_byte() noexcept {
    std::uint8_t b = 0;
    const ssize_t n = ::read(STDIN_FILENO, &b, 1);
    if (n == 1) {
        return b;
    }
    if (n == 0) {
        // True EOF — upstream writer closed the pipe / pressed
        // Ctrl-D. Latch the flag so callers can exit cleanly.
        eof_ = true;
    }
    // n < 0 with EAGAIN/EWOULDBLOCK / EINTR → "no byte right now".
    // Do not latch eof_; the caller may read again later.
    return std::nullopt;
}

void StdioTransport::write_bytes(std::span<const std::uint8_t> bytes) noexcept {
    std::size_t total = 0;
    while (total < bytes.size()) {
        const ssize_t n = ::write(STDOUT_FILENO,
                                  bytes.data() + total,
                                  bytes.size() - total);
        if (n > 0) {
            total += static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        // EAGAIN / EWOULDBLOCK / other errors → give up; partial
        // writes are documented in the chapter §5.1.
        break;
    }
}

}  // namespace lvglpp::playit
