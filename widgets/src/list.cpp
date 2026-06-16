// list.cpp — lv_obj-backed List implementation (LVGLPP-WRAP-05).
//
// PARITY: rlvgl/widgets/src/list.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/widgets/list/lv_list.c.
// DELTA:  see list.hpp.

#include "lvglpp/widgets/list.hpp"

#include <utility>

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>
#else
#  include <stdexcept>
#endif

namespace lvglpp::widgets {

using ::lvglpp::core::ObjectError;
using ::lvglpp::ObjectView;

lvglpp::expected<List, ObjectError> List::try_make(::lvglpp::ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_list_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return List{obj};
}

List List::make(::lvglpp::ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::widgets::List::make: lv_list_create failed");
#endif
    }
    return std::move(result.value());
}

ObjectView List::add_text(const char* text) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj == nullptr || text == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_list_add_text(obj, text)};
}

ObjectView List::add_button(const char* text) noexcept {
    lv_obj_t* obj = borrow_raw();
    if (obj == nullptr || text == nullptr) {
        return ObjectView{nullptr};
    }
    // No icon (nullptr); the row button is owned by the list tree.
    return ObjectView{lv_list_add_button(obj, nullptr, text)};
}

const char* List::button_text(ObjectView row) const noexcept {
    lv_obj_t* obj = borrow_raw();
    lv_obj_t* row_raw = row.borrow_raw();
    if (obj == nullptr || row_raw == nullptr) {
        return "";
    }
    const char* text = lv_list_get_button_text(obj, row_raw);
    return text != nullptr ? text : "";
}

}  // namespace lvglpp::widgets
