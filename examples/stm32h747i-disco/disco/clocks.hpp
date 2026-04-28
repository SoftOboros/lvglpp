// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1569–1596 (PLL tree
//         + PLL3ON gap fix). RM0399 §7 (PWR), §8 (RCC).
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.5 — Clock + power bring-up. The Plan struct is a flat
// aggregate; its defaults are the §5.1 frozen targets. init() runs
// the ten-step bring-up sequence and writes breadcrumb 0xA11C_0005
// at completion.

#pragma once

#include <cstdint>

namespace lvglpp::disco::clocks {

// PLAT-02b §5.1 + §5.2. Defaults are byte-for-byte rlvgl parity.
struct Plan {
    // HSE crystal frequency. The board carries a 25 MHz Y3.
    std::uint32_t hse_hz   = 25'000'000;
    // PLL1: VCO = 800, P→SYSCLK=400, Q→200 (for SDMMC).
    std::uint8_t  pll1_m   = 5;
    std::uint16_t pll1_n   = 160;
    std::uint8_t  pll1_p   = 2;
    std::uint8_t  pll1_q   = 4;
    std::uint8_t  pll1_r   = 2; // unused; written as default
    // PLL2: VCO = 300, R→FMC kernel = 150.
    std::uint8_t  pll2_m   = 5;
    std::uint16_t pll2_n   = 60;
    std::uint8_t  pll2_p   = 2;
    std::uint8_t  pll2_q   = 2;
    std::uint8_t  pll2_r   = 2;
    // PLL3: VCO = 800, R→LTDC pixel ≈ 32 MHz.
    std::uint8_t  pll3_m   = 5;
    std::uint16_t pll3_n   = 160;
    std::uint8_t  pll3_p   = 2;
    std::uint8_t  pll3_q   = 2;
    std::uint8_t  pll3_r   = 25;
};

// Run the ten-step bring-up sequence. On any timeout, traps via
// `__BKPT(0)` (per PLAT-02 § "faults break in place"). On success,
// writes breadcrumb 0xA11C_0005 to 0x3800_0300 and returns.
//
// MUST be called once, from main(), before any peripheral access
// (FMC / LTDC / DSI / I2C / USART / DMA2D / GPIO mux).
void init(const Plan& plan = Plan{}) noexcept;

} // namespace lvglpp::disco::clocks
