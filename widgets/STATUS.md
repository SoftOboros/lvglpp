<!--
STATUS.md — Co-located status block for lvglpp::widgets.
Canonical shape: see CLAUDE.md § "Doc Co-Location Policy".
-->

# lvglpp::widgets — STATUS

Tracks `rlvgl/widgets` @ `v0.2.0` (commit `b178cbc`). Last reconciled:
2026-04-27.

## Roadmap intent

`lvglpp::widgets` ports the rlvgl built-in widget set onto the C++
surface, one widget at a time. Each widget gets its own header and
translation unit; the rlvgl widget file maps 1:1 to the lvglpp widget
file. Widgets land in dependency order — primitives first, composites
later.

Phase plan:

1. **WID-01:** `Label`. Smallest widget; smoke-tests the
   `ObjectView` borrowing pattern, the constructor / builder shape,
   and the `LV_USE_*` gating story. Lands once `core::Event` and
   `core::WidgetNode` exist (CORE-02 + CORE-03).
2. **WID-02:** `Button`. Adds the click event surface.
3. **WID-03:** `Checkbox`, `Switch`. Adds toggleable-state borrowing.
4. **WID-04:** `Slider`, `Bar`/`Progress`. Adds value-bound widgets.
5. **WID-05:** `Container`, `List`. Adds parent-child composition over
   the `external` ownership tag.
6. **WID-06:** `Image`. Adds the asset-loader seam — the same seam
   `rlvgl-creator` will eventually flow into (see top-level CLAUDE.md
   § "Doc Co-Location Policy" / creator-seam discussion).
7. **WID-07:** `Radio`.
8. **WID-08+:** `meters/`, `motion/` from rlvgl widgets v0.2.0 — these
   are richer compositions; gated on lvglpp::core::draw being in place.

## As-built

Implemented (WID-01 — landed 2026-04-27):

- Compiled CMake target `lvglpp::widgets` (was INTERFACE).
- `lvglpp::widgets::Label` with five fields and the four `Widget`
  overrides per `docs/widgets-label/00-label.md` §5.1 / §5.2.
  `Label::draw` issues the canonical two-call sequence
  (`draw_widget_bg` + `draw_text`) per §5.3.
- Module umbrella `widgets.hpp` re-exports `label.hpp`.
- Test target `lvglpp_widgets_label` covers basic accessors,
  default-style opaque bg, translucent bg → `blend_rect`,
  zero-alpha skip-bg, border emits 4 `fill_rects`, text alpha
  modulation by `style.alpha`.
- Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON` — note
  `std::string` is host-friendly; per WID-01 §12 acceptance, an
  embedded-posture variant of Label is a follow-up sub-phase.

Stubbed:

- WID-02 onwards in the Phase plan above. Each needs its own
  per-chapter concepts doc under `docs/widgets-<name>/`.

## Blockers

- **WID-02..WID-08 concepts docs.** Each widget needs its own
  per-chapter concepts doc under `docs/widgets-<name>/` before
  execution. Owner: project lead / per-widget implementer.
- **Asset-loader seam (WID-06).** `Image` is the first widget that
  needs the rlvgl-creator-emitted asset format. The CORE-07
  plugin-surface chapter is ratified, but no decoder plugin has
  landed yet (CORE-07a..m sub-phases). Owner: implementer of the
  rlvgl-creator → lvglpp asset path.

## Definitions

- **`Label`** — As defined in
  `widgets/include/lvglpp/widgets/label.hpp` (this repo). Mirrors
  `rlvgl/widgets/src/label.rs:11` with adapted: `text()` returns
  `std::string_view` (`borrows`) rather than rlvgl's `&str`.
  Authoritative chapter: `docs/widgets-label/00-label.md`.
- **`Button`** — As defined in
  `widgets/include/lvglpp/widgets/button.hpp` (this repo). Mirrors
  `rlvgl/widgets/src/button.rs:11` with adapted: `set_on_click`
  takes `std::function<void(Button&)>` (`owns`, heap-erased) rather
  than rlvgl's `Box<dyn FnMut(&mut Button)>`. Authoritative
  chapter: `docs/widgets-button/00-button.md`.
- **`Checkbox`** — As defined in
  `widgets/include/lvglpp/widgets/checkbox.hpp`. Mirrors
  `rlvgl/widgets/src/checkbox.rs:9`. Authoritative chapter:
  `docs/widgets-toggles/00-checkbox-and-switch.md` §5.1.
- **`Switch`** — As defined in
  `widgets/include/lvglpp/widgets/switch.hpp`. Mirrors
  `rlvgl/widgets/src/switch.rs:10`. Authoritative chapter:
  `docs/widgets-toggles/00-checkbox-and-switch.md` §5.2.
- **`Slider`** — As defined in
  `widgets/include/lvglpp/widgets/slider.hpp`. Mirrors
  `rlvgl/widgets/src/slider.rs:9`. Authoritative chapter:
  `docs/widgets-slider/00-slider.md`.
- **`LV_USE_*` gating** — As defined in
  `lvgl/src/lv_conf_internal.h`; future lvglpp widget headers will
  `#error` if their required `LV_USE_*` is undefined or zero.
  WID-01 (Label) does not currently gate on `LV_USE_LABEL` because
  it does NOT call into LVGL's `lv_label_*` API — text rendering
  goes through `Renderer::draw_text` per the chapter.

## Change log

- 2026-04-27 — Initial scaffold. INTERFACE target only; no widget code.
- 2026-04-27 — WID-01 chapter ratified at
  `docs/widgets-label/00-label.md`. Field set (§5.1), Widget-override
  set (§5.2), draw call sequence (§5.3) frozen. Execution unblocked.
- 2026-04-27 — WID-01 execution landed. `lvglpp::widgets` is now a
  compiled library; `Label` is the first concrete widget. All
  acceptance bullets satisfied; 6 test fixtures green.
- 2026-04-27 — WID-02 chapter ratified
  (`docs/widgets-button/00-button.md`) and execution landed.
  `lvglpp::widgets::Button` composes Label, exposes
  `set_on_click(std::function<void(Button&)>)`, and consumes
  `PressRelease` inside bounds (firing the callback) per §5.3.
  Test target `lvglpp_widgets_button` (7 fixtures); embedded
  posture clean.
- 2026-04-27 — WID-03 chapter ratified
  (`docs/widgets-toggles/00-checkbox-and-switch.md`) and execution
  landed. `lvglpp::widgets::Checkbox` (bounds + text + style +
  text_color + check_color + checked) and
  `lvglpp::widgets::Switch` (bounds + style + knob_color + on)
  share the toggle-on-PressRelease-inside-bounds idiom from WID-02.
  Test targets `lvglpp_widgets_checkbox` (7 fixtures) +
  `lvglpp_widgets_switch` (6 fixtures) green; embedded posture
  clean. **DELTA**: `style.radius` is currently ignored by the
  CORE-04a `fill_rounded_rect` shim; CORE-04b will land actual
  rounded corners.
- 2026-04-27 — WID-04 chapter ratified
  (`docs/widgets-slider/00-slider.md`) and execution landed.
  `lvglpp::widgets::Slider` is the first **value-bound** widget:
  `min`/`max`/`value` triple with ratio-based PressRelease
  position-to-value mapping. `set_value` clamps to range;
  degenerate `min == max` handled. Test target
  `lvglpp_widgets_slider` (11 fixtures) green; embedded posture
  clean. PressRelease-only (no drag-tracking — WID-04a deferred).
