// PARITY: rlvgl/platform/src/display_init.rs L79-98 (LTDC + Layer-1
//         register addresses) + hwcore/regs/ltdc.rs. RM0399 §33.5.
// LVGL:   N/A.
// DELTA:  Layer-1 modelled as a sibling MmioAddr block at LTDC+0x084
//         (rlvgl's LTDC_L1 base), not embedded in the global struct.
//
// PLAT-02d §4/§9 — LTDC typed register block at 0x5000_1000. The
// global timing/control registers plus the Layer-1 sub-block scanned
// to the DSI in adapted command mode. Offsets verified against
// RM0399 §33.5 and the display_init.rs addresses.
//
// `mmio: owned by RM0399 §33.5; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

// LTDC global registers (RM0399 §33.5). Reserved gaps reproduce the
// documented layout so each named field lands on its exact offset.
struct alignas(4) Ltdc {
    std::uint8_t           _rsvd_000[0x008 - 0x000];
    volatile std::uint32_t sscr;    // 0x008 §33.5.1 — sync size config
    volatile std::uint32_t bpcr;    // 0x00C §33.5.2 — back-porch config
    volatile std::uint32_t awcr;    // 0x010 §33.5.3 — active-width config
    volatile std::uint32_t twcr;    // 0x014 §33.5.4 — total-width config
    volatile std::uint32_t gcr;     // 0x018 §33.5.5 — global control (LTDCEN)
    std::uint8_t           _rsvd_01C[0x024 - 0x01C];
    volatile std::uint32_t srcr;    // 0x024 §33.5.6 — shadow reload control
    std::uint8_t           _rsvd_028[0x02C - 0x028];
    volatile std::uint32_t bccr;    // 0x02C §33.5.8 — background color
    std::uint8_t           _rsvd_030[0x034 - 0x030];
    volatile std::uint32_t ier;     // 0x034 §33.5.10 — interrupt enable
    volatile std::uint32_t isr;     // 0x038 §33.5.11 — interrupt status
    volatile std::uint32_t icr;     // 0x03C §33.5.12 — interrupt clear
    volatile std::uint32_t lipcr;   // 0x040 §33.5.13 — line interrupt pos
    volatile std::uint32_t cpsr;    // 0x044 §33.5.14 — current position
    volatile std::uint32_t cdsr;    // 0x048 §33.5.15 — current display status
};

static_assert(offsetof(Ltdc, sscr)  == 0x008, "RM0399 §33.5.1");
static_assert(offsetof(Ltdc, bpcr)  == 0x00C, "RM0399 §33.5.2");
static_assert(offsetof(Ltdc, awcr)  == 0x010, "RM0399 §33.5.3");
static_assert(offsetof(Ltdc, twcr)  == 0x014, "RM0399 §33.5.4");
static_assert(offsetof(Ltdc, gcr)   == 0x018, "RM0399 §33.5.5");
static_assert(offsetof(Ltdc, srcr)  == 0x024, "RM0399 §33.5.6");
static_assert(offsetof(Ltdc, bccr)  == 0x02C, "RM0399 §33.5.8");
static_assert(offsetof(Ltdc, ier)   == 0x034, "RM0399 §33.5.10");
static_assert(offsetof(Ltdc, isr)   == 0x038, "RM0399 §33.5.11");
static_assert(offsetof(Ltdc, icr)   == 0x03C, "RM0399 §33.5.12");
static_assert(offsetof(Ltdc, lipcr) == 0x040, "RM0399 §33.5.13");
static_assert(offsetof(Ltdc, cpsr)  == 0x044, "RM0399 §33.5.14");
static_assert(offsetof(Ltdc, cdsr)  == 0x048, "RM0399 §33.5.15");

// LTDC Layer-1 register sub-block (RM0399 §33.6). Offsets are relative
// to LTDC + 0x084 (the layer-1 base), matching display_init.rs LTDC_L1.
struct alignas(4) LtdcLayer {
    volatile std::uint32_t cr;      // 0x00 §33.6.1  — layer control (LEN)
    volatile std::uint32_t whpcr;   // 0x04 §33.6.2  — window horiz position
    volatile std::uint32_t wvpcr;   // 0x08 §33.6.3  — window vert position
    volatile std::uint32_t ckcr;    // 0x0C §33.6.4  — color keying
    volatile std::uint32_t pfcr;    // 0x10 §33.6.5  — pixel format (0=ARGB8888)
    volatile std::uint32_t cacr;    // 0x14 §33.6.6  — constant alpha
    volatile std::uint32_t dccr;    // 0x18 §33.6.7  — default color
    volatile std::uint32_t bfcr;    // 0x1C §33.6.8  — blending factors
    std::uint8_t           _rsvd_20[0x28 - 0x20];
    volatile std::uint32_t cfbar;   // 0x28 §33.6.11 — framebuffer address
    volatile std::uint32_t cfblr;   // 0x2C §33.6.12 — framebuffer line length
    volatile std::uint32_t cfblnr;  // 0x30 §33.6.13 — framebuffer line count
};

static_assert(offsetof(LtdcLayer, cr)     == 0x00, "RM0399 §33.6.1");
static_assert(offsetof(LtdcLayer, whpcr)  == 0x04, "RM0399 §33.6.2");
static_assert(offsetof(LtdcLayer, wvpcr)  == 0x08, "RM0399 §33.6.3");
static_assert(offsetof(LtdcLayer, pfcr)   == 0x10, "RM0399 §33.6.5");
static_assert(offsetof(LtdcLayer, cacr)   == 0x14, "RM0399 §33.6.6");
static_assert(offsetof(LtdcLayer, dccr)   == 0x18, "RM0399 §33.6.7");
static_assert(offsetof(LtdcLayer, bfcr)   == 0x1C, "RM0399 §33.6.8");
static_assert(offsetof(LtdcLayer, cfbar)  == 0x28, "RM0399 §33.6.11");
static_assert(offsetof(LtdcLayer, cfblr)  == 0x2C, "RM0399 §33.6.12");
static_assert(offsetof(LtdcLayer, cfblnr) == 0x30, "RM0399 §33.6.13");

// `mmio: owned by RM0399 §33.5/§33.6; never freed.`
inline constexpr std::uintptr_t       LTDC_BASE = 0x5000'1000u;
inline constexpr MmioAddr<Ltdc>       LTDC{LTDC_BASE};
inline constexpr MmioAddr<LtdcLayer>  LTDC_L1{LTDC_BASE + 0x084u};

namespace gcr {
    // §33.5.5 — global control. LTDCEN @ bit 0. Reset value 0x0000_2220
    // (HSPOL/VSPOL/DEPOL/PCPOL defaults); display_init.rs writes
    // 0x0000_2221 = reset + LTDCEN.
    inline constexpr std::uint32_t LTDCEN       = 1u << 0;
    inline constexpr std::uint32_t ENABLE_VALUE = 0x0000'2221u;
}

namespace srcr {
    // §33.5.6 — IMR (immediate reload) @ bit 0, VBR @ bit 1.
    inline constexpr std::uint32_t IMR = 1u << 0;
}

namespace l1cr {
    // §33.6.1 — LEN (layer enable) @ bit 0.
    inline constexpr std::uint32_t LEN = 1u << 0;
}

namespace l1pfcr {
    // §33.6.5 — pixel format. 0b000 = ARGB8888.
    inline constexpr std::uint32_t ARGB8888 = 0u;
}

} // namespace lvglpp::disco::regs
