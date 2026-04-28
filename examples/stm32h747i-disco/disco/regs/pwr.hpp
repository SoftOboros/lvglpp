// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs (PWR SMPS + VOS1).
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.4 — PWR typed register block. RM0399 §7.6.
//
// `mmio: owned by RM0399 §7.6; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) Pwr {
    volatile std::uint32_t cr1;     // 0x00 §7.6.1
    volatile std::uint32_t csr1;    // 0x04 §7.6.2
    volatile std::uint32_t cr2;     // 0x08 §7.6.3
    volatile std::uint32_t cr3;     // 0x0C §7.6.4 (SCUEN, LDOEN, SMPSEN, …)
    volatile std::uint32_t cpucr;   // 0x10 §7.6.5
    std::uint32_t          _rsvd_14;
    volatile std::uint32_t d3cr;    // 0x18 §7.6.7 (VOS bits + VOSRDY)
    std::uint32_t          _rsvd_1C;
    volatile std::uint32_t wkupcr;  // 0x20
    volatile std::uint32_t wkupfr;  // 0x24
    volatile std::uint32_t wkupepr; // 0x28
};

static_assert(offsetof(Pwr, cr1)  == 0x00, "RM0399 §7.6.1");
static_assert(offsetof(Pwr, cr3)  == 0x0C, "RM0399 §7.6.4");
static_assert(offsetof(Pwr, d3cr) == 0x18, "RM0399 §7.6.7");

// `mmio: owned by RM0399 §7.6; never freed.`
inline constexpr MmioAddr<Pwr> PWR{0x5802'4800u};

namespace cr3 {
    // §7.6.4
    inline constexpr std::uint32_t BYPASS = 1u <<  0;
    inline constexpr std::uint32_t LDOEN  = 1u <<  1;
    inline constexpr std::uint32_t SDEN   = 1u <<  2; // SMPS step-down enable
    inline constexpr std::uint32_t SCUEN  = 1u <<  2; // alias used in older RMs
    inline constexpr std::uint32_t SMPSEXTHP  = 1u <<  3;
    inline constexpr std::uint32_t SMPSLEVEL_2V5 = 0b10u << 4;
}

namespace d3cr {
    // §7.6.7 — VOS[15:14], VOSRDY @ bit 13.
    inline constexpr std::uint32_t VOSRDY = 1u << 13;
    inline constexpr std::uint32_t VOS_MASK  = 0b11u << 14;
    inline constexpr std::uint32_t VOS_VOS3  = 0b01u << 14;
    inline constexpr std::uint32_t VOS_VOS2  = 0b10u << 14;
    inline constexpr std::uint32_t VOS_VOS1  = 0b11u << 14;
}

} // namespace lvglpp::disco::regs
