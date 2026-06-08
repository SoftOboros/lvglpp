# lvglpp::app_disco_demo — OPTIONS

## Project-wide options that affect this module

| Option | Default | Effect |
| --- | --- | --- |
| `LVGLPP_BUILD_EXAMPLES` | ON | Adds the `examples/` tree; this module is registered there (host configures only — it links the host module libs). |
| `LVGLPP_BUILD_TESTS` | ON | Builds the four per-widget tests under `tests/`. |
| `LVGLPP_EMBEDDED_POSTURE` | OFF | When ON, `-fno-exceptions -fno-rtti` apply via `lvglpp_posture`. The module compiles clean under both postures. |
| `LVGLPP_PLATFORM_HOST_SDL` / `LVGLPP_PLATFORM_DISCO` | OFF | **Do not** gate this module — it is platform-independent (DEMO-05 §2). |

## Per-module flags

None. The module exposes no `LVGLPP_APP_DISCO_DEMO_*` feature flags.

## Asset pipeline

Icon byte arrays are generated at **CMake configure time** from the rlvgl
`.rle` files under
`rlvgl/examples/stm32h747i-disco/assets/icons/` into
`${CMAKE_CURRENT_BINARY_DIR}/generated/disco_demo_assets.inc` (mirrors the
`core/` `font_6x10.bin` → `.inc` pattern). If a required `.rle` is missing
the configure step **fails loudly** (`FATAL_ERROR`) — there is no silent
empty-icon fallback (DEMO-05 §3).

## Relevant `LV_USE_*` symbols

None — the composites are pure-C++ `core::Widget` subclasses and do not pull
in LVGL widget code.
