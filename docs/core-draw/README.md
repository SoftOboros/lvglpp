<!--
README.md — Initiative README for the text/draw/image/mask chapter
(LPAR-08).
-->

# core-draw — initiative README

Wraps LVGL's draw pipeline — text/glyph metrics, draw primitives, image
descriptors/decoders, and masks/layers — as C++ over `lv_draw_*`,
`lv_font_*`, `lv_image_*`. The heaviest substrate phase; ~all text-bearing
and visual widgets depend on it.

This README is **informative**. The normative artifact is
[00-text-draw-image-mask.md](./00-text-draw-image-mask.md) (**LPAR-08**);
the umbrella is [`../lpar/README.md`](../lpar/README.md). Mirrors rlvgl
`v0.2.4` `docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md`; the FONT family
(`docs/font/`) builds on it.
