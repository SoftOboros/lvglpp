<!--
STATUS.md — Co-located status block for lvglpp::playit.
Canonical shape: see CLAUDE.md § "Doc Co-Location Policy".
-->

# lvglpp::playit — STATUS

Tracks `rlvgl/playit` @ `v0.2.5` (commit `f999f75`). Last reconciled:
2026-06-29.

## Roadmap intent

`lvglpp::playit` is the C++ side of the cross-language test harness.
Same wire format, same fixtures, same probe — different language. The
parser is the load-bearing piece; the dispatcher binds parsed commands
to the lvglpp event surface.

Phase plan:

1. **PLAYIT-01:** Command type + parser. Parses `?`, `T<x>,<y>`,
   `PD/PM/PU<x>,<y>`, `KD:<key>`, `KU:<key>` from a line buffer. No
   dynamic allocation in the hot path. Parity test against
   `rlvgl/playit/src/parser.rs` fixtures.
2. **PLAYIT-02:** Dispatcher — binds parsed commands to
   `lvglpp::core::Event` injection. Gated on CORE-02.
3. **PLAYIT-03:** Multi-touch frames (`MT<n>:<id>,<s>,<x>,<y>;...`).
4. **PLAYIT-04:** Tagged-widget queries (`T@<tag>:`, `QB:`, `QE:`,
   `QC:`). Gated on `lvglpp::core::Widget` tag support.
5. **PLAYIT-05:** Framebuffer dump (`D<x>,<y>,<w>,<h>`). Gated on
   renderer readback.
6. **PLAYIT-06:** Event recorder (`RS` / `RE` / `RD`). Gated on a
   small fixed-size ring buffer; opt-in for embedded targets.

## As-built

Implemented (PLAYIT-01 + PLAYIT-02 — landed 2026-04-27):

- Compiled CMake target `lvglpp::playit` with `src/parser.cpp`.
- `EventSpec` / `KeySpec` / `TouchPointSpec` / `TouchStateSpec`
  wire-format types under `lvglpp::playit::*`, parity-cited against
  `rlvgl/playit/src/command.rs`.
- `Command` sum type (`std::variant`) covering every wire-protocol
  command: `Status`, `Inject`, `InjectTagged`, `QueryBounds`,
  `QueryExists`, `QueryChildCount`, `DumpPixels`, `RecordStart`,
  `RecordStop`, `RecordDump`, `Extension`.
- `parse_command(std::string_view) -> std::optional<Command>` —
  allocation-free; tag fields borrow from the input buffer.
- Per-module test target `lvglpp_playit_parser` with parity fixtures
  covering every wire form. Passes under host posture.
- Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.

- `EventPipeline` abstract base + `NullPipeline` passthrough in
  `event_pipeline.hpp`. Test target: `lvglpp_playit_event_pipeline`.
- **LPAR-CPP-04 bridge:** `LvglInputBridge` in
  `playit/include/lvglpp/playit/lvgl_input_bridge.hpp` binds existing
  playit `EventSpec` pointer/key commands to LVGL-backed synthetic
  pointer/keypad input devices without changing the wire grammar. Test
  target: `lvglpp_playit_lvgl_input_bridge`.

Stubbed (later phases):

- PLAYIT-03+ richer pipelines (debouncer, double-tap recogniser),
  PLAYIT-04 tagged-widget queries (gated on CORE-03 widget tree
  execution), PLAYIT-05 framebuffer dump, PLAYIT-06 event recorder.

## Blockers

- **Wire-protocol authority lives in rlvgl.** Per CLAUDE.md
  § "Cross-language change ordering", any new command lands in
  `rlvgl/playit/README.md` first, then mirrors here. The rlvgl
  protocol doc is the source of truth; do not extend playit
  unilaterally on the lvglpp side.

## Definitions

- **Wire protocol** — As defined in `rlvgl/playit/README.md`; mirrored
  here without modification. Any divergence is a bug in lvglpp.
- **`Command`** — As defined in
  `playit/include/lvglpp/playit/command.hpp` (this repo). Mirrors
  `rlvgl/playit/src/command.rs:7` with adapted: the rlvgl `'a` lifetime
  on tag/payload fields is expressed via `std::string_view` whose
  lifetime is bounded by the input buffer fed to `parse_command`.
- **`EventSpec` / `KeySpec` / `TouchPointSpec` / `TouchStateSpec`** —
  As defined in `playit/include/lvglpp/playit/event_spec.hpp` (this
  repo). Mirrors `rlvgl/playit/src/command.rs:34/112/74/63` without
  modification.
