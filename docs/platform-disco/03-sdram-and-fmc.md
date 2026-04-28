# 03 — SDRAM & FMC

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAT-02c**.

## §0 Authority

- FMC SDRAM controller register layout (SDCR1/2, SDTR1/2, SDCMR,
  SDRTR, SDSR), JEDEC command encoding (CTB1/CTB2, MODE, NRFS,
  MRD), bank-2 hardware mapping at `0xD000_0000`: **RM0399 §22**
  ("Flexible memory controller (FMC)"). Authoritative.
- IS42S32800J-6BLI datasheet (32 Mbit × 32, 4 banks, CAS=3 @ 100 MHz):
  ISSI datasheet, authoritative for timing constants.
- Cortex-M7 MPU register layout (MPU_CTRL, MPU_RNR, MPU_RBAR,
  MPU_RASR): **ARM ARM (DDI 0403E.e) §B3.5**. Authoritative.
- Bring-up *intent* (PAC SDCR/SDTR raw writes before HAL clock
  changes, JEDEC sequence ordering, MPU coverage gate) mirrors
  `rlvgl/docs/disco-platform-guide/03-sdram-and-fmc.md` and the
  `pac_sdram_init` feature path in `rlvgl/.../main.rs`
  (v0.2.0 @ 79f730d). When the two diverge, **rlvgl is canonical**
  and this chapter is the bug.
- Underlying clock tree + GPIO mux: PLAT-02b.
- Underlying typed register-block discipline: PLAT-02 §5.5.

## §1 Purpose

PLAT-02c brings the external 32 MB SDRAM up at `0xD000_0000`,
read/write-stable, with MPU coverage so the first read does not
BusFault. Together with PLAT-02b's clock tree this delivers the
substrate that PLAT-02d will park framebuffers in.

After this chapter lands, no further sub-phase has to argue with
the FMC. LTDC bring-up (PLAT-02d) writes pixel words to
`0xD000_0000+` via simple `volatile uint32_t*` and trusts that
each survives a refresh interval. DMA2D (PLAT-02e) treats SDRAM
as a memory-to-memory destination identical to internal SRAM
(modulo D-cache, which lands in PLAT-02e).

## §2 Problem statement

Three failure modes dominate H7 SDRAM bring-up. Each is inherited
from rlvgl Vol II Ch 3 + BOOT.md.

1. **MPU not programmed** — the H7 boots with the MPU disabled.
   With PRIVDEFENA off (the reset default), the default memory
   map at `0xD000_0000` is "device, never cached, fault-on-access"
   in some configurations. The first read MemManage/BusFaults
   even though SDRAM is technically alive. Cure: program MPU
   region 0 to cover `0xD000_0000` as Normal memory, shareable,
   non-cacheable, full-access, then enable MPU with PRIVDEFENA.
2. **JEDEC sequence ordering** — `SDCMR` writes that issue
   commands to the SDRAM chip MUST be done in a specific order
   with `SDSR.BUSY` polled between each. Skipping the
   ≥100 µs power-up delay between Clock-Enable and Precharge,
   or skipping any of the 8 auto-refresh cycles, leaves the
   chip in an undefined state — first reads return random
   bits. Cure: `clock_enable → delay 100 µs → precharge_all →
   auto_refresh ×8 → load_mode_register`, polling BUSY between
   each.
3. **PAC SDTR off-by-one in vendor crates** (rlvgl Vol II Ch 1
   §2). lvglpp uses no vendor PAC, so this gap doesn't bite us
   directly — but the mitigation rlvgl adopted (raw address
   writes with the offsets `0x148`/`0x14C` for SDTR1/SDTR2)
   doubles as our discipline check: every offset MUST appear in
   `disco/regs/fmc.hpp` with a `static_assert(offsetof(...))`
   line so a future refactor cannot quietly break the layout.

## §3 Canonical glossary

- **Bank 2** — FMC SDRAM bank 2. Hardware-mapped at
  `0xD000_0000` (32 MB window). Disco wires its IS42S32800J to
  the SDCKE1/SDNE1 chip-select pair; bank 1 (`0xC000_0000`) is
  unused. RM0399 §22.6.
- **`disco::sdram::init()`** — Owned by this chapter. Programs
  SDCR1/2 + SDTR1/2, runs the JEDEC sequence, programs SDRTR
  refresh, programs MPU region 0 to cover bank 2, enables MPU.
  Writes breadcrumb `0xA11C_0009` at completion. Aborts via
  `__BKPT(0)` on timeout.
