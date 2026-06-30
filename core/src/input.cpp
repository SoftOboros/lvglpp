// input.cpp - LVGL-backed event, focus-group, and input-device wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_event.c, lvgl/src/core/lv_group.c,
//         lvgl/src/indev/lv_indev.c.
// DELTA:  delegates event routing, focus traversal, and input-device
//         processing to LVGL.

#include "lvglpp/core/input.hpp"

namespace lvglpp {

EventCode EventView::code() const noexcept {
    return EventCode::from_lv(raw_code());
}

lv_event_code_t EventView::raw_code() const noexcept {
    return raw_ != nullptr ? lv_event_get_code(raw_) : LV_EVENT_LAST;
}

ObjectView EventView::target() const noexcept {
    if (raw_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_event_get_target_obj(raw_)};
}

ObjectView EventView::current_target() const noexcept {
    if (raw_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_event_get_current_target_obj(raw_)};
}

void* EventView::param() const noexcept {
    return raw_ != nullptr ? lv_event_get_param(raw_) : nullptr;
}

void* EventView::user_data() const noexcept {
    return raw_ != nullptr ? lv_event_get_user_data(raw_) : nullptr;
}

std::uint32_t EventView::key() const noexcept {
    return raw_ != nullptr ? lv_event_get_key(raw_) : 0U;
}

std::int32_t EventView::rotary_diff() const noexcept {
    return raw_ != nullptr ? lv_event_get_rotary_diff(raw_) : 0;
}

void EventView::stop_bubbling() const noexcept {
    if (raw_ != nullptr) {
        lv_event_stop_bubbling(raw_);
    }
}

void EventView::stop_trickling() const noexcept {
    if (raw_ != nullptr) {
        lv_event_stop_trickling(raw_);
    }
}

void EventView::stop_processing() const noexcept {
    if (raw_ != nullptr) {
        lv_event_stop_processing(raw_);
    }
}

EventSubscription::EventSubscription(ObjectView object,
                                     lv_event_dsc_t* descriptor) noexcept
    : raw_object_{object.borrow_raw()}, descriptor_{descriptor} {}

EventSubscription::EventSubscription(EventSubscription&& other) noexcept
    : raw_object_{other.raw_object_}, descriptor_{other.descriptor_} {
    other.raw_object_ = nullptr;
    other.descriptor_ = nullptr;
}

EventSubscription& EventSubscription::operator=(
    EventSubscription&& other) noexcept {
    if (this != &other) {
        reset();
        raw_object_       = other.raw_object_;
        descriptor_       = other.descriptor_;
        other.raw_object_ = nullptr;
        other.descriptor_ = nullptr;
    }
    return *this;
}

EventSubscription::~EventSubscription() {
    reset();
}

bool EventSubscription::empty() const noexcept {
    return raw_object_ == nullptr || descriptor_ == nullptr;
}

lv_event_dsc_t* EventSubscription::borrow_descriptor() const noexcept {
    return descriptor_;
}

lv_event_dsc_t* EventSubscription::release() noexcept {
    lv_event_dsc_t* released = descriptor_;
    raw_object_              = nullptr;
    descriptor_              = nullptr;
    return released;
}

void EventSubscription::reset() noexcept {
    if (raw_object_ != nullptr && descriptor_ != nullptr &&
        lv_obj_is_valid(raw_object_)) {
        lv_obj_remove_event_dsc(raw_object_, descriptor_);
    }
    raw_object_ = nullptr;
    descriptor_ = nullptr;
}

EventSubscription add_event_callback(ObjectView object,
                                     EventCode filter,
                                     lv_event_cb_t callback,
                                     void* user_data) noexcept {
    if (object.empty() || callback == nullptr) {
        return EventSubscription{};
    }

    lv_event_dsc_t* descriptor =
        lv_obj_add_event_cb(object.borrow_raw(), callback, to_lv(filter), user_data);
    return EventSubscription{object, descriptor};
}

lv_result_t send_event(ObjectView object, EventCode code, void* param) noexcept {
    if (object.empty()) {
        return LV_RESULT_INVALID;
    }
    return lv_obj_send_event(object.borrow_raw(), to_lv(code), param);
}

LvGroup::LvGroup(lv_group_t* raw) noexcept : raw_{raw} {}

LvGroup LvGroup::make() noexcept {
    return LvGroup{lv_group_create()};
}

LvGroup::LvGroup(LvGroup&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvGroup& LvGroup::operator=(LvGroup&& other) noexcept {
    if (this != &other) {
        reset();
        raw_       = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvGroup::~LvGroup() {
    reset();
}

GroupView LvGroup::borrow() const noexcept {
    return GroupView{raw_};
}

lv_group_t* LvGroup::borrow_raw() const noexcept {
    return raw_;
}

bool LvGroup::empty() const noexcept {
    return raw_ == nullptr;
}

lv_group_t* LvGroup::release() noexcept {
    lv_group_t* released = raw_;
    raw_                 = nullptr;
    return released;
}

void LvGroup::reset() noexcept {
    if (raw_ != nullptr) {
        lv_group_delete(raw_);
        raw_ = nullptr;
    }
}

void LvGroup::set_default() noexcept {
    if (raw_ != nullptr) {
        lv_group_set_default(raw_);
    }
}

void LvGroup::add_object(ObjectView object) noexcept {
    if (raw_ != nullptr && !object.empty()) {
        lv_group_add_obj(raw_, object.borrow_raw());
    }
}

void LvGroup::remove_object(ObjectView object) noexcept {
    if (!object.empty()) {
        lv_group_remove_obj(object.borrow_raw());
    }
}

void LvGroup::remove_all_objects() noexcept {
    if (raw_ != nullptr) {
        lv_group_remove_all_objs(raw_);
    }
}

void LvGroup::focus_object(ObjectView object) noexcept {
    if (!object.empty()) {
        lv_group_focus_obj(object.borrow_raw());
    }
}

void LvGroup::focus_next() noexcept {
    if (raw_ != nullptr) {
        lv_group_focus_next(raw_);
    }
}

void LvGroup::focus_prev() noexcept {
    if (raw_ != nullptr) {
        lv_group_focus_prev(raw_);
    }
}

ObjectView LvGroup::focused() const noexcept {
    if (raw_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_group_get_focused(raw_)};
}

std::uint32_t LvGroup::object_count() const noexcept {
    return raw_ != nullptr ? lv_group_get_obj_count(raw_) : 0U;
}

ObjectView LvGroup::object_by_index(std::uint32_t index) const noexcept {
    if (raw_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_group_get_obj_by_index(raw_, index)};
}

void LvGroup::set_editing(bool enabled) noexcept {
    if (raw_ != nullptr) {
        lv_group_set_editing(raw_, enabled);
    }
}

bool LvGroup::editing() const noexcept {
    return raw_ != nullptr && lv_group_get_editing(raw_);
}

void LvGroup::set_wrap(bool enabled) noexcept {
    if (raw_ != nullptr) {
        lv_group_set_wrap(raw_, enabled);
    }
}

bool LvGroup::wrap() const noexcept {
    return raw_ != nullptr && lv_group_get_wrap(raw_);
}

lv_group_t* borrow_raw(GroupView group) noexcept {
    return group.borrow_raw();
}

LvInputDevice::LvInputDevice(lv_indev_t* raw) noexcept : raw_{raw} {}

LvInputDevice LvInputDevice::make(InputDeviceType type) noexcept {
    LvInputDevice input{lv_indev_create()};
    input.set_type(type);
    return input;
}

LvInputDevice::LvInputDevice(LvInputDevice&& other) noexcept
    : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvInputDevice& LvInputDevice::operator=(LvInputDevice&& other) noexcept {
    if (this != &other) {
        reset();
        raw_       = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvInputDevice::~LvInputDevice() {
    reset();
}

InputDeviceView LvInputDevice::borrow() const noexcept {
    return InputDeviceView{raw_};
}

lv_indev_t* LvInputDevice::borrow_raw() const noexcept {
    return raw_;
}

bool LvInputDevice::empty() const noexcept {
    return raw_ == nullptr;
}

lv_indev_t* LvInputDevice::release() noexcept {
    lv_indev_t* released = raw_;
    raw_                 = nullptr;
    return released;
}

void LvInputDevice::reset() noexcept {
    if (raw_ != nullptr) {
        lv_indev_delete(raw_);
        raw_ = nullptr;
    }
}

void LvInputDevice::read() noexcept {
    if (raw_ != nullptr) {
        lv_indev_read(raw_);
    }
}

void LvInputDevice::set_type(InputDeviceType type) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_type(raw_, to_lv(type));
    }
}

InputDeviceType LvInputDevice::type() const noexcept {
    return raw_ != nullptr ? input_device_type_from_lv(lv_indev_get_type(raw_))
                           : InputDeviceType::None;
}

InputDeviceState LvInputDevice::state() const noexcept {
    return raw_ != nullptr ? input_device_state_from_lv(lv_indev_get_state(raw_))
                           : InputDeviceState::Released;
}

void LvInputDevice::set_read_callback(lv_indev_read_cb_t callback) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_read_cb(raw_, callback);
    }
}

void LvInputDevice::set_user_data(void* user_data) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_user_data(raw_, user_data);
    }
}

