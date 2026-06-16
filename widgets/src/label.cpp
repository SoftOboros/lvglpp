// label.cpp — lv_obj-backed Label implementation (LVGLPP-WRAP-01).
//
// PARITY: rlvgl/widgets/src/label.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/label/lv_label.c.
// DELTA:  see label.hpp — core::Object subclass over lv_label; widget state
//         lives in LVGL, not in C++ members.

#include "lvglpp/widgets/label.hpp"

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;

lvglpp::expected<Label, ObjectError> Label::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_label_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Label{obj};
}

Label Label::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::Label::make: lv_label_create failed");
#endif
    }
    return std::move(result.value());
}

void Label::set_text(const char* text) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr && text != nullptr) {
        lv_label_set_text(obj, text);  // LVGL copies the text.
    }
}

void Label::set_text_static(const char* text) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr && text != nullptr) {
        // borrows: LVGL stores `text` (no copy); the caller guarantees it
        // outlives the label (label.hpp contract).
        lv_label_set_text_static(obj, text);
    }
}

const char* Label::text() const noexcept {
    lv_obj_t* obj = borrow_raw();
    return obj != nullptr ? lv_label_get_text(obj) : "";
}

void Label::set_long_mode(LongMode mode) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj != nullptr) {
        lv_label_set_long_mode(obj, static_cast<lv_label_long_mode_t>(mode));
    }
}

}  // namespace lvglpp::widgets
