// label_obj_test.cpp — LVGLPP-WRAP-01 acceptance: the lv_obj-backed
// lvglpp::widgets::Label (core::Object subclass over lv_label).
//
// See docs/wrap/00-concepts.md §6 (WRAP-01).

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/widgets/label.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Object;
using lvglpp::core::Screen;
using lvglpp::widgets::Label;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// Create, set text, read it back; long-mode setter is a no-op-safe call.
void test_basic() {
    Screen screen = Screen::make();
    Label label = Label::make(screen.view());
    assert(!label.empty());

    label.set_text("hello");
    assert(std::strcmp(label.text(), "hello") == 0);

    label.set_text("world");
    assert(std::strcmp(label.text(), "world") == 0);

    label.set_long_mode(Label::LongMode::Dots);
    label.set_long_mode(Label::LongMode::Wrap);

    // Inherited Object surface works on the widget.
    label.set_size(80, 20);
    label.add_flag(lvglpp::core::ObjectFlag::Hidden);
    assert(label.has_flag(lvglpp::core::ObjectFlag::Hidden));
    label.remove_flag(lvglpp::core::ObjectFlag::Hidden);
}

// A static-text label borrows caller storage (no copy).
void test_static_text() {
    static const char kStatic[] = "static-text";
    Screen screen = Screen::make();
    Label label = Label::make(screen.view());
    label.set_text_static(kStatic);
    assert(std::strcmp(label.text(), kStatic) == 0);
}

// Delete-safety: deleting the parent must not double-free the child Label.
// The label is parented to a sub-object that we delete out from under it;
// the inherited LV_EVENT_DELETE hook nulls the handle so ~Label is a no-op.
void test_parent_delete_safety() {
    Screen screen = Screen::make();
    {
        Object panel = Object::make(screen.view());
        Label child = Label::make(panel.view());
        child.set_text("child");
        assert(!child.empty());

        // Delete the panel's lv_obj directly (LVGL recursively deletes the
        // child label). The child's delete-safety callback must fire.
        lv_obj_delete(panel.borrow_raw());
        assert(child.empty());  // handle nulled by LV_EVENT_DELETE
        // `panel` and `child` destructors now both no-op (no double free).
    }
}

// Empty (moved-from) Label: every accessor is a safe default.
void test_empty_safe() {
    Screen screen = Screen::make();
    Label label = Label::make(screen.view());
    Label moved = std::move(label);
    (void)moved;
    assert(label.empty());
    assert(std::strcmp(label.text(), "") == 0);  // empty -> ""
    label.set_text("x");                          // no-op, must not crash
    label.set_long_mode(Label::LongMode::Clip);   // no-op
}

// try_make with an empty parent fails cleanly.
void test_try_make_bad_parent() {
    auto result = Label::try_make(ObjectView{nullptr});
    assert(!result.has_value());
    assert(result.error() == lvglpp::core::ObjectError::CreateFailed);
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

    test_basic();
    test_static_text();
    test_parent_delete_safety();
    test_empty_safe();
    test_try_make_bad_parent();

    lv_display_delete(disp);
    return 0;
}
