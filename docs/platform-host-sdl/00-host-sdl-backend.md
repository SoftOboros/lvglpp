# 00 — Host SDL backend

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAT-01**.

The key words **MUST**, **SHOULD**, **MAY** are interpreted per
RFC 2119 / 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `Renderer` virtual surface | `docs/core-renderer/00-renderer-trait.md` | Canonical. The SDL backend implements this; it does not extend it. |
| `Event` variant set + `Key` variant set | `docs/core-event/00-event-surface.md` | Canonical. SDL_Event values translate into these; no new variants are introduced. |
| SDL2 API surface | upstream SDL2 (`<SDL.h>`) | External. Versions: SDL2 ≥ 2.0.18 (any 2.0.x is acceptable). |
| Event ordering across rlvgl + lvglpp | rlvgl's `platform/src/simulator.rs` event-translation logic | Cross-language contract. The SDL→Event mapping in §5.4 MUST produce the same output sequence rlvgl's simulator does for the same physical input. |

## §1 Purpose

Land the first concrete `Renderer` subclass and the first runnable,
on-screen demo target. The host SDL backend turns lvglpp from a
header library into a thing the user can launch and see; it also
forces the renderer / event / widget surfaces to compile end-to-end
against a real consumer.

## §2 Problem statement

Up to and including WID-01 every test renderer in lvglpp has been a
`RecordingRenderer` (test-only subclass). No real backend has
exercised the renderer surface, the event-translation seam, or the
"present a frame" loop. PLAT-01 closes that gap with the smallest
non-test backend that runs on every developer's host machine.

## §3 Canonical glossary

- **`HostSdlBackend`** — Owned by this chapter. Will be mirrored as
  `lvglpp::platform::HostSdlBackend` at
  `platform/include/lvglpp/platform/host_sdl.hpp`. Owns
  `SDL_Window*`, `SDL_Renderer*`, the event-pump state, and the
  `SdlRenderer` instance. RAII; destructor calls `SDL_DestroyRenderer`
  / `SDL_DestroyWindow` / `SDL_Quit`.
- **`SdlRenderer`** — Owned by this chapter. Concrete
  `lvglpp::core::Renderer` subclass that wraps an `SDL_Renderer*`
  (non-owning — `external` lifetime). Implements
  `fill_rect`, `draw_text` (via `lvglpp::core::fonts::FONT_6X10`),
  inherits default `blend_rect` / `draw_pixels`.
- **Quit-requested flag** — `bool quit_requested()` returns true after
  the user closes the window or the host sends `SDL_QUIT`. Application
  loops MUST check this between frames.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Backend ownership shape (RAII over SDL handles) | this chapter §5.1 | `lvglpp::platform::HostSdlBackend` lifetime — single instance per process. |
| SDL→Event translation table | this chapter §5.4 — **Standards Action** | rlvgl's simulator MUST produce the same Event sequence for the same physical input. |
| Window-config defaults | this chapter §5.2 | Tunable per construction; documented defaults frozen. |
| Embedded-posture exclusion | this chapter §5.5 | The header `#error`s under `LVGLPP_EMBEDDED_POSTURE`. |

## §5 Frozen decisions

### §5.1 Backend ownership shape — **Specification Required**

`HostSdlBackend` is RAII over the SDL handles:

- Construction calls `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)`
  if not already initialized, then `SDL_CreateWindow` +
  `SDL_CreateRenderer`.
- Construction is fallible — exposed via
  `try_make(...) -> lvglpp::expected<HostSdlBackend, SdlError>`.
- Destruction releases handles in reverse order
  (`SDL_DestroyRenderer`, `SDL_DestroyWindow`, `SDL_Quit`).
- The backend is **move-only** (move-construct allowed; move-assign
  deleted). Mirrors `Runtime`'s shape.
- At most **one** `HostSdlBackend` MAY be alive at a time. The
  enforcement is a single-instance flag analogous to `Runtime`.

### §5.2 Window / renderer config — **Specification Required**

| Knob | Default | Where set |
| --- | --- | --- |
| Window title | constructor arg | `try_make` parameter. |
| Window size | constructor arg | `try_make` parameter. |
| Window flags | `SDL_WINDOW_SHOWN \| SDL_WINDOW_RESIZABLE` | Frozen. |
| Renderer flags | `SDL_RENDERER_ACCELERATED \| SDL_RENDERER_PRESENTVSYNC` | Frozen. |
| Blend mode | `SDL_BLENDMODE_BLEND` | Set by the constructor on the SDL_Renderer; required for `Color` alpha to round-trip through `fill_rect`. |
| Background clear color | `lvglpp::core::Color{0, 0, 0, 255}` | Tunable per `clear()` call. |

