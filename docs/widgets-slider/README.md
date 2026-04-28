<!--
README.md — Initiative README for the WID-04 Slider chapter.
-->

# widgets-slider — initiative README

This initiative ratifies `lvglpp::widgets::Slider`. The first
**value-bound** widget in lvglpp: `min`/`max`/`value` with a
`PressRelease`-driven ratio mapping from x-coordinate to value.

The normative artifact is [`00-slider.md`](./00-slider.md).

## Status

Chapter ratified at draft level (2026-04-27). WID-04 execution is
unblocked by WID-01 / WID-02 / WID-03 + CORE-04a `fill_rounded_rect`
shim.

## Cross-language pair

Mirrors `rlvgl/widgets/src/slider.rs` (v0.2.0 @ 79f730d). The
radius=0 visual path matches byte-for-byte; rounded track / knob
arrive with CORE-04b.
