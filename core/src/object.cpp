// object.cpp - LVGL-backed object owner implementation.
//
// PARITY: rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/include/lvgl/core/lv_obj.h and lvgl/src/core/lv_obj_tree.h.
// DELTA:  delegates object storage and tree semantics to LVGL.

#include "lvglpp/core/object.hpp"

namespace lvglpp {

namespace {

// The published ObjectFlag values retain their LVGL bit values. Delegate
// each requested property to LVGL's v9.6 setters/getters; LVGL owns storage
// and side effects. ScrollChain is the combination of horizontal/vertical.
struct FlagAccess {
    ObjectFlag flag;
    // external: function address has static program lifetime; borrows object for call.
    void (*set)(lv_obj_t*, bool);
    // external: function address has static program lifetime; observes object for call.
    bool (*get)(const lv_obj_t*);
};

constexpr FlagAccess flag_access[] = {
    {ObjectFlag::Hidden, lv_obj_set_hidden, lv_obj_is_hidden},
    {ObjectFlag::Clickable, lv_obj_set_clickable, lv_obj_is_clickable},
    {ObjectFlag::ClickFocusable, lv_obj_set_click_focusable, lv_obj_is_click_focusable},
    {ObjectFlag::Checkable, lv_obj_set_checkable, lv_obj_is_checkable},
    {ObjectFlag::Scrollable, lv_obj_set_scrollable, lv_obj_is_scrollable},
    {ObjectFlag::ScrollElastic, lv_obj_set_scroll_elastic, lv_obj_is_scroll_elastic},
    {ObjectFlag::ScrollMomentum, lv_obj_set_scroll_momentum, lv_obj_is_scroll_momentum},
    {ObjectFlag::ScrollOne, lv_obj_set_scroll_one, lv_obj_is_scroll_one},
    {ObjectFlag::ScrollChainHorizontal, lv_obj_set_scroll_chain_hor, lv_obj_is_scroll_chain_hor},
    {ObjectFlag::ScrollChainVertical, lv_obj_set_scroll_chain_ver, lv_obj_is_scroll_chain_ver},
    {ObjectFlag::ScrollOnFocus, lv_obj_set_scroll_on_focus, lv_obj_is_scroll_on_focus},
    {ObjectFlag::ScrollWithArrow, lv_obj_set_scroll_with_arrow, lv_obj_is_scroll_with_arrow},
    {ObjectFlag::Snappable, lv_obj_set_snappable, lv_obj_is_snappable},
    {ObjectFlag::EventBubble, lv_obj_set_event_bubble, lv_obj_is_event_bubble},
    {ObjectFlag::EventTrickle, lv_obj_set_event_trickle, lv_obj_is_event_trickle},
    {ObjectFlag::Floating, lv_obj_set_floating, lv_obj_is_floating},
};

// Args: raw borrows a live LVGL object for this call; no ownership transfer.
void borrow_update_flags(lv_obj_t* raw, ObjectFlag flags, bool enabled) noexcept {
    const auto mask = static_cast<std::uint32_t>(flags);
    for (const auto& access : flag_access) {
        if ((mask & static_cast<std::uint32_t>(access.flag)) != 0) {
            access.set(raw, enabled);
        }
    }
}

// Args: raw observes a live LVGL object for this call; no ownership transfer.
[[nodiscard]] bool view_flags(const lv_obj_t* raw, ObjectFlag flags) noexcept {
    auto remaining = static_cast<std::uint32_t>(flags);
    for (const auto& access : flag_access) {
        const auto bit = static_cast<std::uint32_t>(access.flag);
        if ((remaining & bit) != 0) {
            if (!access.get(raw)) {
                return false;
            }
            remaining &= ~bit;
        }
    }
    return remaining == 0;
}

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
        borrow_update_flags(raw_, flag, true);
    }
}

void LvObject::remove_flag(ObjectFlag flag) noexcept {
    if (is_live(raw_)) {
        borrow_update_flags(raw_, flag, false);
    }
}

void LvObject::set_flag(ObjectFlag flag, bool enabled) noexcept {
    if (is_live(raw_)) {
        borrow_update_flags(raw_, flag, enabled);
    }
}

bool LvObject::has_flag(ObjectFlag flag) const noexcept {
    return is_live(raw_) && view_flags(raw_, flag);
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
