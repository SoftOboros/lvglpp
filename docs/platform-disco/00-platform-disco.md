# 00 — Platform: STM32H747I-DISCO

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAT-02** (family chapter).

## §0 Authority

- Vocabulary for clocks, PLLs, FMC/SDRAM, LTDC, DSI, DMA2D,
  GPIO/AFR mux, FT5336 touch protocol, and AXI bus arbitration is
  owned by **STMicroelectronics RM0399** (STM32H745/755 + H747/757
  Reference Manual). lvglpp does not redefine register-bit
  positions or mode codes; cite RM0399 §N.M when invoking them.
- Errata for the silicon (QSPI 2.8.5, etc.) are owned by
  **STMicroelectronics ES0392**.
- Bring-up sequence intent (clock tree, JEDEC SDRAM init, LTDC/DSI
  ordering, DMA2D admission, ERIF holdoff) mirrors
  `rlvgl/docs/disco-platform-guide/` (v0.2.0 @ 79f730d). When the
  two diverge, **rlvgl is canonical** and this family is the bug.
- The toolchain floor is `arm-none-eabi-gcc >= 11.0` with C++20
  (`-std=c++20 -fno-exceptions -fno-rtti`) per
  `LVGLPP_EMBEDDED_POSTURE=ON`. Cross-toolchain selection is owned
  by **PLAT-02a** (`docs/platform-disco/01-toolchain-and-reset.md`).
- Underlying widget surface: WID-01..WID-04. Underlying renderer
  trait: CORE-04. Underlying transport: PLAYIT-07.

## §1 Purpose

PLAT-02 ports the lvglpp runtime to a real Cortex-M7 board — the
**STM32H747I-DISCO** — using bare-metal C++20 against the RM0399
register surface. The board target is the canonical embedded
prong: it exercises every cross-cutting concern (clocks, external
SDRAM framebuffers, DSI panel bring-up, DMA2D acceleration, I2C
touch, USART playit transport, embedded posture under a real cross
compiler) that the host SDL backend cannot.

This chapter is the **family overview**: it freezes the phase set
(PLAT-02a..PLAT-02f), the source-of-truth map across rlvgl ↔
RM0399 ↔ lvglpp, and the cross-language ordering rule with rlvgl.
Each sub-phase has its own concepts doc (01..06) under
`docs/platform-disco/`.

## §2 Problem statement

The five gap-gallery items rlvgl Vol II Chapter 1 catalogues all
apply identically to lvglpp because the silicon is the same:

1. **PLL3ON gap** — `stm32-hal` style PLL helpers historically
   omit a single bit in `RCC_CR`. The DSI pixel clock (PLL3R) goes
   unenabled; the panel never receives a clock. Mirrors
   `rlvgl/examples/stm32h747i-disco/src/main.rs` L1569–1596 raw poke.
2. **FMC SDTR offset** — vendor HAL writes to the wrong word in the
   SDRAM timing register. Mirrors `main.rs` L1034–1132.
3. **I2C blocking** — vendor HAL blocks the calling task during a
   transfer. At 120 Hz touch polling on a cooperative loop this
   eats every spare millisecond. Mirrors `main.rs` L86–281.
4. **QSPI errata 2.8.5** — silicon bug ES0392; needs a raw write
   to `D1CCIPR`. Mirrors `main.rs` L1711–1770.
5. **D-cache transparency** — DMA2D and CPU disagree on framebuffer
   contents until an explicit clean-by-MVAC. Mirrors
   `main.rs`+`platform/src/dma2d.rs`.

