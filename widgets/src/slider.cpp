// slider.cpp — lv_obj-backed Slider implementation (LVGLPP-WRAP-04).
//
// PARITY: rlvgl/widgets/src/slider.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/slider/lv_slider.c.
// DELTA:  see slider.hpp.

#include "lvglpp/widgets/slider.hpp"

#include <utility>

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;

lvglpp::expected<Slider, ObjectError> Slider::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_slider_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Slider{obj};
}

Slider Slider::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Slider::make: lv_slider_create failed");
#endif
    }
    return std::move(result.value());
}

void Slider::set_value(std::int32_t value, bool animate) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr) {
        lv_slider_set_value(obj, value, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    }
}

std::int32_t Slider::value() const noexcept {
    lv_obj_t* obj = borrow_raw();
    return obj != nullptr ? lv_slider_get_value(obj) : 0;
}

void Slider::set_range(std::int32_t min, std::int32_t max) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr) {
        lv_slider_set_range(obj, min, max);
    }
}

std::int32_t Slider::min() const noexcept {
    lv_obj_t* obj = borrow_raw();
    return obj != nullptr ? lv_slider_get_min_value(obj) : 0;
}

std::int32_t Slider::max() const noexcept {
    lv_obj_t* obj = borrow_raw();
    return obj != nullptr ? lv_slider_get_max_value(obj) : 0;
}

void Slider::set_on_change(std::function<void(std::int32_t)> handler) noexcept {
    on(LV_EVENT_VALUE_CHANGED,
       [fn = std::move(handler)](lv_event_t* e) {
           if (fn) {
               lv_obj_t* target = lv_event_get_target_obj(e);
               if (target != nullptr) {
                   fn(lv_slider_get_value(target));
               }
           }
       });
}

}  // namespace lvglpp::widgets
