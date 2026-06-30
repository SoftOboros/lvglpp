// layout.cpp - LVGL-backed geometry, flex, and grid wrapper implementation.
//
// PARITY: rlvgl/docs/concepts/LPAR-10-LAYOUT.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj_pos.c and lvgl/src/layouts/.
// DELTA:  delegates geometry and layout computation to LVGL.

#include "lvglpp/core/layout.hpp"

#include "lvglpp/core/style_lvgl.hpp"

namespace lvglpp {

namespace {

[[nodiscard]] lv_obj_t* raw_or_null(ObjectView object) noexcept {
    return object.empty() ? nullptr : object.borrow_raw();
}

void set_style_num(LvStyle& style,
                   lv_style_prop_t prop,
                   std::int32_t value) noexcept {
    lv_style_value_t style_value{};
    style_value.num = value;
    style.set_prop(prop, style_value);
}

void set_style_ptr(LvStyle& style,
                   lv_style_prop_t prop,
                   const void* value) noexcept {
    lv_style_value_t style_value{};
    style_value.ptr = value;
    style.set_prop(prop, style_value);
}

void set_local_num(ObjectView object,
                   lv_style_prop_t prop,
                   std::int32_t value,
                   StyleSelector selector) noexcept {
    lv_style_value_t style_value{};
    style_value.num = value;
    set_local_style_prop(object, prop, style_value, selector);
}

}  // namespace

SizeValue SizeValue::percent(std::int32_t value) noexcept {
    return SizeValue{lv_pct(value)};
}

#if LV_USE_GRID
std::int32_t grid_fr(std::uint8_t factor) noexcept {
    return lv_grid_fr(factor);
}

GridTrackList::GridTrackList(std::span<const std::int32_t> tracks) {
    assign(tracks);
}

GridTrackList GridTrackList::from_tracks(
    std::span<const std::int32_t> tracks) {
    return GridTrackList{tracks};
}

void GridTrackList::assign(std::span<const std::int32_t> tracks) {
    tracks_.assign(tracks.begin(), tracks.end());
    if (tracks_.empty() || tracks_.back() != LV_GRID_TEMPLATE_LAST) {
        tracks_.push_back(LV_GRID_TEMPLATE_LAST);
    }
}

void GridTrackList::clear() noexcept {
    tracks_.assign(1, LV_GRID_TEMPLATE_LAST);
}

const std::int32_t* GridTrackList::borrow_raw() const noexcept {
    return tracks_.data();
}

std::span<const std::int32_t> GridTrackList::values() const noexcept {
    return tracks_;
}

bool GridTrackList::empty() const noexcept {
    return track_count() == 0;
}

std::size_t GridTrackList::track_count() const noexcept {
    return tracks_.empty() ? 0 : tracks_.size() - 1;
}
#endif

void set_pos(ObjectView object, std::int32_t x, std::int32_t y) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_pos(raw, x, y);
    }
}

void set_x(ObjectView object, std::int32_t x) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_x(raw, x);
    }
}

void set_y(ObjectView object, std::int32_t y) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_y(raw, y);
    }
}

void set_size(ObjectView object, SizeValue width, SizeValue height) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_size(raw, to_lv(width), to_lv(height));
    }
}

void set_width(ObjectView object, SizeValue width) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_width(raw, to_lv(width));
    }
}

void set_height(ObjectView object, SizeValue height) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_height(raw, to_lv(height));
    }
}

void set_content_width(ObjectView object, std::int32_t width) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_content_width(raw, width);
    }
}

void set_content_height(ObjectView object, std::int32_t height) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_content_height(raw, height);
    }
}

std::int32_t x(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_x(raw);
    }
    return 0;
}

std::int32_t y(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_y(raw);
    }
    return 0;
}

std::int32_t x2(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_x2(raw);
    }
    return 0;
}

std::int32_t y2(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_y2(raw);
    }
    return 0;
}

std::int32_t aligned_x(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_x_aligned(raw);
    }
    return 0;
}

std::int32_t aligned_y(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_y_aligned(raw);
    }
    return 0;
}

std::int32_t width(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_width(raw);
    }
    return 0;
}

std::int32_t height(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_height(raw);
    }
    return 0;
}

std::int32_t content_width(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_content_width(raw);
    }
    return 0;
}

std::int32_t content_height(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_content_height(raw);
    }
    return 0;
}

std::int32_t self_width(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_self_width(raw);
    }
    return 0;
}

std::int32_t self_height(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_self_height(raw);
    }
    return 0;
}

