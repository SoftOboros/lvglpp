// disco_controller.cpp — DiscoController + ControllerState + FSM.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/lib.rs (v0.2.0 @ 79f730d)
//         — DiscoController (:820), ControllerState (:285) and every
//         internal method (cycle_main_focus :552, cycle_wing_focus :563,
//         activate_main :579, activate_settings :597, activate_info :649,
//         close_wings :479, handle_key :774, render_info_page :690,
//         push_status :351, sync_focus_highlights :533, show_home :405,
//         show_about :358, show_info :471, show_storage :449).
// LVGL:   N/A (app controller).
// DELTA:  Rc<RefCell> graph collapses to a single-owner tree + raw
//         observing Widget* in ControllerState (DEMO-00 §5). The
//         RefCell-contention `focus_dirty` retry path is preserved as a
//         field but is always clear (no interior-mutability contention in
//         the single-owner model). settings_wing / info_wing / icon_strip
//         carry tags (rlvgl uses None) so the SDL-free parity tests can
//         reach focused_slot() through find_by_tag.
//
// docs/disco-demo/06-controller-and-host-target.md (DEMO-06) §3/§5.

#include "lvglpp/app/disco_demo/disco_controller.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "lvglpp/app/disco_demo/assets.hpp"
#include "lvglpp/app/disco_demo/dashboard_panel.hpp"
#include "lvglpp/app/disco_demo/hotspot.hpp"
#include "lvglpp/app/disco_demo/icon_strip.hpp"
#include "lvglpp/app/disco_demo/wing.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/fonts/font_6x10.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"
#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/ui/event_window.hpp"
#include "lvglpp/widgets/container.hpp"
#include "lvglpp/widgets/label.hpp"

