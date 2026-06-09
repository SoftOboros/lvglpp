// PARITY: rlvgl/platform/src/display_init.rs::init_full_adapted_cmd
//         (boot path) + the bare-metal main.rs clocks→sdram→display
//         ordering (v0.2.0 @ 79f730d).
// LVGL:   N/A.
// DELTA:  CPU-filled four-quadrant test pattern (no DMA2D yet — PLAT-02e).
//
// PLAT-02d §11 acceptance smoke. Boots: clocks → FMC pinmux → SDRAM →
// display::init() → fills the framebuffer at 0xD000_0000 with a
// recognizable four-quadrant + 1px-border ARGB8888 pattern, then relays
// the DSI/LTDC status registers to D3 SRAM so probe-rs can confirm the
// controllers reached the running state before the panel is trusted.
//
// D3-SRAM relay layout (0x3800_0320.. — clear of the SDRAM-test relay at
// 0x3800_0300..0x18):
//   0x3800_0320 = DSI_PSR   (PHY status; lane stop-state / direction)
//   0x3800_0324 = LTDC_CDSR (current display status; VDES/HDES/VSYNC/HSYNC)
//   0x3800_0328 = DSI_WISR  (wrapper interrupt status; RRS/PLLLS/ERIF)
//   0x3800_032C = 0x600D_0000 'reached end' marker

#include <cstddef>
#include <cstdint>

#include "disco/clocks.hpp"
#include "disco/display.hpp"
#include "disco/pinmux.hpp"
#include "disco/regs/dsi.hpp"
#include "disco/regs/ltdc.hpp"
#include "disco/sdram.hpp"

namespace {

inline void dsb() noexcept { asm volatile ("dsb" ::: "memory"); }

inline volatile std::uint32_t* d3(std::uintptr_t off) noexcept {
    return reinterpret_cast<volatile std::uint32_t*>(0x3800'0300u + off);
}

// Four-quadrant ARGB8888 pattern with a 1px white border, written in the
// panel's native portrait orientation (480 wide × 800 tall). Corner
// colors locate the origin and reveal any rotation/geometry error:
//   top-left  RED   top-right    GREEN
//   bot-left  BLUE  bot-right    WHITE
void fill_pattern() noexcept {
    constexpr std::uint32_t W = lvglpp::disco::display::PANEL_W;  // 480
    constexpr std::uint32_t H = lvglpp::disco::display::PANEL_H;  // 800
    constexpr std::uint32_t RED   = 0xFFFF'0000u;
    constexpr std::uint32_t GREEN = 0xFF00'FF00u;
    constexpr std::uint32_t BLUE  = 0xFF00'00FFu;
    constexpr std::uint32_t WHITE = 0xFFFF'FFFFu;

    auto* fb = reinterpret_cast<volatile std::uint32_t*>(
        lvglpp::disco::display::FRAMEBUFFER_BASE);

    for (std::uint32_t y = 0; y < H; ++y) {
        const bool top = y < H / 2;
        for (std::uint32_t x = 0; x < W; ++x) {
            std::uint32_t c;
            if (x == 0 || x == W - 1 || y == 0 || y == H - 1) {
                c = WHITE;                       // 1px border
            } else if (top) {
                c = (x < W / 2) ? RED : GREEN;   // top-left / top-right
            } else {
                c = (x < W / 2) ? BLUE : WHITE;  // bot-left / bot-right
            }
            fb[y * W + x] = c;
        }
    }
    dsb();
}

// Framebuffer "screen capture" — the playit-D equivalent for a board with
// no serial yet. The debug AP cannot read FMC/SDRAM directly, so the CPU
// downsamples the 480×800 framebuffer (every 10th pixel → 48×80) and
// copies it as ARGB8888 into D3 SRAM at 0x3800_1000, which probe-rs CAN
// read. The host reconstructs a 48×80 PNG. NOTE: this shows the
// framebuffer *source* (what the CPU wrote), not the panel's displayed
// output — useful to confirm the source is clean and localize corruption
// to the DSI→panel path.
constexpr std::uint32_t CAP_W = 48;
constexpr std::uint32_t CAP_H = 80;
constexpr std::uint32_t CAP_STEP = 10;  // 480/10=48, 800/10=80

void capture_framebuffer() noexcept {
    constexpr std::uint32_t W = lvglpp::disco::display::PANEL_W;  // 480
    const auto* fb = reinterpret_cast<volatile std::uint32_t*>(
        lvglpp::disco::display::FRAMEBUFFER_BASE);
    auto* cap = reinterpret_cast<volatile std::uint32_t*>(0x3800'1000u);
    for (std::uint32_t cy = 0; cy < CAP_H; ++cy) {
        for (std::uint32_t cx = 0; cx < CAP_W; ++cx) {
            cap[cy * CAP_W + cx] = fb[(cy * CAP_STEP) * W + (cx * CAP_STEP)];
        }
    }
    dsb();
}

void relay_status() noexcept {
    using namespace lvglpp::disco::regs;
    *d3(0x20) = DSI.ref().psr;       // DSI_PSR
    *d3(0x24) = LTDC.ref().cdsr;     // LTDC_CDSR
    *d3(0x28) = DSI_W.ref().wisr;    // DSI_WISR
    *d3(0x2C) = 0x600D'0000u;        // reached-end marker
    dsb();
}

} // namespace

int main() {
    lvglpp::disco::clocks::init();          // 0xA11C_0005
    lvglpp::disco::pinmux::mux_fmc_pins();  // 0xA11C_0007
    lvglpp::disco::sdram::init();            // 0xA11C_0009

    fill_pattern();
    capture_framebuffer();

    lvglpp::disco::display::init();          // 0xA11C_000B (+ LTDC retry diag)

    relay_status();

    for (;;) asm volatile ("wfe");
}


