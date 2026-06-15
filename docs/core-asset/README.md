<!--
README.md — Initiative README for the asset & filesystem chapter (LPAR-09).
-->

# core-asset — initiative README

Wraps LVGL's filesystem + image-decoder source model as C++ over
`lv_fs_*` and `lv_image_decoder_*`, giving widgets typed asset sources
(embedded / FATFS / simulator / memory).

This README is **informative**. The normative artifact is
[00-asset-filesystem.md](./00-asset-filesystem.md) (**LPAR-09**); the
umbrella is [`../lpar/README.md`](../lpar/README.md). Mirrors rlvgl
`v0.2.4` `docs/concepts/LPAR-09-ASSET-FILESYSTEM.md`; reconciles the
CORE-07 plugin slots and CORE-07n RLE decoder.
