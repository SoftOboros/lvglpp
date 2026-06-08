// event_window.cpp — EventWindow + EventWindowBuilder.
//
// PARITY: rlvgl/ui/src/event_window.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (composite overlay).
// DELTA:  board-render telemetry deferred; see DEMO-03 §2.
//
// docs/disco-demo/03-event-window.md (DEMO-03).

#include "lvglpp/ui/event_window.hpp"

#include <utility>  // std::move

#include "lvglpp/core/draw_helpers.hpp"  // fill_rounded_rect, detail::draw_border_straight

namespace lvglpp::ui {

namespace {
namespace lc = ::lvglpp::core;
}  // namespace

// ---------------------------------------------------------------------------
// EventWindow — app-relevant surface
// ---------------------------------------------------------------------------

// Mirrors event_window.rs:173, with the DEMO-03 delta: cap to kMaxLines
// (rlvgl caps at MAX_LINES*2) and set visible (rlvgl does not auto-show).
void EventWindow::push_event(std::string text) {
    if (!enabled_) {
        return;
    }
    entries_.push_back(EventEntry{std::move(text), 0});
    // Cap to kMaxLines, dropping the oldest from the front.
    while (entries_.size() > kMaxLines) {
        entries_.erase(entries_.begin());
    }
    visible_ = true;
}

// Mirrors event_window.rs:90.
void EventWindow::toggle_visible() noexcept {
    if (!enabled_) {
        return;
    }
    if (visible_) {
        hide();
    } else {
        visible_ = true;
    }
}

// Mirrors event_window.rs:102.
void EventWindow::hide() noexcept {
    if (visible_) {
        visible_ = false;
        clear_countdown_ = kClearFrames;
    }
}

// Mirrors event_window.rs:82.
void EventWindow::set_enabled(bool val) noexcept {
    enabled_ = val;
    if (!val) {
        hide();
    }
}

// ---------------------------------------------------------------------------
// EventWindow — Widget overrides
// ---------------------------------------------------------------------------

// bounds() collapses to a zero rect when hidden (DEMO-03 §2). The full
// panel rect lives in bounds_ and is used by draw()/clear_region().
lc::Rect EventWindow::bounds() const {
    if (!visible_) {
        return lc::Rect{};
    }
    return bounds_;
}

// Mirrors event_window.rs:191 minus the deferred header / DMA2D path:
// rounded bg + straight border + up to kMaxLines text lines, newest at
// the bottom.
void EventWindow::draw(lc::Renderer& renderer) const {
    if (!visible_) {
        return;
    }

    lc::fill_rounded_rect(renderer, bounds_, bg_color_, radius_);
    lc::detail::draw_border_straight(renderer, bounds_, border_color_,
                                     border_width_);

    if (font_ == nullptr) {
        return;
    }

    const std::int32_t line_h = font_->scaled_height() + 4;
    const std::int32_t inner_x = bounds_.x + padding_;
    const std::int32_t inner_y = bounds_.y + padding_;
    // entries_ is capped at kMaxLines, so the whole list is drawn:
    // index 0 (oldest) at the top, last (newest) at the bottom.
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        const std::int32_t y =
            inner_y + static_cast<std::int32_t>(i) * line_h;
        font_->draw_str(renderer, inner_x, y, entries_[i].text, text_color_);
    }
}

// Mirrors event_window.rs:238. Tick ages + expires entries and hides
// (arming the clear countdown) when the list empties. Pointer / key
// events are non-consuming — the close-button hit test is part of the
// deferred board surface (DEMO-03 §2).
bool EventWindow::handle_event(const lc::Event& event) {
    if (std::holds_alternative<lc::event::Tick>(event)) {
        for (auto& entry : entries_) {
            entry.age += 1;
        }
        std::erase_if(entries_, [this](const EventEntry& e) {
            return e.age >= expire_ticks_;
        });
        if (entries_.empty() && visible_) {
            clear_countdown_ = kClearFrames;
            visible_ = false;
        }
    }
    return false;
}

// Mirrors event_window.rs:268.
std::optional<lc::Rect> EventWindow::clear_region() {
    if (clear_countdown_ > 0 && !visible_) {
        clear_countdown_ = static_cast<std::uint8_t>(clear_countdown_ - 1);
        return bounds_;
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// EventWindowBuilder
// ---------------------------------------------------------------------------

// Mirrors event_window.rs:295.
EventWindowBuilder::EventWindowBuilder(
    const lc::BitmapFont& font) noexcept
    : window_w_{380}, window_h_{0}, font_{&font} {
    const std::int32_t line_h = font.scaled_height() + 4;
    constexpr std::int32_t padding = 12;
    window_h_ = static_cast<std::int32_t>(kMaxLines) * line_h + padding * 2;
}

EventWindowBuilder& EventWindowBuilder::expire_ticks(
    std::uint32_t ticks) noexcept {
    expire_ticks_ = ticks;
    return *this;
}

EventWindowBuilder& EventWindowBuilder::bg_color(lc::Color c) noexcept {
    bg_color_ = c;
    return *this;
}

EventWindowBuilder& EventWindowBuilder::border_color(lc::Color c) noexcept {
    border_color_ = c;
    return *this;
}

EventWindowBuilder& EventWindowBuilder::radius(std::uint8_t r) noexcept {
    radius_ = r;
    return *this;
}

EventWindowBuilder& EventWindowBuilder::width(std::int32_t w) noexcept {
    window_w_ = w;
    return *this;
}

// Mirrors event_window.rs:349.
EventWindowBuilder& EventWindowBuilder::center(std::int32_t screen_w,
                                               std::int32_t screen_h) noexcept {
    pos_x_ = (screen_w - window_w_) / 2;
    pos_y_ = (screen_h - window_h_) / 2;
    return *this;
}

// Mirrors event_window.rs:356.
EventWindow EventWindowBuilder::build() const {
    constexpr std::int32_t margin = 10;
    EventWindow w;
    w.bounds_ = lc::Rect{pos_x_.value_or(margin), pos_y_.value_or(margin),
                         window_w_, window_h_};
    w.bg_color_ = bg_color_;
    w.border_color_ = border_color_;
    w.text_color_ = text_color_;
    w.border_width_ = border_width_;
    w.radius_ = radius_;
    w.padding_ = 12;
    w.expire_ticks_ = expire_ticks_;
    w.clear_countdown_ = 0;
    w.visible_ = false;
    w.enabled_ = false;
    w.font_ = font_;
    return w;
}

}  // namespace lvglpp::ui
