# 00 — Scroll runtime

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-05**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. [`../lpar/README.md`](../lpar/README.md)
is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Scroll **semantics** (snap, throw, chaining, scrollbars) | rlvgl `v0.2.4` `docs/concepts/LPAR-05-SCROLL-RUNTIME.md` (@ `343f596`) |
| The **primitive** | `lvgl/src/core/lv_obj_scroll.h` |
| C++ idiom | this chapter + `docs/wrap/00-concepts.md` |

## §1 Purpose

Expose LVGL scrolling as methods on `Object`: scroll position, direction,
scrollbar mode, and snap, wrapping `lv_obj_scroll_*`. Momentum/throw is
LVGL's; lvglpp does not re-implement the physics.

## §2 Problem statement

rlvgl re-implements a scroll controller with throw/momentum and snap
(`core::scroll`) plus a `ScrollView` (REND-00). LVGL ships all of this.
lvglpp wraps `lv_obj_scroll_to`/`_by`, `lv_obj_get_scroll_x`/`_y`,
`lv_obj_set_scroll_dir`, `lv_obj_set_scrollbar_mode`, and
`lv_obj_set_scroll_snap_x`/`_y`. rlvgl's REND-00 `ScrollView` folds into
"any scrollable `Object`".

## §3 Canonical glossary

- **Scroll accessors** — `Object::scroll_to(x,y)`, `scroll_by`,
  `scroll_x()`/`scroll_y()` → `lv_obj_scroll_*`/`lv_obj_get_scroll_*`.
- **`ScrollbarMode`** — frozen mirror of `lv_scrollbar_mode_t`
  (`OFF`/`ON`/`ACTIVE`/`AUTO`), via `lv_obj_set_scrollbar_mode`.
- **`ScrollSnap`** — frozen mirror of `lv_scroll_snap_t`, via
  `lv_obj_set_scroll_snap_x`/`_y`.
- **`ScrollDir`** — frozen mirror of `lv_dir_t`, via
  `lv_obj_set_scroll_dir`.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Scroll physics (throw/momentum) | `lvgl` (not re-implemented) |
| Scrollbar/snap/dir enums | this chapter — **Standards Action** (must match `lvgl`) |
| Scroll-event semantics | rlvgl `LPAR-05`; surfaced via LPAR-04 event seam |

## §5 Frozen decisions

1. Scroll state lives in the `lv_obj`; lvglpp adds no scroll state of its
   own and does not re-implement momentum.
2. `ScrollbarMode`/`ScrollSnap`/`ScrollDir` are frozen mirror enums.
3. Scroll begin/end/throw are surfaced as LPAR-04 events, not a bespoke
   callback type.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `REND-00` `ScrollView`** — folds in: any `Object` with scroll
  flags is scrollable; no separate container type.
- **rlvgl `INPUT-00` drag suppression** — handled by LVGL's scroll vs.
  click arbitration; the LPAR-04 gesture layer composes with it.

## §11 Non-goals

- Nested-scroll chaining tuning beyond LVGL defaults; widget-specific
  snap (Roller/Tileview) lives in LPAR-13.

## §12 Acceptance checklist

- [ ] `Object` scroll accessors wrap `lv_obj_scroll_*`.
- [ ] `ScrollbarMode`/`ScrollSnap`/`ScrollDir` mirror the `lv_*` enums.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-05.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-05-SCROLL-RUNTIME.md` (v0.2.4 @ `343f596`)
- `lvgl/src/core/lv_obj_scroll.h`
- `core/include/lvglpp/core/object.hpp` (WRAP-00)

## §14 Unblocks

- LPAR-13 (Dropdown/Roller/Tileview/Menu), LPAR-14 (Table).

## §15 Change log

- **2026-06-15** — LPAR-05 drafted: wrap `lv_obj_scroll_*` + scrollbar/
  snap/dir enums; folds rlvgl REND-00 ScrollView. **Not ratified** —
  batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
