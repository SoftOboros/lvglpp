# 01 — Event, focus & input runtime

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-04**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. The CORE-02 chapter
[`00-event-surface.md`](./00-event-surface.md) (the `Event` value type)
and [`../lpar/README.md`](../lpar/README.md) are referenced; this chapter
adds the runtime that routes events through the `lv_obj` tree.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Event routing / focus-group / input-device **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md` (@ `343f596`) |
| `Event` value type | `docs/core-event/00-event-surface.md` (CORE-02) — used without modification |
| The **primitive** | `lvgl/src/core/lv_obj_event.h`, `lvgl/src/core/lv_group.h`, `lvgl/src/indev/lv_indev.h` |

## §1 Purpose

Route input through the `lv_obj` tree: per-`Object` event callbacks
(`lv_obj_add_event_cb`), focus groups (`lv_group_*`), and input devices
(`lv_indev_*`). Bridge the lvglpp `Event` value type (CORE-02) to/from
`lv_event_t`/`lv_indev` at the driver seam.

## §2 Problem statement

rlvgl re-implements event propagation, focus groups, and input devices
(`core::object` dispatch, `core::focus`, `platform::gesture`). lvglpp
wraps the LVGL equivalents. The hand-rolled `WidgetNode::dispatch_event`
(CORE-03a) is superseded by `lv_obj` event callbacks; the playit
injection path (which currently feeds `Event` into the hand-rolled tree)
re-targets the `lv_obj` tree (the playit↔lv_obj bridge, `LVGLPP-WRAP-0N`).

## §3 Canonical glossary

- **`Object::on(code, handler)`** — Owned by this chapter; registers an
  `lv_obj` event callback (`lv_obj_add_event_cb`) with a C++ handler. The
  handler's lifetime is tied to the `Object` (CLAUDE.md callback rule);
  the back-pointer reuses WRAP-00's `user_data` convention.
- **`FocusGroup`** — RAII over `lv_group_t` (`lv_group_create`/_delete);
  `add(Object)`, `focus(Object)` → `lv_group_add_obj`/`lv_group_focus_obj`.
- **`InputDevice`** — RAII over `lv_indev_t` (`lv_indev_create`); type +
  read-cb + group binding (`lv_indev_set_type`/`set_read_cb`/`set_group`).
- **`Event`/`Key`/`TouchState`** — As defined in CORE-02; converted
  to/from `lv_event_t`/`lv_indev_data_t` at the seam.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| `Event`/`Key` enums | CORE-02 (`docs/core-event/00-event-surface.md`) — **Standards Action** |
| Event-code mapping (`lv_event_code_t` ↔ `Event`) | this chapter |
| Focus-group / indev behavior | rlvgl `LPAR-04`; primitive = `lvgl` |

## §5 Frozen decisions

1. Per-object event handlers wrap `lv_obj_add_event_cb`; the C++ handler
   and its captured state are owned by the `Object` and torn down with it
   (mechanically safe per CLAUDE.md rule 9).
2. The CORE-02 `Event`/`Key`/`TouchState`/`MAX_TOUCH_POINTS` value types
   are unchanged; this chapter only adds the `lv_event_t` ↔ `Event`
   conversion seam.
3. `FocusGroup`/`InputDevice` are RAII over `lv_group_t`/`lv_indev_t`.
4. playit input injection re-targets the `lv_obj` tree (bridge owned by
   `LVGLPP-WRAP-0N`); wire semantics stay byte-identical.

## §10 Reconciliation vs. adjacent primitives

- **`WidgetNode::dispatch_event` (CORE-03a)** — superseded by `lv_obj`
  event callbacks.
- **`playit` `GesturePipeline`/`Dispatcher`** — keeps its wire contract;
  its tree walk + injection re-target `lv_obj` (WRAP-0N).
- **rlvgl `INPUT-00` `DragRecognizer`** — folds in here as the gesture
  layer above `lv_indev` (mirrored where LVGL lacks the behavior).

## §11 Non-goals

- Scroll gesture semantics (LPAR-05); editable-text key routing
  (LPAR-14 Textarea, absorbing rlvgl WID-00).

## §12 Acceptance checklist

- [ ] `Object::on(...)` registers an `lv_obj` event cb with an
      Object-owned C++ handler.
- [ ] `FocusGroup` (RAII `lv_group_t`) and `InputDevice` (RAII
      `lv_indev_t`) with the named setters.
- [ ] `lv_event_t` ↔ CORE-02 `Event` conversion seam, round-trip tested.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-04.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md` (v0.2.4 @ `343f596`)
- `lvgl/src/core/lv_obj_event.h`, `lv_group.h`, `lvgl/src/indev/lv_indev.h`
- `docs/core-event/00-event-surface.md` (CORE-02)
- `playit/include/lvglpp/playit/{dispatcher,gesture,conversion}.hpp`

## §14 Unblocks

- LPAR-12/13/14 (interactive controls need focus + key routing);
  `LVGLPP-WRAP-0N` playit bridge.

## §15 Change log

- **2026-06-15** — LPAR-04 drafted: per-object event cbs, `FocusGroup`,
  `InputDevice`, and the `lv_event_t` ↔ CORE-02 `Event` seam; folds
  rlvgl INPUT-00. **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
