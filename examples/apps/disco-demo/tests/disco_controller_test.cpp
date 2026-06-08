// disco_controller_test.cpp — DEMO-06 controller + FSM parity suite.
//
// SDL-free mirror of the rlvgl `#[cfg(test)]` suite in
// rlvgl/examples/apps/disco-demo/src/lib.rs (v0.2.0 @ 79f730d). Builds a
// DiscoController, feeds core::Event values, and asserts FSM state +
// drained commands + focus-highlight wiring + capability gating.
//
// docs/disco-demo/06-controller-and-host-target.md (DEMO-06) §7.

#include "lvglpp/app/disco_demo/disco_controller.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "lvglpp/app/disco_demo/capabilities.hpp"
#include "lvglpp/app/disco_demo/command.hpp"
#include "lvglpp/app/disco_demo/icon_strip.hpp"
#include "lvglpp/app/disco_demo/wing.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/platform/screen.hpp"
#include "lvglpp/ui/event_window.hpp"

namespace ad = lvglpp::app::disco_demo;
namespace lc = lvglpp::core;
namespace lp = lvglpp::platform;

namespace {

ad::DiscoController make_with(ad::DiscoCapabilities caps) {
    return ad::DiscoController::make(lp::Screen::landscape(800, 480), caps);
}
ad::DiscoController make_sim() { return make_with(ad::DiscoCapabilities::simulator()); }

void key(ad::DiscoController& c, lc::Key k) {
    (void)c.dispatch_event(lc::Event{lc::event::KeyDown{std::move(k)}});
}
lc::Key character(char ch) {
    return lc::Key{lc::key::Character{static_cast<std::uint32_t>(ch)}};
}

// ---- command predicates ---------------------------------------------------

template <typename T>
bool has(const std::vector<ad::DiscoCommand>& v) {
    for (const auto& c : v) {
        if (std::holds_alternative<T>(c)) return true;
    }
    return false;
}
bool has_set_backlight(const std::vector<ad::DiscoCommand>& v, std::uint8_t level) {
    for (const auto& c : v) {
        if (const auto* p = std::get_if<ad::cmd::SetBacklight>(&c)) {
            if (p->level == level) return true;
        }
    }
    return false;
}
bool has_start_effect(const std::vector<ad::DiscoCommand>& v, ad::DiscoEffect e) {
    for (const auto& c : v) {
        if (const auto* p = std::get_if<ad::cmd::StartEffect>(&c)) {
            if (p->effect == e) return true;
        }
    }
    return false;
}

// ---- widget reach-through (static_cast, no RTTI) --------------------------

template <typename W>
W* widget_at(ad::DiscoController& c, std::string_view tag) {
    const lc::WidgetNode* node = lc::find_by_tag(c.root(), tag);
    assert(node != nullptr && node->widget);
    // SAFETY: the controller built this node with a concrete W under this
    // tag; the tree is the sole owner, the pointer is non-owning.
    return static_cast<W*>(node->widget.get());
}
ad::IconStrip* strip(ad::DiscoController& c) {
    return widget_at<ad::IconStrip>(c, "disco.strip");
}
ad::Wing* settings_wing(ad::DiscoController& c) {
    return widget_at<ad::Wing>(c, "disco.settings");
}
ad::Wing* info_wing(ad::DiscoController& c) {
    return widget_at<ad::Wing>(c, "disco.info");
}
std::int32_t hotspot_width(ad::DiscoController& c, std::string_view tag) {
    const lc::WidgetNode* node = lc::find_by_tag(c.root(), tag);
    assert(node != nullptr && node->widget);
    return node->widget->bounds().width;
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void test_initial_state() {
    auto c = make_sim();
    assert(c.focus() == ad::FocusState::main(0));
    assert(strip(c)->focused_slot() == std::optional<std::size_t>{0});
    assert(!settings_wing(c)->focused_slot().has_value());
    assert(!info_wing(c)->focused_slot().has_value());
}

void test_arrow_down_cycles_main() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::ArrowDown{}});
    assert(c.focus() == ad::FocusState::main(1));
    key(c, lc::Key{lc::key::ArrowDown{}});
    assert(c.focus() == ad::FocusState::main(2));
    key(c, lc::Key{lc::key::ArrowDown{}});
    assert(c.focus() == ad::FocusState::main(0));
}

