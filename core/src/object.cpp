// object.cpp — Object + Screen implementation (LVGLPP-WRAP-00).
//
// PARITY: rlvgl/core/src/widget.rs (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/core/lv_obj.c, lv_obj_tree.c, lv_obj_event.c.
// DELTA:  see object.hpp — move-only + LV_EVENT_DELETE delete-safety.

#include "lvglpp/core/object.hpp"

#include "lvglpp/core/style_cascade.hpp"  // LPAR-07: style::Style / style::Selector

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::core {

Object::Object(lv_obj_t* obj) noexcept : obj_{obj} {
    if (obj_ != nullptr) {
        // SAFETY: user_data is owned by the wrapper layer (docs/wrap §5.3).
        //   It holds a back-pointer to this Object, kept current across
        //   moves (see the move constructor). Valid until on_delete_ fires.
        lv_obj_set_user_data(obj_, this);
        lv_obj_add_event_cb(obj_, &Object::on_delete_, LV_EVENT_DELETE, nullptr);
    }
}

Object::Object(Object&& other) noexcept
    : obj_{other.obj_}, handlers_{std::move(other.handlers_)} {
    other.obj_ = nullptr;
    if (obj_ != nullptr) {
        // Rebind the back-pointer to this (new) location. The event
        // callback was registered once on the lv_obj and reads user_data
        // dynamically, so it does not need re-adding. The LPAR-04 event
        // handlers are reached via their own holder addresses (the per-cb
        // user_data), which are unchanged by moving the vector — so they
        // need no rebinding either.
        lv_obj_set_user_data(obj_, this);
    }
}

Object::~Object() {
    if (obj_ != nullptr) {
        // We still own a live object: delete it. The delete-safety
        // callback fires synchronously and nulls obj_, which is harmless
        // here.
        lv_obj_delete(obj_);
    }
}

void Object::on_delete_(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    if (obj == nullptr) {
        return;
    }
    auto* self = static_cast<Object*>(lv_obj_get_user_data(obj));
    if (self != nullptr && self->obj_ == obj) {
        self->obj_ = nullptr;
    }
}

void Object::on_event_(lv_event_t* e) {
    // SAFETY: the per-cb user_data is the EventHandler holder this Object owns
    //   (set in on(); the holder address is stable across moves). Borrowed for
    //   the duration of this call only; the Object retains ownership.
    auto* handler = static_cast<EventHandler*>(lv_event_get_user_data(e));
    if (handler != nullptr && *handler) {
        (*handler)(e);
    }
}

lvglpp::expected<Object, ObjectError> Object::try_make(ObjectView parent) noexcept {
    lv_obj_t* parent_raw = parent.borrow_raw();
    if (parent_raw == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    lv_obj_t* obj = lv_obj_create(parent_raw);
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Object{obj};
}

Object Object::make(ObjectView parent) {
    auto result = try_make(parent);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::Object::make: lv_obj_create failed");
#endif
    }
    return std::move(result.value());
}

lvglpp::expected<Screen, ObjectError> Screen::try_make() noexcept {
    if (lv_display_get_default() == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::NoDisplay};
    }
    lv_obj_t* obj = lv_obj_create(nullptr);  // parentless => screen on default display
    if (obj == nullptr) {
        return lvglpp::unexpected<ObjectError>{ObjectError::CreateFailed};
    }
    return Screen{obj};
}

Screen Screen::make() {
    auto result = try_make();
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::Screen::make: no display or lv_obj_create failed");
#endif
    }
    return std::move(result.value());
}

void Screen::load() noexcept {
    lv_obj_t* raw = borrow_raw();
    if (raw != nullptr) {
        lv_screen_load(raw);
    }
}

// --- LPAR-02: flags, state, hit-test, tree queries ---

void Object::add_flag(ObjectFlag f) noexcept {
    if (obj_ != nullptr) {
        lv_obj_add_flag(obj_, static_cast<lv_obj_flag_t>(f));
    }
}

void Object::remove_flag(ObjectFlag f) noexcept {
    if (obj_ != nullptr) {
        lv_obj_remove_flag(obj_, static_cast<lv_obj_flag_t>(f));
    }
}

bool Object::has_flag(ObjectFlag f) const noexcept {
    return obj_ != nullptr && lv_obj_has_flag(obj_, static_cast<lv_obj_flag_t>(f));
}

