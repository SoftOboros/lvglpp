# 01 — Toolchain, Memory Map, Reset Vector

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAT-02a**.

## §0 Authority

- Cortex-M7 reset behaviour, vector table layout, ARMv7-M
  exceptions: **ARM ARM (DDI 0403E.e)** §B1.5 + **Cortex-M7 TRM
  (DDI 0489D)** §2.6. Authoritative.
- STM32H747XIH6 boot ROM behaviour, FLASH layout
  (`0x0800_0000`..`0x0810_0000` for Bank 1), SRAM regions
  (`0x2000_0000` DTCM 128K, `0x2400_0000` D1 AXI 512K, etc.):
  **RM0399** §3 + §4. Authoritative.
- Memory map mirrors `rlvgl/examples/stm32h747i-disco/memory.x`
  byte-for-byte where possible (canonical for cross-language
  parity).
- Toolchain floor: `arm-none-eabi-gcc >= 11.0` (GNU Arm Embedded
  Toolchain). Reason: C++20 `consteval`, `<concepts>`, and
  `<bit>` rely on libstdc++ ≥ 11. Older toolchains will produce a
  CMake configuration error.
- Underlying posture rules: `docs/std-mapping.md`
  § "Embedded posture".

## §1 Purpose

PLAT-02a delivers the **smallest possible flashable artifact**
for the STM32H747I-DISCO. It is the seam between "host CMake"
and "embedded CMake": after this chapter lands, every subsequent
PLAT-02 sub-phase consumes the toolchain file, the linker script,
and the reset/exception vector table without redefining them.

A successful PLAT-02a artifact does not yet bring up clocks,
SDRAM, or any peripheral. It boots the CM7 from FLASH at
`0x0800_0000`, lands in `Reset_Handler`, copies `.data` from LMA
to VMA, zeroes `.bss`, and enters `main()` which immediately
loops on `__WFE`. That's it. The entire validation gate is "halt
under probe-rs at the `__WFE` and read `$pc` ∈ the `main` symbol
range".

This is deliberately spartan. It exposes every cross-toolchain
gotcha (linker, startup, posture flags, debug info, MAP file
hygiene) without dragging in a single peripheral.

## §2 Problem statement

Cross-toolchain CMake for embedded ARM has many subtly-wrong
ways to spell each step. Common gaps observed in starter projects:

1. **Toolchain file omits `CMAKE_TRY_COMPILE_TARGET_TYPE
   STATIC_LIBRARY`.** CMake then runs link-time test programs
   against a host-shaped main; every `try_compile` call fails on
   missing `_exit`. Fix: set the variable so CMake never tries
   to actually link.
2. **Vector table not placed at `.isr_vector`.** Default linker
   scripts pool everything into `.text`; the vector table ends up
   somewhere other than `0x0800_0000`. Fix: explicit output
   section anchored at `ORIGIN(FLASH)`.
3. **`.data` LMA/VMA mishandled.** `Reset_Handler` copies the
   wrong byte range; `bss` overlaps `data`. Fix: explicit
   `__sdata` / `__edata` / `__sidata` / `__sbss` / `__ebss`
   symbols and a `Reset_Handler` that walks them.
4. **Initial stack pointer slot 0 of vector table is bogus.**
   Cortex-M loads MSP from word 0 on reset; if the linker put a
   pad word there the CPU faults instantly. Fix: linker provides
   `_estack` and the vector table's first entry is `_estack`.
5. **`-fno-exceptions -fno-rtti` skipped on third-party objects.**
   ABI mismatch at link time. Fix: posture flags applied via
   `lvglpp_posture` INTERFACE library that every disco target
   links — the existing host-side mechanism.
6. **`__libc_init_array` skipped.** C++ static constructors
   never run. Fix: `Reset_Handler` calls
   `__libc_init_array()` before `main()`.

PLAT-02a addresses each item explicitly. Subsequent sub-phases
inherit a working seam.

