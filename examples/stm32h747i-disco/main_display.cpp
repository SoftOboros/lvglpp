// PARITY: rlvgl/platform/src/display_init.rs::init_full_adapted_cmd
//         (boot path) + the bare-metal main.rs clocks→sdram→display
//         ordering (v0.2.0 @ 79f730d).
// LVGL:   N/A.
// DELTA:  CPU-filled four-quadrant test pattern, then the PLAT-02e-1
//         first-fill gate: a DMA2D R2M magenta rectangle over the live
//         auto-refreshing scan (05-dma2d-engine.md §5.5).
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
// PLAT-02e first-fill relay (0x3800_0380.. — clear of display.cpp's
// diag block at 0x340–0x36C and post-enable relays at 0x370–0x37C):
//   0x3800_0380 = fill result (0x600D_F111 ok / 0xBAD0_F111 fail)
//   0x3800_0384 = dma2d::last_error() (ISR TEIF|CEIF snapshot)
//   0x3800_0388 = RCC AHB3ENR readback (expect bit 4 set)
//   0x3800_038C = fill CPU pixel-verify (0x600D_0000 | fail bitmask;
//                 bit n = sample n mismatched, 0x600D_0000 = all pass)
//   0x3800_0390 = blit result (0x600D_B117 ok / 0xBAD0_B117 fail)
//   0x3800_0394 = blit CPU compare (0x600D_0000 | mismatch count)
// Blind-bench captures (PLAT-02e §5.5 eyes-substitute): 48×80 ARGB
// downsamples of the framebuffer, probe-readable —
//   0x3800_1000 = capture BEFORE display::init (CPU pattern source)
//   0x3800_5000 = capture AFTER the DMA2D fill + blit gates

#include <cstddef>
#include <cstdint>

#include "disco/breadcrumb.hpp"
#include "disco/clocks.hpp"
#include "disco/display.hpp"
#include "disco/dma2d.hpp"
#include "disco/pinmux.hpp"
#include "disco/regs/dsi.hpp"
#include "disco/regs/ltdc.hpp"
#include "disco/regs/rcc.hpp"
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

