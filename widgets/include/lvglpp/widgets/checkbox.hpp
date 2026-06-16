// checkbox.hpp — labeled checkbox widget, lv_obj-backed (LVGLPP-WRAP-03).
//
// PARITY: rlvgl/widgets/src/checkbox.rs (v0.2.4 @ 343f596) — text + checked.
// LVGL:   lvgl/src/widgets/checkbox/lv_checkbox.h (create / set_text /
//         get_text); checked state is LV_STATE_CHECKED.
// DELTA:  Supersedes lvglpp::widgets::legacy::Checkbox. core::Object subclass
//         over lv_checkbox; checked maps to the inherited state surface; the
//         change handler reads state from the event target (move-safe).

#ifndef LVGLPP_WIDGETS_CHECKBOX_HPP
#define LVGLPP_WIDGETS_CHECKBOX_HPP

#include <functional>

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Checkbox — a tick box with a text label and a checked state.
//
// Ownership: owns its lv_checkbox lv_obj (via core::Object). Move-only.
class Checkbox : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<Checkbox, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static Checkbox make(::lvglpp::ObjectView parent);

    Checkbox(Checkbox&&) noexcept = default;

    // Label text (lv_checkbox_set_text, copies; lv_checkbox_get_text).
    void set_text(const char* text) noexcept;
    [[nodiscard]] const char* text() const noexcept;

    // Checked state (LV_STATE_CHECKED via the inherited state surface).
    void set_checked(bool checked) noexcept;
    [[nodiscard]] bool is_checked() const noexcept;

    // Register a change handler (LV_EVENT_VALUE_CHANGED). The handler receives
    // the new checked state, read from the event's target object — so it stays
    // valid across moves of this Checkbox (no `this` capture).
    void set_on_change(std::function<void(bool)> handler) noexcept;

private:
    explicit Checkbox(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_CHECKBOX_HPP
