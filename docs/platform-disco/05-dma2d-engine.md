# 05 — DMA2D Engine (Chrom-Art) + Bus Admission

Chapter status: **draft, ratified 2026-06-10**.
Phase code: **PLAT-02e** (see family chapter `00-platform-disco.md` §5.1).

## §0 Authority

- Register vocabulary (CR/ISR/IFCR/FGMAR/…/NLR/LWR/AMTCR, mode codes
  R2M / M2M / M2M+PFC / M2M+blend, pixel-format codes) is owned by
  **STMicroelectronics RM0399**, DMA2D ("Chrom-Art Accelerator")
  chapter — §18 in the revision rlvgl cites. Register *names and
  byte offsets* are the citation unit here, not section numbers
  (revision-dependent).
- Driver shape, mode selection, AMTCR dead-time value, ISR-latch and
  admission-control intent are owned by
  `rlvgl/platform/src/dma2d.rs` + `rlvgl/docs/disco-platform-guide/`
  Vol II Ch 7 (v0.2.0 @ 79f730d). When this chapter and rlvgl
  diverge, rlvgl is canonical and this chapter is the bug — except
  where §10 records a deliberate, bench-evidenced deviation.
- NOTE (informative): the ASCII register diagram in rlvgl Vol II Ch 7
  carries two transcription typos (BGPFCCR listed at +0x2C; OPFCCR
  and OMAR both at +0x3C). The *code* in `dma2d.rs` uses the PAC and
  is unaffected. lvglpp's `regs/dma2d.hpp` static_asserts the RM0399
  offsets (§5.2), which is the registration mechanism for exactly
  this class of error.
- Bus-arbitration posture (AXI QoS, LTDC starvation) is owned by
  PLAT-02d (`04-ltdc-dsi-and-panel.md` §15.11) — this chapter
  consumes it, does not amend it.

## §1 Purpose

Bring up the DMA2D engine on the STM32H747I-DISCO so rectangle fills
and buffer blits run in hardware while the LTDC continuously scans
the same SDRAM under DSI adapted-command-mode auto-refresh
(WCFGR.AR=1, PLAT-02d). Lands the typed register block, the clock
gate fix, a blocking-poll blitter, and the first-fill bench gate.
The `InFlight<T>` ownership token and admission control land in
later sub-phases (§5.6).

## §2 Problem statement

1. **The clock gate is wrong in shipped code.** `disco/regs/rcc.hpp`
   defines `ahb1enr::DMA2DEN = 1u << 23` and `disco/clocks.cpp`
   step 7 sets it on `AHB1ENR`/`C1_AHB1ENR`. That is the STM32F4/F7
   bit position. On the H7, DMA2D is on **AHB3ENR bit 4** (RM0399
   RCC AHB3ENR; corroborated by rlvgl `dma2d.rs::steal()` doc — "The
   DMA2D clock must already be enabled (RCC AHB3ENR bit 4)" — and by
   rlvgl's live `AHB3ENR = 0x1010` = FMC|DMA2D vs our `0x1001` =
   FMC|MDMA, bench-read 2026-06-08). Today AHB1 bit 23 lands on an
   unrelated peripheral and DMA2D is unclocked.
2. **No DMA2D driver exists in lvglpp.** `main_display.cpp` fills the
   framebuffer with the CPU (DELTA comment: "no DMA2D yet —
   PLAT-02e"). Every render path beyond first light (LVGL flush,
   text blits) needs the engine.
3. **DMA2D shares the AXI/SDRAM path with a live LTDC scan.** With
   AR=1 the panel refreshes continuously; an unthrottled DMA2D burst
   stream can starve LTDC reads (FIFO underrun → visible tearing /
   snow). rlvgl mitigates with AMTCR dead time + AXI QoS + admission
   control.

## §3 Canonical glossary

- **R2M / M2M / M2M+PFC / M2M+blend** — DMA2D CR.MODE values 3/0/1/2.
  As defined in RM0399 DMA2D_CR; mirrored as constants in
  `examples/stm32h747i-disco/disco/regs/dma2d.hpp`.
