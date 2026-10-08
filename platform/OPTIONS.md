<!--
OPTIONS.md — Build-flag reference for the lvglpp::platform library.
-->

# lvglpp::platform Options

`lvglpp::platform` is a thin INTERFACE umbrella; each backend is opt-in
via its own option. Backends share the project-wide
`LVGLPP_EMBEDDED_POSTURE` toggle.

## Planned per-backend options

These are intentionally not wired up yet. Each lands when its backend
implementation arrives.

| Planned option | Default | Effect |
| --- | --- | --- |
| `LVGLPP_PLATFORM_HOST_SDL` | `OFF` | Build the SDL host backend (`platform/host_sdl/`). Host-only; mutually exclusive with embedded posture. |
| `LVGLPP_PLATFORM_STM32H747I_DISCO` | `OFF` | Build the STM32H747I-DISCO backend (LTDC + DSI + DMA2D + WM8994). Requires `arm-none-eabi` toolchain. |
| `LVGLPP_PLATFORM_BEAGLEBONE_BLACK` | `OFF` | Build the BBB Linux DRM backend. |
| `LVGLPP_PLATFORM_ESP32_LCD` | `OFF` | Build the ESP32 LCD backend. |

A given build SHOULD enable at most one backend; `examples/` ties the
backend choice to the example target.

## LVGL configuration

Consumers supply their `lv_conf.h` through upstream `LV_BUILD_CONF_PATH`
or `LV_BUILD_CONF_DIR`; the bundled `include/lvglpp/lv_conf.h` is only
the fallback. Backend feature macros are consumer-controlled. The
upstream `LV_USE_WAYLAND`, `LV_USE_LINUX_FBDEV`, and `LV_USE_EVDEV`
features are separate from lvglpp's platform adapter options. See
[consumer and Buildroot builds](../README.md#consumer-and-buildroot-builds)
for the matching `CONFIG_LV_*` dependency settings.

`LVGLPP_BUILD_LIBRARIES` enables this module along with the other wrapper
libraries (`ON` by default, `OFF` for `Generic` targets). Library-only
cross builds set it `ON`, disable tests/examples, and set
`LVGLPP_EMBEDDED_POSTURE=ON`. The rlvgl checkout is not needed.
