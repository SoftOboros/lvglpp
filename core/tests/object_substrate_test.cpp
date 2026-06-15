// object_substrate_test.cpp — LPAR-02 acceptance: object flags, state,
// hit-test, and non-owning tree queries over lv_obj_*.
//
// See docs/core-object/00-object-substrate.md §12.

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Object;
using lvglpp::core::ObjectFlag;
using lvglpp::core::ObjectState;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

void test_flags(lv_obj_t* screen) {
    Object o = Object::make(ObjectView{screen});
    assert(!o.has_flag(ObjectFlag::Hidden));
    o.add_flag(ObjectFlag::Hidden);
    assert(o.has_flag(ObjectFlag::Hidden));
    o.remove_flag(ObjectFlag::Hidden);
    assert(!o.has_flag(ObjectFlag::Hidden));
}

void test_state(lv_obj_t* screen) {
    Object o = Object::make(ObjectView{screen});
    assert(!o.has_state(ObjectState::Disabled));
    o.add_state(ObjectState::Disabled);
    assert(o.has_state(ObjectState::Disabled));
    assert((static_cast<std::uint16_t>(o.state()) &
            static_cast<std::uint16_t>(ObjectState::Disabled)) != 0U);
    o.remove_state(ObjectState::Disabled);
    assert(!o.has_state(ObjectState::Disabled));
}

void test_hit_test(lv_obj_t* screen) {
    Object o = Object::make(ObjectView{screen});
    lv_obj_set_pos(o.borrow_raw(), 10, 20);
    lv_obj_set_size(o.borrow_raw(), 30, 40);
    lv_obj_update_layout(o.borrow_raw());

    lv_area_t area;
    lv_obj_get_coords(o.borrow_raw(), &area);
    const std::int32_t cx = (area.x1 + area.x2) / 2;
    const std::int32_t cy = (area.y1 + area.y2) / 2;

    assert(o.hit_test(cx, cy));                          // inside
    assert(!o.hit_test(area.x2 + 1000, area.y2 + 1000)); // far outside
}

void test_tree(lv_obj_t* screen) {
    Object parent = Object::make(ObjectView{screen});
    assert(parent.child_count() == 0U);

    Object c0 = Object::make(parent.view());
    Object c1 = Object::make(parent.view());
    assert(parent.child_count() == 2U);
    assert(parent.child(0U).borrow_raw() == c0.borrow_raw());
    assert(parent.child(1U).borrow_raw() == c1.borrow_raw());
    assert(parent.child(99U).empty());  // out of range -> empty view

    assert(c0.parent().borrow_raw() == parent.borrow_raw());
}

// Empty-Object safety: every accessor returns a safe default.
void test_empty_object_safe() {
    Object empty{Object::make(ObjectView{lv_screen_active()})};
    // Move out to leave `empty` in the empty state.
    Object moved = std::move(empty);
    (void)moved;
    assert(empty.empty());
    empty.add_flag(ObjectFlag::Hidden);          // no-op, must not crash
    assert(!empty.has_flag(ObjectFlag::Hidden));
    assert(empty.state() == ObjectState::Default);
    assert(!empty.hit_test(0, 0));
    assert(empty.parent().empty());
    assert(empty.child_count() == 0U);
    assert(empty.child(0U).empty());
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    static std::uint8_t draw_buf[100 * 20 * 4];
    lv_display_t* disp = lv_display_create(100, 100);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_t* screen = lv_screen_active();
    assert(screen != nullptr);

    test_flags(screen);
    test_state(screen);
    test_hit_test(screen);
    test_tree(screen);
    test_empty_object_safe();

    lv_display_delete(disp);
    return 0;
}
