// object_test.cpp — LVGLPP-WRAP-00 acceptance: RAII lv_obj ownership,
// move transfer, and parent-delete double-free safety.
//
// Mirrors the ownership contract in docs/wrap/00-concepts.md §5 / §12.
// Uses the value()/has_value()/error() surface common to std::expected
// and the lvglpp polyfill (no operator-> / operator*).

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Object;
using lvglpp::core::ObjectError;
using lvglpp::core::Screen;

namespace {

// No-op flush so the throwaway test display is fully formed. It never
// fires — the test creates/deletes objects but never renders a frame.
void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// A Screen requested before any display exists fails with NoDisplay.
void test_screen_requires_display() {
    auto screen = Screen::try_make();
    assert(!screen.has_value());
    assert(screen.error() == ObjectError::NoDisplay);
}

// Dropping an owning Object deletes exactly one lv_obj (child count
// returns to its starting value).
void test_owned_drop_deletes(lv_obj_t* parent) {
    const std::uint32_t before = lv_obj_get_child_count(parent);
    {
        auto obj = Object::try_make(ObjectView{parent});
        assert(obj.has_value());
        assert(!obj.value().empty());
        assert(lv_obj_get_child_count(parent) == before + 1U);
    }
    assert(lv_obj_get_child_count(parent) == before);
}

// Move transfers ownership; the moved-from Object is empty and the moved-to
// Object deletes exactly once on drop.
void test_move_transfers(lv_obj_t* parent) {
    const std::uint32_t before = lv_obj_get_child_count(parent);
    {
        auto src = Object::try_make(ObjectView{parent});
        assert(src.has_value());
        Object dst = std::move(src.value());
        assert(src.value().empty());
        assert(!dst.empty());
        assert(lv_obj_get_child_count(parent) == before + 1U);
    }
    assert(lv_obj_get_child_count(parent) == before);
}

// Deleting a parent lv_obj out from under its wrappers nulls every
// affected wrapper (delete-safety), so their destructors are no-ops and
// nothing double-frees.
void test_parent_delete_safety(lv_obj_t* root) {
    const std::uint32_t before = lv_obj_get_child_count(root);
    {
        auto parent = Object::try_make(ObjectView{root});
        assert(parent.has_value());
        auto child = Object::try_make(parent.value().view());
        assert(child.has_value());
        assert(!parent.value().empty() && !child.value().empty());

        lv_obj_delete(parent.value().borrow_raw());  // deletes parent + child

        assert(parent.value().empty());  // delete-safety nulled the parent
        assert(child.value().empty());   // ...and the recursively-deleted child
        // scope end: both destructors see empty() and do nothing.
    }
    assert(lv_obj_get_child_count(root) == before);
}

#ifndef LVGLPP_EMBEDDED_POSTURE
// The throwing make() convenience yields an owning Object/Screen on the
// host (the abort path is exercised only on embedded targets).
void test_make_convenience(lv_obj_t* parent, lv_obj_t* original_active) {
    const std::uint32_t before = lv_obj_get_child_count(parent);
    {
        Object obj = Object::make(ObjectView{parent});
        assert(!obj.empty());
        assert(lv_obj_get_child_count(parent) == before + 1U);
    }
    assert(lv_obj_get_child_count(parent) == before);

    {
        Screen scr = Screen::make();
        assert(!scr.empty());
        scr.load();
        assert(lv_screen_active() == scr.borrow_raw());
        lv_screen_load(original_active);
    }
}
#endif

// A Screen can be created and made active.
void test_screen_create_and_load(lv_obj_t* original_active) {
    auto screen = Screen::try_make();
    assert(screen.has_value());
    assert(!screen.value().empty());

    screen.value().load();
    assert(lv_screen_active() == screen.value().borrow_raw());

    // Restore the original active screen so `screen` is safe to delete.
    lv_screen_load(original_active);
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    test_screen_requires_display();

    static std::uint8_t draw_buf[100 * 20 * 4];
    lv_display_t* disp = lv_display_create(100, 100);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_obj_t* active = lv_screen_active();
    assert(active != nullptr);

    test_owned_drop_deletes(active);
    test_move_transfers(active);
    test_parent_delete_safety(active);
    test_screen_create_and_load(active);
#ifndef LVGLPP_EMBEDDED_POSTURE
    test_make_convenience(active, active);
#endif

    lv_display_delete(disp);
    return 0;
}
