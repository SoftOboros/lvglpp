// object.hpp - move-only owner for LVGL base objects.
//
// PARITY: rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj.h and lvgl/src/core/lv_obj_tree.h.
// DELTA:  lvglpp owns real LVGL lv_obj_t nodes via RAII instead of modeling
//         an independent Rust-side object substrate.

#ifndef LVGLPP_CORE_OBJECT_HPP
#define LVGLPP_CORE_OBJECT_HPP

#include "lvglpp/core/runtime.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace lvglpp {

enum class ObjectFlag : std::uint32_t {
    Hidden         = static_cast<std::uint32_t>(LV_OBJ_FLAG_HIDDEN),
    Clickable      = static_cast<std::uint32_t>(LV_OBJ_FLAG_CLICKABLE),
    ClickFocusable = static_cast<std::uint32_t>(LV_OBJ_FLAG_CLICK_FOCUSABLE),
    Checkable      = static_cast<std::uint32_t>(LV_OBJ_FLAG_CHECKABLE),
    Scrollable     = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLLABLE),
    ScrollElastic  = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_ELASTIC),
    ScrollMomentum = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_MOMENTUM),
    ScrollOne      = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_ONE),
    ScrollChainHorizontal = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_CHAIN_HOR),
    ScrollChainVertical   = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_CHAIN_VER),
    ScrollChain    = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_CHAIN),
    ScrollOnFocus  = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_ON_FOCUS),
    ScrollWithArrow = static_cast<std::uint32_t>(LV_OBJ_FLAG_SCROLL_WITH_ARROW),
    Snappable      = static_cast<std::uint32_t>(LV_OBJ_FLAG_SNAPPABLE),
    EventBubble    = static_cast<std::uint32_t>(LV_OBJ_FLAG_EVENT_BUBBLE),
    EventTrickle   = static_cast<std::uint32_t>(LV_OBJ_FLAG_EVENT_TRICKLE),
    Floating       = static_cast<std::uint32_t>(LV_OBJ_FLAG_FLOATING),
};

enum class ObjectState : std::uint16_t {
    Default  = static_cast<std::uint16_t>(LV_STATE_DEFAULT),
    Checked  = static_cast<std::uint16_t>(LV_STATE_CHECKED),
    Focused  = static_cast<std::uint16_t>(LV_STATE_FOCUSED),
    FocusKey = static_cast<std::uint16_t>(LV_STATE_FOCUS_KEY),
    Edited   = static_cast<std::uint16_t>(LV_STATE_EDITED),
    Hovered  = static_cast<std::uint16_t>(LV_STATE_HOVERED),
    Pressed  = static_cast<std::uint16_t>(LV_STATE_PRESSED),
    Disabled = static_cast<std::uint16_t>(LV_STATE_DISABLED),
    Scrolled = static_cast<std::uint16_t>(LV_STATE_SCROLLED),
};

[[nodiscard]] constexpr lv_obj_flag_t to_lv(ObjectFlag flag) noexcept {
    return static_cast<lv_obj_flag_t>(flag);
}

[[nodiscard]] constexpr lv_state_t to_lv(ObjectState state) noexcept {
    return static_cast<lv_state_t>(state);
}

// Move-only RAII owner for an LVGL object.
//
// Ownership: owns raw_ while non-null and valid. Destruction deletes with
// lv_obj_delete(). release() transfers the raw handle to the caller and
// disarms deletion.
class LvObject {
public:
    LvObject() noexcept = default;

    [[nodiscard]] static LvObject make_screen() noexcept;
    [[nodiscard]] static LvObject make_child(ObjectView parent) noexcept;
    [[nodiscard]] static LvObject make() noexcept;
    [[nodiscard]] static LvObject make(ObjectView parent) noexcept;

    LvObject(const LvObject&)            = delete;
    LvObject& operator=(const LvObject&) = delete;

    LvObject(LvObject&& other) noexcept;
    LvObject& operator=(LvObject&& other) noexcept;

    ~LvObject();

