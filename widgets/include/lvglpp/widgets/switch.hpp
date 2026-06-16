// switch.hpp — on/off toggle switch widget, lv_obj-backed (LVGLPP-WRAP-03).
//
// PARITY: rlvgl/widgets/src/switch.rs (v0.2.4 @ 343f596) — on/off state.
// LVGL:   lvgl/src/widgets/switch/lv_switch.h (lv_switch_create); the on
//         state is LV_STATE_CHECKED.
// DELTA:  Supersedes lvglpp::widgets::legacy::Switch. core::Object subclass
//         over lv_switch; on maps to the inherited state surface; the change
//         handler reads state from the event target (move-safe).

#ifndef LVGLPP_WIDGETS_SWITCH_HPP
#define LVGLPP_WIDGETS_SWITCH_HPP

#include <functional>

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Switch — a binary on/off toggle.
//
// Ownership: owns its lv_switch lv_obj (via core::Object). Move-only.
class Switch : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<Switch, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static Switch make(::lvglpp::ObjectView parent);

    Switch(Switch&&) noexcept = default;

    // On state (LV_STATE_CHECKED via the inherited state surface).
    void set_on(bool on) noexcept;
    [[nodiscard]] bool is_on() const noexcept;

    // Register a change handler (LV_EVENT_VALUE_CHANGED). The handler receives
    // the new on state, read from the event's target object (no `this`
    // capture; move-safe).
    void set_on_change(std::function<void(bool)> handler) noexcept;

private:
    explicit Switch(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_SWITCH_HPP
