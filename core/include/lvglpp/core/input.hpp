// input.hpp - LVGL-backed event, focus-group, and input-device wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_event.h, lvgl/src/core/lv_group.h,
//         lvgl/src/indev/lv_indev.h.
// DELTA:  lvglpp delegates event routing, focus traversal, and input-device
//         processing to LVGL instead of porting rlvgl's Rust dispatcher.

#ifndef LVGLPP_CORE_INPUT_HPP
#define LVGLPP_CORE_INPUT_HPP

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"

#include <cstdint>

namespace lvglpp {

class EventCode {
public:
    enum class Kind : std::uint8_t {
        All,
        Pressed,
        Pressing,
        Released,
        Clicked,
        DoubleClicked,
        LongPressed,
        LongPressedRepeat,
        Focused,
        Defocused,
        Key,
        Rotary,
        Gesture,
        ScrollBegin,
        Scroll,
        ScrollEnd,
        ScrollThrowBegin,
        ChildChanged,
        Delete,
        ValueChanged,
        Ready,
        Cancel,
        InvalidateArea,
        Other,
    };

    [[nodiscard]] static constexpr EventCode all() noexcept {
        return EventCode{Kind::All, LV_EVENT_ALL};
    }

    [[nodiscard]] static constexpr EventCode from_lv(lv_event_code_t raw) noexcept {
        switch (raw) {
            case LV_EVENT_ALL:                 return EventCode{Kind::All, raw};
            case LV_EVENT_PRESSED:             return EventCode{Kind::Pressed, raw};
            case LV_EVENT_PRESSING:            return EventCode{Kind::Pressing, raw};
            case LV_EVENT_RELEASED:            return EventCode{Kind::Released, raw};
            case LV_EVENT_CLICKED:             return EventCode{Kind::Clicked, raw};
            case LV_EVENT_DOUBLE_CLICKED:      return EventCode{Kind::DoubleClicked, raw};
            case LV_EVENT_LONG_PRESSED:        return EventCode{Kind::LongPressed, raw};
            case LV_EVENT_LONG_PRESSED_REPEAT: return EventCode{Kind::LongPressedRepeat, raw};
            case LV_EVENT_FOCUSED:             return EventCode{Kind::Focused, raw};
            case LV_EVENT_DEFOCUSED:           return EventCode{Kind::Defocused, raw};
            case LV_EVENT_KEY:                 return EventCode{Kind::Key, raw};
            case LV_EVENT_ROTARY:              return EventCode{Kind::Rotary, raw};
            case LV_EVENT_GESTURE:             return EventCode{Kind::Gesture, raw};
            case LV_EVENT_SCROLL_BEGIN:        return EventCode{Kind::ScrollBegin, raw};
            case LV_EVENT_SCROLL:              return EventCode{Kind::Scroll, raw};
            case LV_EVENT_SCROLL_END:          return EventCode{Kind::ScrollEnd, raw};
            case LV_EVENT_SCROLL_THROW_BEGIN:  return EventCode{Kind::ScrollThrowBegin, raw};
            case LV_EVENT_CHILD_CHANGED:       return EventCode{Kind::ChildChanged, raw};
            case LV_EVENT_DELETE:              return EventCode{Kind::Delete, raw};
            case LV_EVENT_VALUE_CHANGED:       return EventCode{Kind::ValueChanged, raw};
            case LV_EVENT_READY:               return EventCode{Kind::Ready, raw};
            case LV_EVENT_CANCEL:              return EventCode{Kind::Cancel, raw};
            case LV_EVENT_INVALIDATE_AREA:     return EventCode{Kind::InvalidateArea, raw};
            default:                           return EventCode{Kind::Other, raw};
        }
    }

    [[nodiscard]] static constexpr EventCode pressed() noexcept {
        return from_lv(LV_EVENT_PRESSED);
    }

