// object_scroll_test.cpp — LPAR-05 acceptance: scroll position, by/to,
// and scrollbar/snap/dir setters over lv_obj_scroll_*.
//
// Assertions check magnitudes (not LVGL's internal scroll sign) so the
// test verifies the wrapper passes arguments through, not LVGL physics.
// See docs/core-scroll/00-scroll-runtime.md §12.

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Object;
using lvglpp::core::ObjectFlag;
using lvglpp::core::ScrollbarMode;
using lvglpp::core::ScrollDir;
using lvglpp::core::ScrollSnap;

namespace {

constexpr std::int32_t iabs(std::int32_t v) { return v < 0 ? -v : v; }

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// A small scrollable viewport with content larger than itself. The content
// is an unwrapped lv_obj owned by the viewport's LVGL tree (deleted with
// the viewport), so it is not removed when this function returns.
Object make_scrollable(lv_obj_t* screen) {
    Object viewport = Object::make(ObjectView{screen});
    viewport.add_flag(ObjectFlag::Scrollable);
    viewport.set_scroll_dir(ScrollDir::All);
    lv_obj_set_size(viewport.borrow_raw(), 50, 50);

    lv_obj_t* content = lv_obj_create(viewport.borrow_raw());
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_size(content, 200, 200);
    lv_obj_update_layout(viewport.borrow_raw());
    return viewport;
}

void test_scroll_position(lv_obj_t* screen) {
    Object v = make_scrollable(screen);
    assert(v.scroll_x() == 0 && v.scroll_y() == 0);

    v.scroll_to(20, 30, false);
    assert(iabs(v.scroll_x()) == 20 && iabs(v.scroll_y()) == 30);

    v.scroll_to(0, 0, false);
    assert(v.scroll_x() == 0 && v.scroll_y() == 0);

    v.scroll_by(5, 7, false);
    assert(iabs(v.scroll_x()) == 5 && iabs(v.scroll_y()) == 7);
}

void test_setters_smoke(lv_obj_t* screen) {
    Object v = make_scrollable(screen);
    v.set_scrollbar_mode(ScrollbarMode::Off);
    v.set_scroll_snap(ScrollSnap::Center, ScrollSnap::Center);
    v.set_scroll_dir(ScrollDir::Ver);
    // No getters for mode/snap; this asserts the calls do not crash.
}

void test_empty_safe() {
    Object e = Object::make(ObjectView{lv_screen_active()});
    Object moved = std::move(e);
    (void)moved;
    assert(e.empty());
    e.scroll_to(1, 1, false);  // no-op
    e.scroll_by(1, 1, false);  // no-op
    assert(e.scroll_x() == 0 && e.scroll_y() == 0);
    e.set_scrollbar_mode(ScrollbarMode::Auto);
    e.set_scroll_snap(ScrollSnap::None, ScrollSnap::None);
    e.set_scroll_dir(ScrollDir::None);
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    static std::uint8_t draw_buf[200 * 20 * 4];
    lv_display_t* disp = lv_display_create(200, 200);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_t* screen = lv_screen_active();
    assert(screen != nullptr);

    test_scroll_position(screen);
    test_setters_smoke(screen);
    test_empty_safe();

    lv_display_delete(disp);
    return 0;
}
