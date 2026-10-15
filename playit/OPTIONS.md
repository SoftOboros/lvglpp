<!--
OPTIONS.md — Build-flag reference for the lvglpp::playit library.
-->

# lvglpp::playit Options

`lvglpp::playit` is a compiled library. It inherits
`LVGLPP_EMBEDDED_POSTURE` from the project-wide CMake scope; the parser MUST compile cleanly under
embedded posture (no exceptions, no dynamic allocation in the hot
parse path) because it runs on every board target.

## Planned options

| Planned option | Default | Effect |
| --- | --- | --- |
| `LVGLPP_PLAYIT_RECORDER` | `OFF` | Enable the `RS` / `RE` / `RD` event-recorder commands. Adds a small ring buffer; gated off by default to keep the embedded footprint small. |
| `LVGLPP_PLAYIT_FRAMEBUFFER_DUMP` | `OFF` | Enable the `D<x>,<y>,<w>,<h>` framebuffer-dump command. Requires the renderer to support readback. |
| `LVGLPP_PLAYIT_TAG_QUERY` | `OFF` | Enable `T@<tag>:`, `QB:`, `QE:`, `QC:` tagged-widget queries. Requires `lvglpp::core::Widget` tag support. |

## Consumer builds

`LVGLPP_BUILD_LIBRARIES` enables this module (`ON` by default, `OFF`
for `Generic` targets). Set it `ON` for library-only cross builds, with
`LVGLPP_BUILD_TESTS=OFF`, `LVGLPP_BUILD_EXAMPLES=OFF`, and
`LVGLPP_EMBEDDED_POSTURE=ON`. Consumer LVGL configurations are selected
through upstream `LV_BUILD_CONF_PATH` or `LV_BUILD_CONF_DIR`; see
[consumer and Buildroot builds](../README.md#consumer-and-buildroot-builds).

## LVGL snapshot reader

`LV_USE_SNAPSHOT` enables upstream snapshot APIs and `LvglSnapshotReader`.
It defaults to `1` in lvglpp's bundled host header; downstream headers retain
control of the setting. With `0`, the adapter is omitted, existing framebuffer
readers remain available, and its host test is reported as skipped.
The reader always requests `LV_COLOR_FORMAT_ARGB8888`; enable the corresponding
draw-unit support (normally `LV_DRAW_SW_SUPPORT_ARGB8888`) even on RGB565 displays.
See [snapshot integration and lifetime requirements](README.md#lvgl-snapshot-dumps).
