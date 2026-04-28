<!--
README.md — Initiative README for the CORE-04 Renderer chapter.
-->

# core-renderer — initiative README

This initiative ratifies the lvglpp renderer abstraction — the
target-agnostic drawing interface every backend (host SDL, STM32
DMA2D, BBB DRM, ESP32 LCD, …) implements and every widget draws into.

This README is **informative**. The normative artifact is the chapter
[`00-renderer-trait.md`](./00-renderer-trait.md).

## Chapters

- [00-renderer-trait.md](./00-renderer-trait.md) — `Renderer` virtual
  surface, default implementations, blend / blit semantics.

## Status

Chapter ratified at draft level (2026-04-27). CORE-04 execution is
unblocked by CORE-03 ratification.

## Cross-language pair

Mirrors `rlvgl/core/src/renderer.rs` (v0.2.0 @ b178cbc) — `Renderer`
trait. No rlvgl change is required to land lvglpp's CORE-04
implementation.