namespace lvglpp::app::disco_demo {

namespace lc = ::lvglpp::core;
namespace lw = ::lvglpp::widgets;
namespace lu = ::lvglpp::ui;

// Controller-internal slot enums (mirror lib.rs:170-243). Named-namespace
// types so they may appear in ControllerState member signatures. FROZEN —
// ordinals must match rlvgl (Standards Action, DEMO-06 §2).
enum class MainSlot : std::size_t { Settings = 0, Files = 1, Info = 2 };
enum class SettingsSlot : std::size_t {
    Audio = 0,
    Camera = 1,
    Display = 2,
    Locale = 3,
    Backlight = 4,
    About = 5,
};
enum class InfoSlot : std::size_t {
    Diagnostics = 0,
    LiveStats = 1,
    StarCrawl = 2,
    AudioScope = 3,
};

namespace {

MainSlot main_slot_from_index(std::size_t index) noexcept {
    switch (index) {
        case 1:
            return MainSlot::Files;
        case 2:
            return MainSlot::Info;
        default:
            return MainSlot::Settings;
    }
}

std::string_view main_slot_label(MainSlot slot) noexcept {
    switch (slot) {
        case MainSlot::Files:
            return "Storage";
        case MainSlot::Info:
            return "Info";
        case MainSlot::Settings:
        default:
            return "Settings";
    }
}

SettingsSlot settings_slot_from_index(std::size_t index) noexcept {
    switch (index) {
        case 1:
            return SettingsSlot::Camera;
        case 2:
            return SettingsSlot::Display;
        case 3:
            return SettingsSlot::Locale;
        case 4:
            return SettingsSlot::Backlight;
        case 5:
            return SettingsSlot::About;
        default:
            return SettingsSlot::Audio;
    }
}

InfoSlot info_slot_from_index(std::size_t index) noexcept {
    switch (index) {
        case 1:
            return InfoSlot::LiveStats;
        case 2:
            return InfoSlot::StarCrawl;
        case 3:
            return InfoSlot::AudioScope;
        default:
            return InfoSlot::Diagnostics;
    }
}

std::string_view yes_no(bool value) noexcept { return value ? "yes" : "no"; }

// Euclidean remainder (matches Rust's rem_euclid) for focus wrap-around.
std::size_t rem_euclid(std::int32_t value, std::int32_t modulus) noexcept {
    std::int32_t r = value % modulus;
    if (r < 0) {
        r += modulus;
    }
    return static_cast<std::size_t>(r);
}

std::string pad2(std::uint64_t n) {
    std::string s = std::to_string(n);
    return s.size() < 2 ? "0" + s : s;
}

// Hotspot bounds for main-strip slot `index` (mirror lib.rs:245).
lc::Rect strip_slot_bounds(std::int32_t display_width, std::size_t index) noexcept {
    const std::int32_t step = STRIP_ICON_SIZE + STRIP_GAP;
    const std::int32_t idx = static_cast<std::int32_t>(index);
    const std::int32_t top =
        index == 0 ? 0 : STRIP_MARGIN_TOP + idx * step - STRIP_GAP / 2;
    const std::int32_t bottom =
        index == SLOT_COUNT - 1
            ? STRIP_MARGIN_TOP + static_cast<std::int32_t>(SLOT_COUNT) * step
            : STRIP_MARGIN_TOP + (idx + 1) * step - STRIP_GAP / 2;
    return lc::Rect{display_width - STRIP_X_OFFSET, top, STRIP_ICON_SIZE,
                    bottom - top};
}

// Hotspot bounds for wing slot `index` of a wing with `slot_count` visible
// slots (mirror lib.rs:265).
lc::Rect wing_slot_bounds(std::size_t index, std::size_t slot_count) noexcept {
    const std::int32_t step = WING_ICON_SIZE + WING_GAP;
    const std::int32_t idx = static_cast<std::int32_t>(index);
    const std::int32_t top =
        index == 0 ? 0 : WING_MARGIN_TOP + idx * step - WING_GAP / 2;
    const std::int32_t bottom =
        index + 1 == slot_count
            ? WING_MARGIN_TOP + static_cast<std::int32_t>(slot_count) * step
            : WING_MARGIN_TOP + (idx + 1) * step - WING_GAP / 2;
    return lc::Rect{WING_X, top, WING_ICON_SIZE, bottom - top};
}

// Build a transparent-background themed label (mirror lib.rs:1305).
std::unique_ptr<lw::Label> make_themed_label(std::string text, lc::Rect bounds,
                                             lc::Color text_color) {
    auto label = std::make_unique<lw::Label>(std::move(text), bounds);
    label->style = lc::StyleBuilder{}.bg_color(lc::Color{0, 0, 0, 0}).alpha(0).build();
    label->text_color = text_color;
    return label;
}

}  // namespace

// ---------------------------------------------------------------------------
// ControllerState — controller-internal mutable state (mirror lib.rs:285).
//
// Every widget pointer below is an observer:
//   observes; owned by the widget tree (DiscoController::root_); valid for
//   the controller's lifetime. Never freed here. These are concrete
//   Widget-subclass pointers (never WidgetNode*) so they stay valid across
//   tree-vector reallocation and controller moves (DEMO-00 §5 O-4).
// ---------------------------------------------------------------------------
struct ControllerState {
    DiscoCapabilities         capabilities{};
    // owns: the pending command queue (drained each frame).
    std::vector<DiscoCommand> commands{};
    DashboardPanel*           dashboard     = nullptr;  // observes (O-3)
    lw::Label*                subtitle      = nullptr;  // observes (O-3)
    lw::Label*                footer        = nullptr;  // observes (O-3)
    lu::EventWindow*          event_window  = nullptr;  // observes (O-3)
    IconStrip*                icon_strip    = nullptr;  // observes (O-3)
    Wing*                     settings_wing = nullptr;  // observes (O-3)
    Wing*                     info_wing     = nullptr;  // observes (O-3)
    FocusState                focus         = FocusState::main(0);
    std::uint64_t             tick_count    = 0;
    std::uint8_t              backlight     = 75;
    bool                      focus_dirty   = false;
    std::optional<InfoSlot>   active_info{};

    // ---- small mutators ---------------------------------------------------

    void set_subtitle(std::string text) { subtitle->set_text(std::move(text)); }
    void set_footer(std::string text) { footer->set_text(std::move(text)); }
    void queue(DiscoCommand command) { commands.push_back(std::move(command)); }

    // footer + event window + queue ShowStatus (mirror lib.rs:351).
    void push_status(std::string text) {
        set_footer(text);
        event_window->push_event(text);
        queue(cmd::ShowStatus{std::move(text)});
    }

    // ---- dashboard content (mirror lib.rs:358-477) ------------------------

