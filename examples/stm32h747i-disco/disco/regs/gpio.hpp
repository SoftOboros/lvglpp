// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1643–1707 (FMC pin
//         AF12 + VeryHigh flood). RM0399 §14.4 layout.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.4 — GPIO typed register block. One layout, eleven
// `MmioAddr<GpioPort>` constants for ports A..K.
//
// `mmio: owned by RM0399 §14.4; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) GpioPort {
    volatile std::uint32_t moder;     // 0x00 §14.4.1
    volatile std::uint32_t otyper;    // 0x04 §14.4.2
    volatile std::uint32_t ospeedr;   // 0x08 §14.4.3
    volatile std::uint32_t pupdr;     // 0x0C §14.4.4
    volatile std::uint32_t idr;       // 0x10 §14.4.5
    volatile std::uint32_t odr;       // 0x14 §14.4.6
    volatile std::uint32_t bsrr;      // 0x18 §14.4.7
    volatile std::uint32_t lckr;      // 0x1C §14.4.8
    volatile std::uint32_t afrl;      // 0x20 §14.4.9  (pins 0..7)
    volatile std::uint32_t afrh;      // 0x24 §14.4.10 (pins 8..15)
};

static_assert(offsetof(GpioPort, moder)   == 0x00, "RM0399 §14.4.1");
static_assert(offsetof(GpioPort, ospeedr) == 0x08, "RM0399 §14.4.3");
static_assert(offsetof(GpioPort, afrl)    == 0x20, "RM0399 §14.4.9");
static_assert(offsetof(GpioPort, afrh)    == 0x24, "RM0399 §14.4.10");

// `mmio: owned by RM0399 §14.4; never freed.`
//   Each port lives at AHB4_BASE + 0x400 × index, where index is
//   the AHB4ENR bit position (GPIOA=0..GPIOK=10).
inline constexpr std::uintptr_t GPIO_BASE = 0x5802'0000u;
inline constexpr MmioAddr<GpioPort> GPIOA{GPIO_BASE + 0x400u * 0};
inline constexpr MmioAddr<GpioPort> GPIOB{GPIO_BASE + 0x400u * 1};
inline constexpr MmioAddr<GpioPort> GPIOC{GPIO_BASE + 0x400u * 2};
inline constexpr MmioAddr<GpioPort> GPIOD{GPIO_BASE + 0x400u * 3};
inline constexpr MmioAddr<GpioPort> GPIOE{GPIO_BASE + 0x400u * 4};
inline constexpr MmioAddr<GpioPort> GPIOF{GPIO_BASE + 0x400u * 5};
inline constexpr MmioAddr<GpioPort> GPIOG{GPIO_BASE + 0x400u * 6};
inline constexpr MmioAddr<GpioPort> GPIOH{GPIO_BASE + 0x400u * 7};
inline constexpr MmioAddr<GpioPort> GPIOI{GPIO_BASE + 0x400u * 8};
inline constexpr MmioAddr<GpioPort> GPIOJ{GPIO_BASE + 0x400u * 9};
inline constexpr MmioAddr<GpioPort> GPIOK{GPIO_BASE + 0x400u * 10};

namespace moder {
    inline constexpr std::uint32_t INPUT     = 0b00u;
    inline constexpr std::uint32_t OUTPUT    = 0b01u;
    inline constexpr std::uint32_t ALTERNATE = 0b10u;
    inline constexpr std::uint32_t ANALOG    = 0b11u;
}

} // namespace lvglpp::disco::regs