- **`disco::regs::Fmc`** — Owned by this chapter. Typed register
  block at `0x5200_4000`. Per PLAT-02 §5.5, every offset used
  carries a `static_assert(offsetof(...) == 0x..., "RM0399 §...")`
  line.
- **`disco::regs::Mpu`** — Owned by this chapter. Typed register
  block for the Cortex-M7 MPU at `0xE000_ED90`. ARM ARM §B3.5.
- **JEDEC command** — One of `clock_enable`, `precharge_all`,
  `auto_refresh`, `load_mode_register`, `self_refresh`,
  `power_down`, `normal_mode`. Issued by writing SDCMR with the
  MODE field, the bank-target bit (CTB2 for bank 2), the
  auto-refresh count (NRFS), and the mode-register data (MRD).
  RM0399 §22.9.5.3.
- **Mode register data** — The 13-bit value programmed into the
  SDRAM's mode register via the load-mode-register command.
  PLAT-02c uses `0x0230` (CAS=3, burst-length=2, sequential,
  single-write burst). Mirrors rlvgl
  `SDRAM_MODE_REGISTER`.
- **Refresh count** — `SDRTR.COUNT[13:1]` value for the refresh
  timer. At SDCLK = 75 MHz with a 64 ms / 8192 row spec → 7.81 µs
  inter-refresh interval → 7.81e-6 × 75e6 ≈ 585 cycles, less a
  margin → **566** (mirrors rlvgl BOOT.md). Programmed value is
  `566 << 1` (the LSB of SDRTR is reserved).

## §4 Source-of-truth map

| Concept | Canonical owner | lvglpp mirror |
| --- | --- | --- |
| FMC SDRAM register layout (offsets 0x140–0x158) | RM0399 §22.9.5 | `disco::regs::Fmc` (§5.1). |
| SDCR1/2 field set (NC/NR/MWID/NB/CAS/SDCLK/RBURST/RPIPE) | RM0399 §22.9.5.1 + rlvgl `main.rs` L1064–1078 | `disco::sdram::init()` (§5.2). |
| SDTR1/2 timing constants (cycle-1 encoding) | IS42S32800J datasheet + rlvgl `main.rs` L1099–1117 | `disco::sdram::init()` (§5.3). |
| JEDEC sequence + SDSR.BUSY poll | RM0399 §22.9.5.3 + rlvgl `main.rs` L1040–1044 | `disco::sdram::init()` (§5.4). |
| MPU layout + region attributes | ARM ARM DDI 0403E.e §B3.5 | `disco::regs::Mpu` + `disco::sdram::init()` MPU step (§5.5). |
| Refresh count derivation | RM0399 §22.9.5.4 + rlvgl BOOT.md | §5.4 step 6. |

## §5 Frozen decisions

### §5.1 `disco::regs::Fmc` field set — **Standards Action**

The FMC base is `0x5200_4000`. The SDRAM-specific register block
starts at offset `0x140`. Every offset used by `disco::sdram::init()`
MUST appear in the typed block with a matching `static_assert`:

```cpp
struct alignas(4) Fmc {
    // … BCRx/BTRx/PCRx/etc. for NOR/PSRAM/NAND modes elided —
    // PLAT-02c only uses the SDRAM register set.
    std::uint8_t           _rsvd_000[0x140];
    volatile std::uint32_t sdcr1;   // 0x140 — RM0399 §22.9.5.1
    volatile std::uint32_t sdcr2;   // 0x144
    volatile std::uint32_t sdtr1;   // 0x148 — RM0399 §22.9.5.2
    volatile std::uint32_t sdtr2;   // 0x14C
    volatile std::uint32_t sdcmr;   // 0x150 — RM0399 §22.9.5.3
    volatile std::uint32_t sdrtr;   // 0x154 — RM0399 §22.9.5.4
    volatile std::uint32_t sdsr;    // 0x158 — RM0399 §22.9.5.5
};
static_assert(offsetof(Fmc, sdcr1) == 0x140, "RM0399 §22.9.5.1");
static_assert(offsetof(Fmc, sdtr1) == 0x148, "RM0399 §22.9.5.2");
static_assert(offsetof(Fmc, sdcmr) == 0x150, "RM0399 §22.9.5.3");
static_assert(offsetof(Fmc, sdrtr) == 0x154, "RM0399 §22.9.5.4");
static_assert(offsetof(Fmc, sdsr)  == 0x158, "RM0399 §22.9.5.5");

// `mmio: owned by RM0399 §22.9; never freed.`
inline constexpr MmioAddr<Fmc> FMC{0x5200'4000u};
```

