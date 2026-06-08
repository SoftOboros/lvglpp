# lvglpp::app_disco_demo

Platform-independent app module for the STM32H747I-DISCO demo UI, the C++
sibling of the rlvgl crate `rlvgl-app-disco-demo`
(`rlvgl/examples/apps/disco-demo`). It houses the four composite
`core::Widget`s the demo is built from and the consume-only icon-asset
pipeline.

**Triangulation.** rlvgl (Rust) is canonical for behavior and layout
constants; LVGL upstream owns nothing here (these are app composites, not
LVGL widgets); lvglpp mirrors rlvgl under single-owner RAII (DEMO-00 §5) —
slots and strings owned by value, the `BitmapFont` and icon bytes borrowed,
tap callbacks are `std::function`. No widget holds a `WidgetNode*` or an
`lv_obj_t*`.

## Main areas

- `IconStrip` / `IconSlot` (`SLOT_COUNT = 3`) — right-edge icon carousel
  with focus highlight and tap-index dispatch.
- `Wing` / `WingSlot` (`MAX_SLOTS = 6`, `CLEAR_FRAMES = 3`) — collapsible
  left-edge popup; bounds collapse to zero when hidden, `clear_region`
  repaints the panel rect for a few frames after close.
- `DashboardPanel` — centered title/caption/lines panel; reuses
  `ui::draw_panel_header` + `panel_close_hit` (DEMO-02) and word-wraps text.
- `ActionHotspot` — invisible tap target with an activation closure and a
  visibility predicate (for playit `T@<tag>` automation).
- `assets` — frozen layout constants (DEMO-00 §6) plus icon-byte accessors
  generated at CMake-configure time from the rlvgl `.rle` files; decoded at
  runtime via `core::rle` (DEMO-04) and blitted with `Renderer::draw_pixels`.

## Where it is used

Consumed by DEMO-06 (`DiscoController` + host-SDL target), which wires the
tap callbacks to the controller state and assembles the widgets into the
`WidgetNode` tree.

## Build

Builds whenever `LVGLPP_BUILD_EXAMPLES` is ON (default) on a host configure;
it is **not** gated on `LVGLPP_PLATFORM_HOST_SDL`/`DISCO`. Tests are gated on
`LVGLPP_BUILD_TESTS`.

## License

MIT (mirrors the rlvgl crate).