    void show_about() {
        active_info = std::nullopt;
        dashboard->show();
        dashboard->set_title("About");
        dashboard->set_caption("rlvgl demo application");
        dashboard->set_accent(lc::Color{0x58, 0xB3, 0xF5, 0xFF});
        std::vector<std::string> lines;
        lines.push_back("Platform: " + std::string{capabilities.platform});
        lines.push_back("Audio: " + std::string{yes_no(capabilities.audio)} +
                        "  Storage: " + std::string{yes_no(capabilities.storage)} +
                        "  Effects: " + std::string{yes_no(capabilities.effects)});
        lines.push_back("Pointer: " + std::string{yes_no(capabilities.pointer)});
        lines.push_back("Diagnostics: " +
                        std::string{yes_no(capabilities.diagnostics)});
        lines.push_back("Backlight: " + std::to_string(static_cast<unsigned>(backlight)) +
                        "%");
        dashboard->set_lines(std::span<const std::string>(lines));
        push_status("About");
    }

    void show_home() {
        active_info = std::nullopt;
        dashboard->show();
        dashboard->set_title("About");
        dashboard->set_caption("rlvgl demo application");
        dashboard->set_accent(lc::Color{0x58, 0xB3, 0xF5, 0xFF});
        std::vector<std::string> lines;
        lines.push_back(
            "Use the right strip to open settings, storage, and info wings.");
        lines.push_back(
            "Arrow keys move focus, Enter activates, Escape closes a wing.");
        lines.push_back("Pointer input: " + std::string{yes_no(capabilities.pointer)} +
                        "  Storage: " + std::string{yes_no(capabilities.storage)} +
                        "  Effects: " + std::string{yes_no(capabilities.effects)});
        lines.push_back("Audio: " + std::string{yes_no(capabilities.audio)} +
                        "  Diagnostics: " +
                        std::string{yes_no(capabilities.diagnostics)} +
                        "  Backlight: " +
                        std::to_string(static_cast<unsigned>(backlight)) + "%");
        dashboard->set_lines(std::span<const std::string>(lines));
    }

    // Defined for full parity (mirror lib.rs:449); rlvgl never calls it
    // either — the Files flow queues LoadStorageSummary instead.
    void show_storage() {
        dashboard->show();
        dashboard->set_title("Storage Browser");
        dashboard->set_caption("Shared file-browser placeholder");
        dashboard->set_accent(lc::Color{0x7A, 0xD6, 0x8A, 0xFF});
        std::vector<std::string> lines;
        lines.push_back(
            "The shared controller reuses the 747 icon flow and layout.");
        lines.push_back(
            "Runtime adapters can replace these lines with real media summaries.");
        if (capabilities.storage) {
            lines.push_back("Request queued: refresh storage summary");
            lines.push_back("Mock sources: onboard flash, SD card, host assets");
        } else {
            lines.push_back("Storage is disabled on this platform.");
        }
        dashboard->set_lines(std::span<const std::string>(lines));
    }

    void show_info(std::string title, std::string caption, lc::Color accent,
                   const std::vector<std::string>& lines) {
        dashboard->show();
        dashboard->set_title(std::move(title));
        dashboard->set_caption(std::move(caption));
        dashboard->set_accent(accent);
        dashboard->set_lines(std::span<const std::string>(lines));
    }

    // ---- focus mechanics (mirror lib.rs:479-577) --------------------------

    void close_wings() {
        active_info = std::nullopt;
        dashboard->hide();
        settings_wing->close();
        info_wing->close();
        const std::size_t focus_index = focus.index;
        focus = FocusState::main(std::min<std::size_t>(focus_index, 2));
        refresh_focus_hint();
        sync_focus_highlights();
    }

    void open_settings() {
        info_wing->close();
        (void)settings_wing->toggle_visible();
        focus = settings_wing->is_visible()
                    ? FocusState::wing_at(WingKind::Settings, 0)
                    : FocusState::main(static_cast<std::size_t>(MainSlot::Settings));
        refresh_focus_hint();
        sync_focus_highlights();
    }

    void open_info() {
        settings_wing->close();
        (void)info_wing->toggle_visible();
        focus = info_wing->is_visible()
                    ? FocusState::wing_at(WingKind::Info, 0)
                    : FocusState::main(static_cast<std::size_t>(MainSlot::Info));
        refresh_focus_hint();
        sync_focus_highlights();
    }

    void refresh_focus_hint() {
        std::string text;
        switch (focus.tag) {
            case FocusState::Tag::Main:
                text = "Focus: main " +
                       std::string{main_slot_label(main_slot_from_index(focus.index))};
                break;
            case FocusState::Tag::Wing:
                if (focus.wing == WingKind::Settings) {
                    text = "Focus: settings wing item " +
                           std::to_string(focus.index + 1U);
                } else {
                    text = "Focus: info wing item " + std::to_string(focus.index + 1U);
                }
                break;
        }
        set_subtitle(std::move(text));
    }

