<!--
STATUS.md — Co-located status block for lvglpp::platform.
Canonical shape: see CLAUDE.md § "Doc Co-Location Policy".
-->

# lvglpp::platform — STATUS

Tracks `rlvgl/platform` @ `v0.2.5` (commit `f999f75`). Last reconciled:
2026-06-29.

## Roadmap intent

`lvglpp::platform` ports the rlvgl backends one at a time. Each backend
mirrors its rlvgl counterpart in shape (display init, input dispatch,
storage seam) and uses upstream LVGL drivers underneath.

Phase plan (parallel-portable, mirroring rlvgl's "four prong" approach
where applicable):

1. **PLAT-01:** Host SDL backend. Smallest target; smoke-tests the
   display driver registration and pointer/keyboard input dispatch
   under host posture.
2. **PLAT-02:** STM32H747I-DISCO backend (LTDC + DSI + DMA2D). The
   canonical embedded target. Mirrors `rlvgl/examples/stm32h747i-disco`.
   Family chapter: `docs/platform-disco/00-platform-disco.md`.
   Six sub-phases:
   - **PLAT-02a:** Toolchain + memory map + reset vector
     (`docs/platform-disco/01-toolchain-and-reset.md`).
   - **PLAT-02b:** Clocks, PLLs, GPIO pin mux.
   - **PLAT-02c:** SDRAM (FMC Bank 1). **Done — hardware-verified
     2026-06-08** (full address/data/range memtest PASS).
   - **PLAT-02d:** LTDC + DSI + OTM8009A. **In progress (bench session 1,
     2026-06-08).** Chain proven working (probe-written LTDC config →
     clean image on panel); blocked on an LTDC clock-domain access bug
     (LTDC unreachable by the running CM7). Full log + learnings:
     `docs/platform-disco/04-ltdc-dsi-and-panel.md` §15.
   - **PLAT-02e:** DMA2D + ERIF gating.
   - **PLAT-02f:** FT5336 touch + USART1 playit transport.
3. **PLAT-LNX:** generic Linux fbdev+evdev layer
   (docs/platform-linux/00-fbdev-evdev.md) — cross-cutting OS
   chapter outside the per-board numbering; first consumer is an
   external fbdev repo. Landed 2026-06-10.
4. **PLAT-03:** BeagleBone Black + NHD cape. Consumes PLAT-LNX and
   adds board specifics only. (Note: rlvgl's generic layer is
   fbdev, not DRM — earlier DRM wording was aspirational.)
4. **PLAT-04:** ESP32 LCD (LCDC + I2C touch). Mirrors
   `rlvgl-chips-esp` BSP consumption.

## As-built

Implemented (PLAT-01 — landed 2026-04-27):

- `lvglpp::platform::HostSdlBackend` (RAII over `SDL_Window` +
  `SDL_Renderer`, single-instance flag mirroring `Runtime`).
  Move-construct allowed; move-assign deleted.
- `lvglpp::platform::SdlRenderer` — first concrete
  `lvglpp::core::Renderer` subclass. Implements `fill_rect` via
  `SDL_RenderFillRect`; `draw_text` via
  `lvglpp::core::fonts::FONT_6X10.draw_str` (translates
  baseline-anchor to top-left). Default `blend_rect` /
  `draw_pixels` inherited.
- `HostSdlBackend::poll_event()` returns
  `std::optional<lvglpp::core::Event>` per the §5.4 translation
  table (mouse → Pointer{Down,Up,Move}; keyboard → Key{Down,Up}
  with the named-key / Function / Character / Other variants).
- CMake target gated on `LVGLPP_PLATFORM_HOST_SDL` option (default
  OFF). When ON, `find_package(SDL2 REQUIRED)` runs; if missing,
  emits a clear "install SDL2" error. Embedded posture +
  HOST_SDL=ON is a configuration error (`message(FATAL_ERROR)`).
