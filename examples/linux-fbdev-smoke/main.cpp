// main.cpp — PLAT-LNX fbdev smoke: render the WID-05/WID-06
// conformance scene to a Linux framebuffer console, then exit on
// first touch (evdev) or SIGINT.
//
// PARITY: rlvgl examples/beaglebone-black main-loop shape, minus all
//         board specifics (PLAT-LNX §5.3: full-frame flush; dirty
//         rects are a consumer concern).
// LVGL:   N/A.
// DELTA:  env overrides LVGLPP_FB / LVGLPP_INPUT (mirrors RLVGL_FB /
//         RLVGL_INPUT).

#include <cstdio>
#include <cstdlib>

#include "../disco-sim/memory_renderer.hpp"
#include "gallery_assets.inc"
#include "../widget-gallery/gallery_scene.hpp"
#include "lvglpp/platform/lnx/evdev_input.hpp"
#include "lvglpp/platform/lnx/fbdev_display.hpp"

namespace lc = lvglpp::core;
namespace lnx = lvglpp::platform::lnx;

int main() {
    const char* fb_path = std::getenv("LVGLPP_FB");
    const char* in_path = std::getenv("LVGLPP_INPUT");
    if (fb_path == nullptr) fb_path = "/dev/fb0";
    if (in_path == nullptr) in_path = "/dev/input/event0";

    auto display = lnx::FbdevDisplay::open(fb_path);
    if (!display.has_value()) {
        std::fprintf(stderr, "fbdev open failed: %s\n", fb_path);
        return 1;
    }
    std::printf("fbdev: %ux%u @ %ubpp\n", display->width(),
                display->height(), display->bits_per_pixel());

    auto scene = lvglpp::gallery::build_scene(
        std::span<const std::uint8_t>(
            lvglpp::gallery::detail::GALLERY_ICON_BYTES));
    if (!scene.has_value()) {
        std::fprintf(stderr, "icon decode failed\n");
        return 1;
    }

    // Offscreen ARGB frame sized to the device, flushed whole
    // (PLAT-LNX §5.3).
    lvglpp::sim::MemoryRenderer renderer{display->width(),
                                         display->height()};
    renderer.fill_rect(
        lc::Rect{0, 0, static_cast<std::int32_t>(display->width()),
                 static_cast<std::int32_t>(display->height())},
        lc::Color{8, 10, 16, 255});
    scene->root.draw(renderer);

    // MemoryRenderer stores ARGB words; flush() wants Colors — read
    // back via pixel(). One conversion pass keeps the renderer
    // reusable across examples.
    std::vector<lc::Color> frame(
        static_cast<std::size_t>(display->width()) * display->height());
    for (std::uint32_t y = 0; y < display->height(); ++y) {
        for (std::uint32_t x = 0; x < display->width(); ++x) {
            const std::uint32_t px = renderer.pixel(x, y);
            frame[static_cast<std::size_t>(y) * display->width() + x] =
                lc::Color{static_cast<std::uint8_t>((px >> 16) & 0xFF),
                          static_cast<std::uint8_t>((px >> 8) & 0xFF),
                          static_cast<std::uint8_t>(px & 0xFF),
                          static_cast<std::uint8_t>(px >> 24)};
        }
    }
    display->flush(lc::Rect{0, 0,
                            static_cast<std::int32_t>(display->width()),
                            static_cast<std::int32_t>(display->height())},
                   std::span<const lc::Color>(frame));
    std::printf("frame flushed; waiting for touch on %s\n", in_path);

    auto input = lnx::EvdevInput::open(in_path);
    if (!input.has_value()) {
        std::fprintf(stderr, "evdev open failed (%s) — exiting\n",
                     in_path);
        return 0;  // frame is up; input is optional for the smoke
    }
    for (;;) {
        if (auto ev = input->poll()) {
            std::printf("touch event received — done\n");
            return 0;
        }
    }
}
