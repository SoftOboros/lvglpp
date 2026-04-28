<!--
README.md — Initiative README for the CORE-05 Style/Theme/Animation chapter.
-->

# core-style — initiative README

Combined initiative covering the appearance layer: `Style` value type
+ `StyleBuilder`, `Theme` virtual surface + light/dark themes, and
the animation primitives `Easing` + `LoopMode`. The richer animation
types (Fade, Slide, Motion, FadeTransition, Timeline) are deferred to
follow-up sub-phases under this same initiative.

This README is **informative**. The normative artifact is the chapter
[`00-appearance.md`](./00-appearance.md).

## Chapters

- [00-appearance.md](./00-appearance.md) — `Style`, `StyleBuilder`,
  `Theme`, `LightTheme`, `DarkTheme`, `Easing`, `LoopMode`.

## Status

Chapter ratified at draft level (2026-04-27). CORE-05 execution is
unblocked.

## Cross-language pair

Mirrors `rlvgl/core/src/style.rs`, `theme.rs`, and the `Easing` /
`LoopMode` enums in `animation.rs` (v0.2.0 @ 79f730d). The richer
animation surface (Fade / Slide / Motion / Timeline) is **out of
scope** for this chapter; those land as CORE-05a / CORE-05b / …
sub-phases when their first call site needs them.
