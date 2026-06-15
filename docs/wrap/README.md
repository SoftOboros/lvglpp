<!--
README.md — Initiative README for LVGLPP-WRAP (object-model unification).
-->

# wrap — initiative README

`LVGLPP-WRAP` is the **lvglpp-internal** initiative that unifies lvglpp
onto a single `lv_obj_t*`-backed object model: it stands up the RAII core
that wraps LVGL objects, and migrates the existing hand-rolled
`WID-01..06` widgets, the `playit` dispatcher, and the platform renderers
onto it. It is the execution arm of the pivot frozen in
[`../lpar/00-concepts.md`](../lpar/00-concepts.md) §5.2.

`LVGLPP-WRAP` does **not** mirror an rlvgl phase — it is the C++-side
plumbing that lets every LPAR/FONT widget wrapper share one tree. Commit
prefix: `LVGLPP-WRAP-NN[a-z]:` (CLAUDE.md § "Execution discipline").

This README is **informative**. The normative artifacts are the
`NN-*.md` chapters here.

## Chapters

| Chapter | Phase | Scope | Status |
| --- | --- | --- | --- |
| [00-concepts.md](./00-concepts.md) | LVGLPP-WRAP-00 | RAII `lv_obj` core (`Object`/`Screen`), ownership + delete-safety + user-data + name/tag conventions, `lv_conf.h` baseline. **Additive.** | **Ratified 2026-06-15** |
| `01-migrate-widgets.md` | LVGLPP-WRAP-01.. | Port `WID-01..06` onto `Object`; retire hand-rolled `Widget`/`WidgetNode`/`Renderer`/`draw_*`. | Owned by 00 §6; pending |
| `0N-playit-bridge.md` | LVGLPP-WRAP-0N | `playit` `Dispatcher` walks the `lv_obj` tree (tag → `lv_obj` name); byte-identical wire responses. | Owned by 00 §6; pending |
| `0N-platform-display.md` | LVGLPP-WRAP-0N | SDL / fbdev / disco become `lv_display_t` flush + `lv_indev_t` providers. | Owned by 00 §6; pending |

## Conformance target

A conforming `LVGLPP-WRAP` deployment MUST keep `ctest` and the shared
`playit` fixtures green at every sub-phase (widget-by-widget migration),
under both default and `LVGLPP_EMBEDDED_POSTURE=ON` builds. The
hand-rolled object layer is retired only after the last widget, the
playit bridge, and the platform display move land.
