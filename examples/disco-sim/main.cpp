// main.cpp — host-SDL simulator for the shared 747-style disco demo.
//
// PARITY: rlvgl examples/disco-sim (rlvgl-disco-sim). Drives the DEMO-06
//         DiscoController in an SDL2 window and over a playit transport,
//         and mirrors the rlvgl automation surface (DEMO-07):
//         --screen=WxH, --headless[=path], --automation-headless,
//         --playit-port[=N] with the PLAYIT_READY handshake.
// LVGL:   N/A (host simulator).
// DELTA:  host command execution is a thin adapter (DEMO-06 §6);
//         PNG capture and --color are declared-unimplemented and the
//         windowed default transport stays stdin (DEMO-07 §5.1).
//
// Build:
//   cmake -S . -B build -DLVGLPP_PLATFORM_HOST_SDL=ON
//   cmake --build build --target lvglpp_example_disco_sim
//
// Headless parity drive (same script as rlvgl-disco-sim):
//   ./lvglpp_example_disco_sim --automation-headless --playit-port=0
//   -> PLAYIT_READY tcp://127.0.0.1:<port>

#include "lvglpp/app/disco_demo/capabilities.hpp"
#include "lvglpp/app/disco_demo/command.hpp"
#include "lvglpp/app/disco_demo/disco_controller.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/widget.hpp"
#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/platform/host_sdl.hpp"
#include "lvglpp/platform/screen.hpp"
#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/event_recorder.hpp"
#include "lvglpp/playit/executor.hpp"
#include "lvglpp/playit/gesture.hpp"
#include "lvglpp/playit/stdio_transport.hpp"
#include "lvglpp/playit/tcp_transport.hpp"

#include "memory_renderer.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>

namespace ad = lvglpp::app::disco_demo;
namespace lc = lvglpp::core;
namespace lp = lvglpp::platform;
namespace lpit = lvglpp::playit;

namespace {

// Windowed clear color — headless frames start from the same value
// (DEMO-07 §5.3).
constexpr lc::Color CLEAR_COLOR{8, 10, 16, 255};
constexpr const char* DEFAULT_HEADLESS_PATH = "disco-headless.txt";

// Quit only on 'q'/'Q' (Escape is a live FSM key — it closes a wing).
bool is_quit_key(const lc::Event& event) noexcept {
    const auto* kd = std::get_if<lc::event::KeyDown>(&event);
    if (kd == nullptr) return false;
    if (const auto* ch = std::get_if<lc::key::Character>(&kd->key)) {
        return ch->codepoint == static_cast<std::uint32_t>('q') ||
               ch->codepoint == static_cast<std::uint32_t>('Q');
    }
    return false;
}

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Thin host adapter for drained controller commands (DEMO-06 §6). Effects
// are out of scope (DEMO-00 §11) — they only log here.
void execute_host(ad::DiscoController& ctl, const ad::DiscoCommand& command) {
    std::visit(
        overloaded{
            [](const ad::cmd::SetBacklight& c) {
                std::fprintf(stderr, "[host] backlight -> %u%%\n",
                             static_cast<unsigned>(c.level));
            },
            [&ctl](const ad::cmd::LoadStorageSummary&) {
                std::fprintf(stderr, "[host] load storage summary\n");
                ctl.publish_status("Storage: flash 2 of 4 MB, SD 1.2 GB free (mock)");
            },
            [](const ad::cmd::StartEffect& c) {
                std::fprintf(stderr, "[host] start effect %d\n",
                             static_cast<int>(c.effect));
            },
            [](const ad::cmd::StopEffect& c) {
                std::fprintf(stderr, "[host] stop effect %d\n",
                             static_cast<int>(c.effect));
            },
            [](const ad::cmd::ShowStatus& c) {
                std::fprintf(stderr, "[host] status: %s\n", c.text.c_str());
            },
            [](const ad::cmd::NoOp&) {},
        },
        command);
}

// ── DEMO-07 §5.1 CLI surface (mirrors rlvgl-disco-sim parse_args) ───
struct CliOptions {
    std::uint32_t width  = 800;
    std::uint32_t height = 480;
    std::optional<std::string> headless_path;
    std::optional<std::string> png_path;
    bool automation_headless = false;
    std::optional<std::uint16_t> playit_port;
};

std::optional<CliOptions> parse_args(int argc, char** argv,
                                     std::string& error) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto next = [&]() -> std::optional<std::string_view> {
            if (i + 1 >= argc) return std::nullopt;
            return std::string_view{argv[++i]};
        };
        auto parse_port = [&](std::string_view v) -> bool {
            char* end = nullptr;
            const std::string s{v};
            const unsigned long port = std::strtoul(s.c_str(), &end, 10);
            if (end == s.c_str() || *end != '\0' || port > 0xFFFFu) {
                error = "invalid --playit-port value: " + s;
                return false;
            }
            options.playit_port = static_cast<std::uint16_t>(port);
            return true;
        };