void Object::add_state(ObjectState s) noexcept {
    if (obj_ != nullptr) {
        lv_obj_add_state(obj_, static_cast<lv_state_t>(s));
    }
}

void Object::remove_state(ObjectState s) noexcept {
    if (obj_ != nullptr) {
        lv_obj_remove_state(obj_, static_cast<lv_state_t>(s));
    }
}

bool Object::has_state(ObjectState s) const noexcept {
    return obj_ != nullptr && lv_obj_has_state(obj_, static_cast<lv_state_t>(s));
}

ObjectState Object::state() const noexcept {
    if (obj_ == nullptr) {
        return ObjectState::Default;
    }
    return static_cast<ObjectState>(lv_obj_get_state(obj_));
}

bool Object::hit_test(std::int32_t x, std::int32_t y) const noexcept {
    if (obj_ == nullptr) {
        return false;
    }
    lv_point_t point{x, y};
    return lv_obj_hit_test(obj_, &point);
}

ObjectView Object::parent() const noexcept {
    return ObjectView{obj_ != nullptr ? lv_obj_get_parent(obj_) : nullptr};
}

std::uint32_t Object::child_count() const noexcept {
    return obj_ != nullptr ? lv_obj_get_child_count(obj_) : 0U;
}

ObjectView Object::child(std::uint32_t index) const noexcept {
    if (obj_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_obj_get_child(obj_, static_cast<std::int32_t>(index))};
}

// --- LPAR-03: invalidation ---

void Object::invalidate() noexcept {
    if (obj_ != nullptr) {
        // lv_obj_invalidate marks the area dirty; its lv_result_t is
        // best-effort (INVALID when off-screen) and intentionally ignored.
        lv_obj_invalidate(obj_);
    }
}

// --- LPAR-05: scroll ---

void Object::scroll_to(std::int32_t x, std::int32_t y, bool animate) noexcept {
    if (obj_ != nullptr) {
        lv_obj_scroll_to(obj_, x, y, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    }
}

void Object::scroll_by(std::int32_t dx, std::int32_t dy, bool animate) noexcept {
    if (obj_ != nullptr) {
        lv_obj_scroll_by(obj_, dx, dy, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    }
}

std::int32_t Object::scroll_x() const noexcept {
    return obj_ != nullptr ? lv_obj_get_scroll_x(obj_) : 0;
}

std::int32_t Object::scroll_y() const noexcept {
    return obj_ != nullptr ? lv_obj_get_scroll_y(obj_) : 0;
}

void Object::set_scroll_dir(ScrollDir dir) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_scroll_dir(obj_, static_cast<lv_dir_t>(dir));
    }
}

void Object::set_scrollbar_mode(ScrollbarMode mode) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_scrollbar_mode(obj_, static_cast<lv_scrollbar_mode_t>(mode));
    }
}

void Object::set_scroll_snap(ScrollSnap x, ScrollSnap y) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_scroll_snap_x(obj_, static_cast<lv_scroll_snap_t>(x));
        lv_obj_set_scroll_snap_y(obj_, static_cast<lv_scroll_snap_t>(y));
    }
}

// --- LPAR-10: layout & sizing ---

void Object::set_size(std::int32_t w, std::int32_t h) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_size(obj_, w, h);
    }
}

void Object::set_flex_flow(FlexFlow flow) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_flex_flow(obj_, static_cast<lv_flex_flow_t>(flow));
    }
}

void Object::set_flex_grow(std::uint8_t grow) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_flex_grow(obj_, grow);
    }
}

void Object::set_flex_align(FlexAlign main, FlexAlign cross, FlexAlign track) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_flex_align(obj_, static_cast<lv_flex_align_t>(main),
                              static_cast<lv_flex_align_t>(cross),
                              static_cast<lv_flex_align_t>(track));
    }
}

void Object::set_grid_dsc(const std::int32_t* col_dsc, const std::int32_t* row_dsc) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_grid_dsc_array(obj_, col_dsc, row_dsc);
    }
}

void Object::set_grid_cell(GridAlign col_align, std::int32_t col_pos, std::int32_t col_span,
                           GridAlign row_align, std::int32_t row_pos, std::int32_t row_span) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_grid_cell(obj_, static_cast<lv_grid_align_t>(col_align), col_pos, col_span,
                             static_cast<lv_grid_align_t>(row_align), row_pos, row_span);
    }
}

