// layout.hpp - LVGL-backed geometry, flex, and grid wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-10-LAYOUT.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj_pos.h, lvgl/src/layouts/lv_layout.h,
//         lvgl/src/layouts/flex/lv_flex.h, and
//         lvgl/src/layouts/grid/lv_grid.h.
// DELTA:  lvglpp delegates layout computation to LVGL instead of porting
//         rlvgl's retained LayoutState/LayoutPass engine.

#ifndef LVGLPP_CORE_LAYOUT_HPP
#define LVGLPP_CORE_LAYOUT_HPP

#include "lvglpp/core/display.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace lvglpp {

class LvStyle;
class StyleSelector;

class SizeValue {
public:
    [[nodiscard]] static constexpr SizeValue pixels(std::int32_t value) noexcept {
        return SizeValue{value};
    }

    [[nodiscard]] static SizeValue percent(std::int32_t value) noexcept;

    [[nodiscard]] static constexpr SizeValue content() noexcept {
        return SizeValue{LV_SIZE_CONTENT};
    }

    [[nodiscard]] static constexpr SizeValue raw(std::int32_t value) noexcept {
        return SizeValue{value};
    }

    [[nodiscard]] constexpr std::int32_t to_lv_coord() const noexcept {
        return raw_;
    }

    [[nodiscard]] constexpr bool operator==(const SizeValue&) const noexcept = default;

private:
    explicit constexpr SizeValue(std::int32_t raw) noexcept : raw_{raw} {}

    std::int32_t raw_ = 0;
};

[[nodiscard]] constexpr std::int32_t to_lv(SizeValue value) noexcept {
    return value.to_lv_coord();
}

enum class Align : std::uint8_t {
    Default         = LV_ALIGN_DEFAULT,
    TopLeft         = LV_ALIGN_TOP_LEFT,
    TopMid          = LV_ALIGN_TOP_MID,
    TopRight        = LV_ALIGN_TOP_RIGHT,
    BottomLeft      = LV_ALIGN_BOTTOM_LEFT,
    BottomMid       = LV_ALIGN_BOTTOM_MID,
    BottomRight     = LV_ALIGN_BOTTOM_RIGHT,
    LeftMid         = LV_ALIGN_LEFT_MID,
    RightMid        = LV_ALIGN_RIGHT_MID,
    Center          = LV_ALIGN_CENTER,
    OutTopLeft      = LV_ALIGN_OUT_TOP_LEFT,
    OutTopMid       = LV_ALIGN_OUT_TOP_MID,
    OutTopRight     = LV_ALIGN_OUT_TOP_RIGHT,
    OutBottomLeft   = LV_ALIGN_OUT_BOTTOM_LEFT,
    OutBottomMid    = LV_ALIGN_OUT_BOTTOM_MID,
    OutBottomRight  = LV_ALIGN_OUT_BOTTOM_RIGHT,
    OutLeftTop      = LV_ALIGN_OUT_LEFT_TOP,
    OutLeftMid      = LV_ALIGN_OUT_LEFT_MID,
    OutLeftBottom   = LV_ALIGN_OUT_LEFT_BOTTOM,
    OutRightTop     = LV_ALIGN_OUT_RIGHT_TOP,
    OutRightMid     = LV_ALIGN_OUT_RIGHT_MID,
    OutRightBottom  = LV_ALIGN_OUT_RIGHT_BOTTOM,
};

[[nodiscard]] constexpr lv_align_t to_lv(Align align) noexcept {
    return static_cast<lv_align_t>(align);
}

enum class LayoutKind : std::uint8_t {
    None = LV_LAYOUT_NONE,
#if LV_USE_FLEX
    Flex = LV_LAYOUT_FLEX,
#endif
#if LV_USE_GRID
    Grid = LV_LAYOUT_GRID,
#endif
};

[[nodiscard]] constexpr lv_layout_t to_lv(LayoutKind layout) noexcept {
    return static_cast<lv_layout_t>(layout);
}

#if LV_USE_FLEX
enum class FlexFlow : std::uint8_t {
    Row               = LV_FLEX_FLOW_ROW,
    Column            = LV_FLEX_FLOW_COLUMN,
    RowWrap           = LV_FLEX_FLOW_ROW_WRAP,
    RowReverse        = LV_FLEX_FLOW_ROW_REVERSE,
    RowWrapReverse    = LV_FLEX_FLOW_ROW_WRAP_REVERSE,
    ColumnWrap        = LV_FLEX_FLOW_COLUMN_WRAP,
    ColumnReverse     = LV_FLEX_FLOW_COLUMN_REVERSE,
    ColumnWrapReverse = LV_FLEX_FLOW_COLUMN_WRAP_REVERSE,
};

enum class FlexAlign : std::uint8_t {
    Start        = LV_FLEX_ALIGN_START,
    End          = LV_FLEX_ALIGN_END,
    Center       = LV_FLEX_ALIGN_CENTER,
    SpaceEvenly  = LV_FLEX_ALIGN_SPACE_EVENLY,
    SpaceAround  = LV_FLEX_ALIGN_SPACE_AROUND,
    SpaceBetween = LV_FLEX_ALIGN_SPACE_BETWEEN,
};

