// switch.cpp — lv_obj-backed Switch implementation (LVGLPP-WRAP-03).
//
// PARITY: rlvgl/widgets/src/switch.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/switch/lv_switch.c.
// DELTA:  see switch.hpp.

#include "lvglpp/widgets/switch.hpp"

#include <utility>

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;
using ::lvglpp::core::ObjectState;

lvglpp::expected<Switch, ObjectError> Switch::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_switch_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Switch{obj};
}

Switch Switch::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Switch::make: lv_switch_create failed");
#endif
    }
    return std::move(result.value());
}

void Switch::set_on(bool on) noexcept {
    if (on) {
        add_state(ObjectState::Checked);
    } else {
        remove_state(ObjectState::Checked);
    }
}

bool Switch::is_on() const noexcept {
    return has_state(ObjectState::Checked);
}

void Switch::set_on_change(std::function<void(bool)> handler) noexcept {
    on(::lvglpp::core::EventCode::ValueChanged,
       [fn = std::move(handler)](lv_event_t* e) {
           if (fn) {
               lv_obj_t* target = lv_event_get_target_obj(e);
               fn(target != nullptr && lv_obj_has_state(target, LV_STATE_CHECKED));
           }
       });
}

}  // namespace lvglpp::widgets
