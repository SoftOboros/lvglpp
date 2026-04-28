<!--
README.md — Publish-facing overview for the lvglpp::widgets library.
-->

# lvglpp::widgets

Library: `lvglpp::widgets` (CMake target: `lvglpp_widgets`, alias
`lvglpp::widgets`).

`lvglpp::widgets` provides the built-in widget set that sits on top of
`lvglpp::core`. It is the C++ triangulation of
[`rlvgl-widgets`](../rlvgl/widgets/) over upstream
[LVGL](../lvgl/src/widgets/).

## Triangulation

- **Behavior** — upstream LVGL `lv_label`, `lv_button`, `lv_slider`, …
- **Discipline** — rlvgl-widgets borrowing rules, event handling, and
  builder shape.
- **Surface** — C++20 widget classes wrapping `lv_obj_t*` per the
  `external` ownership tag in `lvglpp::core::ObjectView`.

## Status

Today this library is an INTERFACE target — the namespace and link
graph exist, but no widgets are implemented yet. See
[`STATUS.md`](./STATUS.md) for the per-widget landing plan.

## Where It Is Used

- `lvglpp::ui` builds higher-level compositions on top of these widgets.
- Application code includes `<lvglpp/widgets/<name>.hpp>` directly or
  the umbrella `<lvglpp/widgets/widgets.hpp>`.

## License

MIT.
