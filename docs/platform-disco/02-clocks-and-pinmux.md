# 02 — Clocks, PLLs, GPIO Pin Mux

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAT-02b**.

## §0 Authority

- RCC register layout, PLL VCO ranges, divider constraints, kernel
  clock muxes, peripheral clock-enable bit positions: **RM0399 §8**
  ("Reset and clock control"). Authoritative for every numeric
  decision below.
- GPIO register layout (`MODER` / `OSPEEDR` / `PUPDR` / `AFRL` /
  `AFRH`), per-pin alternate-function tables: **RM0399 §14**
  ("General-purpose I/Os") + **DS12930 STM32H747XI datasheet
  Table 9** (per-pin AF tables). Authoritative.
- Power-domain transitions (LDO ↔ SMPS, VOS1) needed before
  SYSCLK can run at 400 MHz: **RM0399 §7** ("PWR"). Authoritative.
- Bring-up *intent* (which PLLs go to which peripheral, the
  PLL3ON gap fix, the breadcrumb pattern) mirrors
  `rlvgl/docs/disco-platform-guide/02-clocks-and-plls.md` and
  `rlvgl/docs/disco-platform-guide/04-gpio-pin-mux.md` (v0.2.0
  @ 79f730d). When the two diverge, **rlvgl is canonical** and
  this chapter is the bug.
- Underlying register-block discipline: PLAT-02 §5.5.
- Underlying toolchain / linker / vector table: PLAT-02a.

## §1 Purpose

PLAT-02b brings the CM7 clock tree up from the 64 MHz HSI default
to a production-shaped 400 MHz SYSCLK / 200 MHz HCLK with PLL3_R
=≈32 MHz on standby for the DSI/LTDC pixel clock domain, and
muxes every GPIO that the FMC / LTDC / DSI / I2C4 / USART1
peripherals will subsequently consume.

After this chapter lands, no further sub-phase has to argue with
the clock tree. SDRAM bring-up (PLAT-02c) reads SDCLK ≈ 75 MHz
from a stable PLL2_R; LTDC bring-up (PLAT-02d) finds the pixel
clock domain alive on the first read; DMA2D acceleration
(PLAT-02e) sees a 200 MHz AHB; touch + USART (PLAT-02f) find
their port clocks gated on.

This chapter does **not** bring up SDRAM, LTDC, DSI, or any
peripheral past clock-gate enable. It also does **not** enable
the D-cache / I-cache or program the MPU — those land in
PLAT-02c (where MPU coverage of `0xC000_0000` first matters) and
PLAT-02e (where D-cache transparency vs. DMA2D first matters).

## §2 Problem statement

Three classes of errors dominate H7 clock bring-up. Each is
inherited from rlvgl Vol II Ch 1 § "Gap gallery" and from the
disco BOOT.md notes; lvglpp gets them all without rediscovery.

1. **PLL3ON gap** (rlvgl Vol II Ch 2 § "The HAL/PAC gap"). Vendor
   HAL helpers configure PLL3 dividers and report success without
   actually setting `RCC_CR.PLL3ON` (bit 28). The first LTDC
   register read hangs because the LTDC AHB slave has no pixel
   clock. lvglpp uses no vendor HAL, so the failure mode looks
   different — a *missing* `PLL3ON` write is a typo we can lose
   to a refactor — but the cure is the same: write the bit
   explicitly and poll `PLL3RDY` (bit 29).

2. **Power posture before SYSCLK ramp**. The H7 boots with the
   internal LDO regulator in a low-voltage range. SYSCLK above
   ~200 MHz needs VOS1 (or VOS0 on rev Y/V); SMPS mode is
   required on the disco board because the LDO cannot supply the
   peak transient at 400 MHz. Skipping VOS1 → either the PLL
   never locks or HCLK access subtly corrupts. RM0399 §7.4.1.

