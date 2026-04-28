# 01 — Tick-delta dump (rlvgl wire parity)

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAYIT-06a** (sub-phase under playit-recorder).
Supersedes: PLAYIT-06 §5.3 (the `@<seq>` form is replaced).

## §0 Authority

- `EventRecorder` tick semantics + `tick_delta` shape:
  `rlvgl/playit/src/recorder.rs` (v0.2.0 @ 79f730d). Canonical.
- `dump_recording` per-entry wire form:
  `rlvgl/playit/src/executor.rs:343`. Canonical.
- Saturating tick math:
  `rlvgl/playit/src/recorder.rs:120` (`saturating_sub` + `min`).

## §1 Purpose

Close the rlvgl wire-protocol parity. PLAYIT-06 §5.3 documented a
`@<seq>` placeholder for the recorder dump; this chapter swaps in
`@<tick_delta>` so a captured-on-rlvgl fixture can replay against
lvglpp (and vice-versa) **byte-for-byte** without a normaliser.

## §2 Problem statement

The PLAYIT-06 v1 dump used a monotonic per-entry sequence number
because no tick-counter seam existed. With the SDL demo loop now in
place (PLAYIT-07 + the `examples/host_sdl_label/` integration),
threading a per-frame tick into the Recorder is one line. With that
in hand, the dump can match rlvgl's format exactly.

## §3 Canonical glossary

- **`tick_delta`** — As defined in
  `rlvgl/playit/src/recorder.rs:32`. `uint16_t`, saturating at
  `UINT16_MAX`, computed as `tick_counter - last_event_tick` at
  `record()` time. The first entry after `start()` always has
  `tick_delta == 0` because `tick_counter == last_event_tick == 0`.
- **`tick()`** — As defined in `rlvgl/playit/src/recorder.rs:134`.
  Advances `tick_counter` by 1 **only while the recorder is
  running**. Wraps at `uint32_t` (rlvgl uses `wrapping_add`).
- **Fill-and-stop** — As defined in
  `rlvgl/playit/src/recorder.rs:114-130`. When the buffer reaches
  capacity, `running` is set to `false`. Subsequent `record()`
  calls are no-ops. **DELTA from PLAYIT-06 v1**: lvglpp v1 used a
  ring buffer that overwrote oldest entries. PLAYIT-06a replaces
  that with rlvgl's fill-and-stop semantics.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `tick_delta` field shape (`uint16_t` saturating) | rlvgl `recorder.rs:32` (canonical) | `lvglpp::playit::EventRecorder::Entry`. |
| `tick()` advance rule (only while running, wrapping `u32`) | rlvgl `recorder.rs:134` (canonical) | `lvglpp::playit::EventRecorder::tick()`. |
| Fill-and-stop on full buffer | rlvgl `recorder.rs:114` (canonical) | `lvglpp::playit::EventRecorder::record()`. |
| `start()` clears tick counters | rlvgl `recorder.rs:80` (canonical) | `lvglpp::playit::EventRecorder::start()`. |
| Dump line form `@<tick_delta> <event>\r\n` | rlvgl `executor.rs:343` (canonical) | `lvglpp::playit::Executor::dump_recording`. |

## §5 Frozen decisions

### §5.1 `Entry` field set — **Standards Action**

Replaces PLAYIT-06 §5.1's monotonic-seq form:

| Field | Type | Notes |
| --- | --- | --- |
| `tick_delta` | `std::uint16_t` | Ticks elapsed since the previous record, saturating at `UINT16_MAX`. First entry after `start()` is `0`. |
| `spec` | `EventSpec` | Unchanged. |

The `seq` field from v1 is **removed**. Tests that compared `seq`
values are rewritten in this PR.

### §5.2 `tick()` — **Standards Action**

```
void EventRecorder::tick() noexcept {
    if (running_) {
        tick_counter_ = static_cast<uint32_t>(tick_counter_ + 1U);
    }
}
```

- `tick_counter_` is `uint32_t` and wraps on overflow (rlvgl's
  `wrapping_add`).
- `tick()` is a no-op when the recorder is stopped (rlvgl preserves
  the previous tick_counter across stop/start).
- Callers SHOULD call `tick()` exactly once per main-loop frame.
  The SDL demo at `examples/host_sdl_label/main.cpp` calls it
  immediately after `executor.poll()`, before `present_frame()`.

### §5.3 `record()` delta computation — **Standards Action**

```
delta = saturating_sub(tick_counter_, last_event_tick_);
entry.tick_delta = (delta > UINT16_MAX) ? UINT16_MAX : (uint16_t)delta;
last_event_tick_ = tick_counter_;
```

- Must use **saturating subtraction**: when the recorder wraps in
  the unlikely-but-defined u32 sense, the delta does not produce a
  garbage huge value. Mirrors rlvgl's `saturating_sub`.
