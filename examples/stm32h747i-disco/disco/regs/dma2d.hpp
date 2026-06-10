// PARITY: rlvgl/platform/src/dma2d.rs (Dma2dBlitter register use,
//         v0.2.0 @ 79f730d) — addresses via the svd2rust PAC.
// LVGL:   N/A — bring-up plumbing below the LVGL surface.
// DELTA:  none (offsets static_asserted against RM0399; the ASCII
//         diagram in rlvgl Vol II Ch 7 has two offset typos that the
//         rlvgl *code* does not share — see PLAT-02e §0).
//
// PLAT-02e §5.2 — DMA2D (Chrom-Art) typed register block at
// 0x5200_1000. Offsets per RM0399 DMA2D register map.
//
// `mmio: owned by RM0399 DMA2D chapter; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) Dma2d {
    volatile std::uint32_t cr;       // 0x00 — control (MODE, START, IRQ enables)
    volatile std::uint32_t isr;      // 0x04 — interrupt status
    volatile std::uint32_t ifcr;     // 0x08 — interrupt flag clear (w1c)
    volatile std::uint32_t fgmar;    // 0x0C — foreground memory address
    volatile std::uint32_t fgor;     // 0x10 — foreground line offset (pixels)
    volatile std::uint32_t bgmar;    // 0x14 — background memory address
    volatile std::uint32_t bgor;     // 0x18 — background line offset (pixels)
    volatile std::uint32_t fgpfccr;  // 0x1C — FG pixel-format converter
    volatile std::uint32_t fgcolr;   // 0x20 — FG color (A8/A4 modes)
    volatile std::uint32_t bgpfccr;  // 0x24 — BG pixel-format converter
    volatile std::uint32_t bgcolr;   // 0x28 — BG color
    volatile std::uint32_t fgcmar;   // 0x2C — FG CLUT memory address
    volatile std::uint32_t bgcmar;   // 0x30 — BG CLUT memory address
    volatile std::uint32_t opfccr;   // 0x34 — output pixel format (0=ARGB8888)
    volatile std::uint32_t ocolr;    // 0x38 — output color (R2M source)
    volatile std::uint32_t omar;     // 0x3C — output memory address
    volatile std::uint32_t oor;      // 0x40 — output line offset (pixels)
    volatile std::uint32_t nlr;      // 0x44 — PL[29:16] | NL[15:0]
    volatile std::uint32_t lwr;      // 0x48 — line watermark
    volatile std::uint32_t amtcr;    // 0x4C — AHB master timer (dead time)
};

static_assert(offsetof(Dma2d, cr)      == 0x00, "RM0399 DMA2D_CR");
static_assert(offsetof(Dma2d, isr)     == 0x04, "RM0399 DMA2D_ISR");
static_assert(offsetof(Dma2d, ifcr)    == 0x08, "RM0399 DMA2D_IFCR");
static_assert(offsetof(Dma2d, fgmar)   == 0x0C, "RM0399 DMA2D_FGMAR");
static_assert(offsetof(Dma2d, fgor)    == 0x10, "RM0399 DMA2D_FGOR");
static_assert(offsetof(Dma2d, bgmar)   == 0x14, "RM0399 DMA2D_BGMAR");
static_assert(offsetof(Dma2d, bgor)    == 0x18, "RM0399 DMA2D_BGOR");
static_assert(offsetof(Dma2d, fgpfccr) == 0x1C, "RM0399 DMA2D_FGPFCCR");
static_assert(offsetof(Dma2d, fgcolr)  == 0x20, "RM0399 DMA2D_FGCOLR");
static_assert(offsetof(Dma2d, bgpfccr) == 0x24, "RM0399 DMA2D_BGPFCCR");
static_assert(offsetof(Dma2d, bgcolr)  == 0x28, "RM0399 DMA2D_BGCOLR");
static_assert(offsetof(Dma2d, fgcmar)  == 0x2C, "RM0399 DMA2D_FGCMAR");
static_assert(offsetof(Dma2d, bgcmar)  == 0x30, "RM0399 DMA2D_BGCMAR");
static_assert(offsetof(Dma2d, opfccr)  == 0x34, "RM0399 DMA2D_OPFCCR");
static_assert(offsetof(Dma2d, ocolr)   == 0x38, "RM0399 DMA2D_OCOLR");
static_assert(offsetof(Dma2d, omar)    == 0x3C, "RM0399 DMA2D_OMAR");
static_assert(offsetof(Dma2d, oor)     == 0x40, "RM0399 DMA2D_OOR");
static_assert(offsetof(Dma2d, nlr)     == 0x44, "RM0399 DMA2D_NLR");
static_assert(offsetof(Dma2d, lwr)     == 0x48, "RM0399 DMA2D_LWR");
static_assert(offsetof(Dma2d, amtcr)   == 0x4C, "RM0399 DMA2D_AMTCR");

// `mmio: owned by RM0399 DMA2D chapter; never freed.`
inline constexpr std::uintptr_t   DMA2D_BASE = 0x5200'1000u;
inline constexpr MmioAddr<Dma2d>  DMA2D{DMA2D_BASE};

namespace dma2d_cr {
    // Mirrors dma2d.rs CR_* constants.
    inline constexpr std::uint32_t START          = 1u << 0;
    inline constexpr std::uint32_t TEIE           = 1u << 8;
    inline constexpr std::uint32_t TCIE           = 1u << 9;
    inline constexpr std::uint32_t IRQ_MASK       = TEIE | TCIE;
    inline constexpr std::uint32_t MODE_M2M       = 0x0000'0000u;
    inline constexpr std::uint32_t MODE_M2M_PFC   = 0x0001'0000u;
    inline constexpr std::uint32_t MODE_M2M_BLEND = 0x0002'0000u;
    inline constexpr std::uint32_t MODE_R2M       = 0x0003'0000u;
}

namespace dma2d_isr {
    // Mirrors dma2d.rs ISR_* constants. NOTE bit 0 is TEIF (transfer
    // ERROR); transfer complete is bit 1.
    inline constexpr std::uint32_t TEIF       = 1u << 0;
    inline constexpr std::uint32_t TCIF       = 1u << 1;
    inline constexpr std::uint32_t CEIF       = 1u << 5;
    inline constexpr std::uint32_t ERROR_MASK = TEIF | CEIF;
    inline constexpr std::uint32_t ALL        = 0x3Fu;  // IFCR clear-all
}

namespace dma2d_pfccr {
    // Color-mode codes shared by FG/BG/O PFC registers; values mirror
    // dma2d.rs::dma2d_fmt.
    inline constexpr std::uint32_t ARGB8888 = 0u;
    inline constexpr std::uint32_t RGB565   = 2u;
    inline constexpr std::uint32_t L8       = 5u;
    inline constexpr std::uint32_t A8       = 9u;
    inline constexpr std::uint32_t A4       = 10u;
}

} // namespace lvglpp::disco::regs
