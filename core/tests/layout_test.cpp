// layout_test.cpp - LPAR-CPP-10 acceptance for LVGL layout wrappers.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/layout.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style_lvgl.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

struct DisplayFixture {
    std::array<std::uint8_t, 240 * 160 * 4> draw_buffer{};
    lvglpp::LvDisplay display;

    DisplayFixture() : display{lvglpp::LvDisplay::make(240, 160)} {
        assert(!display.empty());
        display.set_default();
        display.set_buffers(draw_buffer.data(),
                            nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
    }
};

void load_screen(lvglpp::LvObject& screen) {
    lv_screen_load(screen.borrow_raw());
    lvglpp::remove_all_styles(screen.borrow());
    lvglpp::set_size(screen.borrow(),
                     lvglpp::SizeValue::pixels(240),
                     lvglpp::SizeValue::pixels(160));
}

void strip_default_style(lvglpp::LvObject& object) {
    lvglpp::remove_all_styles(object.borrow());
    lvglpp::set_pos(object.borrow(), 0, 0);
}

void test_value_mappings() {
    static_assert(lvglpp::to_lv(lvglpp::SizeValue::pixels(17)) == 17);
    static_assert(lvglpp::to_lv(lvglpp::SizeValue::content()) ==
                  LV_SIZE_CONTENT);
    assert(lvglpp::to_lv(lvglpp::SizeValue::percent(50)) == lv_pct(50));
    static_assert(lvglpp::to_lv(lvglpp::Align::Center) == LV_ALIGN_CENTER);
    static_assert(lvglpp::to_lv(lvglpp::LayoutKind::None) == LV_LAYOUT_NONE);
#if LV_USE_FLEX
    static_assert(lvglpp::to_lv(lvglpp::LayoutKind::Flex) == LV_LAYOUT_FLEX);
    static_assert(lvglpp::to_lv(lvglpp::FlexFlow::RowWrapReverse) ==
                  LV_FLEX_FLOW_ROW_WRAP_REVERSE);
    static_assert(lvglpp::to_lv(lvglpp::FlexAlign::SpaceBetween) ==
                  LV_FLEX_ALIGN_SPACE_BETWEEN);
#endif
#if LV_USE_GRID
    static_assert(lvglpp::to_lv(lvglpp::LayoutKind::Grid) == LV_LAYOUT_GRID);
    static_assert(lvglpp::to_lv(lvglpp::GridAlign::Stretch) ==
                  LV_GRID_ALIGN_STRETCH);
    assert(lvglpp::grid_content() == LV_GRID_CONTENT);
    assert(lvglpp::grid_fr(2) == LV_GRID_FR(2));
#endif
}

void test_geometry_size_alignment_and_content() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    load_screen(screen);
    auto parent = lvglpp::LvObject::make_child(screen.borrow());
    auto child  = lvglpp::LvObject::make_child(parent.borrow());
    strip_default_style(parent);
    strip_default_style(child);

    lvglpp::set_size(parent.borrow(),
                     lvglpp::SizeValue::pixels(200),
                     lvglpp::SizeValue::pixels(100));
    lvglpp::set_pos(parent.borrow(), 10, 20);
    lvglpp::set_size(child.borrow(),
                     lvglpp::SizeValue::percent(50),
                     lvglpp::SizeValue::content());
    lvglpp::set_pos(child.borrow(), 7, 9);
    auto grandchild = lvglpp::LvObject::make_child(child.borrow());
    strip_default_style(grandchild);
    lvglpp::set_size(grandchild.borrow(),
                     lvglpp::SizeValue::pixels(30),
                     lvglpp::SizeValue::pixels(18));
    lvglpp::update_layout(screen.borrow());

    assert(lvglpp::x(parent.borrow()) == 10);
    assert(lvglpp::y(parent.borrow()) == 20);
    assert(lvglpp::width(child.borrow()) == 100);
    assert(lvglpp::height(child.borrow()) == 18);
    assert(lvglpp::aligned_x(child.borrow()) == 7);
    assert(lvglpp::aligned_y(child.borrow()) == 9);
    assert(lvglpp::self_width(grandchild.borrow()) >= 0);
    assert(lvglpp::self_height(grandchild.borrow()) >= 0);

    lvglpp::align(child.borrow(), lvglpp::Align::Center, 3, -4);
    lvglpp::update_layout(screen.borrow());
    assert(lvglpp::aligned_x(child.borrow()) == 3);
    assert(lvglpp::aligned_y(child.borrow()) == -4);

    lvglpp::center(child.borrow());
    lvglpp::update_layout(screen.borrow());
    const lvglpp::LvArea parent_coords = lvglpp::coords(parent.borrow());
    const lvglpp::LvArea child_coords  = lvglpp::coords(child.borrow());
    assert(parent_coords.x1 == 10);
    assert(parent_coords.y1 == 20);
    assert(child_coords.x1 >= parent_coords.x1);
    assert(child_coords.y1 >= parent_coords.y1);