### §5.3 `Renderer` overrides — **Specification Required**

`SdlRenderer` overrides:

| Method | Body |
| --- | --- |
| `fill_rect(Rect, Color)` | `SDL_SetRenderDrawColor(...)` + `SDL_RenderFillRect(...)`. |
| `draw_text(x, y, text, color)` | `lvglpp::core::fonts::FONT_6X10.draw_str(*this, x, y - FONT_6X10.scaled_height(), text, color)`. The Label's baseline-anchor convention (CORE-04 §5.1, WID-01 §5.3) is honoured by translating baseline → top-left via `scaled_height()`. |
| `blend_rect`, `draw_pixels` | inherited defaults from `Renderer`. |

### §5.4 SDL → `Event` translation table — **Standards Action**

The mapping from SDL events to `lvglpp::core::Event`. Per-frame the
backend's `poll_event()` returns the next translated event or
`std::nullopt` once the SDL queue is drained.

| SDL event | Translation |
| --- | --- |
| `SDL_QUIT` | sets `quit_requested()`; `poll_event` returns `nullopt`. |
| `SDL_WINDOWEVENT_CLOSE` | same as `SDL_QUIT`. |
| `SDL_MOUSEBUTTONDOWN` (button=LEFT) | `event::PointerDown{x, y}`. |
| `SDL_MOUSEBUTTONUP` (button=LEFT) | `event::PointerUp{x, y}`. |
| `SDL_MOUSEMOTION` (with LEFT held) | `event::PointerMove{x, y}`. |
| `SDL_MOUSEMOTION` (no button held) | dropped. |
| `SDL_KEYDOWN` (non-repeat) | `event::KeyDown{translate_key(...)}`. |
| `SDL_KEYUP` | `event::KeyUp{translate_key(...)}`. |
| `SDL_FINGERDOWN` / `UP` / `MOTION` | dropped (touch translation deferred to a later sub-phase). |

`translate_key` mapping:

| `SDL_Keycode` | `Key` variant |
| --- | --- |
| `SDLK_ESCAPE` | `key::Escape{}` |
| `SDLK_RETURN` | `key::Enter{}` |
| `SDLK_SPACE` | `key::Space{}` |
| `SDLK_UP` | `key::ArrowUp{}` |
| `SDLK_DOWN` | `key::ArrowDown{}` |
| `SDLK_LEFT` | `key::ArrowLeft{}` |
| `SDLK_RIGHT` | `key::ArrowRight{}` |
| `SDLK_F1`..`SDLK_F12` | `key::Function{n}` (n=1..12) |
| Printable ASCII (range 0x20..=0x7E) | `key::Character{codepoint}` |
| anything else | `key::Other{static_cast<uint32_t>(SDL_Keycode)}` |

This table is **Standards Action**. Adding a translation requires
matching changes on the rlvgl simulator side (`winit Key` →
`Event::KeyDown{key}` mapping at `rlvgl/platform/src/simulator.rs`)
or a documented divergence in §10.

**Gestures are deferred.** PLAT-01 emits raw pointer events only.
`PressDown` / `PressRelease` / `DoubleTap` come from a recogniser
pipeline (PLAYIT-02 `EventPipeline` or a future CORE sub-phase),
fed downstream of the backend's `poll_event()`.

### §5.5 Embedded posture exclusion — **Standards Action**

The header `host_sdl.hpp` MUST `#error` if
`LVGLPP_EMBEDDED_POSTURE` is defined. SDL2 pulls in `<iostream>`,
`<thread>`, dynamic linking, and process-level state that is
fundamentally incompatible with the freestanding subset
(`docs/std-mapping.md` § "Freestanding subset"). Cross-builds MUST
NOT enable `LVGLPP_PLATFORM_HOST_SDL` and `LVGLPP_EMBEDDED_POSTURE`
simultaneously.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `simulator.rs` (winit + wgpu via eframe).** rlvgl picked
  the eframe stack because it brings panic-window + screenshot
  helpers for free, and the rlvgl-creator tooling already owns that
  dependency. lvglpp does NOT have rlvgl-creator-style asset tooling
  yet (the C++ creator path is deferred per CLAUDE.md
  § "Doc Co-Location Policy"), so picking the smaller SDL2 dependency
  is appropriate. The cross-language event-translation contract
  (§5.4) is preserved; the heavy parts of eframe (panic UI, shaders)
  are outside the contract.
- **LVGL display driver API (`lv_display_t` etc.).** PLAT-01 does
  NOT consume LVGL's display driver API. `SdlRenderer` is a direct
  `lvglpp::core::Renderer` subclass — it bypasses LVGL's flush-cb
  pipeline because the render path goes through `Renderer::fill_rect`
  / `draw_text` immediately. A future PLAT sub-phase MAY add a
  `lvglpp::platform::LvglDisplayBridge` if a host backend wants
  LVGL's compositing.