- Header `host_sdl.hpp` `#error`s under
  `LVGLPP_EMBEDDED_POSTURE` to enforce the host-only contract.
- Example `examples/host_sdl_label/` runs a Label in an SDL
  window; quits on close, Esc, or Q.

Implemented (PLAT-02a — landed 2026-04-27):

- `cmake/toolchains/arm-none-eabi.cmake` selects the GNU Arm
  Embedded Toolchain (PATH + Homebrew/apt fallbacks), Cortex-M7
  + fpv5-d16 hard-float flag bundle, forces
  `LVGLPP_EMBEDDED_POSTURE=ON`, and sets
  `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY` so freestanding
  compiler probes succeed.
- `examples/stm32h747i-disco/memory.ld` declares all eight
  STM32H747XI regions (FLASH 1024K, DTCM RAM 128K, D1 AXI 384K
  CM7 / 128K CM4, MAILBOX 1K, D3 SRAM4 64K, QSPI 64M, SDRAM 32M).
  Region origins/lengths match `rlvgl/.../memory.x` byte-for-byte.
- `examples/stm32h747i-disco/disco.ld` defines the section layout
  per PLAT-02a §5.4: `.isr_vector` first in FLASH, `.text` /
  `.rodata` / `.ARM.exidx`, C++ pre-init / init / fini arrays,
  `.data` (LMA in FLASH, VMA in RAM), `.bss`, heap landing slot,
  `_estack` at the top of DTCM.