## §3 Canonical glossary

- **Toolchain file** — `cmake/toolchains/arm-none-eabi.cmake`.
  Sets `CMAKE_SYSTEM_NAME=Generic`,
  `CMAKE_SYSTEM_PROCESSOR=arm`, the `arm-none-eabi-*` compiler
  triple, `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`, the
  Cortex-M7 + FPU flag bundle, and forces
  `LVGLPP_EMBEDDED_POSTURE=ON`.
- **Memory map** — `examples/stm32h747i-disco/memory.ld`.
  Mirrors `rlvgl/examples/stm32h747i-disco/memory.x` (FLASH 1024K
  @ 0x0800_0000, RAM/DTCM 128K @ 0x2000_0000, D1 AXI 384K @
  0x2400_0000, MAILBOX 1K @ 0x3004_7000, SDRAM 32M @
  0xC000_0000). PLAT-02a uses only FLASH + RAM; the rest are
  declared for visibility.
- **Linker script** — `examples/stm32h747i-disco/disco.ld`.
  INCLUDEs `memory.ld` and defines the section layout
  (`.isr_vector` first, then `.text` / `.rodata` / `.data` /
  `.bss` / `.heap` / `.stack`). Provides `_estack`, `__sdata`,
  `__edata`, `__sidata`, `__sbss`, `__ebss`, `_end_of_heap`.
- **Vector table** — Defined in `startup_stm32h747xi.cpp`. 240
  entries: 16 ARMv7-M system handlers + 224 STM32H7 IRQs (per
  RM0399 §11.1.4). PLAT-02a populates `MSP`, `Reset_Handler`,
  `NMI_Handler`, `HardFault_Handler`, `BusFault_Handler`,
  `UsageFault_Handler`, `MemManage_Handler`. All others
  initialise to a default loop-on-self handler.
- **`Reset_Handler`** — Copies `.data` from LMA → VMA, zeros
  `.bss`, calls `__libc_init_array()`, calls `main()`. If
  `main()` returns, loops on `__WFE`. Mirrors cortex-m-rt's
  `Reset` shape.
- **Default fault handler** — Loops on `__BKPT(0)`. Mirrors
  rlvgl `BOOT.md` § "Fault trapping" recommendation: faults must
  break in place, never silently reset.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Memory regions | `rlvgl/.../memory.x` (canonical) | `examples/stm32h747i-disco/memory.ld`. |
| Vector-table layout | RM0399 §11.1.4 (canonical) | `examples/stm32h747i-disco/startup_stm32h747xi.cpp`. |
| Reset sequence (LMA→VMA, bss zero, libc init) | cortex-m-rt's `Reset` (rlvgl-side) (canonical) | `Reset_Handler` in `startup_stm32h747xi.cpp`. |
| Cortex-M7 FPU + cpu-flag bundle | ARM Cortex-M7 TRM (canonical) | `cmake/toolchains/arm-none-eabi.cmake`. |
| Posture flag plumbing | `docs/std-mapping.md` (canonical) | `lvglpp_posture` INTERFACE library (already exists). |

## §5 Frozen decisions

### §5.1 Toolchain triple — **Standards Action**

The cross compiler is `arm-none-eabi-gcc` with the Cortex-M7
+ FPU-double-precision flag bundle:

```
-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
```

`fpv5-d16` is mandatory: the H747's FPU is double-precision
(matches RM0399 §A.4). Single-precision (`fpv5-sp-d16`) compiles
but will mis-link against any `double` use — acceptance gate
rejects the artifact.

The toolchain file searches `PATH` for `arm-none-eabi-*` first,
then falls back to common Homebrew + apt prefixes
(`/opt/homebrew/Cellar/gcc-arm-embedded/*/bin`,
`/usr/local/gcc-arm-none-eabi-*/bin`, `/usr/bin`). A missing
compiler raises an actionable CMake error pointing at the GNU
Arm Embedded Toolchain download page.