## §11 Non-goals

- **Hardware-accelerated path tuning.** SDL_RenderFillRect has
  acceptable performance for the simulator. DMA2D-style overrides
  belong on real boards.
- **Multi-window apps.** One backend = one window.
- **Audio / file I/O.** Out of scope.
- **HiDPI / Retina pixel-density handling.** The backend opens at
  the supplied logical size; HiDPI tuning is a follow-up sub-phase.
- **Touch input.** SDL touch events are dropped; mouse-emulated touch
  is sufficient for the simulator path.
- **Gesture recognition.** PressDown/PressRelease/DoubleTap come
  from PLAYIT-02 / a future recogniser pipeline downstream.

## §12 Acceptance checklist

A conforming PLAT-01 execution PR MUST satisfy:

- [ ] `lvglpp::platform::HostSdlBackend` is RAII over SDL handles
      per §5.1.
- [ ] `lvglpp::platform::SdlRenderer` overrides `fill_rect` and
      `draw_text` per §5.3; `blend_rect` / `draw_pixels` inherit
      defaults.
- [ ] `poll_event` returns `std::optional<Event>` produced by the
      §5.4 translation table.
- [ ] `quit_requested()` correctly latches on `SDL_QUIT` /
      `SDL_WINDOWEVENT_CLOSE`.
- [ ] CMake gating: `LVGLPP_PLATFORM_HOST_SDL` defaults OFF; when
      ON, `find_package(SDL2 REQUIRED)` is called and a clear
      install-SDL2 message is produced if missing.
- [ ] `host_sdl.hpp` `#error`s under `LVGLPP_EMBEDDED_POSTURE`.
- [ ] PARITY/LVGL/DELTA cite block at the head of every public
      header and translation unit.
- [ ] An example target `examples/host_sdl_label` opens a window
      displaying a Label and exits cleanly on close. The example
      builds successfully when `LVGLPP_PLATFORM_HOST_SDL=ON` and
      SDL2 is installed.
- [ ] `platform/STATUS.md` change log records the PLAT-01 landing.

A conforming PR MAY:

- Skip an automated headless test for the SDL backend if SDL2's
  dummy video driver is not reliably available in CI; the
  build-success of `examples/host_sdl_label` is acceptable smoke
  evidence.

## §13 Files cited

- `lvglpp/docs/core-event/00-event-surface.md`,
  `lvglpp/docs/core-renderer/00-renderer-trait.md`,
  `lvglpp/docs/core-widget/00-widget-tree.md`,
  `lvglpp/docs/widgets-label/00-label.md`,
  `lvglpp/docs/std-mapping.md`
- `rlvgl/platform/src/simulator.rs` (informative — alternative
  simulator)
- SDL2 documentation (external; versions 2.0.18+)

## §14 Unblocks

- Running on-screen demos for every future widget.
- PLAYIT-02 `EventPipeline` end-to-end smoke (real input flowing
  through a recogniser into a widget tree).
- A baseline against which PLAT-02 (STM32H747I-DISCO) can be
  compared during bring-up.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Backend ownership
  shape (§5.1), window / renderer config (§5.2), `Renderer`
  overrides (§5.3), SDL → Event translation (§5.4), embedded
  posture exclusion (§5.5) frozen. Execution unblocked.
- 2026-04-27 — PLAT-01 execution landed.
  `platform/include/lvglpp/platform/host_sdl.hpp` +
  `platform/src/host_sdl/host_sdl.cpp`. `lvglpp::platform` switches
  from INTERFACE umbrella to compiled library when
  `LVGLPP_PLATFORM_HOST_SDL=ON`. CMake gating issues a clear
  install-SDL2 error path; embedded-posture mutual-exclusion
  enforced at configure time. Example `examples/host_sdl_label/`
  ships as the build-success smoke target. (Compile / run
  verification on this host requires `brew install sdl2`; OFF-path
  build verified to leave the 9 existing ctest entries green.)
- 2026-04-27 — §5.2 renderer-flags clarified: the backend tries
  `SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC` first
  and **falls back to `flags=0`** (SDL chooses; typically
  software) on failure. This unlocks `SDL_VIDEODRIVER=dummy`
  headless diagnostic runs that would otherwise fail with
  RendererFailed. Verified end-to-end on macOS 13 + SDL2 2.32.10:
  a piped fixture (`QE: / QB: / QC: / RS / T@ok_button:.. / RD /
  ?`) emits the exact rlvgl wire format, including
  `REC:START,2 / @0 T320,160 / @1 T320,160 / REC:END`. The
  cross-language diagnostic loop is operational.
