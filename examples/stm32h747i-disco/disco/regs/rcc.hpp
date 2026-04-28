// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1569–1596 (PLL tree
//         + PLL3ON gap fix). Numerical bit positions per RM0399 §8.7.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.3 — RCC typed register block. Authoritative source for
// every offset and bit position is RM0399 §8.7 Table 38.
//
// `mmio: owned by RM0399 §8.7; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) Rcc {
    volatile std::uint32_t cr;            // 0x000 §8.7.2
    volatile std::uint32_t hsicfgr;       // 0x004 §8.7.3
    volatile std::uint32_t crrcr;         // 0x008
    volatile std::uint32_t csicfgr;       // 0x00C
    volatile std::uint32_t cfgr;          // 0x010 §8.7.5
    std::uint32_t          _rsvd_014;
    volatile std::uint32_t d1cfgr;        // 0x018 §8.7.6
    volatile std::uint32_t d2cfgr;        // 0x01C §8.7.7
    volatile std::uint32_t d3cfgr;        // 0x020 §8.7.8
    std::uint32_t          _rsvd_024;
    volatile std::uint32_t pllckselr;     // 0x028 §8.7.9
    volatile std::uint32_t pllcfgr;       // 0x02C §8.7.10
    volatile std::uint32_t pll1divr;      // 0x030 §8.7.11
    volatile std::uint32_t pll1fracr;     // 0x034
    volatile std::uint32_t pll2divr;      // 0x038
    volatile std::uint32_t pll2fracr;     // 0x03C
    volatile std::uint32_t pll3divr;      // 0x040
    volatile std::uint32_t pll3fracr;     // 0x044
    std::uint32_t          _rsvd_048;
    volatile std::uint32_t d1ccipr;       // 0x04C §8.7.13 (FMCSEL etc.)
    volatile std::uint32_t d2ccip1r;      // 0x050
    volatile std::uint32_t d2ccip2r;      // 0x054
    volatile std::uint32_t d3ccipr;       // 0x058
    std::uint32_t          _rsvd_05C;
    volatile std::uint32_t cier;          // 0x060
    volatile std::uint32_t cifr;          // 0x064
    volatile std::uint32_t cicr;          // 0x068
    std::uint32_t          _rsvd_06C;
    volatile std::uint32_t bdcr;          // 0x070
    volatile std::uint32_t csr;           // 0x074
    std::uint32_t          _rsvd_078;
    volatile std::uint32_t ahb3rstr;      // 0x07C
    volatile std::uint32_t ahb1rstr;      // 0x080
    volatile std::uint32_t ahb2rstr;      // 0x084
    volatile std::uint32_t ahb4rstr;      // 0x088
    volatile std::uint32_t apb3rstr;      // 0x08C
    volatile std::uint32_t apb1lrstr;     // 0x090
    volatile std::uint32_t apb1hrstr;     // 0x094
    volatile std::uint32_t apb2rstr;      // 0x098
    volatile std::uint32_t apb4rstr;      // 0x09C
    volatile std::uint32_t gcr;           // 0x0A0
    std::uint32_t          _rsvd_0A4;
    volatile std::uint32_t d3amr;         // 0x0A8
    std::uint32_t          _rsvd_0AC[9];
    volatile std::uint32_t rsr;           // 0x0D0
    volatile std::uint32_t ahb3enr;       // 0x0D4 §8.7.36
    volatile std::uint32_t ahb1enr;       // 0x0D8 §8.7.38
    volatile std::uint32_t ahb2enr;       // 0x0DC §8.7.39
    volatile std::uint32_t ahb4enr;       // 0x0E0 §8.7.41
    volatile std::uint32_t apb3enr;       // 0x0E4 §8.7.43
    volatile std::uint32_t apb1lenr;      // 0x0E8 §8.7.44
    volatile std::uint32_t apb1henr;      // 0x0EC §8.7.45
    volatile std::uint32_t apb2enr;       // 0x0F0 §8.7.46
    volatile std::uint32_t apb4enr;       // 0x0F4 §8.7.47
    std::uint32_t          _rsvd_0F8[(0x130 - 0x0F8) / 4];
    volatile std::uint32_t c1_rsr;        // 0x130
    volatile std::uint32_t c1_ahb3enr;    // 0x134 §8.7.x — CM7 per-core AHB3 gate
    volatile std::uint32_t c1_ahb1enr;    // 0x138
    volatile std::uint32_t c1_ahb2enr;    // 0x13C
    volatile std::uint32_t c1_ahb4enr;    // 0x140
    volatile std::uint32_t c1_apb3enr;    // 0x144
};

