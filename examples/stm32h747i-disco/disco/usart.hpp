// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1972–2000
//         ("USART1 VCP init", v0.2.0 @ 79f730d).
// LVGL:   N/A.
// DELTA:  Polled TX/RX (PLAT-02f §5.2); rlvgl's IRQ + ring-buffer
//         runtime_serial is the 02f-2 shape.
//
// PLAT-02f-1 — USART1 over the ST-LINK VCP, 115200 8N1 FIFO mode.
// See docs/platform-disco/06-touch-and-uart.md.

#pragma once

#include <cstdint>
#include <string_view>

namespace lvglpp::disco::usart {

// Bring up USART1 on PA9 (TX, AF7) / PA10 (RX, AF7), 115200 8N1,
// FIFO mode. The USART1 peripheral state is `external:` — configured
// once at boot, never torn down.
//
// Pre-condition: clocks::init() has run (GPIOA gated on AHB4, APB2
// running at 100 MHz — the BRR value is frozen against that clock,
// PLAT-02f §5.1).
void init() noexcept;

// Blocking byte/string TX (spins on TXFNF; bounded by the FIFO drain
// rate, ~87 µs/byte at 115200 — no timeout by design, matching the
// rlvgl serial_puts posture).
void put_byte(std::uint8_t b) noexcept;
void put(std::string_view s) noexcept;

// Non-blocking RX: returns -1 when the RX FIFO is empty, else the
// byte. Clears+counts overruns internally (count via rx_overruns()).
[[nodiscard]] int get_byte() noexcept;

// Cumulative ORE (overrun) events since boot, for the D3 relay.
[[nodiscard]] std::uint32_t rx_overruns() noexcept;

} // namespace lvglpp::disco::usart
