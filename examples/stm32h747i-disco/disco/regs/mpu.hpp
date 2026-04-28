// PARITY: ARM ARM (DDI 0403E.e) §B3.5 — Cortex-M7 MPU.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02c §5.5 — Cortex-M7 MPU typed register block at
// 0xE000_ED90.
//
// `mmio: owned by ARM Cortex-M7 architecture; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) Mpu {
    volatile std::uint32_t type_;   // 0x00 — read-only
    volatile std::uint32_t ctrl;    // 0x04
    volatile std::uint32_t rnr;     // 0x08
    volatile std::uint32_t rbar;    // 0x0C
    volatile std::uint32_t rasr;    // 0x10
};

static_assert(offsetof(Mpu, ctrl) == 0x04, "ARM ARM §B3.5.4");
static_assert(offsetof(Mpu, rnr)  == 0x08, "ARM ARM §B3.5.5");
static_assert(offsetof(Mpu, rbar) == 0x0C, "ARM ARM §B3.5.6");
static_assert(offsetof(Mpu, rasr) == 0x10, "ARM ARM §B3.5.7");

// `mmio: owned by ARM Cortex-M7 architecture; never freed.`
inline constexpr MmioAddr<Mpu> MPU{0xE000'ED90u};

namespace ctrl {
    inline constexpr std::uint32_t ENABLE      = 1u << 0;
    inline constexpr std::uint32_t HFNMIENA    = 1u << 1;
    inline constexpr std::uint32_t PRIVDEFENA  = 1u << 2;
}

namespace rbar {
    // bit 4 = VALID; bits 3..0 = REGION (when VALID=1).
    constexpr std::uint32_t pack(std::uint32_t base,
                                 std::uint32_t region) {
        return (base & ~0xFFu) | (1u << 4) | (region & 0xFu);
    }
}

namespace rasr {
    inline constexpr std::uint32_t ENABLE     = 1u << 0;
    inline constexpr std::uint32_t XN         = 1u << 28;
    // SIZE field at bits 1..5: region size = 2^(SIZE+1) bytes.
    // SIZE=24 → 32 MiB.
    constexpr std::uint32_t pack(std::uint32_t size_bits,
                                 std::uint32_t srd,
                                 std::uint32_t b,
                                 std::uint32_t c,
                                 std::uint32_t s,
                                 std::uint32_t tex,
                                 std::uint32_t ap,
                                 bool xn) {
        return ENABLE
             | ((size_bits & 0x1Fu) << 1)
             | ((srd & 0xFFu) << 8)
             | ((b   & 0x1u)  << 16)
             | ((c   & 0x1u)  << 17)
             | ((s   & 0x1u)  << 18)
             | ((tex & 0x7u)  << 19)
             | ((ap  & 0x7u)  << 24)
             | (xn ? XN : 0u);
    }
}

} // namespace lvglpp::disco::regs
