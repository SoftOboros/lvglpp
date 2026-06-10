// PARITY: rlvgl uses cortex_m::peripheral::NVIC (unmask +
//         set_priority, e.g. main.rs L672/L751). ARMv7-M ARM B3.4
//         register layout.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02e-2 — minimal NVIC surface: interrupt enable + priority.
//
// `mmio: owned by ARMv7-M System Control Space (B3.4); never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) NvicIser {
    volatile std::uint32_t iser[16];  // 0xE000E100 — set-enable, 1 bit/IRQ
};

struct alignas(4) NvicIpr {
    volatile std::uint8_t ipr[496];   // 0xE000E400 — 1 byte/IRQ, prio in [7:4]
};

// `mmio: owned by ARMv7-M SCS; never freed.`
inline constexpr MmioAddr<NvicIser> NVIC_ISER{0xE000'E100u};
inline constexpr MmioAddr<NvicIpr>  NVIC_IPR{0xE000'E400u};

inline void nvic_enable_irq(std::uint32_t irqn) noexcept {
    NVIC_ISER.ref().iser[irqn / 32u] = 1u << (irqn % 32u);
}

// prio: 0 (highest) .. 15 — H7 implements 4 priority bits, [7:4].
inline void nvic_set_priority(std::uint32_t irqn, std::uint8_t prio) noexcept {
    NVIC_IPR.ref().ipr[irqn] = static_cast<std::uint8_t>(prio << 4);
}

// RM0399 §11.1.4 vector table positions (verified positionally
// against startup_stm32h747xi.cpp).
inline constexpr std::uint32_t IRQN_USART1 = 37;
inline constexpr std::uint32_t IRQN_DMA2D  = 90;

// ── DWT cycle counter (ARMv7-M ARM C1.8) ────────────────────────────
// `mmio: owned by ARMv7-M debug architecture; never freed.`
// PLAT-02e-3a frame-cadence telemetry; mirrors rlvgl's DWT posture
// (CLAUDE.md profiling guidance: prefer DWT over serial for timing).

struct alignas(4) Dwt {
    volatile std::uint32_t ctrl;     // 0xE0001000 — CYCCNTENA @ bit 0
    volatile std::uint32_t cyccnt;   // 0xE0001004 — free-running cycles
};
static_assert(offsetof(Dwt, cyccnt) == 0x4, "ARMv7-M DWT_CYCCNT");

inline constexpr MmioAddr<Dwt> DWT{0xE000'1000u};
// DEMCR.TRCENA gates the whole DWT block.
inline constexpr MmioAddr<volatile std::uint32_t> DEMCR_WORD{0xE000'EDFCu};

inline void dwt_enable_cyccnt() noexcept {
    DEMCR_WORD.ref() = DEMCR_WORD.ref() | (1u << 24);  // TRCENA
    DWT.ref().cyccnt = 0;
    DWT.ref().ctrl   = DWT.ref().ctrl | 1u;            // CYCCNTENA
}

} // namespace lvglpp::disco::regs
