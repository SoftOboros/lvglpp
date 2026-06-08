// main.cpp — host-SDL simulator for the shared 747-style disco demo.
//
// PARITY: rlvgl examples/disco-sim (rlvgl-disco-sim). Drives the DEMO-06
//         DiscoController in an SDL2 window and over a playit stdin
//         transport, exactly as examples/host_sdl_label/main.cpp drives
//         its widget tree.
// LVGL:   N/A (host simulator).
// DELTA:  host command execution is a thin adapter (DEMO-06 §6):
//         SetBacklight/Start/StopEffect/ShowStatus log to stderr,
//         LoadStorageSummary publishes a canned summary, NoOp does nothing.
//
// Build:
//   cmake -S . -B build -DLVGLPP_PLATFORM_HOST_SDL=ON
//   cmake --build build --target lvglpp_example_disco_sim
//
// Drive from the shell (T@<tag> taps mirror SDL input through the same
// PressRelease edges):
//   echo 'T@disco.main.settings:760,40' | ./.../lvglpp_example_disco_sim
//   echo 'T@disco.main.files:760,110'   | ./.../lvglpp_example_disco_sim

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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <variant>

namespace ad = lvglpp::app::disco_demo;
namespace lc = lvglpp::core;
namespace lp = lvglpp::platform;
namespace lpit = lvglpp::playit;

namespace {

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

}  // namespace

int main() {
    auto backend_or =
        lp::HostSdlBackend::try_make("lvglpp disco-demo", /*width=*/800,
                                     /*height=*/480);
    if (!backend_or.has_value()) {
        std::fprintf(stderr, "lvglpp SDL init failed (code %u)\n",
                     static_cast<unsigned>(backend_or.error()));
        return EXIT_FAILURE;
    }
    auto backend = std::move(backend_or).value();

    // The controller owns the widget tree + state. Built once; never moved
    // after the Dispatcher binds a reference to its root.
    ad::DiscoController ctl = ad::DiscoController::make(
        lp::Screen::landscape(800, 480), ad::DiscoCapabilities::simulator());

    // ---- playit stack (stdin) — T@<tag> taps drive the FSM -------------
    lpit::StdioTransport transport;
    lpit::Dispatcher dispatcher{ctl.root()};
    lpit::EventRecorder recorder;
    lpit::Executor executor{transport, dispatcher};
    executor.set_recorder(&recorder);

    // PLAYIT-04a — raw SDL Pointer{Down,Up,Move} → PressDown/PressRelease/
    // DoubleTap. 60 Hz matches SDL VSYNC.
    lpit::GesturePipeline pipeline{60};

    auto dispatch_pipeline_output = [&](const lpit::PipelineOutput& out) {
        if (out.primary) (void)ctl.dispatch_event(*out.primary);
        if (out.secondary) (void)ctl.dispatch_event(*out.secondary);
    };

    std::uint32_t tick_counter = 0;
    std::uint32_t present_counter = 0;

    while (!backend.quit_requested()) {
        // SDL events (keyboard + mouse) through the gesture recogniser.
        while (auto event_opt = backend.poll_event()) {
            if (is_quit_key(*event_opt)) {
                return EXIT_SUCCESS;
            }
            dispatch_pipeline_output(pipeline.process(*event_opt));
        }

        // playit (stdin) — external T@<tag> / control commands.
        dispatcher.set_status_snapshot(
            lpit::StatusData{tick_counter, present_counter});
        (void)executor.poll();
        if (transport.is_eof()) {
            return EXIT_SUCCESS;
        }

        // Advance one shared demo tick, then run host-side side effects.
        ctl.tick();
        for (const auto& command : ctl.drain_commands()) {
            execute_host(ctl, command);
        }

        backend.clear(lc::Color{8, 10, 16, 255});
        ctl.root().draw(backend.renderer());
        backend.present_frame();

        dispatch_pipeline_output(pipeline.tick());
        recorder.tick();

        ++tick_counter;
        ++present_counter;
    }
    return EXIT_SUCCESS;
}
