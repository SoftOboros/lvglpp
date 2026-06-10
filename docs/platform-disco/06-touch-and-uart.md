# 06 — Touch (FT5336/I2C4) + USART1 (playit transport)

Chapter status: **draft, ratified 2026-06-10 (USART half); touch half
outlined, not yet normative**.
Phase code: **PLAT-02f** (see family chapter `00-platform-disco.md`
§5.1).

## §0 Authority

- USART register vocabulary (CR1/BRR/ISR/ICR/RDR/TDR, FIFO mode) is
  owned by **STMicroelectronics RM0399**, USART chapter.
- The playit **wire protocol** (command grammar AND response formats)
  is owned by `rlvgl/playit/src/protocol.rs` (v0.2.0 @ 79f730d),
  already mirrored in lvglpp by the PLAYIT family
  (`docs/playit-tagged/01-response-formatter.md` PLAYIT-04b freezes
  `format_response` under Standards Action). This chapter adds **no
  new wire vocabulary** — it lands the transport under the existing
  contract.
- USART1 bring-up values (pins, baud, FIFO posture) are owned by
  `rlvgl/examples/stm32h747i-disco/src/main.rs` "USART1 VCP init"
  (~L1972–2000).
- FT5336 protocol is owned by the FT5336 datasheet +
  `rlvgl/platform/src/touch_i2c.rs` — **touch is not normative in
  this revision** (see §5.7).

## §1 Purpose

Land the eyes-and-fingers substitute: USART1 over the ST-LINK VCP
speaking the playit protocol, so the bench can query status (`?`),
dump framebuffer windows (`D…`), and later inject input (`T…`)
without a human looking at or touching the board. This is
deliberately front-loaded ahead of touch because the bench currently
operates blind (PLAT-02e §15 amendment).

## §2 Problem statement

1. Today the only telemetry channels are D3-SRAM relay words and the
   48×80 downsampled captures — bespoke per-experiment, probe-driven,
   and write-only from the firmware's point of view. Every new gate
   means recompiling relay plumbing.
2. The playit parser/formatter already exist in lvglpp
   (`playit/src/parser.cpp`, `format.cpp` — allocation-free,
   freestanding-clean) but nothing on-target consumes them: the cross
   build skips all module libraries (`LVGLPP_HOST_BUILD` gate), and
   no USART driver exists.
3. rlvgl's CM7 binary serves the identical protocol on the same
   USART1/VCP wiring — the cross-language parity loop (family §12)
   needs the lvglpp side to exist before any diff can run.

## §3 Canonical glossary

- **VCP** — the ST-LINK V3 virtual COM port, wired on the
  STM32H747I-DISCO to **USART1 PA9 (TX, AF7) / PA10 (RX, AF7)**. As
  used in rlvgl main.rs "USART1 VCP init"; host side enumerates as
  `/dev/cu.usbmodem*`.
- **`parse_command`** — As defined in
  `playit/include/lvglpp/playit/parser.hpp`; used without
  modification (compiled into the target, §5.4).
- **`format_response` / `Response`** — As defined in
  `playit/include/lvglpp/playit/format.hpp` / `response.hpp`
  (PLAYIT-04b); used without modification.
- **Dump framing** — As defined in `rlvgl/playit/src/executor.rs::
  emit_dump_if_ready`: `DUMP:queued\r\n` on accept; per frame a
  literal `F\r\n`, then `height` rows of space-separated 8-digit
  uppercase-hex ARGB words, CRLF per row; `END\r\n` after the last
  frame. Row width ≤ 40 px (rlvgl's `row_buf` bound — frozen as the
  shared limit).
- **`StatusData`** — As defined in
  `playit/include/lvglpp/playit/response.hpp` (tick_count,
  present_count); wire form `STAT:<tick>,<present>\r\n`.
- **Serial pump** — the cooperative main-loop stage that drains RX
  into a line buffer, parses complete lines, and emits responses.
  Owned by this chapter.

## §4 Source-of-truth map

| Concept | Canonical owner | lvglpp mirror |
| --- | --- | --- |
| USART1 register block | RM0399 USART chapter | `disco/regs/usart.hpp` |
| Pins/baud/FIFO posture | rlvgl main.rs L1972–2000 | `disco/usart.cpp` |
| Command grammar | rlvgl `protocol.rs` | `playit/src/parser.cpp` (PLAYIT-01) |
| Response formats | rlvgl `protocol.rs::format_response` | `playit/src/format.cpp` (PLAYIT-04b) |
| Dump framing | rlvgl `executor.rs` | `examples/stm32h747i-disco/main_display.cpp` responder (§5.5) |
| FT5336 protocol | FT5336 DS + rlvgl `touch_i2c.rs` | deferred (§5.7) |

