# 00 — Tap + Double-tap recognizer

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAYIT-04a**.

## §0 Authority

- Recogniser shapes, state machines, duration constants, tick math:
  `rlvgl/platform/src/gesture.rs` (v0.2.0 @ b178cbc). Canonical.
- Composition shape (Tap ∘ DoubleTap):
  `rlvgl/examples/disco-sim/src/main.rs:158` (`DiscoGesturePipeline`).
  Canonical reference.
- `EventPipeline` interface: PLAYIT-02
  (`docs/playit-tagged/00-tagged-queries.md` §5 references; the
  abstract base lives at `playit/include/lvglpp/playit/event_pipeline.hpp`).

## §1 Purpose

Land the gesture-recogniser pipeline so raw pointer events from
SDL (and embedded touch drivers, eventually) drive widgets via
the canonical gesture events that those widgets consume:

- `Button::handle_event` (WID-02 §5.3) consumes only
  `PressRelease`. SDL clicks today emit
  `PointerDown` → `PointerUp` and the Button never fires.
- `DoubleTap` is a Standards-Action variant in
  `lvglpp::core::event` (CORE-02 §5.1) but no producer exists.

PLAYIT-04a closes both gaps with a single composable pipeline.

## §3 Canonical glossary

- **`TapRecognizer`** — As defined in
  `rlvgl/platform/src/gesture.rs:49`. Mirrored as
  `lvglpp::playit::TapRecognizer`. Three-state FSM (`Idle` /
  `Down` / `PendingRelease`) with a `settle_ticks` countdown
  initialised from `SETTLE_MS` and the constructor's `frame_hz`.
- **`DoubleTapRecognizer`** — As defined in
  `rlvgl/platform/src/gesture.rs:169`. Mirrored as
  `lvglpp::playit::DoubleTapRecognizer`. Two-state FSM
  (`Idle` / `Armed`) with a `countdown_ticks` window and a
  Manhattan-distance threshold.
- **`GesturePipeline`** — Owned by this chapter. Mirrors
  `rlvgl/examples/disco-sim/src/main.rs:158`'s
  `DiscoGesturePipeline` composition. Concrete
  `EventPipeline` subclass: `process()` runs the input through
  the Tap recognizer, then the DoubleTap recognizer; `tick()`
  fires deferred releases / window-expiry events.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `TapRecognizer` FSM + bounce handling | rlvgl `gesture.rs:49` (canonical) | `lvglpp::playit::TapRecognizer`. |
| `DoubleTapRecognizer` FSM + distance / window thresholds | rlvgl `gesture.rs:169` (canonical) | `lvglpp::playit::DoubleTapRecognizer`. |
| Duration constants (SETTLE_MS / SHORT_PRESS_MAX_MS / DOUBLE_TAP_WINDOW_MS / DOUBLE_TAP_MAX_DISTANCE) | rlvgl `gesture.rs:21-34` (canonical) | `lvglpp::playit::SETTLE_MS` etc. **Standards Action** to change. |
| `ms_to_ticks` math (`(ms*hz + 999) / 1000`) | rlvgl `gesture.rs:38` (canonical) | `lvglpp::playit::ms_to_ticks`. |
| Composition order (Tap → DoubleTap) | rlvgl `disco-sim/main.rs:158` (canonical) | `lvglpp::playit::GesturePipeline`. |

## §5 Frozen decisions

### §5.1 Duration constants — **Standards Action**

| Constant | Value | Notes |
| --- | --- | --- |
| `SETTLE_MS` | `200` | Debounce settle period for `TapRecognizer`. Tuned for FT5336 capacitive touch bounce. |
| `SHORT_PRESS_MAX_MS` | `250` | Maximum hold duration counted as "short" by `DoubleTapRecognizer`. Longer holds bypass double-tap detection. |
| `DOUBLE_TAP_WINDOW_MS` | `400` | Maximum gap between consecutive short taps to recognise as a double-tap. |
| `DOUBLE_TAP_MAX_DISTANCE` | `20` | Manhattan distance (pixels) threshold between two taps. |

Frame-rate-independent. Per-recogniser tick counts derive from
`ms_to_ticks(MS, frame_hz)` where `frame_hz` is the constructor
parameter.

### §5.2 `ms_to_ticks` math — **Standards Action**

```
constexpr std::uint8_t ms_to_ticks(std::uint32_t ms, std::uint32_t frame_hz) noexcept {
    return static_cast<std::uint8_t>((ms * frame_hz + 999U) / 1000U);
}
```

