// slider.hpp — value slider widget, lv_obj-backed (LVGLPP-WRAP-04).
//
// PARITY: rlvgl/widgets/src/slider.rs (v0.2.4 @ 343f596) — value + range.
// LVGL:   lvgl/src/widgets/slider/lv_slider.h (create / set_value / get_value
//         / set_range / get_min_value / get_max_value).
// DELTA:  Supersedes lvglpp::widgets::legacy::Slider. core::Object subclass
//         over lv_slider; the change handler reads the value from the event
//         target (move-safe).

#ifndef LVGLPP_WIDGETS_SLIDER_HPP
#define LVGLPP_WIDGETS_SLIDER_HPP

#include <cstdint>
#include <functional>

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Slider — a draggable value selector over an integer range.
//
// Ownership: owns its lv_slider lv_obj (via core::Object). Move-only.
class Slider : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<Slider, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static Slider make(::lvglpp::ObjectView parent);

    Slider(Slider&&) noexcept = default;

    // Current value (lv_slider_set_value / get_value). `animate` selects the
    // LVGL animated transition. Setting is clamped to the range by LVGL.
    void set_value(std::int32_t value, bool animate) noexcept;
    [[nodiscard]] std::int32_t value() const noexcept;

    // Inclusive value range (lv_slider_set_range / get_min_value /
    // get_max_value).
    void set_range(std::int32_t min, std::int32_t max) noexcept;
    [[nodiscard]] std::int32_t min() const noexcept;
    [[nodiscard]] std::int32_t max() const noexcept;

    // Register a change handler (LV_EVENT_VALUE_CHANGED). The handler receives
    // the new value, read from the event's target object (no `this` capture;
    // move-safe).
    void set_on_change(std::function<void(std::int32_t)> handler) noexcept;

private:
    explicit Slider(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_SLIDER_HPP
