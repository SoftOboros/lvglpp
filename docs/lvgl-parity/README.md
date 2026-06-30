# lvglpp LVGL Parity

This family tracks the C++ mirror of rlvgl's LPAR and SCTD work at the
`rlvgl` submodule pin `v0.2.5 @ f999f75`.

The normative artifacts are:

- [`00-concepts.md`](00-concepts.md) — lvglpp parity plan, phase gates,
  LVGL-underneath policy, and SCTD state-chart demo ordering.
- [`01-baseline.md`](01-baseline.md) — LVGL / rlvgl pin, current lvglpp
  coverage, and missing parity-wrapper matrix.
- [`02-object-substrate.md`](02-object-substrate.md) — planned
  LVGL-backed `lv_obj_t` ownership, views, flags/states, tree
  operations, and userdata lifetimes.
- [`03-invalidation-display.md`](03-invalidation-display.md) — ratified
  and implemented
  LVGL invalidation/display wrapper plan using LVGL dirty tracking and
  flush callbacks underneath.
- [`04-event-focus-input.md`](04-event-focus-input.md) — ratified and
  implemented
  LVGL event, focus-group, input-device, and playit injection plan using
  LVGL event/input routing underneath.
- [`05-scroll-runtime.md`](05-scroll-runtime.md) — ratified and
  implemented LVGL scroll flag, offset, scrollbar, snap, event, and
  synthetic-input plan using LVGL scroll routing underneath.
- [`06-timers-object-anim.md`](06-timers-object-anim.md) — ratified and
  implemented
  LVGL timer and animation wrapper plan using LVGL `lv_timer_t`,
  `lv_anim_t`, and explicit tick-driving underneath.
- [`07-style-theme.md`](07-style-theme.md) — ratified and implemented
  LVGL style and theme wrapper plan using LVGL `lv_style_t`, object
  style selectors, transition descriptors, and `lv_theme_t` underneath.
- [`08-text-draw-image-mask.md`](08-text-draw-image-mask.md) — ratified and implemented
  LVGL text, font, label, image descriptor, image widget, draw
  descriptor, and mask wrapper plan using LVGL public APIs underneath.

The README is informative. Per-phase chapters own implementation
contracts once they are split out.
