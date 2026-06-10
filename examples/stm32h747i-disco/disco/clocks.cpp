// PARITY: rlvgl/examples/stm32h747i-disco/src/main.rs L1569–1596 +
//         BOOT.md § "Boot Sequence Snapshot".
// LVGL:   N/A.
// DELTA:  none.
//
// PLAT-02b §5.5 ten-step bring-up sequence:
//   1. Power posture: SMPS-only + VOS1, poll VOSRDY.
//   2. HSE on, poll HSERDY.
//   3. Configure PLL1/2/3 (DIVMx, VCOSEL/RGE, PLLnDIVR).
//   4. Latch PLL1, PLL2; poll PLLnRDY.
//   5. Bus prescalers + SYSCLK switch to PLL1; poll SWS.
//   6. **PLL3ON gap fix.** Force PLL3ON; poll PLL3RDY. Mirrors
//      rlvgl/docs/disco-platform-guide/02-clocks-and-plls.md
//      § "Force PLL3 on" — without this step the LTDC AHB slave
//      hangs on first read because there is no pixel clock.
//   7. Peripheral clock-gates (FMC/MDMA/DMA2D/LTDC/DSI + GPIO ports).
//   8. Kernel-clock muxes (FMCSEL = PLL2_R).
//   9. Breadcrumb write 0xA11C_0005 to 0x3800_0300.
//  10. Return.

#include "clocks.hpp"

#include "addr.hpp"
#include "breadcrumb.hpp"
#include "regs/pwr.hpp"
#include "regs/rcc.hpp"