void Object::set_grid_align(GridAlign col_align, GridAlign row_align) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_grid_align(obj_, static_cast<lv_grid_align_t>(col_align),
                              static_cast<lv_grid_align_t>(row_align));
    }
}

std::int32_t Object::size_content() noexcept {
    return LV_SIZE_CONTENT;
}

std::int32_t Object::pct(std::int32_t value) noexcept {
    return lv_pct(value);
}

// --- LPAR-07: style cascade ---

void Object::add_style(style::Style& s, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        // borrows: LVGL stores s.borrow_raw() (a pointer, not a copy). The
        // caller guarantees `s` outlives this object (object.hpp / §5.1).
        lv_obj_add_style(obj_, s.borrow_raw(), sel.raw());
    }
}

void Object::remove_style(style::Style& s, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_remove_style(obj_, s.borrow_raw(), sel.raw());
    }
}

void Object::remove_all_styles() noexcept {
    if (obj_ != nullptr) {
        lv_obj_remove_style_all(obj_);
    }
}

void Object::set_local_bg_color(lv_color_t c, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_style_bg_color(obj_, c, sel.raw());
    }
}

void Object::set_local_bg_opa(lv_opa_t opa, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_style_bg_opa(obj_, opa, sel.raw());
    }
}

void Object::set_local_text_color(lv_color_t c, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_style_text_color(obj_, c, sel.raw());
    }
}

void Object::set_local_border_width(std::int32_t w, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_style_border_width(obj_, w, sel.raw());
    }
}

void Object::set_local_radius(std::int32_t r, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_style_radius(obj_, r, sel.raw());
    }
}

void Object::set_local_pad_all(std::int32_t p, style::Selector sel) noexcept {
    if (obj_ != nullptr) {
        lv_obj_set_style_pad_all(obj_, p, sel.raw());
    }
}

// --- FONT-00: text-font selection ---

void Object::set_local_text_font(const Font& font, style::Selector sel) noexcept {
    if (obj_ != nullptr && !font.empty()) {
        // borrows: LVGL stores font.borrow_raw() (the lv_font_t*, not a copy).
        // The caller guarantees the font outlives this object (FONT-00 §5.2).
        lv_obj_set_style_text_font(obj_, font.borrow_raw(), sel.raw());
    }
}

Font Object::text_font() const noexcept {
    if (obj_ == nullptr) {
        return Font{};
    }
    // Resolved effective font for the main part (cascade + theme + inherit).
    return Font{lv_obj_get_style_text_font(obj_, LV_PART_MAIN)};
}

// --- LPAR-04: per-object event callbacks ---

void Object::on(EventCode code, std::function<void(lv_event_t*)> handler) noexcept {
    if (obj_ == nullptr) {
        return;
    }
    // Heap-hold the handler so its address is stable (it is the per-cb
    // user_data). The Object owns it via handlers_. Under embedded posture an
    // allocation failure aborts (this method is noexcept), matching the
    // project's panic = abort posture.
    auto holder = std::make_unique<EventHandler>(std::move(handler));
    lv_obj_add_event_cb(obj_, &Object::on_event_, static_cast<lv_event_code_t>(code),
                        holder.get());
    handlers_.push_back(std::move(holder));
}

void Object::on(EventCode code, std::function<void()> handler) noexcept {
    // Adapt the zero-arg shape onto the lv_event_t* storage type.
    on(code, [fn = std::move(handler)](lv_event_t*) {
        if (fn) {
            fn();
        }
    });
}

// --- LVGLPP-WRAP-0N: playit tag channel ---

void Object::set_tag(const char* tag) noexcept {
    if (obj_ != nullptr && tag != nullptr) {
        lv_obj_set_name(obj_, tag);  // LVGL copies the string.
    }
}

const char* Object::tag() const noexcept {
    if (obj_ == nullptr) {
        return "";
    }
    const char* name = lv_obj_get_name(obj_);
    return name != nullptr ? name : "";
}

ObjectView Object::find_by_tag(const char* name) const noexcept {
    if (obj_ == nullptr || name == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_obj_find_by_name(obj_, name)};
}

}  // namespace lvglpp::core
