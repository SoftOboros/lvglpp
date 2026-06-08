# app_disco_demo — STATUS

Tracks rlvgl/examples/apps/disco-demo @ v0.2.0 (commit 79f730d). Last
reconciled: 2026-06-07.

## Roadmap intent

C++ port of the rlvgl disco-demo app crate. Phase plan:

- **DEMO-05 (this module)** — stand up the app module and the four composite
  widgets + the consume-only icon-asset pipeline. Depends on DEMO-01
  (`Container`), DEMO-02 (`ui::draw_panel_header`/`panel_close_hit`), and
  DEMO-04 (`core::rle`), all landed.
- **DEMO-06** — `DiscoController` + `ControllerState`, the host-SDL target,
  and the navigation/focus parity tests. Depends on this module, DEMO-03
  (`EventWindow`), and DEMO-0S (`Screen`).

## As-built

### Implemented
- `assets.hpp` — frozen layout constants (DEMO-00 §6) + 10 icon accessors
  (`icon_settings`/`icon_file`/`icon_info` + the 48px set). Byte arrays are
  generated at CMake-configure time from the rlvgl `.rle` files.
- `IconStrip` / `IconSlot` (`SLOT_COUNT = 3`): focus get/set, per-draw icon
  decode + blit, focus-highlight border, PressRelease tap-index dispatch.
- `Wing` / `WingSlot` (`MAX_SLOTS = 6`, `CLEAR_FRAMES = 3`): visibility
  toggle/close, collapse-to-zero bounds when hidden, `clear_region` for 3
  frames after close, slot-tap dispatch (closes the wing), rounded bg +
  border + icons on draw.
- `DashboardPanel`: title/caption/lines/accent, show/hide, rounded bg +
  `ui::draw_panel_header` + word-wrapped body, close-hit consume.
- `ActionHotspot`: visibility-gated zero bounds, PressRelease activation.
- Four per-widget tests (`lvglpp_app_disco_demo_{icon_strip,wing,
  dashboard_panel,hotspot}`).

### Stubbed
- None. Controller wiring (tap-callback binding, focus FSM, hotspot
  visibility predicates) is DEMO-06, not this module.

## Blockers

- None.

## Definitions

- **`IconStrip` / `IconSlot`** — As defined in
  `rlvgl/examples/apps/disco-demo/src/icon_strip.rs`; mirrored here as
  `include/lvglpp/app/disco_demo/icon_strip.hpp`.
- **`Wing` / `WingSlot`** — As defined in `.../src/wing.rs`; mirrored here as
  `include/lvglpp/app/disco_demo/wing.hpp`; adapted: `bounds()` collapses to
  zero when hidden per the FROZEN DEMO-00 §6 contract (rlvgl `wing.rs:144`
  returns `self.bounds` unconditionally).
- **`DashboardPanel`** — As defined in `.../src/dashboard_panel.rs`; mirrored
  here as `include/lvglpp/app/disco_demo/dashboard_panel.hpp`; adapted:
  rounded border via the core radius==0 fallback; `set_lines` takes a
  `std::span` instead of a Rust `IntoIterator`.
- **`ActionHotspot`** — As defined in `.../src/hotspot.rs`; mirrored here as
  `include/lvglpp/app/disco_demo/hotspot.hpp`.
- **Layout constants / icon names** — As defined in `.../src/assets.rs`;
  mirrored here as `include/lvglpp/app/disco_demo/assets.hpp` (Standards
  Action; values frozen in DEMO-00 §6).
- **`core::rle`** — As defined in `core/include/lvglpp/core/rle.hpp`; used
  without modification.
- **`ui::draw_panel_header` / `ui::panel_close_hit`** — As defined in
  `ui/include/lvglpp/ui/draw_helpers.hpp`; used without modification.

## Change log

- **2026-06-07 — DEMO-05 initial port.** App module `lvglpp::app_disco_demo`
  stood up under `examples/apps/disco-demo/`; the four composite widgets and
  the consume-only CMake-time icon-asset pipeline implemented; four
  per-widget tests added. Per-draw scratch-buffer icon decode (no cache).
  Asset-absent handling: configure-time `FATAL_ERROR` (no silent fallback).
  DELTA: `Wing::bounds()` collapses to zero when hidden, tracking the
  ratified DEMO contract rather than rlvgl `wing.rs:144`.
