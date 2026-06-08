<!-- 05-composite-widgets.md — DEMO-05 concepts doc (normative). -->

# DEMO-05 — Composite widgets + app module + icon assets

Status: **ratified** (2026-06-07). Contract for the four disco-demo
composites is frozen in DEMO-00 §6; this chapter restates it, stands up
the app module, and pins the asset pipeline. RFC 2119 keywords per
DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical: the rlvgl disco-demo app crate —
`rlvgl/examples/apps/disco-demo/src/{icon_strip,wing,dashboard_panel,
hotspot,assets}.rs` (rlvgl `v0.2.0`). Icon blobs:
`rlvgl/examples/stm32h747i-disco/assets/icons/*.rle`. Mirrored as a new
**lvglpp app module** at `examples/apps/disco-demo/`, paralleling the
rlvgl crate `rlvgl-app-disco-demo`.

## §1 Purpose

Provide the four app-specific composite `core::Widget`s the demo UI is
built from, the module that houses them, and the runtime icon-asset
pipeline. Wave-B; depends on DEMO-01/02/04 (landed); unblocks DEMO-06.

## §2 App module (FROZEN decisions)

- **Location:** `examples/apps/disco-demo/` (mirrors rlvgl). Layout:
  `include/lvglpp/app/disco_demo/`, `src/`, `tests/`, plus the doc trio
  (`README.md`, `OPTIONS.md`, `STATUS.md`) per CLAUDE.md § "Doc
  Co-Location Policy".
- **Target:** static library `lvglpp_app_disco_demo`
  (alias `lvglpp::app_disco_demo`), links `lvglpp::core`, `lvglpp::ui`,
  `lvglpp::widgets`. Namespace `lvglpp::app::disco_demo`.
- **Build gating:** added under `examples/` so it builds whenever
  `LVGLPP_BUILD_EXAMPLES` is ON (default), but it is **platform-
  independent** — it MUST NOT be gated on `LVGLPP_PLATFORM_HOST_SDL` or
  `LVGLPP_PLATFORM_DISCO`. Its tests are gated on `LVGLPP_BUILD_TESTS`
  only, so they run on a plain host configure. (If a stricter CI runs
  `-DLVGLPP_BUILD_EXAMPLES=OFF`, log that the app tests are skipped.)
- **Posture:** compiles under embedded-ON and embedded-OFF.

## §3 Icon-asset pipeline (FROZEN)

Mirrors the `include_bytes!` of `assets.rs:1` with the font-blob pattern
already used in `core/CMakeLists.txt` (`font_6x10.bin` → `.inc`):

- At CMake **configure** time, read each required `*.rle` from
  `rlvgl/examples/stm32h747i-disco/assets/icons/` and emit a byte-array
  `.inc` into the build dir.
- `assets.hpp` exposes each icon as
  `std::span<const std::uint8_t>` (e.g. `ICON_SETTINGS`, `ICON_FILE`,
  `ICON_INFO`, and the 48px set `ICON_AUDIO_48` … `ICON_PLAY_48` named in
  `assets.rs:11-40`). Consumers decode them with `core::rle` (DEMO-04)
  and blit via `core::Renderer::draw_pixels`.
- This stays **consume-only**: lvglpp reads rlvgl-produced `.rle` assets;
  it does not generate them (CLAUDE.md § "`creator-cpp` is deferred").
- If the `rlvgl/` submodule is absent (e.g. a CI without it), the
  generator MUST fail loudly OR fall back to a checked-in copy — the
  implementer picks one and records it; silent empty icons are
  forbidden.

Layout constants (FROZEN, mirror `assets.rs` + `lib.rs:225`) live in
`assets.hpp` — the full set is enumerated in DEMO-00 §6.

## §4 Composite-widget contracts (restate DEMO-00 §6)

Each is a `core::Widget` (`bounds`/`draw`/`handle_event`, optional
`clear_region`). Tap callbacks are `std::function` (the controller binds
them in DEMO-06; here they are plain stored functions). Ownership per
DEMO-00 §5: each widget is single-owned by the tree; it owns its slot
vector / strings by value and **borrows** the `BitmapFont`.

- **`IconStrip`** (`icon_strip.rs`, `SLOT_COUNT = 3`):
  `IconSlot{ std::span<const std::uint8_t> rle; bool enabled;
  std::function<void(std::size_t)> on_tap; }`. Ctor
  `IconStrip(x, icon_size, margin_top, gap)`; `set_slot`,
  `set_focused_slot(optional<size_t>)`, `focused_slot()`. `draw` decodes
  each enabled slot's icon (DEMO-04) and blits via `draw_pixels`, drawing
  a `FOCUS_HIGHLIGHT_COLOR` border on the focused slot. `handle_event`:
  `PressRelease` within a slot → `on_tap(index)`, returns true.