- **`parse_command`** — As defined in
  `playit/include/lvglpp/playit/parser.hpp` (this repo). Mirrors
  `rlvgl/playit/src/protocol.rs:110`; both return `Option<Command>`
  (Rust) / `std::optional<Command>` (C++).
- **`to_event` / `to_key` / `to_core`** — As defined in
  `playit/include/lvglpp/playit/conversion.hpp` (this repo).
  Mirrors `rlvgl/playit/src/command.rs:175` (`EventSpec::to_event`),
  `:157` (`KeySpec::to_key`), `:87` (`TouchStateSpec::to_core`).
  Adapted: rlvgl uses methods, lvglpp uses free functions to keep
  `playit/event_spec.hpp` independent of `core/event.hpp`.
- **`Response` / `StatusData`** — As defined in
  `playit/include/lvglpp/playit/response.hpp` (this repo). Mirrors
  `rlvgl/playit/src/response.rs:5, :14` without modification.
- **`Dispatcher`** — As defined in
  `playit/include/lvglpp/playit/dispatcher.hpp` (this repo).
  Adapted from `rlvgl/playit/src/executor.rs`: rlvgl's executor
  owns transport + recorder; lvglpp's Dispatcher owns only the
  command→Response routing. Transport lives in PLAT-NN; recorder
  is PLAYIT-06.
- **`LvglInputBridge`** — As defined in
  `playit/include/lvglpp/playit/lvgl_input_bridge.hpp` (this repo).
  Owned by `docs/lvgl-parity/04-event-focus-input.md`; adapted from
  playit `EventSpec` injection to feed LVGL `lv_indev_t` read state
  instead of the compatibility `WidgetNode` dispatcher.

## Change log

- 2026-04-27 — Initial scaffold. INTERFACE target only; no parser yet.
- 2026-04-27 — PLAYIT-01 landed. `lvglpp::playit` is now a compiled
  library. `Command` / `EventSpec` / `KeySpec` / `TouchPointSpec`
  defined; `parse_command` covers every wire form in
  `rlvgl/playit/src/protocol.rs`. Per-module test target
  `lvglpp_playit_parser` registered with ctest. Compiles cleanly
  under `LVGLPP_EMBEDDED_POSTURE=ON`.
- 2026-04-27 — Conversion seam landed alongside CORE-02 execution.
  `playit/include/lvglpp/playit/conversion.hpp` adds free functions
  `to_event(EventSpec)`, `to_key(KeySpec)`, `to_core(TouchStateSpec)`,
  `to_core(TouchPointSpec)` mirroring rlvgl's `*Spec::to_*` impls.
  `static_assert` guards `playit::MAX_TOUCH_POINTS ==
  core::MAX_TOUCH_POINTS`. Round-trip test
  `lvglpp_playit_conversion` registered with ctest.
- 2026-04-27 — PLAYIT-02 landed. `EventPipeline` abstract base +
  `NullPipeline` concrete passthrough in
  `playit/include/lvglpp/playit/event_pipeline.hpp`. Mirrors
  `rlvgl/playit/src/executor.rs:56, :64`. `PipelineOutput` struct
  carries the (primary, secondary) optional-Event pair.
  Test target `lvglpp_playit_event_pipeline` covers passthrough,
  quiet tick, and secondary-slot fan-out.
- 2026-04-27 — PLAYIT-04 chapter ratified
  (`docs/playit-tagged/00-tagged-queries.md`) and execution landed.
  `lvglpp::playit::Response` (`std::variant` over `Ok` / `Error` /
  `Bounds` / `Exists` / `ChildCount` / `Status` / `DumpEnd`).
  `lvglpp::playit::Dispatcher` routes parsed Commands into a
  `lvglpp::core::WidgetNode` tree per §5.3 — `InjectTagged`
  single-node dispatch, `Inject` DFS dispatch, three Query forms,
  Status from a settable snapshot, deferred phases return
  `Error{"not implemented"}`. Test target
  `lvglpp_playit_dispatcher` (12 fixtures) verifies the full
  cross-language closure: wire-bytes → `parse_command` →
  `Dispatcher::dispatch` → `WidgetNode::dispatch_event` →
  `Button::on_click`. Embedded posture clean.
- 2026-04-27 — PLAYIT-04b chapter ratified
  (`docs/playit-tagged/01-response-formatter.md`) and execution
  landed. `format_response(Response, span<char>) -> size_t` mirrors
  rlvgl's wire format byte-for-byte. INT32_MIN renders without UB.
  Test target `lvglpp_playit_format` (11 fixtures) green.