## §5 Frozen decisions

### §5.1 USART1 configuration — **Standards Action**

Mirrors rlvgl byte-for-byte: PA9=TX/PA10=RX AF7 VeryHigh; 115200
8N1; `CR1 = FIFOEN | TE | RE | UE`; `BRR = 868` (USART1 kernel clock
= rcc_pclk2 = 100 MHz at our frozen prescalers — PLAT-02b — divided
to 115207 baud, 0.006% error). Kernel-clock mux left at reset
(`D2CCIP2R.USART16SEL = 0b000` = rcc_pclk2). Clock gates: APB2ENR
bit 4 AND C1_APB2ENR bit 4 (dual-core rule).

### §5.2 Polled transport for 02f-1 — **Specification Required**

The first leg is **polled**: TX spins on ISR.TXE_TXFNF (bit 7), RX
drains ISR.RXNE_RXFNE (bit 5) into a 128-byte line buffer each pump
iteration. rlvgl's IRQ + ring-buffer transport (main.rs
`runtime_serial`, NVIC prio 3) is the 02f-2 shape, adopted when the
cooperative event-pump loop lands. At 115200 baud (~11.5 KB/s) and
a pump called from a tight WFI-free loop, polled RX cannot overrun
the 8-deep hardware FIFO in practice; overruns are cleared via
ICR.ORECF and counted to a D3 relay regardless.

### §5.3 On-target command subset for 02f-1 — **Specification Required**

| Command | Behaviour |
| --- | --- |
| `?` | `STAT:<tick>,<present>` — tick = pump iterations; present = 0 until present-counting lands (deviation, §10). |
| `D<x>,<y>,<w>,<h>[,<frames>]` | Immediate dump of the live front buffer per §3 framing; w clamped to 40, frames honoured by re-reading the live buffer. |
| `T…`/`P…`/`K…`/`M…` (inject) | Parsed, replied `OK` — **recorded to a D3 relay count** so the host can verify parse-side handling; dispatch lands with the widget tree. |
| `Q…` (query) | `ERR:no-tree` until widgets land on-target. |
| `R…` (recorder) | `ERR:unsupported` in 02f-1. |
| unknown | Extension per parser → `OK` (rlvgl parity). |

### §5.4 Parser/formatter reuse — **Standards Action**

The disco target compiles `playit/src/parser.cpp` and
`playit/src/format.cpp` directly into the example binary (they are
allocation-free and freestanding-clean), with the playit `include/`
dir on the include path. Re-implementing the grammar or response
text in `examples/` is **forbidden** — that is exactly the silent
fork CLAUDE.md § "Definitions" exists to prevent. When the module
libraries gain a cross-build leg, the example switches to linking
`lvglpp::playit` and this section is amended.

### §5.5 Responder placement — **Specification Required**

The 02f-1 responder lives in the display bench binary
(`main_display.cpp` serial pump replacing the terminal `wfe` loop),
so one flashed image carries: full display stack + DMA2D gates + D3
relays + serial protocol. A dedicated app-shell binary is the DEMO
family's concern, not this chapter's.

### §5.6 Acceptance gate (eyes-free) — **Specification Required**

From the host over `/dev/cu.usbmodem*`:
1. `?` returns a well-formed `STAT:` line with tick monotonically
   increasing across two queries.
2. `D140,250,8,2` (top-left corner of the PLAT-02e magenta rect)
   returns `DUMP:queued`, `F`, two rows of eight `FFFF00FF` words,
   `END`.
3. `D0,0,8,2` returns the white border row + RED/border row per the
   CPU pattern (cross-checks dump geometry against known content).
4. An inject command returns `OK` and bumps the D3 inject-count
   relay.

### §5.7 Touch half — **deferred, not normative**

FT5336 on I2C4 @ 0x38 (INT PK7, TIM6-paced polling per rlvgl) is
outlined here only to reserve the chapter shape. It becomes
normative in a revision of this doc once: (a) the widget tree runs
on-target (else a touch has nothing to dispatch into), and (b) a
finger or a serial-injected surrogate is available for the
end-to-end gate. Serial `T…` injection (§5.3) is the standing
fingers-substitute meanwhile.

## §10 Reconciliation vs. adjacent primitives

