<!--
README.md — Initiative README for the CORE-06 Fonts chapter.
-->

# core-font — initiative README

This initiative ratifies the lvglpp font surface — fixed-width
1-bit-packed `BitmapFont` and variable-width grayscale `PackedFont`
with `GlyphMetric`. These are the consumers of `rlvgl-creator`-emitted
font assets on the lvglpp side; the chapter pins the asset-loader
seam so the rust→C++ creator path stays well-worn (per
[`CLAUDE.md`](../../CLAUDE.md) § "Doc Co-Location Policy" — playit /
creator seam).

This README is **informative**. The normative artifact is the chapter
[`00-fonts.md`](./00-fonts.md).

## Status

Chapter ratified at draft level (2026-04-27). CORE-06 execution is
unblocked by CORE-04 ratification.

## Cross-language pair

Mirrors `rlvgl/core/src/bitmap_font.rs` and `packed_font.rs` (v0.2.0
@ 79f730d).