3. **GPIO slew rate at FMC clock** (rlvgl Vol II Ch 4 § "Why
   VeryHigh speed is non-negotiable"). FMC clocks at ~75–100 MHz
   = 10 ns edges. AF12 in the default Low/Medium slew rate
   produces setup/hold violations on SDRAM data lines —
   intermittently corrupted reads, which look like memtest
   failures every Nth iteration. Every FMC pin (~50 pins) MUST
   be programmed to OSPEEDR=11 (VeryHigh).

## §3 Canonical glossary

- **HSE** — High-Speed External crystal. 25 MHz on the
  STM32H747I-DISCO (HARDWARE.md). PLL1/2/3 reference clock.
  Authoritative source: RM0399 §8.5.3.
- **SYSCLK** — System clock muxed onto the CM7 core. PLAT-02b
  freezes SYSCLK = PLL1_P_CK = 400 MHz. RM0399 §8.5.6.
- **HCLK** — AHB bus clock = SYSCLK / HPRE. PLAT-02b freezes
  HPRE = /2 → HCLK = 200 MHz. Drives DMA2D and the AXI
  interconnect. RM0399 §8.5.7.
- **PCLK1..4** — APB bus clocks = HCLK / Dx_PPREn. PLAT-02b
  freezes each /2 → 100 MHz. Most peripheral kernel clocks
  derive from these or from PLLn_R. RM0399 §8.5.7.
- **PLL1_P_CK / PLL1_Q_CK** — Outputs of PLL1. P drives SYSCLK;
  Q drives the SDMMC1/2 kernel clock. PLAT-02b freezes
  PLL1_Q_CK = 200 MHz (matches rlvgl, supports SDMMC bring-up
  in a future sub-phase). RM0399 §8.7.16–8.7.18.
- **PLL2_R_CK** — Output of PLL2 R divider. Drives the FMC
  kernel clock. PLAT-02b freezes 150 MHz; FMC SDCLK derives at
  /2 = 75 MHz (PLAT-02c uses this). RM0399 §8.7.19–8.7.21.
- **PLL3_R_CK** — Output of PLL3 R divider. Drives the LTDC
  pixel clock. PLAT-02b freezes ≈32 MHz (PLL3 N=160, M=5,
  R=25 → 32 MHz). PLAT-02d consumes this. RM0399 §8.7.22–8.7.24.
- **VOS1** — Voltage scaling level 1 (1.2 V regulator output;
  H7 rev V/Y supports VOS0 = 1.35 V for 480 MHz, but
  PLAT-02b stays at 400 MHz to mirror rlvgl). RM0399 §7.4.1.
- **SMPS** — Switched-mode power supply mode of the H7 internal
  regulator. Mandatory on the disco for SYSCLK ≥ 400 MHz
  because the LDO peak transient exceeds package limits.
  RM0399 §7.4.2.
- **AF12** — Alternate function 12. Maps PA0..PK15 to FMC
  signals (per DS12930 Table 9). Some LTDC/DSI/DMA2D pins use
  other AF numbers — covered case-by-case below.
- **`disco::clocks::Plan`** — Owned by this chapter. The frozen
  clock-target struct passed into `disco::clocks::init()`.
  Header: `examples/stm32h747i-disco/disco/clocks.hpp`. Mirrors
  rlvgl's `Rcc` builder call sequence as a flat aggregate.
- **`disco::clocks::init()`** — Owned by this chapter. Drives
  the bring-up sequence (power → HSE → PLLs → bus dividers →
  PLL3ON latch → peripheral clock-gate enables → breadcrumb).
  Aborts via `__BKPT(0)` on any timeout.
- **`disco::regs::Rcc` / `disco::regs::Pwr`** — Owned by this
  chapter. Typed `[[gnu::packed]]` register-block structs per
  PLAT-02 §5.5. Each field carries a
  `static_assert(offsetof(Block, field) == 0x..., "RM0399 §...")`
  line.
- **`disco::regs::GpioPort`** — Owned by this chapter. Typed
  GPIO register block. Eleven instances (`GPIOA..GPIOK`) are
  exposed via `MmioAddr<GpioPort>` constants.
- **`disco::pinmux::af12_high(port, pin)`** — Owned by this
  chapter. C++ analogue of rlvgl's `af12_high!` macro: writes
  `MODER=AF`, `OSPEEDR=VeryHigh`, `AFRL/AFRH=12` for one pin.
  Sub-phases use it for every FMC pin; LTDC and DSI sub-phases
  use the cousin helper `af_speed(port, pin, af, speed)`.
- **Breadcrumb word** — `0x3800_0300` (D3 SRAM). PLAT-02b
  writes `0xA11C_0005` ("pre-gpio-split", clocks live) and
  `0xA11C_0007` ("post-FMC-pins"). Mirrors rlvgl
  `BOOT.md`§ "Breadcrumb" word-for-word; values are the
  cross-language hand-off — a halted target should read the
  same magic regardless of which implementation is flashed.

## §4 Source-of-truth map

| Concept | Canonical owner | lvglpp mirror |
| --- | --- | --- |
| HSE / SYSCLK / HCLK / PCLK targets | rlvgl `02-clocks-and-plls.md` § "Build the PLL tree" | `disco::clocks::Plan` (this chapter §5.1). |
| PLL1/2/3 dividers + role assignment | RM0399 §8.7.16–8.7.24 | `disco::clocks::Plan::pll1/2/3` (§5.2). |
| PLL3ON raw write + PLL3RDY poll | rlvgl `02-clocks-and-plls.md` § "Force PLL3 on" | `disco::clocks::init()` step 6 (§5.5). |
| SMPS + VOS1 ordering | RM0399 §7.4.1–7.4.2 | `disco::clocks::init()` step 1 (§5.5). |
| Peripheral clock-gate enables (LTDC, DMA2D, DSI, FMC, GPIOA..K) | RM0399 §8.7.36–8.7.49 | `disco::clocks::init()` step 7 (§5.5). |
| FMC pin set (~50 pins, AF12 + VeryHigh) | rlvgl `04-gpio-pin-mux.md` § "The FMC pin set" + DS12930 Table 9 | `disco::pinmux::mux_fmc_pins()` (§5.6). |
| `af12_high!` idiom | rlvgl `04-gpio-pin-mux.md` § "The `af12_high!` macro" | `disco::pinmux::af12_high()` (§5.6). |
| Breadcrumb magic at `0x3800_0300` | rlvgl `BOOT.md` § "Breadcrumb" | `disco::breadcrumb::write()` (§5.7). |

## §5 Frozen decisions

### §5.1 Frozen clock targets — **Standards Action**

| Domain | Frozen value | Source |
| --- | --- | --- |
| HSE | **25 MHz** | STM32H747I-DISCO HARDWARE.md (Y3 crystal). |
| SYSCLK = PLL1_P_CK | **400 MHz** | rlvgl BOOT.md § "Clocks (Current)". |
| HCLK | **200 MHz** | HPRE = /2. |
| PCLK1 / PCLK2 / PCLK3 / PCLK4 | **100 MHz** each | D{1,2,3}_PPREn = /2. |
| PLL1_Q_CK | **200 MHz** | SDMMC kernel (PLAT-02-followup). |
| PLL2_R_CK | **150 MHz** | FMC kernel; SDCLK = /2 = 75 MHz (PLAT-02c). |
| PLL3_R_CK | **≈32 MHz** | LTDC pixel clock (PLAT-02d). |
| Power: VOS | **VOS1** (1.2 V) | RM0399 §7.4.1. |
| Power: regulator | **SMPS** (D2DCM=01) | RM0399 §7.4.2; mandatory at SYSCLK = 400 MHz. |

The values above are byte-for-byte rlvgl parity. Changing one is
a Standards Action requiring a family-chapter amendment first
(per CLAUDE.md § "Cross-language change ordering"). Going to 480
MHz is explicitly out of scope for PLAT-02b — see §11.

### §5.2 PLL configuration constants — **Standards Action**

The exact PLL{N,M,P,Q,R} dividers below produce the §5.1 outputs
from a 25 MHz HSE. They mirror rlvgl's stm32h7xx-hal "Iterative"
strategy outcome word-for-word.

| PLL | M | N | P | Q | R | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| PLL1 | 5 | 160 | 2 | 4 | — | VCO = 800 MHz; P→SYSCLK 400, Q→SDMMC 200. |
| PLL2 | 5 | 60  | — | — | 2 | VCO = 300 MHz; R→FMC 150. |
| PLL3 | 5 | 160 | — | — | 25 | VCO = 800 MHz; R→LTDC 32. |

VCO range / RGE / VCOSEL bits per RM0399 §8.7.5–8.7.7 are
selected to match the VCO column above (medium-VCO range for
PLL1 and PLL3; wide-VCO range for PLL2 since 300 MHz < 420 MHz).

### §5.3 `disco::regs::Rcc` field set — **Standards Action**

The RCC typed register block lives at
`examples/stm32h747i-disco/disco/regs/rcc.hpp` and mirrors RM0399
§8.7's table layout. Mandatory shape per PLAT-02 §5.5:

```cpp
struct alignas(4) Rcc {
    volatile std::uint32_t cr;            // 0x000 — RM0399 §8.7.2
    volatile std::uint32_t hsicfgr;       // 0x004 — §8.7.3
    volatile std::uint32_t crrcr;         // 0x008
    volatile std::uint32_t csicfgr;       // 0x00C
    volatile std::uint32_t cfgr;          // 0x010 — §8.7.5
    std::uint32_t           _rsvd_014;     // 0x014
    volatile std::uint32_t d1cfgr;        // 0x018 — §8.7.6 (HPRE/D1PPRE/D1CPRE)
    volatile std::uint32_t d2cfgr;        // 0x01C — §8.7.7 (D2PPRE1/D2PPRE2)
    volatile std::uint32_t d3cfgr;        // 0x020 — §8.7.8 (D3PPRE)
    std::uint32_t           _rsvd_024;
    volatile std::uint32_t pllckselr;     // 0x028 — §8.7.9
    volatile std::uint32_t pllcfgr;       // 0x02C — §8.7.10
    volatile std::uint32_t pll1divr;      // 0x030 — §8.7.11
    volatile std::uint32_t pll1fracr;     // 0x034
    volatile std::uint32_t pll2divr;      // 0x038
    volatile std::uint32_t pll2fracr;     // 0x03C
    volatile std::uint32_t pll3divr;      // 0x040
    volatile std::uint32_t pll3fracr;     // 0x044
    std::uint32_t           _rsvd_048;
    volatile std::uint32_t d1ccipr;       // 0x04C — §8.7.13 (FMCSEL etc.)
    volatile std::uint32_t d2ccip1r;      // 0x050
    volatile std::uint32_t d2ccip2r;      // 0x054
    volatile std::uint32_t d3ccipr;       // 0x058
    /* … abridged. Full layout per RM0399 §8.7 Table 38;
     * AHB3ENR @ 0x0D4, AHB1ENR @ 0x0D8, … */
};
static_assert(offsetof(Rcc, cr)        == 0x000, "RM0399 §8.7.2");
static_assert(offsetof(Rcc, cfgr)      == 0x010, "RM0399 §8.7.5");
static_assert(offsetof(Rcc, d1cfgr)    == 0x018, "RM0399 §8.7.6");
static_assert(offsetof(Rcc, pllckselr) == 0x028, "RM0399 §8.7.9");
static_assert(offsetof(Rcc, pllcfgr)   == 0x02C, "RM0399 §8.7.10");
static_assert(offsetof(Rcc, pll1divr)  == 0x030, "RM0399 §8.7.11");
static_assert(offsetof(Rcc, d1ccipr)   == 0x04C, "RM0399 §8.7.13");

// mmio: owned by RM0399 §8.7; never freed.
inline constexpr MmioAddr<Rcc> RCC{0x5802'4400u};
```

Any RCC offset accessed by a sub-phase MUST appear in the field
set with a matching `static_assert`. Hand-offset pointer
arithmetic (`(volatile uint32_t*)0x5802_4470`) is a discipline
violation per PLAT-02 §5.5 #1.

### §5.4 `disco::regs::GpioPort` + `Pwr` field sets — **Standards Action**

GPIO ports are uniform — eleven `MmioAddr<GpioPort>` constants
(`GPIOA..GPIOK`) at `0x5802_0000 + 0x400 × index`:

```cpp
struct alignas(4) GpioPort {
    volatile std::uint32_t moder;     // 0x00 — RM0399 §14.4.1
    volatile std::uint32_t otyper;    // 0x04 — §14.4.2
    volatile std::uint32_t ospeedr;   // 0x08 — §14.4.3
    volatile std::uint32_t pupdr;     // 0x0C — §14.4.4
    volatile std::uint32_t idr;       // 0x10 — §14.4.5
    volatile std::uint32_t odr;       // 0x14 — §14.4.6
    volatile std::uint32_t bsrr;      // 0x18 — §14.4.7
    volatile std::uint32_t lckr;      // 0x1C — §14.4.8
    volatile std::uint32_t afrl;      // 0x20 — §14.4.9 (pins 0..7)
    volatile std::uint32_t afrh;      // 0x24 — §14.4.10 (pins 8..15)
};
static_assert(offsetof(GpioPort, ospeedr) == 0x08, "RM0399 §14.4.3");
static_assert(offsetof(GpioPort, afrl)    == 0x20, "RM0399 §14.4.9");
static_assert(offsetof(GpioPort, afrh)    == 0x24, "RM0399 §14.4.10");
inline constexpr MmioAddr<GpioPort> GPIOA{0x5802'0000u};
inline constexpr MmioAddr<GpioPort> GPIOB{0x5802'0400u};
// …
inline constexpr MmioAddr<GpioPort> GPIOK{0x5802'2800u};
```

PWR is small enough to be exhaustive:

```cpp
struct alignas(4) Pwr {
    volatile std::uint32_t cr1;        // 0x00 — RM0399 §7.6.1 (LPDS, PVDE, …)
    volatile std::uint32_t csr1;       // 0x04 — §7.6.2 (AVDO, PVDO, …)
    volatile std::uint32_t cr2;        // 0x08 — §7.6.3 (BREN, MONEN, …)
    volatile std::uint32_t cr3;        // 0x0C — §7.6.4 (SCUEN, LDOEN, SMPSEN, …)
    volatile std::uint32_t cpucr;      // 0x10 — §7.6.5
    std::uint32_t           _rsvd_14;
    volatile std::uint32_t d3cr;       // 0x18 — §7.6.7 (VOS bits + VOSRDY)
    std::uint32_t           _rsvd_1C;
    volatile std::uint32_t wkupcr;     // 0x20
    volatile std::uint32_t wkupfr;     // 0x24
    volatile std::uint32_t wkupepr;    // 0x28
};
static_assert(offsetof(Pwr, cr3)  == 0x0C, "RM0399 §7.6.4");
static_assert(offsetof(Pwr, d3cr) == 0x18, "RM0399 §7.6.7");
inline constexpr MmioAddr<Pwr> PWR{0x5802'4800u};
```

### §5.5 `disco::clocks::init()` sequence — **Standards Action**

`disco::clocks::init(const Plan&)` runs the following ten-step
sequence. Each step is fixed in order; reordering across the
PLL3ON latch (step 6) is a discipline violation. Timeouts are
expressed in HSI cycles (≈64 MHz at boot) so they remain valid
even though the SYSCLK source changes mid-sequence.

1. **Power posture.** Set `PWR.CR3.SMPSEN=1, LDOEN=0, SCUEN=1`
   (SMPS-only path). Set `PWR.D3CR.VOS = 0b11` (VOS1). Poll
   `PWR.D3CR.VOSRDY` (bit 13) until set, with a 100k-cycle
   timeout → `__BKPT(0)` on miss.
2. **Enable HSE.** `RCC.CR.HSEON = 1`; poll `HSERDY` (bit 17).
3. **Configure PLLs.** Write `PLLCKSELR.PLLSRC = 0b10` (HSE
   source) + `DIVMx` per §5.2; write `PLLCFGR` for VCO range
   and DIVxEN enables; write `PLLnDIVR` for N/P/Q/R per §5.2.
4. **Latch PLL1, PLL2.** `RCC.CR.PLL1ON = 1`; poll `PLL1RDY`.
   `RCC.CR.PLL2ON = 1`; poll `PLL2RDY`.
5. **Bus prescalers.** Write `RCC.D1CFGR.D1CPRE = /1`,
   `HPRE = /2`, `D1PPRE = /2`. Write `D2CFGR.D2PPRE1/2 = /2`.
   Write `D3CFGR.D3PPRE = /2`. Switch SYSCLK source:
   `RCC.CFGR.SW = 0b011` (PLL1); poll `CFGR.SWS`.
6. **Force PLL3 on.** `RCC.CR.PLL3ON = 1`; poll `PLL3RDY`
   (bit 29). **This is the rlvgl PLL3ON gap fix** — never
   collapse this step into a vendor builder call. Comment in
   code MUST cite rlvgl `02-clocks-and-plls.md` § "Force PLL3
   on" so a future refactor cannot drop it silently.
7. **Peripheral clock-gates.** Set the bits below in their
   respective enable registers. RM0399 cite for each in the
   header comment:
   - `RCC.AHB3ENR.FMCEN = 1`        (§8.7.36 bit 12)
   - `RCC.AHB3ENR.MDMAEN = 1`       (§8.7.36 bit 0)
   - `RCC.AHB1ENR.DMA2DEN = 1`      (§8.7.38 bit 23)
   - `RCC.APB3ENR.LTDCEN = 1`       (§8.7.43 bit 3)
   - `RCC.APB3ENR.DSIEN  = 1`       (§8.7.43 bit 4)
   - `RCC.AHB4ENR.GPIOAEN..GPIOKEN` for ports A,B,D,E,F,G,H,I,J,K
     (§8.7.41 bits 0..10; **not GPIOC** unless a sub-phase opts in).
8. **Kernel-clock muxes.** `RCC.D1CCIPR.FMCSEL = 0b10`
   (PLL2_R) — RM0399 §8.7.13. LTDC pixel clock is hard-wired to
   PLL3_R and needs no mux write.
9. **Breadcrumb.** `*((volatile uint32_t*)0x3800_0300) =
   0xA11C_0005` — "pre-gpio-split" magic. Mirrors rlvgl
   `02-clocks-and-plls.md` § "Breadcrumb to D3 SRAM".
10. **Return.** `init()` returns `void`. Failure is
    catastrophic by definition — the bring-up cannot recover —
    so the only "error" exit is `__BKPT(0)` from a timeout
    above.

The sequence MUST run before any GPIO mux call, any FMC access,
any LTDC access, or any sub-phase entry. The smoke target's
`main()` (PLAT-02a) does **not** call it; the first sub-phase
that needs a peripheral does.

### §5.6 `disco::pinmux` helpers — **Specification Required**

Three helpers are owned by this chapter. All take a typed
`MmioAddr<GpioPort>` and a pin index (`std::uint8_t`, 0..15);
out-of-range pins are a compile-time error via a `requires`
clause.

```cpp
namespace disco::pinmux {

// AF + slew rate for one pin. Writes MODER, OSPEEDR, AFRL/AFRH.
void af_speed(MmioAddr<GpioPort> port, std::uint8_t pin,
              std::uint8_t af, Speed speed);

// Convenience: AF12 + VeryHigh. Mirrors rlvgl's `af12_high!`.
inline void af12_high(MmioAddr<GpioPort> port, std::uint8_t pin) {
    af_speed(port, pin, 12, Speed::VeryHigh);
}

// Floating input. Used for FT5336 INT (PK7 in PLAT-02f).
void floating_input(MmioAddr<GpioPort> port, std::uint8_t pin);

}
```

`Speed` is `enum class Speed : std::uint8_t { Low=0, Medium=1,
High=2, VeryHigh=3 }` — bit pattern matches `OSPEEDR`.

### §5.7 FMC + display pin set — **Specification Required**

`disco::pinmux::mux_fmc_pins()` calls `af12_high()` for every
FMC pin in the table below. The set mirrors rlvgl `04-gpio-pin-mux.md`
§ "The FMC pin set" + HARDWARE.md § "Hardware Pin Summary (FMC
SDRAM)":