- **PLAYIT-07 transport seam.** The family map names `UsartTransport
  : Transport` registered with the `Executor`. 02f-1 deliberately
  does NOT instantiate the playit `Executor` (it drags the widget
  tree); the pump calls `parse_command`/`format_response` directly.
  When widgets land on-target, the pump's parse/dispatch core is
  replaced by the `Executor` and the transport object implements the
  PLAYIT-07 interface. The wire behaviour is identical either way —
  that is the point of freezing the formats in PLAYIT-04b.
- **Dump-on-present vs immediate dump.** rlvgl queues a `D` and
  emits after the *next present* (tear-free snapshot under its
  manual-refresh pipeline). lvglpp 02f-1 runs AR=1 single-buffer:
  there is no present event yet, so the dump reads the live buffer
  immediately. DEVIATION, host-visible only as timing; row content
  of a static scene is identical. Closes when present-counting (TE
  pacing or 02e-2/3 pipeline) lands; until then `STAT:` reports
  present=0, which is honest.
- **Breadcrumbs.** No new breadcrumb codes (the set is frozen
  byte-for-byte with rlvgl). USART status goes to new D3 relay words
  at `0x3800_03A0..` (clear of all existing blocks).

## §11 Non-goals

- IRQ/ring-buffer serial (02f-2), recorder (`RS/RE/RD`), tagged
  queries/injection — need the on-target widget tree.
- TCP bridging (`rlvgl/playit/src/tcp.rs` host tooling works as-is
  against the VCP device).
- FT5336 execution (§5.7), I2C4 driver, TIM6.

## §12 Acceptance checklist (PLAT-02f-1, USART leg)

- [x] `disco/regs/usart.hpp` lands with RM0399 static_asserts.
- [x] `disco/usart.cpp` lands per §5.1/§5.2 with ownership comments.
- [x] Display bench binary answers §5.6 items 1–4 over the VCP
      (2026-06-10, `/dev/cu.usbmodem1302`): `STAT:` ticks monotonic;
      `D140,250,8,2` returned 16× `FFFF00FF` (the PLAT-02e magenta
      rect — independent second-channel confirmation of the DMA2D
      fill); `D0,0,8,2` returned the white border row + border-pixel
      + 7× `FFFF0000` exactly per the CPU pattern; `T100,200` → `OK`
      with the 0x3A4 inject relay at 1; `QE:` → `ERR: no-tree`.
      Relay block 0x3A0: lines=6, injects=1, overruns=0.
- [x] No grammar/format restatement outside `playit/` (§5.4). The
      cross build surfaced one portability fix inside playit itself
      (`std::clamp` deduction — `int32_t` is `long` on arm-none-eabi);
      host parser/format/executor tests stay green.
- [x] Cross-build clean with `-Werror` posture flags (toolchain
      `-Wall -Wextra -Wpedantic -Wconversion …`); link required the
      embedded-posture `cxx_shims.cpp` (std::__throw_*/assert/abort
      → bkpt trap) to keep libstdc++'s exception machinery and
      newlib's heap out of the image.

## §13 Files cited

- `rlvgl/examples/stm32h747i-disco/src/main.rs` L406–760
  (runtime_serial, ISR), L1972–2000 (USART1 VCP init).
- `rlvgl/playit/src/protocol.rs`, `executor.rs` (dump framing),
  `response.rs`.
- `playit/include/lvglpp/playit/{parser,format,response,command}.hpp`,
  `playit/src/{parser,format}.cpp` (PLAYIT-01/-04b).
- `docs/platform-disco/00-platform-disco.md` §5.1, §12;
  `05-dma2d-engine.md` §15 (blind-bench context).
- STMicroelectronics RM0399, USART chapter.

## §14 Unblocks

- Standing eyes-substitute: `D` window dumps at native resolution on
  demand, replacing per-experiment capture plumbing.
- Standing fingers-substitute: serial inject path ready for the
  widget tree (PLAT-02f-2 / DEMO hardware leg).
- The family §12 cross-language parity gate (pipe identical fixtures
  into rlvgl and lvglpp binaries) becomes runnable.

## §15 Change log

- 2026-06-10 — Chapter ratified at draft level, USART half only:
  §5.1 config, §5.2 polled posture, §5.3 command subset, §5.4
  parser reuse, §5.5 placement, §5.6 eyes-free gate frozen. Touch
  half (§5.7) outlined, explicitly non-normative. Front-loaded
  because the bench is operating blind (owner eyes unavailable —
  see 05-dma2d-engine.md §15 2026-06-10 amendment).
- 2026-06-10 — PLAT-02f-1 execution landed same day; all §5.6 gates
  green first run (§12). The playit serial channel is now the
  standing eyes-substitute (D dumps at native resolution) and
  fingers-substitute (inject parse path live, dispatch pending the
  on-target widget tree).