        if (arg.rfind("--screen=", 0) == 0) {
            const std::string s{arg.substr(9)};
            unsigned w = 0, h = 0;
            if (std::sscanf(s.c_str(), "%ux%u", &w, &h) != 2 || w == 0 ||
                h == 0) {
                error = "invalid --screen value: " + s;
                return std::nullopt;
            }
            options.width  = w;
            options.height = h;
        } else if (arg == "--headless") {
            auto v = next();
            options.headless_path =
                std::string{v.value_or(DEFAULT_HEADLESS_PATH)};
        } else if (arg.rfind("--headless=", 0) == 0) {
            options.headless_path = std::string{arg.substr(11)};
        } else if (arg == "--automation-headless") {
            options.automation_headless = true;
        } else if (arg == "--playit-port") {
            auto v = next();
            if (!v) {
                error = "--playit-port requires a port value";
                return std::nullopt;
            }
            if (!parse_port(*v)) return std::nullopt;
        } else if (arg.rfind("--playit-port=", 0) == 0) {
            if (!parse_port(arg.substr(14))) return std::nullopt;
        } else if (arg == "--color" || arg.rfind("--color=", 0) == 0) {
            // DEMO-07 §5.1 DELTA — declared unimplemented.
            error = "--color not implemented in lvglpp_example_disco_sim "
                    "(DEMO-07 §5.1 DELTA; ARGB8888 only)";
            return std::nullopt;
        } else {
            options.png_path = std::string{arg};
        }
    }
    if (options.automation_headless &&
        (options.headless_path.has_value() || options.png_path.has_value())) {
        // Same message text as rlvgl-disco-sim.
        error = "--automation-headless cannot be combined with screenshot "
                "or ASCII dump flags";
        return std::nullopt;
    }
    if (options.png_path.has_value()) {
        // DEMO-07 §5.1 DELTA — declared unimplemented.
        error = "PNG capture not implemented in lvglpp_example_disco_sim "
                "(DEMO-07 §5.1 DELTA)";
        return std::nullopt;
    }
    return options;
}

// One shared demo step: playit poll → tick → drain → (caller renders).
struct SimStack {
    ad::DiscoController controller;
    lpit::Dispatcher dispatcher;
    lpit::EventRecorder recorder;
    lpit::Executor executor;
    std::uint32_t tick_counter    = 0;
    std::uint32_t present_counter = 0;

    SimStack(lp::Screen screen, lpit::Transport& transport)
        : controller{ad::DiscoController::make(
              screen, ad::DiscoCapabilities::simulator())},
          dispatcher{controller.root()},
          executor{transport, dispatcher} {
        executor.set_recorder(&recorder);
    }

    void step() {
        dispatcher.set_status_snapshot(
            lpit::StatusData{tick_counter, present_counter});
        (void)executor.poll();
        controller.tick();
        for (const auto& command : controller.drain_commands()) {
            execute_host(controller, command);
        }
        recorder.tick();
        ++tick_counter;
    }

    void render(lc::Renderer& renderer, std::uint32_t width,
                std::uint32_t height) {
        renderer.fill_rect(lc::Rect{0, 0, static_cast<std::int32_t>(width),
                                    static_cast<std::int32_t>(height)},
                           CLEAR_COLOR);
        controller.root().draw(renderer);
        ++present_counter;
    }
};

