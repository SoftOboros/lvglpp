// input.hpp — focus groups, input devices, and the lv_event_t ↔ CORE-02
// Event conversion seam (LPAR-04). See docs/core-event/01-event-focus-input.md.
//
// PARITY: rlvgl/core/{focus,object dispatch} + platform input (v0.2.4 @
//         343f596). rlvgl re-implements focus/indev; lvglpp wraps LVGL's.
// LVGL:   lvgl/src/core/lv_group.h, lvgl/src/indev/lv_indev.h,
//         lvgl/src/core/lv_obj_event.h, lvgl/src/misc/lv_event.h.
// DELTA:  FocusGroup and InputDevice are move-only RAII over lv_group_t /
//         lv_indev_t (lv_group_delete / lv_indev_delete on drop). The seam is
//         a small set of free functions: it does NOT shadow lv_event_t (the
//         CORE-02 Event value type stays in event.hpp, lvgl-free); it only
//         maps between them for the representable subset (pointer/key), the
//         same subset the playit injection bridge (LVGLPP-WRAP-0N) replays.

#ifndef LVGLPP_CORE_INPUT_HPP
#define LVGLPP_CORE_INPUT_HPP

#include <cstdint>
#include <optional>

#include "lvglpp/core/event.hpp"    // CORE-02 Event / Key / TouchState
#include "lvglpp/core/object.hpp"   // ObjectView
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// ---------------------------------------------------------------------------
// FocusGroup — RAII over lv_group_t (chapter §3)
// ---------------------------------------------------------------------------

enum class FocusGroupError : std::uint8_t {
    CreateFailed = 1,  // lv_group_create returned nullptr (e.g. OOM).
};

// Ownership: owns its lv_group_t. The destructor calls lv_group_delete iff it
// still owns a group. LVGL auto-removes objects from their group when those
// objects are deleted, so a group outliving its members is safe; but an
// InputDevice bound to this group MUST NOT outlive it (unbind or destroy the
// indev first).
class FocusGroup {
public:
    [[nodiscard]] static lvglpp::expected<FocusGroup, FocusGroupError> try_make() noexcept;
    [[nodiscard]] static FocusGroup make();

    FocusGroup(const FocusGroup&)            = delete;
    FocusGroup& operator=(const FocusGroup&) = delete;

    // Move-construct only (mirrors Object). The moved-from group is emptied.
    FocusGroup(FocusGroup&& other) noexcept;
    FocusGroup& operator=(FocusGroup&&) = delete;

    ~FocusGroup();

    // All accessors below are empty-safe no-ops when this group is empty.
    // Add `obj` to this group (lv_group_add_obj).
    //   obj: borrows; the object joins this group's focus ring. LVGL drops it
    //        automatically if the object is later deleted.
    void add(ObjectView obj) noexcept;
    // Remove `obj` from whatever group it belongs to (lv_group_remove_obj is
    // object-centric in LVGL). No-op if obj is empty.
    static void remove(ObjectView obj) noexcept;
    // Focus `obj` within its group (lv_group_focus_obj, object-centric).
    static void focus(ObjectView obj) noexcept;
    // Advance/retreat focus within this group.
    void focus_next() noexcept;
    void focus_prev() noexcept;
    // Toggle edit mode (lv_group_set_editing) for encoder-style navigation.
    void set_editing(bool edit) noexcept;

    // borrows: valid only while this FocusGroup owns a live group.
    [[nodiscard]] lv_group_t* borrow_raw() const noexcept { return group_; }
    [[nodiscard]] bool        empty()      const noexcept { return group_ == nullptr; }

private:
    explicit FocusGroup(lv_group_t* group) noexcept : group_{group} {}

    lv_group_t* group_ = nullptr;  // owns: the LVGL focus group.
};

// ---------------------------------------------------------------------------
// InputDevice — RAII over lv_indev_t (chapter §3)
// ---------------------------------------------------------------------------

// Mirror of lv_indev_type_t (lvgl/src/indev/lv_indev.h). Standards Action.
enum class InputType : std::uint8_t {
    None    = LV_INDEV_TYPE_NONE,
    Pointer = LV_INDEV_TYPE_POINTER,
    Keypad  = LV_INDEV_TYPE_KEYPAD,
    Encoder = LV_INDEV_TYPE_ENCODER,
    Button  = LV_INDEV_TYPE_BUTTON,
};

