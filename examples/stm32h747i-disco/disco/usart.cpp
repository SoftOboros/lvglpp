// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1972–2000
//         ("USART1 VCP init") + L733–760 (error clear + RX drain).
// LVGL:   N/A.
// DELTA:  Polled (PLAT-02f §5.2); dual-core APB2 gate set explicitly
//         on both views (rlvgl writes the global register only).
//
// PLAT-02f-1 — USART1 VCP transport implementation.

#include "usart.hpp"

#include "regs/gpio.hpp"
#include "regs/rcc.hpp"
#include "regs/usart.hpp"

namespace lvglpp::disco::usart {

namespace {

// observes: cumulative overrun count; single-context (no USART IRQ
// until PLAT-02f-2), so a plain word suffices.
std::uint32_t g_rx_overruns = 0;

} // namespace

void init() noexcept {
    using namespace regs;

    // Clock gates. GPIOA is already on (clocks.cpp step 7); USART1
    // needs APB2 on both the global and CM7 per-core views
    // (PLAT-02b dual-core rule).
    RCC->apb2enr    = RCC->apb2enr    | apb2enr::USART1EN;
    RCC->c1_apb2enr = RCC->c1_apb2enr | apb2enr::USART1EN;
    (void)RCC->apb2enr;  // read-back barrier, mirrors rlvgl

    // PA9 = TX, PA10 = RX, both AF7. AFRH nibbles 1 (PA9) and 2
    // (PA10); MODER 10 (alternate) for both.
    auto& pa = GPIOA.ref();
    pa.afrh  = (pa.afrh & ~((0xFu << 4) | (0xFu << 8)))
             | (7u << 4) | (7u << 8);
    pa.moder = (pa.moder & ~((0x3u << 18) | (0x3u << 20)))
             | (moder::ALTERNATE << 18) | (moder::ALTERNATE << 20);

    // 115200 8N1, FIFO mode. Kernel clock = rcc_pclk2 = 100 MHz
    // (D2CCIP2R.USART16SEL left at reset); BRR = 100e6 / 115200.
    auto& u = USART1.ref();
    u.brr = 868u;
    u.cr1 = usart_cr1::FIFOEN | usart_cr1::TE | usart_cr1::RE
          | usart_cr1::UE;
}

void put_byte(std::uint8_t b) noexcept {
    using namespace regs;
    auto& u = USART1.ref();
    while ((u.isr & usart_isr::TXE_TXFNF) == 0u) {}
    u.tdr = b;
}

void put(std::string_view s) noexcept {
    for (char c : s) put_byte(static_cast<std::uint8_t>(c));
}

int get_byte() noexcept {
    using namespace regs;
    auto& u = USART1.ref();
    if ((u.isr & usart_isr::ORE) != 0u) {
        u.icr = usart_icr::ORECF;
        ++g_rx_overruns;
    }
    if ((u.isr & usart_isr::RXNE_RXFNE) == 0u) return -1;
    return static_cast<int>(u.rdr & 0xFFu);
}

std::uint32_t rx_overruns() noexcept {
    return g_rx_overruns;
}

} // namespace lvglpp::disco::usart
