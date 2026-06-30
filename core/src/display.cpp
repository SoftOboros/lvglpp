// display.cpp - LVGL-backed display owner and invalidation helpers.
//
// PARITY: rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/display/lv_display.c and lvgl/src/core/lv_obj_pos.c.
// DELTA:  delegates dirty planning and flush cadence to LVGL.

#include "lvglpp/core/display.hpp"

namespace lvglpp {

LvDisplay::LvDisplay(lv_display_t* raw) noexcept : raw_{raw} {}

LvDisplay LvDisplay::make(std::int32_t width, std::int32_t height) noexcept {
    return LvDisplay{lv_display_create(width, height)};
}

LvDisplay::LvDisplay(LvDisplay&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvDisplay& LvDisplay::operator=(LvDisplay&& other) noexcept {
    if (this != &other) {
        reset();
        raw_       = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvDisplay::~LvDisplay() {
    reset();
}

DisplayView LvDisplay::borrow() const noexcept {
    return DisplayView{raw_};
}

lv_display_t* LvDisplay::borrow_raw() const noexcept {
    return raw_;
}

bool LvDisplay::empty() const noexcept {
    return raw_ == nullptr;
}

lv_display_t* LvDisplay::release() noexcept {
    lv_display_t* released = raw_;
    raw_                   = nullptr;
    return released;
}

void LvDisplay::reset() noexcept {
    if (raw_ != nullptr) {
        lv_display_delete(raw_);
        raw_ = nullptr;
    }
}

void LvDisplay::set_default() noexcept {
    if (raw_ != nullptr) {
        lv_display_set_default(raw_);
    }
}

void LvDisplay::set_buffers(void* buffer1,
                            void* buffer2,
                            std::uint32_t byte_size,
                            lv_display_render_mode_t render_mode) noexcept {
    if (raw_ != nullptr) {
        lv_display_set_buffers(raw_, buffer1, buffer2, byte_size, render_mode);
    }
}

void LvDisplay::set_flush_callback(lv_display_flush_cb_t callback) noexcept {
    if (raw_ != nullptr) {
        lv_display_set_flush_cb(raw_, callback);
    }
}

lv_display_t* borrow_raw(DisplayView display) noexcept {
    return display.borrow_raw();
}

lv_result_t invalidate(ObjectView object) noexcept {
    if (object.empty()) {
        return LV_RESULT_INVALID;
    }
    return lv_obj_invalidate(object.borrow_raw());
}

lv_result_t invalidate_area(ObjectView object, LvArea area) noexcept {
    if (object.empty()) {
        return LV_RESULT_INVALID;
    }
    const lv_area_t lv_area = area.to_lv();
    return lv_obj_invalidate_area(object.borrow_raw(), &lv_area);
}

}  // namespace lvglpp