    [[nodiscard]] static constexpr EventCode clicked() noexcept {
        return from_lv(LV_EVENT_CLICKED);
    }

    [[nodiscard]] static constexpr EventCode key() noexcept {
        return from_lv(LV_EVENT_KEY);
    }

    [[nodiscard]] static constexpr EventCode rotary() noexcept {
        return from_lv(LV_EVENT_ROTARY);
    }

    [[nodiscard]] static constexpr EventCode scroll_begin() noexcept {
        return from_lv(LV_EVENT_SCROLL_BEGIN);
    }

    [[nodiscard]] static constexpr EventCode scroll() noexcept {
        return from_lv(LV_EVENT_SCROLL);
    }

    [[nodiscard]] static constexpr EventCode scroll_end() noexcept {
        return from_lv(LV_EVENT_SCROLL_END);
    }

    [[nodiscard]] constexpr Kind kind() const noexcept { return kind_; }
    [[nodiscard]] constexpr lv_event_code_t raw() const noexcept { return raw_; }
    [[nodiscard]] constexpr bool known() const noexcept { return kind_ != Kind::Other; }

    [[nodiscard]] constexpr bool operator==(const EventCode&) const noexcept = default;

private:
    constexpr EventCode(Kind kind, lv_event_code_t raw) noexcept
        : kind_{kind}, raw_{raw} {}

    Kind            kind_ = Kind::Other;
    lv_event_code_t raw_  = LV_EVENT_LAST;
};

[[nodiscard]] constexpr lv_event_code_t to_lv(EventCode code) noexcept {
    return code.raw();
}

// Thin non-owning view of an lv_event_t*.
//
// Ownership: borrows raw_ for callback duration only. The event is owned
// by LVGL's dispatch stack and this view must not outlive that callback.
class EventView {
public:
    // Args:
    //   raw: borrows LVGL event for the current callback only.
    explicit EventView(lv_event_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_event_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool        empty() const noexcept { return raw_ == nullptr; }

    [[nodiscard]] EventCode code() const noexcept;
    [[nodiscard]] lv_event_code_t raw_code() const noexcept;
    [[nodiscard]] ObjectView target() const noexcept;
    [[nodiscard]] ObjectView current_target() const noexcept;
    [[nodiscard]] void*      param() const noexcept;
    [[nodiscard]] void*      user_data() const noexcept;
    [[nodiscard]] std::uint32_t key() const noexcept;
    [[nodiscard]] std::int32_t  rotary_diff() const noexcept;

    void stop_bubbling() const noexcept;
    void stop_trickling() const noexcept;
    void stop_processing() const noexcept;

private:
    // borrows: LVGL-owned event, valid only while LVGL is invoking a callback.
    lv_event_t* raw_;
};

class EventSubscription {
public:
    EventSubscription() noexcept = default;

    // Args:
    //   object: observes LVGL object; must remain live until reset().
    //   descriptor: observes LVGL-owned event descriptor returned from
    //               lv_obj_add_event_cb().
    EventSubscription(ObjectView object, lv_event_dsc_t* descriptor) noexcept;

    EventSubscription(const EventSubscription&)            = delete;
    EventSubscription& operator=(const EventSubscription&) = delete;

    EventSubscription(EventSubscription&& other) noexcept;
    EventSubscription& operator=(EventSubscription&& other) noexcept;

    ~EventSubscription();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] lv_event_dsc_t* borrow_descriptor() const noexcept;

    // Disarms removal and returns the observed LVGL descriptor.
    // Returns: observes LVGL-owned event descriptor. Caller takes
    // responsibility for later removal if needed.
    [[nodiscard]] lv_event_dsc_t* release() noexcept;