static_assert(offsetof(Rcc, cr)        == 0x000, "RM0399 §8.7.2");
static_assert(offsetof(Rcc, cfgr)      == 0x010, "RM0399 §8.7.5");
static_assert(offsetof(Rcc, d1cfgr)    == 0x018, "RM0399 §8.7.6");
static_assert(offsetof(Rcc, d2cfgr)    == 0x01C, "RM0399 §8.7.7");
static_assert(offsetof(Rcc, d3cfgr)    == 0x020, "RM0399 §8.7.8");
static_assert(offsetof(Rcc, pllckselr) == 0x028, "RM0399 §8.7.9");
static_assert(offsetof(Rcc, pllcfgr)   == 0x02C, "RM0399 §8.7.10");
static_assert(offsetof(Rcc, pll1divr)  == 0x030, "RM0399 §8.7.11");
static_assert(offsetof(Rcc, pll2divr)  == 0x038, "RM0399 §8.7.11");
static_assert(offsetof(Rcc, pll3divr)  == 0x040, "RM0399 §8.7.11");
static_assert(offsetof(Rcc, d1ccipr)   == 0x04C, "RM0399 §8.7.13");
static_assert(offsetof(Rcc, ahb3enr)   == 0x0D4, "RM0399 §8.7.36");
static_assert(offsetof(Rcc, ahb1enr)   == 0x0D8, "RM0399 §8.7.38");
static_assert(offsetof(Rcc, ahb4enr)   == 0x0E0, "RM0399 §8.7.41");
static_assert(offsetof(Rcc, apb3enr)   == 0x0E4, "RM0399 §8.7.43");
static_assert(offsetof(Rcc, c1_ahb3enr) == 0x134, "RM0399 §8.7.x (CM7 AHB3 gate)");