namespace lvglpp::disco::clocks {

namespace {

// Bounded spin. At HSI 64 MHz boot, 1M cycles ≈ 16 ms — comfortably
// longer than VOSRDY / HSERDY / PLLnRDY datasheet typicals.
template <typename Pred>
[[nodiscard]] bool wait_until(Pred pred) noexcept {
    for (std::uint32_t i = 0; i < 1'000'000u; ++i) {
        if (pred()) return true;
    }
    return false;
}

[[noreturn]] void trap() noexcept {
    for (;;) asm volatile ("bkpt 0");
}

void write_pll_dividers(volatile std::uint32_t& divr,
                        std::uint16_t n,
                        std::uint8_t  p,
                        std::uint8_t  q,
                        std::uint8_t  r) noexcept {
    using namespace regs::plldivr;
    divr = pack(n, p, q, r);
}

} // namespace

void init(const Plan& plan) noexcept {
    using namespace regs;

    // --- Step 1: Power posture (SMPS + VOS1) ------------------------
    {
        // Default reset state of CR3 has LDOEN=1, SDEN=0. Switch to
        // SMPS-only by clearing LDOEN and setting SDEN. RM0399 §7.6.4.
        std::uint32_t cr3 = PWR->cr3;
        cr3 &= ~cr3::LDOEN;
        cr3 |=  cr3::SDEN;
        PWR->cr3 = cr3;

        // Drive VOS to VOS1 (1.2 V).
        std::uint32_t d3cr = PWR->d3cr;
        d3cr = (d3cr & ~d3cr::VOS_MASK) | d3cr::VOS_VOS1;
        PWR->d3cr = d3cr;

        if (!wait_until([] { return (PWR->d3cr & d3cr::VOSRDY) != 0; }))
            trap();
    }

    // --- Step 2: HSE on ---------------------------------------------
    RCC->cr = RCC->cr | cr::HSEON;
    if (!wait_until([] { return (RCC->cr & cr::HSERDY) != 0; }))
        trap();

    // --- Step 3: Configure PLL1/2/3 ---------------------------------
    // DIVMx + PLLSRC.
    {
        const std::uint32_t pllckselr =
            pllckselr::PLLSRC_HSE
            | (static_cast<std::uint32_t>(plan.pll1_m) << pllckselr::DIVM1_SHIFT)
            | (static_cast<std::uint32_t>(plan.pll2_m) << pllckselr::DIVM2_SHIFT)
            | (static_cast<std::uint32_t>(plan.pll3_m) << pllckselr::DIVM3_SHIFT);
        RCC->pllckselr = pllckselr;
    }

    // VCOSEL + RGE + DIVxEN. With M=5 and HSE=25 MHz, PLLn input is
    // 5 MHz — RGE = 0b10 (4..8 MHz). VCO target 800/300/800 MHz; all
    // ≥ 192 MHz so VCOSEL=0 (wide-VCO range, 192..836 MHz). FRACEN
    // off (no fractional multiplier).
    RCC->pllcfgr =
          pllcfgr::PLL1RGE_4_8 | pllcfgr::DIVP1EN | pllcfgr::DIVQ1EN
        | pllcfgr::PLL2RGE_4_8 | pllcfgr::DIVR2EN
        | pllcfgr::PLL3RGE_4_8 | pllcfgr::DIVR3EN;

    write_pll_dividers(RCC->pll1divr, plan.pll1_n, plan.pll1_p, plan.pll1_q, plan.pll1_r);
    write_pll_dividers(RCC->pll2divr, plan.pll2_n, plan.pll2_p, plan.pll2_q, plan.pll2_r);
    write_pll_dividers(RCC->pll3divr, plan.pll3_n, plan.pll3_p, plan.pll3_q, plan.pll3_r);

    // --- Step 4: Latch PLL1, PLL2, PLL3 -----------------------------
    // BENCH ITER (PLAT-02d §15.9): turn on ALL THREE PLLs here, BEFORE the
    // SYSCLK switch — matching stm32h7xx-hal `freeze()` order. Previously
    // PLL3 was forced on AFTER the SYSCLK→PLL1 switch (old "step 6"); the
    // hypothesis is that enabling pll3_r_ck after the clock-tree switch
    // failed to route ltdc_ker_ck to the LTDC (errata 2.13.1: LTDC access
    // stalls when the pixel clock is disabled).
    RCC->cr = RCC->cr | cr::PLL1ON;
    if (!wait_until([] { return (RCC->cr & cr::PLL1RDY) != 0; }))
        trap();
    RCC->cr = RCC->cr | cr::PLL2ON;
    if (!wait_until([] { return (RCC->cr & cr::PLL2RDY) != 0; }))
        trap();
    RCC->cr = RCC->cr | cr::PLL3ON;
    if (!wait_until([] { return (RCC->cr & cr::PLL3RDY) != 0; }))
        trap();

    // --- Step 5: Bus prescalers, then switch SYSCLK to PLL1 ---------
    RCC->d1cfgr = d1cfgr::HPRE_DIV2 | d1cfgr::D1PPRE_DIV2 | d1cfgr::D1CPRE_DIV1;
    RCC->d2cfgr = d2cfgr::D2PPRE1_DIV2 | d2cfgr::D2PPRE2_DIV2;
    RCC->d3cfgr = d3cfgr::D3PPRE_DIV2;

    {
        std::uint32_t cfgr = RCC->cfgr;
        cfgr = (cfgr & ~cfgr::SW_MASK) | cfgr::SW_PLL1;
        RCC->cfgr = cfgr;
    }
    if (!wait_until([] {
        return (RCC->cfgr & cfgr::SWS_MASK) == cfgr::SWS_PLL1;
    })) trap();

    // --- Step 7: Peripheral clock-gates -----------------------------
    // The H7 has dual-core RCC: each peripheral clock gate exists in
    // BOTH the combined register (RCC_AHBxENR) and a per-core copy
    // (RCC_C1_AHBxENR for CM7). Both must be set or the peripheral
    // appears ungated from CM7's perspective. Mirrors rlvgl
    // early_fmc_setup which writes both for FMC.
    // PLAT-02e §5.1 FIX: DMA2D is AHB3 bit 4 on the H7 (the old
    // ahb1enr bit-23 gate was the F4/F7 position — DMA2D was never
    // clocked). rlvgl bench AHB3ENR=0x1010 = FMC|DMA2D.
    RCC->ahb3enr    = RCC->ahb3enr
                    | ahb3enr::FMCEN | ahb3enr::MDMAEN | ahb3enr::DMA2DEN;
    RCC->c1_ahb3enr = RCC->c1_ahb3enr
                    | ahb3enr::FMCEN | ahb3enr::MDMAEN | ahb3enr::DMA2DEN;
    RCC->apb3enr    = RCC->apb3enr    | apb3enr::LTDCEN | apb3enr::DSIEN;
    // PLAT-02d FIX: LTDC/DSI also need the per-core (CM7) gate, exactly
    // like FMC above. Without RCC_C1_APB3ENR set, the LTDC is ungated
    // from CM7's perspective during normal run: register writes are
    // dropped and reads return 0 (proven on the bench — GCR read 0 while
    // running, 0x2220 only when the debugger halted the core). The DSI
    // PLL/regulator happen to come up regardless, but the LTDC needs
    // this to latch its config and scan the framebuffer.
    RCC->c1_apb3enr = RCC->c1_apb3enr | apb3enr::LTDCEN | apb3enr::DSIEN;

    // GPIO ports A,B,D,E,F,G,H,I,J,K (NOT C — no FMC/LTDC/DSI/touch
    // pin uses PCx in the PLAT-02b set; sub-phases that need PCx opt
    // in by setting the bit themselves).
    RCC->ahb4enr = RCC->ahb4enr
        | ahb4enr::bit(0)   // GPIOA
        | ahb4enr::bit(1)   // GPIOB
        | ahb4enr::bit(3)   // GPIOD
        | ahb4enr::bit(4)   // GPIOE
        | ahb4enr::bit(5)   // GPIOF
        | ahb4enr::bit(6)   // GPIOG
        | ahb4enr::bit(7)   // GPIOH
        | ahb4enr::bit(8)   // GPIOI
        | ahb4enr::bit(9)   // GPIOJ
        | ahb4enr::bit(10); // GPIOK

    // --- Step 8: Kernel-clock muxes ---------------------------------
    // FMCSEL=PLL2_R required: SDCR.SDCLK=0b01 only produces a usable
    // SDCLK divider when the kernel is PLL2_R; with the reset default
    // (HCLK), the chip outputs no clock at all and SDRAM is silent.
    {
        std::uint32_t d1ccipr = RCC->d1ccipr;
        d1ccipr = (d1ccipr & ~d1ccipr::FMCSEL_MASK) | d1ccipr::FMCSEL_PLL2_R;
        RCC->d1ccipr = d1ccipr;
    }

    // --- Step 9: Breadcrumb -----------------------------------------
    breadcrumb::write(breadcrumb::CLOCKS_LIVE_PRE_GPIO_SPLIT);
}

} // namespace lvglpp::disco::clocks