PLAT-02c does **not** wrap BCR/BTR (NOR-flash side); subsequent
phases (e.g. a hypothetical NOR-flash sub-phase) would extend
the block.

### §5.2 SDCR1 / SDCR2 values — **Standards Action**

| Field | SDCR1 | SDCR2 | Source |
| --- | --- | --- | --- |
| NC (column bits) | `01` (9) | `01` | IS42S32800J = 9-col. |
| NR (row bits) | `01` (12) | `01` | IS42S32800J = 12-row. |
| MWID (bus width) | `10` (32) | `10` | IS42S32800J = 32-bit. |
| NB (banks) | `1` (4) | `1` | IS42S32800J = 4 banks. |
| CAS (latency) | `11` (3 cycles) | `11` | IS42S32800J at 100 MHz timing → CAS=3. |
| WP (write-protect) | `0` | `0` | RW. |
| SDCLK[11:10] | `01` (silicon-required `Reserved` value; rlvgl Vol II Ch 3 § 2 confirms) | — | SDCR1 only. |
| RBURST[12] | `1` | — | SDCR1 only. |
| RPIPE[14:13] | `00` | — | SDCR1 only. |

Resulting words (per byte-for-byte rlvgl parity):
- `SDCR1 = 0x0000_15D5` — same field set as SDCR2 plus
  SDCLK + RBURST.
- `SDCR2 = 0x0000_01D5` — NC/NR/MWID/NB/CAS only.

Bank 1 fields below SDCLK/RBURST/RPIPE are still required because
RM0399 §22.9.5.1 says the chip latches certain fields (NC, NR,
MWID, NB, CAS, WP) from whichever SDCR was written last during
controller init — programming them in both is safe and matches
rlvgl.

### §5.3 SDTR1 / SDTR2 timing values — **Standards Action**

Programmed values are `cycles − 1` per RM0399 §22.9.5.2:

| Timing | Cycles | Field | Bits | SDTRx |
| --- | --- | --- | --- | --- |
| TMRD | 2 | `1` | 0..3 | SDTR2 |
| TXSR | 7 | `6` | 4..7 | SDTR2 |
| TRAS | 5 | `4` | 8..11 | SDTR2 |
| TRC | 7 | `6` | 12..15 | SDTR1 |
| TWR | 2 | `1` | 16..19 | SDTR2 |
| TRP | 2 | `1` | 20..23 | SDTR1 |
| TRCD | 2 | `1` | 24..27 | SDTR2 |

Resulting words (byte-for-byte rlvgl):
- `SDTR1 = 0x0010_6000` — TRP + TRC.
- `SDTR2 = 0x0101_0461` — TRCD + TWR + TRAS + TXSR + TMRD.

### §5.4 JEDEC sequence + refresh — **Standards Action**

`disco::sdram::init()` runs the following steps in order. SDSR.BUSY
(bit 5) MUST be polled between each `SDCMR` write; a stuck BUSY
after 1M HSI cycles is a `__BKPT(0)` trap.

| Step | Command | `SDCMR` value | Notes |
| --- | --- | --- | --- |
| 1 | Clock Enable | `0x09` (MODE=001, CTB2=1) | After step 1, the SDRAM has SDCLK present. |
| 2 | (delay ≥100 µs) | — | RM0399 §22.6.5 power-up wait. ≈40k cycles at 400 MHz. |
| 3 | Precharge All | `0x0A` (MODE=010, CTB2=1) | Drives all rows to idle. |
| 4 | Auto-Refresh ×8 | `0xEB` (MODE=011, NRFS=7, CTB2=1) | NRFS field is `count − 1`. |
| 5 | Load Mode Register | `0x4_600C` (MODE=100, MRD=0x230, CTB2=1) | Mode reg = 0x230 (CAS=3, BL=2). |
| 6 | Refresh timer | write `SDRTR = 566 << 1 = 0x46C` | Enables auto-refresh; after this SDRAM is live. |

Bit-for-bit derivation of step 5: MODE[2:0]=`100`=0x4; CTB2 (bit 3)=0x8;
MRD[21:9]=`0x230 << 9`=0x46000. Sum = `0x4_600C`. NRFS field is unused
for load-mode (bits 5..8 are zero).

### §5.5 MPU programming — **Standards Action**

Cortex-M7 MPU at `0xE000_ED90`. PLAT-02c uses **region 0** to
cover bank-2 SDRAM. Every other region is left disabled; PRIVDEFENA
is set so unmapped addresses fall back to the default memory map.