[[noreturn]] void run_automation_headless(SimStack& stack,
                                          std::uint32_t width,
                                          std::uint32_t height) {
    lvglpp::sim::MemoryRenderer renderer{width, height};
    constexpr auto FRAME_TIME = std::chrono::microseconds{16'667};  // 60 Hz
    for (;;) {
        const auto started = std::chrono::steady_clock::now();
        stack.step();
        stack.render(renderer, width, height);
        const auto elapsed = std::chrono::steady_clock::now() - started;
        if (elapsed < FRAME_TIME) {
            std::this_thread::sleep_for(FRAME_TIME - elapsed);
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::string error;
    const auto options_or = parse_args(argc, argv, error);
    if (!options_or.has_value()) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return EXIT_FAILURE;
    }
    const CliOptions& options = *options_or;

    // Transport selection (DEMO-07 §5.1): TCP when --playit-port is
    // given; stdin otherwise (lvglpp windowed default — DELTA vs
    // rlvgl's null transport).
    std::optional<lpit::TcpServerTransport> tcp =
        options.playit_port.has_value()
            ? lpit::TcpServerTransport::bind_loopback(*options.playit_port)
            : std::nullopt;
    lpit::StdioTransport stdio;
    if (options.playit_port.has_value()) {
        if (!tcp.has_value()) {
            std::fprintf(stderr, "failed to bind playit transport\n");
            return EXIT_FAILURE;
        }
        std::printf("PLAYIT_READY tcp://127.0.0.1:%u\n",
                    static_cast<unsigned>(tcp->local_port()));
        std::fflush(stdout);
    }
    lpit::Transport& transport =
        tcp.has_value() ? static_cast<lpit::Transport&>(*tcp)
                        : static_cast<lpit::Transport&>(stdio);

    const auto screen = lp::Screen::landscape(options.width, options.height);
    SimStack stack{screen, transport};

    // ── headless ASCII: one step, one frame, write, exit ────────────
    if (options.headless_path.has_value()) {
        lvglpp::sim::MemoryRenderer renderer{options.width, options.height};
        stack.step();
        stack.render(renderer, options.width, options.height);
        std::ofstream out{*options.headless_path, std::ios::binary};
        if (!out) {
            std::fprintf(stderr, "failed to write headless output\n");
            return EXIT_FAILURE;
        }
        out << renderer.ascii_frame();
        return EXIT_SUCCESS;
    }

    // ── automation-headless: loop forever, no window ────────────────
    if (options.automation_headless) {
        run_automation_headless(stack, options.width, options.height);
    }

    // ── windowed (DEMO-06 behavior) ─────────────────────────────────
    auto backend_or = lp::HostSdlBackend::try_make(
        "lvglpp disco-demo", static_cast<int>(options.width),
        static_cast<int>(options.height));
    if (!backend_or.has_value()) {
        std::fprintf(stderr, "lvglpp SDL init failed (code %u)\n",
                     static_cast<unsigned>(backend_or.error()));
        return EXIT_FAILURE;
    }
    auto backend = std::move(backend_or).value();

    // PLAYIT-04a — raw SDL Pointer{Down,Up,Move} → PressDown/PressRelease/
    // DoubleTap. 60 Hz matches SDL VSYNC.
    lpit::GesturePipeline pipeline{60};
    auto dispatch_pipeline_output = [&](const lpit::PipelineOutput& out) {
        if (out.primary) (void)stack.controller.dispatch_event(*out.primary);
        if (out.secondary)
            (void)stack.controller.dispatch_event(*out.secondary);
    };

    while (!backend.quit_requested()) {
        while (auto event_opt = backend.poll_event()) {
            if (is_quit_key(*event_opt)) {
                return EXIT_SUCCESS;
            }
            dispatch_pipeline_output(pipeline.process(*event_opt));
        }

        stack.step();
        if (!tcp.has_value() && stdio.is_eof()) {
            return EXIT_SUCCESS;
        }

        backend.clear(CLEAR_COLOR);
        stack.controller.root().draw(backend.renderer());
        backend.present_frame();
        ++stack.present_counter;

        dispatch_pipeline_output(pipeline.tick());
    }
    return EXIT_SUCCESS;
}