LvArea coords(ObjectView object) noexcept {
    lv_area_t area{};
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_get_coords(raw, &area);
    }
    return LvArea::from_lv(area);
}

LvArea content_coords(ObjectView object) noexcept {
    lv_area_t area{};
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_get_content_coords(raw, &area);
    }
    return LvArea::from_lv(area);
}

void set_align(ObjectView object, Align align) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_align(raw, to_lv(align));
    }
}

void align(ObjectView object,
           Align align,
           std::int32_t x_offset,
           std::int32_t y_offset) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_align(raw, to_lv(align), x_offset, y_offset);
    }
}

void align_to(ObjectView object,
              ObjectView base,
              Align align,
              std::int32_t x_offset,
              std::int32_t y_offset) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_align_to(raw,
                        base.empty() ? nullptr : base.borrow_raw(),
                        to_lv(align),
                        x_offset,
                        y_offset);
    }
}

void center(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_center(raw);
    }
}

void set_layout(ObjectView object, LayoutKind layout) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_layout(raw, to_lv(layout));
    }
}

bool is_layout_positioned(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_is_layout_positioned(raw);
    }
    return false;
}

void mark_layout_dirty(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_mark_layout_as_dirty(raw);
    }
}

void update_layout(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_update_layout(raw);
    }
}

#if LV_USE_FLEX
void set_flex_flow(ObjectView object, FlexFlow flow) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_flex_flow(raw, to_lv(flow));
    }
}

void set_flex_align(ObjectView object,
                    FlexAlign main_place,
                    FlexAlign cross_place,
                    FlexAlign track_cross_place) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_flex_align(
            raw, to_lv(main_place), to_lv(cross_place), to_lv(track_cross_place));
    }
}

void set_flex_grow(ObjectView object, std::uint8_t grow) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_flex_grow(raw, grow);
    }
}
#endif

#if LV_USE_GRID
void set_grid_descriptor_array(ObjectView object,
                               const GridTrackList& columns,
                               const GridTrackList& rows) noexcept {
    set_grid_descriptor_array(
        object, GridTrackView{columns.borrow_raw()}, GridTrackView{rows.borrow_raw()});
}

void set_grid_descriptor_array(ObjectView object,
                               GridTrackView columns,
                               GridTrackView rows) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_grid_dsc_array(raw, columns.borrow_raw(), rows.borrow_raw());
    }
}

void set_grid_align(ObjectView object,
                    GridAlign column_align,
                    GridAlign row_align) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_grid_align(raw, to_lv(column_align), to_lv(row_align));
    }
}

void set_grid_cell(ObjectView object,
                   GridAlign column_align,
                   std::int32_t column_position,
                   std::int32_t column_span,
                   GridAlign row_align,
                   std::int32_t row_position,
                   std::int32_t row_span) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_grid_cell(raw,
                             to_lv(column_align),
                             column_position,
                             column_span,
                             to_lv(row_align),
                             row_position,
                             row_span);
    }
}
#endif

void style_set_width(LvStyle& style, SizeValue width) noexcept {
    set_style_num(style, LV_STYLE_WIDTH, to_lv(width));
}

void style_set_height(LvStyle& style, SizeValue height) noexcept {
    set_style_num(style, LV_STYLE_HEIGHT, to_lv(height));
}

void style_set_align(LvStyle& style, Align align) noexcept {
    set_style_num(style, LV_STYLE_ALIGN, to_lv(align));
}

void style_set_pad_top(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_TOP, value);
}

void style_set_pad_bottom(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_BOTTOM, value);
}

void style_set_pad_left(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_LEFT, value);
}

void style_set_pad_right(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_RIGHT, value);
}

void style_set_pad_radial(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_RADIAL, value);
}

void style_set_row_gap(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_ROW, value);
}

void style_set_column_gap(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_PAD_COLUMN, value);
}

void style_set_margin_top(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_MARGIN_TOP, value);
}

void style_set_margin_bottom(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_MARGIN_BOTTOM, value);
}

void style_set_margin_left(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_MARGIN_LEFT, value);
}

void style_set_margin_right(LvStyle& style, std::int32_t value) noexcept {
    set_style_num(style, LV_STYLE_MARGIN_RIGHT, value);
}

void style_set_layout(LvStyle& style, LayoutKind layout) noexcept {
    set_style_num(style, LV_STYLE_LAYOUT, to_lv(layout));
}

void local_style_set_width(ObjectView object,
                           SizeValue width,
                           StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_WIDTH, to_lv(width), selector);
}