    lvglpp::set_content_width(parent.borrow(), 140);
    lvglpp::set_content_height(parent.borrow(), 80);
    lvglpp::update_layout(screen.borrow());
    assert(lvglpp::content_width(parent.borrow()) == 140);
    assert(lvglpp::content_height(parent.borrow()) == 80);
    const lvglpp::LvArea content = lvglpp::content_coords(parent.borrow());
    assert(content.x2 >= content.x1);
    assert(content.y2 >= content.y1);
}

void test_layout_dirty_and_style_helpers() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    load_screen(screen);
    auto object = lvglpp::LvObject::make_child(screen.borrow());
    strip_default_style(object);

    lvglpp::set_layout(object.borrow(), lvglpp::LayoutKind::None);
    lvglpp::mark_layout_dirty(object.borrow());
    lvglpp::update_layout(object.borrow());
    assert(!lvglpp::is_layout_positioned(screen.borrow()));

    lvglpp::LvStyle style;
    lvglpp::style_set_width(style, lvglpp::SizeValue::percent(75));
    lvglpp::style_set_height(style, lvglpp::SizeValue::pixels(44));
    lvglpp::style_set_align(style, lvglpp::Align::BottomRight);
    lvglpp::style_set_pad_top(style, 1);
    lvglpp::style_set_pad_bottom(style, 2);
    lvglpp::style_set_pad_left(style, 3);
    lvglpp::style_set_pad_right(style, 4);
    lvglpp::style_set_pad_radial(style, 5);
    lvglpp::style_set_row_gap(style, 6);
    lvglpp::style_set_column_gap(style, 7);
    lvglpp::style_set_margin_top(style, 8);
    lvglpp::style_set_margin_bottom(style, 9);
    lvglpp::style_set_margin_left(style, 10);
    lvglpp::style_set_margin_right(style, 11);
    lvglpp::style_set_layout(style, lvglpp::LayoutKind::None);

    lv_style_value_t value{};
    assert(style.get_prop(LV_STYLE_WIDTH, value) == LV_STYLE_RES_FOUND);
    assert(value.num == lv_pct(75));
    assert(style.get_prop(LV_STYLE_PAD_COLUMN, value) == LV_STYLE_RES_FOUND);
    assert(value.num == 7);
    assert(style.get_prop(LV_STYLE_MARGIN_RIGHT, value) == LV_STYLE_RES_FOUND);
    assert(value.num == 11);

    lvglpp::local_style_set_width(
        object.borrow(), lvglpp::SizeValue::pixels(22), lvglpp::StyleSelector{});
    lvglpp::local_style_set_height(
        object.borrow(), lvglpp::SizeValue::pixels(33), lvglpp::StyleSelector{});
    lvglpp::local_style_set_pad_all(object.borrow(), 4, lvglpp::StyleSelector{});
    lvglpp::local_style_set_margin_all(object.borrow(), 5, lvglpp::StyleSelector{});
    lvglpp::local_style_set_row_gap(object.borrow(), 6, lvglpp::StyleSelector{});
    lvglpp::local_style_set_column_gap(object.borrow(), 7, lvglpp::StyleSelector{});
    lvglpp::local_style_set_align(
        object.borrow(), lvglpp::Align::Center, lvglpp::StyleSelector{});
    lvglpp::local_style_set_layout(
        object.borrow(), lvglpp::LayoutKind::None, lvglpp::StyleSelector{});

    assert(lvglpp::local_style_prop(object.borrow(),
                                    LV_STYLE_HEIGHT,
                                    value,
                                    lvglpp::StyleSelector{}) ==
           LV_STYLE_RES_FOUND);
    assert(value.num == 33);
    assert(lvglpp::local_style_prop(object.borrow(),
                                    LV_STYLE_PAD_ROW,
                                    value,
                                    lvglpp::StyleSelector{}) ==
           LV_STYLE_RES_FOUND);
    assert(value.num == 6);
}

