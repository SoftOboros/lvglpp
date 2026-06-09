// PARITY: rlvgl/platform/src/display_init.rs::init_full_adapted_cmd +
//         all init_dsi_* / configure_ltdc_timing / setup_ltdc_layer /
//         enable_ltdc, dsi_cmd_mode.rs (configure_te_gpio /
//         configure_adapted_cmd_mode / start_dsi / *_lp_cmd_overrides),
//         and nt35510.rs::init (v0.2.0 @ 79f730d). RM0399 §33/§34.
// LVGL:   N/A.
// DELTA:  (1) Peripheral clock-gating + PLL3-force are NOT repeated
//         here — disco::clocks::init() already gates LTDC/DSI/DMA2D +
//         GPIO G/J and forces PLL3 on (clocks.cpp steps 6-7); a guarded
//         PLL3 re-check is kept for parity with ensure_pll3_running.
//         The LPENR keep-clocked-in-CSleep writes are omitted: on
//         bare-metal the *LPENR reset default is LPEN=1, so LTDC/FMC
//         stay clocked through the main_display WFE loop. (2) Pixel
//         clock for the DSI video-timing derivation is taken as the
//         display_init.rs shipped const (27.5 MHz); the lvglpp PLL3_R
//         is 32 MHz — flagged for bench verification (§8 timing).
//
// PLAT-02d §7 init order. See docs/platform-disco/04-ltdc-dsi-and-panel.md.
//
// `external: DSI/LTDC/panel state configured once at boot; lifecycle is
//  owned by the hardware and never torn down.`

#include "display.hpp"

#include <cstddef>
#include <cstdint>

#include "breadcrumb.hpp"
#include "regs/dsi.hpp"
#include "regs/gpio.hpp"
#include "regs/ltdc.hpp"

