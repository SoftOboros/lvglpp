// button.hpp — clickable button widget, lv_obj-backed (LVGLPP-WRAP-02).
//
// PARITY: rlvgl/widgets/src/button.rs (v0.2.4 @ 343f596) — text + click.
// LVGL:   lvgl/src/widgets/button/lv_button.h (lv_button_create); a child
//         lv_label carries the text (LVGL buttons have no built-in text).
// DELTA:  Supersedes lvglpp::widgets::legacy::Button. core::Object subclass
//         over lv_button. The click handler uses the inherited move-safe
//         Object::on (a std::function<void()> stored by holder address, not
//         capturing `this`).

#ifndef LVGLPP_WIDGETS_BUTTON_HPP
#define LVGLPP_WIDGETS_BUTTON_HPP

#include <functional>

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Button — clickable button with an optional centered text label child.
//
// Ownership: owns its lv_button lv_obj (via core::Object). Move-only.
class Button : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<Button, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static Button make(::lvglpp::ObjectView parent);

    Button(Button&&) noexcept = default;

    // Set the button's centered text, creating the child label on first use
    // (lv_label_create + lv_label_set_text + lv_obj_center). No-op when empty.
    //   text: borrows for the call only (copied into the child label).
    void set_text(const char* text) noexcept;
    // The button's text (child label), or "" if none / empty.
    [[nodiscard]] const char* text() const noexcept;

    // Register a click handler (LV_EVENT_CLICKED) via the inherited Object::on.
    // The handler is owned by this Button and move-safe (no `this` capture).
    //   handler: owns; invoked on each click until the Button is destroyed.
    void set_on_click(std::function<void()> handler) noexcept;

private:
    explicit Button(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_BUTTON_HPP