```
Address (A0..A11):     PF0..PF5, PF11..PF15, PG0..PG2
Bank address (BA0/1):  PG4, PG5
Clock / enable:        PG8 (SDCLK), PH7 (SDCKE1), PH6 (SDNE1)
Control:               PF11 (SDNRAS), PG15 (SDNCAS), PH5 (SDNWE)
Byte lanes (NBL0..3):  PE0, PE1, PI4, PI5
Data (D0..D31):        PD0/1/8/9/10/14/15, PE7..PE15,
                       PH8..PH15, PI0..PI3/6/7/9/10
```

LTDC and DSI pin sets are **deferred** to PLAT-02d (the chapter
that owns the panel bring-up). The AF numbers for LTDC pins are
mostly AF14 (not AF12), so they don't fit the `af12_high` helper
unmodified — PLAT-02d will use `af_speed` directly.

USART1 (PA9/PA10 at AF7), I2C4 (PD12/PD13 at AF4), and FT5336
INT (PK7 floating input) belong to PLAT-02f.

### §5.8 Breadcrumb codes — **Standards Action**

The lvglpp side mirrors rlvgl's breadcrumb codes word-for-word
so a halted target's D3 SRAM dump diffs cleanly across the two
implementations:

| Address | Magic | Meaning |
| --- | --- | --- |
| `0x3800_0300` | `0xA11C_0005` | Clocks live, GPIO ports unsplit. PLAT-02b step 9. |
| `0x3800_0300` | `0xA11C_0007` | FMC pins muxed. End of `mux_fmc_pins()`. |
| `0x3800_0300` | `0xA11C_0009` | SDRAM init complete (PLAT-02c). |
| `0x3800_0300` | `0xA11C_000B` | Panel up (PLAT-02d). |
| `0x3800_0300` | `0xA11C_000D` | DMA2D first transfer complete (PLAT-02e). |
| `0x3800_0300` | `0xA11C_000F` | First touch event dispatched (PLAT-02f). |

