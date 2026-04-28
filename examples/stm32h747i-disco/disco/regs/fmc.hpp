// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1034–1132
//         (pac_sdram_init). RM0399 §22.9.5 layout.
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02c §5.1 — FMC SDRAM typed register block. Only the SDRAM
// register set is modelled; NOR/PSRAM/NAND modes are not used by
// PLAT-02c and would be added by their own sub-phase.
//
// `mmio: owned by RM0399 §22.9; never freed.`

#pragma once

#include <cstddef>
#include <cstdint>

#include "../addr.hpp"

namespace lvglpp::disco::regs {

struct alignas(4) Fmc {
    // BCR1 carries the FMC controller master-enable (FMCEN, bit 31).
    // rlvgl sets it before any SDRAM register write — without it the
    // FMC bus clock is on but the controller does not respond,
    // SDRAM reads return random bus garbage.
    volatile std::uint32_t bcr1;    // 0x000 — RM0399 §22.7.2 (BCR1.FMCEN @ bit 31)
    // BTR1/BCR2..4/BTR2..4/PCR for non-SDRAM modes elided. The SDRAM
    // block starts at offset 0x140.
    std::uint8_t           _rsvd_004[0x140 - 0x004];
    volatile std::uint32_t sdcr1;   // 0x140 — RM0399 §22.9.5.1
    volatile std::uint32_t sdcr2;   // 0x144
    volatile std::uint32_t sdtr1;   // 0x148 — RM0399 §22.9.5.2
    volatile std::uint32_t sdtr2;   // 0x14C
    volatile std::uint32_t sdcmr;   // 0x150 — RM0399 §22.9.5.3
    volatile std::uint32_t sdrtr;   // 0x154 — RM0399 §22.9.5.4
    volatile std::uint32_t sdsr;    // 0x158 — RM0399 §22.9.5.5
};

static_assert(offsetof(Fmc, bcr1)  == 0x000, "RM0399 §22.7.2");
static_assert(offsetof(Fmc, sdcr1) == 0x140, "RM0399 §22.9.5.1");
static_assert(offsetof(Fmc, sdcr2) == 0x144, "RM0399 §22.9.5.1");
static_assert(offsetof(Fmc, sdtr1) == 0x148, "RM0399 §22.9.5.2");
static_assert(offsetof(Fmc, sdtr2) == 0x14C, "RM0399 §22.9.5.2");
static_assert(offsetof(Fmc, sdcmr) == 0x150, "RM0399 §22.9.5.3");
static_assert(offsetof(Fmc, sdrtr) == 0x154, "RM0399 §22.9.5.4");
static_assert(offsetof(Fmc, sdsr)  == 0x158, "RM0399 §22.9.5.5");

// `mmio: owned by RM0399 §22.9; never freed.`
inline constexpr MmioAddr<Fmc> FMC{0x5200'4000u};

namespace bcr1 {
    inline constexpr std::uint32_t FMCEN = 1u << 31;  // §22.7.2
}

namespace sdcr {
    // §22.9.5.1
    inline constexpr std::uint32_t NC_9     = 0b01u <<  0;  // 9 column bits
    inline constexpr std::uint32_t NR_12    = 0b01u <<  2;  // 12 row bits
    inline constexpr std::uint32_t MWID_32  = 0b10u <<  4;  // 32-bit data
    inline constexpr std::uint32_t NB_4     = 1u    <<  6;  // 4 internal banks
    inline constexpr std::uint32_t CAS_3    = 0b11u <<  7;  // CAS = 3 cycles
    inline constexpr std::uint32_t SDCLK_2  = 0b01u << 10;  // silicon-required
                                                            // (rlvgl Vol II Ch 3 §2)
    inline constexpr std::uint32_t RBURST   = 1u    << 12;
}

namespace sdtr {
    // §22.9.5.2 — programmed value = cycles - 1.
    constexpr std::uint32_t pack(std::uint32_t tmrd,
                                 std::uint32_t txsr,
                                 std::uint32_t tras,
                                 std::uint32_t trc,
                                 std::uint32_t twr,
                                 std::uint32_t trp,
                                 std::uint32_t trcd) {
        return ((tmrd & 0xFu)        )
             | ((txsr & 0xFu) <<  4)
             | ((tras & 0xFu) <<  8)
             | ((trc  & 0xFu) << 12)
             | ((twr  & 0xFu) << 16)
             | ((trp  & 0xFu) << 20)
             | ((trcd & 0xFu) << 24);
    }
}

namespace sdcmr {
    // §22.9.5.3
    inline constexpr std::uint32_t MODE_NORMAL    = 0b000u;
    inline constexpr std::uint32_t MODE_CLK_EN    = 0b001u;
    inline constexpr std::uint32_t MODE_PALL      = 0b010u;
    inline constexpr std::uint32_t MODE_AUTO_REF  = 0b011u;
    inline constexpr std::uint32_t MODE_LOAD_MODE = 0b100u;
    inline constexpr std::uint32_t CTB2 = 1u << 3;
    inline constexpr std::uint32_t CTB1 = 1u << 4;
    constexpr std::uint32_t pack(std::uint32_t mode,
                                 std::uint32_t ctb,
                                 std::uint32_t nrfs,
                                 std::uint32_t mrd) {
        return (mode & 0b111u)
             | (ctb  & (CTB1 | CTB2))
             | ((nrfs & 0xFu) <<  5)
             | ((mrd  & 0x1FFFu) <<  9);
    }
}

namespace sdsr {
    // §22.9.5.5 — bit 5 = BUSY (read-only).
    inline constexpr std::uint32_t BUSY = 1u << 5;
}

} // namespace lvglpp::disco::regs
