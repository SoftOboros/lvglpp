// object.cpp - LVGL-backed object owner implementation.
//
// PARITY: rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj.h and lvgl/src/core/lv_obj_tree.h.
// DELTA:  delegates object storage and tree semantics to LVGL.

#include "lvglpp/core/object.hpp"

namespace lvglpp {

namespace {

[[nodiscard]] bool is_live(lv_obj_t* raw) noexcept {
    return raw != nullptr && lv_obj_is_valid(raw);
}

}  // namespace

LvObject::LvObject(lv_obj_t* raw) noexcept : raw_{raw} {}

LvObject LvObject::make_screen() noexcept {
    return LvObject{lv_obj_create(nullptr)};
}

LvObject LvObject::make_child(ObjectView parent) noexcept {
    if (parent.empty()) {
        return LvObject{};
    }
    return LvObject{lv_obj_create(parent.borrow_raw())};
}

LvObject::LvObject(LvObject&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvObject& LvObject::operator=(LvObject&& other) noexcept {
    if (this != &other) {
        reset();
        raw_       = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvObject::~LvObject() {
    reset();
}

ObjectView LvObject::borrow() const noexcept {
    return ObjectView{raw_};
}

lv_obj_t* LvObject::borrow_raw() const noexcept {
    return raw_;
}

bool LvObject::empty() const noexcept {
    return raw_ == nullptr;
}

bool LvObject::valid() const noexcept {
    return is_live(raw_);
}

lv_obj_t* LvObject::release() noexcept {
    lv_obj_t* released = raw_;
    raw_               = nullptr;
    return released;
}

void LvObject::reset() noexcept {
    if (is_live(raw_)) {
        lv_obj_delete(raw_);
    }
    raw_ = nullptr;
}

ObjectView LvObject::parent() const noexcept {
    if (!is_live(raw_)) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_obj_get_parent(raw_)};
}

std::uint32_t LvObject::child_count() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_obj_get_child_count(raw_);
}

ObjectView LvObject::child(std::int32_t index) const noexcept {
    if (!is_live(raw_)) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_obj_get_child(raw_, index)};
}

void LvObject::set_parent(ObjectView parent) noexcept {
    if (is_live(raw_) && !parent.empty()) {
        lv_obj_set_parent(raw_, parent.borrow_raw());
    }
}

void LvObject::move_to_index(std::int32_t index) noexcept {
    if (is_live(raw_)) {
        lv_obj_move_to_index(raw_, index);
    }
}

void LvObject::raise_to_front() noexcept {
    move_to_index(-1);
}

void LvObject::lower_to_back() noexcept {
    move_to_index(0);
}

void LvObject::clean_children() noexcept {
    if (is_live(raw_)) {
        lv_obj_clean(raw_);
    }
}

void LvObject::add_flag(ObjectFlag flag) noexcept {
    if (is_live(raw_)) {
        lv_obj_add_flag(raw_, to_lv(flag));
    }
}

void LvObject::remove_flag(ObjectFlag flag) noexcept {
    if (is_live(raw_)) {
        lv_obj_remove_flag(raw_, to_lv(flag));
    }
}

void LvObject::set_flag(ObjectFlag flag, bool enabled) noexcept {
    if (is_live(raw_)) {
        lv_obj_set_flag(raw_, to_lv(flag), enabled);
    }
}

bool LvObject::has_flag(ObjectFlag flag) const noexcept {
    return is_live(raw_) && lv_obj_has_flag(raw_, to_lv(flag));
}

void LvObject::add_state(ObjectState state) noexcept {
    if (is_live(raw_)) {
        lv_obj_add_state(raw_, to_lv(state));
    }
}

void LvObject::remove_state(ObjectState state) noexcept {
    if (is_live(raw_)) {
        lv_obj_remove_state(raw_, to_lv(state));
    }
}

void LvObject::set_state(ObjectState state, bool enabled) noexcept {
    if (is_live(raw_)) {
        lv_obj_set_state(raw_, to_lv(state), enabled);
    }
}

bool LvObject::has_state(ObjectState state) const noexcept {
    return is_live(raw_) && lv_obj_has_state(raw_, to_lv(state));
}

}  // namespace lvglpp
