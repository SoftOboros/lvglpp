// PARITY: rlvgl/platform/src/hwcore/regs/usart.rs (typed Usart handle)
//         + rlvgl/examples/stm32h747i-disco/src/main.rs L1992–2000
//         (register addresses used by the VCP init). RM0399 USART
//         chapter layout.
// LVGL:   N/A — bring-up plumbing below the LVGL surface.
// DELTA:  none.
//
// PLAT-02f §5.1 — USART1 typed register block at 0x4001_1000.
//
// `mmio: owned by RM0399 USART chapter; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) Usart {
    volatile std::uint32_t cr1;    // 0x00 — control 1 (UE/RE/TE/FIFOEN)
    volatile std::uint32_t cr2;    // 0x04 — control 2 (STOP bits)
    volatile std::uint32_t cr3;    // 0x08 — control 3
    volatile std::uint32_t brr;    // 0x0C — baud rate
    volatile std::uint32_t gtpr;   // 0x10
    volatile std::uint32_t rtor;   // 0x14
    volatile std::uint32_t rqr;    // 0x18 — request (RXFRQ flush)
    volatile std::uint32_t isr;    // 0x1C — status
    volatile std::uint32_t icr;    // 0x20 — interrupt flag clear (w1c)
    volatile std::uint32_t rdr;    // 0x24 — receive data
    volatile std::uint32_t tdr;    // 0x28 — transmit data
    volatile std::uint32_t presc;  // 0x2C — prescaler
};

static_assert(offsetof(Usart, cr1)   == 0x00, "RM0399 USART_CR1");
static_assert(offsetof(Usart, brr)   == 0x0C, "RM0399 USART_BRR");
static_assert(offsetof(Usart, rqr)   == 0x18, "RM0399 USART_RQR");
static_assert(offsetof(Usart, isr)   == 0x1C, "RM0399 USART_ISR");
static_assert(offsetof(Usart, icr)   == 0x20, "RM0399 USART_ICR");
static_assert(offsetof(Usart, rdr)   == 0x24, "RM0399 USART_RDR");
static_assert(offsetof(Usart, tdr)   == 0x28, "RM0399 USART_TDR");
static_assert(offsetof(Usart, presc) == 0x2C, "RM0399 USART_PRESC");

// `mmio: owned by RM0399 USART chapter; never freed.`
inline constexpr MmioAddr<Usart> USART1{0x4001'1000u};

namespace usart_cr1 {
    inline constexpr std::uint32_t UE     = 1u << 0;
    inline constexpr std::uint32_t RE     = 1u << 2;
    inline constexpr std::uint32_t TE     = 1u << 3;
    inline constexpr std::uint32_t FIFOEN = 1u << 29;
}

namespace usart_isr {
    // FIFO-mode names (FIFOEN=1): RXFNE/TXFNF.
    inline constexpr std::uint32_t ORE        = 1u << 3;
    inline constexpr std::uint32_t RXNE_RXFNE = 1u << 5;
    inline constexpr std::uint32_t TXE_TXFNF  = 1u << 7;
}

namespace usart_icr {
    inline constexpr std::uint32_t ORECF = 1u << 3;
    // PE/FE/NE/ORE — the rlvgl runtime_serial ERROR_CLEAR set.
    inline constexpr std::uint32_t ERROR_CLEAR = 0b1111u;
}

} // namespace lvglpp::disco::regs
