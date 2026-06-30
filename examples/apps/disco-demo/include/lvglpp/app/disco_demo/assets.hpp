// assets.hpp — frozen layout constants + icon-asset accessors.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/assets.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite asset table).
// DELTA:  rlvgl uses `include_bytes!`; lvglpp generates the byte arrays at
//         CMake-configure time from lvglpp-owned `.rle` files and exposes
//         each as a `std::span<const std::uint8_t>` accessor function.
//
// docs/disco-demo/05-composite-widgets.md (DEMO-05) §3. Layout constants are
// the FROZEN set enumerated in DEMO-00 §6; icon names mirror assets.rs:11-45.

#ifndef LVGLPP_APP_DISCO_DEMO_ASSETS_HPP
#define LVGLPP_APP_DISCO_DEMO_ASSETS_HPP

#include <cstdint>
#include <span>

#include "lvglpp/core/widget.hpp"  // core::Color

namespace lvglpp::app::disco_demo {

// ---------------------------------------------------------------------------
// FROZEN layout constants (DEMO-00 §6; mirror assets.rs + lib.rs:225).
// Standards Action — values must equal rlvgl.
// ---------------------------------------------------------------------------
inline constexpr std::int32_t DISPLAY_WIDTH  = 800;
inline constexpr std::int32_t DISPLAY_HEIGHT = 480;
inline constexpr std::int32_t PANEL_WIDTH    = 620;
inline constexpr std::int32_t PANEL_HEIGHT   = 312;
inline constexpr std::int32_t PANEL_X        = 84;
inline constexpr std::int32_t PANEL_Y        = 84;

inline constexpr std::int32_t STRIP_ICON_SIZE  = 60;
inline constexpr std::int32_t STRIP_MARGIN_TOP = 17;
inline constexpr std::int32_t STRIP_GAP        = 10;
inline constexpr std::int32_t STRIP_X_OFFSET   = 70;

inline constexpr std::int32_t WING_X          = 10;
inline constexpr std::int32_t WING_ICON_SIZE  = 60;
inline constexpr std::int32_t WING_MARGIN_TOP = 17;
inline constexpr std::int32_t WING_GAP        = 10;

// Focus highlight border color (cyan accent) + width. Mirrors
// assets.rs:48-51.
inline constexpr core::Color FOCUS_HIGHLIGHT_COLOR{0, 180, 255, 255};
inline constexpr std::uint8_t FOCUS_BORDER_WIDTH = 2;

// ---------------------------------------------------------------------------
// Icon-asset accessors. Each returns a non-owning view into a static byte
// array generated at CMake-configure time from the lvglpp-owned `.rle` file named
// in assets.rs. Ownership: the returned span `borrows` immortal static
// storage (lifetime is the program's). Consumers decode with core::rle.
// ---------------------------------------------------------------------------

// Main-strip icons (assets.rs:18-23).
[[nodiscard]] std::span<const std::uint8_t> icon_settings() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_file() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_info() noexcept;

// 48px wing icon set (assets.rs:26-45).
[[nodiscard]] std::span<const std::uint8_t> icon_audio48() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_camera48() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_monitor48() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_globe48() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_bug48() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_cpu48() noexcept;
[[nodiscard]] std::span<const std::uint8_t> icon_play48() noexcept;

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_ASSETS_HPP
