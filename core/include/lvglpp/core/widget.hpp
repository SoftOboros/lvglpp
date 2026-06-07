// widget.hpp — geometry, color, and widget-base types.
//
// PARITY: rlvgl/core/src/widget.rs (v0.2.0 @ d99f793) — Rect (line 14),
//         Color (line 27), Widget trait (line 66).
// LVGL:   lvgl/src/core/lv_obj.h — informative only. lvglpp::core::Widget
//         is a higher-level abstraction; LVGL's lv_obj_t is wrapped by
//         ObjectView (CORE-01) at the platform translation unit level.
// DELTA:  rlvgl uses a Rust trait with `&dyn` virtual dispatch; lvglpp
//         uses a classical C++ abstract base class. The contract
//         (four virtual methods, default clear_region) is identical.
//
// This file implements docs/core-widget/00-widget-tree.md (CORE-03).
// Variant sets and field shapes are FROZEN under Standards Action.

#ifndef LVGLPP_CORE_WIDGET_HPP
#define LVGLPP_CORE_WIDGET_HPP

#include <cstdint>
#include <optional>

#include "lvglpp/core/event.hpp"

namespace lvglpp::core {

// Forward declaration; full definition in renderer.hpp (CORE-04).
class Renderer;

// Rectangle bounds. Concepts doc §5.2 — int32_t x/y/width/height.
// Coordinates are relative to the parent widget; signed to simplify
// layout arithmetic.
struct Rect {
    std::int32_t x      = 0;
    std::int32_t y      = 0;
    std::int32_t width  = 0;
    std::int32_t height = 0;

    constexpr bool operator==(const Rect&) const noexcept = default;
};

// RGBA color, four 8-bit channels in R, G, B, A order.
// Concepts doc §5.3 — channel order is normative; A=255 is opaque.
//
// Mirrors rlvgl/core/src/widget.rs:27 — same channel order, same
// to_argb8888() / with_alpha() helpers (concepts doc §3).
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr bool operator==(const Color&) const noexcept = default;

    // Pack to ARGB8888, the canonical wire format for display
    // backends. Mirrors rlvgl/core/src/widget.rs:45.
    [[nodiscard]] constexpr std::uint32_t to_argb8888() const noexcept {
        return (static_cast<std::uint32_t>(a) << 24) |
               (static_cast<std::uint32_t>(r) << 16) |
               (static_cast<std::uint32_t>(g) << 8)  |
               (static_cast<std::uint32_t>(b));
    }

    // Multiply the alpha channel by `opacity` (0..=255) and return a
    // new Color. Mirrors rlvgl/core/src/widget.rs:52 — the
    // computation `(a as u16 * opacity as u16) / 255` is preserved
    // exactly.
    [[nodiscard]] constexpr Color with_alpha(std::uint8_t opacity) const noexcept {
        const auto blended = (static_cast<std::uint16_t>(a) *
                              static_cast<std::uint16_t>(opacity)) / 255U;
        return Color{r, g, b, static_cast<std::uint8_t>(blended)};
    }
};

// Widget abstract base — every concrete widget inherits.
//
// Concepts doc §5.1 freezes the four virtual methods. rlvgl's trait
// shape is mirrored field-for-field; the only structural difference
// is that C++ requires explicit `virtual` and `override` keywords
// while Rust's trait dispatches implicitly.
//
// Ownership: Widget is a value-type abstraction that does NOT own
// the underlying lv_obj_t (if any). Concrete widgets that wrap an
// LVGL object hold an ObjectView (CORE-01, `external` ownership).
// The Widget destructor is virtual but does NOT call lv_obj_del;
// LVGL tree teardown is the application's responsibility per the
// `external` lifetime tag.
class Widget {
public:
    Widget()                                 = default;
    Widget(const Widget&)                    = default;
    Widget(Widget&&) noexcept                = default;
    Widget& operator=(const Widget&)         = default;
    Widget& operator=(Widget&&) noexcept     = default;
    virtual ~Widget()                        = default;

    // Bounds of this widget, relative to its parent.
    // Returns: a Rect by value. Pure virtual.
    [[nodiscard]] virtual Rect bounds() const = 0;

    // Render the widget through the supplied renderer.
    // Args:
    //   renderer: borrows Renderer for the duration of the call;
    //             the widget MUST NOT retain a reference past return.
    virtual void draw(Renderer& renderer) const = 0;

    // Handle an input event. Returns true iff this widget consumed
    // the event (parent / sibling propagation MAY then stop, per
    // dispatch-layer rules — outside this chapter's scope).
    //
    // Args:
    //   event: borrows Event for the duration of the call.
    [[nodiscard]] virtual bool handle_event(const Event& event) = 0;

    // Per-frame compositor hook. Default: no clearing required.
    // Override for overlay widgets that have just become hidden;
    // return the previous bounds so the compositor can restore the
    // pristine background.
    //
    // Returns: borrows nothing; the Rect is returned by value.
    [[nodiscard]] virtual std::optional<Rect> clear_region() {
        return std::nullopt;
    }
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_WIDGET_HPP