Adding or renumbering a code is a Standards Action requiring a
family-chapter amendment.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `Rcc::freeze()` builder.** The lvglpp `disco::clocks::init()`
  flat-aggregate shape is *adapted* — Rust's typed builder gives
  rlvgl a compile-time guarantee that PLLs are configured before
  `freeze()`; lvglpp gets the same guarantee from the §5.5 fixed
  step order plus the typed register-block discipline (PLAT-02 §5.5).
  Numerical outputs are byte-for-byte identical.
- **PLAT-02 §5.5 register-block discipline.** §5.3 / §5.4 land
  the first concrete `MmioAddr<...>` typed-block instances. The
  pattern repeats verbatim for FMC (PLAT-02c), LTDC + DSI
  (PLAT-02d), DMA2D (PLAT-02e), USART1 + I2C4 (PLAT-02f).
- **PLAT-02a `Reset_Handler`.** Reset still does **not** call
  `disco::clocks::init()`. The smoke target's `main()` runs at
  HSI 64 MHz with no peripheral clock gates open. Sub-phases
  beyond PLAT-02a opt in by calling `init()` from `main()`
  before any peripheral access.
- **rlvgl `cpu_stats` / DWT cycle counter** (BOOT.md). DWT
  cycle-counter setup is **not** part of PLAT-02b. It belongs
  to a follow-up profiling sub-phase; the bring-up sequence
  uses HSI-cycle-counted `for` loops for timeouts (rough but
  monotonic and bounded).
