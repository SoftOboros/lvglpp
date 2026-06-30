<!--
STATUS.md — Co-located status block for lvglpp::ui.
Canonical shape: see CLAUDE.md § "Doc Co-Location Policy".
-->

# lvglpp::ui — STATUS

Tracks `rlvgl/ui` @ `v0.2.5` (commit `f999f75`). Last reconciled:
2026-06-29.

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

- Compiled STATIC CMake target `lvglpp::ui` (moved off the INTERFACE
  stub in DEMO-02; first compiled unit `src/draw_helpers.cpp`).
- `draw_panel_header` + `panel_close_hit` (DEMO-02) in
  `include/lvglpp/ui/draw_helpers.hpp` / `src/draw_helpers.cpp`, with
  the `kPanelPadding` / `kCloseSize` constants mirroring rlvgl.
- Module umbrella `ui.hpp` re-exports `draw_helpers.hpp`.
- Per-module test `tests/draw_helpers_test.cpp`
  (`lvglpp_ui_draw_helpers`).
- README / OPTIONS / STATUS docs.

- `EventWindow` + `EventWindowBuilder` (DEMO-03) in
  `include/lvglpp/ui/event_window.hpp` / `src/event_window.cpp`;
  app-relevant surface, board-render telemetry deferred. Test
  `lvglpp_ui_event_window`.

Stubbed:

- Theming surface, layout helpers (UI-03..UI-04) — not yet ported.

## Blockers

- **CORE-02 / CORE-04 / CORE-05.** Every UI phase depends on a
  load-bearing `lvglpp::core` chapter that has not landed. Owner:
  `lvglpp::core` implementer.
- **Concepts doc.** UI surface shape requires a concepts doc under
  `docs/` once UI-01 is in flight.

## Definitions

- **`draw_panel_header`** — As defined in
  `rlvgl/ui/src/draw_helpers.rs:41`; mirrored here as
  `ui/include/lvglpp/ui/draw_helpers.hpp` (DEMO-02). `&str` →
  `std::string_view`; no behavioral delta.
- **`panel_close_hit`** — As defined in
  `rlvgl/ui/src/draw_helpers.rs:98`; mirrored here as
  `ui/include/lvglpp/ui/draw_helpers.hpp` (DEMO-02).
- **`kPanelPadding` / `kCloseSize`** — As defined in
  `rlvgl/ui/src/draw_helpers.rs:27,29` (`PANEL_PADDING` = 20,
  `CLOSE_SIZE` = 48); used without modification.
- **`EventWindow` / `EventWindowBuilder`** — As defined in
  `rlvgl/ui/src/event_window.rs:34,279`; mirrored here as
  `ui/include/lvglpp/ui/event_window.hpp` (DEMO-03). App-relevant
  surface only; board-render telemetry (`dma2d_mode`, `frozen`,
  `diag_state`, `draw_seq`, …) deferred to a PLAT-02e-era chapter.
  `String` → `std::string`; `&'static BitmapFont` →
  `const core::BitmapFont&` (borrows). Authoritative chapter:
  `docs/disco-demo/03-event-window.md`.
- **theme** — Owned by chapter UI-03; does not exist in repo yet. Will
  mirror the rlvgl surface with `external` ownership tags on every
  `lv_obj_t*` it holds.

## Change log

- 2026-04-27 — Initial scaffold. INTERFACE target only; no UI code.
- 2026-06-07 — DEMO-02: ported `draw_panel_header` + `panel_close_hit`
  from `rlvgl/ui/src/draw_helpers.rs`. `ui/` moved from INTERFACE stub
  to a compiled STATIC library; top-level CMake posture link switched
  INTERFACE → PUBLIC accordingly.
- 2026-06-07 — DEMO-03: ported `EventWindow` + `EventWindowBuilder`
  from `rlvgl/ui/src/event_window.rs` (second compiled unit
  `src/event_window.cpp`). App-relevant surface (builder, `push_event`,
  visibility, Tick-aging expiry, `clear_region`); board DMA2D/telemetry
  hooks deferred. Bg via `core::fill_rounded_rect`, border via
  `core::detail::draw_border_straight` (rounded corners deferred to
  CORE-04b). Test `lvglpp_ui_event_window`; embedded-posture clean.
- 2026-06-29 — Status reconciled to the `rlvgl` `v0.2.5` submodule pin
  (`f999f75`) and the lvglpp LVGL-backed parity baseline at
  `docs/lvgl-parity/01-baseline.md`. Existing UI helpers remain
  app/compatibility surfaces until LPAR-CPP layout/style/widget wrapper
  phases consume them.