#if LV_USE_FLEX
void test_flex_layout() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    load_screen(screen);
    auto container = lvglpp::LvObject::make_child(screen.borrow());
    auto first     = lvglpp::LvObject::make_child(container.borrow());
    auto second    = lvglpp::LvObject::make_child(container.borrow());
    strip_default_style(container);
    strip_default_style(first);
    strip_default_style(second);

    lvglpp::set_size(container.borrow(),
                     lvglpp::SizeValue::pixels(100),
                     lvglpp::SizeValue::pixels(30));
    lvglpp::set_size(first.borrow(),
                     lvglpp::SizeValue::pixels(10),
                     lvglpp::SizeValue::pixels(10));
    lvglpp::set_size(second.borrow(),
                     lvglpp::SizeValue::pixels(10),
                     lvglpp::SizeValue::pixels(10));
    lvglpp::set_layout(container.borrow(), lvglpp::LayoutKind::Flex);
    lvglpp::set_flex_flow(container.borrow(), lvglpp::FlexFlow::Row);
    lvglpp::set_flex_align(container.borrow(),
                           lvglpp::FlexAlign::Start,
                           lvglpp::FlexAlign::Start,
                           lvglpp::FlexAlign::Start);
    lvglpp::set_flex_grow(second.borrow(), 1);
    lvglpp::update_layout(screen.borrow());

    assert(lvglpp::is_layout_positioned(first.borrow()));
    assert(lvglpp::is_layout_positioned(second.borrow()));
    assert(lvglpp::x(second.borrow()) > lvglpp::x(first.borrow()));
    assert(lvglpp::width(second.borrow()) > lvglpp::width(first.borrow()));

    lvglpp::LvStyle style;
    lvglpp::style_set_flex_flow(style, lvglpp::FlexFlow::Column);
    lvglpp::style_set_flex_main_place(style, lvglpp::FlexAlign::Center);
    lvglpp::style_set_flex_cross_place(style, lvglpp::FlexAlign::Start);
    lvglpp::style_set_flex_track_place(style, lvglpp::FlexAlign::SpaceAround);
    lvglpp::style_set_flex_grow(style, 2);
    lv_style_value_t value{};
    assert(style.get_prop(LV_STYLE_FLEX_FLOW, value) == LV_STYLE_RES_FOUND);
    assert(value.num == LV_FLEX_FLOW_COLUMN);
    assert(style.get_prop(LV_STYLE_FLEX_GROW, value) == LV_STYLE_RES_FOUND);
    assert(value.num == 2);
}
#endif

#if LV_USE_GRID
void test_grid_layout() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    load_screen(screen);
    auto container = lvglpp::LvObject::make_child(screen.borrow());
    auto child     = lvglpp::LvObject::make_child(container.borrow());
    strip_default_style(container);
    strip_default_style(child);

    const std::int32_t columns_raw[] = {lvglpp::grid_fr(1), lvglpp::grid_fr(1)};
    const std::int32_t rows_raw[]    = {30};
    lvglpp::GridTrackList columns{columns_raw};
    lvglpp::GridTrackList rows{rows_raw};
    assert(columns.track_count() == 2);
    assert(columns.values().back() == LV_GRID_TEMPLATE_LAST);
    assert(rows.track_count() == 1);

    lvglpp::set_size(container.borrow(),
                     lvglpp::SizeValue::pixels(100),
                     lvglpp::SizeValue::pixels(30));
    lvglpp::set_layout(container.borrow(), lvglpp::LayoutKind::Grid);
    lvglpp::set_grid_descriptor_array(container.borrow(), columns, rows);
    lvglpp::set_grid_align(container.borrow(),
                           lvglpp::GridAlign::Stretch,
                           lvglpp::GridAlign::Stretch);
    lvglpp::set_grid_cell(child.borrow(),
                          lvglpp::GridAlign::Stretch,
                          1,
                          1,
                          lvglpp::GridAlign::Stretch,
                          0,
                          1);
    lvglpp::update_layout(screen.borrow());

    assert(lvglpp::is_layout_positioned(child.borrow()));
    assert(lvglpp::x(child.borrow()) >= 45);
    assert(lvglpp::width(child.borrow()) >= 45);
    assert(lvglpp::height(child.borrow()) == 30);

    lvglpp::LvStyle style;
    lvglpp::style_set_grid_column_descriptor_array(
        style, lvglpp::GridTrackView{columns.borrow_raw()});
    lvglpp::style_set_grid_row_descriptor_array(
        style, lvglpp::GridTrackView{rows.borrow_raw()});
    lvglpp::style_set_grid_column_align(style, lvglpp::GridAlign::Center);
    lvglpp::style_set_grid_row_align(style, lvglpp::GridAlign::End);
    lvglpp::style_set_grid_cell_column_position(style, 1);
    lvglpp::style_set_grid_cell_column_span(style, 1);
    lvglpp::style_set_grid_cell_x_align(style, lvglpp::GridAlign::Stretch);
    lvglpp::style_set_grid_cell_row_position(style, 0);
    lvglpp::style_set_grid_cell_row_span(style, 1);
    lvglpp::style_set_grid_cell_y_align(style, lvglpp::GridAlign::Start);

    lv_style_value_t value{};
    assert(style.get_prop(LV_STYLE_GRID_COLUMN_DSC_ARRAY, value) ==
           LV_STYLE_RES_FOUND);
    assert(value.ptr == columns.borrow_raw());
    assert(style.get_prop(LV_STYLE_GRID_CELL_COLUMN_POS, value) ==
           LV_STYLE_RES_FOUND);
    assert(value.num == 1);
}
#endif

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_value_mappings();
    test_geometry_size_alignment_and_content();
    test_layout_dirty_and_style_helpers();
#if LV_USE_FLEX
    test_flex_layout();
#endif
#if LV_USE_GRID
    test_grid_layout();
#endif

    return 0;
}
