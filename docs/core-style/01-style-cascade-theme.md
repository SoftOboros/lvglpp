# 01 — Style cascade & theme

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-07**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. The CORE-05 chapter
[`00-appearance.md`](./00-appearance.md) (value-type `Style`)
and [`../lpar/README.md`](../lpar/README.md) are referenced; this chapter
adds the LVGL part/state cascade.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Style cascade / part / state / theme **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-07-STYLE-THEME.md` (@ `343f596`) |
| The **primitive** | `lvgl/src/core/lv_obj_style.h`, `lvgl/src/misc/lv_style.h`, `lvgl/src/themes/lv_theme.h` |
| Existing value-type `Style` | `docs/core-style/00-appearance.md` (CORE-05) |

## §1 Purpose

Wrap LVGL's style cascade: reusable `lv_style_t` objects attached to
objects by part+state selector (`lv_obj_add_style`), per-object local
property overrides (`lv_obj_set_style_*` / `lv_obj_set_local_style_prop`),
and themes (`lv_theme_*`).

## §2 Problem statement

rlvgl re-implements a selector/part/state cascade with inheritance and
transitions (`core::style_cascade`, `core::theme`). LVGL ships the
cascade. lvglpp wraps it. The CORE-05 value-type `Style`/`Theme` (a flat
descriptor) is reconciled against `lv_style_t` + part/state selectors.

## §3 Canonical glossary

- **`Style`** — RAII over `lv_style_t` (`lv_style_init`/`lv_style_reset`);
  property setters wrap `lv_style_set_*`. Owns the `lv_style_t` storage,
  which MUST outlive every object it is added to (LVGL stores a pointer,
  not a copy) — a load-bearing ownership rule.
- **`Selector`** — a `(Part, State)` pair packed into
  `lv_style_selector_t`; `Part` mirrors `lv_part_t`, `State` mirrors
  `lv_state_t`.
- **`Object::add_style(Style&, Selector)`** — wraps `lv_obj_add_style`;
  borrows the `Style` (does not own it).
- **`Object::set_local_*`** — per-object overrides via
  `lv_obj_set_style_*` / `lv_obj_set_local_style_prop`.
- **`Theme`** — wraps `lv_theme_t` (`lv_theme_default_init`,
  `lv_theme_apply`, `lv_theme_get_from_obj`, `lv_theme_set_parent`).

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Style property set / values | `lvgl` `lv_style_prop_t` + rlvgl `LPAR-07` semantics |
| Part / state values | `lvgl` `lv_part_t` / `lv_state_t` — **Standards Action** to mirror |
| `Style` storage lifetime | this chapter (the outlives-objects rule) |

## §5 Frozen decisions

1. **`Style` lifetime is load-bearing.** `lv_obj_add_style` stores a
   pointer; the `Style`'s `lv_style_t` MUST outlive every object using it.
   The wrapper marks added `Style` as `borrows`-into-LVGL; destroying a
   `Style` still referenced by a live object is UB and forbidden.
2. `Part`/`State` mirror `lv_part_t`/`lv_state_t` (**Standards Action**).
3. CORE-05 value-type `Style` is reconciled: either re-expressed as a
   thin builder over `lv_style_t`, or deprecated — decided in execution,
   recorded as a CORE-05 DELTA.
4. Transitions wrap `lv_style_transition_dsc_t` and compose with LPAR-06.

## §10 Reconciliation vs. adjacent primitives

- **CORE-05 `Style`/`StyleBuilder`/`Theme`/`LightTheme`/`DarkTheme`** —
  superseded/realigned onto `lv_style_t`/`lv_theme_t`. The
  `Light`/`Dark` themes map to `lv_theme_default_init(dark=…)`.
- **CORE-04a `draw_widget_bg`** — superseded; background/border come from
  style properties resolved by LVGL's draw pipeline (LPAR-08).

## §11 Non-goals

- The draw pipeline that consumes styles (LPAR-08); widget default styles
  (per-widget phases).

## §12 Acceptance checklist

- [ ] `Style` RAII over `lv_style_t` with the outlives-objects rule
      documented (`borrows`-into-LVGL tag).
- [ ] `Object::add_style(Style&, Selector)` + local-style setters.
- [ ] `Part`/`State` mirror enums; `Theme` over `lv_theme_*`.
- [ ] CORE-05 reconciliation recorded as a DELTA.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-07.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-07-STYLE-THEME.md` (v0.2.4 @ `343f596`)
- `lvgl/src/core/lv_obj_style.h`, `lvgl/src/misc/lv_style.h`, `lvgl/src/themes/lv_theme.h`
- `docs/core-style/00-appearance.md` (CORE-05)

## §14 Unblocks

- LPAR-08 (style-driven draw), every widget phase (part/state styling).

## §15 Change log

- **2026-06-15** — LPAR-07 drafted: wrap `lv_style`/`lv_obj_style`/
  `lv_theme`; freeze the `Style`-outlives-objects ownership rule;
  reconcile CORE-05. **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