namespace lvglpp::disco::display {

namespace {

using namespace regs;

inline void dsb() noexcept { asm volatile ("dsb" ::: "memory"); }

// Bounded busy-wait. Same shape as sdram::delay_cycles — the asm
// clobber blocks the optimizer from eliding the loop. Counts mirror
// the cortex_m::asm::delay() arguments in display_init.rs.
void delay_cycles(std::uint32_t n) noexcept {
    for (std::uint32_t i = 0; i < n; ++i) asm volatile ("" ::: "memory");
}

// Bounded poll for a status bit; returns false on timeout (mirrors the
// `tries` loops in display_init.rs, which abandon the step on timeout).
template <typename Pred>
[[nodiscard]] bool poll(Pred pred) noexcept {
    for (std::uint32_t i = 0; i < 1'000'000u; ++i) {
        if (pred()) return true;
    }
    return false;
}

// ── Panel reset / backlight / TE (GPIO) ──────────────────────────────

// Drive PG3 (panel reset) low, wait, then high. display_init.rs:175.
void pulse_panel_reset() noexcept {
    auto& g = GPIOG.ref();
    // PG3 as GP output: MODER[7:6] = 01.
    g.moder = (g.moder & ~(3u << 6)) | (1u << 6);
    g.bsrr  = 1u << 19;   // PG3 low (reset bit = 16 + 3)
    delay_cycles(4'000'000);  // ~10 ms
    g.bsrr  = 1u << 3;    // PG3 high
    delay_cycles(4'000'000);  // ~10 ms recovery
}

// Drive PJ12 (backlight) high. display_init.rs:191.
void enable_backlight() noexcept {
    auto& j = GPIOJ.ref();
    // PJ12 as GP output: MODER[25:24] = 01.
    j.moder = (j.moder & ~(3u << 24)) | (1u << 24);
    j.bsrr  = 1u << 12;   // PJ12 high
}

// Configure PJ2 as DSI_TE (AF13). dsi_cmd_mode.rs::configure_te_gpio.
void configure_te_gpio() noexcept {
    auto& j = GPIOJ.ref();
    j.moder = (j.moder & ~(3u << 4)) | (2u << 4);    // PJ2 = AF
    j.afrl  = (j.afrl  & ~(0xFu << 8)) | (13u << 8); // AFRL[11:8] = AF13
}

// ── DSI bring-up (display_init.rs steps 2..8) ────────────────────────

bool init_dsi_regulator() noexcept {
    auto& w = DSI_W.ref();
    w.wrpcr = w.wrpcr | wrpcr::REGEN;
    return poll([&] { return (DSI_W.ref().wisr & wisr::RRS) != 0; });
}

bool init_dsi_pll() noexcept {
    // HSE 25 MHz / IDF 5 = 5 MHz, NDIV 100 → VCO 1000 MHz, ODF 0 →
    // 500 Mbps/lane, lane byte clock 62.5 MHz. display_init.rs:228-242.
    DSI_W.ref().wrpcr = wrpcr::pll(/*idf*/5, /*ndiv*/100, /*odf*/0);
    return poll([&] { return (DSI_W.ref().wisr & wisr::PLLLS) != 0; });
}

void init_dsi_phy() noexcept {
    auto& d = DSI.ref();
    d.cr  = cr::EN;            // enable host first (HAL order)
    d.ccr = 4;                // TXECKDIV = 4
    d.pctlr = d.pctlr | (1u << 1);  // DEN
    d.pctlr = d.pctlr | (1u << 2);  // CKE
    d.pconfr = (1u & 0x3u) | ((0x28u & 0xFFu) << 8);  // NL=1 (2 lanes), SW_TIME=0x28
    (void)poll([&] { return (DSI.ref().psr & psr::LANES_STOPPED) == psr::LANES_STOPPED; });
}

void init_dsi_phy_wpcr_and_clk() noexcept {
    auto& d = DSI.ref();
    auto& w = DSI_W.ref();
    w.wpcr0 = (w.wpcr0 & ~0x3Fu) | 8u;  // UIX4 = 8
    d.cr = 0;                            // disable host (HAL: after PHY init)
    d.clcr = 0x03;                       // DPCC | ACR (auto clock-lane control)
}

void init_dsi_lane_timings() noexcept {
    auto& d = DSI.ref();
    d.cltcr = (50u << 16) | 50u;
    d.dltcr = (15u << 24) | (50u << 16) | 50u;
}

void init_dsi_flow_control() noexcept {
    DSI.ref().pcr = 0x15;     // ETTXE | BTAE | ECCRXE
}

void init_dsi_ltdc_interface() noexcept {
    auto& d = DSI.ref();
    d.lvcidr = 0;             // VCID = 0
    d.lcolcr = (1u << 8) | 5u; // LPE | COLC=5 (RGB888)
    d.lpcr   = 0;             // default polarity
}

void init_dsi_video_timings() noexcept {
    auto& d = DSI.ref();
    // Lane byte clock 62.5 MHz; pixel clock per display_init.rs shipped
    // const (27.5 MHz). See DELTA at file head re: lvglpp PLL3_R = 32 MHz.
    constexpr std::uint32_t lane_byte_clk = 62500;  // kHz
    constexpr std::uint32_t pixel_clk     = 27500;  // kHz
    constexpr std::uint32_t total_pixels =
        std::uint32_t{HSW} + HBP + PANEL_W + HFP;
    const std::uint32_t hsa   = std::uint32_t{HSW} * lane_byte_clk / pixel_clk;
    const std::uint32_t hbp   = std::uint32_t{HBP} * lane_byte_clk / pixel_clk;
    const std::uint32_t hline = total_pixels * lane_byte_clk / pixel_clk;

    d.vhsacr = hsa   & 0xFFFu;
    d.vhbpcr = hbp   & 0xFFFu;
    d.vlcr   = hline & 0x7FFFu;
    d.vvsacr = std::uint32_t{VSW} & 0x3FFu;
    d.vvbpcr = std::uint32_t{VBP} & 0x3FFu;
    d.vvfpcr = std::uint32_t{VFP} & 0x3FFu;
    d.vvacr  = std::uint32_t{PANEL_H} & 0x3FFFu;
    d.vpcr   = std::uint32_t{PANEL_W} & 0x3FFFu;
    d.vccr   = 0;
    d.vnpcr  = 0xFFFu;
    // VMCR: non-burst with sync events, LP commands enabled.
    d.vmcr = (0b10u << 0)
           | (1u << 8) | (1u << 9) | (1u << 10) | (1u << 11)
           | (1u << 12) | (1u << 13) | (1u << 15);
    d.lpmcr = (64u << 16) | 64u;
}

// ── Adapted command mode (dsi_cmd_mode.rs) ───────────────────────────

void configure_adapted_cmd_mode() noexcept {
    auto& d = DSI.ref();
    auto& w = DSI_W.ref();
    // LCCR @ 0x064 — command size = display width. The dsi.hpp offsetof
    // assert is what makes the "0x64 not 0x2C" invariant compile-enforced.
    d.lccr = std::uint32_t{PANEL_W};
    w.wcfgr = (1u << 0)   // DSIM = adapted command mode
            | (5u << 1)   // COLMUX = RGB888
            | (1u << 4);  // TESRC (external TE)
    d.cmcr = 1;           // TEARE
    w.wier = 0x03;        // TEIE | ERIE
}

void start_dsi() noexcept {
    DSI.ref().cr    = cr::EN;
    DSI_W.ref().wcr = wcr::DSIEN;   // DSIEN only — LTDCEN pulsed per frame
    dsb();
    delay_cycles(2'000'000);        // ~5 ms settle
}

void enable_lp_cmd_overrides() noexcept {
    DSI.ref().cmcr =
          (1u << 19)  // DLWTX — DCS long write in LP
        | (1u << 17)  // DSW1TX — DCS short write 1p in LP
        | (1u << 16)  // DSW0TX — DCS short write 0p in LP
        | (1u << 14)  // GLWTX — generic long write in LP
        | (1u << 10)  // GSW2TX
        | (1u << 9)   // GSW1TX
        | (1u << 8);  // GSW0TX
}

void disable_lp_cmd_overrides() noexcept {
    DSI.ref().cmcr = 1;  // TEARE only
}

// ── NT35510 panel DCS init (nt35510.rs::init) ────────────────────────

bool wait_cmd_fifo_empty() noexcept {
    return poll([&] { return (DSI.ref().gpsr & gpsr::CMDFE) != 0; });
}

bool dcs_short_write(std::uint8_t cmd) noexcept {
    if (!wait_cmd_fifo_empty()) return false;
    // GHCR: DT=0x05 | WCLSB=cmd. (DCS short write, 0 params.)
    DSI.ref().ghcr = 0x05u | (std::uint32_t{cmd} << 8);
    return true;
}

bool dcs_short_write1(std::uint8_t cmd, std::uint8_t param) noexcept {
    if (!wait_cmd_fifo_empty()) return false;
    // GHCR: DT=0x15 | WCLSB=cmd | WCMSB=param. (DCS short write, 1 param.)
    DSI.ref().ghcr =
        0x15u | (std::uint32_t{cmd} << 8) | (std::uint32_t{param} << 16);
    return true;
}

// DCS long write (DT=0x39). `data[0]` is the DCS command byte.
bool dcs_long_write(const std::uint8_t* data, std::size_t len) noexcept {
    if (len == 0) return true;
    if (!wait_cmd_fifo_empty()) return false;
    auto& d = DSI.ref();
    // Pack the payload into GPDR, 4 little-endian bytes per word.
    for (std::size_t i = 0; i < len; i += 4) {
        std::uint32_t word = 0;
        for (std::size_t j = 0; j < 4; ++j) {
            if (i + j < len) word |= std::uint32_t{data[i + j]} << (8 * j);
        }
        d.gpdr = word;
    }
    // Header: DT=0x39 | WC = len (WCLSB/WCMSB).
    const std::uint16_t wc = static_cast<std::uint16_t>(len);
    d.ghcr = 0x39u
           | (std::uint32_t{static_cast<std::uint8_t>(wc)} << 8)
           | (std::uint32_t{static_cast<std::uint8_t>(wc >> 8)} << 16);
    return true;
}

// Convenience wrapper for fixed-size DCS long writes.
template <std::size_t N>
bool dcs_long(const std::uint8_t (&bytes)[N]) noexcept {
    return dcs_long_write(bytes, N);
}

bool init_nt35510_panel() noexcept {
    // ── Phase 1: Page 1 — power supply config ──
    static const std::uint8_t p1_enter[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01};
    if (!dcs_long(p1_enter)) return false;
    static const std::uint8_t avdd[]   = {0xB0, 0x03, 0x03, 0x03};
    static const std::uint8_t avddr[]  = {0xB6, 0x46, 0x46, 0x46};
    static const std::uint8_t avee[]   = {0xB1, 0x03, 0x03, 0x03};
    static const std::uint8_t aveer[]  = {0xB7, 0x36, 0x36, 0x36};
    static const std::uint8_t vcl[]    = {0xB2, 0x00, 0x00, 0x02};
    static const std::uint8_t vclr[]   = {0xB8, 0x26, 0x26, 0x26};
    dcs_long(avdd); dcs_long(avddr); dcs_long(avee); dcs_long(aveer);
    dcs_long(vcl);  dcs_long(vclr);
    dcs_short_write1(0xBF, 0x01);                       // VGH setting
    static const std::uint8_t vgh[]   = {0xB3, 0x09, 0x09, 0x09};
    static const std::uint8_t vghr[]  = {0xB9, 0x36, 0x36, 0x36};
    static const std::uint8_t vgl[]   = {0xB5, 0x08, 0x08, 0x08};
    static const std::uint8_t vglxr[] = {0xBA, 0x26, 0x26, 0x26};
    static const std::uint8_t vgmp[]  = {0xBC, 0x00, 0x80, 0x00};
    static const std::uint8_t vgmn[]  = {0xBD, 0x00, 0x80, 0x00};
    static const std::uint8_t vcom[]  = {0xBE, 0x00, 0x50};
    dcs_long(vgh); dcs_long(vghr); dcs_long(vgl); dcs_long(vglxr);
    dcs_long(vgmp); dcs_long(vgmn); dcs_long(vcom);

    // ── Phase 2: Page 0 — display control ──
    static const std::uint8_t p0_enter[] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00};
    dcs_long(p0_enter);
    static const std::uint8_t dispopt[] = {0xB1, 0xFC, 0x00};
    dcs_long(dispopt);
    dcs_short_write1(0xB6, 0x03);                       // source output hold
    dcs_short_write1(0xB5, 0x50);                       // resolution (480×800)
    static const std::uint8_t gateeq[]  = {0xB7, 0x00, 0x00};
    dcs_long(gateeq);
    static const std::uint8_t srceq[]   = {0xB8, 0x01, 0x02, 0x02, 0x02};
    dcs_long(srceq);
    static const std::uint8_t inv[]     = {0xBC, 0x00, 0x00, 0x00};
    dcs_long(inv);
    static const std::uint8_t disptim[] = {0xCC, 0x03, 0x00, 0x00};
    dcs_long(disptim);
    dcs_short_write1(0xBA, 0x01);                       // source drive capability

