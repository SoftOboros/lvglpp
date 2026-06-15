# 00 — Timers & object animations

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-06**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. [`../lpar/README.md`](../lpar/README.md)
is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Timer / object-animation **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md` + `ANIM-00` (@ `343f596`) |
| The **primitive** | `lvgl/src/misc/lv_timer.h`, `lvgl/src/misc/lv_anim.h` |
| Easing math (existing) | `docs/core-style/00-appearance.md` (CORE-05 `Easing`) |

## §1 Purpose

Wrap LVGL timers (`lv_timer_*`) and animations (`lv_anim_*`) as RAII C++,
and drive the LVGL refresh/animation pipeline via `lv_timer_handler`.

## §2 Problem statement

rlvgl re-implements deterministic tick-driven tweens (`core::anim`,
ANIM-00) and a timer registry (`core::timer`). LVGL ships both, tick-
driven via `lv_tick`/`lv_timer_handler`. lvglpp wraps them. The hand-
rolled CORE-05 `Easing`/`LoopMode` value types reconcile against
`lv_anim`'s path callbacks.

## §3 Canonical glossary

- **`Timer`** — RAII over `lv_timer_t` (`lv_timer_create`/`lv_timer_delete`);
  `set_period`/`pause`/`resume`. The callback is owned by the `Timer`.
- **`Animation`** — value wrapper over `lv_anim_t`
  (`lv_anim_init`/`lv_anim_start`); `set_var`/`set_values`/`set_time`/
  `set_exec_cb`/`set_path_cb`. LVGL copies the descriptor on `start`, so
  ownership of the running animation is LVGL's (keyed by var+exec).
- **`Easing` (CORE-05)** — maps to `lv_anim`'s `path_cb` (e.g.
  `lv_anim_path_ease_in_out`).

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Tick source / handler | `lvgl` (`lv_tick`, `lv_timer_handler`) |
| Easing curves | CORE-05 `Easing` ↔ `lv_anim` path callbacks |
| Animation determinism | rlvgl `ANIM-00` (tick-driven, no wall clock) — preserved by driving `lv_tick_inc` from a deterministic source in tests |

## §5 Frozen decisions

1. `Timer` is RAII over `lv_timer_t`; the C++ callback is owned by the
   `Timer` and torn down with it.
2. `Animation` wraps `lv_anim_t`; because LVGL copies the descriptor at
   `lv_anim_start`, the running animation is LVGL-owned and identified by
   `(var, exec_cb)` — documented as an ownership DELTA.
3. Determinism (ANIM-00) is preserved by feeding `lv_tick` from a
   controlled source in tests/fixtures, not by re-implementing tweens.
4. CORE-05 `Easing` is retained as the curve vocabulary, mapped to
   `lv_anim` path callbacks.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `ANIM-00` `Tween`/`Animations`** — folds in as `Animation`
  over `lv_anim`.
- **CORE-05 `Easing`/`LoopMode`** — retained; bound to `lv_anim` paths /
  `lv_anim_set_repeat_*`.

## §11 Non-goals

- Transition styles (LPAR-07 `lv_style_transition`); widget-specific
  spinners (LPAR-11).

## §12 Acceptance checklist

- [ ] `Timer` (RAII `lv_timer_t`) with period/pause/resume and an
      owned callback.
- [ ] `Animation` over `lv_anim_t` with the named setters; ownership
      DELTA documented.
- [ ] `Easing` (CORE-05) mapped to `lv_anim` path callbacks.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-06.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md`, `ANIM-00-CONCEPTS.md` (v0.2.4 @ `343f596`)
- `lvgl/src/misc/lv_timer.h`, `lv_anim.h`
- `docs/core-style/00-appearance.md` (CORE-05 `Easing`)

## §14 Unblocks

- LPAR-03 refresh driving, LPAR-11 Spinner, LPAR-07 transitions.

## §15 Change log

- **2026-06-15** — LPAR-06 drafted: wrap `lv_timer_*` + `lv_anim_*`;
  fold rlvgl ANIM-00; retain CORE-05 Easing as the curve vocabulary.
  **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
