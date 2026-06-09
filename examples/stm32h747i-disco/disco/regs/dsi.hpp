// PARITY: rlvgl/platform/src/display_init.rs L28-75 (DSI host @ 0x5000_0000
//         + wrapper @ +0x400) + hwcore/regs/dsi.rs. RM0399 §34.16.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02d §3/§4/§9 — DSI host + wrapper typed register block.
//
// ADDRESS-ALIAS HAZARD (display_init.rs L52-56): DSI_LCCR lives at
// 0x064, NOT 0x02C (which is DSI_PCR). The 0x02C alias once corrupted
// PCR and left CMDSIZE=0, producing snow on the panel. The named
// fields + offsetof asserts below make that wrong offset a
// compile-time failure rather than a runtime hazard.
//
// `mmio: owned by RM0399 §34.16; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

// DSI host registers (RM0399 §34.16.x), base 0x5000_0000. Reserved
// gaps reproduce the documented layout so each named field lands on
// its exact offset — see the LCCR@0x064 hazard note above.
struct alignas(4) Dsi {
    volatile std::uint32_t vr;      // 0x000 — version
    volatile std::uint32_t cr;      // 0x004 — control (EN)
    volatile std::uint32_t ccr;     // 0x008 — clock control (TXECKDIV)
    volatile std::uint32_t lvcidr;  // 0x00C — virtual channel ID
    volatile std::uint32_t lcolcr;  // 0x010 — color coding (LPE/COLC)
    volatile std::uint32_t lpcr;    // 0x014 — LTDC polarity
    volatile std::uint32_t lpmcr;   // 0x018 — low-power mode config
    std::uint8_t           _rsvd_01C[0x02C - 0x01C];
    volatile std::uint32_t pcr;     // 0x02C — protocol config (flow ctrl)
    volatile std::uint32_t gvcidr;  // 0x030 — generic VCID
    volatile std::uint32_t mcr;     // 0x034 — mode config (CMDM)
    volatile std::uint32_t vmcr;    // 0x038 — video mode config
    volatile std::uint32_t vpcr;    // 0x03C — video packet config
    volatile std::uint32_t vccr;    // 0x040 — video chunks config
    volatile std::uint32_t vnpcr;   // 0x044 — video null packet config
    volatile std::uint32_t vhsacr;  // 0x048 — video HSA config
    volatile std::uint32_t vhbpcr;  // 0x04C — video HBP config
    volatile std::uint32_t vlcr;    // 0x050 — video line config
    volatile std::uint32_t vvsacr;  // 0x054 — video VSA config
    volatile std::uint32_t vvbpcr;  // 0x058 — video VBP config
    volatile std::uint32_t vvfpcr;  // 0x05C — video VFP config
    volatile std::uint32_t vvacr;   // 0x060 — video VA config
    volatile std::uint32_t lccr;    // 0x064 — command config (CMDSIZE) !alias
    volatile std::uint32_t cmcr;    // 0x068 — command mode config
    volatile std::uint32_t ghcr;    // 0x06C — generic header (DCS/generic TX)
    volatile std::uint32_t gpdr;    // 0x070 — generic payload data
    volatile std::uint32_t gpsr;    // 0x074 — generic packet status (CMDFE)
    volatile std::uint32_t tccr0;   // 0x078 — timeout counter config 0
    std::uint8_t           _rsvd_07C[0x094 - 0x07C];
    volatile std::uint32_t clcr;    // 0x094 — clock lane control (DPCC/ACR)
    volatile std::uint32_t cltcr;   // 0x098 — clock lane timer config
    volatile std::uint32_t dltcr;   // 0x09C — data lane timer config
    volatile std::uint32_t pctlr;   // 0x0A0 — PHY control (DEN/CKE)
    volatile std::uint32_t pconfr;  // 0x0A4 — PHY config (NL/SW_TIME)
    std::uint8_t           _rsvd_0A8[0x0B0 - 0x0A8];
    volatile std::uint32_t psr;     // 0x0B0 — PHY status (PSS0/PSS1/PSSC)
};