| Register | Value | Meaning |
| --- | --- | --- |
| `MPU_RNR` (`0xE000_ED98`) | `0` | Select region 0. |
| `MPU_RBAR` (`0xE000_ED9C`) | `0xD000_0010` | Base = 0xD000_0000, REGION=0, VALID=1. |
| `MPU_RASR` (`0xE000_EDA0`) | `0x130C_0031` | ENABLE=1, SIZE=24 (32 MB), B=0, C=0, S=1, TEX=001 (Normal), AP=011 (full access), XN=1. |
| `MPU_CTRL` (`0xE000_ED94`) | `0x05` | ENABLE=1, PRIVDEFENA=1, HFNMIENA=0. |

Followed by `DSB; ISB` to ensure subsequent SDRAM accesses see
the new map.

### §5.6 Bring-up ordering — **Standards Action**

`disco::sdram::init()` MUST be called from `main()` **after**
`disco::clocks::init()` and `disco::pinmux::mux_fmc_pins()`,
**before** any access to `0xD000_0000+`. The ten-step PLAT-02b
sequence + the FMC pin flood are the prerequisites; ordering
inside `disco::sdram::init()` is:

1. Program `SDCR1 = 0x15D5`, `SDCR2 = 0x1D5`.
2. Program `SDTR1 = 0x10_6000`, `SDTR2 = 0x101_0461`.
3. Run JEDEC steps 1–5 (§5.4) with BUSY-polling.
4. Program `SDRTR = 0x46C`.
5. Program MPU region 0 + enable MPU (§5.5).
6. `DSB; ISB`.
7. Breadcrumb `0xA11C_0009`.
8. Return.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `pac_sdram_init` feature.** Numerical values
  (SDCR/SDTR/SDCMR/SDRTR + mode register) are byte-for-byte
  identical. Sequencing is identical. lvglpp's typed
  register-block + `static_assert(offsetof)` discipline replaces
  rlvgl's "raw write at 0x148" workaround for the PAC
  off-by-one — the typed layout cannot drift by definition.
- **PLAT-02b.** Consumes `PLL2_R = 150 MHz` → SDCLK = /2 = 75 MHz
  + the FMC pin flood. Failure to run PLAT-02b first is a
  silent latent bug — either PLLs aren't ready or the pins are
  in their reset state (Analog/Low-speed) and SDRAM accesses
  return garbage. PLAT-02c does not defensively re-check; it is
  a discipline violation to call `sdram::init()` without
  `clocks::init()` + `mux_fmc_pins()` first.
- **MPU & D-cache.** PLAT-02c programs the MPU but leaves
  D-cache off. PLAT-02e re-enters the MPU code path to add
  cache-clean-by-MVAC discipline for DMA2D coherency; until
  then, leaving the SDRAM region as Normal-non-cacheable is
  correct.
- **Linker `SDRAM` region** in `examples/stm32h747i-disco/memory.ld`
  remains at `0xC000_0000` (visibility-only, byte-for-byte rlvgl
  parity). No `.sdram_*` output sections are placed by
  PLAT-02c — direct pointer access only. PLAT-02d may
  introduce a `.sdram_framebuffer` section anchored at
  `0xD000_0000`; the linker region origin will be amended in
  the same chapter.

## §11 Non-goals

- **D-cache enable**, cache-clean-by-MVAC primitives. PLAT-02e.
- **`.sdram_*` output sections** in the linker script. PLAT-02d
  introduces a framebuffer section if needed; PLAT-02c uses
  raw pointer access only.
- **Bank 1 SDRAM** at `0xC000_0000`. Disco wires Bank 2 only;
  there's no consumer.
- **Self-refresh / power-down** transitions. Bring-up only.
- **NOR / PSRAM / NAND** FMC modes. SDRAM only; non-SDRAM
  registers are not modeled in `disco::regs::Fmc`.
- **480 MHz silicon support** that requires VOS0 (rev V/Y).
  PLAT-02b stays at 400 MHz; PLAT-02c inherits.

## §12 Acceptance checklist

- [ ] `examples/stm32h747i-disco/disco/regs/fmc.hpp` ships the
      typed `Fmc` block per §5.1 with `static_assert(offsetof(...))`
      lines for every offset used.
- [ ] `examples/stm32h747i-disco/disco/regs/mpu.hpp` ships the
      typed `Mpu` block per §5.5 with `static_assert(offsetof(...))`
      lines.
- [ ] `examples/stm32h747i-disco/disco/sdram.hpp` exposes
      `disco::sdram::init()`.
