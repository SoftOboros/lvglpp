// input.cpp — FocusGroup + InputDevice RAII and the lv_event_t ↔ CORE-02
// Event conversion seam (LPAR-04).
//
// PARITY: rlvgl/core/{focus,object dispatch} + platform input (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/core/lv_group.c, lvgl/src/indev/lv_indev.c,
//         lvgl/src/core/lv_obj_event.c, lvgl/src/misc/lv_event.c.
// DELTA:  see input.hpp — move-only RAII over lv_group_t/lv_indev_t; the seam
//         is lvgl-free CORE-02 Event on one side, lv_event_t/lv_indev_data_t
//         on the other, mapping only the representable pointer/key subset.

#include "lvglpp/core/input.hpp"

#include <utility>  // std::move
#include <variant>  // std::holds_alternative / std::get_if

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::core {

// --- FocusGroup ---

lvglpp::expected<FocusGroup, FocusGroupError> FocusGroup::try_make() noexcept {
    lv_group_t* group = lv_group_create();
    if (group == nullptr) {
        return lvglpp::unexpected<FocusGroupError>{FocusGroupError::CreateFailed};
    }
    return FocusGroup{group};
}

FocusGroup FocusGroup::make() {
    auto result = try_make();
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::FocusGroup::make: lv_group_create failed");
#endif
    }
    return std::move(result.value());
}

FocusGroup::FocusGroup(FocusGroup&& other) noexcept : group_{other.group_} {
    other.group_ = nullptr;
}

FocusGroup::~FocusGroup() {
    if (group_ != nullptr) {
        lv_group_delete(group_);
    }
}

void FocusGroup::add(ObjectView obj) noexcept {
    lv_obj_t* raw = obj.borrow_raw();
    if (group_ != nullptr && raw != nullptr) {
        lv_group_add_obj(group_, raw);
    }
}

void FocusGroup::remove(ObjectView obj) noexcept {
    lv_obj_t* raw = obj.borrow_raw();
    if (raw != nullptr) {
        // lv_group_remove_obj is object-centric: it removes the object from
        // whatever group currently owns it.
        lv_group_remove_obj(raw);
    }
}

void FocusGroup::focus(ObjectView obj) noexcept {
    lv_obj_t* raw = obj.borrow_raw();
    if (raw != nullptr) {
        lv_group_focus_obj(raw);
    }
}

void FocusGroup::focus_next() noexcept {
    if (group_ != nullptr) {
        lv_group_focus_next(group_);
    }
}

void FocusGroup::focus_prev() noexcept {
    if (group_ != nullptr) {
        lv_group_focus_prev(group_);
    }
}

void FocusGroup::set_editing(bool edit) noexcept {
    if (group_ != nullptr) {
        lv_group_set_editing(group_, edit);
    }
}

// --- InputDevice ---

lvglpp::expected<InputDevice, InputDeviceError> InputDevice::try_make() noexcept {
    lv_indev_t* indev = lv_indev_create();
    if (indev == nullptr) {
        return lvglpp::unexpected<InputDeviceError>{InputDeviceError::CreateFailed};
    }
    return InputDevice{indev};
}

InputDevice InputDevice::make() {
    auto result = try_make();
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::InputDevice::make: lv_indev_create failed");
#endif
    }
    return std::move(result.value());
}

InputDevice::InputDevice(InputDevice&& other) noexcept : indev_{other.indev_} {
    other.indev_ = nullptr;
}

InputDevice::~InputDevice() {
    if (indev_ != nullptr) {
        lv_indev_delete(indev_);
    }
}

void InputDevice::set_type(InputType type) noexcept {
    if (indev_ != nullptr) {
        lv_indev_set_type(indev_, static_cast<lv_indev_type_t>(type));
    }
}

void InputDevice::set_read_cb(lv_indev_read_cb_t read_cb) noexcept {
    if (indev_ != nullptr) {
        lv_indev_set_read_cb(indev_, read_cb);
    }
}

void InputDevice::set_group(const FocusGroup& group) noexcept {
    if (indev_ != nullptr) {
        lv_indev_set_group(indev_, group.borrow_raw());
    }
}

void InputDevice::set_display(lv_display_t* disp) noexcept {
    if (indev_ != nullptr) {
        lv_indev_set_display(indev_, disp);
    }
}

void InputDevice::set_user_data(void* user_data) noexcept {
    if (indev_ != nullptr) {
        lv_indev_set_user_data(indev_, user_data);
    }
}

// --- lv_event_t ↔ CORE-02 Event seam ---