static_assert(offsetof(Dsi, vr)     == 0x000, "RM0399 §34.16");
static_assert(offsetof(Dsi, cr)     == 0x004, "RM0399 §34.16");
static_assert(offsetof(Dsi, ccr)    == 0x008, "RM0399 §34.16");
static_assert(offsetof(Dsi, lvcidr) == 0x00C, "RM0399 §34.16");
static_assert(offsetof(Dsi, lcolcr) == 0x010, "RM0399 §34.16");
static_assert(offsetof(Dsi, lpcr)   == 0x014, "RM0399 §34.16");
static_assert(offsetof(Dsi, lpmcr)  == 0x018, "RM0399 §34.16");
static_assert(offsetof(Dsi, pcr)    == 0x02C, "RM0399 §34.16 (NOT LCCR)");
static_assert(offsetof(Dsi, gvcidr) == 0x030, "RM0399 §34.16");
static_assert(offsetof(Dsi, mcr)    == 0x034, "RM0399 §34.16");
static_assert(offsetof(Dsi, vmcr)   == 0x038, "RM0399 §34.16");
static_assert(offsetof(Dsi, vpcr)   == 0x03C, "RM0399 §34.16");
static_assert(offsetof(Dsi, vccr)   == 0x040, "RM0399 §34.16");
static_assert(offsetof(Dsi, vnpcr)  == 0x044, "RM0399 §34.16");
static_assert(offsetof(Dsi, vhsacr) == 0x048, "RM0399 §34.16");
static_assert(offsetof(Dsi, vhbpcr) == 0x04C, "RM0399 §34.16");
static_assert(offsetof(Dsi, vlcr)   == 0x050, "RM0399 §34.16");
static_assert(offsetof(Dsi, vvsacr) == 0x054, "RM0399 §34.16");
static_assert(offsetof(Dsi, vvbpcr) == 0x058, "RM0399 §34.16");
static_assert(offsetof(Dsi, vvfpcr) == 0x05C, "RM0399 §34.16");
static_assert(offsetof(Dsi, vvacr)  == 0x060, "RM0399 §34.16");
// The load-bearing assertion: LCCR is at 0x064, NOT 0x02C.
static_assert(offsetof(Dsi, lccr)   == 0x064, "RM0399 §34.16 (LCCR@0x64 not 0x2C)");
static_assert(offsetof(Dsi, cmcr)   == 0x068, "RM0399 §34.16");
static_assert(offsetof(Dsi, ghcr)   == 0x06C, "RM0399 §34.16");
static_assert(offsetof(Dsi, gpdr)   == 0x070, "RM0399 §34.16");
static_assert(offsetof(Dsi, gpsr)   == 0x074, "RM0399 §34.16");
static_assert(offsetof(Dsi, tccr0)  == 0x078, "RM0399 §34.16");
static_assert(offsetof(Dsi, clcr)   == 0x094, "RM0399 §34.16");
static_assert(offsetof(Dsi, cltcr)  == 0x098, "RM0399 §34.16");
static_assert(offsetof(Dsi, dltcr)  == 0x09C, "RM0399 §34.16");
static_assert(offsetof(Dsi, pctlr)  == 0x0A0, "RM0399 §34.16");
static_assert(offsetof(Dsi, pconfr) == 0x0A4, "RM0399 §34.16");
static_assert(offsetof(Dsi, psr)    == 0x0B0, "RM0399 §34.16");

// DSI wrapper registers (RM0399 §34.16.x), base = DSI + 0x400.
struct alignas(4) DsiWrapper {
    volatile std::uint32_t wcfgr;   // 0x00 — wrapper config (DSIM/COLMUX/TESRC)
    volatile std::uint32_t wcr;     // 0x04 — wrapper control (DSIEN/LTDCEN)
    volatile std::uint32_t wier;    // 0x08 — wrapper interrupt enable
    volatile std::uint32_t wisr;    // 0x0C — wrapper interrupt status (RRS/PLLLS/ERIF)
    volatile std::uint32_t wifcr;   // 0x10 — wrapper interrupt flag clear
    std::uint8_t           _rsvd_14[0x18 - 0x14];
    volatile std::uint32_t wpcr0;   // 0x18 — wrapper PHY config 0 (UIX4)
    std::uint8_t           _rsvd_1C[0x30 - 0x1C];
    volatile std::uint32_t wrpcr;   // 0x30 — wrapper regulator/PLL config
};

static_assert(offsetof(DsiWrapper, wcfgr) == 0x00, "RM0399 §34.16");
static_assert(offsetof(DsiWrapper, wcr)   == 0x04, "RM0399 §34.16");
static_assert(offsetof(DsiWrapper, wier)  == 0x08, "RM0399 §34.16");
static_assert(offsetof(DsiWrapper, wisr)  == 0x0C, "RM0399 §34.16");
static_assert(offsetof(DsiWrapper, wifcr) == 0x10, "RM0399 §34.16");
static_assert(offsetof(DsiWrapper, wpcr0) == 0x18, "RM0399 §34.16");
static_assert(offsetof(DsiWrapper, wrpcr) == 0x30, "RM0399 §34.16");

// `mmio: owned by RM0399 §34.16; never freed.`
inline constexpr std::uintptr_t          DSI_BASE = 0x5000'0000u;
inline constexpr MmioAddr<Dsi>           DSI{DSI_BASE};
inline constexpr MmioAddr<DsiWrapper>    DSI_W{DSI_BASE + 0x400u};

// --- Bit positions used by display::init() (mirror display_init.rs) ---

namespace cr {
    inline constexpr std::uint32_t EN = 1u << 0;     // host enable
}

namespace wrpcr {
    inline constexpr std::uint32_t REGEN = 1u << 24; // regulator enable
    inline constexpr std::uint32_t PLLEN = 1u << 0;  // PLL enable
    // IDF[14:11], NDIV[8:2], ODF[17:16] (PAC field layout).
    constexpr std::uint32_t pll(std::uint32_t idf,
                                std::uint32_t ndiv,
                                std::uint32_t odf) {
        return REGEN | PLLEN
             | ((idf  & 0x0Fu) << 11)
             | ((ndiv & 0x7Fu) <<  2)
             | ((odf  & 0x03u) << 16);
    }
}

namespace wisr {
    inline constexpr std::uint32_t PLLLS = 1u << 8;  // PLL lock status
    inline constexpr std::uint32_t RRS   = 1u << 12; // regulator ready status
    inline constexpr std::uint32_t ERIF  = 1u << 1;  // end-of-refresh flag
}

namespace psr {
    // 2-lane stop state: PSS0(bit1) + PSS1(bit2) + PSSC(bit3) = 0x0E.
    inline constexpr std::uint32_t LANES_STOPPED = 0x0Eu;
}

namespace gpsr {
    inline constexpr std::uint32_t CMDFE = 1u << 0;  // command FIFO empty
}

namespace wcr {
    inline constexpr std::uint32_t DSIEN  = 1u << 3; // wrapper DSI enable
    inline constexpr std::uint32_t LTDCEN = 1u << 2; // wrapper LTDC enable (pulse)
}

} // namespace lvglpp::disco::regs