void capture_framebuffer(std::uintptr_t cap_base) noexcept {
    constexpr std::uint32_t W = lvglpp::disco::display::PANEL_W;  // 480
    const auto* fb = reinterpret_cast<volatile std::uint32_t*>(
        lvglpp::disco::display::FRAMEBUFFER_BASE);
    auto* cap = reinterpret_cast<volatile std::uint32_t*>(cap_base);
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

// PLAT-02e-1 first-fill bench gate (05-dma2d-engine.md §5.5): DMA2D
// R2M fill of a centered 200×300 magenta rectangle over the live
// quadrant pattern. Magenta appears nowhere in the CPU pattern, so a
// visible rectangle proves the DMA2D engine wrote it.
void dma2d_first_fill() noexcept {
    namespace disco = lvglpp::disco;
    constexpr std::uint32_t W = disco::display::PANEL_W;  // 480
    constexpr std::uint32_t H = disco::display::PANEL_H;  // 800
    constexpr std::uint32_t RECT_W = 200;
    constexpr std::uint32_t RECT_H = 300;
    constexpr std::uint32_t X0 = (W - RECT_W) / 2;
    constexpr std::uint32_t Y0 = (H - RECT_H) / 2;
    constexpr std::uint32_t MAGENTA = 0xFFFF'00FFu;

    *d3(0x88) = disco::regs::RCC->ahb3enr;  // expect DMA2DEN (bit 4)

    disco::dma2d::init();
    auto* fb = reinterpret_cast<volatile std::uint32_t*>(
        disco::display::FRAMEBUFFER_BASE);
    const bool ok = disco::dma2d::fill(fb + Y0 * W + X0, W,
                                       RECT_W, RECT_H, MAGENTA);

    *d3(0x80) = ok ? 0x600D'F111u : 0xBAD0'F111u;
    *d3(0x84) = disco::dma2d::last_error();
    dsb();
    if (ok) {
        disco::breadcrumb::write(disco::breadcrumb::DMA2D_FIRST_DONE);
    }

    // CPU pixel-verify (eyes-substitute): sample inside the rect (must
    // be magenta) and just outside each edge (must be the untouched
    // quadrant color). Fail bitmask relayed to 0x38C.
    struct Sample { std::uint32_t x, y, expect; };
    constexpr std::uint32_t RED  = 0xFFFF'0000u;
    constexpr std::uint32_t GREEN = 0xFF00'FF00u;
    constexpr std::uint32_t BLUE = 0xFF00'00FFu;
    constexpr Sample samples[] = {
        {X0,              Y0,              MAGENTA},  // rect TL
        {X0 + RECT_W - 1, Y0,              MAGENTA},  // rect TR
        {X0,              Y0 + RECT_H - 1, MAGENTA},  // rect BL
        {X0 + RECT_W - 1, Y0 + RECT_H - 1, MAGENTA},  // rect BR
        {X0 + RECT_W / 2, Y0 + RECT_H / 2, MAGENTA},  // rect center
        {X0 - 1,          Y0,              RED},      // left of rect, top
        {X0 + RECT_W,     Y0,              GREEN},    // right of rect, top
        {X0 - 1,          Y0 + RECT_H - 1, BLUE},     // left of rect, bottom
        {X0,              Y0 - 1,          RED},      // above rect
        {X0,              Y0 + RECT_H,     BLUE},     // below rect
    };
    std::uint32_t fail = 0;
    for (std::uint32_t i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
        if (fb[samples[i].y * W + samples[i].x] != samples[i].expect)
            fail |= 1u << i;
    }
    *d3(0x8C) = 0x600D'0000u | fail;
    dsb();
}

// PLAT-02e-1 blit gate: M2M copy of a 120×20 block spanning the
// RED→BLUE quadrant seam (src y=390..409) into the GREEN top-right
// quadrant, then CPU-compare src vs dst word-for-word. A stride or
// geometry error in the M2M path shows up as a nonzero mismatch
// count at 0x394.
void dma2d_blit_gate() noexcept {
    namespace disco = lvglpp::disco;
    constexpr std::uint32_t W = disco::display::PANEL_W;
    constexpr std::uint32_t BW = 120, BH = 20;
    constexpr std::uint32_t SX = 10,  SY = 390;   // spans seam at y=400
    constexpr std::uint32_t DX = 330, DY = 20;    // inside GREEN quadrant

    auto* fb = reinterpret_cast<volatile std::uint32_t*>(
        disco::display::FRAMEBUFFER_BASE);
    const bool ok = disco::dma2d::blit(fb + SY * W + SX, W,
                                       fb + DY * W + DX, W, BW, BH);
    *d3(0x90) = ok ? 0x600D'B117u : 0xBAD0'B117u;

    std::uint32_t mismatches = 0;
    for (std::uint32_t y = 0; y < BH; ++y) {
        for (std::uint32_t x = 0; x < BW; ++x) {
            if (fb[(SY + y) * W + SX + x] != fb[(DY + y) * W + DX + x])
                ++mismatches;
        }
    }
    *d3(0x94) = 0x600D'0000u | (mismatches & 0xFFFFu);
    dsb();
}

} // namespace

int main() {
    lvglpp::disco::clocks::init();          // 0xA11C_0005
    lvglpp::disco::pinmux::mux_fmc_pins();  // 0xA11C_0007
    lvglpp::disco::sdram::init();            // 0xA11C_0009

    fill_pattern();
    capture_framebuffer(0x3800'1000u);       // pre-display CPU pattern

    lvglpp::disco::display::init();          // 0xA11C_000B (+ LTDC retry diag)

    relay_status();

    dma2d_first_fill();                      // 0xA11C_000D on success
    dma2d_blit_gate();
    capture_framebuffer(0x3800'5000u);       // post-DMA2D blind capture

    for (;;) asm volatile ("wfe");
}