Key key_from_lv(std::uint32_t lv_key) noexcept {
    switch (lv_key) {
        case LV_KEY_UP:
            return key::ArrowUp{};
        case LV_KEY_DOWN:
            return key::ArrowDown{};
        case LV_KEY_LEFT:
            return key::ArrowLeft{};
        case LV_KEY_RIGHT:
            return key::ArrowRight{};
        case LV_KEY_ESC:
            return key::Escape{};
        case LV_KEY_ENTER:
            return key::Enter{};
        default:
            break;
    }
    if (lv_key == static_cast<std::uint32_t>(' ')) {
        return key::Space{};
    }
    // Printable ASCII maps to a Character; anything else is opaque Other.
    if (lv_key >= 0x20U && lv_key <= 0x7EU) {
        return key::Character{lv_key};
    }
    return key::Other{lv_key};
}

std::uint32_t lv_key_of(const Key& key) noexcept {
    if (std::holds_alternative<key::ArrowUp>(key)) {
        return static_cast<std::uint32_t>(LV_KEY_UP);
    }
    if (std::holds_alternative<key::ArrowDown>(key)) {
        return static_cast<std::uint32_t>(LV_KEY_DOWN);
    }
    if (std::holds_alternative<key::ArrowLeft>(key)) {
        return static_cast<std::uint32_t>(LV_KEY_LEFT);
    }
    if (std::holds_alternative<key::ArrowRight>(key)) {
        return static_cast<std::uint32_t>(LV_KEY_RIGHT);
    }
    if (std::holds_alternative<key::Escape>(key)) {
        return static_cast<std::uint32_t>(LV_KEY_ESC);
    }
    if (std::holds_alternative<key::Enter>(key)) {
        return static_cast<std::uint32_t>(LV_KEY_ENTER);
    }
    if (std::holds_alternative<key::Space>(key)) {
        return static_cast<std::uint32_t>(' ');
    }
    if (const auto* c = std::get_if<key::Character>(&key)) {
        return c->codepoint;
    }
    if (const auto* f = std::get_if<key::Function>(&key)) {
        return f->n;  // best-effort: LVGL has no function-key code.
    }
    if (const auto* o = std::get_if<key::Other>(&key)) {
        return o->code;
    }
    return 0U;
}

std::optional<lv_event_code_t> lv_code_of(const Event& ev) noexcept {
    if (std::holds_alternative<event::PressDown>(ev)) {
        return LV_EVENT_PRESSED;
    }
    if (std::holds_alternative<event::PressRelease>(ev)) {
        return LV_EVENT_RELEASED;
    }
    if (std::holds_alternative<event::PointerMove>(ev)) {
        return LV_EVENT_PRESSING;
    }
    if (std::holds_alternative<event::KeyDown>(ev)) {
        return LV_EVENT_KEY;
    }
    return std::nullopt;
}

std::optional<Event> event_of_code(lv_event_code_t code, std::int32_t x, std::int32_t y,
                                   std::uint32_t key) noexcept {
    switch (code) {
        case LV_EVENT_PRESSED:
            return Event{event::PressDown{x, y}};
        case LV_EVENT_RELEASED:
            return Event{event::PressRelease{x, y}};
        case LV_EVENT_PRESSING:
            return Event{event::PointerMove{x, y}};
        case LV_EVENT_KEY:
            return Event{event::KeyDown{key_from_lv(key)}};
        default:
            return std::nullopt;
    }
}

std::optional<Event> event_from_lv(lv_event_t* e) noexcept {
    if (e == nullptr) {
        return std::nullopt;
    }
    const lv_event_code_t code = lv_event_get_code(e);

    std::int32_t x = 0;
    std::int32_t y = 0;
    if (lv_indev_t* indev = lv_event_get_indev(e); indev != nullptr) {
        lv_point_t point{};
        lv_indev_get_point(indev, &point);
        x = point.x;
        y = point.y;
    }

    std::uint32_t key = 0;
    if (code == LV_EVENT_KEY) {
        key = lv_event_get_key(e);
    }
    return event_of_code(code, x, y, key);
}

bool event_to_indev(const Event& ev, lv_indev_data_t* out) noexcept {
    if (out == nullptr) {
        return false;
    }
    if (const auto* p = std::get_if<event::PressDown>(&ev)) {
        out->point.x = p->x;
        out->point.y = p->y;
        out->state   = LV_INDEV_STATE_PRESSED;
        return true;
    }
    if (const auto* p = std::get_if<event::PointerMove>(&ev)) {
        out->point.x = p->x;
        out->point.y = p->y;
        out->state   = LV_INDEV_STATE_PRESSED;
        return true;
    }
    if (const auto* p = std::get_if<event::PressRelease>(&ev)) {
        out->point.x = p->x;
        out->point.y = p->y;
        out->state   = LV_INDEV_STATE_RELEASED;
        return true;
    }
    if (const auto* k = std::get_if<event::KeyDown>(&ev)) {
        out->key   = lv_key_of(k->key);
        out->state = LV_INDEV_STATE_PRESSED;
        return true;
    }
    if (const auto* k = std::get_if<event::KeyUp>(&ev)) {
        out->key   = lv_key_of(k->key);
        out->state = LV_INDEV_STATE_RELEASED;
        return true;
    }
    return false;
}

}  // namespace lvglpp::core