- [ ] `disco::sdram::init()` runs the §5.6 ordering: SDCR/SDTR
      writes, JEDEC sequence with BUSY-polling, SDRTR, MPU
      region 0 + enable, DSB/ISB, breadcrumb.
- [ ] A new smoke target `lvglpp_stm32h747i_disco_sdram` calls
      `clocks::init()` → `mux_fmc_pins()` → `sdram::init()`,
      then a 32-pattern memtest (per below) over the first 4 KB
      of SDRAM, breadcrumb `0xA11C_0009`, loop on WFE.
- [ ] On hardware, post-flash + reset:
      - Breadcrumb `*((uint32_t*)0x3800_0300)` reads `0xA11C_0009`.
      - Probe-rs reads of `0xD000_0000` + `0xD000_0004` return
        the test pattern (e.g. `0xDEADBEEF` / `0xCAFEBABE`).
      - SDCR1/2 + SDTR1/2 readback at `0x5200_4140..0x5200_414F`
        match §5.2 + §5.3 values.
- [ ] Build under cross toolchain stays inside
      `text < 12 KiB`, `data == 0`, `bss < 1 KiB`.
- [ ] `platform/STATUS.md` change-log entry recording PLAT-02c
      landing, dated.

## §13 Files cited

- `rlvgl/docs/disco-platform-guide/03-sdram-and-fmc.md` (v0.2.0
  @ 79f730d).
- `rlvgl/examples/stm32h747i-disco/{BOOT.md,HARDWARE.md}`.
- `rlvgl/examples/stm32h747i-disco/src/main.rs` L1034–1132
  (`pac_sdram_init` + JEDEC sequence).
- STMicroelectronics RM0399 §22 (FMC), §3.3 (memory map).
- ISSI IS42S32800J-6BLI datasheet (timing constants).
- ARM ARM (DDI 0403E.e) §B3.5 (Cortex-M7 MPU).
- `lvglpp/docs/platform-disco/00-platform-disco.md` (family — §5.5
  register-block discipline, §5.8 breadcrumb codes via PLAT-02b).
- `lvglpp/docs/platform-disco/02-clocks-and-pinmux.md` (PLAT-02b,
  pre-condition).

## §14 Unblocks

- 32 MB of off-chip RAM is available at `0xD000_0000`. PLAT-02d
  parks framebuffers (e.g. 800×480 × 2 bytes RGB565 = 768 KB
  per frame; double-buffer = 1.5 MB) here without any allocator.
- DMA2D (PLAT-02e) gets a destination buffer big enough for any
  realistic workload; ERIF holdoff timing analysis can use real
  AXI bus contention numbers.
- The "memtest is green" property is the cross-language hand-off:
  the same probe-rs read of `0xD000_0000+` works whether rlvgl
  or lvglpp is flashed; if the pattern survives a refresh
  interval (~7.8 µs), refresh is configured correctly.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Fmc + Mpu typed
  register blocks (§5.1, §5.5), SDCR/SDTR values (§5.2, §5.3),
  JEDEC sequence + refresh count (§5.4), MPU region attributes
  (§5.5), bring-up ordering (§5.6) all frozen.
- 2026-04-28 — **`SDCLK[1:0]` encoding correction.** Memalpha
  verification against **RM0399 §23.9.5.1** + **RM0433 §22.9.5.1**
  confirmed:
  - `00` = SDCLK clock disabled
  - `01` = **Reserved**
  - `10` = SDCLK = `fmc_ker_ck / 2`
  - `11` = SDCLK = `fmc_ker_ck / 3`
  Previous chapter §5.2 + execution wrote `0b01` per rlvgl
  Vol II Ch 3 § 2's claim that this is "Reserved per RM0399, but
  required on this silicon." On the lvglpp/disco execution path
  (PLLs configured + FMCSEL=PLL2_R *before* SDRAM init), `0b01`
  produces an undefined/half-broken controller state: writes
  reach SDRAM but column-address bit 2 is inverted (writes at
  `0xD000_0000` read back at `0xD000_0004` and vice versa) and
  higher offsets alias. Switching to the documented `0b10`
  (`fmc_ker_ck / 2` → SDCLK = 75 MHz with PLL2_R = 150 MHz) is
  the correct value per RM0399. Code change:
  `disco::regs::sdcr::SDCLK_2` → `SDCLK_DIV2` (= `0b10 << 10`)
  and the rlvgl-claim documented in the header comment for
  audit trail. Hardware verification to follow once probe-rs
  re-enumerates the ST-LINK.
