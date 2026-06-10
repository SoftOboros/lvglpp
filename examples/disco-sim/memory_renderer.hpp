// memory_renderer.hpp — offscreen ARGB8888 renderer for headless modes.
//
// PARITY: rlvgl-disco-sim's FrameMirror role (the CPU-visible frame
//         the ASCII dump reads). Text via core FONT_6X10 — the same
//         font SdlRenderer uses, so headless content matches the
//         windowed output.
// LVGL:   N/A.
// DELTA:  none beyond living in the example until a second consumer
//         appears (DEMO-07 §5.3).

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "lvglpp/core/fonts/font_6x10.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::sim {

class MemoryRenderer final : public core::Renderer {
public:
    MemoryRenderer(std::uint32_t width, std::uint32_t height)
        : w_{width}, h_{height},
          // owns: host frame storage, ARGB8888 words.
          buf_(static_cast<std::size_t>(width) * height, 0u) {}

    void fill_rect(core::Rect rect, core::Color color) override {
        std::int32_t x0 = rect.x, y0 = rect.y;
        std::int32_t x1 = rect.x + rect.width, y1 = rect.y + rect.height;
        const auto w = static_cast<std::int32_t>(w_);
        const auto h = static_cast<std::int32_t>(h_);
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > w) x1 = w;
        if (y1 > h) y1 = h;
        const std::uint32_t argb =
            (static_cast<std::uint32_t>(color.a) << 24)
          | (static_cast<std::uint32_t>(color.r) << 16)
          | (static_cast<std::uint32_t>(color.g) << 8)
          |  static_cast<std::uint32_t>(color.b);
        for (std::int32_t y = y0; y < y1; ++y) {
            for (std::int32_t x = x0; x < x1; ++x) {
                buf_[static_cast<std::size_t>(y) * w_ +
                     static_cast<std::size_t>(x)] = argb;
            }
        }
    }

    void draw_text(std::int32_t x, std::int32_t y,
                   std::string_view text, core::Color color) override {
        core::fonts::FONT_6X10.draw_str(*this, x, y, text, color);
    }

    // ASCII frame per DEMO-07 §5.2 — byte-for-byte the rlvgl
    // dump_ascii_frame luminance mapping.
    [[nodiscard]] std::string ascii_frame() const {
        std::string out;
        out.reserve(static_cast<std::size_t>(w_ + 1) * h_);
        for (std::uint32_t y = 0; y < h_; ++y) {
            for (std::uint32_t x = 0; x < w_; ++x) {
                const std::uint32_t px =
                    buf_[static_cast<std::size_t>(y) * w_ + x];
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

    // Pixel inspection for the PLAYIT-07a FramebufferReader adapter.
    [[nodiscard]] std::uint32_t width() const noexcept { return w_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return h_; }
    [[nodiscard]] std::uint32_t pixel(std::uint32_t x,
                                      std::uint32_t y) const noexcept {
        if (x >= w_ || y >= h_) return 0;
        return buf_[static_cast<std::size_t>(y) * w_ + x];
    }

private:
    std::uint32_t w_;
    std::uint32_t h_;
    std::vector<std::uint32_t> buf_;  // owns: see constructor.
};

}  // namespace lvglpp::sim
