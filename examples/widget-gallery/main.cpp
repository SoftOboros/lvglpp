// main.cpp — widget-gallery conformance sim (WID-05/WID-06 bar), lv_obj path
// (LVGLPP-WRAP-0N).
//
// PARITY: reuses the DEMO-07 automation surface (PLAYIT_READY handshake,
//         headless ASCII, playit TCP + D dumps); the scene is
//         gallery_scene_obj.hpp. No SDL, no window — this binary exists so the
//         List+Image composition is drivable and dump-verifiable headlessly.
// LVGL:   the scene renders through a headless lv_display (lv_framebuffer.hpp).
// DELTA:  migrated off the hand-rolled core::Renderer/WidgetNode path onto
//         lv_obj widgets + the lv_obj ObjDispatcher. Flag subset unchanged:
//         --headless[=path], --automation-headless, --playit-port[=N].

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <thread>

#include "gallery_assets.inc"  // generated: GALLERY_ICON_BYTES
#include "gallery_scene_obj.hpp"
#include "lv_framebuffer.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/playit/executor.hpp"
#include "lvglpp/playit/framebuffer.hpp"
#include "lvglpp/playit/obj_dispatcher.hpp"
#include "lvglpp/playit/tcp_transport.hpp"

namespace lpit = lvglpp::playit;

namespace {

constexpr std::uint32_t W = 800, H = 480;

// FramebufferReader over the headless lv_display framebuffer (PLAYIT-07a `D`).
class Reader final : public lpit::FramebufferReader {
public:
    Reader(const lvglpp::gallery::LvFramebuffer& fb, const std::uint32_t& present) noexcept
        : fb_{&fb}, present_{&present} {}

    [[nodiscard]] std::uint32_t read_pixel(std::int32_t x, std::int32_t y) const override {
        if (x < 0 || y < 0) return 0;
        return fb_->pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
    }
    std::size_t read_row(std::int32_t x, std::int32_t y, std::uint16_t width,
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
    [[nodiscard]] std::uint32_t present_count() const override { return *present_; }

private:
    const lvglpp::gallery::LvFramebuffer* fb_;  // observes
    const std::uint32_t* present_;              // observes
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
            port = static_cast<std::uint16_t>(std::strtoul(a.c_str() + 14, nullptr, 10));
        else {
            std::fprintf(stderr, "unknown arg: %s\n", a.c_str());
            return 1;
        }
    }

    auto runtime = lvglpp::Runtime::try_make();
    if (!runtime.has_value()) {
        std::fprintf(stderr, "lv_init failed\n");
        return 1;
    }

    lvglpp::gallery::LvFramebuffer fb{W, H};
    lvglpp::gallery::ObjScene scene;
    if (!lvglpp::gallery::build_obj_scene(
            scene, std::span<const std::uint8_t>(
                       lvglpp::gallery::detail::GALLERY_ICON_BYTES))) {
        std::fprintf(stderr, "icon decode failed\n");
        return 1;
    }

    std::uint32_t tick = 0, present = 0;
    auto render = [&] {
        lv_tick_inc(16);
        fb.render();
        ++present;
    };

    if (headless_path.has_value()) {
        render();
        std::ofstream out{*headless_path, std::ios::binary};
        if (!out) return 1;
        out << fb.ascii_frame();
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

    lpit::ObjDispatcher dispatcher{scene.screen.view()};
    lpit::Executor executor{*tcp, dispatcher};
    Reader reader{fb, present};
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
