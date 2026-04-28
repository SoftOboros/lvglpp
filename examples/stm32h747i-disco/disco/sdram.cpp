// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1034–1132 +
//         BOOT.md § "SDRAM Bring-Up (Unrolled PAC Init)".
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02c §5.6. Order: SDCR/SDTR → JEDEC → SDRTR → MPU → DSB/ISB →
// breadcrumb.

#include "sdram.hpp"

#include "addr.hpp"
#include "breadcrumb.hpp"
#include "regs/fmc.hpp"
#include "regs/mpu.hpp"

namespace lvglpp::disco::sdram {

namespace {

[[noreturn]] void trap() noexcept {
    for (;;) asm volatile ("bkpt 0");
}

void wait_busy_clear() noexcept {
    using namespace regs;
    for (std::uint32_t i = 0; i < 1'000'000u; ++i) {
        if ((FMC->sdsr & sdsr::BUSY) == 0) return;
    }
    trap();
}

void issue(std::uint32_t mode,
           std::uint32_t nrfs,
           std::uint32_t mrd) noexcept {
    using namespace regs;
    wait_busy_clear();
    FMC->sdcmr = sdcmr::pack(mode, sdcmr::CTB2, nrfs, mrd);
    wait_busy_clear();
}

void delay_cycles(std::uint32_t n) noexcept {
    // Bounded busy-wait. At 400 MHz, 100k iterations ≈ 250 µs —
    // comfortably > the 100 µs SDRAM power-up requirement. The
    // asm clobber prevents the compiler from optimizing the loop
    // away; no volatile increment needed.
    for (std::uint32_t i = 0; i < n; ++i) {
        asm volatile ("" ::: "memory");
    }
}

} // namespace

void init() noexcept {
    using namespace regs;

    // --- Step 0: FMC controller master-enable ----------------------
    // BCR1.FMCEN must be set before any SDRAM register write —
    // without it the FMC bus clock is on but the controller doesn't
    // respond. Mirrors rlvgl `configure_fmc_sdram` step 1.
    FMC->bcr1 = FMC->bcr1 | bcr1::FMCEN;

    // --- Step 1: SDCR1 / SDCR2 -------------------------------------
    // SDCR1 carries ONLY the shared bits (SDCLK + RBURST + RPIPE);
    // bank-shape fields (NC/NR/MWID/NB/CAS) live exclusively in
    // SDCR2 since the SDRAM is wired to bank 2. Mirrors rlvgl
    // configure_fmc_sdram exactly: bank-shape duplication in SDCR1
    // is a chapter-prose hazard that doesn't reflect rlvgl's
    // shipped values.
    FMC->sdcr1 = sdcr::SDCLK_2 | sdcr::RBURST;
    FMC->sdcr2 = sdcr::NC_9 | sdcr::NR_12 | sdcr::MWID_32
               | sdcr::NB_4 | sdcr::CAS_3;

    // --- Step 2: SDTR1 / SDTR2 -------------------------------------
    // SDTR1: TRP=2, TRC=7 (others unused on bank 2's row of fields).
    // SDTR2: TMRD=2, TXSR=7, TRAS=5, TWR=2, TRCD=2.
    FMC->sdtr1 = sdtr::pack(/*tmrd*/0, /*txsr*/0, /*tras*/0,
                            /*trc*/6,
                            /*twr*/0,
                            /*trp*/1,
                            /*trcd*/0);
    FMC->sdtr2 = sdtr::pack(/*tmrd*/1, /*txsr*/6, /*tras*/4,
                            /*trc*/0,
                            /*twr*/1,
                            /*trp*/0,
                            /*trcd*/1);

    // --- Step 3: JEDEC sequence ------------------------------------
    issue(sdcmr::MODE_CLK_EN,    /*nrfs*/0, /*mrd*/0);
    delay_cycles(100'000);                            // ≥100 µs
    issue(sdcmr::MODE_PALL,      /*nrfs*/0, /*mrd*/0);
    issue(sdcmr::MODE_AUTO_REF,  /*nrfs*/7, /*mrd*/0); // 8 cycles
    issue(sdcmr::MODE_LOAD_MODE, /*nrfs*/0, /*mrd*/0x230);
    // 6th command — Normal mode. rlvgl shipped sequence concludes
    // with this; without it some H7 silicon revisions leave the
    // controller in load-mode-register state and reads return 0.
    issue(sdcmr::MODE_NORMAL,    /*nrfs*/0, /*mrd*/0);

    // --- Step 4: SDRTR refresh timer -------------------------------
    // 566 << 1 — RM0399 §22.9.5.4 LSB is reserved (always 0).
    FMC->sdrtr = 566u << 1;
    wait_busy_clear(); // final BUSY drop, rlvgl wait_for_sdram_ready

    // --- Step 5: MPU region 0 covers 0xD000_0000 (32 MiB) ----------
    // Disable MPU first to let RBAR/RASR settle.
    MPU->ctrl = 0;
    asm volatile ("dsb" ::: "memory");
    MPU->rnr  = 0;
    MPU->rbar = rbar::pack(BASE, /*region*/0);
    MPU->rasr = rasr::pack(/*size_bits*/24, /*srd*/0,
                           /*b*/0, /*c*/0, /*s*/1, /*tex*/0b001,
                           /*ap*/0b011, /*xn*/true);
    MPU->ctrl = ctrl::ENABLE | ctrl::PRIVDEFENA;
    asm volatile ("dsb\n\tisb" ::: "memory");

    // --- Step 6: Breadcrumb ----------------------------------------
    breadcrumb::write(breadcrumb::SDRAM_INIT_COMPLETE);
}

} // namespace lvglpp::disco::sdram
