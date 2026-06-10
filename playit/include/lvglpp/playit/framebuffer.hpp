// framebuffer.hpp — read-only framebuffer inspection seam.
//
// PARITY: rlvgl/playit/src/framebuffer.rs (FramebufferReader trait,
//         v0.2.0 @ 79f730d) — read_pixel / read_row / present_count.
// LVGL:   N/A.
// DELTA:  none (abstract base instead of trait).
//
// Platform backends implement this to expose the visible framebuffer
// to the Executor for `D` dump commands (PLAYIT-07a).

#ifndef LVGLPP_PLAYIT_FRAMEBUFFER_HPP
#define LVGLPP_PLAYIT_FRAMEBUFFER_HPP

#include <cstdint>
#include <span>

namespace lvglpp::playit {

// Read-only access to the display front buffer.
//
// Ownership: non-owning interface; the implementor owns the pixel
// storage. The Executor borrows the reader (set_framebuffer_reader)
// for its lifetime and never mutates through it.
class FramebufferReader {
public:
    FramebufferReader()                                     = default;
    FramebufferReader(const FramebufferReader&)             = default;
    FramebufferReader(FramebufferReader&&) noexcept         = default;
    FramebufferReader& operator=(const FramebufferReader&)  = default;
    FramebufferReader& operator=(FramebufferReader&&) noexcept = default;
    virtual ~FramebufferReader()                            = default;

    // Single pixel at landscape (x, y), ARGB8888. The implementor
    // owns any coordinate transform (portrait panels included).
    [[nodiscard]] virtual std::uint32_t
    read_pixel(std::int32_t x, std::int32_t y) const = 0;

    // Horizontal run starting at (x, y) into `out`. Returns the
    // number of pixels written (less than `width` when the run
    // leaves the framebuffer).
    virtual std::size_t read_row(std::int32_t x,
                                 std::int32_t y,
                                 std::uint16_t width,
                                 std::span<std::uint32_t> out) const = 0;

    // Display present count — gates frame-synchronised dumps.
    [[nodiscard]] virtual std::uint32_t present_count() const = 0;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_FRAMEBUFFER_HPP
