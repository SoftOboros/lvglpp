// screen.hpp — display descriptor value type (DEMO-0S).
//
// PARITY: rlvgl/platform/src/screen.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (host/board display descriptor — not an LVGL handle).
// DELTA:  static `make` + `const` builder methods (Rust `new`/builders
//         clash with the C++ type name); enums become `enum class`;
//         ColorFormat is the DEMO-0S frozen 4-variant set (rlvgl's
//         Rgb444/L8 are out of scope here); `ColorFormat::quantize` is
//         deferred until a consumer needs it. No field/semantic delta on
//         the mirrored surface.
//
// Reference: docs/disco-demo/0S-screen-descriptor.md (DEMO-0S).

#ifndef LVGLPP_PLATFORM_SCREEN_HPP
#define LVGLPP_PLATFORM_SCREEN_HPP

#include <cstdint>

namespace lvglpp::platform {

// Scan-direction rotation applied between logical draw coordinates and
// the physical framebuffer. FROZEN (Standards Action — cross-language):
// variant set mirrors rlvgl/platform/src/screen.rs:40.
enum class Rotation : std::uint8_t {
    Deg0,    // No rotation: logical coordinates match physical.
    Deg90,   // Logical drawing rotated 90° clockwise into the framebuffer.
    Deg180,  // Logical drawing rotated 180° (upside-down).
    Deg270,  // Logical drawing rotated 270° clockwise.
};

// Returns true when this rotation swaps the framebuffer axes (portrait
// physical scan from a landscape logical space). Mirrors rlvgl's
// `Rotation::swaps_axes` (screen.rs:58); named `is_portrait` per the
// DEMO-0S C++ mirror.
[[nodiscard]] constexpr bool is_portrait(Rotation r) noexcept {
    return r == Rotation::Deg90 || r == Rotation::Deg270;
}

// Native colour format of the physical display panel. FROZEN (Standards
// Action — cross-language): variant set is the DEMO-0S 4-variant subset
// of rlvgl/platform/src/screen.rs:77. `color_format` is host-advisory
// for this initiative (the SDL backend renders ARGB8888); the field
// exists for 1:1 parity and the board target later. `quantize` is
// deferred — see DELTA above.
enum class ColorFormat : std::uint8_t {
    Argb8888,  // True-colour 32-bit ARGB. No quantization.
    Rgb888,    // True-colour 24-bit RGB. Discards alpha.
    Rgb565,    // 16-bit RGB565. 5/6/5 bits.
    Mono,      // 1-bit monochrome (e.g. e-paper).
};

// Default display refresh rate used by `Screen::make` when the caller
// does not pick one via `with_frame_hz`. FROZEN (Standards Action).
// Mirrors rlvgl `DEFAULT_FRAME_HZ` (screen.rs:137).
inline constexpr std::uint32_t DEFAULT_FRAME_HZ = 60;

// Logical display geometry plus the scan rotation used to reach the
// physical framebuffer.
//
// Ownership: trivially-copyable value type (DEMO-00 §5). No pointers, no
// LVGL handle, no allocation. Passed by value into `DiscoController::make`.
struct Screen {
    std::uint32_t width = 0;   // Logical width in pixels.
    std::uint32_t height = 0;  // Logical height in pixels.
    Rotation rotation = Rotation::Deg0;
    // host-advisory: simulator quantization preview; HW drivers ignore.
    ColorFormat color_format = ColorFormat::Argb8888;
    // host-advisory: target refresh cadence in Hz; drivers may read it.
    std::uint32_t frame_hz = DEFAULT_FRAME_HZ;

    // Create a screen with an explicit rotation. Defaults to Argb8888
    // and DEFAULT_FRAME_HZ (60 Hz). Mirrors rlvgl `Screen::new`.
    [[nodiscard]] static constexpr Screen make(std::uint32_t w, std::uint32_t h,
                                               Rotation r) noexcept {
        Screen s;
        s.width = w;
        s.height = h;
        s.rotation = r;
        return s;
    }

    // Create an unrotated landscape screen (Deg0, Argb8888, 60 Hz).
    // Mirrors rlvgl `Screen::landscape`.
    [[nodiscard]] static constexpr Screen landscape(std::uint32_t w,
                                                    std::uint32_t h) noexcept {
        return make(w, h, Rotation::Deg0);
    }

    // Return a copy of this screen with a different colour format.
    // Mirrors rlvgl `Screen::with_color_format`.
    [[nodiscard]] constexpr Screen with_color_format(ColorFormat f) const noexcept {
        Screen s = *this;
        s.color_format = f;
        return s;
    }

    // Return a copy of this screen with a different target refresh rate.
    // `frame_hz == 0` is clamped to 1 to keep divide-by-zero guards
    // downstream trivial. Mirrors rlvgl `Screen::with_frame_hz`.
    [[nodiscard]] constexpr Screen with_frame_hz(std::uint32_t hz) const noexcept {
        Screen s = *this;
        s.frame_hz = (hz == 0) ? 1U : hz;
        return s;
    }

    [[nodiscard]] constexpr bool operator==(const Screen&) const noexcept = default;
};

}  // namespace lvglpp::platform

#endif  // LVGLPP_PLATFORM_SCREEN_HPP