- **Cache + MPU programming.** Deferred to PLAT-02c (MPU
  region for SDRAM at `0xC000_0000`) and PLAT-02e (D-cache
  clean-by-MVAC before DMA2D). PLAT-02b leaves both off; FLASH
  + DTCM access is correct without either.

## §11 Non-goals

- **480 MHz SYSCLK** (VOS0 on rev V/Y). PLAT-02b stays at 400
  MHz to mirror rlvgl. Going to 480 MHz is a follow-up
  Standards-Action amendment with its own thermal + DMA2D
  timing review.
- **DWT cycle-counter** init for profiling. Deferred.
- **MPU programming.** PLAT-02c.
- **Cache enable.** PLAT-02e.
- **CM4 clock domain.** PLAT-02 §5.3 freezes CM7-only.
- **Audio kernel clocks** (SAI1 / SAI4 for the WM8994 + PDM
  mic). Out of scope; the audio prong belongs to a follow-up
  family.
- **QSPI kernel clock + errata 2.8.5 raw write to D1CCIPR.**
  Belongs to the deferred QSPI sub-phase, not PLAT-02b.
- **Runtime clock-frequency change.** `disco::clocks::init()`
  runs once, at boot. Dynamic frequency scaling is out of scope.
- **HSE bypass / external clock generator paths**. The disco
  carries a 25 MHz Y3 crystal; that's the only configuration
  PLAT-02b supports.

