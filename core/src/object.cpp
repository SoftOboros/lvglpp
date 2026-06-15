// object.cpp — Object + Screen implementation (LVGLPP-WRAP-00).
//
// PARITY: rlvgl/core/src/widget.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/core/lv_obj.c, lv_obj_tree.c, lv_obj_event.c.
// DELTA:  see object.hpp — move-only + LV_EVENT_DELETE delete-safety.

#include "lvglpp/core/object.hpp"

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::core {

Object::Object(lv_obj_t* obj) noexcept : obj_{obj} {
    if (obj_ != nullptr) {
        // SAFETY: user_data is owned by the wrapper layer (docs/wrap §5.3).
        //   It holds a back-pointer to this Object, kept current across
        //   moves (see the move constructor). Valid until on_delete_ fires.
        lv_obj_set_user_data(obj_, this);
        lv_obj_add_event_cb(obj_, &Object::on_delete_, LV_EVENT_DELETE, nullptr);
    }
}

Object::Object(Object&& other) noexcept : obj_{other.obj_} {
    other.obj_ = nullptr;
    if (obj_ != nullptr) {
        // Rebind the back-pointer to this (new) location. The event
        // callback was registered once on the lv_obj and reads user_data
        // dynamically, so it does not need re-adding.
        lv_obj_set_user_data(obj_, this);
    }
}

Object::~Object() {
    if (obj_ != nullptr) {
        // We still own a live object: delete it. The delete-safety
        // callback fires synchronously and nulls obj_, which is harmless
        // here.
        lv_obj_delete(obj_);
    }
}

void Object::on_delete_(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    if (obj == nullptr) {
        return;
    }
    auto* self = static_cast<Object*>(lv_obj_get_user_data(obj));
    if (self != nullptr && self->obj_ == obj) {
        self->obj_ = nullptr;
    }
}

lvglpp::expected<Object, ObjectError> Object::try_make(ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_obj_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Object{obj};
}

Object Object::make(ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::Object::make: lv_obj_create failed");
#endif
    }
    return std::move(result.value());
}

lvglpp::expected<Screen, ObjectError> Screen::try_make() noexcept {
    if (lv_display_get_default() == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::NoDisplay};
    }
    lv_obj_t* obj = lv_obj_create(nullptr);  // parentless => screen on default display
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Screen{obj};
}

Screen Screen::make() {
    auto result = try_make();
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::Screen::make: no display or lv_obj_create failed");
#endif
    }
    return std::move(result.value());
}

void Screen::load() noexcept {
    lv_obj_t* raw = borrow_raw();
    if (raw != nullptr) {
        lv_screen_load(raw);
    }
}

// --- LPAR-02: flags, state, hit-test, tree queries ---

void Object::add_flag(ObjectFlag f) noexcept {
    if (obj_ != nullptr) {
        lv_obj_add_flag(obj_, static_cast<lv_obj_flag_t>(f));
    }
}

void Object::remove_flag(ObjectFlag f) noexcept {
    if (obj_ != nullptr) {
        lv_obj_remove_flag(obj_, static_cast<lv_obj_flag_t>(f));
    }
}

bool Object::has_flag(ObjectFlag f) const noexcept {
    return obj_ != nullptr && lv_obj_has_flag(obj_, static_cast<lv_obj_flag_t>(f));
}

void Object::add_state(ObjectState s) noexcept {
    if (obj_ != nullptr) {
        lv_obj_add_state(obj_, static_cast<lv_state_t>(s));
    }
}

void Object::remove_state(ObjectState s) noexcept {
    if (obj_ != nullptr) {
        lv_obj_remove_state(obj_, static_cast<lv_state_t>(s));
    }
}

bool Object::has_state(ObjectState s) const noexcept {
    return obj_ != nullptr && lv_obj_has_state(obj_, static_cast<lv_state_t>(s));
}

ObjectState Object::state() const noexcept {
    if (obj_ == nullptr) {
        return ObjectState::Default;
    }
    return static_cast<ObjectState>(lv_obj_get_state(obj_));
}

bool Object::hit_test(std::int32_t x, std::int32_t y) const noexcept {
    if (obj_ == nullptr) {
        return false;
    }
    lv_point_t point{x, y};
    return lv_obj_hit_test(obj_, &point);
}

ObjectView Object::parent() const noexcept {
    return ObjectView{obj_ != nullptr ? lv_obj_get_parent(obj_) : nullptr};
}

std::uint32_t Object::child_count() const noexcept {
    return obj_ != nullptr ? lv_obj_get_child_count(obj_) : 0U;
}

ObjectView Object::child(std::uint32_t index) const noexcept {
    if (obj_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_obj_get_child(obj_, static_cast<std::int32_t>(index))};
}

}  // namespace lvglpp::core
