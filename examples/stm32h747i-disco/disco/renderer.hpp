// PARITY: rlvgl/platform/src/stm32h747i_disco.rs (display renderer
//         path: DMA2D fills + bitmap-font text into the SDRAM
//         framebuffer); family doc 00 §10 names this surface
//         "DiscoRenderer".
// LVGL:   lvgl/src/draw — informative only; CORE-04 owns the
//         interface.
// DELTA:  Cost-split fill (CPU below a threshold, DMA2D above) is
//         local; rlvgl routes per-call via Blitter caps instead.
//
// PLAT-02e-3b — core::Renderer backend for the disco framebuffer.
// See docs/platform-disco/05-dma2d-engine.md §15 (2026-06-10).

#pragma once

#include <cstdint>
#include <string_view>

#include "framebuffer.hpp"
#include "lvglpp/core/renderer.hpp"

namespace lvglpp::disco {

class DiscoRenderer final : public core::Renderer {
public:
    // Args:
    //   fb: borrows for the renderer's lifetime; the caller must not
    //       move the FrameBuffer (e.g. into a DMA2D InFlight token)
    //       while a draw() traversal is running.
    explicit DiscoRenderer(FrameBuffer& fb) noexcept : fb_{&fb} {}

    // Clipped opaque fill. Rects with area >= DMA2D_MIN_AREA go to
    // the DMA2D engine; smaller ones (glyph fragments — BitmapFont
    // emits scale×scale rects per set bit) are CPU-written, where
    // per-transfer setup would dominate.
    void fill_rect(core::Rect rect, core::Color color) override;

    // Built-in FONT_6X10 (rlvgl bring-up parity font), top-left
    // anchored at (x, y) per BitmapFont::draw_str.
    void draw_text(std::int32_t x,
                   std::int32_t y,
                   std::string_view text,
                   core::Color color) override;

private:
    static constexpr std::uint32_t DMA2D_MIN_AREA = 64;

    FrameBuffer* fb_;  // borrows: see constructor contract.
};

} // namespace lvglpp::disco
