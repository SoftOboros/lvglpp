// checkbox.cpp — lv_obj-backed Checkbox implementation (LVGLPP-WRAP-03).
//
// PARITY: rlvgl/widgets/src/checkbox.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/checkbox/lv_checkbox.c.
// DELTA:  see checkbox.hpp.

#include "lvglpp/widgets/checkbox.hpp"

#include <utility>

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;
using ::lvglpp::core::ObjectState;

lvglpp::expected<Checkbox, ObjectError> Checkbox::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_checkbox_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Checkbox{obj};
}

Checkbox Checkbox::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Checkbox::make: lv_checkbox_create failed");
#endif
    }
    return std::move(result.value());
}

void Checkbox::set_text(const char* text) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr && text != nullptr) {
        lv_checkbox_set_text(obj, text);
    }
}

const char* Checkbox::text() const noexcept {
    lv_obj_t* obj = borrow_raw();
    return obj != nullptr ? lv_checkbox_get_text(obj) : "";
}

void Checkbox::set_checked(bool checked) noexcept {
    if (checked) {
        add_state(ObjectState::Checked);
    } else {
        remove_state(ObjectState::Checked);
    }
}

bool Checkbox::is_checked() const noexcept {
    return has_state(ObjectState::Checked);
}

void Checkbox::set_on_change(std::function<void(bool)> handler) noexcept {
    on(LV_EVENT_VALUE_CHANGED,
       [fn = std::move(handler)](lv_event_t* e) {
           if (fn) {
               lv_obj_t* target = lv_event_get_target_obj(e);
               fn(target != nullptr && lv_obj_has_state(target, LV_STATE_CHECKED));
           }
       });
}

}  // namespace lvglpp::widgets