    void sync_focus_highlights() {
        std::optional<std::size_t> strip_slot{};
        std::optional<std::size_t> settings_slot{};
        std::optional<std::size_t> info_slot{};
        switch (focus.tag) {
            case FocusState::Tag::Main:
                strip_slot = focus.index;
                break;
            case FocusState::Tag::Wing:
                if (focus.wing == WingKind::Settings) {
                    settings_slot = focus.index;
                } else {
                    info_slot = focus.index;
                }
                break;
        }
        icon_strip->set_focused_slot(strip_slot);
        settings_wing->set_focused_slot(settings_slot);
        info_wing->set_focused_slot(info_slot);
        focus_dirty = false;
    }

    void cycle_main_focus(std::int32_t delta) {
        if (focus.tag != FocusState::Tag::Main) {
            return;
        }
        const std::int32_t current = static_cast<std::int32_t>(focus.index);
        focus = FocusState::main(rem_euclid(current + delta, 3));
        refresh_focus_hint();
        sync_focus_highlights();
    }

    void cycle_wing_focus(std::int32_t delta) {
        if (focus.tag == FocusState::Tag::Wing) {
            const std::int32_t current = static_cast<std::int32_t>(focus.index);
            const std::int32_t modulus = focus.wing == WingKind::Settings ? 6 : 4;
            focus = FocusState::wing_at(focus.wing, rem_euclid(current + delta, modulus));
        }
        refresh_focus_hint();
        sync_focus_highlights();
    }

    // ---- activation (mirror lib.rs:579-684) -------------------------------

    void activate_main(MainSlot slot) {
        focus = FocusState::main(static_cast<std::size_t>(slot));
        sync_focus_highlights();
        switch (slot) {
            case MainSlot::Settings:
                open_settings();
                break;
            case MainSlot::Files:
                close_wings();
                if (capabilities.storage) {
                    queue(cmd::LoadStorageSummary{});
                    push_status("Storage");
                } else {
                    push_status("Storage: not available");
                }
                break;
            case MainSlot::Info:
                open_info();
                break;
        }
    }

    void activate_settings(SettingsSlot slot) {
        active_info = std::nullopt;
        focus = FocusState::wing_at(WingKind::Settings, static_cast<std::size_t>(slot));
        sync_focus_highlights();
        switch (slot) {
            case SettingsSlot::Audio:
                if (capabilities.audio) {
                    push_status("Queued audio scope effect");
                    queue(cmd::StartEffect{DiscoEffect::AudioScope});
                } else {
                    push_status("Audio scope is unavailable on this platform");
                    queue(cmd::NoOp{});
                }
                break;
            case SettingsSlot::Camera:
                push_status("Camera: not available");
                break;
            case SettingsSlot::Display:
                push_status("Display: platform-managed");
                break;
            case SettingsSlot::Locale:
                push_status("Pointer: " +
                            std::string{capabilities.pointer ? "on" : "off"} +
                            " | Diag: " +
                            std::string{capabilities.diagnostics ? "on" : "off"});
                break;
            case SettingsSlot::Backlight:
                switch (backlight) {
                    case 100:
                        backlight = 25;
                        break;
                    case 75:
                        backlight = 100;
                        break;
                    case 50:
                        backlight = 75;
                        break;
                    case 25:
                        backlight = 50;
                        break;
                    default:
                        backlight = 75;
                        break;
                }
                push_status("Backlight " + std::to_string(static_cast<unsigned>(backlight)) +
                            "%");
                queue(cmd::SetBacklight{backlight});
                break;
            case SettingsSlot::About:
                show_about();
                break;
        }
    }

