// icon_strip.hpp — right-edge vertical icon strip composite widget.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/icon_strip.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  Rc<RefCell>/Box<FnMut> ownership replaced by single-owner
//         core::Widget; slots owned by value, callbacks are std::function
//         (DEMO-00 §5). Per-draw icon decode into a stack-local scratch
//         buffer (no widget-owned pixel cache).
//
// docs/disco-demo/05-composite-widgets.md (DEMO-05) §4.

#ifndef LVGLPP_APP_DISCO_DEMO_ICON_STRIP_HPP
#define LVGLPP_APP_DISCO_DEMO_ICON_STRIP_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::app::disco_demo {

// Number of icon slots in the strip. FROZEN (DEMO-00 §6) — mirrors
// icon_strip.rs:16 SLOT_COUNT.
inline constexpr std::size_t SLOT_COUNT = 3;

// A single icon slot with RLE-compressed icon data.
//
// Ownership: `rle` borrows immortal static icon bytes (from assets.hpp);
// `on_tap` is owned by value and (when bound in DEMO-06) observes the
// controller — its capture lifetime is documented at the bind site.
struct IconSlot {
    // borrows: RLEC blob bytes; lifetime is the program's (static asset).
    std::span<const std::uint8_t> rle{};
    // Whether the slot is interactive.
    bool enabled = false;
    // owns: tap callback; invoked with the slot index on PressRelease.
    std::function<void(std::size_t)> on_tap{};
};

// Right-edge icon strip mirroring the STM32H747I-DISCO demo layout.
class IconStrip final : public core::Widget {
public:
    // Args:
    //   x:          left edge of the strip column.
    //   icon_size:  square icon side length.
    //   margin_top: gap above the first slot.
    //   gap:        vertical gap between slots.
    IconStrip(std::int32_t x,
              std::int32_t icon_size,
              std::int32_t margin_top,
              std::int32_t gap) noexcept
        : x_{x}, margin_top_{margin_top}, gap_{gap}, icon_size_{icon_size} {}

    // Place a slot at `index` (ignored when out of range). Takes ownership
    // of `slot` (and its callback) by move.
    void set_slot(std::size_t index, IconSlot slot);

    // Set / query the focused slot index for highlight rendering.
    void set_focused_slot(std::optional<std::size_t> index) noexcept {
        focused_slot_ = index;
    }
    [[nodiscard]] std::optional<std::size_t> focused_slot() const noexcept {
        return focused_slot_;
    }

    // ---- Widget overrides --------------------------------------------------
    [[nodiscard]] core::Rect bounds() const override;
    void draw(core::Renderer& renderer) const override;
    [[nodiscard]] bool handle_event(const core::Event& event) override;

private:
    [[nodiscard]] core::Rect slot_bounds(std::size_t index) const noexcept;

    std::array<std::optional<IconSlot>, SLOT_COUNT> slots_{};
    std::int32_t x_          = 0;
    std::int32_t margin_top_ = 0;
    std::int32_t gap_        = 0;
    std::int32_t icon_size_  = 0;
    std::optional<std::size_t> focused_slot_{};
};

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_ICON_STRIP_HPP
