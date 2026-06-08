// wing.hpp — collapsible left-edge popup wing composite widget.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/wing.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  single-owner core::Widget; slots owned by value, callbacks are
//         std::function (DEMO-00 §5). Per-draw icon decode into a stack-local
//         scratch buffer. Rounded border falls back to a straight border
//         (lvglpp core lacks the rounded-border path — CORE-04b).
//
// docs/disco-demo/05-composite-widgets.md (DEMO-05) §4.

#ifndef LVGLPP_APP_DISCO_DEMO_WING_HPP
#define LVGLPP_APP_DISCO_DEMO_WING_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <utility>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::app::disco_demo {

// FROZEN (DEMO-00 §6) — mirror wing.rs:17,23.
inline constexpr std::size_t  MAX_SLOTS    = 6;
inline constexpr std::uint8_t CLEAR_FRAMES = 3;

// A single wing slot with icon data and an optional callback.
//
// Ownership mirrors IconSlot: `rle` borrows static icon bytes; `on_tap` is
// owned by value and observes the controller when bound (DEMO-06).
struct WingSlot {
    // borrows: RLEC blob bytes; lifetime is the program's (static asset).
    std::span<const std::uint8_t> rle{};
    bool enabled = false;
    // owns: tap callback; invoked with the slot index on PressRelease.
    std::function<void(std::size_t)> on_tap{};
};

// Left-edge wing shown when a main-strip icon expands.
class Wing final : public core::Widget {
public:
    // Construct a wing from a vertical stack of `(rle, enabled)` pairs.
    // Only the first MAX_SLOTS pairs are used. The wing starts hidden.
    //
    // Args:
    //   icons: borrows the pair array for the duration of the call only;
    //          each blob span is copied into a slot (the bytes themselves
    //          stay borrowed from static storage).
    explicit Wing(
        std::span<const std::pair<std::span<const std::uint8_t>, bool>> icons);

    // Set / query the focused slot index for highlight rendering.
    void set_focused_slot(std::optional<std::size_t> index) noexcept {
        focused_slot_ = index;
    }
    [[nodiscard]] std::optional<std::size_t> focused_slot() const noexcept {
        return focused_slot_;
    }

    [[nodiscard]] bool is_visible() const noexcept { return visible_; }

    // Toggle visibility; returns the new state.
    bool toggle_visible() noexcept;

    // Hide the wing and request a short clear countdown.
    void close() noexcept;

    // Non-owning mutable access to the slots for callback wiring (DEMO-06).
    [[nodiscard]] std::array<std::optional<WingSlot>, MAX_SLOTS>&
    slots_mut() noexcept {
        return slots_;
    }

    // ---- Widget overrides --------------------------------------------------
    // DELTA: the FROZEN DEMO-00 §6 / DEMO-05 §4 contract requires bounds()
    // to collapse to zero when hidden (so input/automation queries ignore a
    // closed wing). rlvgl wing.rs:144 currently returns self.bounds
    // unconditionally; lvglpp tracks the ratified spec, not that source line.
    [[nodiscard]] core::Rect bounds() const override {
        return visible_ ? bounds_ : core::Rect{0, 0, 0, 0};
    }
    void draw(core::Renderer& renderer) const override;
    [[nodiscard]] bool handle_event(const core::Event& event) override;
    [[nodiscard]] std::optional<core::Rect> clear_region() override;

private:
    [[nodiscard]] core::Rect icon_rect(std::size_t index) const noexcept;

    std::array<std::optional<WingSlot>, MAX_SLOTS> slots_{};
    std::size_t   slot_count_      = 0;
    bool          visible_         = false;
    core::Rect    bounds_{};
    std::uint8_t  clear_countdown_ = 0;
    std::optional<std::size_t> focused_slot_{};
};

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_WING_HPP