- 2026-04-27 — PLAYIT-07 chapter ratified
  (`docs/playit-transport/00-transport-and-executor.md`) and
  execution landed. `Transport` abstract base + `StdioTransport`
  host-only concrete + `Executor` line-accumulator + dispatch
  loop. Test target `lvglpp_playit_executor` (8 fixtures) green.
- 2026-04-27 — PLAYIT-06 chapter ratified
  (`docs/playit-recorder/00-event-recorder.md`) and execution
  landed. `EventRecorder` (256-entry ring) + `format_event_spec`.
  Executor `set_recorder(…)` intercepts RS/RE/RD with the
  `REC:recording` / `REC:START,<n>` / `@<seq> <event>` /
  `REC:END` wire format. **DELTA from rlvgl:** `@<seq>` instead
  of `@<tick_delta>` — PLAYIT-06a will close parity once a
  tick-counter seam lands. Test target `lvglpp_playit_recorder`
  (10 fixtures) green.
- 2026-04-27 — `examples/host_sdl_label/` plumbed with
  `StdioTransport` + `Executor` + `EventRecorder`. The running
  window now accepts piped wire commands on stdin and emits
  Responses on stdout. The lvglpp diagnostic harness is operational:
  pipe a fixture, watch the window, dump the captured sequence.
- 2026-04-27 — PLAYIT-06a chapter ratified
  (`docs/playit-recorder/01-tick-delta.md`) and execution landed —
  closes the rlvgl wire-protocol parity gap. `EventRecorder::Entry`
  now carries `tick_delta` (uint16_t saturating, replacing the v1
  `seq` field); recorder gained `tick()` per-frame advance and
  fill-and-stop semantics (replacing ring overwrite).
  `Executor::dump_recording` emits `@<tick_delta>` per PLAYIT-06a
  §5.5 — byte-for-byte rlvgl parity. SDL example calls
  `recorder.tick()` once per frame after `present_frame()`. End-to-end
  smoke verified on macOS 13 + SDL2 dummy driver:
  `RS\nT@ok_button:…\nT@ok_button:…\nRD` produces the expected
  `@0 / @0` deltas (both taps in same frame); injecting across
  frames produces non-zero deltas. Captured fixtures now diff
  cleanly across rlvgl ↔ lvglpp without a normaliser.
- 2026-04-27 — PLAYIT-04a chapter ratified
  (`docs/playit-recognizer/00-tap-and-double-tap.md`) and execution
  landed. `lvglpp::playit::TapRecognizer` (Pointer{Down,Up,Move} →
  PressDown/PressRelease, settled per SETTLE_MS, FT5336-bounce
  resilient) + `DoubleTapRecognizer` (PressRelease pair → DoubleTap,
  with SHORT_PRESS_MAX_MS / DOUBLE_TAP_WINDOW_MS /
  DOUBLE_TAP_MAX_DISTANCE thresholds) + `GesturePipeline`
  (concrete `EventPipeline` composing both, mirrors
  `DiscoGesturePipeline`). Duration constants and `ms_to_ticks`
  math match `rlvgl/platform/src/gesture.rs` byte-for-byte. Test
  target `lvglpp_playit_gesture` (11 fixtures). SDL demo wires
  `GesturePipeline` between backend events and tree dispatch: on-
  screen mouse clicks now drive the Button identically to a piped
  `T@ok_button:…` command. Piped commands still bypass the
  pipeline (PLAYIT-04a §10 — deferred PLAYIT-04a-1); existing
  dispatcher/executor tests remain unaffected. 16/16 ctest
  entries green; embedded posture clean.
- 2026-06-29 — Status reconciled to the `rlvgl` `v0.2.5` submodule pin
  (`f999f75`) and the lvglpp LVGL-backed parity baseline at
  `docs/lvgl-parity/01-baseline.md`. Wire parsing/formatting remains
  first-class; LVGL-backed dispatch is deferred to LPAR-CPP-04 after
  object/event wrappers exist.
- 2026-06-29 — LPAR-CPP-04 playit bridge landed.
  `LvglInputBridge` consumes existing `EventSpec` pointer/key variants
  and feeds synthetic LVGL pointer/keypad input-device reads. The
  wire grammar is unchanged. Test target
  `lvglpp_playit_lvgl_input_bridge` verifies `PressRelease` drives an
  LVGL click and `KeyDown` reaches the focused object through an
  `LvGroup`.
