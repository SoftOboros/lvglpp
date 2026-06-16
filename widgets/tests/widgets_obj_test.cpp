// widgets_obj_test.cpp — LVGLPP-WRAP-02..06 acceptance: the lv_obj-backed
// Button/Checkbox/Switch/Slider/Container/List/Image (core::Object subclasses).
//
// See docs/wrap/00-concepts.md §6.

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/widgets/button.hpp"
#include "lvglpp/widgets/checkbox.hpp"
#include "lvglpp/widgets/container.hpp"
#include "lvglpp/widgets/image.hpp"
#include "lvglpp/widgets/label.hpp"
#include "lvglpp/widgets/list.hpp"
#include "lvglpp/widgets/slider.hpp"
#include "lvglpp/widgets/switch.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::Screen;
namespace lw = lvglpp::widgets;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

void test_button(const Screen& screen) {
    lw::Button btn = lw::Button::make(screen.view());
    assert(std::strcmp(btn.text(), "") == 0);  // no label yet
    btn.set_text("OK");
    assert(std::strcmp(btn.text(), "OK") == 0);

    int clicks = 0;
    btn.set_on_click([&clicks] { ++clicks; });
    lv_obj_send_event(btn.borrow_raw(), LV_EVENT_CLICKED, nullptr);
    assert(clicks == 1);

    // Click handler survives a move (holder address stable; no `this` capture).
    lw::Button moved = std::move(btn);
    lv_obj_send_event(moved.borrow_raw(), LV_EVENT_CLICKED, nullptr);
    assert(clicks == 2);
}

void test_checkbox(const Screen& screen) {
    lw::Checkbox cb = lw::Checkbox::make(screen.view());
    cb.set_text("Accept");
    assert(std::strcmp(cb.text(), "Accept") == 0);
    assert(!cb.is_checked());
    cb.set_checked(true);
    assert(cb.is_checked());

    bool last = false;
    int hits = 0;
    cb.set_on_change([&](bool checked) { last = checked; ++hits; });
    // The handler reads checked from the event target — move-safe.
    lw::Checkbox moved = std::move(cb);
    lv_obj_send_event(moved.borrow_raw(), LV_EVENT_VALUE_CHANGED, nullptr);
    assert(hits == 1 && last == true);  // still checked
    moved.set_checked(false);
    lv_obj_send_event(moved.borrow_raw(), LV_EVENT_VALUE_CHANGED, nullptr);
    assert(hits == 2 && last == false);
}

void test_switch(const Screen& screen) {
    lw::Switch sw = lw::Switch::make(screen.view());
    assert(!sw.is_on());
    sw.set_on(true);
    assert(sw.is_on());

    bool last = true;
    sw.set_on_change([&last](bool on) { last = on; });
    sw.set_on(false);
    lv_obj_send_event(sw.borrow_raw(), LV_EVENT_VALUE_CHANGED, nullptr);
    assert(last == false);
}

void test_slider(const Screen& screen) {
    lw::Slider sl = lw::Slider::make(screen.view());
    sl.set_range(0, 200);
    assert(sl.min() == 0 && sl.max() == 200);
    sl.set_value(150, false);
    assert(sl.value() == 150);

    std::int32_t last = -1;
    sl.set_on_change([&last](std::int32_t v) { last = v; });
    sl.set_value(75, false);
    lv_obj_send_event(sl.borrow_raw(), LV_EVENT_VALUE_CHANGED, nullptr);
    assert(last == 75);
}

void test_container(const Screen& screen) {
    lw::Container c = lw::Container::make(screen.view());
    assert(!c.empty());
    // Inherited core::Object surface works.
    c.set_size(120, 80);
    c.set_flex_flow(LV_FLEX_FLOW_COLUMN);
    lw::Label child = lw::Label::make(c.view());
    child.set_text("inside");
    assert(c.child_count() == 1U);
}

void test_list(const Screen& screen) {
    lw::List list = lw::List::make(screen.view());
    ObjectView text_row = list.add_text("Section");
    assert(!text_row.empty());
    ObjectView btn_row = list.add_button("Item 1");
    assert(!btn_row.empty());
    assert(std::strcmp(list.button_text(btn_row), "Item 1") == 0);
}

void test_image(const Screen& screen) {
    lw::Image img = lw::Image::make(screen.view());
    assert(!img.empty());
    img.set_src(nullptr);        // no-op, must not crash
    img.set_src(LV_SYMBOL_OK);   // symbol source (a string) — safe without render
}

void test_empty_safe() {
    Screen screen = Screen::make();
    lw::Slider sl = lw::Slider::make(screen.view());
    lw::Slider moved = std::move(sl);
    (void)moved;
    assert(sl.empty());
    assert(sl.value() == 0);
    sl.set_value(5, false);  // no-op
    sl.set_range(0, 10);     // no-op
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

    Screen screen = Screen::make();
    test_button(screen);
    test_checkbox(screen);
    test_switch(screen);
    test_slider(screen);
    test_container(screen);
    test_list(screen);
    test_image(screen);
    test_empty_safe();

    lv_display_delete(disp);
    return 0;
}
