// main.cpp — widget-gallery conformance sim (WID-05/WID-06 bar).
//
// PARITY: reuses the DEMO-07 automation surface (PLAYIT_READY
//         handshake, headless ASCII, playit TCP + D dumps); the
//         scene is gallery_scene.hpp. No SDL, no window — this
//         binary exists so the List+Image composition is drivable
//         and dump-verifiable headlessly.
// LVGL:   N/A.
// DELTA:  flag subset of DEMO-07 §5.1: --headless[=path],
//         --automation-headless, --playit-port[=N]. No windowed
//         mode (use disco-sim for windows).

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <thread>

#include "../disco-sim/memory_renderer.hpp"
#include "gallery_assets.inc"  // generated: GALLERY_ICON_BYTES
#include "gallery_scene.hpp"
#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/executor.hpp"
#include "lvglpp/playit/framebuffer.hpp"
#include "lvglpp/playit/tcp_transport.hpp"

namespace lc = lvglpp::core;
namespace lpit = lvglpp::playit;

namespace {

constexpr std::uint32_t W = 800, H = 480;

class Reader final : public lpit::FramebufferReader {
public:
    Reader(const lvglpp::sim::MemoryRenderer& fb,
           const std::uint32_t& present) noexcept
        : fb_{&fb}, present_{&present} {}
    [[nodiscard]] std::uint32_t read_pixel(std::int32_t x,
                                           std::int32_t y) const override {
        if (x < 0 || y < 0) return 0;
        return fb_->pixel(static_cast<std::uint32_t>(x),
                          static_cast<std::uint32_t>(y));
    }
    std::size_t read_row(std::int32_t x, std::int32_t y,
                         std::uint16_t width,
                         std::span<std::uint32_t> out) const override {
        std::size_t n = 0;
        for (std::uint16_t i = 0; i < width && n < out.size(); ++i) {
            const std::int32_t px = x + i;
            if (px < 0 || y < 0 ||
                static_cast<std::uint32_t>(px) >= fb_->width() ||
                static_cast<std::uint32_t>(y) >= fb_->height()) break;
            out[n++] = fb_->pixel(static_cast<std::uint32_t>(px),
                                  static_cast<std::uint32_t>(y));
        }
        return n;
    }
    [[nodiscard]] std::uint32_t present_count() const override {
        return *present_;
    }

private:
    const lvglpp::sim::MemoryRenderer* fb_;  // observes
    const std::uint32_t* present_;           // observes
};

}  // namespace

int main(int argc, char** argv) {
    std::optional<std::string> headless_path;
    bool automation = false;
    std::optional<std::uint16_t> port;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--headless") headless_path = "gallery-headless.txt";
        else if (a.rfind("--headless=", 0) == 0) headless_path = a.substr(11);
        else if (a == "--automation-headless") automation = true;
        else if (a.rfind("--playit-port=", 0) == 0)
            port = static_cast<std::uint16_t>(
                std::strtoul(a.c_str() + 14, nullptr, 10));
        else {
            std::fprintf(stderr, "unknown arg: %s\n", a.c_str());
            return 1;
        }
    }

    auto scene = lvglpp::gallery::build_scene(
        std::span<const std::uint8_t>(
            lvglpp::gallery::detail::GALLERY_ICON_BYTES));
    if (!scene.has_value()) {
        std::fprintf(stderr, "icon decode failed\n");
        return 1;
    }

    lvglpp::sim::MemoryRenderer renderer{W, H};
    std::uint32_t tick = 0, present = 0;
    auto render = [&] {
        renderer.fill_rect(
            lc::Rect{0, 0, static_cast<std::int32_t>(W),
                     static_cast<std::int32_t>(H)},
            lc::Color{8, 10, 16, 255});
        scene->root.draw(renderer);
        ++present;
    };

    if (headless_path.has_value()) {
        render();
        std::ofstream out{*headless_path, std::ios::binary};
        if (!out) return 1;
        out << renderer.ascii_frame();
        return 0;
    }

    if (!automation || !port.has_value()) {
        std::fprintf(stderr,
                     "usage: --headless[=path] | "
                     "--automation-headless --playit-port=N\n");
        return 1;
    }

    auto tcp = lpit::TcpServerTransport::bind_loopback(*port);
    if (!tcp.has_value()) return 1;
    std::printf("PLAYIT_READY tcp://127.0.0.1:%u\n",
                static_cast<unsigned>(tcp->local_port()));
    std::fflush(stdout);

    lpit::Dispatcher dispatcher{scene->root};
    lpit::Executor executor{*tcp, dispatcher};
    Reader reader{renderer, present};
    executor.set_framebuffer_reader(&reader);

    constexpr auto FRAME = std::chrono::microseconds{16'667};
    for (;;) {
        const auto t0 = std::chrono::steady_clock::now();
        dispatcher.set_status_snapshot(lpit::StatusData{tick, present});
        (void)executor.poll();
        render();
        ++tick;
        const auto dt = std::chrono::steady_clock::now() - t0;
        if (dt < FRAME) std::this_thread::sleep_for(FRAME - dt);
    }
}
