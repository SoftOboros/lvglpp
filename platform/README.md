<!--
README.md — Publish-facing overview for the lvglpp::platform library.
-->

# lvglpp::platform

Library: `lvglpp::platform` (CMake target: `lvglpp_platform`, alias
`lvglpp::platform`).

`lvglpp::platform` provides the display, input, and storage backends
that bind lvglpp to a concrete target — host SDL for desktop testing,
STM32H747I-DISCO for the canonical board, BeagleBone Black + NHD cape
for the Linux-prong, ESP32 LCD for the Espressif family.

Triangulated from [`rlvgl-platform`](../rlvgl/platform/) over upstream
[LVGL](../lvgl/src/drivers/) drivers.

## Status

INTERFACE target only. No backend has been implemented yet. See
[`STATUS.md`](./STATUS.md) for the landing plan.

## License

MIT.
