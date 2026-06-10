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
// PLAT-02f-1 serial-pump relay (0x3800_03A0..):
//   0x3800_03A0 = complete lines processed
//   0x3800_03A4 = dark_mode click count (mirror of 0x3C0)
//   0x3800_03A8 = USART RX overrun count
//   0x3800_03AC = pump tick (low word, free-running)
// PLAT-02f-1b widget-tree relay:
//   0x3800_03C0 = dark_mode on_click fire count (fingers-substitute
//                 proof: serial T@/T inject → real Dispatcher →
//                 Button::handle_event → callback)
// PLAT-02e-3a frame-cadence relay:
//   0x3800_03C4 = present count (WISR.BUSY rising edges = AR=1
//                 auto-refresh frames)
//   0x3800_03C8 = last edge-to-edge interval (DWT cycles @ 400 MHz)
//   0x3800_03CC = interval EMA (alpha = 1/8, rlvgl FRAME_BUDGET style)
// PLAT-02e-2 async/ISR gate relay (0x3800_03B0..):
//   0x3800_03B0 = DMA2D ISR complete count (expect 3)
//   0x3800_03B4 = DMA2D ISR error count (expect 0)
//   0x3800_03B8 = gate result (0x600D_0E22 ok / 0xBAD0_0E22 fail)

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include "disco/breadcrumb.hpp"
#include "disco/clocks.hpp"
#include "disco/display.hpp"
#include "disco/dma2d.hpp"
#include "disco/pinmux.hpp"
#include "disco/regs/dsi.hpp"
#include "disco/regs/ltdc.hpp"
#include "disco/regs/nvic.hpp"
#include "disco/regs/rcc.hpp"
#include "disco/sdram.hpp"
#include "disco/renderer.hpp"
#include "disco/usart.hpp"
#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/format.hpp"
#include "lvglpp/playit/parser.hpp"
#include "lvglpp/playit/response.hpp"
#include "lvglpp/widgets/button.hpp"
#include "lvglpp/widgets/container.hpp"
#include "lvglpp/widgets/label.hpp"

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

