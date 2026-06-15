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
| `docs/wrap/` (LVGLPP-WRAP) | object-model unification | Owned by LPAR-00 §7; chapter pending |
| `docs/core-*` / `docs/widgets-*` per-phase chapters | LPAR-02..16, FONT-00..05 | Owned by LPAR-00 §7; chapters land per wave |

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