void* LvInputDevice::user_data() const noexcept {
    return raw_ != nullptr ? lv_indev_get_user_data(raw_) : nullptr;
}

void LvInputDevice::set_display(DisplayView display) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_display(raw_, display.borrow_raw());
    }
}

void LvInputDevice::set_group(GroupView group) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_group(raw_, group.borrow_raw());
    }
}

GroupView LvInputDevice::group() const noexcept {
    if (raw_ == nullptr) {
        return GroupView{nullptr};
    }
    return GroupView{lv_indev_get_group(raw_)};
}

void LvInputDevice::set_long_press_time(std::uint16_t ms) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_long_press_time(raw_, ms);
    }
}

void LvInputDevice::set_long_press_repeat_time(std::uint16_t ms) noexcept {
    if (raw_ != nullptr) {
        lv_indev_set_long_press_repeat_time(raw_, ms);
    }
}

void LvInputDevice::reset_long_press() noexcept {
    if (raw_ != nullptr) {
        lv_indev_reset_long_press(raw_);
    }
}

void LvInputDevice::enable(bool enabled) noexcept {
    if (raw_ != nullptr) {
        lv_indev_enable(raw_, enabled);
    }
}

lv_indev_t* borrow_raw(InputDeviceView input) noexcept {
    return input.borrow_raw();
}

}  // namespace lvglpp