    void activate_info(InfoSlot slot) {
        focus = FocusState::wing_at(WingKind::Info, static_cast<std::size_t>(slot));
        sync_focus_highlights();
        switch (slot) {
            case InfoSlot::Diagnostics:
                active_info = slot;
                render_info_page(slot);
                push_status("Diagnostics page opened");
                break;
            case InfoSlot::LiveStats:
                active_info = slot;
                render_info_page(slot);
                push_status("Live stats panel refreshed");
                break;
            case InfoSlot::StarCrawl:
                active_info = std::nullopt;
                if (capabilities.effects) {
                    push_status("Queued star crawl effect");
                    queue(cmd::StartEffect{DiscoEffect::StarCrawl});
                } else {
                    push_status("Star crawl is unavailable on this platform");
                    queue(cmd::NoOp{});
                }
                break;
            case InfoSlot::AudioScope:
                active_info = std::nullopt;
                if (capabilities.audio) {
                    push_status("Queued audio scope effect");
                    queue(cmd::StartEffect{DiscoEffect::AudioScope});
                } else {
                    push_status("Audio scope is unavailable on this platform");
                    queue(cmd::NoOp{});
                }
                break;
        }
    }

    // ---- info-page rendering (mirror lib.rs:690-772) ----------------------

    void render_info_page(InfoSlot slot) {
        switch (slot) {
            case InfoSlot::Diagnostics:
                show_info("Diagnostics", "STM32H747XIH6 — simulated telemetry",
                          lc::Color{0xF2, 0x85, 0x85, 0xFF}, mock_diagnostics_lines());
                break;
            case InfoSlot::LiveStats:
                show_info("Live Stats", "Frame-rate ticker + mock hw counters",
                          lc::Color{0x58, 0xB3, 0xF5, 0xFF}, mock_live_stats_lines());
                break;
            case InfoSlot::StarCrawl:
            case InfoSlot::AudioScope:
                // Effects don't render a page; intentionally no-op.
                break;
        }
    }

    [[nodiscard]] std::vector<std::string> mock_diagnostics_lines() const {
        std::vector<std::string> lines;
        lines.push_back("MCU: STM32H747XIH6  (rev Y)");
        lines.push_back("SYSCLK: 400 MHz    HCLK: 200 MHz");
        lines.push_back("Flash: 2048 KB     SRAM: 1024 KB");
        lines.push_back("Diagnostics: " + std::string{yes_no(capabilities.diagnostics)} +
                        "   Audio: " + std::string{yes_no(capabilities.audio)});
        lines.push_back("Storage: " + std::string{yes_no(capabilities.storage)} +
                        "       Effects: " + std::string{yes_no(capabilities.effects)});
        lines.push_back("LTDC: 800x480 ARGB8888  @ 60 Hz");
        lines.push_back("Bus: AXI 200 MHz  QoS: high");
        return lines;
    }

    [[nodiscard]] std::vector<std::string> mock_live_stats_lines() const {
        constexpr std::uint64_t kTickHz = 60;
        const std::uint64_t seconds_whole = tick_count / kTickHz;
        const std::uint64_t seconds_frac = (tick_count % kTickHz) * 100ULL / kTickHz;
        const std::uint32_t heap_used_kb =
            48U + static_cast<std::uint32_t>((tick_count / 30ULL) % 16ULL);
        const std::uint32_t heap_total_kb = 256U;
        const std::uint32_t heap_pct = heap_used_kb * 100U / heap_total_kb;
        const std::uint64_t dma_frames = tick_count / 2ULL;
        std::vector<std::string> lines;
        lines.push_back("Uptime: " + std::to_string(seconds_whole) + "." +
                        pad2(seconds_frac) + " s");
        lines.push_back("Ticks: " + std::to_string(tick_count));
        lines.push_back("FPS: " + std::to_string(kTickHz) + "   Frame: " +
                        std::to_string(1000ULL / kTickHz) + " ms");
        lines.push_back("Heap: " + std::to_string(heap_used_kb) + "/" +
                        std::to_string(heap_total_kb) + " KB  (" +
                        std::to_string(heap_pct) + "%)");
        lines.push_back("DMA2D frames: " + std::to_string(dma_frames));
        lines.push_back("Backlight: " + std::to_string(static_cast<unsigned>(backlight)) +
                        "%");
        std::string focus_str;
        switch (focus.tag) {
            case FocusState::Tag::Main:
                focus_str = "main[" + std::to_string(focus.index) + "]";
                break;
            case FocusState::Tag::Wing:
                focus_str = (focus.wing == WingKind::Settings ? "settings[" : "info[") +
                            std::to_string(focus.index) + "]";
                break;
        }
        lines.push_back("Focus: " + focus_str);
        return lines;
    }

    // ---- key handling (mirror lib.rs:774) ---------------------------------

