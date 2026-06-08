// event_window.hpp — transient overlay listing recent input events.
//
// PARITY: rlvgl/ui/src/event_window.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (composite overlay).
// DELTA:  board-render telemetry deferred; see DEMO-03 §2.
//
// docs/disco-demo/03-event-window.md (DEMO-03).

#ifndef LVGLPP_UI_EVENT_WINDOW_HPP
#define LVGLPP_UI_EVENT_WINDOW_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/font.hpp"      // BitmapFont
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"    // Color, Rect, Widget

namespace lvglpp::ui {

// Default expiry if none specified (10 s at 6 Hz — legacy). FROZEN —
// mirrors rlvgl/ui/src/event_window.rs:19 (DEFAULT_EXPIRE_TICKS).
inline constexpr std::uint32_t kDefaultExpireTicks = 60;

// Maximum visible lines in the window. FROZEN — mirrors
// rlvgl/ui/src/event_window.rs:22 (MAX_LINES).
inline constexpr std::size_t kMaxLines = 10;

// Frames to keep clearing after hiding (double-buffer + 1 margin).
// FROZEN — mirrors rlvgl/ui/src/event_window.rs:25 (CLEAR_FRAMES).
inline constexpr std::uint8_t kClearFrames = 3;

// Themed overlay that displays recent input events, auto-expiring after
// a tick budget. Constructed via EventWindowBuilder.
//
// Ownership: a core::Widget owned by the WidgetNode tree (DEMO-00 §5
// O-1). It owns its std::vector of entries (value) and BORROWS the
// BitmapFont (`const core::BitmapFont&`, must outlive the window). It
// holds no LVGL handle. The controller observes it via a raw
// EventWindow* (O-3) to call push_event / toggle_visible.
class EventWindow final : public ::lvglpp::core::Widget {
public:
    // ---- App-relevant surface (mirror event_window.rs) ----------------

    // Push a pre-formatted event string into the display list. Appends
    // an entry (age 0), trims the front past kMaxLines so the oldest is
    // dropped, and sets the window visible. No-op when disabled.
    // Mirrors event_window.rs:173 (with the visibility delta below).
    //
    // Args:
    //   text: owns; moved into the entry list.
    void push_event(std::string text);

    [[nodiscard]] bool is_visible() const noexcept { return visible_; }

    // Toggle visibility. Only works when enabled. Mirrors :90.
    void toggle_visible() noexcept;

    // Hide the window (arms the clear countdown for framebuffer
    // cleanup). Mirrors :102.
    void hide() noexcept;

    // Enable or disable event collection. When disabled, push_event is a
    // no-op; disabling also hides. Mirrors :82.
    void set_enabled(bool val) noexcept;

    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    [[nodiscard]] std::size_t entry_count() const noexcept {
        return entries_.size();
    }

    // ---- Widget overrides ---------------------------------------------

    // Collapses to a zero rect when hidden so the compositor does not
    // reserve space for an invisible overlay. Mirrors :187 + the
    // hidden-collapse behavior frozen by DEMO-03 §2.
    [[nodiscard]] ::lvglpp::core::Rect bounds() const override;

    void draw(::lvglpp::core::Renderer& renderer) const override;

    // Tick ages every entry and drops those past expire_ticks; when the
    // list empties while visible, hides and arms the clear countdown.
    // Never consumes (returns false) for pointer / key events — the
    // close-button hit test is part of the deferred board surface.
    // Mirrors :238.
    [[nodiscard]] bool handle_event(
        const ::lvglpp::core::Event& event) override;

    // Returns the panel rect while the clear countdown is active and the
    // window is hidden, decrementing the countdown. Mirrors :268.
    [[nodiscard]] std::optional<::lvglpp::core::Rect> clear_region() override;

private:
    friend class EventWindowBuilder;

    // A single event log entry. Mirrors event_window.rs:28.
    struct EventEntry {
        std::string   text;
        std::uint32_t age = 0;
    };

    EventWindow() = default;

    ::lvglpp::core::Rect bounds_{};
    ::lvglpp::core::Color bg_color_{};
    ::lvglpp::core::Color border_color_{};
    ::lvglpp::core::Color text_color_{};
    std::uint8_t border_width_ = 2;
    std::uint8_t radius_       = 18;
    std::int32_t padding_      = 12;
    std::uint32_t expire_ticks_ = kDefaultExpireTicks;
    std::uint8_t clear_countdown_ = 0;
    bool visible_ = false;
    bool enabled_ = false;
    std::vector<EventEntry> entries_{};

    // borrows: glyph storage owned by the application; must outlive this
    // window. Mirrors rlvgl's &'static BitmapFont.
    const ::lvglpp::core::BitmapFont* font_ = nullptr;
};

// Builder for EventWindow with the dark-overlay theme. Mirrors
// rlvgl/ui/src/event_window.rs:279.
class EventWindowBuilder {
public:
    // Create a builder with default dark-overlay theme values. The
    // window is sized to hold kMaxLines of text at the font's scaled
    // line height. Mirrors :295.
    //
    // Args:
    //   font: borrows; must outlive every EventWindow built from this
    //         builder.
    explicit EventWindowBuilder(
        const ::lvglpp::core::BitmapFont& font) noexcept;

    EventWindowBuilder& expire_ticks(std::uint32_t ticks) noexcept;
    EventWindowBuilder& bg_color(::lvglpp::core::Color c) noexcept;
    EventWindowBuilder& border_color(::lvglpp::core::Color c) noexcept;
    EventWindowBuilder& radius(std::uint8_t r) noexcept;
    EventWindowBuilder& width(std::int32_t w) noexcept;
    EventWindowBuilder& center(std::int32_t screen_w,
                               std::int32_t screen_h) noexcept;

    [[nodiscard]] EventWindow build() const;

private:
    std::int32_t window_w_;
    std::int32_t window_h_;
    std::optional<std::int32_t> pos_x_{};
    std::optional<std::int32_t> pos_y_{};
    ::lvglpp::core::Color bg_color_{25, 25, 25, 255};
    ::lvglpp::core::Color border_color_{80, 80, 80, 255};
    ::lvglpp::core::Color text_color_{220, 220, 220, 255};
    std::uint8_t border_width_ = 2;
    std::uint8_t radius_       = 18;
    std::uint32_t expire_ticks_ = kDefaultExpireTicks;
    // borrows: see EventWindowBuilder ctor.
    const ::lvglpp::core::BitmapFont* font_;
};

}  // namespace lvglpp::ui

#endif  // LVGLPP_UI_EVENT_WINDOW_HPP