[[nodiscard]] constexpr lv_flex_flow_t to_lv(FlexFlow flow) noexcept {
    return static_cast<lv_flex_flow_t>(flow);
}

[[nodiscard]] constexpr lv_flex_align_t to_lv(FlexAlign align) noexcept {
    return static_cast<lv_flex_align_t>(align);
}
#endif

#if LV_USE_GRID
enum class GridAlign : std::uint8_t {
    Start        = LV_GRID_ALIGN_START,
    Center       = LV_GRID_ALIGN_CENTER,
    End          = LV_GRID_ALIGN_END,
    Stretch      = LV_GRID_ALIGN_STRETCH,
    SpaceEvenly  = LV_GRID_ALIGN_SPACE_EVENLY,
    SpaceAround  = LV_GRID_ALIGN_SPACE_AROUND,
    SpaceBetween = LV_GRID_ALIGN_SPACE_BETWEEN,
};

[[nodiscard]] constexpr lv_grid_align_t to_lv(GridAlign align) noexcept {
    return static_cast<lv_grid_align_t>(align);
}

[[nodiscard]] constexpr std::int32_t grid_content() noexcept {
    return LV_GRID_CONTENT;
}

[[nodiscard]] std::int32_t grid_fr(std::uint8_t factor) noexcept;

class GridTrackList {
public:
    GridTrackList() = default;
    explicit GridTrackList(std::span<const std::int32_t> tracks);

    [[nodiscard]] static GridTrackList from_tracks(
        std::span<const std::int32_t> tracks);

    void assign(std::span<const std::int32_t> tracks);
    void clear() noexcept;

    [[nodiscard]] const std::int32_t* borrow_raw() const noexcept;
    [[nodiscard]] std::span<const std::int32_t> values() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t track_count() const noexcept;

private:
    // owns: sentinel-terminated LVGL grid track descriptor array.
    std::vector<std::int32_t> tracks_{LV_GRID_TEMPLATE_LAST};
};

class GridTrackView {
public:
    // Args:
    //   raw: observes external sentinel-terminated LVGL grid descriptor
    //        array. Caller keeps it alive while LVGL can read it.
    explicit GridTrackView(const std::int32_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] const std::int32_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool empty() const noexcept { return raw_ == nullptr; }

private:
    // observes: caller-owned LVGL grid track descriptor array.
    const std::int32_t* raw_ = nullptr;
};
#endif

void set_pos(ObjectView object, std::int32_t x, std::int32_t y) noexcept;
void set_x(ObjectView object, std::int32_t x) noexcept;
void set_y(ObjectView object, std::int32_t y) noexcept;
void set_size(ObjectView object, SizeValue width, SizeValue height) noexcept;
void set_width(ObjectView object, SizeValue width) noexcept;
void set_height(ObjectView object, SizeValue height) noexcept;
void set_content_width(ObjectView object, std::int32_t width) noexcept;
void set_content_height(ObjectView object, std::int32_t height) noexcept;

// Coordinate and size getters read LVGL's current computed layout. Call
// update_layout() first when reading before the next redraw.
[[nodiscard]] std::int32_t x(ObjectView object) noexcept;
[[nodiscard]] std::int32_t y(ObjectView object) noexcept;
[[nodiscard]] std::int32_t x2(ObjectView object) noexcept;
[[nodiscard]] std::int32_t y2(ObjectView object) noexcept;
[[nodiscard]] std::int32_t aligned_x(ObjectView object) noexcept;
[[nodiscard]] std::int32_t aligned_y(ObjectView object) noexcept;
[[nodiscard]] std::int32_t width(ObjectView object) noexcept;
[[nodiscard]] std::int32_t height(ObjectView object) noexcept;
[[nodiscard]] std::int32_t content_width(ObjectView object) noexcept;
[[nodiscard]] std::int32_t content_height(ObjectView object) noexcept;
[[nodiscard]] std::int32_t self_width(ObjectView object) noexcept;
[[nodiscard]] std::int32_t self_height(ObjectView object) noexcept;
[[nodiscard]] LvArea coords(ObjectView object) noexcept;
[[nodiscard]] LvArea content_coords(ObjectView object) noexcept;

void set_align(ObjectView object, Align align) noexcept;
void align(ObjectView object,
           Align align,
           std::int32_t x_offset,
           std::int32_t y_offset) noexcept;
void align_to(ObjectView object,
              ObjectView base,
              Align align,
              std::int32_t x_offset,
              std::int32_t y_offset) noexcept;
void center(ObjectView object) noexcept;

void set_layout(ObjectView object, LayoutKind layout) noexcept;
[[nodiscard]] bool is_layout_positioned(ObjectView object) noexcept;
void mark_layout_dirty(ObjectView object) noexcept;
void update_layout(ObjectView object) noexcept;