- `examples/stm32h747i-disco/startup_stm32h747xi.cpp` — full 240
  -slot vector table (RM0399 §11.1.4): MSP, `Reset_Handler`, six
  named system handlers, `SVC` / `DebugMon` / `PendSV` / `SysTick`,
  every STM32H7 peripheral IRQ slot weak-aliased to
  `Default_Handler`. `Reset_Handler` is a naked trampoline that
  sets MSP and jumps to `__lvglpp_disco_reset_main`, which copies
  `.data` LMA→VMA, zeros `.bss`, walks `.init_array` directly
  (avoiding newlib's `__libc_init_array`→`_init` chain), and
  calls `main()`. Default fault handler loops on `bkpt 0` per
  rlvgl's "faults break in place" recommendation.
- `examples/stm32h747i-disco/main_smoke.cpp` — minimal `main()`
  loops on `wfe`. CMake target `lvglpp_stm32h747i_disco_smoke`,
  output `lvglpp-stm32h747i-disco-smoke.elf`. `.bin` + `.hex`
  emitted post-build alongside `arm-none-eabi-size` summary.
- Top-level CMakeLists gated on `LVGLPP_HOST_BUILD` (derived from
  `CMAKE_SYSTEM_NAME == "Generic"`): on a cross build, `lvgl/` +
  per-module libs + tests are skipped; only the disco example
  links the `lvglpp_warnings` + `lvglpp_posture` INTERFACE libs.
  Subsequent sub-phases will broaden the cross surface as
  individual modules need on-target paths.
- Verified end-to-end on macOS 13 with the STM32CubeIDE-bundled
  arm-none-eabi-gcc 13.3.1: cross configure + build clean,
  `arm-none-eabi-readelf -h` reports `Class=ELF32, Machine=ARM,
  Entry=0x08000319` (Reset_Handler + Thumb bit), vector table
  at `0x08000000` opens with `0x20020000` (`_estack`) /
  `0x08000319` (Reset). FLASH = 828 B, RAM = 0 B, BSS = 0 B —
  inside the §12 sanity bound.

- PLAT-02b clocks + GPIO pinmux (`disco/clocks.{hpp,cpp}` +
  `disco/pinmux.{hpp,cpp}`), hardware-verified 2026-04-27.
- PLAT-02c SDRAM (`disco/sdram.{hpp,cpp}` + `disco/regs/{fmc,mpu}.hpp`),
  hardware-verified 2026-06-08 (full memtest PASS).

Stubbed:

- PLAT-02d..f (LTDC/DSI/OTM8009A/DMA2D/touch/USART). Each has
  its own concepts doc placeholder under `docs/platform-disco/`.
- PLAT-03 (BBB Linux DRM), PLAT-04 (ESP32 LCD).

## Blockers

- **PLAT-02b clocks-and-pinmux concepts doc.** Owner: next disco
  sub-phase implementer. PLAT-02a landed without bringing up clocks;
  the chapter must freeze the PLL1/2/3 plan + the GPIO AF12 flood
  before any peripheral code can land.
- ~~**Hardware-in-loop probe-rs run.**~~ Resolved 2026-04-27 —
  PLAT-02a + PLAT-02b both verified on the attached
  STM32H747I-DISCO via probe-rs 0.29.1 + ST-LINK V3. See change
  log entries.
- **PLAT-03 / PLAT-04 concepts docs.** Owner: respective
  board-bring-up implementer. None exist yet.

## Definitions

- **`HostSdlBackend`** — As defined in
  `platform/include/lvglpp/platform/host_sdl.hpp` (this repo).
  Authoritative chapter:
  `docs/platform-host-sdl/00-host-sdl-backend.md`.
- **`SdlRenderer`** — As defined in
  `platform/include/lvglpp/platform/host_sdl.hpp`. Mirrors the
  `Renderer for ...` impl shape in
  `rlvgl/platform/src/pixels_renderer.rs:80` *adapted*: lvglpp's
  text rendering goes through the bring-up bitmap font instead of
  embedded-graphics' MonoTextStyle.
- **Backend** — Owned by chapters PLAT-0N. Each backend MUST be a
  `lvglpp::core::Renderer` subclass and MUST NOT call into `lv_*`
  directly outside the backend translation unit.
- **Embedded posture** — As defined in `docs/std-mapping.md`
  § "Embedded posture"; that document is the authoritative source for
  what is and isn't allowed (no exceptions, no RTTI, panic ≡ abort).
- **`arm-none-eabi` toolchain file** — As defined in
  `cmake/toolchains/arm-none-eabi.cmake` (this repo). Authoritative
  chapter: `docs/platform-disco/01-toolchain-and-reset.md`. The
  `LVGLPP_ARM_CPU` / `LVGLPP_ARM_FPU` / `LVGLPP_ARM_FLOAT_ABI` cache
  vars default to Cortex-M7 + fpv5-d16 hard-float; other Cortex-M
  boards override them.
- **STM32H747I-DISCO smoke target** — As defined in
  `examples/stm32h747i-disco/` (this repo). Authoritative chapter:
  `docs/platform-disco/01-toolchain-and-reset.md`. CMake target
  name: `lvglpp_stm32h747i_disco_smoke`; output ELF:
  `lvglpp-stm32h747i-disco-smoke.elf`. Probe-rs chip id:
  `STM32H747XIHx`.
- **`Reset_Handler` / `Default_Handler`** — As defined in
  `examples/stm32h747i-disco/startup_stm32h747xi.cpp`. Mirrors
  `cortex-m-rt`'s `Reset` shape; *adapted*: walks `.init_array`
  directly instead of calling newlib's `__libc_init_array` so we
  don't pull `_init`. Default fault handler loops on `bkpt 0`,
  matching rlvgl `BOOT.md` § "Fault trapping".
- **`Screen` / `Rotation` / `ColorFormat` / `DEFAULT_FRAME_HZ`** — As
  defined in `rlvgl/platform/src/screen.rs:142,40,77,137`; mirrored here
  as `platform/include/lvglpp/platform/screen.hpp`. Full five-field
  descriptor (`width`, `height`, `rotation`, `color_format`, `frame_hz`).
  `color_format` / `frame_hz` are host-advisory for this initiative.
  DELTA: Rust `new`/builders → C++ `static make` + `const` builder
  methods; enums → `enum class`; `ColorFormat` is the DEMO-0S frozen
  4-variant set (rlvgl's `Rgb444`/`L8` out of scope here);
  `ColorFormat::quantize` deferred until a consumer needs it.
  Authoritative chapter: `docs/disco-demo/0S-screen-descriptor.md`.

## Change log

- 2026-06-07 — DEMO-0S execution landed. Mirrored the rlvgl `Screen`
  display descriptor into `platform/include/lvglpp/platform/screen.hpp`
  as a header-only `constexpr` trivially-copyable value type plus the
  `Rotation` and `ColorFormat` `enum class`es and `DEFAULT_FRAME_HZ`.
  `Screen::make` / `landscape` static factories + `with_color_format` /
  `with_frame_hz` `const` builders; defaulted `constexpr operator==`.
  Builds without SDL and under embedded posture (no LVGL/SDL include).
  `ColorFormat::quantize` deferred (no consumer yet). New
  `platform/tests/` dir wired via `add_subdirectory(tests)` gated on
  `LVGLPP_BUILD_TESTS` only (not on `LVGLPP_PLATFORM_HOST_SDL`); the
  SDL-independent test links the INTERFACE umbrella `lvglpp::platform`
  for its include path. New ctest `lvglpp_platform_screen` (20/20 green).
  `platform.hpp` umbrella now includes `screen.hpp` unconditionally.
  Per DEMO-0S §2: `Rotation` + `ColorFormat` variant sets and
  `DEFAULT_FRAME_HZ` are FROZEN (Standards Action — cross-language).
- 2026-04-27 — Initial scaffold. INTERFACE target only; no backends.
- 2026-04-27 — PLAT-01 chapter ratified at
  `docs/platform-host-sdl/00-host-sdl-backend.md`. Backend ownership
  shape (§5.1), window/renderer config (§5.2), Renderer overrides
  (§5.3), SDL→Event translation (§5.4), embedded-posture exclusion
  (§5.5) all frozen.
- 2026-04-27 — PLAT-01 execution landed. `lvglpp::platform` is now
  a conditionally-compiled library: INTERFACE umbrella when no
  backend selected, compiled lib when a backend (today, only
  HOST_SDL) is enabled. `lvglpp::platform::HostSdlBackend` +
  `SdlRenderer` defined; `examples/host_sdl_label/` ships as the
  smoke target. `LVGLPP_PLATFORM_HOST_SDL` defaults OFF; OFF-path
  build is unchanged (9/9 ctest entries still green); ON-path
  emits an actionable "install SDL2" error when SDL2 is missing.
- 2026-04-27 — PLAT-02 family chapter ratified
  (`docs/platform-disco/00-platform-disco.md`). Phase set
  PLAT-02a..f (§5.1), CM7-only posture (§5.3), bare-metal
  cooperative posture (§5.4), register-block discipline (§5.5),
  embedded-posture-mandatory rule (§5.6) all frozen.
- 2026-04-28 — rlvgl pin advanced on `v0.2.0`:
  `b178cbc` → `79f730d` (1 commit). Upstream `DISCO-03: SDCLK
  0b01 → 0b10 — fix SDRAM column-aliasing` lands the same fix
  lvglpp identified via memalpha (RM0399 Rev 4 §23.9.5.1):
  `FMC_SDCR1.SDCLK[1:0] = 0b01` is **Reserved**, not the
  silicon-required value the original CubeMX-derived comment
  claimed. The documented `/2` divider is `0b10`. Touches
  `rlvgl/examples/stm32h747i-disco/src/main.rs:1192-1193` +
  `rlvgl/docs/disco-platform-guide/03-sdram-and-fmc.md`. Per
  CLAUDE.md § "Cross-language change ordering": rlvgl
  amendment (`79f730d`) lands first; lvglpp's
  `disco/regs/fmc.hpp::sdcr::SDCLK_DIV2 = 0b10 << 10` (already
  applied 2026-04-28 with memalpha-cited audit comment) is the
  mirror. Bulk SHA refresh across 64 lvglpp files
  (`b178cbc` → `79f730d`).
- 2026-06-08 — **PLAT-02d bench session 1 (LTDC/DSI/panel).** Landed +
  flashed `disco/regs/{ltdc,dsi}.hpp`, `disco/display.{hpp,cpp}`,
  `main_display.cpp`, `lvglpp_stm32h747i_disco_display` target (faithful
  mirror of `rlvgl/platform/src/display_init.rs` + `nt35510`). On the
  bench: framebuffer fill, DSI host/PHY/PLL, panel init, and PLL3-R pixel
  clock all PROVEN good — probe-writing the LTDC config lights the panel
  with the clean four-quadrant pattern. **BLOCKER:** the LTDC is
  inaccessible to the *running* CM7 (register reads return 0, writes
  don't latch) though fine to the *halted* probe (reads 0x2220) — so the
  firmware never configures it → no scan → DSI sends garbage → rainbow
  snow. Two real clock fixes found & applied (uncommitted): PLL3 32→27.5
  MHz (rlvgl parity) and the missing `C1_APB3ENR`/`C1_AHB1ENR` CM7 clock
  gates for LTDC/DSI/DMA2D (FMC already set both; the file's own comment
  mandates it) — verified landed but NOT sufficient; ≥1 more clock-domain
  bit is missing. NEXT: flash the working rlvgl disco binary on this board
  and diff its live RCC+LTDC clock registers vs ours. Full log, ruled-out
  list, and technique pros/cons: `docs/platform-disco/04-...md` §15. Code
  on disk, uncommitted.
- 2026-06-08 — **PLAT-02c SDRAM hardware-verified; column-bit-2 bug
  RESOLVED.** The SDCLK `0b01`→`0b10` correction (in
  `disco/regs/fmc.hpp` `SDCLK_DIV2 = 0b10<<10`, so the live
  `SDCR1 = 0x1800`, not the `0x1400` recorded in the 2026-04-27 entry
  below) was applied in code 2026-04-28 but never run on the board.
  Flashed `lvglpp_stm32h747i_disco_sdram` to the attached DISCO via
  probe-rs 0.29.1 + ST-LINK V3 and read the D3-SRAM relay at
  `0x3800_0300`: breadcrumb `a11c0009`, and the previously-broken
  `0xD000_0000`↔`0xD000_0004` pair now reads back correctly
  (`deadbeef`/`cafebabe`, no swap). Replaced the 4-canary smoke in
  `main_sdram.cpp` with a rigorous memtest — address-line walk over
  byte-offset bits 2–24 (full 32 MiB, catches swapped/stuck/shorted
  address lines), data-line walk (32-bit walking ones + complement),
  and a 512-point range sweep — relay status `0x600D_0000` (PASS),
  fail-offset `0xFFFF_FFFF` (none). The earlier "bring-up order /
  early-FMC-at-HSI" hypothesis is moot: clocks-then-SDRAM at
  FMCSEL=PLL2_R (SDCLK 75 MHz) works with the correct `0b10` divider.
  **PLAT-02d (LTDC + DSI + first pixels) is now unblocked.**
- 2026-04-27 — PLAT-02c chapter ratified
  (`docs/platform-disco/03-sdram-and-fmc.md`) and execution
  landed (`disco/regs/{fmc,mpu}.hpp` + `disco/sdram.{hpp,cpp}` +
  `main_sdram.cpp` + new `lvglpp_stm32h747i_disco_sdram` target,
  4716 B FLASH). All FMC + RCC + MPU registers programmed
  byte-for-byte rlvgl: BCR1.FMCEN set, SDCR1=0x1400,
  SDCR2=0x1E5, SDTR1=0x106000, SDTR2=0x1010461, SDRTR=0x46C,
  PLL2_R=150 MHz, FMCSEL=PLL2_R, both `RCC.AHB3ENR.FMCEN` and
  `RCC.C1_AHB3ENR.FMCEN` set (CM7 per-core gate), JEDEC
  sequence (clk_en → 100 µs delay → PALL → AR×8 → load_mode →
  normal) issued with BUSY-poll between each, MPU region 0
  configured Normal/Shareable/non-cacheable.
  **Hardware verification: PARTIAL.** Writes do reach SDRAM
  (previous-run pattern1 values still occupy higher offsets,
  proving the chip is alive). But column-address bit 2 is
  inverted: a write of `0xDEADBEEF` to `0xD000_0000` reads back
  at `0xD000_0004`, and `0xCAFEBABE` written to `0xD000_0004`
  reads back at `0xD000_0000`. Higher offsets (0x100, 0x200)
  return *stale data from the previous run* at unpredictable
  aliased addresses, suggesting writes there don't land where
  expected. PLL/FMC config is provably correct via probe-rs
  register readback.
  **Suspected root cause:** bring-up order. rlvgl runs
  `early_fmc_setup` → `configure_fmc_sdram` BEFORE the HAL
  configures PLLs (i.e., at HSI 64 MHz default with FMC kernel
  at HSI). Our flow runs `clocks::init()` (PLLs up, FMCSEL=PLL2_R)
  BEFORE `sdram::init()`. The IS42S32800J chip may need its
  init sequence to occur at the original/lower clock with the
  later kernel-clock switch happening *after* SDRAM is alive.
  **Next step (deferred):** restructure so a minimal
  `disco::early_fmc_init()` runs first at HSI (enables FMC bus
  clock, GPIO clocks for the FMC ports, muxes pins, runs the
  full SDRAM JEDEC + SDCR/SDTR/SDRTR), THEN `clocks::init()`
  brings up PLLs and switches FMCSEL=PLL2_R. PLAT-02d (LTDC +
  DSI + first pixels) is **blocked** on this — without
  reliable SDRAM addressing, framebuffer storage is unsound.
- 2026-04-27 — rlvgl pin advanced on `v0.2.0`:
  `d99f793` → `79f730d` (4 commits). Upstream changes are
  rlvgl-internal: APP-00 Application Schema chapters
  (`docs/app-schema/00..03.md` + sample `app.yaml` for
  beetle-esp32c3 — does not affect any contract lvglpp mirrors
  today; potential future relevance to a creator-cpp follow-up),
  plus three BBB-prong improvements (Linux dirty-rect 16bpp,
  Linux↔FreeRTOS toggle, bare-metal playit-lite over UART0). No
  files lvglpp mirrors changed (`git diff --stat` empty for
  `examples/stm32h747i-disco/`, `widgets/`, `playit/`). Bulk SHA
  refresh across 63 lvglpp files; cross + host builds still
  green (19/19 ctest, both `*_smoke.elf` and `*_clocks.elf`
  unaffected).
- 2026-04-27 — PLAT-02b execution **verified on hardware**.
  STM32H747I-DISCO over ST-LINK V3 + probe-rs 0.29.1:
  `lvglpp-stm32h747i-disco-clocks.elf` flashed, reset, breadcrumb
  at `0x3800_0300` reads `0xA11C_0007` (FMC_PINS_MUXED — final
  breadcrumb).  Acceptance registers post-bring-up:
  `RCC.CR=0x3F03C025` (`& 0x3000_0000 == 0x3000_0000`,
  PLL1/2/3 all on+ready, HSE ready),
  `RCC.CFGR=0x0000001B` (`SW=SWS=0b011`, PLL1 active),
  `PWR.D3CR=0x0000E000` (VOS1 + VOSRDY),
  `GPIOF.MODER=0xAABFFAAA` / `OSPEEDR=0xFFC00FFF` /
  `AFRL=0x00CCCCCC` (FMC pins 0-5 + 11-15 in AF12 + VeryHigh,
  untouched pins at defaults). All §12 acceptance gates green.
- 2026-04-27 — PLAT-02a execution **verified on hardware**.
  Vector-table read-back at `0x0800_0000` returns
  `[0x20020000, 0x08000319, 0x0800032D, 0x0800032D, 0x0800032D,
  0x0800032D, 0x0800032D, 0x00000000]` — `_estack` + Reset
  (Thumb bit) + five fault handlers all weak-aliased to
  `Default_Handler`. CPU running per `DHCSR.S_HALT=0`.
- 2026-04-27 — PLAT-02b chapter ratified
  (`docs/platform-disco/02-clocks-and-pinmux.md`) and execution
  landed. `examples/stm32h747i-disco/disco/{addr.hpp,
  breadcrumb.hpp, clocks.hpp, clocks.cpp, pinmux.hpp, pinmux.cpp}`
  + `regs/{rcc,pwr,gpio}.hpp` + `main_clocks.cpp`.
  `MmioAddr<T>` typed handle is the sole MMIO entry point per
  PLAT-02 §5.5 #2; every register block carries
  `static_assert(offsetof(...))` lines on the offsets used. The
  ten-step `disco::clocks::init()` (PWR/SMPS+VOS1 → HSE →
  PLL1/2/3 dividers → PLL1+PLL2 latch → bus prescalers + SYSCLK
  switch → **PLL3ON** + PLL3RDY → AHB3/AHB1/APB3/AHB4 clock-gates
  → FMCSEL=PLL2_R → breadcrumb `0xA11C_0005`) is concrete; the
  PLL3ON step carries the rlvgl gap-fix cite in code so a
  refactor cannot drop it silently. `disco::pinmux::mux_fmc_pins()`
  applies AF12 + VeryHigh to the ~50 FMC pins per §5.7 and
  writes breadcrumb `0xA11C_0007`. New CMake target
  `lvglpp_stm32h747i_disco_clocks` cross-builds clean under
  arm-none-eabi-gcc 13.3.1 at 3528 B FLASH / 0 B data / 0 B BSS
  — well inside the §12 sanity envelope. Host build (19/19
  ctest) unaffected.
- 2026-04-27 — PLAT-02a chapter ratified
  (`docs/platform-disco/01-toolchain-and-reset.md`) and execution
  landed. `cmake/toolchains/arm-none-eabi.cmake` +
  `examples/stm32h747i-disco/{memory.ld,disco.ld,
  startup_stm32h747xi.cpp,main_smoke.cpp,CMakeLists.txt}`.
  Cross-build under arm-none-eabi-gcc 13.3.1 (STM32CubeIDE bundle)
  produces a 828 B FLASH / 0 B RAM `.elf` with vector table at
  `0x08000000` opening `0x20020000` (`_estack`) /
  `0x08000319` (Reset_Handler + Thumb). Top-level CMakeLists
  gated on `LVGLPP_HOST_BUILD` so cross builds skip lvgl + tests
  + per-module libs cleanly; host build (19/19 ctest, including
  the SDL demo + recorder) unaffected.

- 2026-06-10 — PLAT-LNX landed: FbdevDisplay (32/24/16bpp,
  line_length-honouring rect flush) + EvdevInput (single-touch +
  MT protocol B; state machine portable and host-tested with
  synthetic streams). LVGLPP_PLATFORM_LINUX_FBDEV, Linux-only.
  BLOCKER: examples/linux-fbdev-smoke console rendering needs a
  Linux host — none on this bench. Owner: external fbdev consumer
  / project lead.
- 2026-06-29 — Status reconciled to the `rlvgl` `v0.2.5` submodule pin
  (`f999f75`) and the lvglpp LVGL-backed parity baseline at
  `docs/lvgl-parity/01-baseline.md`. FireBeetle 2 ESP32-P4 work is
  scoped by LPAR-CPP-00/SCTD-CPP-03 as ESP-IDF C hardware ownership plus
  C++ app payload ownership.
