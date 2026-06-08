<!-- 0S-screen-descriptor.md — DEMO-0S concepts doc (normative, thin). -->

# DEMO-0S — `Screen` descriptor

Status: **ratified** (2026-06-07). Thin chapter; contract inherited from
DEMO-00 (D4). RFC 2119 keywords per DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical: `rlvgl/platform/src/screen.rs`
(rlvgl `v0.2.0`). This **refines** DEMO-00 §8 D4's shorthand
(`Screen{width,height,rotation}`): the real rlvgl struct has five fields,
and D4 resolved to "mirror 1:1", so DEMO-0S mirrors all five.

## §1 Purpose

Provide the display descriptor `DiscoController::make(Screen, caps)`
takes (DEMO-00 §9), matching the rlvgl constructor 1:1. Wave-A,
independent; gates DEMO-06's constructor.

## §2 Frozen contract (mirror)

As defined in `rlvgl/platform/src/screen.rs:142` (+ `:40,:77`):

```rust
pub enum Rotation { Deg0, Deg90, Deg180, Deg270 }     // is_portrait() => Deg90|Deg270
pub enum ColorFormat { Argb8888, Rgb888, Rgb565, Mono }  // quantize(r,g,b)
pub const DEFAULT_FRAME_HZ: u32 = 60;

pub struct Screen {
    pub width: u32, pub height: u32,
    pub rotation: Rotation,
    pub color_format: ColorFormat,   // host: quantization preview; HW drivers ignore
    pub frame_hz: u32,               // advisory cadence
}
impl Screen {
    pub const fn new(w, h, rotation) -> Self;        // Argb8888, 60 Hz
    pub const fn landscape(w, h) -> Self;            // Deg0
    pub const fn with_color_format(self, f) -> Self;
    pub const fn with_frame_hz(self, hz) -> Self;
}
```

C++ mirror (`platform/`, value type — no LVGL handle):

```cpp
// platform/include/lvglpp/platform/screen.hpp
namespace lvglpp::platform {
enum class Rotation : std::uint8_t { Deg0, Deg90, Deg180, Deg270 };
[[nodiscard]] constexpr bool is_portrait(Rotation r) noexcept;       // Deg90|Deg270
enum class ColorFormat : std::uint8_t { Argb8888, Rgb888, Rgb565, Mono };
inline constexpr std::uint32_t DEFAULT_FRAME_HZ = 60;

struct Screen {
  std::uint32_t width = 0, height = 0;
  Rotation      rotation     = Rotation::Deg0;
  ColorFormat   color_format = ColorFormat::Argb8888;
  std::uint32_t frame_hz     = DEFAULT_FRAME_HZ;
  static constexpr Screen make(std::uint32_t w, std::uint32_t h, Rotation r) noexcept;
  static constexpr Screen landscape(std::uint32_t w, std::uint32_t h) noexcept;
  constexpr Screen with_color_format(ColorFormat f) const noexcept;
  constexpr Screen with_frame_hz(std::uint32_t hz) const noexcept;
};
}
```

FROZEN (Standards Action — cross-language): the `Rotation` and
`ColorFormat` variant sets and `DEFAULT_FRAME_HZ`. `color_format` /
`frame_hz` are **host-advisory** for this initiative (the SDL backend
renders ARGB8888 at its own cadence); they exist for 1:1 parity and for
the board target later. `ColorFormat::quantize` MAY be deferred until a
consumer needs it (note in the chapter if so).

DELTA vs rlvgl: Rust `new`/builder → C++ `static make` + `const`
builder methods (avoids clashing with the type name); enums become
`enum class`. No field/semantic delta.

## §3 Files

- `platform/include/lvglpp/platform/screen.hpp` (new)
- `platform/src/screen.cpp` (new, only if `quantize` is implemented;
  otherwise header-only `constexpr`)
- `platform/tests/screen_test.cpp` (new) — constructor defaults,
  `is_portrait`, builder chaining
- `platform/include/lvglpp/platform/platform.hpp` — add the include
- `platform/STATUS.md` — append change-log line

Triangulation cite block per file
(`// PARITY: rlvgl/platform/src/screen.rs` / `// LVGL: N/A (host/board
descriptor)` / `// DELTA: static make + const builders`).

## §4 Ownership

`Screen` is a trivially-copyable value type (DEMO-00 §5: prefer value
types). No pointers, no LVGL handle, no allocation. Passed by value into
`DiscoController::make`.

## §5 Acceptance

- [ ] `Screen::make(800,480,Deg0)` == `Screen::landscape(800,480)` with
      `Argb8888`, `frame_hz == 60`.
- [ ] `is_portrait` true for `Deg90`/`Deg270` only.
- [ ] Builder methods return modified copies (value semantics).
- [ ] Variant sets equal rlvgl's; FROZEN note recorded.
- [ ] Compiles embedded-ON/OFF; Pre-Publish 0–3 green.

## §6 Change log

- _drafted_ — DEMO-0S restated from DEMO-00 §8 D4 / §14; expanded to the
  full five-field rlvgl `Screen`.
- **2026-06-07 — ratified.** Owner directed Wave-A ratification; full
  five-field `Screen` mirror confirmed. Execution may proceed.
