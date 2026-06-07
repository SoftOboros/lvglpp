<!--
OPTIONS.md — Build-flag reference for the lvglpp::core library.
-->

# lvglpp::core Options

`lvglpp::core` provides the runtime, widget tree, renderer, styling,
plugins, and other foundational pieces. The library targets the
freestanding subset of the C++ standard library.

## Default configuration

- Default features: minimal (Runtime + ObjectView only).
- Runtime model: C++20, freestanding-friendly. Adds full `<atomic>`
  use; everything else is `<cstdint>`-and-friends.
- `LVGLPP_EMBEDDED_POSTURE = OFF` by default (host build, exceptions
  enabled, RTTI enabled).

## Project-wide CMake options that affect this library

These are defined at the top-level `CMakeLists.txt` and apply to
`lvglpp_core`:

| Option | Default | Effect |
| --- | --- | --- |
| `LVGLPP_EMBEDDED_POSTURE` | `OFF` | Adds `-fno-exceptions -fno-rtti`; throwing `Runtime()` constructor calls `std::abort()` instead of throwing. Mirrors rlvgl `no_std` + `panic = abort`. |
| `LVGLPP_BUILD_TESTS` | `ON` | Builds host smoke tests under `tests/`. |
| `LVGLPP_BUILD_EXAMPLES` | `ON` | Builds desktop and board examples under `examples/`. |
| `LVGLPP_USE_RLVGL` | `OFF` | Surfaces rlvgl reference paths to consumers. The rlvgl Rust crate is not linked; this only affects parity tooling. |

## LVGL configuration

`lvglpp::core` includes `<lvgl.h>`, which is configured by the
top-level `include/lvglpp/lv_conf.h`. Override by setting
`LV_BUILD_CONF_PATH` on the CMake command line, or by replacing
`lv_conf.h` in a downstream consumer.

The host default is documented in
[`include/lvglpp/lv_conf.h`](../include/lvglpp/lv_conf.h):

- `LV_COLOR_DEPTH = 32`
- `LV_USE_LOG = 1`
- `LV_LOG_LEVEL = LV_LOG_LEVEL_WARN`
- `LV_USE_OBJ`, `LV_USE_LABEL`, `LV_USE_BUTTON` enabled

Board-specific consumers (e.g. STM32H747I-DISCO) supply their own
`lv_conf.h` and override `LV_BUILD_CONF_PATH`.

## Per-feature flags (planned)

`lvglpp::core` will gain feature flags as the rlvgl-core surface lands.
These mirror rlvgl-core's Cargo features one-for-one; the lvglpp option
prefix is `LVGLPP_CORE_`:

| Planned option | Mirrors rlvgl | Effect |
| --- | --- | --- |
| `LVGLPP_CORE_PNG` | `png` | Enable PNG decoder plugin. |
| `LVGLPP_CORE_JPEG` | `jpeg` | Enable JPEG decoder plugin. |
| `LVGLPP_CORE_GIF` | `gif` | Enable GIF decoder plugin. |
| `LVGLPP_CORE_QRCODE` | `qrcode` | Enable QR generator helper. |
| `LVGLPP_CORE_FONTDUE` | `fontdue` | Enable rich font loading on host. |
| `LVGLPP_CORE_LOTTIE` | `lottie` | Enable Lottie API surface. |
| `LVGLPP_CORE_CANVAS` | `canvas` | Enable off-screen canvas helpers. |
| `LVGLPP_CORE_FATFS` | `fatfs` | Enable FAT filesystem helpers. |

These are intentionally **not** wired up yet. See `STATUS.md` for the
roadmap.

## What that means in practice

- The library compiles cleanly today with no feature flags set.
- Host builds keep exceptions enabled so `Runtime{}` can throw on
  AlreadyAlive. Embedded builds disable exceptions; consumers there
  must use `Runtime::try_make()` and consume the
  `lvglpp::expected<Runtime, RuntimeError>`.
- Code size and runtime cost of the base library are dominated by
  upstream LVGL itself, not by this wrapper.
