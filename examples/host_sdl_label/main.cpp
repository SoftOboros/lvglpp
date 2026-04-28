// main.cpp — end-to-end demo. The full lvglpp stack rendered in an
// SDL2 window AND driven by a stdio playit transport. Clicks the
// Button, toggles the Checkbox + Switch, drags the Slider — same
// commands work piped from a rlvgl playit fixture.
//
// Stack exercised:
//   PLAT-01 SDL backend → SdlRenderer (CORE-04 subclass)
//   CORE-03a WidgetNode tree
//   WID-01 Label / WID-02 Button / WID-03 Checkbox / WID-03 Switch /
//     WID-04 Slider — each tagged for playit addressing.
//   CORE-04a draw_widget_bg + fill_rounded_rect (radius=0 path)
//   PLAYIT-01 parse_command, PLAYIT-04 Dispatcher,
//   PLAYIT-04a GesturePipeline (Tap + DoubleTap),
//   PLAYIT-04b format_response, PLAYIT-06 EventRecorder,
//   PLAYIT-06a tick-delta dump,
//   PLAYIT-07 StdioTransport + Executor.
//
// Build (with SDL2 installed):
//   cmake -S . -B build -DLVGLPP_PLATFORM_HOST_SDL=ON
//   cmake --build build --target lvglpp_example_host_sdl_label
//
// Run + drive from the shell:
//   ./build/.../lvglpp_example_host_sdl_label
//   echo 'T@ok_button:120,90' | ./.../lvglpp_example_host_sdl_label
//   echo 'T@dark_mode:340,90' | ./.../lvglpp_example_host_sdl_label
//   echo 'T@volume:300,180'   | ./.../lvglpp_example_host_sdl_label
//   echo -e 'RS\nT@ok_button:120,90\nT@volume:500,180\nRD' | ./...

#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/platform/host_sdl.hpp"
#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/event_recorder.hpp"
#include "lvglpp/playit/executor.hpp"
#include "lvglpp/playit/gesture.hpp"
#include "lvglpp/playit/parser.hpp"
#include "lvglpp/playit/stdio_transport.hpp"
#include "lvglpp/widgets/button.hpp"
#include "lvglpp/widgets/checkbox.hpp"
#include "lvglpp/widgets/label.hpp"
#include "lvglpp/widgets/slider.hpp"
#include "lvglpp/widgets/switch.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/style.hpp"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace lc = lvglpp::core;
namespace lp = lvglpp::platform;
namespace lpit = lvglpp::playit;
namespace lw = lvglpp::widgets;

namespace {

bool is_quit_key(const lc::Event& event) noexcept {
    const auto* kd = std::get_if<lc::event::KeyDown>(&event);
    if (kd == nullptr) return false;
    if (std::holds_alternative<lc::key::Escape>(kd->key)) return true;
    if (const auto* ch = std::get_if<lc::key::Character>(&kd->key)) {
        return ch->codepoint == static_cast<std::uint32_t>('q') ||
               ch->codepoint == static_cast<std::uint32_t>('Q');
    }
    return false;
}

// Apply a palette to all the styled widgets at once.
void apply_palette(lw::Label& status,
                   lw::Button& ok_button,
                   lw::Checkbox& dark_check,
                   lw::Switch& mute_switch,
                   lw::Slider& volume_slider,
                   bool dark) {
    if (dark) {
        const lc::Color bg{32, 32, 48, 255};
        const lc::Color fg{255, 255, 255, 255};
        const lc::Color accent{60, 90, 200, 255};
        status.style.bg_color     = bg;
        status.text_color         = fg;
        ok_button.style().bg_color = accent;
        ok_button.text_color()     = fg;
        dark_check.style.bg_color = bg;
        dark_check.text_color     = fg;
        dark_check.check_color    = accent;
        mute_switch.style.bg_color = bg;
        mute_switch.knob_color     = accent;
        volume_slider.style.bg_color  = bg;
        volume_slider.knob_color      = accent;
    } else {
        const lc::Color bg{240, 240, 245, 255};
        const lc::Color fg{16, 16, 16, 255};
        const lc::Color accent{60, 120, 60, 255};
        status.style.bg_color     = bg;
        status.text_color         = fg;
        ok_button.style().bg_color = accent;
        ok_button.text_color()     = lc::Color{255, 255, 255, 255};
        dark_check.style.bg_color = bg;
        dark_check.text_color     = fg;
        dark_check.check_color    = accent;
        mute_switch.style.bg_color = bg;
        mute_switch.knob_color     = accent;
        volume_slider.style.bg_color  = bg;
        volume_slider.knob_color      = accent;
    }
}

}  // namespace

