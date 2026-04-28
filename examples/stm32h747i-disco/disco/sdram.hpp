// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1034–1132
//         + BOOT.md § "SDRAM Bring-Up (Unrolled PAC Init)".
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02c §5.6 — SDRAM bank-2 bring-up. After init() returns,
// 0xD000_0000..0xD2000000 (32 MB) is read/write-stable, MPU
// region 0 covers it as Normal/Shareable/non-cacheable, and
// breadcrumb 0xA11C_0009 is written.

#pragma once

#include <cstdint>

namespace lvglpp::disco::sdram {

inline constexpr std::uintptr_t BASE   = 0xD000'0000u;
inline constexpr std::uint32_t  SIZE   = 32u * 1024u * 1024u;

// Run the bring-up sequence. Pre-condition: clocks::init() and
// pinmux::mux_fmc_pins() have already run. On any timeout, traps
// via __BKPT(0). On success writes breadcrumb 0xA11C_0009.
void init() noexcept;

} // namespace lvglpp::disco::sdram