## §12 Acceptance checklist

- [ ] `examples/stm32h747i-disco/disco/regs/{rcc,pwr,gpio}.hpp`
      ship the typed register-block structs per §5.3 + §5.4
      with `static_assert(offsetof(...) == 0x..., "RM0399 §...")`
      lines on every documented offset.
- [ ] `examples/stm32h747i-disco/disco/clocks.hpp` exposes
      `disco::clocks::Plan` (with the §5.1 defaults baked in)
      and `disco::clocks::init(const Plan&)`.
- [ ] `examples/stm32h747i-disco/disco/pinmux.hpp` exposes
      `af_speed`, `af12_high`, `floating_input`, and
      `mux_fmc_pins()`.
- [ ] `disco::clocks::init()` follows the §5.5 ten-step order
      verbatim. Step 6 carries a code comment citing rlvgl
      `02-clocks-and-plls.md` § "Force PLL3 on".
- [ ] A new smoke target `lvglpp_stm32h747i_disco_clocks` calls
      `disco::clocks::init(Plan{})` from `main()`, then writes
      breadcrumb `0xA11C_0007`, then loops on WFE.
- [ ] On hardware: `probe-rs run` → halt at the WFE → reads:
      - `RCC.CR @ 0x5802_4400` AND `0x3000_0000` == `0x3000_0000`
        (PLL3ON + PLL3RDY both set).
      - `RCC.CFGR.SWS @ 0x5802_4410[5:3]` == `0b011` (PLL1 selected).
      - `PWR.D3CR.VOS @ 0x5802_4818[15:14]` == `0b11` (VOS1).
      - `*((uint32_t*)0x3800_0300)` == `0xA11C_0007` after a
        full bring-up; == `0xA11C_0005` if the run halts before
        `mux_fmc_pins()`.
