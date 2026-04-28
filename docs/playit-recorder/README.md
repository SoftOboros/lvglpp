<!--
README.md — Initiative README for the PLAYIT-06 EventRecorder.
-->

# playit-recorder — initiative README

This initiative ratifies the **event recorder** that captures
EventSpec values flowing through the playit Executor and replays
them on the wire when the host issues `RS` / `RE` / `RD`. The
recording side is the diagnostic loop the lvglpp project will use
to capture interaction sequences and hand them back to a rlvgl
playit host (or vice-versa) for replay.

Chapters:

- [00-event-recorder.md](./00-event-recorder.md) — Recorder ring-buffer,
  `format_event_spec`, dump wire format, RS/RE/RD wiring at the
  Executor level.

## Status

Chapter ratified at draft level (2026-04-27). Execution unblocked
by PLAYIT-04b (format_response) + PLAYIT-07 (Executor).

## Cross-language pair

Mirrors `rlvgl/playit/src/recorder.rs` + the `dump_recording()`
helper at `rlvgl/playit/src/executor.rs:343` (v0.2.0 @ 79f730d).
**Documented DELTA**: lvglpp v1 emits a monotonic per-entry
sequence number (`@<seq>`) instead of rlvgl's `@<tick_delta>`.
A future sub-phase (PLAYIT-06a) will introduce a tick-counter
seam to land the rlvgl-equivalent tick-delta dump.
