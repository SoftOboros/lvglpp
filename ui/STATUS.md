<!--
STATUS.md — Co-located status block for lvglpp::ui.
Canonical shape: see CLAUDE.md § "Doc Co-Location Policy".
-->

# lvglpp::ui — STATUS

Tracks `rlvgl/ui` @ `v0.2.0` (commit `79f730d`). Last reconciled:
2026-04-27.

## Roadmap intent

`lvglpp::ui` provides the layer between raw widgets and applications:
draw helpers, event-window scaffolding, theming surface, layout helpers,
and the small composition utilities `rlvgl-ui` ships.

Phase plan:

1. **UI-01:** Draw helpers — port of `rlvgl/ui/src/draw_helpers.rs`,
   gated on `core::Renderer` (CORE-04).
2. **UI-02:** Event window — port of `rlvgl/ui/src/event_window.rs`,
   gated on `core::Event` (CORE-02).
3. **UI-03:** Theming surface — port of `rlvgl/ui` theme bits, gated on
   `core::Theme` / `core::Style` (CORE-05).
4. **UI-04:** Layout helpers (flex / grid wrappers around
   `lv_layout_flex` / `lv_layout_grid`).

## As-built

Implemented:

- INTERFACE CMake target `lvglpp::ui`.
- Module umbrella `ui.hpp` (no headers yet — listed as commented
  `#include` lines).
- README / OPTIONS / STATUS docs.

## Blockers

- **CORE-02 / CORE-04 / CORE-05.** Every UI phase depends on a
  load-bearing `lvglpp::core` chapter that has not landed. Owner:
  `lvglpp::core` implementer.
- **Concepts doc.** UI surface shape requires a concepts doc under
  `docs/` once UI-01 is in flight.

## Definitions

- **Draw helper, event window, theme** — Owned by chapters UI-01 / UI-02
  / UI-03; do not exist in repo yet. Will mirror the corresponding rlvgl
  surfaces with `external` ownership tags on every `lv_obj_t*` they hold.

## Change log

- 2026-04-27 — Initial scaffold. INTERFACE target only; no UI code.
