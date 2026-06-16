// button.cpp — lv_obj-backed Button implementation (LVGLPP-WRAP-02).
//
// PARITY: rlvgl/widgets/src/button.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/button/lv_button.c; child lv_label for text.
// DELTA:  see button.hpp.

#include "lvglpp/widgets/button.hpp"

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;

lvglpp::expected<Button, ObjectError> Button::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_button_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Button{obj};
}

Button Button::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Button::make: lv_button_create failed");
#endif
    }
    return std::move(result.value());
}

void Button::set_text(const char* text) noexcept {
    lv_obj_t* btn = borrow_raw();
    if (btn == nullptr || text == nullptr) {
        return;
    }
    // The text label is child 0; create it on first use.
    lv_obj_t* label = (lv_obj_get_child_count(btn) > 0) ? lv_obj_get_child(btn, 0)
                                                        : lv_label_create(btn);
    if (label != nullptr) {
        lv_label_set_text(label, text);
        lv_obj_center(label);
    }
}

const char* Button::text() const noexcept {
    lv_obj_t* btn = borrow_raw();
    if (btn == nullptr || lv_obj_get_child_count(btn) == 0) {
        return "";
    }
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    return label != nullptr ? lv_label_get_text(label) : "";
}

void Button::set_on_click(std::function<void()> handler) noexcept {
    // Inherited Object::on stores the handler by holder address (stable across
    // moves) and does not capture `this` — move-safe.
    on(LV_EVENT_CLICKED, std::move(handler));
}

}  // namespace lvglpp::widgets
