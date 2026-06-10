// PARITY: rlvgl/platform/src/dma2d.rs (new / fill_engine_raw /
//         blit_engine_raw / wait, v0.2.0 @ 79f730d).
// LVGL:   N/A (see dma2d.hpp).
// DELTA:  Bounded poll instead of unbounded wait(); error bits are
//         latched for D3-SRAM relay instead of returned via trait.
//
// PLAT-02e §5.3 — DMA2D engine implementation.

#include "dma2d.hpp"

#include "regs/dma2d.hpp"

namespace lvglpp::disco::dma2d {

namespace {

// observes: ISR error bits of the most recent transfer; written only
// by wait_done(), read by last_error(). Single-context (no ISR yet —
// PLAT-02e-2), so a plain word suffices.
std::uint32_t g_last_error = 0;

// Bounded busy-wait on CR.START. A full-screen 480×800 ARGB8888 R2M
// fill is ~384k AXI words; 100M iterations at 400 MHz (~0.25 s+) is
// orders of magnitude past any legitimate transfer, matching the
// bounded-poll posture of clocks.cpp (never hang the boot path).
bool wait_done() noexcept {
    using namespace regs;
    auto& d = DMA2D.ref();
    bool stopped = false;
    for (std::uint32_t i = 0; i < 100'000'000u; ++i) {
        if ((d.cr & dma2d_cr::START) == 0u) { stopped = true; break; }
    }
    const std::uint32_t isr = d.isr;
    g_last_error = isr & dma2d_isr::ERROR_MASK;
    const bool ok = stopped
                 && g_last_error == 0u
                 && (isr & dma2d_isr::TCIF) != 0u;
    d.ifcr = dma2d_isr::ALL;   // mirrors dma2d.rs wait() flag clear
    return ok;
}

// Mirrors dma2d.rs write_cr_mode + START modify: preserve the
// interrupt-enable bits, set mode, then kick START.
void kick(std::uint32_t mode) noexcept {
    using namespace regs;
    auto& d = DMA2D.ref();
    d.ifcr = dma2d_isr::ALL;   // prepare_start()
    const std::uint32_t irq = d.cr & dma2d_cr::IRQ_MASK;
    d.cr = mode | irq;
    d.cr = d.cr | dma2d_cr::START;
}

// SAFETY:
//   Pointer→integer cast for DMA address programming. The pointee is
//   a dma-marked framebuffer region in FMC SDRAM (0xD000_0000 bank);
//   the integer is consumed only by the DMA2D address registers, so
//   no CPU-side pointer is ever re-materialized from it.
std::uint32_t dma_addr(const volatile void* p) noexcept {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));
}

} // namespace

void init() noexcept {
    using namespace regs;
    auto& d = DMA2D.ref();
    // Preserve any in-flight transfer (dma2d.rs Dma2dBlitter::new).
    if ((d.cr & dma2d_cr::START) == 0u) {
        d.ifcr = dma2d_isr::ALL;
    }
    // AMTCR: DT=8 dead cycles between AXI bursts, EN=1 — keeps the
    // engine from starving the live LTDC scan (PLAT-02e §5.3).
    d.amtcr = (8u << 8) | 1u;
}

bool fill(volatile std::uint32_t* dst,
          std::uint32_t stride_px,
          std::uint32_t width,
          std::uint32_t height,
          std::uint32_t argb) noexcept {
    using namespace regs;
    auto& d = DMA2D.ref();
    d.omar  = dma_addr(dst);
    d.ocolr = argb;
    // OPFCCR left at ARGB8888; set explicitly in case a prior blit
    // changed it (dma2d.rs elides this only because its fill assumes
    // the reset value).
    d.opfccr = dma2d_pfccr::ARGB8888;
    d.oor    = stride_px - width;            // line offset in pixels
    d.nlr    = (width << 16) | height;       // PL[29:16] | NL[15:0]
    kick(dma2d_cr::MODE_R2M);
    return wait_done();
}

bool blit(const volatile std::uint32_t* src,
          std::uint32_t src_stride_px,
          volatile std::uint32_t* dst,
          std::uint32_t dst_stride_px,
          std::uint32_t width,
          std::uint32_t height) noexcept {
    using namespace regs;
    auto& d = DMA2D.ref();
    d.fgmar   = dma_addr(src);
    d.fgor    = src_stride_px - width;
    d.fgpfccr = dma2d_pfccr::ARGB8888;
    d.omar    = dma_addr(dst);
    d.oor     = dst_stride_px - width;
    d.opfccr  = dma2d_pfccr::ARGB8888;
    d.nlr     = (width << 16) | height;
    kick(dma2d_cr::MODE_M2M_PFC);
    return wait_done();
}

std::uint32_t last_error() noexcept {
    return g_last_error;
}

} // namespace lvglpp::disco::dma2d