#if LV_USE_FLEX
void set_flex_flow(ObjectView object, FlexFlow flow) noexcept;
void set_flex_align(ObjectView object,
                    FlexAlign main_place,
                    FlexAlign cross_place,
                    FlexAlign track_cross_place) noexcept;
void set_flex_grow(ObjectView object, std::uint8_t grow) noexcept;
#endif

#if LV_USE_GRID
// Args:
//   columns: borrows descriptor storage; must outlive the container's use
//            of the grid layout.
//   rows:    borrows descriptor storage; same lifetime rule as columns.
void set_grid_descriptor_array(ObjectView object,
                               const GridTrackList& columns,
                               const GridTrackList& rows) noexcept;
// Args:
//   columns: observes external sentinel-terminated descriptor storage.
//   rows:    observes external sentinel-terminated descriptor storage.
void set_grid_descriptor_array(ObjectView object,
                               GridTrackView columns,
                               GridTrackView rows) noexcept;
void set_grid_align(ObjectView object,
                    GridAlign column_align,
                    GridAlign row_align) noexcept;
void set_grid_cell(ObjectView object,
                   GridAlign column_align,
                   std::int32_t column_position,
                   std::int32_t column_span,
                   GridAlign row_align,
                   std::int32_t row_position,
                   std::int32_t row_span) noexcept;
#endif

void style_set_width(LvStyle& style, SizeValue width) noexcept;
void style_set_height(LvStyle& style, SizeValue height) noexcept;
void style_set_align(LvStyle& style, Align align) noexcept;
void style_set_pad_top(LvStyle& style, std::int32_t value) noexcept;
void style_set_pad_bottom(LvStyle& style, std::int32_t value) noexcept;
void style_set_pad_left(LvStyle& style, std::int32_t value) noexcept;
void style_set_pad_right(LvStyle& style, std::int32_t value) noexcept;
void style_set_pad_radial(LvStyle& style, std::int32_t value) noexcept;
void style_set_row_gap(LvStyle& style, std::int32_t value) noexcept;
void style_set_column_gap(LvStyle& style, std::int32_t value) noexcept;
void style_set_margin_top(LvStyle& style, std::int32_t value) noexcept;
void style_set_margin_bottom(LvStyle& style, std::int32_t value) noexcept;
void style_set_margin_left(LvStyle& style, std::int32_t value) noexcept;
void style_set_margin_right(LvStyle& style, std::int32_t value) noexcept;
void style_set_layout(LvStyle& style, LayoutKind layout) noexcept;

void local_style_set_width(ObjectView object,
                           SizeValue width,
                           StyleSelector selector) noexcept;
void local_style_set_height(ObjectView object,
                            SizeValue height,
                            StyleSelector selector) noexcept;
void local_style_set_align(ObjectView object,
                           Align align,
                           StyleSelector selector) noexcept;
void local_style_set_pad_all(ObjectView object,
                             std::int32_t value,
                             StyleSelector selector) noexcept;
void local_style_set_margin_all(ObjectView object,
                                std::int32_t value,
                                StyleSelector selector) noexcept;
void local_style_set_row_gap(ObjectView object,
                             std::int32_t value,
                             StyleSelector selector) noexcept;
void local_style_set_column_gap(ObjectView object,
                                std::int32_t value,
                                StyleSelector selector) noexcept;
void local_style_set_layout(ObjectView object,
                            LayoutKind layout,
                            StyleSelector selector) noexcept;

#if LV_USE_FLEX
void style_set_flex_flow(LvStyle& style, FlexFlow flow) noexcept;
void style_set_flex_main_place(LvStyle& style, FlexAlign align) noexcept;
void style_set_flex_cross_place(LvStyle& style, FlexAlign align) noexcept;
void style_set_flex_track_place(LvStyle& style, FlexAlign align) noexcept;
void style_set_flex_grow(LvStyle& style, std::uint8_t grow) noexcept;
#endif

#if LV_USE_GRID
void style_set_grid_column_descriptor_array(LvStyle& style,
                                            GridTrackView columns) noexcept;
void style_set_grid_row_descriptor_array(LvStyle& style,
                                         GridTrackView rows) noexcept;
void style_set_grid_column_align(LvStyle& style, GridAlign align) noexcept;
void style_set_grid_row_align(LvStyle& style, GridAlign align) noexcept;
void style_set_grid_cell_column_position(LvStyle& style,
                                         std::int32_t position) noexcept;
void style_set_grid_cell_column_span(LvStyle& style, std::int32_t span) noexcept;
void style_set_grid_cell_x_align(LvStyle& style, GridAlign align) noexcept;
void style_set_grid_cell_row_position(LvStyle& style,
                                      std::int32_t position) noexcept;
void style_set_grid_cell_row_span(LvStyle& style, std::int32_t span) noexcept;
void style_set_grid_cell_y_align(LvStyle& style, GridAlign align) noexcept;
#endif

}  // namespace lvglpp

#endif  // LVGLPP_CORE_LAYOUT_HPP