Round-up division (rlvgl's `div_ceil`) so we never undercount the
settle / window. Returns `uint8_t` — the recogniser fields are
narrow (≤ 255 ticks at any sane frame rate).

### §5.3 `TapRecognizer::process` semantics — **Standards Action**

State machine with three states (`Idle` / `Down` / `PendingRelease`).
Behaviour mirrors `rlvgl/platform/src/gesture.rs:86`:

| State | Input | Action |
| --- | --- | --- |
| `Idle` | `PointerDown{x,y}` | → `Down`; emit `PressDown{x,y}` |
| `Idle` | `PointerUp` | ignore (spurious) — return `nullopt` |
| `Idle` | `PointerMove` | pass through |
| `Down` | `PointerDown` | update pos (drag) — return `nullopt` |
| `Down` | `PointerUp{x,y}` | → `PendingRelease`; arm settle — return `nullopt` |
| `Down` | `PointerMove{x,y}` | update pos; pass through (`PointerMove`) |
| `PendingRelease` | `PointerDown` | → `Down`, reset settle — return `nullopt` (bounce) |
| `PendingRelease` | `PointerUp` | re-arm settle — return `nullopt` |
| any | other (`Tick`, `Key*`, etc.) | pass through |

Non-pointer events (`Tick`, `KeyDown/Up`, `Touch`) pass through
unchanged.

### §5.4 `TapRecognizer::tick` semantics — **Standards Action**

Decrements the settle counter when in `PendingRelease`. When the
counter hits 0, transitions to `Idle` and returns
`PressRelease{pos}`. Returns `nullopt` otherwise.

### §5.5 `DoubleTapRecognizer::process` semantics — **Standards Action**

State machine with two states (`Idle` / `Armed`). Behaviour
mirrors `rlvgl/platform/src/gesture.rs:214`:

| State | Input | Action |
| --- | --- | --- |
| any | `PressDown{x,y}` | record `down_tick = tick_counter`; pass through |
| `Idle` | `PressRelease` (short hold) | → `Armed`; buffer position; arm window — return `(nullopt, nullopt)` |
| `Idle` | `PressRelease` (long hold) | pass through (long press, not double-tap candidate) |
| `Armed` | `PressRelease` (short, in-window, in-distance) | → `Idle`; emit `(DoubleTap{x,y}, nullopt)` |
| `Armed` | `PressRelease` (short, out-of-distance) | re-arm with new pos; emit `(buffered first PressRelease, nullopt)` |
| `Armed` | `PressRelease` (long) | → `Idle`; emit `(buffered first PressRelease, this PressRelease)` |
| any | other | pass through `(event, nullopt)` |

`is_short` test: `hold_ticks <= short_press_max_ticks` where
`hold_ticks = tick_counter - down_tick` (rlvgl's `wrapping_sub`).

### §5.6 `DoubleTapRecognizer::tick` semantics — **Standards Action**

Increments the internal tick counter (wrapping `uint8_t`).
Decrements `countdown_ticks` when in `Armed` state. When the
countdown hits 0, transitions to `Idle` and returns the buffered
`PressRelease{armed_pos}`. Returns `nullopt` otherwise.

### §5.7 `GesturePipeline` composition — **Standards Action**

```
GesturePipeline(uint32_t frame_hz);

PipelineOutput process(const Event& e) {
    auto out_tap = tap_.process(e);
    if (!out_tap) return {nullopt, nullopt};
    return double_tap_.process(*out_tap);
}

PipelineOutput tick() {
    PipelineOutput outputs{};
    if (auto tap_out = tap_.tick()) {
        auto pair = double_tap_.process(*tap_out);
        push_output(outputs, pair.primary);
        push_output(outputs, pair.secondary);
    }
    if (auto dtap_out = double_tap_.tick()) {
        push_output(outputs, dtap_out);
    }
    return outputs;
}
```

`push_output` fills `primary` if empty, else `secondary`. Mirrors
`DiscoGesturePipeline::push_output` at
`rlvgl/examples/disco-sim/src/main.rs:165`.

## §10 Reconciliation vs. adjacent primitives

- **PLAYIT-02 `NullPipeline`.** Still useful for tests and for
  consumers that already have gesture-level events. Drop-in
  replacement: any `EventPipeline&` slot accepts either.
- **Piped playit commands** (`T@<tag>:<x>,<y>`, `TD<x>,<y>` etc.).
  These already specify gesture-level events at parse time. The
  current Executor / Dispatcher path bypasses the recogniser for
  piped commands — they go straight through `to_event` →
  `WidgetNode::dispatch_event`. PLAYIT-04a does NOT change that
  path; piping `T@<tag>:50,50` still drives the Button without
  needing the pipeline. Wiring the pipeline into the piped-inject
  path (rlvgl does this) is a follow-up sub-phase
  (PLAYIT-04a-1) once a fixture needs the DoubleTap composition
  for piped events. See §11.
- **SDL backend** (PLAT-01). The SDL demo's main loop wires the
  `GesturePipeline` between `backend.poll_event()` and
  `root.dispatch_event(...)`. `pipeline.tick()` runs once per
  frame after `executor.poll()` and any tick-driven gesture
  outputs (deferred PressRelease, double-tap-window timeout) are
  dispatched into the tree.
- **Recorder.** `pipeline.tick()` runs alongside `recorder.tick()`
  per frame. Today the recorder only captures EventSpec from
  piped Inject commands (PLAYIT-06 §5.4); SDL gestures are NOT
  auto-recorded. Capturing gestured input is PLAYIT-06b territory.

## §11 Non-goals

- **Pipeline-on-piped-injects.** rlvgl's executor runs the
  pipeline on Inject Commands too; lvglpp's PLAYIT-04 Dispatcher
  bypasses it. PLAYIT-04a-1 will close that gap when a real
  fixture needs it.
- **Long-press recogniser.** A widget-level callback on
  long-press is out of scope; the recogniser passes long
  PressReleases through unchanged.
- **Multi-finger gestures.** `Touch{count, points}` events pass
  through unchanged. Pinch / swipe / rotate recogniser is a
  future sub-phase.
- **Per-widget gesture overrides.** All widgets share one
  pipeline at the application level.

## §12 Acceptance checklist

- [ ] `lvglpp::playit::TapRecognizer` exposes the §5.3/§5.4 FSM
      with `process(Event) -> std::optional<Event>` and
      `tick() -> std::optional<Event>`.
- [ ] `lvglpp::playit::DoubleTapRecognizer` exposes the §5.5/§5.6
      FSM with `process(Event) -> PipelineOutput` and
      `tick() -> std::optional<Event>`.
- [ ] `lvglpp::playit::GesturePipeline` composes both per §5.7
      and inherits from `lvglpp::playit::EventPipeline`.
- [ ] Duration constants and `ms_to_ticks` math match §5.1 / §5.2
      byte-for-byte.
- [ ] Test fixture ports the rlvgl tests at
      `gesture.rs:299-448`: tap-produces-down-then-release,
      bounce-suppressed, settle-scales-with-frame-rate,
      double-tap-emits-double-tap, single-tap-emits-after-timeout,
      long-press-passes-through, distance-rejection.
- [ ] PARITY/LVGL/DELTA cite block on every public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] SDL demo wires `GesturePipeline` between backend events and
      tree dispatch; clicking the on-screen Button fires its
      `on_click` callback.
- [ ] `playit/STATUS.md` change log records PLAYIT-04a landing.

## §13 Files cited

- `rlvgl/platform/src/gesture.rs` (v0.2.0 @ b178cbc).
- `rlvgl/examples/disco-sim/src/main.rs:158-200` (canonical
  pipeline composition).
- `lvglpp/docs/core-event/00-event-surface.md`,
  `lvglpp/docs/playit-tagged/00-tagged-queries.md`
  (PLAYIT-02 `EventPipeline` reference),
  `lvglpp/docs/widgets-button/00-button.md` (the consumer).

## §14 Unblocks

- **Interactive SDL demo:** clicking the Button in the host
  window fires its callback identically to a piped
  `T@ok_button:…` command.
- **PLAT-02 STM32H747I-DISCO bring-up:** the same recogniser
  drops in unchanged once the FT5336 driver produces raw pointer
  events.
- **Future PLAYIT-04a-1** (pipeline-on-piped-injects) — closes
  the remaining executor parity gap.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Duration
  constants (§5.1), `ms_to_ticks` math (§5.2), TapRecognizer FSM
  (§5.3 / §5.4), DoubleTapRecognizer FSM (§5.5 / §5.6), pipeline
  composition (§5.7) all frozen with **Standards Action**
  registration matching rlvgl byte-for-byte.
- 2026-04-27 — PLAYIT-04a execution landed.
  `playit/include/lvglpp/playit/gesture.hpp` (header-only). Test
  target `lvglpp_playit_gesture` (11 fixtures, mirroring the
  rlvgl tests at `gesture.rs:299-448`). SDL demo wires the
  pipeline between backend events and tree dispatch + per-frame
  `pipeline.tick()`; mouse clicks on the on-screen Button now
  fire its `on_click` callback. Piped playit commands continue to
  bypass the pipeline per §10 (PLAYIT-04a-1 deferred). 16/16
  ctest entries green; embedded posture clean.
