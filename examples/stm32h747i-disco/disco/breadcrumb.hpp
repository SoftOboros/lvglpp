// PARITY: rlvgl/examples/stm32h747i-disco BOOT.md § "Breadcrumb to D3
//         SRAM" + Vol II Ch 2/4 breadcrumb writes.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.8 — D3 SRAM breadcrumb word at 0x3800_0300. Codes are
// frozen and shared with rlvgl byte-for-byte; a halted target's
// memory dump diffs cleanly across implementations.
//
// `mmio: owned by D3 SRAM4 (RM0399 §3.3.5); never freed.`

#pragma once

#include <cstdint>

namespace lvglpp::disco::breadcrumb {

inline constexpr std::uintptr_t WORD_ADDR = 0x3800'0300u;

inline constexpr std::uint32_t CLOCKS_LIVE_PRE_GPIO_SPLIT = 0xA11C'0005u;
inline constexpr std::uint32_t FMC_PINS_MUXED             = 0xA11C'0007u;
inline constexpr std::uint32_t SDRAM_INIT_COMPLETE        = 0xA11C'0009u;
inline constexpr std::uint32_t PANEL_UP                   = 0xA11C'000Bu;
inline constexpr std::uint32_t DMA2D_FIRST_DONE           = 0xA11C'000Du;
inline constexpr std::uint32_t FIRST_TOUCH_DISPATCHED     = 0xA11C'000Fu;

inline void write(std::uint32_t code) noexcept {
    *reinterpret_cast<volatile std::uint32_t*>(WORD_ADDR) = code;
}

} // namespace lvglpp::disco::breadcrumb