    void reset() noexcept;

private:
    // observes: LVGL object that owns descriptor_; must outlive reset()
    // unless LVGL already deleted it, in which case lv_obj_is_valid()
    // prevents removal through a stale object pointer.
    lv_obj_t* raw_object_ = nullptr;
    // observes: LVGL-owned event descriptor; removed through
    // lv_obj_remove_event_dsc() while armed.
    lv_event_dsc_t* descriptor_ = nullptr;
};

// Args:
//   object: observes LVGL object; object must outlive returned subscription.
//   filter: LVGL event-code filter.
//   callback: external function pointer invoked by LVGL.
//   user_data: observes/external callback state; caller owns lifetime.
// Returns: owns removal responsibility through EventSubscription.
[[nodiscard]] EventSubscription add_event_callback(ObjectView object,
                                                   EventCode filter,
                                                   lv_event_cb_t callback,
                                                   void* user_data) noexcept;

[[nodiscard]] lv_result_t send_event(ObjectView object,
                                     EventCode code,
                                     void* param = nullptr) noexcept;

// Thin non-owning view of an lv_group_t*.
//
// Ownership: external. The underlying group is owned by LVGL or by an
// LvGroup owner. This view never deletes it.
class GroupView {
public:
    // Args:
    //   raw: external; owned outside this view.
    explicit GroupView(lv_group_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_group_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool        empty() const noexcept { return raw_ == nullptr; }

private:
    // external: owned by LVGL/LvGroup; this view never frees it.
    lv_group_t* raw_;
};

class LvGroup {
public:
    LvGroup() noexcept = default;

    [[nodiscard]] static LvGroup make() noexcept;

    LvGroup(const LvGroup&)            = delete;
    LvGroup& operator=(const LvGroup&) = delete;

    LvGroup(LvGroup&& other) noexcept;
    LvGroup& operator=(LvGroup&& other) noexcept;

    ~LvGroup();

    [[nodiscard]] GroupView   borrow() const noexcept;
    [[nodiscard]] lv_group_t* borrow_raw() const noexcept;
    [[nodiscard]] bool        empty() const noexcept;

    // Returns: owns raw LVGL group handle; caller is responsible for
    // its lifecycle.
    [[nodiscard]] lv_group_t* release() noexcept;

    void reset() noexcept;
    void set_default() noexcept;

    void add_object(ObjectView object) noexcept;
    void remove_object(ObjectView object) noexcept;
    void remove_all_objects() noexcept;
    void focus_object(ObjectView object) noexcept;
    void focus_next() noexcept;
    void focus_prev() noexcept;

    [[nodiscard]] ObjectView focused() const noexcept;
    [[nodiscard]] std::uint32_t object_count() const noexcept;
    [[nodiscard]] ObjectView object_by_index(std::uint32_t index) const noexcept;

    void set_editing(bool enabled) noexcept;
    [[nodiscard]] bool editing() const noexcept;
    void set_wrap(bool enabled) noexcept;
    [[nodiscard]] bool wrap() const noexcept;

private:
    explicit LvGroup(lv_group_t* raw) noexcept;

