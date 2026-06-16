// container.cpp — lv_obj-backed Container implementation (LVGLPP-WRAP-05).
//
// PARITY: rlvgl/widgets/src/container.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/core/lv_obj.c (lv_obj_create).
// DELTA:  see container.hpp.

#include "lvglpp/widgets/container.hpp"

#include <utility>

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;

lvglpp::expected<Container, ObjectError> Container::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_obj_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Container{obj};
}

Container Container::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Container::make: lv_obj_create failed");
#endif
    }
    return std::move(result.value());
}

}  // namespace lvglpp::widgets