lvglpp inherits each gap unchanged. It does not pick a vendor HAL;
it speaks RM0399 directly, with the **same raw fixes** rlvgl already
shipped, expressed in C++ via typed register-block headers
(`#[repr(C)]`-equivalent: `struct alignas(4)` + `static_assert
(offsetof(...) == 0x..)` per CLAUDE.md § "Strict and Explicit
Ownership").

## §3 Canonical glossary

- **Disco backend** — Owned by this family. The lvglpp
  `lvglpp::platform` library, when configured with
  `LVGLPP_PLATFORM_DISCO=ON`, owns clocks/SDRAM/LTDC/DSI/DMA2D/
  touch/USART bring-up for the STM32H747I-DISCO. Header:
  `platform/include/lvglpp/platform/disco.hpp` (mirrored once
  PLAT-02b lands).
- **CM7-only build** — lvglpp targets the CM7 (480 MHz, D-cache,
  FPU). The CM4 is held in deep sleep at reset (the BootCM4 flag
  cleared on the CM7 side). Mirrors rlvgl's default CM7 boot.
  Multi-core is **out of scope** for PLAT-02 — see §11.
- **Embedded posture** — As defined in `docs/std-mapping.md`
  § "Embedded posture". Bare-metal disco builds set
  `LVGLPP_EMBEDDED_POSTURE=ON` mandatorily.
- **MMIO register block** — Per CLAUDE.md § "Strict and Explicit
  Ownership", every MMIO peripheral is expressed as a `struct
  alignas(4)` with `volatile` field types and `static_assert`
  offset checks. Wrapping happens in `platform/src/disco/regs/`;
  no inline `(volatile uint32_t*)0x….` casts in widget or app
  code. Mirrors rlvgl's "Register-Mashing Discipline".
- **Address domain types** — `MmioAddr<T>`, `PhysAddr`, `DmaAddr`
  in `platform/include/lvglpp/platform/disco/addr.hpp`. Mirrors
  rlvgl `platform::hwcore::addr` (CLAUDE.md, rlvgl side, "Three
  address domains, three types").
- **`FrameBuffer` / `FrontBuffer` / `BackBuffer`** — Typed
  framebuffer ownership handles (RAII over the SDRAM region a
  scan-line read or DMA2D write borrows). Mirrors rlvgl's
  `platform::framebuffer` typed handles. Owned by **PLAT-02d**.
- **`InFlight<T>`** — DMA2D in-flight token. Borrows the
  destination `FrameBuffer` for the duration of a transfer; CPU
  access during a transfer is a compile error. Mirrors rlvgl's
  `InFlight<'dma, T>`. Owned by **PLAT-02e**.
- **ERIF holdoff** — The pattern that keeps DMA2D from racing the
  LTDC scan line on the AXI bus. Mirrors rlvgl Vol II Ch 5
  + Ch 7. Owned by **PLAT-02d/e**.
- **FT5336 touch** — Capacitive controller on I2C4 @ 0x38, INT on
  PK7. Mirrors rlvgl `touch_i2c.rs`. Owned by **PLAT-02f**.

## §4 Source-of-truth map

| Concept | Canonical owner | lvglpp mirror |
| --- | --- | --- |
| Clock tree (HSE → PLL1/2/3 → SYSCLK/HCLK/PCLK) | RM0399 §8 + `rlvgl/.../main.rs:1569` | `platform/src/disco/clocks.cpp` (PLAT-02b). |
| FMC SDRAM init (JEDEC sequence, SDCR/SDTR) | RM0399 §22 + `rlvgl/.../main.rs:1034` | `platform/src/disco/sdram.cpp` (PLAT-02c). |
| GPIO AF mux for FMC + LTDC + DSI | RM0399 §8 + `rlvgl/.../main.rs:1643` | `platform/src/disco/pinmux.cpp` (PLAT-02b). |
| LTDC + DSI bring-up + OTM8009A wake | RM0399 §32 + §33 + `rlvgl/.../main.rs:335` | `platform/src/disco/display.cpp` (PLAT-02d). |
| DMA2D engine + ERIF admission gating | RM0399 §16 + `rlvgl/platform/src/dma2d.rs` | `platform/src/disco/dma2d.cpp` (PLAT-02e). |
| FT5336 I2C4 state machine + TIM6 polling | FT5336 datasheet + `rlvgl/.../touch_i2c.rs` | `platform/src/disco/touch.cpp` (PLAT-02f). |
| USART1 raw register init for playit | RM0399 §49 + `rlvgl/.../main.rs:1823` | `platform/src/disco/usart.cpp` (PLAT-02f). |
| Reset vector + linker memory layout | rlvgl `memory.x` (canonical) | `examples/disco/memory.ld` + `cmake/toolchains/arm-none-eabi.cmake` (PLAT-02a). |

## §5 Frozen decisions

### §5.1 Phase set — **Standards Action**

PLAT-02 lands in six sub-phases. Each ships its own concepts doc
under `docs/platform-disco/0N-<topic>.md`; the codes below are the
frozen names referenced in commits, STATUS, and acceptance gates.

| Code | Title | Concepts doc | Acceptance |
| --- | --- | --- | --- |
| **PLAT-02a** | Toolchain, memory map, reset vector | `01-toolchain-and-reset.md` | Cross-build emits a flashable `.elf` with a valid Cortex-M7 vector table; halts at `Reset_Handler` under probe-rs. |
| **PLAT-02b** | Clocks, PLLs, GPIO pin mux | `02-clocks-and-pinmux.md` | SYSCLK = 400 MHz (PLL1_P); HCLK = 200 MHz; PLL3_R = 32 MHz drives the DSI/LTDC pixel clock with `PLL3ON` explicitly latched; FMC + LTDC + DSI pins muxed AF12 at VeryHigh speed. Verified by reading `RCC_CR` (bits 28/29) + `GPIOx_AFRH` from a halted target. |
| **PLAT-02c** | SDRAM (FMC Bank1) | `03-sdram-and-fmc.md` | 32 MB at `0xC000_0000` is read/write-stable; the JEDEC init sequence completes; raw SDTR offset fix applied. Verified by a 32-pattern memtest sweep. |
| **PLAT-02d** | LTDC + DSI + OTM8009A | `04-ltdc-and-dsi.md` | 800×480 @ 60 Hz; first `Label` rendered into a SDRAM front buffer scans out cleanly. ERIF deadline observed; no panel snow. |
| **PLAT-02e** | DMA2D + ERIF gating | `05-dma2d-engine.md` | `fill_rect` + `blend_rect` accelerated by DMA2D; `InFlight<T>` token gates CPU access; admission-control test passes under deliberate scan-line pressure. |
| **PLAT-02f** | Touch (FT5336/I2C4) + USART (playit) | `06-touch-and-uart.md` | A piped `T@<tag>:x,y` over USART1 toggles the on-screen Button. Touching the panel does the same via `GesturePipeline`. |

Adding a phase requires a change-log amendment to this file +
explicit go-ahead. Cross-language change ordering (CLAUDE.md): if
a sub-phase touches the rlvgl ↔ lvglpp wire protocol, rlvgl
amends first.

### §5.2 Build artifact + binary name — **Specification Required**

The CM7 binary is `lvglpp-stm32h747i-disco`, mirroring rlvgl's
`rlvgl-stm32h747i-disco`. The CMake target name is
`lvglpp_stm32h747i_disco`. Both live under
`examples/stm32h747i-disco/`. Probe-rs chip id:
**`STM32H747XIHx`** (matches rlvgl's CLAUDE.md).

### §5.3 Multi-core posture — **Standards Action**

PLAT-02 builds **CM7-only**. The CM4 boots into a wfi loop owned
by the bootloader; lvglpp does not bring up the CM4 in this
family. A future PLAT-02g may add a CM4 mailbox seam, but it is
**not in PLAT-02's acceptance gate**. The canonical CM4 path lives
in rlvgl `examples/stm32h747i-disco/src/cm4_main.rs` and lvglpp
will mirror it whenever a real consumer needs it.

### §5.4 RTOS posture — **Standards Action**

PLAT-02 is **bare-metal cooperative**. No FreeRTOS, no Zephyr.
The main loop is a stateful event-pump that pumps DSI presents,
DMA2D completions, USART RX, and tree dispatch in cooperative
order. rlvgl's FreeRTOS prong (`docs/disco-freertos-guide/`) and
Zephyr prong (`docs/disco-zephyr-guide/`) are out of scope here;
each would land as its own PLAT-NN family if/when needed.

### §5.5 Register-block discipline — **Standards Action**

Every MMIO peripheral lvglpp brings up MUST:

1. Live under `platform/include/lvglpp/platform/disco/regs/`
   as a `struct alignas(4)` with `volatile` field types and a
   `static_assert(offsetof(Block, field) == 0x…, "RM0399 §N.M")`
   line for every documented offset. Hand-offset pointer
   arithmetic is forbidden.
2. Be accessed only through an `MmioAddr<Block>` typed handle
   from `platform/include/lvglpp/platform/disco/addr.hpp`. Bare
   `reinterpret_cast<volatile uint32_t*>(0x…)` is banned outside
   the regs layer.
3. Carry an ownership comment per CLAUDE.md
   (`// mmio: owned by RM0399 §N.M; never freed.`).
4. Be written through a typed accessor — `block.cr.modify([](auto
   v){ v.set_pll3on(); return v; })` style — not through field-by-field
   bit-shifts in caller code.

The point mirrors rlvgl Step 5 of its register-mashing discipline:
**the wrong RM0399 offset becomes a compile-time failure.**

### §5.6 Embedded posture — **Standards Action**

`LVGLPP_EMBEDDED_POSTURE=ON` is **mandatory** for any
`lvglpp_stm32h747i_disco*` target. The toolchain file
(PLAT-02a) sets it implicitly when it selects the
`arm-none-eabi-*` triple; an explicit
`-DLVGLPP_EMBEDDED_POSTURE=OFF` on a cross build is a
configuration error (`message(FATAL_ERROR)`).

## §10 Reconciliation vs. adjacent primitives

- **PLAT-01 host SDL.** Same `lvglpp::core::Renderer` subclass
  shape — `DiscoRenderer` overrides `fill_rect` (DMA2D-accelerated,
  PLAT-02e) and `draw_text` (font goes through SDRAM blits). The
  SDL backend's host posture is **mutually exclusive** with the
  disco target by CMake convention; both libraries can compile in
  parallel under a `LVGLPP_PLATFORM_*` umbrella, but no single
  target combines them.
- **rlvgl `disco-platform-guide` Vol II.** Per-chapter mapping
  (Ch 2 ↔ PLAT-02b, Ch 3 ↔ PLAT-02c, etc.). Where a chapter
  bundles multiple concerns (Ch 8 "secondary peripherals" mixes
  QSPI + USART + SAI + backlight), lvglpp splits along the
  acceptance-gate boundary: USART belongs in PLAT-02f because
  playit transport gates the diagnostic loop; QSPI/SAI/backlight
  defer to a follow-up family.
- **PLAYIT-07 transport.** Disco USART1 backend is a concrete
  `Transport` subclass (`UsartTransport`) registered with the
  `Executor`. PLAT-02f `usart.cpp` IS that subclass.
- **CORE-04 Renderer trait.** No new methods on the trait; the
  disco backend implements the existing surface. If a real
  consumer needs a `present()` hook (e.g. front/back swap), it
  lands in CORE-04 first — PLAT-02 does not amend the renderer
  trait unilaterally.
- **WID-01..WID-04 widgets.** All four widgets MUST render
  identically on the disco target as on the host SDL backend.
  The cross-language test loop (PLAYIT-04 + PLAYIT-04a) — pipe
  the same fixture into both targets, observe identical
  recorder output — is the acceptance proof.

## §11 Non-goals

- **CM4 bring-up.** CM7-only for PLAT-02. CM4 is parked in wfi.
- **FreeRTOS / Zephyr ports.** Bare-metal cooperative only.
- **QSPI flash, SAI audio, SDMMC SD card, backlight PWM.**
  Belong to a follow-up family (PLAT-02-secondary or split per
  peripheral). Not on the PLAT-02 acceptance critical path.
- **CubeMX `.ioc` import.** rlvgl-creator owns the YAML → BSP
  generator path. lvglpp consumes generated BSPs eventually but
  the disco bring-up is hand-written first to expose the gaps the
  generator must close (per CLAUDE.md § "Things That Are NOT
  Goals For lvglpp").
- **Generated lvglpp BSPs.** Deferred to creator-cpp follow-up.
- **Other STM32 boards.** Each board target lives in its own
  PLAT-NN family.

## §12 Acceptance checklist

- [ ] `docs/platform-disco/01-toolchain-and-reset.md` ratified;
      PLAT-02a execution landed (cross-build emits a flashable
      `.elf` halting at `Reset_Handler`).
- [ ] `docs/platform-disco/02-clocks-and-pinmux.md` ratified;
      PLAT-02b execution landed.
- [ ] `docs/platform-disco/03-sdram-and-fmc.md` ratified;
      PLAT-02c execution landed (32 MB SDRAM stable).
- [ ] `docs/platform-disco/04-ltdc-and-dsi.md` ratified;
      PLAT-02d execution landed (Label visible on panel).
- [ ] `docs/platform-disco/05-dma2d-engine.md` ratified;
      PLAT-02e execution landed.
- [ ] `docs/platform-disco/06-touch-and-uart.md` ratified;
      PLAT-02f execution landed.
- [ ] `examples/stm32h747i-disco/` builds clean under both
      `Debug` and `MinSizeRel` cross profiles with `-Werror`.
- [ ] `lvglpp_stm32h747i_disco.elf` boots under probe-rs
      (`probe-rs run --chip STM32H747XIHx`) and does not panic.
- [ ] Cross-language wire-protocol parity: pipe
      `T@dark_mode:300,90\nQE:dark_mode\nRD\n` into both rlvgl
      and lvglpp disco targets via USART1; recorder output diffs
      cleanly without a normaliser.
- [ ] `platform/STATUS.md` change log records each sub-phase
      landing, dated, with a one-line as-built note.

## §13 Files cited

- `rlvgl/docs/disco-platform-guide/README.md` (Vol II index,
  v0.2.0 @ 79f730d).
- `rlvgl/examples/stm32h747i-disco/{HARDWARE,MEMORY,BOOT,BRINGUP}.md`.
- `rlvgl/examples/stm32h747i-disco/memory.x`.
- `rlvgl/examples/stm32h747i-disco/src/main.rs` (line ranges per
  §4 source-of-truth map).
- `rlvgl/platform/src/dma2d.rs`, `display_init.rs`, `touch_i2c.rs`.
- STMicroelectronics RM0399 (STM32H745/755 + H747/757 RM).
- STMicroelectronics ES0392 (STM32H747xI errata).
- `lvglpp/docs/platform-host-sdl/00-host-sdl-backend.md`
  (PLAT-01, sibling backend).
- `lvglpp/docs/playit-transport/00-transport-and-executor.md`
  (PLAYIT-07, transport seam consumed by PLAT-02f).
- `lvglpp/docs/std-mapping.md` § "Embedded posture".

## §14 Unblocks

- A real cross-build exercises `LVGLPP_EMBEDDED_POSTURE=ON` end
  to end. The freestanding-subset header allowlist becomes
  enforced in practice, not just by inspection.
- The diagnostic harness (`Executor` + `EventRecorder`) has a
  hardware target. Piped fixtures drive lvglpp on real silicon
  identically to the host-SDL demo and to rlvgl's CM7 binary.
- WID-05+ widgets land with **two** acceptance gates from day
  one — host SDL and disco — so the C++ ↔ Rust parity test loop
  remains intact as the surface grows.
- PLAT-03 (BBB Linux DRM) and PLAT-04 (ESP32 LCD) gain a worked
  example for the "cross-toolchain + linker + reset" boilerplate
  in PLAT-02a; subsequent boards mostly copy and re-target.

## §15 Change log

- 2026-04-27 — Family chapter ratified at draft level. Phase set
  (§5.1), build artifact name (§5.2), CM7-only posture (§5.3),
  bare-metal cooperative posture (§5.4), register-block
  discipline (§5.5), embedded-posture-mandatory rule (§5.6) all
  frozen.
- 2026-04-27 — PLAT-02c address-map clarification: the disco's
  IS42S32800J SDRAM is wired to **FMC Bank 2** (SDCKE1/SDNE1),
  hardware-mapped at **`0xD000_0000`** per RM0399 §22.6. The
  earlier informal references to `0xC000_0000` (Bank 1) inherited
  from rlvgl's `memory.x` linker comment are *decorative* — rlvgl
  never actually links anything to that region; the bring-up code
  uses 0xD000_0000. lvglpp follows suit. memory.ld region kept at
  0xC000_0000 for byte-for-byte rlvgl parity (visibility-only;
  no section places into it); actual access goes to 0xD000_0000.
- 2026-04-27 — §5.1 PLAT-02b acceptance amended to spell the
  clock targets canonically (SYSCLK = 400 MHz, HCLK = 200 MHz,
  PLL3_R = 32 MHz) — matches rlvgl's shipped numbers
  byte-for-byte. The earlier "SYSCLK = 480 MHz" wording was
  aspirational (H7 silicon supports 480 MHz on rev Y/V) but
  diverged from the canonical bring-up; corrected before
  PLAT-02b execution begins.
