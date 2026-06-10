// PARITY: see renderer.hpp.
// LVGL:   N/A.
// DELTA:  see renderer.hpp.
//
// PLAT-02e-3b — DiscoRenderer implementation.

#include "renderer.hpp"

#include "dma2d.hpp"
#include "lvglpp/core/fonts/font_6x10.hpp"

namespace lvglpp::disco {

namespace {

[[nodiscard]] constexpr std::uint32_t to_argb(core::Color c) noexcept {
    return (static_cast<std::uint32_t>(c.a) << 24)
         | (static_cast<std::uint32_t>(c.r) << 16)
         | (static_cast<std::uint32_t>(c.g) << 8)
         |  static_cast<std::uint32_t>(c.b);
}

} // namespace

void DiscoRenderer::fill_rect(core::Rect rect, core::Color color) {
    // Clip against the framebuffer; widgets may legitimately emit
    // partially off-screen rects (e.g. the Label baseline quirk).
    std::int32_t x0 = rect.x;
    std::int32_t y0 = rect.y;
    std::int32_t x1 = rect.x + rect.width;
    std::int32_t y1 = rect.y + rect.height;
    const auto fbw = static_cast<std::int32_t>(fb_->width());
    const auto fbh = static_cast<std::int32_t>(fb_->height());
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > fbw) x1 = fbw;
    if (y1 > fbh) y1 = fbh;
    if (x0 >= x1 || y0 >= y1) return;

    const auto w = static_cast<std::uint32_t>(x1 - x0);
    const auto h = static_cast<std::uint32_t>(y1 - y0);
    const std::uint32_t argb   = to_argb(color);
    const std::uint32_t stride = fb_->stride_px();
    volatile std::uint32_t* dst =
        fb_->pixels() + static_cast<std::uint32_t>(y0) * stride
                      + static_cast<std::uint32_t>(x0);

    if (w * h >= DMA2D_MIN_AREA) {
        (void)dma2d::fill(dst, stride, w, h, argb);
        return;
    }
    for (std::uint32_t row = 0; row < h; ++row) {
        for (std::uint32_t col = 0; col < w; ++col) {
            dst[row * stride + col] = argb;
        }
    }
}

void DiscoRenderer::draw_text(std::int32_t x,
                              std::int32_t y,
                              std::string_view text,
                              core::Color color) {
    core::fonts::FONT_6X10.draw_str(*this, x, y, text, color);
}

} // namespace lvglpp::disco
