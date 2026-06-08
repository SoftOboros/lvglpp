// platform.hpp — module umbrella for lvglpp::platform.
//
// PARITY: rlvgl/platform/src/lib.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/drivers/, lvgl/src/display/, lvgl/src/indev/.
//
// This umbrella does not pull in any backend by default. Each backend
// (host SDL, STM32H747I-DISCO, BeagleBone Black DRM, ESP32 LCD, …) is
// opt-in via its own CMake option and exposes its own header under
// `include/lvglpp/platform/<backend>.hpp`.

#ifndef LVGLPP_PLATFORM_PLATFORM_HPP
#define LVGLPP_PLATFORM_PLATFORM_HPP

// DEMO-0S display descriptor — backend-independent value type. Builds
// without SDL and under embedded posture; always available.
#include "lvglpp/platform/screen.hpp"

// PLAT-01 host SDL backend is opt-in via LVGLPP_PLATFORM_HOST_SDL.
// Pull in the header only when that option is on (the header itself
// `#error`s under embedded posture).
#if defined(LVGLPP_PLATFORM_HOST_SDL_ENABLED)
#include "lvglpp/platform/host_sdl.hpp"
#endif

// Future per-backend headers (added as they land):
//   #include "lvglpp/platform/stm32h747i_disco.hpp"   // PARITY: rlvgl/platform/src/stm32h747i_disco/
//   #include "lvglpp/platform/beaglebone_black.hpp"   // PARITY: rlvgl/platform/src/beaglebone/
//   #include "lvglpp/platform/esp32_lcd.hpp"          // PARITY: rlvgl/platform/src/esp32/

#endif  // LVGLPP_PLATFORM_PLATFORM_HPP
