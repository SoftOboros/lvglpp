// lvgl_input_bridge.hpp - playit EventSpec injection into LVGL input devices.
//
// PARITY: rlvgl/playit/src/command.rs (v0.2.5 @ f999f75) for wire event
//         variants; docs/lvgl-parity/04-event-focus-input.md for the
//         LVGL-backed injection seam.
// LVGL:   lvgl/src/indev/lv_indev.h.
// DELTA:  rlvgl dispatches into its Rust runtime; lvglpp updates
//         synthetic LVGL input-device read state and asks LVGL to read it.

#ifndef LVGLPP_PLAYIT_LVGL_INPUT_BRIDGE_HPP
#define LVGLPP_PLAYIT_LVGL_INPUT_BRIDGE_HPP

#include "lvglpp/core/input.hpp"
#include "lvglpp/playit/event_spec.hpp"

#include <cstdint>

namespace lvglpp::playit {

class LvglInputBridge {
public:
    LvglInputBridge() noexcept = default;

    LvglInputBridge(const LvglInputBridge&)            = delete;
    LvglInputBridge& operator=(const LvglInputBridge&) = delete;

    LvglInputBridge(LvglInputBridge&&)            = delete;
    LvglInputBridge& operator=(LvglInputBridge&&) = delete;

    // Args:
    //   device: observes LVGL pointer device. The device must outlive
    //           this bridge or be detached before destruction.
    void bind_pointer(::lvglpp::InputDeviceView device) noexcept;

    // Args:
    //   device: observes LVGL keypad device. The device must outlive
    //           this bridge or be detached before destruction.
    void bind_keypad(::lvglpp::InputDeviceView device) noexcept;

    void detach_pointer() noexcept;
    void detach_keypad() noexcept;

    // Args:
    //   spec: borrows playit event spec for the duration of the call.
    // Returns: true when the event was consumed by a bound LVGL input
    //          device; false when no matching device is bound or the
    //          event has no LVGL input equivalent.
    [[nodiscard]] bool inject(const EventSpec& spec) noexcept;

    [[nodiscard]] int pointer_reads() const noexcept { return pointer_reads_; }
    [[nodiscard]] int keypad_reads() const noexcept { return keypad_reads_; }

private:
    static void read_pointer(lv_indev_t* indev, lv_indev_data_t* data);
    static void read_keypad(lv_indev_t* indev, lv_indev_data_t* data);

    [[nodiscard]] bool read_pointer_once() noexcept;
    [[nodiscard]] bool read_keypad_once() noexcept;

    void set_pointer(std::int32_t x,
                     std::int32_t y,
                     lv_indev_state_t state) noexcept;
    void set_key(std::uint32_t key, lv_indev_state_t state) noexcept;

    [[nodiscard]] static std::uint32_t to_lv_key(const KeySpec& key) noexcept;
    [[nodiscard]] static lv_indev_state_t to_lv_state(TouchStateSpec state) noexcept;

    // observes: LVGL input device owned outside this bridge.
    lv_indev_t* pointer_ = nullptr;
    // observes: LVGL input device owned outside this bridge.
    lv_indev_t* keypad_ = nullptr;

    // owns: synthetic pointer read state borrowed by LVGL during callbacks.
    lv_indev_state_t pointer_state_ = LV_INDEV_STATE_RELEASED;
    lv_point_t pointer_point_{0, 0};

    // owns: synthetic key read state borrowed by LVGL during callbacks.
    lv_indev_state_t key_state_ = LV_INDEV_STATE_RELEASED;
    std::uint32_t key_ = 0;

    int pointer_reads_ = 0;
    int keypad_reads_ = 0;
};

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_LVGL_INPUT_BRIDGE_HPP