    void handle_key(const lc::Key& key) {
        using namespace lc::key;
        if (std::holds_alternative<ArrowUp>(key)) {
            if (focus.is_wing()) {
                cycle_wing_focus(-1);
            } else {
                cycle_main_focus(-1);
            }
        } else if (std::holds_alternative<ArrowDown>(key)) {
            if (focus.is_wing()) {
                cycle_wing_focus(1);
            } else {
                cycle_main_focus(1);
            }
        } else if (std::holds_alternative<ArrowLeft>(key)) {
            if (focus.is_wing()) {
                close_wings();
            } else {
                cycle_main_focus(-1);
            }
        } else if (std::holds_alternative<ArrowRight>(key)) {
            if (focus.is_wing()) {
                close_wings();
            } else {
                cycle_main_focus(1);
            }
        } else if (std::holds_alternative<Enter>(key) ||
                   std::holds_alternative<Space>(key)) {
            switch (focus.tag) {
                case FocusState::Tag::Main:
                    activate_main(main_slot_from_index(focus.index));
                    break;
                case FocusState::Tag::Wing:
                    if (focus.wing == WingKind::Settings) {
                        activate_settings(settings_slot_from_index(focus.index));
                    } else {
                        activate_info(info_slot_from_index(focus.index));
                    }
                    break;
            }
        } else if (std::holds_alternative<Escape>(key)) {
            close_wings();
        } else if (const auto* ch = std::get_if<Character>(&key)) {
            switch (ch->codepoint) {
                case 's':
                case 'S':
                    activate_main(MainSlot::Settings);
                    break;
                case 'f':
                case 'F':
                    activate_main(MainSlot::Files);
                    break;
                case 'i':
                case 'I':
                    activate_main(MainSlot::Info);
                    break;
                case 'b':
                case 'B':
                    activate_settings(SettingsSlot::Backlight);
                    break;
                default:
                    break;
            }
        }
    }
};

// ---------------------------------------------------------------------------
// DiscoController (mirror lib.rs:825).
// ---------------------------------------------------------------------------

DiscoController::DiscoController(core::WidgetNode&& root,
                                std::unique_ptr<ControllerState> state) noexcept
    : root_{std::move(root)}, state_{std::move(state)} {}

DiscoController::DiscoController(DiscoController&&) noexcept = default;
DiscoController& DiscoController::operator=(DiscoController&&) noexcept = default;
DiscoController::~DiscoController() = default;

DiscoController DiscoController::make(platform::Screen screen,
                                     DiscoCapabilities caps) {
    const std::int32_t logical_w = static_cast<std::int32_t>(screen.width);
    const std::int32_t logical_h = static_cast<std::int32_t>(screen.height);
    const std::int32_t width = logical_w == 0 ? DISPLAY_WIDTH : logical_w;
    const std::int32_t height = logical_h == 0 ? DISPLAY_HEIGHT : logical_h;

    // ---- 1. Build every widget; root container is transparent ------------
    auto root_container = std::make_unique<lw::Container>(lc::Rect{0, 0, width, height});
    root_container->style =
        lc::StyleBuilder{}.bg_color(lc::Color{0, 0, 0, 0}).alpha(0).build();
    core::WidgetNode root{std::move(root_container), "disco.root"};

    auto title = make_themed_label("STM32H747I-DISCO Runtime",
                                   lc::Rect{84, 24, 420, 18},
                                   lc::Color{248, 249, 250, 255});
    auto subtitle = make_themed_label("Focus: main Settings",
                                      lc::Rect{84, 48, 420, 18},
                                      lc::Color{148, 162, 184, 255});
    auto footer = make_themed_label("Ready", lc::Rect{84, height - 32, 620, 18},
                                    lc::Color{192, 203, 215, 255});
    lw::Label* subtitle_ptr = subtitle.get();
    lw::Label* footer_ptr = footer.get();

    auto dashboard = std::make_unique<DashboardPanel>(
        lc::Rect{std::min(PANEL_X, width - PANEL_WIDTH - 12),
                 std::min(PANEL_Y, height - PANEL_HEIGHT - 24),
                 std::min(PANEL_WIDTH, width - 120),
                 std::min(PANEL_HEIGHT, height - 120)},
        "About", "rlvgl demo application");
    DashboardPanel* dashboard_ptr = dashboard.get();

    auto event_window = std::make_unique<lu::EventWindow>(
        lu::EventWindowBuilder{core::fonts::FONT_6X10}
            .width(420)
            .center(width, height)
            .expire_ticks(180U)
            .build());
    lu::EventWindow* event_window_ptr = event_window.get();

    using IconPair = std::pair<std::span<const std::uint8_t>, bool>;
    const std::array<IconPair, 6> settings_icons{{
        {icon_audio48(), caps.audio},
        {icon_camera48(), false},
        {icon_monitor48(), true},
        {icon_globe48(), true},
        {icon_bug48(), true},
        {icon_info(), true},  // About slot reuses ICON_INFO (lib.rs:915).
    }};
    auto settings_wing =
        std::make_unique<Wing>(std::span<const IconPair>(settings_icons));
    Wing* settings_wing_ptr = settings_wing.get();

    const std::array<IconPair, 4> info_icons{{
        {icon_cpu48(), true},
        {icon_monitor48(), true},
        {icon_play48(), caps.effects},
        {icon_audio48(), caps.audio},
    }};
    auto info_wing = std::make_unique<Wing>(std::span<const IconPair>(info_icons));
    Wing* info_wing_ptr = info_wing.get();

    auto icon_strip = std::make_unique<IconStrip>(
        width - STRIP_X_OFFSET, STRIP_ICON_SIZE, STRIP_MARGIN_TOP, STRIP_GAP);
    IconStrip* icon_strip_ptr = icon_strip.get();

    // ---- 2/3. Capture observers, construct ControllerState ----------------
    auto state = std::make_unique<ControllerState>();
    state->capabilities = caps;
    state->dashboard = dashboard_ptr;
    state->subtitle = subtitle_ptr;
    state->footer = footer_ptr;
    state->event_window = event_window_ptr;
    state->icon_strip = icon_strip_ptr;
    state->settings_wing = settings_wing_ptr;
    state->info_wing = info_wing_ptr;
    state->focus = FocusState::main(0);
    state->sync_focus_highlights();  // mirror ControllerState::new (lib.rs:335)
    // observes the controller state; valid for the controller's lifetime
    // (state_ outlives the tree that owns the widgets holding these
    // callbacks, DEMO-00 §5 O-5).
    ControllerState* st = state.get();

    // ---- 4. Bind tap / visibility callbacks (capture st / const Wing*) ----
    // IconSlot on_tap: capture ControllerState* (observes) — never owning.
    icon_strip_ptr->set_slot(
        0, IconSlot{icon_settings(), true,
                    [st](std::size_t) { st->activate_main(MainSlot::Settings); }});
    icon_strip_ptr->set_slot(
        1, IconSlot{icon_file(), true,
                    [st](std::size_t) { st->activate_main(MainSlot::Files); }});
    icon_strip_ptr->set_slot(
        2, IconSlot{icon_info(), true,
                    [st](std::size_t) { st->activate_main(MainSlot::Info); }});

    for (std::size_t i = 0; i < 6; ++i) {
        settings_wing_ptr->slots_mut()[i]->on_tap = [st](std::size_t slot) {
            st->activate_settings(settings_slot_from_index(slot));
        };
    }
    for (std::size_t i = 0; i < 4; ++i) {
        info_wing_ptr->slots_mut()[i]->on_tap = [st](std::size_t slot) {
            st->activate_info(info_slot_from_index(slot));
        };
    }

    // ---- 1 (cont). Move widgets into the tree (mirror lib.rs:1020) --------
    root.add_child(core::WidgetNode{std::move(title)});
    root.add_child(core::WidgetNode{std::move(subtitle), "disco.subtitle"});
    root.add_child(core::WidgetNode{std::move(dashboard), "disco.dashboard"});
    root.add_child(core::WidgetNode{std::move(footer), "disco.footer"});
    root.add_child(core::WidgetNode{std::move(event_window), "disco.events"});
    root.add_child(core::WidgetNode{std::move(settings_wing), "disco.settings"});
    root.add_child(core::WidgetNode{std::move(info_wing), "disco.info"});
    root.add_child(core::WidgetNode{std::move(icon_strip), "disco.strip"});

    // const Wing* observers for the hotspot visibility predicates (O-5).
    const Wing* settings_view = settings_wing_ptr;
    const Wing* info_view = info_wing_ptr;

    // Main-strip hotspots (always visible).
    {
        auto hs = std::make_unique<ActionHotspot>(
            strip_slot_bounds(width, 0),
            [st] { st->activate_main(MainSlot::Settings); });
        root.add_child(core::WidgetNode{std::move(hs), "disco.main.settings"});
    }
    {
        auto hs = std::make_unique<ActionHotspot>(
            strip_slot_bounds(width, 1),
            [st] { st->activate_main(MainSlot::Files); });
        root.add_child(core::WidgetNode{std::move(hs), "disco.main.files"});
    }
    {
        auto hs = std::make_unique<ActionHotspot>(
            strip_slot_bounds(width, 2),
            [st] { st->activate_main(MainSlot::Info); });
        root.add_child(core::WidgetNode{std::move(hs), "disco.main.info"});
    }

    // Settings-wing hotspots (visible only while the settings wing is open;
    // slot_count=5 in lib.rs even though the wing has 6 slots).
    const auto add_settings_hotspot = [&](std::size_t idx, SettingsSlot slot,
                                          std::string_view tag) {
        auto hs = std::make_unique<ActionHotspot>(
            wing_slot_bounds(idx, 5),
            [st, slot] { st->activate_settings(slot); });
        hs->with_visibility([settings_view] { return settings_view->is_visible(); });
        root.add_child(core::WidgetNode{std::move(hs), tag});
    };
    add_settings_hotspot(0, SettingsSlot::Audio, "disco.settings.audio");
    add_settings_hotspot(1, SettingsSlot::Camera, "disco.settings.camera");
    add_settings_hotspot(2, SettingsSlot::Display, "disco.settings.display");
    add_settings_hotspot(3, SettingsSlot::Locale, "disco.settings.locale");
    add_settings_hotspot(4, SettingsSlot::Backlight, "disco.settings.backlight");

    // Info-wing hotspots (slot_count=4).
    const auto add_info_hotspot = [&](std::size_t idx, InfoSlot slot,
                                      std::string_view tag) {
        auto hs = std::make_unique<ActionHotspot>(
            wing_slot_bounds(idx, 4), [st, slot] { st->activate_info(slot); });
        hs->with_visibility([info_view] { return info_view->is_visible(); });
        root.add_child(core::WidgetNode{std::move(hs), tag});
    };
    add_info_hotspot(0, InfoSlot::Diagnostics, "disco.info.diagnostics");
    add_info_hotspot(1, InfoSlot::LiveStats, "disco.info.live_stats");
    add_info_hotspot(2, InfoSlot::StarCrawl, "disco.info.star_crawl");
    add_info_hotspot(3, InfoSlot::AudioScope, "disco.info.audio_scope");

    // ---- 5. Initial content: home populated, dashboard hidden -------------
    st->show_home();
    st->dashboard->hide();
    st->set_footer("Ready: tap the strip or use arrows + Enter");

    return DiscoController{std::move(root), std::move(state)};
}

bool DiscoController::dispatch_event(const core::Event& event) {
    const bool consumed = root_.dispatch_event(event);
    handle_event(event);
    return consumed;
}

void DiscoController::handle_event(const core::Event& event) {
    ControllerState& st = *state_;
    if (st.focus_dirty) {
        st.sync_focus_highlights();
    }
    if (std::holds_alternative<core::event::Tick>(event)) {
        st.tick_count = st.tick_count + 1;  // wraps on overflow (uint64).
        if (st.tick_count % 600ULL == 0ULL) {
            st.set_footer("Ready: " + std::to_string(st.tick_count) +
                          " ticks | backlight " +
                          std::to_string(static_cast<unsigned>(st.backlight)) + "%");
        }
        if (st.active_info) {
            st.render_info_page(*st.active_info);
        }
    } else if (const auto* kd = std::get_if<core::event::KeyDown>(&event)) {
        st.handle_key(kd->key);
    } else if (const auto* pr = std::get_if<core::event::PressRelease>(&event)) {
        if (!st.capabilities.pointer) {
            st.push_status("Pointer input ignored at (" + std::to_string(pr->x) +
                           ", " + std::to_string(pr->y) + ")");
        }
    }
}

void DiscoController::tick() {
    const core::Event ev{core::event::Tick{}};
    (void)root_.dispatch_event(ev);
    handle_event(ev);
}

std::vector<DiscoCommand> DiscoController::drain_commands() {
    std::vector<DiscoCommand> out = std::move(state_->commands);
    state_->commands.clear();
    return out;
}

void DiscoController::publish_status(std::string text) {
    state_->push_status(std::move(text));
}

FocusState DiscoController::focus() const noexcept { return state_->focus; }

}  // namespace lvglpp::app::disco_demo