void test_arrow_up_wraps() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::ArrowUp{}});
    assert(c.focus() == ad::FocusState::main(2));
}

void test_enter_opens_wing_escape_closes() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::Enter{}});
    assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Settings, 0));
    assert(hotspot_width(c, "disco.settings.audio") > 0);
    key(c, lc::Key{lc::key::Escape{}});
    assert(!c.focus().is_wing());
    assert(hotspot_width(c, "disco.settings.audio") == 0);
}

void test_arrow_left_from_wing_returns_main() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::Enter{}});
    assert(c.focus().is_wing());
    key(c, lc::Key{lc::key::ArrowLeft{}});
    assert(!c.focus().is_wing());
}

void test_wing_focus_cycles_settings() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::Enter{}});
    assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Settings, 0));
    for (std::size_t i = 1; i < 6; ++i) {
        key(c, lc::Key{lc::key::ArrowDown{}});
        assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Settings, i));
    }
    key(c, lc::Key{lc::key::ArrowDown{}});
    assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Settings, 0));
}

void test_wing_focus_cycles_info() {
    auto c = make_sim();
    key(c, character('i'));
    assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Info, 0));
    for (std::size_t i = 1; i < 4; ++i) {
        key(c, lc::Key{lc::key::ArrowDown{}});
        assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Info, i));
    }
    key(c, lc::Key{lc::key::ArrowDown{}});
    assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Info, 0));
}

void test_opening_info_closes_settings() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::Enter{}});
    assert(hotspot_width(c, "disco.settings.audio") > 0);
    key(c, character('i'));
    assert(hotspot_width(c, "disco.settings.audio") == 0);
    assert(hotspot_width(c, "disco.info.diagnostics") > 0);
}

// ---------------------------------------------------------------------------
// Focus-highlight wiring
// ---------------------------------------------------------------------------

void test_focus_highlight_wiring() {
    // ArrowDown moves the strip highlight to the next slot.
    auto c1 = make_sim();
    key(c1, lc::Key{lc::key::ArrowDown{}});
    assert(strip(c1)->focused_slot() == std::optional<std::size_t>{1});

    // Enter on main(0)=Settings moves the highlight into the settings wing;
    // Escape restores it to the strip. Fresh controller (start on Settings).
    auto c2 = make_sim();
    key(c2, lc::Key{lc::key::Enter{}});
    assert(!strip(c2)->focused_slot().has_value());
    assert(settings_wing(c2)->focused_slot() == std::optional<std::size_t>{0});
    key(c2, lc::Key{lc::key::Escape{}});
    assert(strip(c2)->focused_slot().has_value());
    assert(!settings_wing(c2)->focused_slot().has_value());
}

// ---------------------------------------------------------------------------
// Hotkeys
// ---------------------------------------------------------------------------

void test_hotkey_s_activates_settings() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::ArrowDown{}});  // move off slot 0
    key(c, character('s'));
    assert(hotspot_width(c, "disco.settings.audio") > 0);
}

void test_hotkey_f_emits_storage_command() {
    auto c = make_sim();
    key(c, character('f'));
    assert(has<ad::cmd::LoadStorageSummary>(c.drain_commands()));
}

void test_hotkey_i_activates_info() {
    auto c = make_sim();
    key(c, character('i'));
    assert(hotspot_width(c, "disco.info.diagnostics") > 0);
}