    [[nodiscard]] ObjectView borrow() const noexcept;
    [[nodiscard]] ObjectView view() const noexcept;
    [[nodiscard]] lv_obj_t*  borrow_raw() const noexcept;
    [[nodiscard]] bool       empty() const noexcept;
    [[nodiscard]] bool       valid() const noexcept;

    // Returns: owns raw LVGL handle; caller is responsible for its lifecycle.
    [[nodiscard]] lv_obj_t* release() noexcept;

    void reset() noexcept;

    [[nodiscard]] ObjectView   parent() const noexcept;
    [[nodiscard]] std::uint32_t child_count() const noexcept;
    [[nodiscard]] ObjectView   child(std::int32_t index) const noexcept;

    void set_parent(ObjectView parent) noexcept;
    void set_size(std::int32_t width, std::int32_t height) noexcept;
#if LV_USE_FLEX
    void set_flex_flow(lv_flex_flow_t flow) noexcept;
#endif
    void move_to_index(std::int32_t index) noexcept;
    void raise_to_front() noexcept;
    void lower_to_back() noexcept;
    void clean_children() noexcept;

    void add_flag(ObjectFlag flag) noexcept;
    void remove_flag(ObjectFlag flag) noexcept;
    void set_flag(ObjectFlag flag, bool enabled) noexcept;
    [[nodiscard]] bool has_flag(ObjectFlag flag) const noexcept;

    void add_state(ObjectState state) noexcept;
    void remove_state(ObjectState state) noexcept;
    void set_state(ObjectState state, bool enabled) noexcept;
    [[nodiscard]] bool has_state(ObjectState state) const noexcept;

    // --- LVGLPP-WRAP-0N: playit tag channel (over the lv_obj name) ---
    // The LVGL object name doubles as the playit tag (WRAP-00 §5.4 reserves
    // the name field for exactly this). set_tag copies the string
    // (lv_obj_set_name), so `tag` need not outlive the call. No-op when empty.
    void set_tag(const char* tag) noexcept;
    // The object's tag, or "" if unset / empty (lv_obj_get_name).
    [[nodiscard]] const char* tag() const noexcept;
    // Breadth-first search this object's subtree for a descendant whose tag
    // matches `name` (lv_obj_find_by_name). Does NOT match this object itself.
    // Returns an empty ObjectView if not found or this Object is empty.
    [[nodiscard]] ObjectView find_by_tag(const char* name) const noexcept;

    // Args:
    //   handler: owns callable; held until reset/destruction.
    void on(lv_event_code_t code, std::function<void(lv_event_t*)> handler) noexcept;
    void on(lv_event_code_t code, std::function<void()> handler) noexcept;

protected:
    explicit LvObject(lv_obj_t* raw) noexcept;

private:
    struct EventHandler {
        explicit EventHandler(std::function<void(lv_event_t*)> fn_in)
            : fn{std::move(fn_in)} {}

        // owns: callback invoked by LVGL while registered on raw_.
        std::function<void(lv_event_t*)> fn;
    };

    static void handle_event(lv_event_t* event) noexcept;
    static void handle_delete(lv_event_t* event) noexcept;

    void install_delete_hook() noexcept;
    void remove_delete_hook() noexcept;
    void rebind_delete_hook(lv_obj_t** previous_raw_slot) noexcept;

    // owns: deleted with lv_obj_delete() when non-null and still valid.
    // Parent/subtree deletion can invalidate this pointer first; valid()
    // checks LVGL's registry before destructor deletion.
    lv_obj_t* raw_ = nullptr;
    // owns: event callback holders whose addresses are stored as LVGL
    // user_data. Unique ownership keeps callback lifetime single-writer.
    std::vector<std::unique_ptr<EventHandler>> event_handlers_{};
};

}  // namespace lvglpp

namespace lvglpp::core {

enum class ObjectError : std::uint8_t {
    CreateFailed,
};

using Object = ::lvglpp::LvObject;
using Screen = ::lvglpp::LvObject;
using ObjectFlag = ::lvglpp::ObjectFlag;
using ObjectState = ::lvglpp::ObjectState;
#if LV_USE_FLEX
using FlexFlow = lv_flex_flow_t;
#endif

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_OBJECT_HPP
