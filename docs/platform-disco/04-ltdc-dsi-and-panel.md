<!-- 04-ltdc-dsi-and-panel.md — PLAT-02d concepts doc (normative). -->

# PLAT-02d — LTDC + DSI + panel → first pixels

Status: **ratified** (2026-06-08). RFC 2119 keywords per
`00-platform-disco.md`. Sections cited by §12 are normative;
narrative/non-goals/change-log are informative.

## §0 Authority policy

- **Display bring-up register sequence** — canonical:
  `rlvgl/platform/src/display_init.rs` (the raw-MMIO DSI + LTDC init,
  itself a transcription of STM32CubeH7 `stm32h747i_discovery_lcd.c`
  `HAL_DSI_Init` / `HAL_DSI_ConfigAdaptedCommandMode` / `HAL_DSI_Start`).
  lvglpp mirrors register-for-register.
- **Panel command sequence** — canonical: `rlvgl/platform/src/otm8009a.rs`
  + `dsi_cmd_mode.rs` (and the NT35510 path in `display_init.rs`). The
  MB1166 module ships either OTM8009A or NT35510 depending on revision;
  lvglpp mirrors the rlvgl sequence actually used by the pinned binary.
- **Architecture / why** — `rlvgl/docs/disco-platform-guide/05-ltdc-dsi-and-axi-holdoff.md`
  (LTDC/DSI timing, adapted command mode, the ERIF-gated LTDCEN holdoff)
  and `07-adapted-cmd-deep-dive.md`. These are the normative concepts
  source for the *why*; the code is normative for register fields.
- **Registers** — RM0399 (LTDC §33, DSI §34) + the panel datasheet.
- **C++ ownership / MMIO encapsulation deltas** — owned here.

## §1 Purpose

Get the first controlled pixels onto the panel: bring up the DSI host +
PHY + PLL, the OTM8009A/NT35510 panel, and the LTDC scanning an
ARGB8888 framebuffer in SDRAM. Acceptance is a smoke target that fills
the framebuffer with a known pattern and shows it. CPU fills only —
DMA2D is PLAT-02e.

## §2 Problem statement / preconditions (informative)

- **SDRAM is up** (PLAT-02c, hardware-verified 2026-06-08): the
  framebuffer lives at `0xD000_0000`. ✓
- **PLL3 feeds the LTDC pixel clock** (~32 MHz; `disco::clocks` already
  brings PLL3 up with the PLL3ON gap-fix). The DSI has its own PLL
  (`DSI_WRPCR`) fed from HSE.
- **Peripheral clocks**: LTDC (`APB3ENR`), DSI (`APB3ENR`), DMA2D
  (`AHB3ENR`), and the panel-control GPIO ports must be gated on
  (`rlvgl::enable_display_peripheral_clocks`). `disco::clocks` already
  gates LTDC/DSI/DMA2D; PLAT-02d adds any missing GPIO ports (G for
  panel reset PG3, J for backlight PJ12).