void local_style_set_height(ObjectView object,
                            SizeValue height,
                            StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_HEIGHT, to_lv(height), selector);
}

void local_style_set_align(ObjectView object,
                           Align align,
                           StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_ALIGN, to_lv(align), selector);
}

void local_style_set_pad_all(ObjectView object,
                             std::int32_t value,
                             StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_PAD_TOP, value, selector);
    set_local_num(object, LV_STYLE_PAD_BOTTOM, value, selector);
    set_local_num(object, LV_STYLE_PAD_LEFT, value, selector);
    set_local_num(object, LV_STYLE_PAD_RIGHT, value, selector);
}

void local_style_set_margin_all(ObjectView object,
                                std::int32_t value,
                                StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_MARGIN_TOP, value, selector);
    set_local_num(object, LV_STYLE_MARGIN_BOTTOM, value, selector);
    set_local_num(object, LV_STYLE_MARGIN_LEFT, value, selector);
    set_local_num(object, LV_STYLE_MARGIN_RIGHT, value, selector);
}

void local_style_set_row_gap(ObjectView object,
                             std::int32_t value,
                             StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_PAD_ROW, value, selector);
}

void local_style_set_column_gap(ObjectView object,
                                std::int32_t value,
                                StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_PAD_COLUMN, value, selector);
}

void local_style_set_layout(ObjectView object,
                            LayoutKind layout,
                            StyleSelector selector) noexcept {
    set_local_num(object, LV_STYLE_LAYOUT, to_lv(layout), selector);
}

#if LV_USE_FLEX
void style_set_flex_flow(LvStyle& style, FlexFlow flow) noexcept {
    set_style_num(style, LV_STYLE_FLEX_FLOW, to_lv(flow));
}

void style_set_flex_main_place(LvStyle& style, FlexAlign align) noexcept {
    set_style_num(style, LV_STYLE_FLEX_MAIN_PLACE, to_lv(align));
}

void style_set_flex_cross_place(LvStyle& style, FlexAlign align) noexcept {
    set_style_num(style, LV_STYLE_FLEX_CROSS_PLACE, to_lv(align));
}

void style_set_flex_track_place(LvStyle& style, FlexAlign align) noexcept {
    set_style_num(style, LV_STYLE_FLEX_TRACK_PLACE, to_lv(align));
}

void style_set_flex_grow(LvStyle& style, std::uint8_t grow) noexcept {
    set_style_num(style, LV_STYLE_FLEX_GROW, grow);
}
#endif

#if LV_USE_GRID
void style_set_grid_column_descriptor_array(LvStyle& style,
                                            GridTrackView columns) noexcept {
    set_style_ptr(style, LV_STYLE_GRID_COLUMN_DSC_ARRAY, columns.borrow_raw());
}

void style_set_grid_row_descriptor_array(LvStyle& style,
                                         GridTrackView rows) noexcept {
    set_style_ptr(style, LV_STYLE_GRID_ROW_DSC_ARRAY, rows.borrow_raw());
}

void style_set_grid_column_align(LvStyle& style, GridAlign align) noexcept {
    set_style_num(style, LV_STYLE_GRID_COLUMN_ALIGN, to_lv(align));
}

void style_set_grid_row_align(LvStyle& style, GridAlign align) noexcept {
    set_style_num(style, LV_STYLE_GRID_ROW_ALIGN, to_lv(align));
}

void style_set_grid_cell_column_position(LvStyle& style,
                                         std::int32_t position) noexcept {
    set_style_num(style, LV_STYLE_GRID_CELL_COLUMN_POS, position);
}

void style_set_grid_cell_column_span(LvStyle& style, std::int32_t span) noexcept {
    set_style_num(style, LV_STYLE_GRID_CELL_COLUMN_SPAN, span);
}

void style_set_grid_cell_x_align(LvStyle& style, GridAlign align) noexcept {
    set_style_num(style, LV_STYLE_GRID_CELL_X_ALIGN, to_lv(align));
}

void style_set_grid_cell_row_position(LvStyle& style,
                                      std::int32_t position) noexcept {
    set_style_num(style, LV_STYLE_GRID_CELL_ROW_POS, position);
}

void style_set_grid_cell_row_span(LvStyle& style, std::int32_t span) noexcept {
    set_style_num(style, LV_STYLE_GRID_CELL_ROW_SPAN, span);
}

void style_set_grid_cell_y_align(LvStyle& style, GridAlign align) noexcept {
    set_style_num(style, LV_STYLE_GRID_CELL_Y_ALIGN, to_lv(align));
}
#endif

}  // namespace lvglpp
