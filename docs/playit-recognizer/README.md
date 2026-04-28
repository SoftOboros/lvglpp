<!--
README.md — Initiative README for the PLAYIT-04a gesture recognizer.
-->

# playit-recognizer — initiative README

This initiative ratifies the **gesture recognizer pipeline** that
sits between raw input (SDL mouse events, board touch IRQs) and the
widget tree. Without it, raw `Pointer{Down,Up,Move}` events flow
straight into widgets — and `Button` only consumes the debounced
`PressRelease` (WID-02 §5.3), so SDL clicks don't fire callbacks
unless something synthesises the gesture event.

The recogniser closes that gap: it converts raw pointer events into
the gesture events (`PressDown`, `PressRelease`, `DoubleTap`) that
widgets actually consume.

Chapters:

- [00-tap-and-double-tap.md](./00-tap-and-double-tap.md) —
  `TapRecognizer`, `DoubleTapRecognizer`, `GesturePipeline`
  (concrete `EventPipeline` composing both).

## Status

Chapter ratified at draft level (2026-04-27). Execution unblocked
by PLAYIT-02 (`EventPipeline` abstract base) + CORE-02 (`Event`
variants).

## Cross-language pair

Mirrors `rlvgl/platform/src/gesture.rs` (v0.2.0 @ 79f730d) — same
duration constants, same algorithm, same tick-based timer. The
`DiscoGesturePipeline` composition shape from
`rlvgl/examples/disco-sim/src/main.rs:158` is the canonical
reference for how `TapRecognizer` and `DoubleTapRecognizer`
compose.

**Note:** rlvgl places the recogniser in `rlvgl-platform` because
embedded targets (STM32H747I-DISCO) need it for capacitive-touch
debounce. lvglpp places it in `lvglpp::playit` because PLAYIT
already houses the EventPipeline trait it composes with. A future
relocation to `lvglpp::core` is fine if a non-playit consumer
appears (the recogniser has zero playit-specific dependencies).
