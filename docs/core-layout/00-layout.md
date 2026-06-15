# 00 — Layout (flex & grid)

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-10**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. [`../lpar/README.md`](../lpar/README.md)
is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Layout / sizing **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-10-LAYOUT.md` (@ `343f596`) |
| The **primitive** | `lvgl/src/layouts/flex/lv_flex.h`, `lvgl/src/layouts/grid/lv_grid.h`, `lvgl/src/core/lv_obj_pos.h` |
| Existing layout helpers | UI-04 (stubbed) |

## §1 Purpose

Wrap LVGL's flex and grid layout engines and the sizing model
(`lv_obj_set_size`, content sizing, align) as C++ on `Object`, replacing
the stubbed UI-04 helper surface.

## §2 Problem statement

rlvgl re-implements flex/grid engines + sizing (`core::layout`,
`Dimension`, `LayoutState`). LVGL ships both layout engines. lvglpp wraps
`lv_obj_set_flex_flow`/`flex_grow`/`flex_align` and
`lv_obj_set_grid_dsc_array`/`grid_cell`/`grid_align`, plus sizing helpers.
The stubbed `ui::layout` (UI-04) is superseded.

## §3 Canonical glossary

- **`Object::set_flex_flow(FlexFlow)`** etc. — wrap `lv_obj_set_flex_flow`
  / `lv_obj_set_flex_grow` / `lv_obj_set_flex_align`. `FlexFlow`/
  `FlexAlign` mirror `lv_flex_flow_t`/`lv_flex_align_t`.
- **Grid** — `set_grid_dsc(...)` / `set_grid_cell(...)` wrap
  `lv_obj_set_grid_dsc_array` / `lv_obj_set_grid_cell` /
  `lv_obj_set_grid_align`. Track descriptor arrays are caller-owned and
  MUST outlive the object (LVGL stores the pointer) — an ownership rule
  analogous to LPAR-07 `Style`.
- **`Dimension`** — sizing helper over `lv_obj_set_size` /
  `LV_SIZE_CONTENT` / percentage (`lv_pct`).

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Flex/grid behavior | `lvgl` engines + rlvgl `LPAR-10` semantics |
| Flex/grid enums | `lvgl` `lv_flex_*_t` / grid aligns — **Standards Action** |
| Grid track-array lifetime | this chapter (outlives-object rule) |

## §5 Frozen decisions

1. lvglpp does NOT re-implement flex/grid; it wraps the LVGL engines.
2. **Grid track descriptors are caller-owned and MUST outlive the
   object** (LVGL stores the array pointer) — tagged `borrows`-into-LVGL,
   same hazard class as LPAR-07 `Style`.
3. `FlexFlow`/`FlexAlign`/grid aligns are frozen mirror enums.
4. UI-04 helper API is superseded; any kept helper is re-expressed over
   `lv_obj` layout.

## §10 Reconciliation vs. adjacent primitives

- **UI-04 layout helpers (stubbed)** — superseded by this chapter; the
  UI module re-exports or drops them (recorded as a UI DELTA).
- **`Container` (WID-05/DEMO-01)** — gains flex/grid via these wrappers
  once migrated onto `Object` (`LVGLPP-WRAP`).

## §11 Non-goals

- Widget-internal layouts (Tabview/Menu/Keyboard own theirs in LPAR-13).

## §12 Acceptance checklist

- [ ] `Object` flex setters wrap `lv_obj_set_flex_*`; grid setters wrap
      `lv_obj_set_grid_*`, with the track-array outlives-object rule
      documented.
- [ ] `FlexFlow`/`FlexAlign`/grid-align mirror enums; `Dimension` sizing.
- [ ] UI-04 supersession recorded as a DELTA.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-10.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-10-LAYOUT.md` (v0.2.4 @ `343f596`)
- `lvgl/src/layouts/flex/lv_flex.h`, `lvgl/src/layouts/grid/lv_grid.h`, `lvgl/src/core/lv_obj_pos.h`
- `core/include/lvglpp/core/object.hpp` (WRAP-00)

## §14 Unblocks

- LPAR-13/14 composite widgets (Menu/Window/Table/Keyboard placement),
  Container migration.

## §15 Change log

- **2026-06-15** — LPAR-10 drafted: wrap `lv_flex`/`lv_grid` + sizing;
  freeze the grid-track-array outlives-object rule; supersede UI-04.
  **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