// `mmio: owned by RM0399 §8.7; never freed.`
inline constexpr MmioAddr<Rcc> RCC{0x5802'4400u};

// --- Bit positions used by disco::clocks::init() --------------------
// (Caller code MUST NOT replicate these; if a sub-phase needs a new
// bit, add the constant here with the RM0399 cite.)

namespace cr {
    inline constexpr std::uint32_t HSEON   = 1u << 16; // §8.7.2
    inline constexpr std::uint32_t HSERDY  = 1u << 17;
    inline constexpr std::uint32_t PLL1ON  = 1u << 24;
    inline constexpr std::uint32_t PLL1RDY = 1u << 25;
    inline constexpr std::uint32_t PLL2ON  = 1u << 26;
    inline constexpr std::uint32_t PLL2RDY = 1u << 27;
    inline constexpr std::uint32_t PLL3ON  = 1u << 28;
    inline constexpr std::uint32_t PLL3RDY = 1u << 29;
}

namespace cfgr {
    // §8.7.5 SW[2:0] @ bits 0..2; SWS[2:0] @ bits 3..5
    inline constexpr std::uint32_t SW_HSI  = 0b000u;
    inline constexpr std::uint32_t SW_PLL1 = 0b011u;
    inline constexpr std::uint32_t SW_MASK = 0b111u;
    inline constexpr std::uint32_t SWS_PLL1 = 0b011u << 3;
    inline constexpr std::uint32_t SWS_MASK = 0b111u << 3;
}

namespace d1cfgr {
    // §8.7.6 — D1CPRE[7:4] = SYSCLK divider, HPRE[3:0] = AHB divider,
    //          D1PPRE[6:4]@offset 4 = APB3 divider.
    // /1 = 0b0000; /2 = 0b1000; /4 = 0b1001; …
    inline constexpr std::uint32_t HPRE_DIV2   = 0b1000u <<  0;
    inline constexpr std::uint32_t D1PPRE_DIV2 = 0b100u  <<  4;
    inline constexpr std::uint32_t D1CPRE_DIV1 = 0b0000u <<  8;
}

namespace d2cfgr {
    inline constexpr std::uint32_t D2PPRE1_DIV2 = 0b100u << 4;
    inline constexpr std::uint32_t D2PPRE2_DIV2 = 0b100u << 8;
}

namespace d3cfgr {
    inline constexpr std::uint32_t D3PPRE_DIV2 = 0b100u << 4;
}

namespace pllckselr {
    // §8.7.9 — PLLSRC[1:0] @ bits 0..1; DIVMx[5:0] @ bits 4..9 (×N for x=1..3)
    inline constexpr std::uint32_t PLLSRC_HSE = 0b10u;
    // DIVM1 @ bits 4..9; DIVM2 @ bits 12..17; DIVM3 @ bits 20..25.
    inline constexpr std::uint32_t DIVM1_SHIFT = 4;
    inline constexpr std::uint32_t DIVM2_SHIFT = 12;
    inline constexpr std::uint32_t DIVM3_SHIFT = 20;
}

namespace pllcfgr {
    // §8.7.10. Per-PLL VCOSEL @ bit 1/9/17 (1=mediumVCO=150-420MHz,
    // 0=wideVCO=192-836MHz). RGE[1:0] @ bit 2-3/10-11/18-19 selects
    // the input frequency range: HSE/M = 5 MHz (for our M=5) → range
    // 0b10 (4-8 MHz).
    inline constexpr std::uint32_t PLL1FRACEN = 1u <<  0;
    inline constexpr std::uint32_t PLL1VCOSEL = 1u <<  1;  // 1 = medium VCO
    inline constexpr std::uint32_t PLL1RGE_4_8 = 0b10u << 2;
    inline constexpr std::uint32_t PLL2FRACEN = 1u <<  4;
    inline constexpr std::uint32_t PLL2VCOSEL = 1u <<  5;
    inline constexpr std::uint32_t PLL2RGE_4_8 = 0b10u << 6;
    inline constexpr std::uint32_t PLL3FRACEN = 1u <<  8;
    inline constexpr std::uint32_t PLL3VCOSEL = 1u <<  9;
    inline constexpr std::uint32_t PLL3RGE_4_8 = 0b10u << 10;
    // DIVxEN per output (P / Q / R for each PLL).
    inline constexpr std::uint32_t DIVP1EN = 1u << 16;
    inline constexpr std::uint32_t DIVQ1EN = 1u << 17;
    inline constexpr std::uint32_t DIVR1EN = 1u << 18;
    inline constexpr std::uint32_t DIVP2EN = 1u << 19;
    inline constexpr std::uint32_t DIVQ2EN = 1u << 20;
    inline constexpr std::uint32_t DIVR2EN = 1u << 21;
    inline constexpr std::uint32_t DIVP3EN = 1u << 22;
    inline constexpr std::uint32_t DIVQ3EN = 1u << 23;
    inline constexpr std::uint32_t DIVR3EN = 1u << 24;
}

namespace plldivr {
    // §8.7.11 — PLLnDIVR layout (same shape for n=1,2,3):
    //   bits  0.. 8  DIVN1[8:0]  -> programmed value = N - 1
    //   bits  9..15  DIVP1[6:0]  -> programmed value = P - 1
    //   bits 16..22  DIVQ1[6:0]  -> programmed value = Q - 1
    //   bits 24..30  DIVR1[6:0]  -> programmed value = R - 1
    constexpr std::uint32_t pack(std::uint16_t n,
                                 std::uint8_t  p,
                                 std::uint8_t  q,
                                 std::uint8_t  r) {
        return (static_cast<std::uint32_t>(n - 1) & 0x1FFu)
             | ((static_cast<std::uint32_t>(p - 1) & 0x7Fu) <<  9)
             | ((static_cast<std::uint32_t>(q - 1) & 0x7Fu) << 16)
             | ((static_cast<std::uint32_t>(r - 1) & 0x7Fu) << 24);
    }
}

namespace d1ccipr {
    // §8.7.13. FMCSEL[1:0] @ bits 0..1.
    inline constexpr std::uint32_t FMCSEL_PLL2_R = 0b10u;
    inline constexpr std::uint32_t FMCSEL_MASK   = 0b11u;
}

namespace ahb3enr {
    // §8.7.36
    inline constexpr std::uint32_t MDMAEN = 1u <<  0;
    inline constexpr std::uint32_t FMCEN  = 1u << 12;
}

namespace ahb1enr {
    // §8.7.38
    inline constexpr std::uint32_t DMA2DEN = 1u << 23;
}

namespace ahb4enr {
    // §8.7.41 — port enables, GPIOAEN..GPIOKEN @ bits 0..10.
    inline constexpr std::uint32_t bit(std::uint32_t port_index) {
        return 1u << port_index;
    }
}

namespace apb3enr {
    // §8.7.43
    inline constexpr std::uint32_t LTDCEN = 1u << 3;
    inline constexpr std::uint32_t DSIEN  = 1u << 4;
}

} // namespace lvglpp::disco::regs
