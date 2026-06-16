<!--
README.md — Initiative README for the LPAR (LVGL parity) family in lvglpp.
-->

# lpar — initiative README

LPAR (**L**VGL **par**ity) is the lvglpp initiative that brings the C++
wrapper up to the **rlvgl v0.2.4** widget-and-runtime surface. It mirrors
the rlvgl `LPAR-*` and `FONT-*` concepts families (`rlvgl/docs/concepts/`)
onto upstream LVGL via **RAII wrappers that call `lv_*` directly**, rather
than re-implementing the runtime in C++.

This README is **informative**. The normative artifacts are the
per-chapter concepts docs (this directory's `NN-*.md` files and the
per-widget / per-substrate chapters co-located under `docs/<topic>/`).

## The reframe (why lvglpp's LPAR is smaller than rlvgl's)

rlvgl needed LPAR-02..10 because Rust **re-implements** LVGL's object
tree, draw pipeline, scroll, layout, style, and timers from scratch.
lvglpp **wraps the real LVGL C library** — so almost all of that
substrate already exists upstream and the lvglpp substrate phases
collapse into thin RAII-over-`lv_*` wrappers plus ownership contracts:

| rlvgl substrate phase | lvglpp wraps |
| --- | --- |
| LPAR-02 object / LPAR-03 invalidation | `lv_obj_*`, `lv_obj_invalidate` |
| LPAR-04 event / focus / input | `lv_event_*`, `lv_group_*`, `lv_indev_*` |
| LPAR-05 scroll / LPAR-06 timers+anim | `lv_obj_scroll_*`, `lv_timer_*`, `lv_anim_*` |
| LPAR-07 style+theme / LPAR-08 text+draw+image+mask | `lv_style_*`, `lv_obj_set_style_*`, `lv_draw_*`, `lv_font_*`, `lv_image_*` |
| LPAR-09 asset+fs / LPAR-10 flex+grid | `lv_fs_*`, `lv_obj_set_flex_*`, `lv_obj_set_grid_*` |

The bulk of the work is the **widget wrappers** (LPAR-11..15, ~30 widgets,
each a RAII handle over `lv_<widget>_create` + setters/getters + an
ownership story) and the **FONT** family.

## Architectural pivot — one object model

Until this initiative, lvglpp's own library code called **zero** `lv_*`
functions: the existing core (`Widget`/`Renderer`/`WidgetNode`/`draw_*`)
and the eight WID-01..06 widgets are hand-rolled C++ mirroring rlvgl's
Rust. LPAR unifies everything onto a single `lv_obj_t*`-backed model:

- New v0.2.4 surface wraps `lv_*` directly.
- The existing hand-rolled widgets and platform renderers are migrated
  onto the same model under the lvglpp-internal **`LVGLPP-WRAP`**
  initiative (`docs/wrap/`), including the `playit`↔`lv_obj` query bridge
  and the `lv_display_t`/`lv_indev_t` platform re-architecture.

## Chapters

| Chapter | Phase | Status |
| --- | --- | --- |
| [00-concepts.md](./00-concepts.md) | LPAR-00 | **Ratified 2026-06-15** |
| [01-baseline.md](./01-baseline.md) | LPAR-01 | **Ratified 2026-06-15** |
| [`../wrap/00-concepts.md`](../wrap/00-concepts.md) | LVGLPP-WRAP-00 | **Ratified + landed 2026-06-15** (RAII `Object`/`Screen`) |

### Wave 1 — substrate wrappers (ratified 2026-06-15; **9/9 implemented ✅**)

| Phase | Chapter | Wraps | Status |
| --- | --- | --- | --- |
| LPAR-02 | [`../core-object/00-object-substrate.md`](../core-object/00-object-substrate.md) | `lv_obj_*` flags/state/hit-test | ✅ landed |
| LPAR-03 | [`../core-object/01-invalidation-display.md`](../core-object/01-invalidation-display.md) | `lv_obj_invalidate`, `lv_display_*` | ✅ landed |
| LPAR-04 | [`../core-event/01-event-focus-input.md`](../core-event/01-event-focus-input.md) | `lv_event_*`, `lv_group_*`, `lv_indev_*` | ✅ landed |
| LPAR-05 | [`../core-scroll/00-scroll-runtime.md`](../core-scroll/00-scroll-runtime.md) | `lv_obj_scroll_*` | ✅ landed |
| LPAR-06 | [`../core-timer/00-timers-object-anim.md`](../core-timer/00-timers-object-anim.md) | `lv_timer_*`, `lv_anim_*` | ✅ landed |
| LPAR-07 | [`../core-style/01-style-cascade-theme.md`](../core-style/01-style-cascade-theme.md) | `lv_style_*`, `lv_theme_*` | ✅ landed |
| LPAR-08 | [`../core-draw/00-text-draw-image-mask.md`](../core-draw/00-text-draw-image-mask.md) | `lv_draw_*`, `lv_font_*`, `lv_image_*` | ✅ landed (v1) |
| LPAR-09 | [`../core-asset/00-asset-filesystem.md`](../core-asset/00-asset-filesystem.md) | `lv_fs_*`, `lv_image_decoder_*` | ✅ landed |
| LPAR-10 | [`../core-layout/00-layout.md`](../core-layout/00-layout.md) | `lv_obj_set_flex_*`, `lv_obj_set_grid_*` | ✅ landed |

Later waves land their chapters per wave per LPAR-00 §7. **Wave 2 — Font**
is **complete** under [`../font/`](../font/) (FONT-00 selection + AA,
FONT-05 registry — both landed 2026-06-15). Waves 3–7 (LVGLPP-WRAP
migration, LPAR-11..16 widgets) follow.

## Conformance target

A conforming lvglpp v0.2.4-parity deployment **MUST** satisfy the
per-phase acceptance checklists for the phases it claims, and **MUST**
pass the LPAR-16 conformance fixtures shared with rlvgl on at least the
host-SDL, Linux-fbdev, and disco-sim surfaces. Disco-hardware conformance
is a **MAY** level (the LTDC display path is verification-blocked; see
the project status notes).

## Cross-language pair

- **rlvgl side**: every LPAR/FONT chapter mirrors an
  already-implemented, already-ratified surface in rlvgl `v0.2.4`
  (`rlvgl/docs/concepts/LPAR-*.md`, `FONT-*.md`, pinned at `343f596`).
  No rlvgl change is required to land lvglpp's implementations.
- **Change ordering**: per CLAUDE.md § "Cross-language change ordering",
  any future extension to a shared contract is a **Standards Action**
  that lands in rlvgl `v0.2.4` first, then in the lvglpp chapter, then in
  both implementations.