- [ ] On hardware (FMC pins): `GPIOF.MODER` pins 0..5 + 11..15
      read `0b10` (AF); `GPIOF.OSPEEDR` same pins read `0b11`
      (VeryHigh); `GPIOF.AFRL/AFRH` same pins read nibble
      `0xC` (AF12). Same for the GPIOG / GPIOH / GPIOI / GPIOD /
      GPIOE pins listed in §5.7.
- [ ] Build under `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake`
      stays inside the §12 sanity envelope: `text` < 8 KiB,
      `data` == 0, `bss` < 1 KiB. (Bigger means a vendor HAL
      crept in.)
- [ ] `platform/STATUS.md` change-log entry recording PLAT-02b
      landing, dated.

## §13 Files cited

- `rlvgl/docs/disco-platform-guide/02-clocks-and-plls.md` (v0.2.0
  @ 79f730d).
- `rlvgl/docs/disco-platform-guide/04-gpio-pin-mux.md` (v0.2.0
  @ 79f730d).
- `rlvgl/examples/stm32h747i-disco/{HARDWARE,BOOT}.md`.
- `rlvgl/examples/stm32h747i-disco/src/main.rs` L1569–1596
  (PLL tree + PLL3ON), L1606–1608 (GPIO split),
  L1643–1707 (FMC pin mux).