enum class InputDeviceError : std::uint8_t {
    CreateFailed = 1,  // lv_indev_create returned nullptr (e.g. OOM).
};

// Ownership: owns its lv_indev_t. The destructor calls lv_indev_delete iff it
// still owns one. The read callback and any bound FocusGroup are borrowed —
// both MUST outlive this device.
class InputDevice {
public:
    [[nodiscard]] static lvglpp::expected<InputDevice, InputDeviceError> try_make() noexcept;
    [[nodiscard]] static InputDevice make();

    InputDevice(const InputDevice&)            = delete;
    InputDevice& operator=(const InputDevice&) = delete;

    InputDevice(InputDevice&& other) noexcept;
    InputDevice& operator=(InputDevice&&) = delete;

    ~InputDevice();

    // All setters below are empty-safe no-ops when this device is empty.
    void set_type(InputType type) noexcept;
    // read_cb: borrows; void(lv_indev_t*, lv_indev_data_t*). Stored by LVGL
    // and called each read cycle; MUST outlive this device.
    void set_read_cb(lv_indev_read_cb_t read_cb) noexcept;
    // group: borrows; for keypad/encoder devices (lv_indev_set_group). The
    // group MUST outlive this device.
    void set_group(const FocusGroup& group) noexcept;
    // disp: borrows; the display this device drives (lv_indev_set_display).
    void set_display(lv_display_t* disp) noexcept;
    // user_data: external; opaque pointer threaded to the read callback.
    void set_user_data(void* user_data) noexcept;

    // borrows: valid only while this InputDevice owns a live indev.
    [[nodiscard]] lv_indev_t* borrow_raw() const noexcept { return indev_; }
    [[nodiscard]] bool        empty()      const noexcept { return indev_ == nullptr; }

private:
    explicit InputDevice(lv_indev_t* indev) noexcept : indev_{indev} {}

    lv_indev_t* indev_ = nullptr;  // owns: the LVGL input device.
};

// ---------------------------------------------------------------------------
// lv_event_t ↔ CORE-02 Event conversion seam (chapter §3, §5.2)
// ---------------------------------------------------------------------------
//
// The mapping covers the representable subset: LVGL pointer/key events map to
// CORE-02 PressDown/PressRelease/PointerMove/KeyDown. CORE-02 Events without
// an lv_event_code_t (Tick, Touch frames, DoubleTap) and LVGL events without
// a CORE-02 form (ValueChanged, Focused, …) yield std::nullopt. The pointer
// subset round-trips exactly (acceptance §12); see input_test.cpp.

// Map an LVGL key code (LV_KEY_* / printable scalar) to a CORE-02 Key.
[[nodiscard]] Key key_from_lv(std::uint32_t lv_key) noexcept;
// Map a CORE-02 Key back to its LVGL key code.
[[nodiscard]] std::uint32_t lv_key_of(const Key& key) noexcept;

// Map a CORE-02 Event to the lv_event_code_t it corresponds to, or nullopt.
[[nodiscard]] std::optional<lv_event_code_t> lv_code_of(const Event& ev) noexcept;

// Build a CORE-02 Event from an event code plus the point/key payload that
// LVGL would carry, or nullopt for codes outside the representable subset.
[[nodiscard]] std::optional<Event> event_of_code(lv_event_code_t code, std::int32_t x,
                                                 std::int32_t y, std::uint32_t key) noexcept;

// Convenience over a live event during dispatch: pulls code + point (from the
// active indev) + key and calls event_of_code. nullopt if not representable.
[[nodiscard]] std::optional<Event> event_from_lv(lv_event_t* e) noexcept;

// Fill an lv_indev_data_t from a CORE-02 Event for replay through a read
// callback (the playit injection path, WRAP-0N). Returns true if `ev` was
// representable as indev data; false (and leaves *out untouched) otherwise.
//   out: borrows mut; populated on success.
[[nodiscard]] bool event_to_indev(const Event& ev, lv_indev_data_t* out) noexcept;

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_INPUT_HPP
