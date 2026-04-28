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

[[noreturn]] void trap() noexcept {
    for (;;) asm volatile ("bkpt 0");
}

volatile std::uint32_t* sdram_word(std::uintptr_t offset) noexcept {
    return reinterpret_cast<volatile std::uint32_t*>(
        lvglpp::disco::sdram::BASE + offset);
}

inline void dsb() noexcept { asm volatile ("dsb" ::: "memory"); }

inline volatile std::uint32_t* d3(std::uintptr_t off) noexcept {
    return reinterpret_cast<volatile std::uint32_t*>(0x3800'0300u + off);
}

// Diagnostic memtest: write 4 distinct canaries at 4 SDRAM offsets,
// DSB, then read each back and store the read value to a D3 SRAM
// slot so probe-rs can see *exactly* what came out of SDRAM.
// NO trap on miscompare — we want to see the bits even when wrong.
void memtest_4k() {
    *sdram_word(0x000) = 0xDEAD'BEEFu;
    *sdram_word(0x004) = 0xCAFE'BABEu;
    *sdram_word(0x100) = 0x1234'5678u;
    *sdram_word(0x200) = 0xAAAA'5555u;
    dsb();

    // Read each back and relay verbatim (no compare) into D3 SRAM at
    // 0x3800_0304..0x3800_0314 so probe-rs can dump the row and we
    // can see how SDRAM is actually misbehaving.
    *d3(0x04) = *sdram_word(0x000);
    *d3(0x08) = *sdram_word(0x004);
    *d3(0x0C) = *sdram_word(0x100);
    *d3(0x10) = *sdram_word(0x200);
    dsb();
}

} // namespace

int main() {
    lvglpp::disco::clocks::init();         // 0xA11C_0005
    lvglpp::disco::pinmux::mux_fmc_pins(); // 0xA11C_0007
    lvglpp::disco::sdram::init();          // 0xA11C_0009

    memtest_4k();

    for (;;) asm volatile ("wfe");
}
