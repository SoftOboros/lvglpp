# 00 — Event recorder

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAYIT-06**.

## §0 Authority

- Recorder shape: `rlvgl/playit/src/recorder.rs` (v0.2.0 @ b178cbc).
- Dump wire format: `rlvgl/playit/src/executor.rs:343`
  (`dump_recording()`).
- Per-event wire format: `rlvgl/playit/src/protocol.rs:359`
  (`format_event_spec`).
- Executor pump: PLAYIT-07.

## §1 Purpose

Capture interaction sequences for diagnostics + replay. The
recorder is the load-bearing piece of the cross-language regression
loop: a sequence captured on a rlvgl target can be replayed against
lvglpp, and vice-versa. Same wire format → same fixture file.

## §3 Canonical glossary

- **`EventRecorder`** — Owned by this chapter. Mirrors
  `rlvgl/playit/src/recorder.rs`. Fixed-capacity ring buffer of
  `Entry{seq, EventSpec}`. State: `running` / not.
- **`Entry`** — `(uint32_t seq, EventSpec spec)`. Mirrors rlvgl's
  recorder entry shape with **DELTA**: rlvgl's `tick_delta` is
  replaced by a monotonic per-entry sequence number for v1; tick
  counting integration is a PLAYIT-06a sub-phase.
- **`format_event_spec`** — Free function `format_event_spec(EventSpec, span<char>) -> size_t`. Mirrors `rlvgl/playit/src/protocol.rs:359` byte-for-byte (`TK`, `T<x>,<y>`, `TD<x>,<y>`, `TT<x>,<y>`, `PD/PU/PM<x>,<y>`, `KD:<key>`/`KU:<key>`, `MT<n>:<id>,<state>,<x>,<y>;...`).

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Recorder ring shape | rlvgl `recorder.rs` (canonical) | `lvglpp::playit::EventRecorder`. |
| Per-event wire form | rlvgl `protocol.rs:359` (canonical) | `lvglpp::playit::format_event_spec`. |
| Dump wire form | rlvgl `executor.rs:343` (canonical) | Executor `RS`/`RE`/`RD` handler. |

## §5 Frozen decisions

### §5.1 `EventRecorder` capacity — **Specification Required**

`EventRecorder::CAPACITY = 256` entries (compile-time constexpr).
When the buffer fills, oldest entries are overwritten (ring
behavior). Mirrors rlvgl's default.

### §5.2 `format_event_spec` per-variant strings — **Standards Action**

Byte-for-byte parity with `rlvgl/playit/src/protocol.rs:359`:

| EventSpec variant | Wire form |
| --- | --- |
| `Tick` | `TK` |
| `PressRelease{x,y}` | `T<x>,<y>` |
| `PressDown{x,y}` | `TD<x>,<y>` |
| `DoubleTap{x,y}` | `TT<x>,<y>` |
| `PointerDown{x,y}` | `PD<x>,<y>` |
| `PointerUp{x,y}` | `PU<x>,<y>` |
| `PointerMove{x,y}` | `PM<x>,<y>` |
| `KeyDown{key}` | `KD:<key-name>` |
| `KeyUp{key}` | `KU:<key-name>` |
| `Touch{count,points}` | `MT<count>:<id>,<state>,<x>,<y>;...` |

`<key-name>` mirrors rlvgl's keyspec writer:
- Named keys: `Escape`, `Enter`, `Space`, `ArrowUp`, `ArrowDown`,
  `ArrowLeft`, `ArrowRight`.
- `Function(n)`: `F<n>`.
- `Character(c)`: ASCII byte directly when c is in 0x20..=0x7E
  (UTF-8 multi-byte deferred — rlvgl uses `encode_utf8` which is
  out of scope for v1).
- `Other(v)`: decimal value as-is.

`<state>` mirrors `format_event_spec`'s touch-state byte: `D` /
`U` / `C`.

### §5.3 Dump wire format — **Standards Action**

When the Executor handles `RD` (and as part of `RE`), it emits:

```
REC:START,<count>\r\n
@<seq> <event-line>\r\n
@<seq> <event-line>\r\n
...
REC:END\r\n
```

`<count>` is the recorder's current length (uint16_t). `<seq>` is
the per-entry monotonic sequence number assigned at `record()`
time, starting at 0 each `RS`.

**SUPERSEDED by PLAYIT-06a** (2026-04-27): the `@<seq>` form is
replaced by `@<tick_delta>` for byte-for-byte parity with rlvgl.
See `docs/playit-recorder/01-tick-delta.md`. The lines above are
preserved for history; the live wire format is now
`@<tick_delta> <event-line>\r\n` per PLAYIT-06a §5.5.

