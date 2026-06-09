// PARITY: rlvgl/platform/src/display_init.rs (init_full_adapted_cmd +
//         panel constants) + nt35510.rs + dsi_cmd_mode.rs
//         (v0.2.0 @ 79f730d). RM0399 §33/§34.
// LVGL:   N/A.
// DELTA:  Single init() folds rlvgl's free-function sequence; the
//         NT35510 DCS sequence is inlined (nt35510.rs parity) rather
//         than split into a panel module.
//
// PLAT-02d — LTDC + DSI + NT35510 panel bring-up to first pixels.
// See docs/platform-disco/04-ltdc-dsi-and-panel.md.

#pragma once

#include <cstdint>

namespace lvglpp::disco::display {

// PLAT-02d §5 FROZEN — panel geometry (MB1166 / NT35510, portrait).
inline constexpr std::uint16_t PANEL_W = 480;
inline constexpr std::uint16_t PANEL_H = 800;

// PLAT-02d §8 FROZEN — porch constants (display_init.rs L116-133).
inline constexpr std::uint16_t HSW = 2;
inline constexpr std::uint16_t HBP = 34;
inline constexpr std::uint16_t HFP = 34;
inline constexpr std::uint16_t VSW = 120;
inline constexpr std::uint16_t VBP = 150;
inline constexpr std::uint16_t VFP = 150;

// PLAT-02d §5 FROZEN — front framebuffer base (sdram::BASE). ARGB8888,
// 480×800×4 = 1,536,000 B. Back buffer (PLAT-02e) at BASE + 0x0018_0000.
inline constexpr std::uintptr_t FRAMEBUFFER_BASE = 0xD000'0000u;

// Run the full DSI + LTDC + NT35510 bring-up in adapted command mode,
// mirroring display_init.rs::init_full_adapted_cmd register-for-register.
//
// Args:
//   none. The framebuffer base is FRAMEBUFFER_BASE; the DSI/LTDC/panel
//   state is `external:` — configured once at boot, lifecycle owned by
//   the hardware, never torn down.
// Returns:
//   void. On a bounded-poll timeout (regulator/PLL/PHY/FIFO) the step is
//   abandoned and bring-up continues; the D3-SRAM relay in main_display
//   exposes DSI/LTDC status so a halted target reveals where it stalled.
//
// Pre-condition: clocks::init() (gates LTDC/DSI/DMA2D + GPIO G/J, forces
// PLL3 on) and sdram::init() (framebuffer memory live) have already run.
// Writes breadcrumb 0xA11C_000B (PANEL_UP) on completion.
void init() noexcept;

} // namespace lvglpp::disco::display
