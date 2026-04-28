<!--
README.md — Initiative README for the CORE-03 Widget tree chapter.
-->

# core-widget — initiative README

This initiative ratifies the lvglpp widget abstraction, the geometry
type (`Rect`), and the color value type (`Color`) — the load-bearing
shape that every concrete widget compiles against.

This README is **informative**. The normative artifact is the chapter
[`00-widget-tree.md`](./00-widget-tree.md).

## Chapters

- [00-widget-tree.md](./00-widget-tree.md) — `Widget` virtual surface,
  `Rect`, `Color`, `clear_region` semantics.

## Conformance target

A conforming `lvglpp::core::Widget` implementation MUST satisfy the
Acceptance checklist in
[`00-widget-tree.md`](./00-widget-tree.md#12-acceptance-checklist).

## Status

Chapter ratified at draft level (2026-04-27). CORE-03 execution is
unblocked.

## Cross-language pair

- **rlvgl side**: mirrors `rlvgl/core/src/widget.rs` (v0.2.0 @
  b178cbc) — `Widget` trait + `Rect` + `Color`. No rlvgl change is
  required to land lvglpp's CORE-03 implementation.