    // owns: deleted with lv_group_delete() when non-null.
    lv_group_t* raw_ = nullptr;
};

[[nodiscard]] lv_group_t* borrow_raw(GroupView group) noexcept;

enum class InputDeviceType : std::uint8_t {
    None,
    Pointer,
    Keypad,
    Button,
    Encoder,
};

enum class InputDeviceState : std::uint8_t {
    Released,
    Pressed,
};

[[nodiscard]] constexpr lv_indev_type_t to_lv(InputDeviceType type) noexcept {
    switch (type) {
        case InputDeviceType::None:    return LV_INDEV_TYPE_NONE;
        case InputDeviceType::Pointer: return LV_INDEV_TYPE_POINTER;
        case InputDeviceType::Keypad:  return LV_INDEV_TYPE_KEYPAD;
        case InputDeviceType::Button:  return LV_INDEV_TYPE_BUTTON;
        case InputDeviceType::Encoder: return LV_INDEV_TYPE_ENCODER;
    }
    return LV_INDEV_TYPE_NONE;
}

[[nodiscard]] constexpr InputDeviceType input_device_type_from_lv(
    lv_indev_type_t type) noexcept {
    switch (type) {
        case LV_INDEV_TYPE_POINTER: return InputDeviceType::Pointer;
        case LV_INDEV_TYPE_KEYPAD:  return InputDeviceType::Keypad;
        case LV_INDEV_TYPE_BUTTON:  return InputDeviceType::Button;
        case LV_INDEV_TYPE_ENCODER: return InputDeviceType::Encoder;
        case LV_INDEV_TYPE_NONE:
        default:                    return InputDeviceType::None;
    }
}

[[nodiscard]] constexpr lv_indev_state_t to_lv(InputDeviceState state) noexcept {
    return state == InputDeviceState::Pressed
        ? LV_INDEV_STATE_PRESSED
        : LV_INDEV_STATE_RELEASED;
}

[[nodiscard]] constexpr InputDeviceState input_device_state_from_lv(
    lv_indev_state_t state) noexcept {
    return state == LV_INDEV_STATE_PRESSED
        ? InputDeviceState::Pressed
        : InputDeviceState::Released;
}

// Thin non-owning view of an lv_indev_t*.
//
// Ownership: external. The underlying input device is owned by LVGL or
// by an LvInputDevice owner. This view never deletes it.
class InputDeviceView {
public:
    // Args:
    //   raw: external; owned outside this view.
    explicit InputDeviceView(lv_indev_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_indev_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool        empty() const noexcept { return raw_ == nullptr; }

private:
    // external: owned by LVGL/LvInputDevice; this view never frees it.
    lv_indev_t* raw_;
};

class LvInputDevice {
public:
    LvInputDevice() noexcept = default;

    [[nodiscard]] static LvInputDevice make(InputDeviceType type) noexcept;

    LvInputDevice(const LvInputDevice&)            = delete;
    LvInputDevice& operator=(const LvInputDevice&) = delete;

    LvInputDevice(LvInputDevice&& other) noexcept;
    LvInputDevice& operator=(LvInputDevice&& other) noexcept;

    ~LvInputDevice();

    [[nodiscard]] InputDeviceView borrow() const noexcept;
    [[nodiscard]] lv_indev_t*     borrow_raw() const noexcept;
    [[nodiscard]] bool            empty() const noexcept;

    // Returns: owns raw LVGL input-device handle; caller is responsible
    // for its lifecycle.
    [[nodiscard]] lv_indev_t* release() noexcept;

    void reset() noexcept;
    void read() noexcept;

    void set_type(InputDeviceType type) noexcept;
    [[nodiscard]] InputDeviceType type() const noexcept;
    [[nodiscard]] InputDeviceState state() const noexcept;

    // Args:
    //   callback: external function pointer. Callback state is supplied
    //             through set_user_data() or driver-owned storage.
    void set_read_callback(lv_indev_read_cb_t callback) noexcept;

    // Args:
    //   user_data: observes/external; caller guarantees lifetime until
    //              replaced or this input device is destroyed.
    void set_user_data(void* user_data) noexcept;
    [[nodiscard]] void* user_data() const noexcept;

    void set_display(DisplayView display) noexcept;
    void set_group(GroupView group) noexcept;
    [[nodiscard]] GroupView group() const noexcept;

    void set_long_press_time(std::uint16_t ms) noexcept;
    void set_long_press_repeat_time(std::uint16_t ms) noexcept;
    void reset_long_press() noexcept;
    void enable(bool enabled) noexcept;

private:
    explicit LvInputDevice(lv_indev_t* raw) noexcept;

    // owns: deleted with lv_indev_delete() when non-null.
    lv_indev_t* raw_ = nullptr;
};

[[nodiscard]] lv_indev_t* borrow_raw(InputDeviceView input) noexcept;

}  // namespace lvglpp

#endif  // LVGLPP_CORE_INPUT_HPP