### §5.2 Memory map — **Specification Required**

| Region | Origin | Length | Purpose |
| --- | --- | --- | --- |
| `FLASH` | `0x0800_0000` | `1024K` | Bank 1, CM7-only. `.isr_vector` first. |
| `RAM` (DTCM) | `0x2000_0000` | `128K` | `.data`, `.bss`, `.heap`, `.stack`. |
| `D1_CM7` | `0x2400_0000` | `384K` | D1 AXI SRAM CM7 share. Reserved; PLAT-02a does not use. |
| `D1_CM4` | `0x2406_0000` | `128K` | D1 AXI SRAM CM4 share. Visibility only. |
| `MAILBOX` | `0x3004_7000` | `1K` | Cross-core mailbox. Visibility only. |
| `D3_CM4` | `0x3800_0000` | `64K` | D3 SRAM4 (CM4-owned). Visibility only. |
| `QSPI_FLASH` | `0x9000_0000` | `64M` | Memory-mapped QSPI. Visibility only. |
| `SDRAM` | `0xC000_0000` | `32M` | External SDRAM. Brought up in PLAT-02c. |

Adding a region is **Specification Required** (per-chapter
amendment); changing an origin / length is **Standards Action**
(family-chapter amendment, byte-for-byte rlvgl parity check).

### §5.3 Vector table contents — **Standards Action**

PLAT-02a defines the first 16 ARMv7-M system entries plus a
default IRQ handler covering all 224 STM32H7 peripheral IRQ
slots:

| Index | Symbol | Behaviour |
| --- | --- | --- |
| 0 | `_estack` | Initial MSP (top of `RAM`). |
| 1 | `Reset_Handler` | LMA→VMA, bss zero, libc init, `main()`. |
| 2 | `NMI_Handler` | Default `__BKPT(0)` loop. |
| 3 | `HardFault_Handler` | Default `__BKPT(0)` loop. |
| 4 | `MemManage_Handler` | Default `__BKPT(0)` loop. |
| 5 | `BusFault_Handler` | Default `__BKPT(0)` loop. |
| 6 | `UsageFault_Handler` | Default `__BKPT(0)` loop. |
| 7..10 | reserved | `0x0000_0000`. |
| 11 | `SVC_Handler` | Default loop. |
| 12 | `DebugMon_Handler` | Default loop. |
| 13 | reserved | `0x0000_0000`. |
| 14 | `PendSV_Handler` | Default loop. |
| 15 | `SysTick_Handler` | Default loop. |
| 16..239 | `IRQ_<N>_Handler` | Default loop, each weak-aliased to `Default_Handler`. |

All non-system handlers are `__attribute__((weak,
alias("Default_Handler")))`; later sub-phases override the slots
they need (e.g. PLAT-02f overrides `USART1_IRQHandler`).

### §5.4 Linker output sections — **Standards Action**

```
SECTIONS {
  .isr_vector : ALIGN(4) {
      KEEP(*(.isr_vector))
  } > FLASH
  .text       : { *(.text*) *(.glue_7*) *(.eh_frame) } > FLASH
  .rodata     : { *(.rodata*) }                          > FLASH
  .ARM.exidx  : { *(.ARM.exidx*) }                       > FLASH
  .preinit_array : { __preinit_array_start = .;
                     KEEP(*(.preinit_array*))
                     __preinit_array_end = .; }          > FLASH
  .init_array    : { __init_array_start = .;
                     KEEP(*(SORT(.init_array.*)))
                     KEEP(*(.init_array*))
                     __init_array_end = .; }             > FLASH
  .fini_array    : { KEEP(*(.fini_array*)) }             > FLASH

  __sidata = LOADADDR(.data);
  .data : ALIGN(4) {
      __sdata = .;
      *(.data*)
      __edata = .;
  } > RAM AT > FLASH

  .bss : ALIGN(4) {
      __sbss = .;
      *(.bss*)
      *(COMMON)
      __ebss = .;
  } > RAM

  .heap : ALIGN(8) {
      __end = .;
      _end_of_heap = .;
  } > RAM
  /* stack grows downward from _estack (top of RAM) */
  _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

The vector table is **always first** in FLASH so the CPU's
boot-time MSP load (word 0 of FLASH) and `Reset` jump (word 1)
are correct.

### §5.5 Reset sequence — **Standards Action**

```cpp
extern "C" [[gnu::naked, noreturn]] void Reset_Handler() {
    asm volatile (
        "ldr  r0, =_estack\n"
        "msr  msp, r0\n"        // re-load MSP (boot ROM also did this)
        "bl   __lvglpp_disco_reset_main\n"
    );
}