void test_hotkey_b_cycles_backlight() {
    auto c = make_sim();  // default backlight is 75
    key(c, character('b'));
    assert(has_set_backlight(c.drain_commands(), 100));
    key(c, character('b'));
    assert(has_set_backlight(c.drain_commands(), 25));
    key(c, character('b'));
    assert(has_set_backlight(c.drain_commands(), 50));
    key(c, character('b'));
    assert(has_set_backlight(c.drain_commands(), 75));
}

// ---------------------------------------------------------------------------
// Command emission
// ---------------------------------------------------------------------------

void test_storage_from_main_strip_flow() {
    auto c = make_sim();
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::Enter{}});
    assert(has<ad::cmd::LoadStorageSummary>(c.drain_commands()));
}

void test_effect_command_for_enabled_platform() {
    auto c = make_with(ad::DiscoCapabilities::stm32h747i_disco());
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::Enter{}});  // open info wing
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::Enter{}});  // activate StarCrawl (index 2)
    assert(has_start_effect(c.drain_commands(), ad::DiscoEffect::StarCrawl));
}

void test_audio_capable_emits_start_effect() {
    auto c = make_with(ad::DiscoCapabilities::stm32h747i_disco());
    key(c, lc::Key{lc::key::Enter{}});  // open settings wing
    key(c, lc::Key{lc::key::Enter{}});  // activate audio (index 0)
    assert(has_start_effect(c.drain_commands(), ad::DiscoEffect::AudioScope));
}

void test_info_diagnostics_emits_show_status() {
    auto c = make_sim();
    key(c, character('i'));
    key(c, lc::Key{lc::key::Enter{}});  // activate diagnostics (index 0)
    assert(has<ad::cmd::ShowStatus>(c.drain_commands()));
}

void test_drain_clears_queue() {
    auto c = make_sim();
    key(c, character('f'));
    assert(!c.drain_commands().empty());
    assert(c.drain_commands().empty());
}

void test_publish_status_surfaces_everywhere() {
    auto c = make_sim();
    // The event window starts disabled (parity with rlvgl), so push_event is
    // a no-op until enabled. Enable it to prove publish_status routes to it.
    auto* events = widget_at<lvglpp::ui::EventWindow>(c, "disco.events");
    events->set_enabled(true);
    c.publish_status("hello world");
    assert(events->entry_count() >= 1);
    assert(has<ad::cmd::ShowStatus>(c.drain_commands()));
}

// ---------------------------------------------------------------------------
// Capability gating
// ---------------------------------------------------------------------------

void test_unsupported_audio_neutralizes() {
    auto c = make_with(ad::DiscoCapabilities::uefi());
    key(c, lc::Key{lc::key::Enter{}});  // open settings
    key(c, lc::Key{lc::key::Enter{}});  // activate audio — unavailable
    const auto cmds = c.drain_commands();
    assert(has<ad::cmd::NoOp>(cmds));
    assert(has<ad::cmd::ShowStatus>(cmds));
}

void test_effects_disabled_star_crawl_neutralizes() {
    // Custom caps: simulator-like but effects disabled.
    ad::DiscoCapabilities caps{/*audio=*/false, /*storage=*/true,
                               /*diagnostics=*/true, /*effects=*/false,
                               /*pointer=*/true, /*platform=*/"custom"};
    auto c = make_with(caps);
    key(c, character('i'));              // open info wing
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::ArrowDown{}});
    key(c, lc::Key{lc::key::Enter{}});  // activate StarCrawl (index 2)
    const auto cmds = c.drain_commands();
    assert(has<ad::cmd::NoOp>(cmds));
    assert(!has_start_effect(cmds, ad::DiscoEffect::StarCrawl));
}

void test_pointer_ignored_when_disabled() {
    auto c = make_with(ad::DiscoCapabilities::uefi());  // pointer=false
    (void)c.dispatch_event(lc::Event{lc::event::PressRelease{100, 100}});
    // Gated path queues an "ignored" status (ShowStatus).
    assert(has<ad::cmd::ShowStatus>(c.drain_commands()));
}