    // ── Phase 3: standard DCS ──
    dcs_short_write1(0x36, 0x00);                       // MADCTL: portrait
    static const std::uint8_t caset[] = {0x2A, 0x00, 0x00, 0x01, 0xDF};  // col 0..479
    static const std::uint8_t paset[] = {0x2B, 0x00, 0x00, 0x03, 0x1F};  // page 0..799
    dcs_long(caset); dcs_long(paset);

    if (!dcs_short_write(0x11)) return false;           // sleep out
    delay_cycles(80'000'000);                           // ~200 ms

    dcs_short_write1(0x3A, 0x77);                        // pixel format RGB888
    dcs_short_write1(0x35, 0x00);                        // set tear on (V-blank)
    dcs_short_write1(0x51, 0x7F);                        // brightness 50%
    dcs_short_write1(0x53, 0x2C);                        // BL+DD+BL_EN
    dcs_short_write1(0x55, 0x02);                        // CABC still picture
    dcs_short_write1(0x5E, 0xFF);                        // CABC min brightness

    if (!dcs_short_write(0x29)) return false;            // display on
    delay_cycles(4'000'000);
    return true;
}

// ── LTDC config (display_init.rs configure_ltdc_timing / setup_ltdc_layer) ──

void configure_ltdc_timing() noexcept {
    auto& l = LTDC.ref();
    constexpr std::uint32_t hswm1  = HSW - 1u;
    constexpr std::uint32_t vshm1  = VSW - 1u;
    constexpr std::uint32_t ahbp   = std::uint32_t{HSW} + HBP - 1u;
    constexpr std::uint32_t avbp   = std::uint32_t{VSW} + VBP - 1u;
    constexpr std::uint32_t aaw    = std::uint32_t{HSW} + HBP + PANEL_W - 1u;
    constexpr std::uint32_t aah    = std::uint32_t{VSW} + VBP + PANEL_H - 1u;
    constexpr std::uint32_t totalw = std::uint32_t{HSW} + HBP + PANEL_W + HFP - 1u;
    constexpr std::uint32_t totalh = std::uint32_t{VSW} + VBP + PANEL_H + VFP - 1u;
    l.sscr = (hswm1  << 16) | vshm1;
    l.bpcr = (ahbp   << 16) | avbp;
    l.awcr = (aaw    << 16) | aah;
    l.twcr = (totalw << 16) | totalh;
    l.bccr = 0;
}

void setup_ltdc_layer() noexcept {
    auto& y = LTDC_L1.ref();
    constexpr std::uint32_t pitch = std::uint32_t{PANEL_W} * 4u;  // ARGB8888
    constexpr std::uint32_t x0 = std::uint32_t{HSW} + HBP + 1u;
    constexpr std::uint32_t x1 = x0 + PANEL_W - 1u;
    constexpr std::uint32_t y0 = std::uint32_t{VSW} + VBP + 1u;
    constexpr std::uint32_t y1 = y0 + PANEL_H - 1u;
    y.whpcr  = (x1 << 16) | x0;
    y.wvpcr  = (y1 << 16) | y0;
    y.pfcr   = l1pfcr::ARGB8888;
    y.cacr   = 255;                       // constant alpha
    y.bfcr   = 0x0405;                     // PAxCA / 1-PAxCA blending
    y.cfbar  = static_cast<std::uint32_t>(FRAMEBUFFER_BASE);
    y.cfblr  = (pitch << 16) | (pitch + 7u);
    y.cfblnr = PANEL_H;
    y.cr     = y.cr | l1cr::LEN;
    LTDC.ref().srcr = srcr::IMR;           // immediate shadow reload
}

void enable_ltdc() noexcept {
    auto& l = LTDC.ref();
    l.gcr  = gcr::ENABLE_VALUE;            // reset value + LTDCEN
    l.srcr = srcr::IMR;
    dsb();
}

} // namespace

