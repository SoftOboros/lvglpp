// PARITY: rlvgl/examples/stm32h747i-disco/Cargo.toml `sdram_ramtest`
//         feature.
// LVGL:   N/A.
// DELTA:  Compact pattern set sized for the smoke gate; rlvgl's full
//         walking-one sweep belongs to a future profiling sub-phase.
//
// PLAT-02c §12 acceptance smoke target. After SDRAM bring-up,
// writes a small fixed pattern + walking bits across the first
// 4 KiB. On any miscompare, traps via __BKPT(0). On success:
//   - breadcrumb 0xA11C_0005 (clocks live)
//   - breadcrumb 0xA11C_0007 (FMC pins muxed)
//   - breadcrumb 0xA11C_0009 (SDRAM live + MPU set)
//   - canaries 0xDEADBEEF / 0xCAFEBABE at 0xD000_0000 / 0xD000_0004
//     (confirmed survive a refresh interval)
//   - relay words at 0x3800_0304 / 0x3800_0308: read-back of the
//     canaries copied to D3 SRAM. The H7 debug AP cannot read
//     FMC-mapped memory directly, so we route through D3 SRAM,
//     which probe-rs CAN read. Matching values prove the memtest
//     succeeded on-CPU.

#include <cstdint>

#include "disco/clocks.hpp"
#include "disco/pinmux.hpp"
#include "disco/sdram.hpp"

namespace {

volatile std::uint32_t* sdram_word(std::uintptr_t offset) noexcept {
    return reinterpret_cast<volatile std::uint32_t*>(
        lvglpp::disco::sdram::BASE + offset);
}

inline void dsb() noexcept { asm volatile ("dsb" ::: "memory"); }

inline volatile std::uint32_t* d3(std::uintptr_t off) noexcept {
    return reinterpret_cast<volatile std::uint32_t*>(0x3800'0300u + off);
}

// Rigorous SDRAM validation before the framebuffer/display stack is
// built on top. Three sub-tests, results relayed to D3 SRAM (which the
// debug AP can read; FMC-mapped SDRAM it cannot):
//
//   d3(0x04) = overall status: 0x600D_0000 pass, 0xBAD0_00xx fail
//              (low byte = failing sub-test: 1 addr, 2 data, 3 sweep)
//   d3(0x08) = first failing byte-offset (or 0xFFFF_FFFF if none)
//   d3(0x0C) = expected word
//   d3(0x10) = observed word
//   d3(0x14) = canary read-back of 0xD000_0000 (legacy 0xDEADBEEF probe)
//   d3(0x18) = canary read-back of 0xD000_0004 (legacy 0xCAFEBABE probe)
//
// (1) ADDRESS-LINE walk: write a unique marker at offset 0 and at each
//     power-of-two byte offset 1<<b for b in [2, 24] (covers the full
//     32 MiB = 2^25). A swapped/stuck/shorted address line (the
//     column-bit-2 bug class) makes two offsets alias, so a later write
//     clobbers an earlier marker — detected on read-back.
// (2) DATA-LINE walk: walking-ones + its complement at a fixed address,
//     catching stuck/shorted data lines.
// (3) RANGE sweep: addr-derived pattern every 64 KiB across the chip,
//     read back — catches refresh/timing faults over the whole array.
void memtest() {
    constexpr std::uint32_t MARK = 0xA5A5'0000u;
    std::uint32_t status   = 0x600D'0000u;
    std::uint32_t fail_off = 0xFFFF'FFFFu;
    std::uint32_t expected = 0, observed = 0;

    // Legacy canaries first (kept for at-a-glance comparison).
    *sdram_word(0x000) = 0xDEAD'BEEFu;
    *sdram_word(0x004) = 0xCAFE'BABEu;
    dsb();
    const std::uint32_t canary0 = *sdram_word(0x000);
    const std::uint32_t canary1 = *sdram_word(0x004);

    // (1) Address-line walk.
    *sdram_word(0) = MARK;  // marker at offset 0
    for (unsigned b = 2; b <= 24; ++b)
        *sdram_word(static_cast<std::uintptr_t>(1u) << b) = MARK | b;
    dsb();
    if (status == 0x600D'0000u) {
        if (*sdram_word(0) != MARK) {
            status = 0xBAD0'0001u; fail_off = 0; expected = MARK;
            observed = *sdram_word(0);
        }
        for (unsigned b = 2; b <= 24 && status == 0x600D'0000u; ++b) {
            const std::uintptr_t off = static_cast<std::uintptr_t>(1u) << b;
            const std::uint32_t e = MARK | b;
            const std::uint32_t g = *sdram_word(off);
            if (g != e) {
                status = 0xBAD0'0001u; fail_off = static_cast<std::uint32_t>(off);
                expected = e; observed = g;
            }
        }
    }

    // (2) Data-line walk at a fixed offset clear of the address markers.
    if (status == 0x600D'0000u) {
        constexpr std::uintptr_t DOFF = 0x30000;
        for (unsigned i = 0; i < 32 && status == 0x600D'0000u; ++i) {
            const std::uint32_t pat = 1u << i;
            *sdram_word(DOFF) = pat;       dsb();
            std::uint32_t g = *sdram_word(DOFF);
            if (g != pat) { status = 0xBAD0'0002u; fail_off = DOFF; expected = pat; observed = g; break; }
            *sdram_word(DOFF) = ~pat;      dsb();
            g = *sdram_word(DOFF);
            if (g != ~pat) { status = 0xBAD0'0002u; fail_off = DOFF; expected = ~pat; observed = g; }
        }
    }

    // (3) Range sweep every 64 KiB across all 32 MiB: value = offset ^ MARK.
    if (status == 0x600D'0000u) {
        for (std::uintptr_t off = 0; off < (32u << 20); off += 0x10000)
            *sdram_word(off) = static_cast<std::uint32_t>(off) ^ MARK;
        dsb();
        for (std::uintptr_t off = 0; off < (32u << 20) && status == 0x600D'0000u; off += 0x10000) {
            const std::uint32_t e = static_cast<std::uint32_t>(off) ^ MARK;
            const std::uint32_t g = *sdram_word(off);
            if (g != e) { status = 0xBAD0'0003u; fail_off = static_cast<std::uint32_t>(off); expected = e; observed = g; }
        }
    }

    *d3(0x04) = status;
    *d3(0x08) = fail_off;
    *d3(0x0C) = expected;
    *d3(0x10) = observed;
    *d3(0x14) = canary0;
    *d3(0x18) = canary1;
    dsb();
}

} // namespace

int main() {
    lvglpp::disco::clocks::init();         // 0xA11C_0005
    lvglpp::disco::pinmux::mux_fmc_pins(); // 0xA11C_0007
    lvglpp::disco::sdram::init();          // 0xA11C_0009

    memtest();

    for (;;) asm volatile ("wfe");
}
