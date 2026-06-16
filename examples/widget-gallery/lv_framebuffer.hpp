// lv_framebuffer.hpp — headless lv_display rendering to an in-memory
// XRGB8888 framebuffer (LVGLPP-WRAP-0N example migration).
//
// PARITY: replaces the hand-rolled core::Renderer software rasterizer
//         (disco-sim/memory_renderer.hpp) for the lv_obj widget path.
// LVGL:   lvgl/src/display/lv_display.h (lv_display_create / set_buffers /
//         flush_cb), lvgl/src/core/lv_refr.h (lv_refr_now).
// DELTA:  DIRECT render mode: the draw buffer IS the framebuffer, so after
//         render() the framebuffer already holds the rendered pixels (the
//         flush callback just acks — no copy). LV_COLOR_DEPTH is 32, so each
//         pixel is an XRGB8888 uint32 (0x00RRGGBB) the playit D dump and the
//         ASCII capture read directly.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::gallery {

// Owns an lv_display rendering into a private XRGB8888 framebuffer.
class LvFramebuffer {
public:
    LvFramebuffer(std::uint32_t width, std::uint32_t height)
        : w_{width},
          h_{height},
          buf_(static_cast<std::size_t>(width) * height, 0u) {
        disp_ = lv_display_create(static_cast<std::int32_t>(width),
                                  static_cast<std::int32_t>(height));
        lv_display_set_flush_cb(disp_, &LvFramebuffer::flush_cb);
        // DIRECT mode: buf_ is the persistent full-screen framebuffer.
        lv_display_set_buffers(
            disp_, buf_.data(), nullptr,
            static_cast<std::uint32_t>(buf_.size() * sizeof(std::uint32_t)),
            LV_DISPLAY_RENDER_MODE_DIRECT);
    }

    ~LvFramebuffer() {
        if (disp_ != nullptr) {
            lv_display_delete(disp_);
        }
    }

    LvFramebuffer(const LvFramebuffer&)            = delete;
    LvFramebuffer& operator=(const LvFramebuffer&) = delete;

    [[nodiscard]] lv_display_t* display() const noexcept { return disp_; }

    // Render the active screen into the framebuffer (lv_refr_now).
    void render() noexcept { lv_refr_now(disp_); }

    [[nodiscard]] std::uint32_t width()  const noexcept { return w_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return h_; }

    [[nodiscard]] std::uint32_t pixel(std::uint32_t x, std::uint32_t y) const noexcept {
        if (x >= w_ || y >= h_) {
            return 0u;
        }
        return buf_[static_cast<std::size_t>(y) * w_ + x];
    }

    // Luminance ASCII capture (same mapping as the legacy MemoryRenderer).
    [[nodiscard]] std::string ascii_frame() const {
        std::string out;
        out.reserve(static_cast<std::size_t>(w_ + 1) * h_);
        for (std::uint32_t y = 0; y < h_; ++y) {
            for (std::uint32_t x = 0; x < w_; ++x) {
                const std::uint32_t px = buf_[static_cast<std::size_t>(y) * w_ + x];
                const std::uint32_t r = (px >> 16) & 0xFFu;
                const std::uint32_t g = (px >> 8) & 0xFFu;
                const std::uint32_t b = px & 0xFFu;
                const std::uint32_t val = (r + g + b) / 3u;
                char ch = '@';
                if (val == 0) ch = ' ';
                else if (val <= 63) ch = '.';
                else if (val <= 127) ch = ':';
                else if (val <= 191) ch = '*';
                else if (val <= 223) ch = '#';
                out.push_back(ch);
            }
            out.push_back('\n');
        }
        return out;
    }

private:
    static void flush_cb(lv_display_t* disp, const lv_area_t* /*area*/,
                         std::uint8_t* /*px_map*/) noexcept {
        // DIRECT mode: nothing to copy — buf_ is already the framebuffer.
        lv_display_flush_ready(disp);
    }

    std::uint32_t              w_;
    std::uint32_t              h_;
    std::vector<std::uint32_t> buf_;  // owns: XRGB8888 framebuffer + draw buffer.
    lv_display_t*              disp_ = nullptr;  // owns.
};

}  // namespace lvglpp::gallery
