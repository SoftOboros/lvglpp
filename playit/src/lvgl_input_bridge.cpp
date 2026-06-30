// lvgl_input_bridge.cpp - playit EventSpec injection into LVGL input devices.
//
// PARITY: rlvgl/playit/src/command.rs (v0.2.5 @ f999f75) for wire event
//         variants; docs/lvgl-parity/04-event-focus-input.md for the
//         LVGL-backed injection seam.
// LVGL:   lvgl/src/indev/lv_indev.c.
// DELTA:  updates synthetic LVGL read state instead of dispatching into
//         rlvgl's Rust runtime.

#include "lvglpp/playit/lvgl_input_bridge.hpp"

#include <type_traits>
#include <variant>

namespace lvglpp::playit {

void LvglInputBridge::bind_pointer(::lvglpp::InputDeviceView device) noexcept {
    pointer_ = device.borrow_raw();
    if (pointer_ != nullptr) {
        lv_indev_set_user_data(pointer_, this);
        lv_indev_set_read_cb(pointer_, read_pointer);
    }
}

void LvglInputBridge::bind_keypad(::lvglpp::InputDeviceView device) noexcept {
    keypad_ = device.borrow_raw();
    if (keypad_ != nullptr) {
        lv_indev_set_user_data(keypad_, this);
        lv_indev_set_read_cb(keypad_, read_keypad);
    }
}

void LvglInputBridge::detach_pointer() noexcept {
    if (pointer_ != nullptr) {
        lv_indev_set_read_cb(pointer_, nullptr);
        lv_indev_set_user_data(pointer_, nullptr);
    }
    pointer_ = nullptr;
}

void LvglInputBridge::detach_keypad() noexcept {
    if (keypad_ != nullptr) {
        lv_indev_set_read_cb(keypad_, nullptr);
        lv_indev_set_user_data(keypad_, nullptr);
    }
    keypad_ = nullptr;
}

bool LvglInputBridge::inject(const EventSpec& spec) noexcept {
    return std::visit([this](const auto& payload) noexcept -> bool {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, event_spec::Tick>) {
            return false;
        } else if constexpr (std::is_same_v<T, event_spec::PressRelease>) {
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_PRESSED);
            const bool pressed = read_pointer_once();
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_RELEASED);
            return read_pointer_once() || pressed;
        } else if constexpr (std::is_same_v<T, event_spec::PressDown>) {
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_PRESSED);
            return read_pointer_once();
        } else if constexpr (std::is_same_v<T, event_spec::PointerDown>) {
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_PRESSED);
            return read_pointer_once();
        } else if constexpr (std::is_same_v<T, event_spec::PointerUp>) {
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_RELEASED);
            return read_pointer_once();
        } else if constexpr (std::is_same_v<T, event_spec::PointerMove>) {
            set_pointer(payload.x, payload.y, pointer_state_);
            return read_pointer_once();
        } else if constexpr (std::is_same_v<T, event_spec::DoubleTap>) {
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_PRESSED);
            const bool first_down = read_pointer_once();
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_RELEASED);
            const bool first_up = read_pointer_once();
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_PRESSED);
            const bool second_down = read_pointer_once();
            set_pointer(payload.x, payload.y, LV_INDEV_STATE_RELEASED);
            return read_pointer_once() || first_down || first_up || second_down;
        } else if constexpr (std::is_same_v<T, event_spec::KeyDown>) {
            set_key(to_lv_key(payload.key), LV_INDEV_STATE_PRESSED);
            return read_keypad_once();
        } else if constexpr (std::is_same_v<T, event_spec::KeyUp>) {
            set_key(to_lv_key(payload.key), LV_INDEV_STATE_RELEASED);
            return read_keypad_once();
        } else if constexpr (std::is_same_v<T, event_spec::Touch>) {
            if (payload.count == 0) {
                return false;
            }
            const TouchPointSpec& point = payload.points[0];
            set_pointer(point.x, point.y, to_lv_state(point.state));
            return read_pointer_once();
        } else {
            static_assert(sizeof(T) == 0,
                "LvglInputBridge::inject(): unhandled EventSpec variant");
        }
    }, spec);
}

void LvglInputBridge::read_pointer(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* bridge = static_cast<LvglInputBridge*>(lv_indev_get_user_data(indev));
    if (bridge == nullptr || data == nullptr) {
        return;
    }

    ++bridge->pointer_reads_;
    data->state = bridge->pointer_state_;
    data->point = bridge->pointer_point_;
}

void LvglInputBridge::read_keypad(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* bridge = static_cast<LvglInputBridge*>(lv_indev_get_user_data(indev));
    if (bridge == nullptr || data == nullptr) {
        return;
    }

    ++bridge->keypad_reads_;
    data->state = bridge->key_state_;
    data->key = bridge->key_;
}

bool LvglInputBridge::read_pointer_once() noexcept {
    if (pointer_ == nullptr) {
        return false;
    }
    lv_indev_read(pointer_);
    return true;
}

bool LvglInputBridge::read_keypad_once() noexcept {
    if (keypad_ == nullptr) {
        return false;
    }
    lv_indev_read(keypad_);
    return true;
}

void LvglInputBridge::set_pointer(std::int32_t x,
                                  std::int32_t y,
                                  lv_indev_state_t state) noexcept {
    pointer_point_.x = x;
    pointer_point_.y = y;
    pointer_state_ = state;
}

void LvglInputBridge::set_key(std::uint32_t key, lv_indev_state_t state) noexcept {
    key_ = key;
    key_state_ = state;
}

std::uint32_t LvglInputBridge::to_lv_key(const KeySpec& key) noexcept {
    switch (key.kind) {
        case KeySpec::Kind::Escape:     return LV_KEY_ESC;
        case KeySpec::Kind::Enter:      return LV_KEY_ENTER;
        case KeySpec::Kind::Space:      return static_cast<std::uint32_t>(' ');
        case KeySpec::Kind::ArrowUp:    return LV_KEY_UP;
        case KeySpec::Kind::ArrowDown:  return LV_KEY_DOWN;
        case KeySpec::Kind::ArrowLeft:  return LV_KEY_LEFT;
        case KeySpec::Kind::ArrowRight: return LV_KEY_RIGHT;
        case KeySpec::Kind::Function:
        case KeySpec::Kind::Character:
        case KeySpec::Kind::Other:      return key.value;
    }
    return key.value;
}

lv_indev_state_t LvglInputBridge::to_lv_state(TouchStateSpec state) noexcept {
    switch (state) {
        case TouchStateSpec::Down:
        case TouchStateSpec::Contact: return LV_INDEV_STATE_PRESSED;
        case TouchStateSpec::Up:      return LV_INDEV_STATE_RELEASED;
    }
    return LV_INDEV_STATE_RELEASED;
}

}  // namespace lvglpp::playit