- **`Wing`** (`wing.rs`, `MAX_SLOTS = 6`, `CLEAR_FRAMES = 3`):
  `WingSlot` like `IconSlot`. Ctor `Wing(std::span<const
  std::pair<std::span<const std::uint8_t>, bool>>)` (icon, enabled
  pairs). `set_focused_slot`, `focused_slot`, `is_visible`,
  `toggle_visible`, `close`. `bounds()` collapses to zero when hidden;
  `draw` (only if visible) fills the panel bg + border + icons;
  `clear_region()` returns the panel rect for `CLEAR_FRAMES` after close.
  `handle_event`: `PressRelease` in a slot → `on_tap(index)`. FROZEN
  geometry/colors per `wing.rs` (`RADIUS = 18`, `BG_COLOR`,
  `BORDER_COLOR`, …).
- **`DashboardPanel`** (`dashboard_panel.rs`): fields `bounds, title,
  caption, lines, accent, visible`, borrows `BitmapFont`. Ctor
  `DashboardPanel(bounds, title, caption)`; `set_title/caption/lines/
  accent`, `show/hide`, `is_visible`. `draw` (if visible): rounded bg +
  **`ui::draw_panel_header`** (DEMO-02) + word-wrapped caption/lines.
  `handle_event`: `ui::panel_close_hit` (DEMO-02) → consume (hide via the
  controller). `bounds()` zero when hidden. FROZEN palette + `PADDING =
  20`.
- **`ActionHotspot`** (`hotspot.rs`): `bounds`,
  `std::function<void()> on_tap`, `std::function<bool()> is_visible`.
  Ctor `ActionHotspot(bounds, on_tap)`; `with_visibility(pred)` builder.
  `draw` is a no-op. `bounds()` zero when `is_visible()` false.
  `handle_event`: `PressRelease` → `on_tap()`.

## §5 Files

- `examples/apps/disco-demo/CMakeLists.txt` (new lib target + asset
  `.inc` generation) and registration from `examples/CMakeLists.txt`.
- `include/lvglpp/app/disco_demo/{assets,icon_strip,wing,dashboard_panel,
  hotspot}.hpp` (new)
- `src/{icon_strip,wing,dashboard_panel,hotspot}.cpp` (new)
- `tests/{icon_strip,wing,dashboard_panel,hotspot}_test.cpp` (new) +
  `tests/CMakeLists.txt`; tests `lvglpp_app_disco_demo_{icon_strip,wing,
  dashboard_panel,hotspot}`.
- Doc trio `README.md` / `OPTIONS.md` / `STATUS.md` for the module.

Cite block per file: `// PARITY: rlvgl/examples/apps/disco-demo/src/<f>.rs
(v0.2.0 @ 79f730d).` / `// LVGL: N/A (app composite).` / `// DELTA: …`.

## §6 Ownership

Per DEMO-00 §5: every composite is a single-owned `core::Widget`; slot
vectors and strings are owned by value; `BitmapFont` and decoded-icon
inputs are borrowed; `std::function` callbacks are owned by the widget
but their captures (bound in DEMO-06) observe the controller. No widget
holds a `WidgetNode*` or an LVGL handle. Decoded-icon pixel buffers are
either decoded into a widget-owned `std::vector<core::Color>` once (cache)
or per-draw into a stack/scratch buffer — the implementer picks; either
way the buffer is owned, never aliased with DMA (host has no DMA).

## §7 Reconciliation / non-goals (informative)

- `DashboardPanel` and `Wing` rendering reuse DEMO-02 helpers and the
  DEMO-04 decoder; they introduce no new core/ui surface.
- The controller wiring (binding callbacks to `ControllerState`, the
  focus FSM, hotspot visibility predicates) is **DEMO-06**, not here.
- Effects (StarCrawl/AudioScope live content) are not rendered by these
  widgets (DEMO-00 §11).

## §8 Acceptance

- [ ] App module `lvglpp::app_disco_demo` builds on a plain host
      configure (no SDL) and under embedded posture.
- [ ] Asset `.inc` generation produces non-empty icon spans from the
      rlvgl `.rle` files (and fails loudly / falls back if absent — which
      one is recorded).
- [ ] Per-widget tests mirror the rlvgl unit behavior: icon-strip focus
      + tap-index; wing visibility toggle + collapse-to-zero bounds +
      clear_region; dashboard show/hide + close-hit consume + wrap;
      hotspot visibility-gated bounds + tap.
- [ ] `SLOT_COUNT=3`, `MAX_SLOTS=6`, `CLEAR_FRAMES=3`, and the frozen
      layout constants equal rlvgl.
- [ ] No `WidgetNode*` stored; no LVGL handle held (DEMO-00 §5 audit).
- [ ] Pre-Publish 0–3 green.

## §9 Change log

- _drafted_ — DEMO-05 restated from DEMO-00 §6/§14; app module layout,
  target, and consume-only icon-asset pipeline pinned.
- **2026-06-07 — ratified.** Owner directed Wave-B; app module
  (`examples/apps/disco-demo/`, `lvglpp::app_disco_demo`), the four
  composite contracts, and the consume-only CMake-time icon-asset
  pipeline frozen. Execution may proceed.
