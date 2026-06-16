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

LvObject::LvObject(lv_obj_t* raw) noexcept : raw_{raw} {
    install_delete_hook();
}

LvObject LvObject::make_screen() noexcept {
    return LvObject{lv_obj_create(nullptr)};
}

LvObject LvObject::make_child(ObjectView parent) noexcept {
    if (parent.empty()) {
        return LvObject{};
    }
    return LvObject{lv_obj_create(parent.borrow_raw())};
}

LvObject LvObject::make() noexcept {
    return make_screen();
}

LvObject LvObject::make(ObjectView parent) noexcept {
    return make_child(parent);
}

LvObject::LvObject(LvObject&& other) noexcept
    : raw_{other.raw_},
      event_handlers_{std::move(other.event_handlers_)} {
    rebind_delete_hook(&other.raw_);
    other.raw_ = nullptr;
}

LvObject& LvObject::operator=(LvObject&& other) noexcept {
    if (this != &other) {
        reset();
        raw_       = other.raw_;
        event_handlers_ = std::move(other.event_handlers_);
        rebind_delete_hook(&other.raw_);
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

ObjectView LvObject::view() const noexcept {
    return borrow();
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
    remove_delete_hook();
    lv_obj_t* released = raw_;
    raw_               = nullptr;
    return released;
}

void LvObject::reset() noexcept {
    if (is_live(raw_)) {
        lv_obj_delete(raw_);
    }
    raw_ = nullptr;
    event_handlers_.clear();
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

void LvObject::set_size(std::int32_t width, std::int32_t height) noexcept {
    if (is_live(raw_)) {
        lv_obj_set_size(raw_, width, height);
    }
}

#if LV_USE_FLEX
void LvObject::set_flex_flow(lv_flex_flow_t flow) noexcept {
    if (is_live(raw_)) {
        lv_obj_set_flex_flow(raw_, flow);
    }
}
#endif

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

void LvObject::set_tag(const char* tag) noexcept {
    if (is_live(raw_) && tag != nullptr) {
        lv_obj_set_name(raw_, tag);
    }
}

const char* LvObject::tag() const noexcept {
    if (!is_live(raw_)) {
        return "";
    }
    const char* name = lv_obj_get_name(raw_);
    return name != nullptr ? name : "";
}

ObjectView LvObject::find_by_tag(const char* name) const noexcept {
    if (!is_live(raw_) || name == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_obj_find_by_name(raw_, name)};
}

void LvObject::on(lv_event_code_t code,
                  std::function<void(lv_event_t*)> handler) noexcept {
    if (!is_live(raw_) || !handler) {
        return;
    }
    auto holder = std::make_unique<EventHandler>(std::move(handler));
    lv_obj_add_event_cb(raw_, &LvObject::handle_event, code, holder.get());
    event_handlers_.push_back(std::move(holder));
}

void LvObject::on(lv_event_code_t code, std::function<void()> handler) noexcept {
    on(code, [fn = std::move(handler)](lv_event_t*) {
        if (fn) {
            fn();
        }
    });
}

void LvObject::handle_event(lv_event_t* event) noexcept {
    auto* holder = static_cast<EventHandler*>(lv_event_get_user_data(event));
    if (holder != nullptr && holder->fn) {
        holder->fn(event);
    }
}

void LvObject::handle_delete(lv_event_t* event) noexcept {
    auto** raw_slot = static_cast<lv_obj_t**>(lv_event_get_user_data(event));
    if (raw_slot != nullptr) {
        *raw_slot = nullptr;
    }
}

void LvObject::install_delete_hook() noexcept {
    if (is_live(raw_)) {
        lv_obj_add_event_cb(raw_, &LvObject::handle_delete, LV_EVENT_DELETE,
                            &raw_);
    }
}

void LvObject::remove_delete_hook() noexcept {
    if (is_live(raw_)) {
        lv_obj_remove_event_cb_with_user_data(raw_, &LvObject::handle_delete,
                                              &raw_);
    }
}

void LvObject::rebind_delete_hook(lv_obj_t** previous_raw_slot) noexcept {
    if (is_live(raw_)) {
        lv_obj_remove_event_cb_with_user_data(raw_, &LvObject::handle_delete,
                                              previous_raw_slot);
        lv_obj_add_event_cb(raw_, &LvObject::handle_delete, LV_EVENT_DELETE,
                            &raw_);
    }
}

}  // namespace lvglpp