extern "C" [[noreturn]] void __lvglpp_disco_reset_main() {
    // 1. .data: LMA -> VMA
    extern uint32_t __sidata, __sdata, __edata;
    for (uint32_t* dst = &__sdata, *src = &__sidata; dst < &__edata; )
        *dst++ = *src++;

    // 2. .bss: zero
    extern uint32_t __sbss, __ebss;
    for (uint32_t* p = &__sbss; p < &__ebss; ) *p++ = 0;

    // 3. C++ static constructors.
    extern "C" void __libc_init_array();
    __libc_init_array();

    // 4. main(). If it ever returns, loop on WFE forever.
    extern "C" int main();
    main();
    for (;;) asm volatile ("wfe");
}
```

The two-step naked-trampoline shape is mirrored from cortex-m-rt;
the naked frame keeps the MSP load atomic with the jump.

### §5.6 PLAT-02a `main()` — **Specification Required**

The PLAT-02a smoke target's `main()` is:

```cpp
int main() {
    for (;;) asm volatile ("wfe");
}
```

It exists so the linker has a symbol to land on. Subsequent
sub-phases replace it without touching reset/startup code.

### §5.7 CMake target name — **Specification Required**

The target produced by PLAT-02a is `lvglpp_stm32h747i_disco_smoke`,
yielding a `lvglpp-stm32h747i-disco-smoke.elf`. The full demo
target (`lvglpp_stm32h747i_disco`) is reserved for PLAT-02d+
once a Label can be displayed.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `memory.x`.** lvglpp's `memory.ld` mirrors region
  origins/lengths byte-for-byte. The two scripts have different
  *syntax* (rlvgl uses cortex-m-rt's `link.x` overlay; lvglpp
  emits the full SECTIONS block) but identical *semantics* —
  both place the vector table at `0x0800_0000` and the initial
  MSP at `_estack = 0x2002_0000`.
- **rlvgl cortex-m-rt's `Reset`.** The four-step sequence
  (data copy / bss zero / static-ctor / main) is identical;
  lvglpp adds `__libc_init_array()` because C++ requires it
  (Rust's `cortex-m-rt` doesn't).
- **PLAT-01 host SDL.** The SDL backend's main never runs on the
  disco target; CMake's host/cross split keeps the two
  toolchains from colliding. A single source tree may build
  *either* a host-SDL artifact *or* a disco artifact, never
  both at once (no fat binaries here).
- **`lvglpp_posture` INTERFACE library.** Already wired by the
  host build. PLAT-02a flips the default ON for cross targets;
  the existing posture-flag plumbing is reused unchanged.

## §11 Non-goals

- Clock setup. PLAT-02b owns this.
- SDRAM init. PLAT-02c owns this.
- Any peripheral access. PLAT-02b+ own this.
- MPU programming. Deferred to PLAT-02c (where it first matters
  for SDRAM access).
- D-cache / I-cache enable. Deferred to PLAT-02b.
- Semihosting / `printf` plumbing. Deferred; PLAT-02f's USART1
  is the production debug channel.
- Custom panic handler. PLAT-02a's "loop on `__BKPT`" is the
  permanent panic shape; no `std::abort` / `__cxa_throw` paths
  exist under embedded posture.
- CM4 startup. CM7-only per PLAT-02 §5.3.

## §12 Acceptance checklist

- [ ] `cmake/toolchains/arm-none-eabi.cmake` selects
      `arm-none-eabi-gcc` (with PATH + Homebrew/apt fallbacks),
      sets the Cortex-M7 + fpv5-d16 hard-float flag bundle,
      forces `LVGLPP_EMBEDDED_POSTURE=ON`, and sets
      `CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`.
- [ ] `examples/stm32h747i-disco/memory.ld` declares all eight
      regions per §5.2 with origins/lengths matching
      `rlvgl/.../memory.x` byte-for-byte (visibility-only
      regions are OK to declare-but-not-use).
- [ ] `examples/stm32h747i-disco/disco.ld` defines the SECTIONS
      block per §5.4; `__sidata`, `__sdata`, `__edata`,
      `__sbss`, `__ebss`, `_estack` resolved by
      `arm-none-eabi-nm`.
- [ ] `startup_stm32h747xi.cpp` emits a 240-entry vector table;
      slot 0 = `_estack`, slot 1 = `Reset_Handler`, every other
      IRQ slot weak-aliased to `Default_Handler`.
- [ ] Building under
      `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake`
      yields `lvglpp-stm32h747i-disco-smoke.elf` with
      `arm-none-eabi-readelf -h` reporting Class=ELF32,
      Machine=ARM, Entry=`0x08000XXX` inside the FLASH region.
- [ ] `arm-none-eabi-objdump -d` on the artifact shows the
      vector table starting at `0x08000000` with word 0 ==
      `0x20020000` (`_estack`) and word 1 == address of
      `Reset_Handler | 1` (Thumb bit).
- [ ] Build size sanity: `arm-none-eabi-size` reports `text`
      < 4 KiB, `data` == 0, `bss` < 1 KiB. (Substantially
      bigger means dead code or an unwanted libc dependency
      slipped in.)
- [ ] `probe-rs run --chip STM32H747XIHx
      lvglpp-stm32h747i-disco-smoke.elf` lands at the `__WFE`
      loop in `main` without panicking. (Optional acceptance:
      requires hardware.)
- [ ] `platform/STATUS.md` change-log entry recording PLAT-02a
      landing, dated.

## §13 Files cited

- `rlvgl/examples/stm32h747i-disco/memory.x` (canonical regions).
- `rlvgl/examples/stm32h747i-disco/BOOT.md` § "Fault trapping",
  § "Hardware Pin Summary".
- `rlvgl/cortex-m-rt/...` (reset shape; consulted, not mirrored
  line-for-line).
- STMicroelectronics RM0399 §3 (memory map), §4 (boot flow),
  §11.1.4 (vector table).
- ARM ARM (DDI 0403E.e) §B1.5 (exception model).
- ARM Cortex-M7 TRM (DDI 0489D) §2.6 (reset behaviour).
- `lvglpp/docs/platform-disco/00-platform-disco.md` (family
  chapter — sub-phase set, posture rules).
- `lvglpp/docs/std-mapping.md` § "Embedded posture".

## §14 Unblocks

- The cross-toolchain seam exists. PLAT-02b clocks/PLLs work
  begins on a known-good vector table + linker script.
- The host build stays unchanged: existing `cmake -S . -B build`
  invocations pick the host toolchain by default; cross builds
  require the explicit `-DCMAKE_TOOLCHAIN_FILE=...` override.
- `lvglpp_posture` gets exercised by a real cross compiler.
  Header-allowlist regressions surface at build time, not at
  inspection time.
- Fault handlers default to `__BKPT(0)` from day one. Subsequent
  sub-phases inherit a "faults break in place" posture without
  re-deciding it.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Toolchain triple
  (§5.1), memory map (§5.2), vector-table shape (§5.3), linker
  output sections (§5.4), reset sequence (§5.5), smoke-target
  `main` (§5.6), CMake target name (§5.7) all frozen.