int main() {
    auto backend_or = lp::HostSdlBackend::try_make(
        "lvglpp host SDL — Label + Button + Checkbox + Switch + Slider",
        /*width=*/640, /*height=*/240);
    if (!backend_or.has_value()) {
        std::fprintf(stderr, "lvglpp SDL init failed (code %u)\n",
                     static_cast<unsigned>(backend_or.error()));
        return EXIT_FAILURE;
    }
    auto backend = std::move(backend_or).value();

    // ---- Widget construction ---------------------------------------

    auto status_label = std::make_unique<lw::Label>(
        std::string{"Click widgets or pipe playit (T@<tag>:<x>,<y>)"},
        lc::Rect{10, 10, 620, 50});
    status_label->style.border_color = lc::Color{200, 200, 220, 255};
    status_label->style.border_width = 1;
    auto* status_ptr = status_label.get();

    auto button = std::make_unique<lw::Button>(
        std::string{"Toggle theme"},
        lc::Rect{10, 70, 200, 50});
    button->style().border_color = lc::Color{255, 255, 255, 255};
    button->style().border_width = 2;
    auto* button_ptr = button.get();

    auto dark_check = std::make_unique<lw::Checkbox>(
        std::string{"Dark mode"},
        lc::Rect{220, 70, 180, 50});
    dark_check->style.border_color = lc::Color{200, 200, 220, 255};
    dark_check->style.border_width = 1;
    dark_check->set_checked(true);  // app starts in dark mode
    auto* dark_check_ptr = dark_check.get();

    auto mute_switch = std::make_unique<lw::Switch>(
        lc::Rect{410, 70, 80, 50});
    mute_switch->style.border_color = lc::Color{200, 200, 220, 255};
    mute_switch->style.border_width = 1;
    auto* mute_ptr = mute_switch.get();

    auto volume_slider = std::make_unique<lw::Slider>(
        lc::Rect{10, 140, 620, 60}, 0, 100);
    volume_slider->style.border_color = lc::Color{200, 200, 220, 255};
    volume_slider->style.border_width = 1;
    volume_slider->set_value(50);
    auto* volume_ptr = volume_slider.get();

    // ---- WidgetNode tree -------------------------------------------

    auto root_filler = std::make_unique<lw::Label>(
        std::string{""}, lc::Rect{-1, -1, 0, 0});
    lc::WidgetNode root{std::move(root_filler), "root"};
    root.add_child(lc::WidgetNode{std::move(status_label),  "status_label"});
    root.add_child(lc::WidgetNode{std::move(button),        "ok_button"});
    root.add_child(lc::WidgetNode{std::move(dark_check),    "dark_mode"});
    root.add_child(lc::WidgetNode{std::move(mute_switch),   "mute"});
    root.add_child(lc::WidgetNode{std::move(volume_slider), "volume"});

    // Initial palette.
    apply_palette(*status_ptr, *button_ptr, *dark_check_ptr,
                  *mute_ptr, *volume_ptr, /*dark=*/true);

    // Button shares its callback with the Checkbox so either can
    // toggle theme. State is read from dark_check.
    auto refresh_status = [&]() {
        status_ptr->set_text(
            "vol=" + std::to_string(volume_ptr->value()) +
            "  mute=" + (mute_ptr->is_on() ? std::string{"on"}
                                            : std::string{"off"}) +
            "  theme=" + (dark_check_ptr->is_checked()
                              ? std::string{"dark"}
                              : std::string{"light"}));
    };
    button_ptr->set_on_click([&](lw::Button&) {
        dark_check_ptr->set_checked(!dark_check_ptr->is_checked());
        apply_palette(*status_ptr, *button_ptr, *dark_check_ptr,
                      *mute_ptr, *volume_ptr,
                      dark_check_ptr->is_checked());
        refresh_status();
    });

    // ---- playit stack ----------------------------------------------

    lpit::StdioTransport transport;
    lpit::Dispatcher     dispatcher{root};
    lpit::EventRecorder  recorder;
    lpit::Executor       executor{transport, dispatcher};
    executor.set_recorder(&recorder);

    // PLAYIT-04a — gesture pipeline turns raw Pointer{Down,Up,Move}
    // into PressDown/PressRelease/DoubleTap. 60Hz matches SDL VSYNC.
    lpit::GesturePipeline pipeline{60};

    auto dispatch_pipeline_output = [&](const lpit::PipelineOutput& out) {
        if (out.primary)   (void)root.dispatch_event(*out.primary);
        if (out.secondary) (void)root.dispatch_event(*out.secondary);
    };

    std::uint32_t tick_counter    = 0;
    std::uint32_t present_counter = 0;

    // Snapshot widget state each frame to detect changes; the
    // toggles + slider don't fire callbacks (parity with rlvgl), so
    // we observe state and refresh the status label when anything
    // moves.
    auto snapshot = std::make_tuple(dark_check_ptr->is_checked(),
                                     mute_ptr->is_on(),
                                     volume_ptr->value());

    while (!backend.quit_requested()) {
        // SDL events through the recogniser.
        while (auto event_opt = backend.poll_event()) {
            if (is_quit_key(*event_opt)) {
                return EXIT_SUCCESS;
            }
            dispatch_pipeline_output(pipeline.process(*event_opt));
        }

        // playit (stdio) — external commands.
        dispatcher.set_status_snapshot(
            lpit::StatusData{tick_counter, present_counter});
        (void)executor.poll();

        if (transport.is_eof()) {
            return EXIT_SUCCESS;
        }

        // Refresh on widget-state change. Apply theme too, since
        // ticking the Checkbox or Button both flip the palette.
        const auto fresh = std::make_tuple(dark_check_ptr->is_checked(),
                                            mute_ptr->is_on(),
                                            volume_ptr->value());
        if (fresh != snapshot) {
            // Sync theme from checkbox state.
            apply_palette(*status_ptr, *button_ptr, *dark_check_ptr,
                          *mute_ptr, *volume_ptr,
                          dark_check_ptr->is_checked());
            refresh_status();
            snapshot = fresh;
        }

        backend.clear(lc::Color{16, 16, 24, 255});
        root.draw(backend.renderer());
        backend.present_frame();

        dispatch_pipeline_output(pipeline.tick());
        recorder.tick();

        ++tick_counter;
        ++present_counter;
    }
    return EXIT_SUCCESS;
}