void init() noexcept {
    // Pre-condition: clocks::init() gated LTDC/DSI/DMA2D + GPIO G/J and
    // forced PLL3 on (clocks.cpp steps 6-7); sdram::init() made the
    // framebuffer memory live. No clock re-gating needed here.

    // §7.3 — panel reset early (mirrors main.rs early PG3 pulse).
    pulse_panel_reset();

    // §7.5-7.11 — DSI host bring-up.
    (void)init_dsi_regulator();
    (void)init_dsi_pll();
    init_dsi_phy();                  // host EN, CCR, PCTLR, PCONFR, PSR wait
    init_dsi_phy_wpcr_and_clk();     // WPCR0 UIX4, host disable, CLCR
    init_dsi_lane_timings();
    init_dsi_flow_control();
    init_dsi_ltdc_interface();
    init_dsi_video_timings();

    // Adapted command mode + TE, then start the host/wrapper.
    configure_te_gpio();
    configure_adapted_cmd_mode();
    start_dsi();

    // §7.14 — panel DCS init in LP.
    enable_lp_cmd_overrides();
    (void)init_nt35510_panel();
    disable_lp_cmd_overrides();

    // §7.12-7.13 — LTDC timing + layer + enable.
    // BENCH ITER (PLAT-02d): firmware LTDC writes don't latch on the
    // first pass during init (the LTDC isn't settled), while the same
    // writes from a fully-settled state do (proven: probe-written config
    // → correct image). Write-and-verify with retry until GCR reads back
    // the enable value, then relay the retry count + final GCR to D3 for
    // diagnosis.
    std::uint32_t ltdc_tries = 0;
    do {
        configure_ltdc_timing();
        setup_ltdc_layer();
        enable_ltdc();
        ++ltdc_tries;
    } while (LTDC.ref().gcr != gcr::ENABLE_VALUE && ltdc_tries < 200'000u);
    *reinterpret_cast<volatile std::uint32_t*>(0x3800'0330u) = ltdc_tries;
    *reinterpret_cast<volatile std::uint32_t*>(0x3800'0334u) = LTDC.ref().gcr;
    *reinterpret_cast<volatile std::uint32_t*>(0x3800'0338u) = LTDC.ref().twcr;

    enable_backlight();

    // §7.15 — enable the LTDC→DSI path free-running (WCR = DSIEN|LTDCEN),
    // matching rlvgl's init_full_adapted_cmd and the probe-driven test
    // that produced a clean image on the panel. (The earlier single-shot
    // was a mis-diagnosis: the SDRAM "corruption" seen via framebuffer
    // capture was only the CPU's *concurrent* reads contending with the
    // LTDC, not the LTDC's own display read path. The TE/ERIF-gated
    // holdoff present is still PLAT-02e.)
    delay_cycles(8'000'000);         // ~20 ms settle
    DSI_W.ref().wcr = wcr::DSIEN | wcr::LTDCEN;  // 0x0C
    dsb();
    delay_cycles(8'000'000);         // let the first frame complete

    breadcrumb::write(breadcrumb::PANEL_UP);  // 0xA11C_000B
}

} // namespace lvglpp::disco::display