- **Blitter** — As defined in `rlvgl/platform/src/blit.rs` (trait) +
  `dma2d.rs` (`Dma2dBlitter`); mirrored here as
  `examples/stm32h747i-disco/disco/dma2d.hpp` free functions in
  `lvglpp::disco::dma2d` (bring-up shape; the `lvglpp::core::Renderer`
  integration is PLAT-02e-3).
- **AMTCR dead time** — AHB master timer: DT cycles inserted between
  DMA2D AXI bursts. As defined in `rlvgl/platform/src/dma2d.rs:33`
  ("DT=8 cycles between DMA2D AXI bursts gives LTDC room to read");
  used without modification.
- **Admission control (`dma2d_admits`)** — As defined in
  `rlvgl/examples/stm32h747i-disco/src/main.rs` L426–438 and Vol II
  Ch 7 §2. Owned by PLAT-02e-3; does not exist in this repo yet.
- **`InFlight<T>`** — As defined in `00-platform-disco.md` §3
  (mirrors rlvgl `InFlight<'dma, T>`). Owned by PLAT-02e-2; does not
  exist in repo yet.
- **Completion latch** — As defined in rlvgl Vol II Ch 7 §3 (ISR
  sets atomic latch; poll-then-clear races). Owned by PLAT-02e-2
  (lvglpp is polling-only until then; the race rlvgl's latch fixes
  only exists once completion is consumed asynchronously).
