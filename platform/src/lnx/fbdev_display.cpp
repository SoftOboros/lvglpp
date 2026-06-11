// fbdev_display.cpp — Linux framebuffer display backend.
//
// PARITY: rlvgl/platform/src/linux_fbdev.rs:116–265 (v0.2.0 @
//         79f730d) — open/FBIOGET_*/mmap, the 32/24/16bpp converter
//         set, row-by-row rect flush honouring line_length.
//
// Linux-only translation unit (PLAT-LNX §5.2); the CMake gate
// guarantees __linux__ here.

#include "lvglpp/platform/lnx/fbdev_display.hpp"

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace lvglpp::platform::lnx {

std::optional<FbdevDisplay> FbdevDisplay::open(const char* path) noexcept {
    const int fd = ::open(path, O_RDWR);
    if (fd < 0) return std::nullopt;

    fb_var_screeninfo vinfo{};
    fb_fix_screeninfo finfo{};
    if (::ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0 ||
        ::ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        ::close(fd);
        return std::nullopt;
    }
    if (vinfo.bits_per_pixel != 32 && vinfo.bits_per_pixel != 24 &&
        vinfo.bits_per_pixel != 16) {
        ::close(fd);  // linux_fbdev.rs: unsupported format is an error
        return std::nullopt;
    }

    const std::size_t mlen =
        static_cast<std::size_t>(finfo.line_length) * vinfo.yres;
    void* mem = ::mmap(nullptr, mlen, PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        ::close(fd);
        return std::nullopt;
    }

    FbdevDisplay d;
    d.fd_          = fd;
    d.mem_         = static_cast<std::uint8_t*>(mem);
    d.mlen_        = mlen;
    d.width_       = vinfo.xres;
    d.height_      = vinfo.yres;
    d.bpp_         = vinfo.bits_per_pixel;
    d.line_length_ = finfo.line_length;
    return d;
}

FbdevDisplay::FbdevDisplay(FbdevDisplay&& other) noexcept
    : fd_{other.fd_}, mem_{other.mem_}, mlen_{other.mlen_},
      width_{other.width_}, height_{other.height_}, bpp_{other.bpp_},
      line_length_{other.line_length_} {
    other.fd_  = -1;
    other.mem_ = nullptr;
}

FbdevDisplay::~FbdevDisplay() {
    if (mem_ != nullptr) ::munmap(mem_, mlen_);
    if (fd_ >= 0) ::close(fd_);
}

void FbdevDisplay::flush(core::Rect rect,
                         std::span<const core::Color> colors) noexcept {
    if (mem_ == nullptr) return;
    const std::int32_t w = static_cast<std::int32_t>(width_);
    const std::int32_t h = static_cast<std::int32_t>(height_);
    std::int32_t x0 = rect.x < 0 ? 0 : rect.x;
    std::int32_t y0 = rect.y < 0 ? 0 : rect.y;
    std::int32_t x1 = rect.x + rect.width;
    std::int32_t y1 = rect.y + rect.height;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;

    for (std::int32_t y = y0; y < y1; ++y) {
        std::uint8_t* row =
            mem_ + static_cast<std::size_t>(y) * line_length_;
        for (std::int32_t x = x0; x < x1; ++x) {
            const std::size_t src_idx =
                static_cast<std::size_t>(y - rect.y) *
                    static_cast<std::size_t>(rect.width) +
                static_cast<std::size_t>(x - rect.x);
            if (src_idx >= colors.size()) return;
            const core::Color c = colors[src_idx];
            switch (bpp_) {
                case 32: {  // BGRA8888 (linux_fbdev.rs:233)
                    std::uint8_t* p = row + static_cast<std::size_t>(x) * 4;
                    p[0] = c.b; p[1] = c.g; p[2] = c.r; p[3] = c.a;
                    break;
                }
                case 24: {  // BGR888
                    std::uint8_t* p = row + static_cast<std::size_t>(x) * 3;
                    p[0] = c.b; p[1] = c.g; p[2] = c.r;
                    break;
                }
                case 16: {  // RGB565 little-endian
                    const std::uint16_t v = static_cast<std::uint16_t>(
                        ((c.r & 0xF8u) << 8) | ((c.g & 0xFCu) << 3) |
                        (c.b >> 3));
                    std::uint8_t* p = row + static_cast<std::size_t>(x) * 2;
                    p[0] = static_cast<std::uint8_t>(v & 0xFFu);
                    p[1] = static_cast<std::uint8_t>(v >> 8);
                    break;
                }
                default: return;
            }
        }
    }
}

}  // namespace lvglpp::platform::lnx
