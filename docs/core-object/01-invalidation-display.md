# 01 — Invalidation & display

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-03**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. [`../lpar/README.md`](../lpar/README.md)
is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Invalidation / refresh **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md` (@ `343f596`) |
| The **primitive** | `lvgl/src/core/lv_obj.h` (`lv_obj_invalidate`), `lvgl/src/display/lv_display.h` |
| C++ idiom / ownership | this chapter + `docs/wrap/00-concepts.md` |

## §1 Purpose

Wrap LVGL's dirty-area + display-refresh model: `Object::invalidate()`,
and a `Display` RAII wrapper over `lv_display_t` (flush callback, draw
buffers, render mode) that the platform backends (LPAR/WRAP-0N) provide.

## §2 Problem statement

rlvgl re-implements a dirty-rect planner and present plan
(`core::invalidation`, `platform::present`). lvglpp does not need to:
LVGL owns the dirty-rect machinery. lvglpp wraps the entry points —
`lv_obj_invalidate` and the `lv_display_*` flush contract — and lets
`lv_timer_handler` (LPAR-06) drive refresh. The hand-rolled `Renderer`
full-frame model (CORE-04) is superseded by LVGL's partial-refresh
display.

## §3 Canonical glossary

- **`Object::invalidate()`** — wraps `lv_obj_invalidate`; marks the
  object's area dirty. As defined in rlvgl `LPAR-03` (mirrored).
- **`Display`** — Owned by this chapter; RAII over `lv_display_t`
  (`lv_display_create` / `lv_display_delete`). Owns the display; the
  flush callback and draw buffers are `dma`/`external` per CLAUDE.md
  ownership tags. The active screen is owned by the `Display`, exposed as
  an `ObjectView`.
- **Render mode** — `LV_DISPLAY_RENDER_MODE_PARTIAL` / `_FULL` /
  `_DIRECT`; a frozen mirror enum.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Dirty-rect planning | `lvgl` (not re-implemented) |
| Flush contract (`lv_display_set_flush_cb`, `lv_display_flush_ready`) | `lvgl`; platform backends implement it (WRAP-0N) |
| Render-mode enum | this chapter — **Standards Action** (must match `lv_display_render_mode_t`) |

## §5 Frozen decisions

1. lvglpp does NOT re-implement dirty-rect merging or present planning;
   it wraps `lv_obj_invalidate` and the `lv_display_*` flush contract.
2. `Display` owns its `lv_display_t`; draw buffers are caller-provided
   and tagged `dma`/`external` (never CPU-mutated during an active
   flush, CLAUDE.md ownership rule 10).
3. Refresh is driven by `lv_timer_handler` (LPAR-06), not by a bespoke
   loop.

## §10 Reconciliation vs. adjacent primitives

- **`Renderer` (CORE-04/04a)** — superseded by LVGL's draw pipeline +
  `Display` flush. The platform `SdlRenderer`/`DiscoRenderer`/fbdev move
  to `lv_display` flush callbacks under `LVGLPP-WRAP-0N`.
- **`Screen`/`Display`** — `Screen::load()` (WRAP-00) targets the active
  screen of the default `Display`.

## §11 Non-goals

- Platform flush implementations (WRAP-0N). This chapter freezes the
  `Display` wrapper contract; the SDL/fbdev/disco flush bodies land with
  the platform migration.

## §12 Acceptance checklist

- [ ] `Object::invalidate()` wraps `lv_obj_invalidate`.
- [ ] `Display` RAII over `lv_display_t` with flush-cb + draw-buffer +
      render-mode setters; draw buffers tagged `dma`/`external`.
- [ ] Render-mode enum mirrors `lv_display_render_mode_t`.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-03.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md` (v0.2.4 @ `343f596`)
- `lvgl/src/display/lv_display.h`, `lvgl/src/core/lv_obj.h`
- `core/include/lvglpp/core/object.hpp` (WRAP-00)

## §14 Unblocks

- LPAR-05 (scroll needs invalidation), LPAR-08 (draw to display),
  `LVGLPP-WRAP-0N` (platform flush implementations).

## §15 Change log

- **2026-06-15** — LPAR-03 drafted: wrap `lv_obj_invalidate` + a
  `Display` RAII over `lv_display_t`; dirty-rect planning delegated to
  LVGL. **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
