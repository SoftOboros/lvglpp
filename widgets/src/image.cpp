// image.cpp — lv_obj-backed Image implementation (LVGLPP-WRAP-06).
//
// PARITY: rlvgl/widgets/src/image.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/image/lv_image.c.
// DELTA:  see image.hpp.

#include "lvglpp/widgets/image.hpp"

#include <utility>

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;

lvglpp::expected<Image, ObjectError> Image::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_image_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Image{obj};
}

Image Image::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Image::make: lv_image_create failed");
#endif
    }
    return std::move(result.value());
}

void Image::set_src(const void* src) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr && src != nullptr) {
        // borrows: LVGL stores the pointer; the caller guarantees `src`
        // outlives this image (image.hpp contract).
        lv_image_set_src(obj, src);
    }
}

}  // namespace lvglpp::widgets
