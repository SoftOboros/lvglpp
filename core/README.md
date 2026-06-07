<!--
README.md — Publish-facing overview for the lvglpp::core library.
-->

# lvglpp::core

Library: `lvglpp::core` (CMake target: `lvglpp_core`, alias
`lvglpp::core`).

`lvglpp::core` is the foundation library for the lvglpp project. It owns
the runtime bootstrap, the widget-tree borrowing surface, and the
renderer / event / style / theme / animation / font / draw seams that
the rest of lvglpp builds on. It is the C++ triangulation of
[`rlvgl-core`](../rlvgl/core/) over upstream
[LVGL](../lvgl/).

## Triangulation

- **Behavior** comes from upstream LVGL (`lvgl/src/core/`).
- **Discipline** comes from rlvgl-core (`rlvgl/core/src/`): ownership,
  error type, `no_std` posture, public-API names.
- **Surface** is C++20 expressed in this library: RAII wrappers,
  `lvglpp::expected` for fallible factories, ownership-tag comments on
  every raw pointer.

## Main Areas

- Runtime bootstrap (`runtime.hpp`)
- Widget-tree borrowing surface (`widget.hpp`, planned)
- Event dispatch (`event.hpp`, planned)
- Renderer trait (`renderer.hpp`, planned)
- Style / theme / animation (`style.hpp`, `theme.hpp`, `animation.hpp`, planned)
- Font helpers — bitmap + packed (`font.hpp`, planned)
- Draw helpers (`draw.hpp`, planned)
- Plugin registration for image / QR / lottie decoders, mirrored from
  rlvgl-core's `plugins/` (planned, see STATUS.md)

## Target Model

`lvglpp::core` aims for the freestanding subset of the C++ standard
library — see [`docs/std-mapping.md`](../docs/std-mapping.md) §
"Freestanding subset". When `LVGLPP_EMBEDDED_POSTURE` is on, exceptions
and RTTI are disabled and panic-equivalent paths use `std::abort()` to
mirror rlvgl's `no_std` + `panic = abort` posture.

## Where It Is Used

- `lvglpp::widgets` builds concrete widgets on top of it.
- `lvglpp::ui` layers higher-level components and theming on top.
- `lvglpp::platform` provides display / input backends for it.
- `lvglpp::playit` consumes the event surface to inject synthetic input.

## Typical Use

Reach for `lvglpp::core` when you are building a custom widget, writing
a renderer or display backend, or integrating lvglpp into a new runtime.
Most applications consume it indirectly through the umbrella
`lvglpp/lvglpp.hpp`.

## License

MIT.