- **DMA2D_FIRST_DONE** — breadcrumb `0xA11C_000D`. As defined in
  `examples/stm32h747i-disco/disco/breadcrumb.hpp:24` (frozen
  byte-for-byte with rlvgl's breadcrumb set); used without
  modification.

## §4 Source-of-truth map

| Concept | Canonical owner | lvglpp mirror |
| --- | --- | --- |
| Register block layout | RM0399 DMA2D chapter | `disco/regs/dma2d.hpp` (static_asserts) |
| Clock gate (AHB3 bit 4, dual-core) | RM0399 RCC + rlvgl bench `AHB3ENR=0x1010` | `disco/regs/rcc.hpp` `ahb3enr::DMA2DEN` + `disco/clocks.cpp` step 7 |
| Mode selection + driver sequencing | `rlvgl/platform/src/dma2d.rs` | `disco/dma2d.cpp` |
| AMTCR DT=8 | `rlvgl/platform/src/dma2d.rs:33` | `disco/dma2d.cpp::init` |
| AXI QoS (INI5 DMA2D = 0x4) | PLAT-02d §15.11 / rlvgl `stm32h747i_disco.rs:601–610` | `disco/display.cpp` (already lands it) |
| Admission / ERIF budget | rlvgl `main.rs` L426–438 + Vol II Ch 5/7 | PLAT-02e-3 (not yet) |

## §5 Frozen decisions

### §5.1 Clock gate — **Standards Action**

`DMA2DEN` is **AHB3ENR bit 4**, set on BOTH `RCC_AHB3ENR` and
`RCC_C1_AHB3ENR` (dual-core rule, PLAT-02b). The wrong
`ahb1enr::DMA2DEN` constant is deleted, not deprecated — nothing may
keep compiling against it.

### §5.2 Register block — **Specification Required**

`disco/regs/dma2d.hpp`, base `0x5200_1000`, `struct alignas(4)` with
`volatile` fields and `static_assert` for every documented offset:
CR 0x00, ISR 0x04, IFCR 0x08, FGMAR 0x0C, FGOR 0x10, BGMAR 0x14,
BGOR 0x18, FGPFCCR 0x1C, FGCOLR 0x20, BGPFCCR 0x24, BGCOLR 0x28,
FGCMAR 0x2C, BGCMAR 0x30, OPFCCR 0x34, OCOLR 0x38, OMAR 0x3C,
OOR 0x40, NLR 0x44, LWR 0x48, AMTCR 0x4C. Per family §5.5 this makes
a wrong offset a compile-time failure (and is the lvglpp-side check
against the rlvgl Ch 7 diagram typos, §0).

### §5.3 Driver sequence — **Standards Action** (mirrors dma2d.rs)

- `init()`: if CR.START clear, `IFCR = 0x3F`; then
  `AMTCR = (8 << 8) | 1` (DT=8, EN). Mirrors `Dma2dBlitter::new`.
- `fill` (R2M): program OMAR/OCOLR/OOR/NLR, clear flags, write CR =
  mode|START preserving interrupt-enable bits. OPFCCR is left at
  reset (ARGB8888) — same elision as rlvgl `fill_engine_raw`.
- `blit` (M2M+PFC): FGMAR/FGOR/FGPFCCR + OMAR/OOR/OPFCCR + NLR.
- Completion: poll CR.START for busy; ISR.TCIF (bit 1 — bit 0 is
  TEIF) for complete; errors = ISR.TEIF|CEIF.
- NLR packing: `PL[29:16] = width`, `NL[15:0] = height`.
- Line offsets (FGOR/OOR) are in **pixels**, not bytes.

### §5.4 Coherency posture — **Standards Action**

PLAT-02 runs with CM7 I/D caches **off** and the SDRAM MPU region
TEX=001/C=0/B=0 (normal, non-cacheable — `disco/sdram.cpp` step 5).
Therefore no cache clean/invalidate is required around DMA2D
transfers in this family. Any future phase that enables the D-cache
MUST amend this section first and add the clean-by-MVAC discipline
(family §2 gap 5) in the same change.

### §5.5 First-fill bench gate — **Specification Required**

The PLAT-02e-1 acceptance artifact is a DMA2D R2M fill executed
**after** PANEL_UP, over the live auto-refreshing framebuffer: a
centered magenta (0xFFFF00FF) rectangle on the PLAT-02d quadrant
pattern. Pass = breadcrumb `0xA11C_000D` + relayed ISR shows TCIF
without TEIF/CEIF + owner sees the rectangle. Magenta appears
nowhere in the quadrant pattern, so a CPU-fallback false pass is
impossible (nothing else writes it).

### §5.6 Sub-phase set — **Specification Required**

- **PLAT-02e-1** (this doc's execution gate): gate fix + regs +
  blocking-poll fill/blit + first-fill bench gate.
- **PLAT-02e-2**: `InFlight<T>` token (CPU access to the destination
  during a transfer becomes a compile error), DMA2D NVIC ISR +
  completion latch, non-blocking submission.
- **PLAT-02e-3**: admission control under scan pressure +
  `lvglpp::core::Renderer` integration (`fill_rect`/`blend_rect`).
  Gate: deliberate-pressure test per family §5.1.

### §5.7 PLAT-02e-2 — ISR latch + InFlight token — **Specification Required**

Added by the 2026-06-10 §15 amendment; mirrors rlvgl Vol II Ch 7 §3
+ `hwcore/surface.rs` (`BorrowedForDma` / `InFlight`).

- **ISR**: `DMA2D_IRQHandler` (vector slot 90, verified positionally
  against the startup table) clears the asserted ISR bits via IFCR
  and latches TCIF into a completion latch, TEIF|CEIF into an error
  latch; counts both. NVIC priority 3 (rlvgl parity: one notch below
  the future TIM6 touch tick at 2). The latch is consumed by a
  single `take_complete()` swap — poll-then-clear from the main loop
  races the next job (rlvgl Ch 7 §3); the latch does not.
- **Latch storage**: plain `volatile` words, NOT C11/C++ atomics —
  single core, single writer (ISR) / single reader (main loop),
  word-sized aligned accesses. Becomes `std::atomic` the day a
  second context appears.
- **`FrameBuffer`**: move-only value handle (base, w, h, stride) over
  a `dma:`-marked SDRAM region. CPU mutation goes through the
  handle; the handle does not own the memory (`external:` SDRAM,
  never freed).
- **`InFlight`**: returned by `start_fill_async` / `start_blit_async`,
  which take the `FrameBuffer` **by value (moved in)**. While the
  token holds the buffer, the caller has no handle to mutate — the
  closest C++ gets to rlvgl's borrow-checked guarantee, and a
  compile error for the straight-line misuse (use-after-move is
  flagged by clang-tidy per the ownership lint posture).
  `try_release()` returns the buffer only after the latch reports
  completion; `release_blocking()` spins on the latch. Misuse that
  C++ cannot reject at compile time (releasing while busy) returns
  no buffer rather than trapping.
- **Engine exclusivity**: one outstanding transfer; `start_*_async`
  with a transfer in flight traps (bkpt) — single-context bring-up
  posture, revisited when the cooperative pump owns scheduling.



- **AR=1 vs rlvgl's ERIF-holdoff pipeline.** rlvgl's admission
  control assumes the Ch 5 manual present pipeline (ERIF ISR clears
  LTDCEN; render; re-enable). lvglpp's PLAT-02d path runs WCFGR.AR=1
  — continuous TE-driven refresh, no LTDCEN pulsing, no ERIF ISR
  yet. Consequences frozen here: (a) PLAT-02e-1 relies on AMTCR
  dead time + AXI QoS alone to protect the scan — acceptable for a
  single bounded fill; (b) the rlvgl `dma2d_admits` budget math does
  not transplant verbatim — PLAT-02e-3 must either derive the budget
  from TE/ERIF timestamps under AR=1 or adopt manual refresh during
  render bursts. That choice is **deliberately unfrozen** until
  bench evidence (it is the load-bearing open question of 02e-3).
- **PLAT-02d framebuffer layout.** Front buffer `0xD000_0000`,
  ARGB8888 480×800; back buffer reserved at `+0x18_0000`
  (`display.hpp`). 02e-1 fills the front buffer directly (single
  buffer, live scan — tearing during the fill is in-spec for the
  bench gate); double-buffered presents are 02e-2/3 territory.
- **Bring-up shape vs family §4 map.** The family maps DMA2D to
  `platform/src/disco/dma2d.cpp`; like clocks/sdram/display before
  it, the code lands under `examples/stm32h747i-disco/disco/` first
  and migrates to `platform/` when the library target lands. Same
  standing deviation, same resolution path.

## §11 Non-goals

- M2M+blend (A8 glyph path) — needed for text, lands with the
  Renderer integration (02e-3) where it has a consumer.
- CLUT loading, watermark (LWR), dead-time tuning beyond DT=8.
- D-cache enablement (§5.4).
- CM4-side DMA2D use (family §5.3: CM7-only).

## §12 Acceptance checklist (PLAT-02e-1)

- [x] `ahb1enr::DMA2DEN` deleted; `ahb3enr::DMA2DEN = 1u << 4`;
      `clocks.cpp` sets it on AHB3ENR + C1_AHB3ENR. (Bench:
      AHB3ENR readback 0x1011.)
- [x] `disco/regs/dma2d.hpp` lands with all §5.2 static_asserts.
- [x] `disco/dma2d.{hpp,cpp}` lands per §5.3 with ownership
      comments per CLAUDE.md.
- [x] `lvglpp_stm32h747i_disco_display` performs the §5.5 fill;
      bench shows breadcrumb 0xA11C_000D, TCIF=1, TEIF=CEIF=0
      (relay 0x600D_F111 / err 0). 2026-06-10.
- [x] **Blind substitute for the visual gate (§15 amendment):**
      CPU pixel-verify of the rect interior + all four untouched
      neighbours passed (relay 0x600D_0000, 10/10 samples); M2M
      blit across the RED→BLUE seam CPU-compared 0/2400 mismatches;
      48×80 D3 capture decoded host-side — magenta bbox exactly
      (14,25)–(33,54) cells, blit block present, zero diffs outside
      the two expected regions; WISR stayed 0x7307 throughout.
- [ ] Owner confirms the magenta rectangle on the panel — panel
      OPTICS only; deferred until eyes are available again. The
      LTDC→DSI→panel path itself was owner-verified in PLAT-02d.
- [x] Cross-build clean with `-Werror`.

### PLAT-02e-2 (§5.7)

- [x] `regs/nvic.hpp` (enable + priority, IRQN_DMA2D=90 verified
      positionally against the startup vector table — note the
      double-entry reserved line at slots 66/67 when counting).
- [x] `DMA2D_IRQHandler` + completion/error latches + counters;
      `enable_irq()` at NVIC priority 3.
- [x] `FrameBuffer` move-only handle + `InFlight` token;
      `start_fill_async` takes the buffer by value and the token
      returns it only when done.
- [x] Bench gate (2026-06-10, blind): three sequential async fills
      completed via `take_complete()` only (no CR.START busy-wait);
      relays 0x3B0=3 / 0x3B4=0 / 0x3B8=0x600D_0E22; all PLAT-02e-1
      relays unchanged. Serial `D` probes confirmed all three rect
      interiors AND adjacent untouched pixels (yellow/cyan/orange
      at exact coordinates).

## §13 Files cited

- `rlvgl/platform/src/dma2d.rs`, `rlvgl/platform/src/blit.rs`
  (v0.2.0 @ 79f730d).
- `rlvgl/platform/src/stm32h747i_disco.rs` (AXI QoS L601–610).
- `rlvgl/examples/stm32h747i-disco/src/main.rs` L319–333, L426–438,
  L720–800 (ISR + admission, via Vol II Ch 7).
- `rlvgl/docs/disco-platform-guide/07-dma2d-engine.md`.
- `docs/platform-disco/00-platform-disco.md` §5.1/§5.5;
  `04-ltdc-dsi-and-panel.md` §15.11.
- STMicroelectronics RM0399, DMA2D chapter.

## §14 Unblocks

- LVGL flush-callback acceleration (fill + blit are the two flush
  primitives) and the disco `Renderer` (family §10).
- The disco-demo app-shell port's hardware leg (DEMO family) — its
  compositor assumes accelerated fills.
- PLAT-02e-2/3 (token + admission) have a working engine to gate.

## §15 Change log

- 2026-06-10 — Chapter ratified at draft level. Clock-gate fix
  (§5.1), register block (§5.2), driver sequence (§5.3), coherency
  posture (§5.4), first-fill gate (§5.5), sub-phase split (§5.6)
  frozen. AR=1 admission question deliberately left open (§10).
- 2026-06-10 — §5.5 AMENDED: owner eyes unavailable; the visual
  half of the gate is replaced by a framebuffer-level blind bench
  (CPU pixel-verify relay at 0x3800_038C, M2M seam-blit compare at
  0x390/0x394, post-DMA2D 48×80 capture at 0x3800_5000 decoded
  host-side). Panel-optics confirm stays an open checklist item,
  non-blocking for 02e-2/3. PLAT-02e-1 execution landed same day;
  all blind gates green first run.
- 2026-06-10 — §5.7 ADDED (PLAT-02e-2 design): DMA2D ISR +
  completion latch, volatile-not-atomic rationale, FrameBuffer /
  InFlight move semantics, engine-exclusivity trap. Eyes-free gate:
  three sequential async fills completed via the ISR latch (no
  CR.START busy-wait), relayed counters at 0x3800_03B0.., content
  verified over playit serial `D` dumps.
- 2026-06-10 — PLAT-02e-2 execution landed; all gates green first
  run (§12). Ordering constraint recorded in code: enable_irq()
  must follow the blocking-path gates, since the ISR clears the
  flags wait_done() inspects. Remaining sub-phase: 02e-3
  (admission control — the §10 AR=1 question).
- 2026-06-10 — PLAT-02e-3a: the §10 open question now has bench
  evidence. Frame cadence measured blind by edge-counting WISR.BUSY
  from the serial pump with DWT timestamps (relays 0x3800_03C4..):
  **AR=1 auto-refresh ≈ 29.9 Hz, interval ≈ 13.37 M cycles @
  400 MHz (EMA stable)** — about half rlvgl's 60 Hz assumption, so
  `dma2d_admits` budget math must use the measured EMA, not the
  transplanted constant. STAT present_count is now this counter
  (closes the 06 §10 present=0 deviation). Full admission gating
  (cost model + guard band + underrun proof under deliberate
  pressure) remains 02e-3 proper.
