// PARITY: rlvgl/platform/src/dma2d.rs (Dma2dBlitter: new / fill_raw /
//         blit_raw / poll-complete surface, v0.2.0 @ 79f730d).
// LVGL:   lv_draw_sw + DMA2D draw-unit precedent (lvgl/src/draw/);
//         not consumed here — this is the bring-up engine the future
//         Renderer integration (PLAT-02e-3) wraps.
// DELTA:  Free functions over a singleton MMIO block instead of a
//         peripheral-moving struct (no PAC ownership token in C++);
//         blocking-poll only — InFlight<T> token + ISR latch land in
//         PLAT-02e-2.
//
// PLAT-02e §5.3 — DMA2D engine: R2M fill + M2M(+PFC) blit.
// See docs/platform-disco/05-dma2d-engine.md.

#pragma once

#include <cstdint>

namespace lvglpp::disco::dma2d {

// One-time engine setup. Mirrors Dma2dBlitter::new(): clears stale
// interrupt flags (only when no transfer is in flight) and programs
// AMTCR dead time DT=8 so DMA2D bursts leave the LTDC scan AXI slots
// to read from SDRAM (PLAT-02e §5.3 / dma2d.rs:33).
//
// Pre-condition: clocks::init() has gated DMA2D on AHB3ENR +
// C1_AHB3ENR (PLAT-02e §5.1).
void init() noexcept;

// Blocking R2M fill of a width×height ARGB8888 rectangle.
//
// Args:
//   dst:    dma — first pixel of the rectangle inside a framebuffer
//           the DMA2D engine writes; caller guarantees the region is
//           not CPU-mutated until this call returns (trivially true:
//           the call blocks). The live LTDC scan reading the same
//           SDRAM concurrently is in-spec (PLAT-02e §10).
//   stride_px: destination row pitch in PIXELS (framebuffer width).
//   width, height: rectangle size in pixels; width <= stride_px.
//   argb:   fill color, ARGB8888.
// Returns:
//   true on TCIF completion; false if the engine reported
//   TEIF/CEIF or the bounded poll timed out.
bool fill(volatile std::uint32_t* dst,
          std::uint32_t stride_px,
          std::uint32_t width,
          std::uint32_t height,
          std::uint32_t argb) noexcept;

// Blocking M2M copy of a width×height ARGB8888 rectangle.
//
// Args:
//   src:    dma — first source pixel; borrows for the duration of
//           the call; must not alias dst's rectangle.
//   src_stride_px, dst_stride_px: row pitches in PIXELS.
//   dst:    dma — first destination pixel (same contract as fill()).
// Returns:
//   true on TCIF completion; false on TEIF/CEIF or poll timeout.
bool blit(const volatile std::uint32_t* src,
          std::uint32_t src_stride_px,
          volatile std::uint32_t* dst,
          std::uint32_t dst_stride_px,
          std::uint32_t width,
          std::uint32_t height) noexcept;

// Last observed ISR error bits (TEIF|CEIF) from the most recent
// fill/blit, for D3-SRAM relay diagnosis. 0 = clean.
[[nodiscard]] std::uint32_t last_error() noexcept;

} // namespace lvglpp::disco::dma2d