### §5.4 Executor wiring — **Standards Action**

The Executor (PLAYIT-07) gains an `EventRecorder*` slot
(default `nullptr`). When non-null:

- `Inject{spec}` and `InjectTagged{tag, spec}` Commands cause
  `recorder->record(spec)` BEFORE the Dispatcher consumes them
  (only when `recorder->running()`).
- `RecordStart` Command: `recorder->start()`; Executor writes
  `REC:recording\r\n` (matching rlvgl) and skips the Dispatcher.
- `RecordStop`: `recorder->stop()`; Executor performs the §5.3
  dump via `dump_recording(transport)`; skips the Dispatcher.
- `RecordDump`: same dump as `RecordStop` but does NOT stop the
  recorder.

When the recorder slot is `nullptr`, RS/RE/RD pass through to the
Dispatcher (which returns `Error{"not implemented"}` per
PLAYIT-04 §5.3).

## §10 Reconciliation vs. adjacent primitives

- **Dispatcher (PLAYIT-04).** Dispatcher continues to reject RS/
  RE/RD with `Error{"not implemented"}` because the recorder is
  Executor-level state. Tests that go through Dispatcher directly
  (without the Executor wrapper) see the unchanged behavior.
- **`format_response` (PLAYIT-04b).** Recorder dump bypasses the
  Response sum type and writes raw lines through the transport
  (matches rlvgl). The dump is multi-line; squeezing it through
  Response would require a streaming Response API, which is
  deferred.

## §11 Non-goals

- Tick-delta dump format. PLAYIT-06a.
- Replay-from-buffer (host-side; the recorder only captures).
- Persisting recordings to disk. Application-level concern.
- Multi-byte UTF-8 character keys in dump. v1 emits ASCII only;
  out-of-range Character codepoints render via the `Other(v)`
  decimal path.

## §12 Acceptance checklist

- [ ] `lvglpp::playit::EventRecorder` with the §3 surface and the
      §5.1 capacity.
- [ ] `lvglpp::playit::format_event_spec(EventSpec, span<char>)`
      mirrors §5.2 byte-for-byte for every EventSpec variant.
- [ ] Executor wiring per §5.4 — verified by a fixture that:
      `RS` → record some events → `RD` and asserts the dump.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] `playit/STATUS.md` change log records PLAYIT-06 landing.

## §13 Files cited

- `rlvgl/playit/src/recorder.rs` (canonical),
  `rlvgl/playit/src/executor.rs:343` (canonical),
  `rlvgl/playit/src/protocol.rs:359` (canonical).
- `lvglpp/docs/playit-tagged/01-response-formatter.md`,
  `lvglpp/docs/playit-transport/00-transport-and-executor.md`.

## §14 Unblocks

- A captured rlvgl-side fixture file can be replayed against
  lvglpp; resulting dump can be diffed against the rlvgl-side
  reference (modulo the `@<seq>` vs `@<tick_delta>` normaliser
  noted in §5.3).
- Diagnostic capture during the SDL example demo: pipe `RS`,
  click around the window, pipe `RD` to dump the captured input
  sequence.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Capacity (§5.1),
  per-event wire form (§5.2), dump format (§5.3), Executor wiring
  (§5.4) all frozen. PLAYIT-06a deferred (tick-delta).
- 2026-04-27 — PLAYIT-06 execution landed.
  `playit/include/lvglpp/playit/event_recorder.hpp` (256-entry
  ring); `format_event_spec` added to `format.{hpp,cpp}`. Executor
  gained `set_recorder(...)` + intercept logic for RS/RE/RD per
  §5.4. Inject / InjectTagged are auto-recorded when running. Test
  target `lvglpp_playit_recorder` (10 fixtures) green: format
  parity per §5.2, ring overwrites oldest, RS emits the recording
  marker, RD/RE produce the §5.3 wire format, no-recorder path
  falls through to Dispatcher's "not implemented" arm intact.
- 2026-04-27 — §5.3 superseded by PLAYIT-06a
  (`docs/playit-recorder/01-tick-delta.md`). The recorder now emits
  `@<tick_delta>` byte-for-byte with rlvgl and uses fill-and-stop
  semantics (no ring overwrite). The PLAYIT-06 chapter sections
  §5.1, §5.2, §5.4 (Executor wiring), §5.5 remain unchanged; the
  superseded §5.3 paragraph is preserved above for history.