- Must clamp delta to `uint16_t::MAX = 65535`. Mirrors
  `rlvgl/playit/src/recorder.rs:122`.

### §5.4 Fill-and-stop — **Standards Action**

When `len_ >= CAPACITY`, the recorder sets `running_ = false`.
Subsequent `record()` calls are no-ops. The dump still emits
whatever was captured before the buffer filled. This **replaces**
the ring-overwrite behavior from PLAYIT-06 v1.

`start()` resets `len_`, `tick_counter_`, `last_event_tick_`, and
sets `running_ = true`. Existing buffer contents are discarded.

### §5.5 Dump wire format — **Standards Action**

The dump line for each entry becomes:

```
@<tick_delta> <event-line>\r\n
```

Identical to rlvgl byte-for-byte. The `REC:START,<count>\r\n`
header and `REC:END\r\n` footer (PLAYIT-06 §5.3) are unchanged.

## §10 Reconciliation vs. adjacent primitives

- **PLAYIT-06 v1 (this chapter's predecessor).** §5.1 (Entry shape)
  and §5.3 (dump format) are superseded. §5.2, §5.4, §5.5 of
  PLAYIT-06 remain unchanged.
- **rlvgl `recorder.rs`.** With this chapter, lvglpp's recorder is
  a **byte-for-byte parity port** of rlvgl's. Captured fixtures
  diff cleanly across the language pair.
- **SDL demo loop.** The demo gains exactly one new line: `recorder.tick();` immediately after `executor.poll()`. Other
  callers (embedded targets, headless test fixtures) are
  responsible for advancing tick at their natural per-frame rate.

## §11 Non-goals

- Sub-tick timing. The recorder records at frame granularity; finer
  timing belongs in a later sub-phase if a use case appears.
- Exposing `tick_counter` on the public API. It's an implementation
  detail; only `tick()` is public.
- Replay-side tick-delta consumption. The host script that replays
  a captured fixture decides how to interpret the deltas;
  out-of-scope here.

## §12 Acceptance checklist

- [ ] `EventRecorder::Entry::tick_delta` (`uint16_t`) replaces
      `seq` (`uint32_t`).
- [ ] `EventRecorder::tick()` advances only while running, wraps
      on `uint32_t` overflow.
- [ ] `EventRecorder::record()` computes saturating delta and
      clamps to `UINT16_MAX`.
- [ ] Fill-and-stop replaces ring overwrite — verified by a test
      that records past CAPACITY and asserts `running() == false`,
      `size() == CAPACITY`, and oldest entries are preserved
      (mirrors `rlvgl/playit/src/recorder.rs:236` test).
- [ ] Executor's `dump_recording` emits `@<tick_delta>` per §5.5.
- [ ] SDL demo calls `recorder.tick()` once per main-loop frame.
- [ ] `lvglpp_playit_recorder` test rewritten to assert tick-delta
      semantics; `lvglpp_playit_executor` and other Executor tests
      remain unaffected.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] PLAYIT-06 §5.3 amended to point at this chapter.
- [ ] `playit/STATUS.md` change log records PLAYIT-06a landing.
- [ ] End-to-end smoke (piping `RS\nT@…\nT@…\nRD\n` through the
      SDL demo with `SDL_VIDEODRIVER=dummy`) shows `@<tick_delta>`
      values that match rlvgl's expectation pattern (first entry
      0, subsequent entries reflect frame deltas).

## §13 Files cited

- `rlvgl/playit/src/recorder.rs` (v0.2.0 @ 79f730d).
- `rlvgl/playit/src/executor.rs:343` (v0.2.0 @ 79f730d).
- `lvglpp/docs/playit-recorder/00-event-recorder.md` (this
  chapter's predecessor).

## §14 Unblocks

- Cross-language fixture replay with byte-identical dumps. A
  fixture captured on rlvgl can be replayed against lvglpp and the
  resulting dump diffed without a normaliser.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Entry shape (§5.1),
  `tick()` (§5.2), saturating delta (§5.3), fill-and-stop (§5.4),
  dump line format (§5.5) all frozen with **Standards Action**
  registration, all matching rlvgl byte-for-byte.
- 2026-04-27 — PLAYIT-06a execution landed.
  `EventRecorder` rewritten: `Entry::tick_delta` (uint16_t
  saturating), `tick()`, fill-and-stop. `Executor::dump_recording`
  emits `@<tick_delta>` per §5.5. SDL example calls
  `recorder.tick()` once per frame after `present_frame()`.
  Test target `lvglpp_playit_recorder` rewritten — added
  `tick_delta_basic`, `tick_delta_saturates`,
  `tick_while_stopped_is_noop`, `fill_and_stop`,
  `executor_dump_with_tick_advance`. PLAYIT-06 §5.3 amended to
  point at this chapter. End-to-end wire output verified against
  the SDL demo with `SDL_VIDEODRIVER=dummy`. 15/15 ctest entries
  green; embedded posture clean.