- STMicroelectronics RM0399 §7 (PWR), §8 (RCC), §14 (GPIO).
- STMicroelectronics DS12930 (STM32H747XI datasheet) Table 9.
- `lvglpp/docs/platform-disco/00-platform-disco.md` (family
  chapter — §5.5 register-block discipline, §5.6 embedded
  posture, §5.1 PLAT-02b acceptance).
- `lvglpp/docs/platform-disco/01-toolchain-and-reset.md`
  (PLAT-02a — toolchain seam consumed by PLAT-02b).

## §14 Unblocks

- PLAT-02c SDRAM bring-up gets a stable `PLL2_R = 150 MHz` and
  the FMC pin set already muxed at AF12 + VeryHigh — the
  chapter can focus purely on the JEDEC init sequence.
- PLAT-02d LTDC/DSI gets `PLL3_R ≈ 32 MHz` already locked
  (`PLL3ON` set, `PLL3RDY` high). The first LTDC register read
  no longer hangs.
- PLAT-02e DMA2D gets `HCLK = 200 MHz` and `RCC.AHB1ENR.DMA2DEN
  = 1` already — the chapter can focus on transfer geometry +
  ERIF holdoff.
- PLAT-02f USART1 + I2C4 + FT5336 INT each get a port-clock
  gate already open; only their per-peripheral pins need
  muxing in that chapter.
- The cross-language hand-off is concrete: a probe-rs reader
  on a halted target reads identical breadcrumb codes whether
  rlvgl or lvglpp is flashed.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Frozen clock
  targets (§5.1), PLL configuration constants (§5.2), RCC + PWR
  + GPIO typed register-block field sets (§5.3, §5.4),
  ten-step `init()` sequence (§5.5), pinmux helpers (§5.6),
  FMC pin set (§5.7), breadcrumb codes (§5.8) all frozen.