- DSI/LTDC bring-up is the most failure-prone phase: register-address
  aliases (rlvgl's `DSI_LCCR`@0x64-vs-`DSI_PCR`@0x2C bug → snow), PHY
  timing, and panel DCS ordering all bite. On-bench iteration is
  expected; this chapter freezes the *contract*, not a one-shot.

## §3 Canonical glossary

- **DSI host / wrapper** — MIPI-DSI controller at `0x5000_0000`
  (host regs) + `0x5000_0400` (wrapper). As defined in
  `display_init.rs:30,69`; mirrored as `disco/regs/dsi.hpp`.
- **LTDC** — LCD-TFT controller at `0x5000_1000`, scans the framebuffer
  to the DSI in adapted command mode. As defined in `display_init.rs:79`;
  mirrored as `disco/regs/ltdc.hpp`.
- **Adapted command mode** — DSI mode where LTDC drives a single-shot
  frame into panel GRAM on refresh (vs continuous video mode). rlvgl
  ships adapted command mode; lvglpp mirrors it. *Owned by rlvgl
  disco-platform-guide ch.5/ch.7.*
- **OTM8009A / NT35510** — the MB1166 panel controller (480×800 portrait).
  As defined in `rlvgl/platform/src/otm8009a.rs`; mirrored as
  `disco/otm8009a.hpp`.
- **Framebuffer** — ARGB8888 buffer in SDRAM scanned by LTDC layer 1
  (`LTDC_L1CFBAR`). 480×800×4 = 1,536,000 B per buffer.
- **ERIF** — DSI end-of-refresh interrupt (`DSI_WISR` bit), used by
  PLAT-02e for tear-free present; **not** required for first pixels.

## §4 Source-of-truth map

| Concept | RM0399 / panel | rlvgl (canonical) | lvglpp (mirror) |
| --- | --- | --- | --- |
| DSI regs | §34 | `display_init.rs:30-75` | `disco/regs/dsi.hpp` (new) |
| LTDC regs | §33 | `display_init.rs:79-` | `disco/regs/ltdc.hpp` (new) |
| DSI PLL / regulator / PHY | §34 | `init_dsi_regulator/pll/phy` (`:204,228,260`) | `disco/display.cpp` (new) |
| DSI video timings | §34 | `init_dsi_video_timings` (`:337`) | `disco/display.cpp` |
| LTDC timing | §33 | `configure_ltdc_timing` (`:482`) | `disco/display.cpp` |
| Panel DCS init | datasheet | `otm8009a.rs` / `dsi_cmd_mode.rs` | `disco/otm8009a.{hpp,cpp}` (new) |
| Panel reset / backlight | board | `pulse_panel_reset`/`enable_backlight` (`:175,191`) | `disco/display.cpp` |
| Display peripheral clocks | §RCC | `enable_display_peripheral_clocks` (`:395`) | extend `disco/clocks` or `disco/display.cpp` |
| Framebuffer layout | — | `display_init.rs` L1CFBAR | `disco/display.hpp` (new) |

## §5 FROZEN — panel geometry, pixel format, framebuffer

- **Panel:** 480×800 **portrait** native (MB1166). FROZEN.
- **Pixel format:** ARGB8888 (LTDC L1PFCR = 0; 32 bpp). FROZEN. Matches
  `core::Color::to_argb8888()` and `core::Renderer::draw_pixels`.
- **Framebuffer base:** front buffer at `sdram::BASE = 0xD000_0000`.
  Back buffer (PLAT-02e double-buffering) at `0xD000_0000 + 0x0018_0000`
  (mirror rlvgl's offset). PLAT-02d uses the front buffer only.
- **Stride:** 480×4 = 1920 B/line. Size 1,536,000 B/buffer (< 32 MiB). ✓
- **Rotation:** the app renders **landscape 800×480**; the panel is
  portrait 480×800. rlvgl applies landscape rotation in software
  (`RotatedRenderer`) — for PLAT-02d first-pixels, the smoke writes in
  the panel's native portrait orientation; the landscape rotation seam
  is a DiscoRenderer concern (Task #6), noted as a DELTA, not frozen
  here.

## §6 FROZEN — display clocks

- **LTDC pixel clock:** PLL3_R (`disco::clocks` already programs PLL3,
  PLL3ON gap-fix applied). The exact LTDC clock for 480×800 @ ~60 Hz is
  set by the timing in §8; PLL3_R target ≈ the rlvgl value
  (`ensure_pll3_running`, `display_init.rs:457`). FROZEN to rlvgl's
  PLL3 config.
- **DSI PLL:** fed from HSE; `DSI_WRPCR` NDIV/IDF/ODF → VCO 1000 MHz →
  500 Mbps/lane, lane byte clock 62.5 MHz (mirror `init_dsi_pll`,
  `:228`). FROZEN to rlvgl's NDIV/IDF/ODF.
- **Lanes:** 2 data lanes (mirror rlvgl). FROZEN.

## §7 FROZEN — init order (mirror `display_init.rs` exactly)

1. `enable_display_peripheral_clocks` (LTDC/DSI/DMA2D + GPIO G/J).
2. `ensure_pll3_running` (LTDC pixel clock).
3. `pulse_panel_reset` (PG3 low→high with delays).
4. `enable_backlight` (PJ12 high).
5. `init_dsi_regulator` (`DSI_WRPCR` bit 24; poll ready).
6. `init_dsi_pll` (`DSI_WRPCR` NDIV/IDF/ODF; poll lock).
7. `init_dsi_phy` (`DSI_CCR`, `DSI_PCTLR`, `DSI_PCONFR`, `DSI_WPCR0`,
   `DSI_CR`).
8. `init_dsi_lane_timings` (`DSI_CLTCR`/`DSI_DLTCR`).
9. `init_dsi_flow_control`.
10. `init_dsi_ltdc_interface` (`DSI_LCOLCR`, `DSI_LPCR`).
11. `init_dsi_video_timings(480, 800, …)` (VHSACR/VHBPCR/VLCR/VVSACR/
    VVBPCR/VVFPCR/VVACR/VPCR/VMCR/LPMCR).
12. `configure_ltdc_timing(480, 800, …)` (SSCR/BPCR/AWCR/TWCR/BCCR).
13. LTDC layer-1 config: window (L1WHPCR/L1WVPCR), pixel format
    (L1PFCR=0), framebuffer addr (L1CFBAR=`0xD000_0000`), line length /
    pitch (L1CFBLR), line count (L1CFBLNR), constant alpha, default
    color; then L1CR.LEN + LTDC SRCR reload.
14. Panel DCS init sequence (OTM8009A/NT35510): exit sleep (`0x11`),
    pixel format, orientation/MADCTL, gamma, display on (`0x29`) — via
    DSI generic/DCS short/long writes (`DSI_GHCR`/`DSI_GPDR`).
15. LTDC enable (`LTDC_GCR.LTDCEN`) + DSI start (`DSI_CR`/wrapper).

The address-alias hazards (`DSI_LCCR`@0x64 ≠ `DSI_PCR`@0x2C;
`DSI_LCOLCR` etc.) MUST be transcribed from `display_init.rs` verbatim —
this is where rlvgl bled.

## §8 FROZEN — LTDC + DSI timing derivation

LTDC timing registers are derived from `(HSW, HBP, W, HFP)` /
`(VSW, VBP, H, VFP)` exactly as `configure_ltdc_timing`
(`display_init.rs:482-507`):
`SSCR=(HSWm1<<16)|VSWm1`, `BPCR=(AHBP<<16)|AVBP`,
`AWCR=(AAW<<16)|AAH`, `TWCR=(TotalWm1<<16)|TotalHm1`, `BCCR=0`, where
`AHBP=HSW+HBP-1`, `AAW=HSW+HBP+W-1`, `TotalW=HSW+HBP+W+HFP-1` (and the
vertical analogues). The porch constants (HSW/HBP/HFP/VSW/VBP/VFP) are
FROZEN to rlvgl's values (`display_init.rs` / the guide ch.5 timing
table). DSI video timings (`init_dsi_video_timings`, `:337`) derive
HSA/HBP/HLINE in lane-byte-clock units from the same porches — mirror
the arithmetic verbatim.

## §9 Ownership / MMIO discipline

- `disco/regs/{ltdc,dsi}.hpp` follow the existing typed-MMIO pattern
  (`MmioAddr<T>` + `volatile` register structs + `offsetof` static
  asserts, like `regs/fmc.hpp`), each marked
  `// mmio: owned by RM0399 §33/§34; never freed.`
- No raw `lv_obj_t*`; this is pre-LVGL bare-metal. Framebuffer is a
  `// dma:`-class buffer once PLAT-02e lands (LTDC scans it); for
  PLAT-02d CPU fills are fine (LTDC reads, CPU writes — distinct frames).
- Init functions are free functions in `lvglpp::disco::display`;
  ownership of the panel/DSI state is "external: lifecycle controlled by
  the hardware, configured once at boot."

## §10 Non-goals (informative)

- DMA2D acceleration + ERIF-gated tear-free present — PLAT-02e.
- The landscape `RotatedRenderer` seam + `DiscoRenderer` — Task #6.
- Touch + USART1 playit — PLAT-02f.
- Continuous video mode (rlvgl ships adapted command mode; video mode is
  an alt path documented in the Zephyr guide, out of scope).

## §11 Acceptance (normative)

- [ ] `disco/regs/{ltdc,dsi}.hpp` compile; `offsetof` asserts match
      RM0399 / `display_init.rs` addresses.
- [ ] A `main_display.cpp` smoke target runs clocks → pinmux → SDRAM →
      `display::init()` → fills the framebuffer at `0xD000_0000` with a
      known pattern (solid color + corner markers + gradient).
- [ ] **On the bench: the panel shows the pattern**, stable, correct
      orientation/geometry (corner markers locate the origin; no snow /
      tearing-free for a static frame).
- [ ] A breadcrumb (`0xA11C_000B`) + a D3-SRAM relay of DSI/LTDC status
      regs (`DSI_PSR`/`LTDC_CDSR`) lets probe-rs confirm the controllers
      reached the running state even before the panel is trusted.
- [ ] Cross-build clean; disco firmware size within the target budget.

## §12 Files cited

Canonical: `rlvgl/platform/src/display_init.rs` (`:30,69,79,175,191,204,
228,260,301,314,323,337,395,457,482`), `rlvgl/platform/src/otm8009a.rs`,
`rlvgl/platform/src/dsi_cmd_mode.rs`,
`rlvgl/docs/disco-platform-guide/05-ltdc-dsi-and-axi-holdoff.md`,
`07-adapted-cmd-deep-dive.md`. Mirror target:
`examples/stm32h747i-disco/disco/regs/{ltdc,dsi}.hpp`,
`disco/display.{hpp,cpp}`, `disco/otm8009a.{hpp,cpp}`,
`examples/stm32h747i-disco/main_display.cpp`. Existing:
`disco/clocks.cpp` (PLL3), `disco/sdram.hpp` (`BASE`).

## §13 Unblocks

PLAT-02e (DMA2D + ERIF present), PLAT-02f (touch/serial in parallel),
Task #6 (DiscoRenderer → run the disco demo on the panel).

## §14 Change log

- _drafted_ — PLAT-02d contract: panel geometry/pixel-format/framebuffer,
  display clocks, the mirror-`display_init.rs` init order, LTDC/DSI
  timing derivation, MMIO discipline, and a first-pixels acceptance
  smoke.
- **2026-06-08 — ratified.** Owner directed proceeding (remote). Faithful
  register-for-register mirror of `rlvgl/platform/src/display_init.rs`;
  on-bench iteration expected. Execution may proceed.
- **2026-06-08 — bench session 1 (see §15).** Implementation landed and
  flashed. Whole display chain proven working; blocked on an LTDC
  clock-domain access bug. Two real clock fixes found. Details + learnings
  in §15. **Code is on disk, uncommitted.**

## §15 Bench bring-up log & learnings (informative)

Session 1 (2026-06-08), STM32H747I-DISCO on the bench via probe-rs 0.29.1
+ ST-LINK V3. ~18 flash/measure cycles. This section is the resume point.

### §15.1 Current symptom

Panel shows **rainbow snow** (over stale GRAM content from a prior rlvgl
session — the "gear"). The firmware boots fully: breadcrumbs reach
`0xA11C_000B` (PANEL_UP), no fault.

### §15.2 What is PROVEN working (do not re-litigate)

- **SDRAM framebuffer**: `fill_pattern()` writes a perfect 480×800
  four-quadrant ARGB8888 image; CPU read-back is 100% clean (verified by
  the `capture_framebuffer()` → D3-SRAM → host-PNG path).
- **DSI host/PHY/PLL + NT35510 panel**: fully configured. Direct probe
  reads: `DSI_MCR=1` (command mode), `WCFGR=0x1b`, `LCCR=0x1e0` (=480),
  `WRPCR=0x01002991` (PLL), `WPCR0`=UIX4 8; `WISR` PLL-lock + reg-ready.
- **PLL3-R pixel clock**: `PLLCFGR=0x01230888` → `DIVR3EN=1`,
  `PLL3RGE=0b10`, `PLL3RDY=1`. `PLL3DIVR`: N=132, R=24 → 27.5 MHz.
- **THE WHOLE CHAIN**: when the **LTDC config is written via the probe**
  (core halted) + `WCR=0x0C`, **the panel displays the clean quad
  pattern** (owner-confirmed). So framebuffer→LTDC→DSI→panel all work.

### §15.3 THE BLOCKER (open)

The LTDC peripheral is **inaccessible to the running CM7** but accessible
to the halted debug probe:
- CM7 (running) reads `LTDC_GCR` → `0x00000000`; writes don't latch.
- Debug probe (core halted) reads `LTDC_GCR` → `0x00002220` (reset);
  writes stick.

So `configure_ltdc_timing()`/`setup_ltdc_layer()`/`enable_ltdc()` execute
but never configure the LTDC (it reads all-reset: `SSCR/TWCR=0`,
`GCR=0x2220`, `L1CFBAR=0`). The unconfigured LTDC never scans the
framebuffer → the DSI transmits garbage → snow. A retry-verify loop
(re-apply config until `GCR==ENABLE_VALUE`, up to 200 000×) **never
succeeds** — confirming writes never latch during run.

The "accessible-when-halted, not-when-running" signature = an LTDC
**clock-domain** problem specific to the LTDC (the DSI, same APB3 bus,
configures fine from the running CM7).

### §15.4 Fixes found this session (KEEP — both real, neither sufficient alone)

1. **PLL3 → 27.5 MHz** (`disco/clocks.hpp`: `pll3_n=132, pll3_r=24`, was
   `160/25`=32 MHz). Matches rlvgl's required adapted-command-mode pixel
   clock (`display_init.rs:625`). Was a genuine mismatch.
2. **Missing `C1_APB3ENR` CM7 clock gate** (`disco/clocks.cpp`): LTDC/DSI
   only set the combined `APB3ENR`; FMC (which works) sets **both**
   `AHB3ENR` and `C1_AHB3ENR`. The file's own comment mandates "both must
   be set or the peripheral appears ungated from CM7's perspective."
   Added `RCC->c1_apb3enr |= LTDCEN|DSIEN` (+ `c1_ahb1enr |= DMA2DEN`).
   Verified landed (`C1_APB3ENR=0x18`) — but **did NOT fix** the LTDC
   access. So there is at least one more clock-domain bit missing.

### §15.5 Ruled out

- LTDC held in reset — no (`APB3RSTR=0`).
- LTDC bus clock off — no (`APB3ENR.LTDCEN=1` + `C1_APB3ENR.LTDCEN=1`).
- PLL3-R absent/unlocked — no (verified on + locked).
- "Free-running LTDC corrupts SDRAM" — **mis-diagnosis**; the 95%-noise
  framebuffer capture was only the CPU's *concurrent* reads contending
  with the LTDC, not the LTDC's display read path (probe free-run showed
  clean quads). The single-shot WCR workaround was reverted; free-running
  `WCR=0x0C` (rlvgl's value) is correct.

### §15.6 Leading hypothesis & NEXT STEP

The LTDC kernel/pixel clock (PLL3-R) is not effectively reaching the LTDC
register domain during run, even though PLL3 is locked and `DIVR3EN=1`
(debug-halt makes it visible). **Fastest path:** flash the known-working
**rlvgl disco binary** (owner has it; its image is the GRAM "gear") onto
this same board and **diff its live RCC + LTDC clock registers vs ours**
register-by-register over the probe — that reveals the missing bit
directly. Alternative: RM0399 §8 (RCC) + §33 (LTDC) kernel-clock-domain
study; check DBGMCU clock-keep-alive bits.

### §15.7 Pros / cons of techniques (for the next session)

**Worked well (reuse):**
- **Direct probe reads of peripheral registers** (`probe-rs read b32 <addr>`)
  — LTDC/DSI/RCC are debug-readable; far more reliable than firmware
  relays for diagnosis. *Peripherals are debug-readable; FMC/SDRAM are
  NOT* (that's why SDRAM uses the D3-SRAM relay).
- **Framebuffer capture → D3 SRAM → PNG** (`main_display.cpp`
  `capture_framebuffer()`): isolated the clean source from the snowy
  output — the decisive "it's downstream of SDRAM" finding.
- **Probe-writing the full LTDC config** to prove the chain end-to-end
  (panel lit → narrowed to firmware write-path).
- **Diffing our register config vs rlvgl source** caught the C1-gate
  omission.
- Bounded poll loops in `display.cpp` (no infinite hangs on bring-up).

**Pitfalls (avoid / cost us cycles):**
- **D3-SRAM relays are treacherous**: D3 SRAM **survives reset**, so a
  stale relay reads as if "current"; and a *faulting* relay routine
  leaves stale bytes. Multiple cycles chased stale/garbage relay values.
  → Prefer direct probe reads; add a fresh sentinel + clear before any
  relay; trust breadcrumbs only when they advance.
- **Wrong RM0399 offset**: read `0x28` (PLLCKSELR) as PLLCFGR (it's
  `0x2C`) → false "DIVR3EN=0". → Verify peripheral offsets.
- **"reads OK via probe" ≠ "firmware can access it"** — the halted debug
  probe sees a different clock state than the running CM7. THE key gotcha
  this session.
- **Source edits via `sed`/`python`** deleted `main()` → build break. →
  Use the Edit tool for surgical source changes, never stream editors.
- The 200 000× retry loop is slow when the LTDC actually responds (wait
  states) — re-read after a longer settle, and lower the cap.

### §15.8 Uncommitted code on disk (the resume state)

- `disco/clocks.hpp` — PLL3 27.5 MHz (KEEP).
- `disco/clocks.cpp` — `c1_apb3enr`/`c1_ahb1enr` CM7 gates (KEEP).
- `disco/display.cpp` — LTDC config in a write-verify **retry loop** with
  a D3 diag relay at `0x3800_0330` (tries/GCR/TWCR); free-running
  `WCR=0x0C`. The retry loop is debug scaffolding — simplify once fixed.
- `main_display.cpp` — `fill_pattern` + `capture_framebuffer` (48×80 →
  D3 `0x3800_1000`) + `relay_status`. Capture is a keeper utility.
- `disco/regs/{ltdc,dsi}.hpp`, `display.{hpp,cpp}`, `main_display.cpp`,
  `CMakeLists.txt` (the `_display` target) — the PLAT-02d implementation.
- D3-SRAM relay map in use: `0x3800_0300` breadcrumb; `0x3800_0320`
  DSI_PSR/LTDC_CDSR/DSI_WISR/marker; `0x3800_0330` LTDC retry diag;
  `0x3800_1000` 48×80 framebuffer capture.
- Build/flash: `cmake -S . -B build-disco -G Ninja -DCMAKE_TOOLCHAIN_FILE=
  cmake/toolchains/arm-none-eabi.cmake -DLVGLPP_PLATFORM_DISCO=ON
  -DLVGLPP_BUILD_TESTS=OFF -DLVGLPP_BUILD_EXAMPLES=ON` (PATH needs
  `~/toolchains/arm-gnu-15.2/bin`); `probe-rs download/reset --chip
  STM32H747XIHx`; target `lvglpp_stm32h747i_disco_display`.

### §15.9 Session 2 (2026-06-08) — diagnosis CORRECTED

**§15.3–§15.6 above were partly wrong: built on STALE D3 SRAM.** The
`ltdc_tries=200000` / "writes dropped" data was from a *pre-compaction*
run; D3 survives reset. Clean method: seed EVERY relay word with a
sentinel + verify the breadcrumb advanced. Also: `display::init()` takes
**~14 s** on the bench (panel delays + bounded-poll timeouts) — poll the
breadcrumb over ~15 s, never read once at 5 s.

**Firmware-authoritative diag** (relay the CM7's OWN reads right at LTDC
config time — added to `display.cpp`, relays at `0x3800_0340..036C`):
- PLL3 valid & locked at config time: `RCC_CR=0x3f03c025` (ON+RDY),
  `PLLCFGR=0x01230888` (`DIVR3EN=1`, bit 24 datasheet-confirmed),
  `DIVM3=5` → 5 MHz in → `PLL3_R=27.5 MHz`. **Clock CONFIG is correct.**
- CM7 write to benign `LTDC_BCCR` → reads back 0; CM7 reads `GCR` → 0.
- **Same run, CM7 writes the DSI fine** (reached PANEL_UP after DSI init).
- CM7 address map: reads `0x5000_0000`–`0x5000_0C00` (DSI) fine; reads
  `0x5000_1000`+ (LTDC) as **0**. Hard boundary at the LTDC page.

**Probe write test (clean, before any PLL3 fiddling) — THE key result:**
- `GCR` (0x18, **APB domain** `ltdc_pclk`): `0x2220→0x2221` ✓ sticks.
- `BCCR` (0x2C, **APB domain**): `0→0xABCDEF` ✓ sticks.
- `TWCR`/`SSCR` (**PIXEL domain** `ltdc_ker_ck`): writes never read back
  the written value (return 0 or stale-bus data) ✗.

**ROOT CAUSE (unified): `ltdc_ker_ck` (LCD_CLK = PLL3_R) does not reach
the LTDC pixel-clock domain.** Per RM0399 §33.3.3 the timing regs
(SSCR/BPCR/AWCR/TWCR) live in the pixel-clock domain; absent ker_ck their
accesses never complete (datasheet's exact diagnostic). This single fault
also explains the CM7 seeing the whole LTDC as 0 (its AXIM bus
transactions to the LTDC slave never complete without ker_ck; the debug
AHB-AP is more tolerant and reaches the APB-domain regs). It is
**PLL3-config-INDEPENDENT**: 27.5 MHz and 32 MHz both fail (32 MHz was
tried earlier — see `clocks.hpp:36`), live DIVR3EN-toggle and full PLL3
recycle don't help. PLL3 is locked with DIVR3EN=1 and there is no
separate LTDC kernel-clock mux (ker_ck is hardwired to pll3_r_ck).

**RULED OUT this session:** PLL3 config (both freqs), DIVR3EN bit
(=bit24, set), `LTDCEN` gate (APB3ENR & C1_APB3ENR both `0x18`,
firmware-confirmed), reset (`APB3RSTR=0`), cache/MPU (our firmware has
NONE — default Device map), CM7 bus PORT (datasheet: CM7 reaches both DSI
and LTDC via AXIM, not AHBP), and stale-D3 artifacts.

**rlvgl's CM7 drives the LTDC fully** (gear renders) with an
LTDC-equivalent clock tree (its live regs via gdb: PLL3=32 MHz,
APB3ENR/C1_APB3ENR=0x18, same as ours). So the fix is in *how* rlvgl
establishes ker_ck, not the PLL3 numbers.

**NEXT STEP:** rlvgl uses `stm32h7xx-hal`'s RCC `.freeze()` (with
`pll3_r_ck`) — diff its exact PLL3/LTDC clock programming SEQUENCE against
our hand-rolled `clocks.cpp` register-by-register (it may set a bit or
ordering our manual path misses). Also: ST errata sheet for an
H747 CM7↔LTDC / PLL3-R-to-LTDC quirk. Secondary fixes to land regardless:
DMA2D must gate on **AHB3 bit 4** (not AHB1) in `clocks.cpp`.

**Side findings (durable):** rlvgl's `*LPENR` offsets are wrong by +0x40
(RM0399: AHB3LPENR=0xFC, APB3LPENR=0x10C, C1_*=D+0x60 e.g.
C1_APB3LPENR=0x16C) — and rlvgl works anyway, so LPENR is not load-bearing
for LTDC. RM0399 PLLCKSELR DIVM shifts are 4/12/20 (our rcc.hpp is
correct). Reading rlvgl's debug-hostile (sleeping) binary: `probe-rs gdb
--connect-under-reset` + gdb `set mem inaccessible-by-default off` + bp at
`nt35510::init` (debug build @ 0x080193a8).

**Bench caution:** live PLL3 off/on via probe while the LTDC is mid-config
HANGS the LTDC bus interface (all reads freeze at the last bus value);
a chip reset recovers it.

**Uncommitted:** `display.cpp` now also carries the §15.9 firmware diag
(PLL3 self-read + BCCR write-back + CM7 address map at `0x3800_0340`).

### §15.10 Session 3 (2026-06-09) — TWO root causes found; both fixed

The §15.9 "ltdc_ker_ck absent" diagnosis was correct AND there is a second,
independent fault. Both are now understood; the panel still shows snow only
because of a third, narrower DSI-command-mode issue.

**ROOT CAUSE 1 — PLL3 enabled too late (FIXED in `clocks.cpp`).** Errata
ES0445 §2.13.1: "Device stalled when accessing LTDC registers while pixel
clock is disabled." Our clocks bring-up turned **PLL3 on AFTER the SYSCLK→
PLL1 switch** (old step 6). With PLL3 enabled after the clock-tree switch,
`pll3_r_ck` (= `ltdc_ker_ck`) never reached the LTDC. **Fix:** turn PLL1,
PLL2 **and PLL3** all on in step 4, BEFORE the SYSCLK switch — matching
`stm32h7xx-hal` `freeze()`. **Confirmed:** after the reorder the probe can
write LTDC PIXEL-domain registers (TWCR/SSCR), which it could not before.
PLL3 register *config* was never wrong (DIVR3EN=bit24 set, 27.5 MHz valid);
only the *ordering* relative to the SYSCLK switch.

**ROOT CAUSE 2 — CM7 reaches the LTDC only after `WCR.LTDCEN` (FIXED in
`display.cpp` ordering).** The running CM7 reads the whole LTDC as 0 / drops
writes when the config runs right after panel init; a settle delay alone
does NOT help. Setting the DSI wrapper `WCR.LTDCEN` (DSIEN|LTDCEN) + settle
is what opens CM7 access (adapted-command-mode quirk). **Confirmed:** with
WCR.LTDCEN set, the CM7 fully configures the LTDC — `TWCR=0x022504C3`,
`AWCR=0x0203042D`, `L1CFBAR=0xD0000000`, `L1CR=1` all latch from CM7 writes
(granular per-step relay).

**STILL OPEN — LTDC controller ENABLE hangs the scan (still snow).** With
ker_ck present and the LTDC fully configured, setting `GCR.LTDCEN=1`
permanently stalls the LTDC register bus (reads freeze at stale
`0x00000001`) and yields no valid frame → rainbow snow (owner-confirmed).
Ruled out: framebuffer location (hangs the same with fb at AXI-SRAM
`0x24000000` and SDRAM `0xD0000000`), IMR-vs-VBR shadow reload, and
WCR.LTDCEN free-run-vs-pulse-vs-cleared. This is a DSI-adapted-command-mode
scan/packetization problem, NOT clock/access. The "config latches but
enable hangs" split is the key clue.

**Next:** diff `configure_adapted_cmd_mode` + LTDC timing params + the DSI
command-mode regs (LCCR/WCFGR/CMCR/VMCR/TE) against rlvgl register-for-
register; validate LTDC timing (SSCR<BPCR<AWCR<TWCR); decide free-run vs
TE-gated single-shot for GCR.LTDCEN. Tooling that worked: granular per-step
D3 relays (write reg → relay readback) pinpointed exactly that config
latches but enable hangs.

### §15.11 Session 4 (2026-06-09) — ROOT CAUSE 3: WCFGR.AR=0; scan pipeline now cycling

The register-for-register diff had to target the right reference:
`display_init.rs` / `dsi_cmd_mode.rs` are the **Zephyr** path; the binary
gdb-verified scanning on this bench is the bare-metal inline sequence in
`rlvgl/platform/src/stm32h747i_disco.rs::new()`. Diffing against THAT
surfaced the one remaining wrapper-register delta:

**ROOT CAUSE 3 — `WCFGR.AR` (bit 6, automatic refresh) must be 1 at
GCR-enable time (FIXED in `display.cpp`).** The proven path sets
`WCFGR = DSIM|COLMUX(5)|TESRC|AR` (stm32h747i_disco.rs:529-537); we had
copied `AR=0` from `dsi_cmd_mode.rs` (Zephyr steady-state, where ERIF ISR
+ manual `present()` pulses replace AR). With AR=0, every enable variant
(§15.10 free-run/pulse/cleared) enabled the LTDC into a dead wrapper
handshake → bus wedge. With AR=1 the wrapper re-arms a frame transfer on
each panel TE event, so the LTDC always drains.

Also adopted from the proven path: AXI QoS (INI6 LTDC read=0xF,
INI5 DMA2D=0x4, stm32h747i_disco.rs:601-610) and the enable order
GCR=0x2221 → SRCR.IMR → ~20 ms settle → set WCR.LTDCEN (no WCR.LTDCEN
clear in between).

**Bench evidence (seeded-relay run, flash+reset, breadcrumb advanced to
PANEL_UP):**

- AR=1 alone opens CM7→LTDC access *before* any manual WCR.LTDCEN: the
  early diag now shows `BCCR=0x00ABCDEF` sticking and the TWCR
  pixel-domain write-discriminator alive (`0x0AFE0000`) — confirming
  ROOT CAUSE 2's mechanism (wrapper-gated LTDC access; TE auto-pulses
  provide it continuously).
- Post-enable: `WISR=0x7307` → TEIF=1 (panel TE arriving), ERIF=1
  (refreshes completing), BUSY=1 (transfer in flight). Live probe reads
  of WISR alternate 0x7303/0x7307 — BUSY toggling = continuous
  auto-refresh cycling. `init()` ran to completion; no bus wedge.
- LTDC registers read back `0x00000001`/garbage DURING active scan —
  matches the proven path's own observation (readbacks only "BEFORE GCR
  enable (no aliasing yet)", stm32h747i_disco.rs:700,733). Post-enable
  LTDC readback unreliability is expected on this part, not a fault.
  Do not diagnose from post-enable LTDC reads.

**Remaining deltas vs proven path (flagged, not adopted):** pixel clock
27.5 MHz w/ matching DSI timing consts (proven bare-metal: 32/32 — both
self-consistent and valid); LTDC timing written post-DSI-init under
explicit WCR.LTDCEN rather than blind pre-DSI writes.

**OWNER-CONFIRMED (2026-06-09): four distinct colours visible on the
panel** — the four-quadrant test pattern (`main_display.cpp::
fill_pattern`) is rendering. PLAT-02d first light achieved: clocks →
SDRAM framebuffer → LTDC → DSI adapted command mode → NT35510 panel,
end-to-end.

### §15.12 Geometry verification: two rlvgl-inherited off-by-ones found+fixed

Owner pattern-key readout (board-landscape viewing): quadrants exactly
matched the native-portrait scan rotated to landscape — **no
rotation/mirror error**. Convention note (owner-stated): "landscape" in
this initiative = the orientation natural for the joystick + button,
the same orientation rlvgl's UI uses; the panel scan itself is native
portrait (480×800, MADCTL=0x00).

The owner also saw the 1px white border on only two edges plus a 1px
colour bleed at the seam → two real bugs in `setup_ltdc_layer`,
inherited verbatim from rlvgl:

1. **Layer window origin +1 in both axes** (`x0 = HSW+HBP+1`): RM0399
   LxWHPCR requires WHSTPOS = AHBP+1 = HSW+HBP. The +1 shifted the
   layer, pushed the window stop past AAW/AAH, and clipped the
   framebuffer's last column/row (the missing border edges).
2. **`CFBLR.CFBLL = pitch+7`**: RM0399 says line length **+3**. The +7
   over-fetches one pixel per line (the next row's first pixel) — the
   1px seam bleed — and reads 4 bytes past the buffer on the last line.

Both fixed in `display.cpp::setup_ltdc_layer` (`x0 = HSW+HBP`,
`y0 = VSW+VBP`, `CFBLL = pitch+3`); **owner-confirmed aligned** after
reflash. rlvgl has the same bugs at three sites (L1 bare-metal, L1
Zephyr, L2 FreeRTOS) — upstream report with cites, bench evidence, and
suggested patch: `docs/platform-disco/rlvgl-ltdc-layer-off-by-one.md`.
The lvglpp deviation carries a PARITY-deviation comment until the rlvgl
fix lands and the submodule pin bumps.
