# 00 — Object substrate

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-02**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. The initiative umbrella
[`../lpar/README.md`](../lpar/README.md) is informative.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| Object flags/state/hit-test/lifecycle **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md` (@ `343f596`) | Canonical behavior for the rlvgl/lvglpp pair. |
| The **primitive** (flags, states, tree ops) | `lvgl/src/core/lv_obj.h`, `lv_obj_tree.h` (9.6 @ `ee436e8`) | What `lv_obj_*` actually does. |
| RAII owner + C++ idiom | `docs/wrap/00-concepts.md` (`Object`/`Screen`) + this chapter | Normative for lvglpp. |

## §1 Purpose

Expose LVGL's object flags, state machine, hit-testing, and parent/child
lifecycle as methods on the `Object` wrapper (WRAP-00), so every later
widget phase has a stable, ownership-safe object vocabulary.

## §2 Problem statement

`Object` (WRAP-00) owns an `lv_obj_t*` but exposes only create/delete/
view. Widgets need flags (`hidden`/`clickable`/`scrollable`), the
interaction state machine (`pressed`/`focused`/`disabled`), hit-testing,
and tree queries (parent, child count, index). rlvgl re-implements these
on `ObjectNode`; lvglpp wraps the `lv_obj_*` equivalents. The hand-rolled
`lvglpp::core::WidgetNode` (CORE-03a) is the surface this supersedes.

## §3 Canonical glossary

- **`ObjectFlag`** — Owned by this chapter; mirrors rlvgl `ObjectFlags`
  (`LPAR-02`). A C++ `enum class : uint32_t` over the `LV_OBJ_FLAG_*`
  bits (`lvgl/src/core/lv_obj.h`). Applied via `Object::add_flag` /
  `remove_flag` / `has_flag` → `lv_obj_add_flag` / `lv_obj_remove_flag` /
  `lv_obj_has_flag`.
- **`ObjectState`** — Owned by this chapter; mirrors rlvgl `ObjectState`.
  A C++ `enum class : uint16_t` over `LV_STATE_*`. Applied via
  `Object::add_state` / `remove_state` / `has_state` / `state()` →
  `lv_obj_add_state` / `lv_obj_remove_state` / `lv_obj_has_state` /
  `lv_obj_get_state`.
- **Hit-test** — As defined in `LPAR-02`; wraps `lv_obj_hit_test`.
- **Tree queries** — `parent()`, `child_count()`, `child(i)` →
  `lv_obj_get_parent`, `lv_obj_get_child_count`, `lv_obj_get_child`,
  returning `ObjectView` (non-owning) for tree neighbors.

## §4 Source-of-truth map

| Concept | Owner | Mirror site |
| --- | --- | --- |
| Flag/state bit values | `lvgl` `LV_OBJ_FLAG_*` / `LV_STATE_*` | `ObjectFlag`/`ObjectState` (mapped 1:1) — **Standards Action** to add a value that must match rlvgl |
| Flag/state behavior | rlvgl `LPAR-02` | `Object` methods |
| Tree neighbor ownership | `docs/wrap/00-concepts.md` | neighbors are `ObjectView` (observes), never owning |

## §5 Frozen decisions

1. Flags/states are wrapped as `enum class` over the LVGL bit constants;
   the C++ method surface is `add_*`/`remove_*`/`has_*`/`state()`. The
   underlying bit values are LVGL's — **Standards Action** to diverge.
2. Tree-neighbor accessors return `ObjectView` (non-owning). Walking the
   tree NEVER transfers ownership; only `Object`/`Screen` own.
3. Hit-test wraps `lv_obj_hit_test`; lvglpp does not re-implement
   point-in-rect logic.
4. No new lifecycle: creation/deletion remain WRAP-00's `make_*`/dtor.
   This chapter adds query/mutation, not ownership.

## §10 Reconciliation vs. adjacent primitives

- **`WidgetNode` (CORE-03a)** — superseded. Its `dispatch_event`/`draw`/
  `find_by_tag` move to the `lv_obj` tree (event dispatch → LPAR-04; draw
  → LVGL; tag → LPAR-04/playit bridge). Retired under `LVGLPP-WRAP`.
- **`Widget`/`Rect` (CORE-03)** — `bounds()` maps to `lv_obj_get_coords`;
  the abstract `Widget` base is superseded by `Object` subclasses.

## §11 Non-goals

- Event dispatch / focus (LPAR-04), scroll flags behavior (LPAR-05),
  style state resolution (LPAR-07) — only the flag/state *setters* live
  here.

## §12 Acceptance checklist

- [ ] `Object` gains `add_flag`/`remove_flag`/`has_flag` over
      `ObjectFlag`, and `add_state`/`remove_state`/`has_state`/`state`
      over `ObjectState`, wrapping the named `lv_obj_*` calls.
- [ ] Tree accessors (`parent`/`child_count`/`child`) return `ObjectView`.
- [ ] Cite blocks present; ownership tags on any raw pointer.
- [ ] Builds + tests under default and `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] `core/STATUS.md` records LPAR-02.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md` (v0.2.4 @ `343f596`)
- `lvgl/src/core/lv_obj.h`, `lv_obj_tree.h` (9.6 @ `ee436e8`)
- `core/include/lvglpp/core/object.hpp` (WRAP-00)
- `core/include/lvglpp/core/widget_node.hpp` (CORE-03a, superseded)

## §14 Unblocks

- LPAR-04 (events on the object tree), LPAR-05 (scroll flags), LPAR-07
  (style by part/state), and every widget phase.

## §15 Change log

- **2026-06-15** — LPAR-02 drafted as a wrapper over `lv_obj_*` flags/
  state/hit-test on the WRAP-00 `Object`. **Not ratified** — batch
  pending owner go-ahead with the rest of Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
