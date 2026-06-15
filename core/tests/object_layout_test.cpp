// object_layout_test.cpp — LPAR-10 acceptance: flex flow/align/grow,
// grid descriptor/cell/align, and sizing helpers over lv_obj_set_*.
//
// See docs/core-layout/00-layout.md §12.

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::FlexAlign;
using lvglpp::core::FlexFlow;
using lvglpp::core::GridAlign;
using lvglpp::core::Object;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// A flex row lays its children out left-to-right on a common baseline.
void test_flex(lv_obj_t* screen) {
    Object c = Object::make(ObjectView{screen});
    c.set_size(200, 60);
    c.set_flex_flow(FlexFlow::Row);
    c.set_flex_align(FlexAlign::Start, FlexAlign::Center, FlexAlign::Start);

    lv_obj_t* a = lv_obj_create(c.borrow_raw());
    lv_obj_set_size(a, 30, 30);
    lv_obj_t* b = lv_obj_create(c.borrow_raw());
    lv_obj_set_size(b, 30, 30);
    lv_obj_update_layout(c.borrow_raw());

    assert(lv_obj_get_x(b) > lv_obj_get_x(a));   // row order
    assert(lv_obj_get_y(a) == lv_obj_get_y(b));  // equal-height, cross-center
}

// A grid cell placed at (col 1, row 1) sits past the first column/row.
void test_grid(lv_obj_t* screen) {
    static const std::int32_t col_dsc[] = {50, 50, LV_GRID_TEMPLATE_LAST};
    static const std::int32_t row_dsc[] = {40, 40, LV_GRID_TEMPLATE_LAST};

    Object g = Object::make(ObjectView{screen});
    g.set_size(200, 200);
    g.set_grid_dsc(col_dsc, row_dsc);

    Object cell = Object::make(g.view());
    cell.set_grid_cell(GridAlign::Stretch, 1, 1, GridAlign::Stretch, 1, 1);
    lv_obj_update_layout(g.borrow_raw());

    assert(lv_obj_get_x(cell.borrow_raw()) >= 50);  // second column
    assert(lv_obj_get_y(cell.borrow_raw()) >= 40);  // second row
}

void test_sizing() {
    assert(Object::size_content() == LV_SIZE_CONTENT);
    assert(Object::pct(50) == lv_pct(50));
}

void test_empty_safe() {
    static const std::int32_t dsc[] = {10, LV_GRID_TEMPLATE_LAST};
    Object e = Object::make(ObjectView{lv_screen_active()});
    Object moved = std::move(e);
    (void)moved;
    assert(e.empty());
    e.set_size(10, 10);                                  // no-op
    e.set_flex_flow(FlexFlow::Column);
    e.set_flex_grow(1);
    e.set_flex_align(FlexAlign::Center, FlexAlign::Center, FlexAlign::Center);
    e.set_grid_dsc(dsc, dsc);
    e.set_grid_cell(GridAlign::Start, 0, 1, GridAlign::Start, 0, 1);
    e.set_grid_align(GridAlign::Center, GridAlign::Center);
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

    test_flex(screen);
    test_grid(screen);
    test_sizing();
    test_empty_safe();

    lv_display_delete(disp);
    return 0;
}
