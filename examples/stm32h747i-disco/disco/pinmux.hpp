// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1643–1707 (FMC pin
//         flood). RM0399 §14.4.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.6 + §5.7 — GPIO pinmux helpers + the ~50-pin FMC mux
// table. mux_fmc_pins() writes breadcrumb 0xA11C_0007 at completion.

#pragma once

#include <cstdint>

#include "addr.hpp"
#include "regs/gpio.hpp"

namespace lvglpp::disco::pinmux {

enum class Speed : std::uint8_t {
    Low      = 0,
    Medium   = 1,
    High     = 2,
    VeryHigh = 3,
};

// Set MODER=AF, OSPEEDR=<speed>, AFRL/AFRH=<af> for one pin (0..15).
// PUPDR is left untouched (default no pull-up/down on reset).
void af_speed(MmioAddr<regs::GpioPort> port,
              std::uint8_t pin,
              std::uint8_t af,
              Speed        speed) noexcept;

// AF12 + VeryHigh. Mirrors rlvgl's `af12_high!` macro.
inline void af12_high(MmioAddr<regs::GpioPort> port,
                      std::uint8_t pin) noexcept {
    af_speed(port, pin, 12, Speed::VeryHigh);
}

// MODER=Input, PUPDR=NoPull. For pins observed via IDR (e.g. FT5336 INT).
void floating_input(MmioAddr<regs::GpioPort> port,
                    std::uint8_t pin) noexcept;

// FMC pin set per PLAT-02b §5.7 — calls af12_high() for every pin
// the FMC controller (SDRAM Bank 1) consumes. Writes breadcrumb
// 0xA11C_0007 at completion. Pre-condition: disco::clocks::init()
// has run (port clocks gated on).
void mux_fmc_pins() noexcept;

} // namespace lvglpp::disco::pinmux
