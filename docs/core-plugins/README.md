<!--
README.md — Initiative README for the CORE-07 Plugin surface chapter.
-->

# core-plugins — initiative README

This initiative ratifies the **plugin registration mechanism** —
how lvglpp wires optional decoders / generators (PNG, JPEG, GIF,
QR, Lottie, Canvas, FATFS, fontdue, …) into the core surface
without forcing every consumer to compile every dependency.

Per-plugin concepts (the actual decoder semantics for each format)
land in their **own concepts docs** as sub-phases:

- CORE-07a — PNG plugin
- CORE-07b — JPEG plugin
- CORE-07c — GIF plugin
- … (one per `rlvgl/core/src/plugins/<name>.rs` we eventually port)

Per-plugin chapters are deferred until their first lvglpp call site
needs them.

This README is **informative**. The normative artifact is the chapter
[`00-plugin-surface.md`](./00-plugin-surface.md).

## Status

Chapter ratified at draft level (2026-04-27). CORE-07 execution
(the registration mechanism + a no-op smoke plugin) is unblocked by
CORE-04 ratification.

## Cross-language pair

Mirrors `rlvgl/core/src/plugins/mod.rs` (v0.2.0 @ 79f730d).
