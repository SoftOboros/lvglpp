<!--
README.md — Initiative README for the PLAT-01 Host SDL backend.
-->

# platform-host-sdl — initiative README

This initiative ratifies the **host SDL backend** — the first concrete
`lvglpp::core::Renderer` subclass and the first runnable, on-screen
demo target. Used as the default desktop simulator for lvglpp; not
intended for embedded targets (those are PLAT-02 / PLAT-03 / PLAT-04
in `platform/STATUS.md`).

This README is **informative**. The normative artifact is the chapter
[`00-host-sdl-backend.md`](./00-host-sdl-backend.md).

## Status

Chapter ratified at draft level (2026-04-27). PLAT-01 execution is
unblocked by ratification of CORE-02 / CORE-03 / CORE-04 / CORE-05 /
CORE-06 and the WID-01 Label landing.

## Cross-language pair

rlvgl's analogous backend is `rlvgl/platform/src/simulator.rs` (winit
+ wgpu via egui/eframe — a heavier stack). lvglpp picks **SDL2**
instead because it is smaller, more portable to lightweight CI, and
more idiomatic for a C++ host backend. This is a deliberate
divergence — the chapter §10 reconciliation explains the choice.

The contract that crosses the language pair is the **event
translation table** (SDL_Event → `lvglpp::core::Event`). That
translation MUST agree with how rlvgl's simulator translates winit
events — the playit fixtures depend on identical event ordering.

## Dependencies

- SDL2 (header + library). On macOS: `brew install sdl2`. On Debian /
  Ubuntu: `apt install libsdl2-dev`. The CMake configuration uses
  `find_package(SDL2 REQUIRED)` and emits a clear "install SDL2" error
  if the package is missing.

PLAT-01 is **excluded under `LVGLPP_EMBEDDED_POSTURE=ON`**. Building
this backend in embedded posture is a configuration error.