// PLAT-02e-2 gate (05 §5.7): three sequential async fills, each
// completed via the DMA2D ISR latch — no CR.START busy-wait in this
// path. The FrameBuffer handle round-trips through the InFlight
// token (moved in at submit, released after the latch). MUST run
// after the blocking gates: enable_irq() makes the ISR clear the
// flags wait_done() inspects.
void dma2d_async_gate() noexcept {
    namespace disco = lvglpp::disco;
    namespace d2 = lvglpp::disco::dma2d;
    constexpr std::uint32_t W = disco::display::PANEL_W;
    constexpr std::uint32_t H = disco::display::PANEL_H;
    struct Rect { std::uint32_t x, y, argb; };
    constexpr Rect rects[] = {
        {40,  40,  0xFFFF'FF00u},   // yellow,  RED quadrant
        {220, 380, 0xFF00'FFFFu},   // cyan,    spans the center seams
        {400, 720, 0xFFFF'8000u},   // orange,  WHITE quadrant
    };

    d2::enable_irq();
    disco::FrameBuffer fb{disco::display::FRAMEBUFFER_BASE, W, H, W};

    bool ok = true;
    for (const auto& r : rects) {
        auto inflight = d2::start_fill_async(
            static_cast<disco::FrameBuffer&&>(fb), r.x, r.y, 40, 40, r.argb);
        bool latched = false;
        for (std::uint32_t i = 0; i < 100'000'000u; ++i) {
            if (d2::take_complete()) { latched = true; break; }
        }
        fb = inflight.try_release();
        ok = ok && latched && fb.valid();
    }
    ok = ok && d2::complete_count() == 3 && d2::error_count() == 0;

    *d3(0xB0) = d2::complete_count();
    *d3(0xB4) = d2::error_count();
    *d3(0xB8) = ok ? 0x600D'0E22u : 0xBAD0'0E22u;
    dsb();
}

// ── PLAT-02f-1 serial pump (06-touch-and-uart.md §5.3/§5.5) ─────────
// Speaks the playit wire protocol over USART1/VCP using the canonical
// lvglpp parser + formatter (grammar/format restatement here is
// forbidden — 06 §5.4). Subset: Status, DumpPixels, Inject (parse-ack
// only), Extension; queries/recorder answer ERR until the widget tree
// lands on-target.

void send_response(const lvglpp::playit::Response& resp) noexcept {
    char buf[128];
    const std::size_t n =
        lvglpp::playit::format_response(resp, std::span<char>{buf});
    lvglpp::disco::usart::put(std::string_view{buf, n});
}

// PARITY: rlvgl/playit/src/protocol.rs:566 write_hex_u32 (8 uppercase
// hex digits).
void put_hex_u32(std::uint32_t v) noexcept {
    constexpr char HEX[] = "0123456789ABCDEF";
    char out[8];
    for (int i = 0; i < 8; ++i)
        out[i] = HEX[(v >> ((7 - i) * 4)) & 0xFu];
    lvglpp::disco::usart::put(std::string_view{out, 8});
}

// PARITY: rlvgl/playit/src/executor.rs::emit_dump_if_ready framing.
// DELTA (06 §10): emits immediately from the live AR=1 front buffer;
// rlvgl waits for the next present. Out-of-bounds pixels read as 0.
void emit_dump(const lvglpp::playit::DumpSpec& spec) noexcept {
    namespace disco = lvglpp::disco;
    constexpr std::int32_t W = disco::display::PANEL_W;
    constexpr std::int32_t H = disco::display::PANEL_H;
    const auto* fb = reinterpret_cast<const volatile std::uint32_t*>(
        disco::display::FRAMEBUFFER_BASE);

    disco::usart::put("DUMP:queued\r\n");
    for (std::uint8_t frame = 0; frame < spec.frames; ++frame) {
        disco::usart::put("F\r\n");
        for (std::uint16_t row = 0; row < spec.height; ++row) {
            for (std::uint16_t col = 0; col < spec.width; ++col) {
                const std::int32_t x = spec.x + col;
                const std::int32_t y = spec.y + row;
                const bool in = x >= 0 && x < W && y >= 0 && y < H;
                put_hex_u32(in ? fb[y * W + x] : 0u);
                if (col + 1 < spec.width) disco::usart::put(" ");
            }
            disco::usart::put("\r\n");
        }
    }
    send_response(lvglpp::playit::response::DumpEnd{});
}

// observes: button-click count; written by the dark_mode on_click
// handler, relayed to 0x3800_03C0.
std::uint32_t g_clicks = 0;

[[noreturn]] void serial_pump() noexcept {
    namespace pi = lvglpp::playit;
    namespace cc = lvglpp::core;
    namespace wi = lvglpp::widgets;
    namespace disco = lvglpp::disco;

    // PLAT-02f-1b on-target widget tree — the first heap consumer
    // (WidgetNode children vector + unique_ptr widgets; disco.ld
    // `end` symbol). Tags mirror the cross-language fixture set
    // (family §12 uses `dark_mode`). Static storage: the tree and
    // dispatcher live for the life of the pump.
    // Styled for blind D-dump verification (PLAT-02e-3b): every
    // surface a distinct, greppable ARGB value.
    constexpr cc::Color NAVY  {16,  24,  48,  255};  // FF101830
    constexpr cc::Color FACE  {200, 60,  60,  255};  // FFC83C3C
    constexpr cc::Color WHITE {255, 255, 255, 255};  // FFFFFFFF

    static cc::WidgetNode root = [&] {
        auto cont = std::make_unique<wi::Container>(cc::Rect{0, 0, 480, 800});
        cont->style.bg_color = NAVY;
        cc::WidgetNode node{std::move(cont), "root"};

        auto title = std::make_unique<wi::Label>(
            std::string{"lvglpp"}, cc::Rect{40, 40, 200, 32});
        title->style.bg_color = NAVY;
        title->text_color     = WHITE;
        node.add_child(cc::WidgetNode{std::move(title), "title"});

        auto btn = std::make_unique<wi::Button>(
            std::string{"Dark"}, cc::Rect{40, 600, 200, 64});
        btn->style().bg_color = FACE;
        btn->text_color()     = WHITE;
        btn->set_on_click([](wi::Button&) { *d3(0xC0) = ++g_clicks; });
        node.add_child(cc::WidgetNode{std::move(btn), "dark_mode"});
        return node;
    }();
    static pi::Dispatcher dispatcher{root};

    // PLAT-02e-3b: render the tree into the live framebuffer. The
    // PLAT-02d/02e test pattern + gate rects are intentionally
    // painted over — from here on, serial D dumps show real widgets.
    static lvglpp::disco::FrameBuffer fb{
        disco::display::FRAMEBUFFER_BASE,
        disco::display::PANEL_W, disco::display::PANEL_H,
        disco::display::PANEL_W};
    static lvglpp::disco::DiscoRenderer renderer{fb};
    root.draw(renderer);

    char line[128];
    std::size_t len = 0;
    std::uint32_t lines = 0, tick = 0;

    // PLAT-02e-3a frame-cadence telemetry: under AR=1 each panel TE
    // re-arms a frame transfer; WISR.BUSY pulses once per refresh.
    // Edge-counting it from the pump (sampling at MHz rates vs a
    // ~50 Hz cadence) gives present_count + the frame-budget input
    // for the 05 §10 admission question, DWT-timestamped.
    disco::regs::dwt_enable_cyccnt();
    std::uint32_t presents = 0, prev_busy = 0;
    std::uint32_t last_edge_cyc = 0, interval = 0, interval_ema = 0;

    for (;;) {
        ++tick;
        {
            const std::uint32_t busy =
                disco::regs::DSI_W.ref().wisr & disco::regs::wisr::BUSY;
            if (busy != 0u && prev_busy == 0u) {
                ++presents;
                const std::uint32_t now = disco::regs::DWT.ref().cyccnt;
                interval      = now - last_edge_cyc;
                last_edge_cyc = now;
                interval_ema  = interval_ema == 0u
                    ? interval
                    : interval_ema - (interval_ema >> 3) + (interval >> 3);
            }
            prev_busy = busy;
        }
        int c;
        while ((c = disco::usart::get_byte()) >= 0) {
            const char ch = static_cast<char>(c);
            if (ch == '\n' || ch == '\r') {
                if (len > 0) {
                    ++lines;
                    if (auto cmd = pi::parse_command({line, len})) {
                        // Status and DumpPixels stay pump-local (the
                        // dispatcher has no tick source or
                        // framebuffer reader); everything else goes
                        // through the real host-tested Dispatcher.
                        if (std::get_if<pi::command::Status>(&*cmd)) {
                            send_response(pi::response::Status{{tick, presents}});
                        } else if (const auto* d =
                                       std::get_if<pi::command::DumpPixels>(&*cmd)) {
                            emit_dump(d->spec);
                        } else {
                            send_response(dispatcher.dispatch(*cmd));
                            // Injected events may mutate widget
                            // state; redraw the tree (cheap at the
                            // ~30 Hz scan cadence, and queries are
                            // harmless to repaint after).
                            root.draw(renderer);
                        }
                    }
                    len = 0;
                }
            } else if (len < sizeof(line)) {
                line[len++] = ch;
            } else {
                len = 0;  // oversize line: drop, resync at newline
            }
        }
        if ((tick & 0xFFFu) == 0) {  // relay refresh, ~per-ms scale
            *d3(0xA0) = lines;
            *d3(0xA4) = g_clicks;
            *d3(0xA8) = disco::usart::rx_overruns();
            *d3(0xAC) = tick;
            *d3(0xC4) = presents;
            *d3(0xC8) = interval;
            *d3(0xCC) = interval_ema;
            dsb();
        }
    }
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
    dma2d_async_gate();                      // PLAT-02e-2 ISR latch path
    capture_framebuffer(0x3800'5000u);       // post-DMA2D blind capture

    lvglpp::disco::usart::init();
    serial_pump();                           // never returns
}


