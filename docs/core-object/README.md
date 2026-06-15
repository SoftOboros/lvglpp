<!--
README.md — Initiative README for the core-object substrate chapters
(LPAR-02 object substrate, LPAR-03 invalidation/display).
-->

# core-object — initiative README

These chapters wrap the LVGL object-tree substrate — flags, states,
hit-testing, lifecycle (LPAR-02) and invalidation / display refresh
(LPAR-03) — as RAII C++ over `lv_obj_*`, on top of the `Object`/`Screen`
core from `LVGLPP-WRAP-00` (`docs/wrap/`).

This README is **informative**. The normative artifacts are the chapters
below; the initiative umbrella is [`../lpar/README.md`](../lpar/README.md).

## Chapters

- [00-object-substrate.md](./00-object-substrate.md) — **LPAR-02**: flags,
  states, hit-test, parent/child, lifecycle over `lv_obj_*`.
- [01-invalidation-display.md](./01-invalidation-display.md) — **LPAR-03**:
  `lv_obj_invalidate`, `lv_display` flush / refresh, render modes.

## Cross-language pair

Mirrors rlvgl `v0.2.4` `docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md` and
`LPAR-03-INVALIDATION-DISPLAY.md` (@ `343f596`). rlvgl re-implements this
substrate in Rust; lvglpp wraps the equivalent upstream `lv_*` surface.
