// fbdev_display.hpp — generic Linux framebuffer display backend.
//
// PARITY: rlvgl/platform/src/linux_fbdev.rs (v0.2.0 @ 79f730d) —
//         open/ioctl/mmap sequence, the 32/24/16bpp format set, and
//         the row-by-row dirty-rect flush.
// LVGL:   lvgl/src/drivers/display/fb — informative only.
// DELTA:  namespace `lnx` (not `linux`: that identifier is a
//         predefined macro under GNU dialects). Open errors are an
//         enum + optional instead of io::Error.
//
// docs/platform-linux/00-fbdev-evdev.md (PLAT-LNX) §5.1. Linux-only
// (LVGLPP_PLATFORM_LINUX_FBDEV).

#ifndef LVGLPP_PLATFORM_LNX_FBDEV_DISPLAY_HPP
#define LVGLPP_PLATFORM_LNX_FBDEV_DISPLAY_HPP

#include <cstdint>
#include <optional>
#include <span>

#include "lvglpp/core/widget.hpp"  // core::Color, core::Rect

namespace lvglpp::platform::lnx {

// Owns the framebuffer fd and the mmap'd pixel region; both released
// in the destructor. Move-only.
class FbdevDisplay {
public:
    // Open `path` (e.g. "/dev/fb0"), query FBIOGET_VSCREENINFO /
    // FBIOGET_FSCREENINFO, mmap MAP_SHARED. Returns nullopt on any
    // failure (open/ioctl/mmap/unsupported bpp).
    [[nodiscard]] static std::optional<FbdevDisplay>
    open(const char* path) noexcept;

    FbdevDisplay(const FbdevDisplay&)            = delete;
    FbdevDisplay& operator=(const FbdevDisplay&) = delete;
    FbdevDisplay(FbdevDisplay&& other) noexcept;
    FbdevDisplay& operator=(FbdevDisplay&&)      = delete;
    ~FbdevDisplay();

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::uint32_t bits_per_pixel() const noexcept {
        return bpp_;
    }

    // Write `colors` (row-major, rect.width × rect.height) into the
    // device, converting ARGB → the device format (32bpp BGRA8888 /
    // 24bpp BGR888 / 16bpp RGB565 — linux_fbdev.rs:233–262), honouring
    // line_length. Rect is clipped to the screen. Dirty-rect
    // coalescing is the caller's job (rlvgl parity).
    //
    // Args:
    //   colors: borrows for the call; size must be ≥ rect area.
    void flush(core::Rect rect,
               std::span<const core::Color> colors) noexcept;

private:
    FbdevDisplay() = default;

    int            fd_   = -1;       // owns
    std::uint8_t*  mem_  = nullptr;  // owns (munmap), mmio-like: device memory
    std::size_t    mlen_ = 0;
    std::uint32_t  width_ = 0, height_ = 0, bpp_ = 0;
    std::uint32_t  line_length_ = 0;  // bytes per device row
};

}  // namespace lvglpp::platform::lnx

#endif  // LVGLPP_PLATFORM_LNX_FBDEV_DISPLAY_HPP
