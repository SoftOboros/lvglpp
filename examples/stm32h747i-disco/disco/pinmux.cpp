// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1643–1707
//         (FMC pin flood). HARDWARE.md § "Hardware Pin Summary".
// LVGL:   N/A.
// DELTA:  none.

#include "pinmux.hpp"

#include "breadcrumb.hpp"

namespace lvglpp::disco::pinmux {

namespace {

// Generic 2-bit-per-pin field write (MODER, OSPEEDR, PUPDR shape).
void write_2bit(volatile std::uint32_t& reg,
                std::uint8_t pin,
                std::uint32_t value_2b) noexcept {
    const std::uint32_t shift = static_cast<std::uint32_t>(pin) * 2u;
    const std::uint32_t mask  = 0b11u << shift;
    const std::uint32_t v     = reg;
    reg = (v & ~mask) | ((value_2b & 0b11u) << shift);
}

// 4-bit-per-pin field write (AFRL pins 0..7, AFRH pins 8..15).
void write_4bit(volatile std::uint32_t& reg,
                std::uint8_t pin_in_reg,
                std::uint32_t value_4b) noexcept {
    const std::uint32_t shift = static_cast<std::uint32_t>(pin_in_reg) * 4u;
    const std::uint32_t mask  = 0xFu << shift;
    const std::uint32_t v     = reg;
    reg = (v & ~mask) | ((value_4b & 0xFu) << shift);
}

} // namespace

void af_speed(MmioAddr<regs::GpioPort> port,
              std::uint8_t pin,
              std::uint8_t af,
              Speed        speed) noexcept {
    auto& p = port.ref();
    write_2bit(p.moder,   pin, regs::moder::ALTERNATE);
    write_2bit(p.ospeedr, pin, static_cast<std::uint32_t>(speed));
    if (pin < 8) {
        write_4bit(p.afrl, pin,     af);
    } else {
        write_4bit(p.afrh, pin - 8, af);
    }
}

void floating_input(MmioAddr<regs::GpioPort> port,
                    std::uint8_t pin) noexcept {
    auto& p = port.ref();
    write_2bit(p.moder, pin, regs::moder::INPUT);
    write_2bit(p.pupdr, pin, 0b00u); // no pull
}

void mux_fmc_pins() noexcept {
    // Per PLAT-02b §5.7 + rlvgl HARDWARE.md § "Hardware Pin Summary
    // (FMC SDRAM)". Every pin gets AF12 + VeryHigh.
    using namespace regs;

    // Address A0..A11 + bank addresses + clock/enable + control:
    //   PF0..PF5 (A0..A5), PF11..PF15 (A6 + NRAS + A7..A9? — see RM)
    // The exact AX↔Pin mapping is in DS12930 Table 9; we do not need
    // to know the assignment here, only that each pin gets AF12.

    // GPIOF: PF0..PF5, PF11..PF15
    af12_high(GPIOF, 0);
    af12_high(GPIOF, 1);
    af12_high(GPIOF, 2);
    af12_high(GPIOF, 3);
    af12_high(GPIOF, 4);
    af12_high(GPIOF, 5);
    af12_high(GPIOF, 11); // SDNRAS
    af12_high(GPIOF, 12);
    af12_high(GPIOF, 13);
    af12_high(GPIOF, 14);
    af12_high(GPIOF, 15);

    // GPIOG: PG0..PG2, PG4 (BA0), PG5 (BA1), PG8 (SDCLK), PG15 (SDNCAS)
    af12_high(GPIOG, 0);
    af12_high(GPIOG, 1);
    af12_high(GPIOG, 2);
    af12_high(GPIOG, 4);
    af12_high(GPIOG, 5);
    af12_high(GPIOG, 8);
    af12_high(GPIOG, 15);

    // GPIOH: PH5 (SDNWE), PH6 (SDNE1), PH7 (SDCKE1), PH8..PH15 (D0..D7)
    af12_high(GPIOH, 5);
    af12_high(GPIOH, 6);
    af12_high(GPIOH, 7);
    af12_high(GPIOH, 8);
    af12_high(GPIOH, 9);
    af12_high(GPIOH, 10);
    af12_high(GPIOH, 11);
    af12_high(GPIOH, 12);
    af12_high(GPIOH, 13);
    af12_high(GPIOH, 14);
    af12_high(GPIOH, 15);

    // GPIOI: PI0..PI3 (D8..D11), PI4..PI5 (NBL2/NBL3),
    //        PI6/PI7/PI9/PI10 (D12..D15)
    af12_high(GPIOI, 0);
    af12_high(GPIOI, 1);
    af12_high(GPIOI, 2);
    af12_high(GPIOI, 3);
    af12_high(GPIOI, 4);
    af12_high(GPIOI, 5);
    af12_high(GPIOI, 6);
    af12_high(GPIOI, 7);
    af12_high(GPIOI, 9);
    af12_high(GPIOI, 10);

    // GPIOD: PD0/1/8/9/10/14/15 (D2..D3, D13..D15, D0..D1 mix)
    af12_high(GPIOD, 0);
    af12_high(GPIOD, 1);
    af12_high(GPIOD, 8);
    af12_high(GPIOD, 9);
    af12_high(GPIOD, 10);
    af12_high(GPIOD, 14);
    af12_high(GPIOD, 15);

    // GPIOE: PE0/PE1 (NBL0/NBL1), PE7..PE15 (data lanes)
    af12_high(GPIOE, 0);
    af12_high(GPIOE, 1);
    af12_high(GPIOE, 7);
    af12_high(GPIOE, 8);
    af12_high(GPIOE, 9);
    af12_high(GPIOE, 10);
    af12_high(GPIOE, 11);
    af12_high(GPIOE, 12);
    af12_high(GPIOE, 13);
    af12_high(GPIOE, 14);
    af12_high(GPIOE, 15);

    breadcrumb::write(breadcrumb::FMC_PINS_MUXED);
}

} // namespace lvglpp::disco::pinmux