void test_pointer_enabled_no_ignore_status() {
    auto c = make_sim();  // pointer=true
    // A raw PressRelease is consumed by the first always-visible hotspot
    // (disco.main.settings) — hotspots fire on any PressRelease (bounds are
    // only used for tag-targeted dispatch), mirroring rlvgl. With pointer
    // enabled, the controller's PressRelease branch must NOT queue an
    // "ignored" status, so the only effect is the Settings activation.
    (void)c.dispatch_event(lc::Event{lc::event::PressRelease{5, 5}});
    assert(c.focus() == ad::FocusState::wing_at(ad::WingKind::Settings, 0));
    assert(!has<ad::cmd::ShowStatus>(c.drain_commands()));
}

// ---------------------------------------------------------------------------
// Construction / move / sizing
// ---------------------------------------------------------------------------

void test_required_tags_present() {
    const char* tags[] = {
        "disco.root",          "disco.dashboard",     "disco.subtitle",
        "disco.footer",        "disco.events",        "disco.main.settings",
        "disco.main.files",    "disco.main.info",     "disco.settings.audio",
        "disco.settings.camera", "disco.settings.display",
        "disco.settings.locale", "disco.settings.backlight",
        "disco.info.diagnostics", "disco.info.live_stats",
        "disco.info.star_crawl", "disco.info.audio_scope"};
    for (auto caps : {ad::DiscoCapabilities::simulator(),
                      ad::DiscoCapabilities::uefi(),
                      ad::DiscoCapabilities::stm32h747i_disco()}) {
        auto c = make_with(caps);
        for (const char* tag : tags) {
            assert(lc::find_by_tag(c.root(), tag) != nullptr);
        }
    }
}

void test_custom_display_size_respected() {
    auto c = ad::DiscoController::make(lp::Screen::landscape(1024, 600),
                                       ad::DiscoCapabilities::simulator());
    const lc::Rect b = c.root().widget->bounds();
    assert(b.width == 1024 && b.height == 600);
}

void test_move_only_preserves_observers() {
    auto c = make_sim();
    auto moved = std::move(c);
    key(moved, lc::Key{lc::key::ArrowDown{}});
    assert(moved.focus() == ad::FocusState::main(1));
    // Observers survived the move: focus highlight still wired.
    assert(strip(moved)->focused_slot() == std::optional<std::size_t>{1});
}

void test_tick_advances_without_panicking() {
    auto c = make_sim();
    for (int i = 0; i < 5; ++i) {
        c.tick();
    }
    // Tick consumes no commands here; drain stays empty.
    assert(c.drain_commands().empty());
}

}  // namespace

int main() {
    test_initial_state();
    test_arrow_down_cycles_main();
    test_arrow_up_wraps();
    test_enter_opens_wing_escape_closes();
    test_arrow_left_from_wing_returns_main();
    test_wing_focus_cycles_settings();
    test_wing_focus_cycles_info();
    test_opening_info_closes_settings();
    test_focus_highlight_wiring();
    test_hotkey_s_activates_settings();
    test_hotkey_f_emits_storage_command();
    test_hotkey_i_activates_info();
    test_hotkey_b_cycles_backlight();
    test_storage_from_main_strip_flow();
    test_effect_command_for_enabled_platform();
    test_audio_capable_emits_start_effect();
    test_info_diagnostics_emits_show_status();
    test_drain_clears_queue();
    test_publish_status_surfaces_everywhere();
    test_unsupported_audio_neutralizes();
    test_effects_disabled_star_crawl_neutralizes();
    test_pointer_ignored_when_disabled();
    test_pointer_enabled_no_ignore_status();
    test_required_tags_present();
    test_custom_display_size_respected();
    test_move_only_preserves_observers();
    test_tick_advances_without_panicking();
    return 0;
}
