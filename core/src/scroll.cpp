// scroll.cpp - LVGL-backed scroll helpers.
//
// PARITY: rlvgl/docs/concepts/LPAR-05-SCROLL-RUNTIME.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj_scroll.c and lvgl/src/core/lv_obj.c.
// DELTA:  delegates scroll behavior to LVGL.

#include "lvglpp/core/scroll.hpp"

namespace lvglpp {

namespace {

[[nodiscard]] lv_obj_t* raw_or_null(ObjectView object) noexcept {
    return object.empty() ? nullptr : object.borrow_raw();
}

}  // namespace

void set_scrollbar_mode(ObjectView object, ScrollbarMode mode) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_scrollbar_mode(raw, to_lv(mode));
    }
}

ScrollbarMode scrollbar_mode(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return scrollbar_mode_from_lv(lv_obj_get_scrollbar_mode(raw));
    }
    return ScrollbarMode::Auto;
}

void set_scroll_direction(ObjectView object, ScrollDirection direction) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_scroll_dir(raw, to_lv(direction));
    }
}

ScrollDirection scroll_direction(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return scroll_direction_from_lv(lv_obj_get_scroll_dir(raw));
    }
    return ScrollDirection::None;
}

void set_scroll_snap_x(ObjectView object, ScrollSnap snap) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_scroll_snap_x(raw, to_lv(snap));
    }
}

void set_scroll_snap_y(ObjectView object, ScrollSnap snap) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_scroll_snap_y(raw, to_lv(snap));
    }
}

ScrollSnap scroll_snap_x(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return scroll_snap_from_lv(lv_obj_get_scroll_snap_x(raw));
    }
    return ScrollSnap::None;
}

ScrollSnap scroll_snap_y(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return scroll_snap_from_lv(lv_obj_get_scroll_snap_y(raw));
    }
    return ScrollSnap::None;
}

ScrollOffset scroll_offset(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return ScrollOffset{lv_obj_get_scroll_x(raw), lv_obj_get_scroll_y(raw)};
    }
    return ScrollOffset{};
}

ScrollExtents scroll_extents(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return ScrollExtents{
            lv_obj_get_scroll_top(raw),
            lv_obj_get_scroll_bottom(raw),
            lv_obj_get_scroll_left(raw),
            lv_obj_get_scroll_right(raw),
        };
    }
    return ScrollExtents{};
}

ScrollOffset scroll_end(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_point_t end{};
        lv_obj_get_scroll_end(raw, &end);
        return ScrollOffset{end.x, end.y};
    }
    return ScrollOffset{};
}

void scroll_by(ObjectView object,
               std::int32_t dx,
               std::int32_t dy,
               AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_by(raw, dx, dy, to_lv(animation));
    }
}

void scroll_by_bounded(ObjectView object,
                       std::int32_t dx,
                       std::int32_t dy,
                       AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_by_bounded(raw, dx, dy, to_lv(animation));
    }
}

void scroll_to(ObjectView object,
               std::int32_t x,
               std::int32_t y,
               AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_to(raw, x, y, to_lv(animation));
    }
}

void scroll_to_x(ObjectView object,
                 std::int32_t x,
                 AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_to_x(raw, x, to_lv(animation));
    }
}

void scroll_to_y(ObjectView object,
                 std::int32_t y,
                 AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_to_y(raw, y, to_lv(animation));
    }
}

void scroll_to_view(ObjectView object, AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_to_view(raw, to_lv(animation));
    }
}

void scroll_to_view_recursive(ObjectView object, AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scroll_to_view_recursive(raw, to_lv(animation));
    }
}

bool is_scrolling(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_is_scrolling(raw);
    }
    return false;
}

void stop_scroll_anim(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_stop_scroll_anim(raw);
    }
}

void update_snap(ObjectView object, AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_update_snap(raw, to_lv(animation));
    }
}

void readjust_scroll(ObjectView object, AnimationMode animation) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_readjust_scroll(raw, to_lv(animation));
    }
}

ScrollbarAreas scrollbar_areas(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_area_t horizontal{};
        lv_area_t vertical{};
        lv_obj_get_scrollbar_area(raw, &horizontal, &vertical);
        return ScrollbarAreas{
            LvArea::from_lv(horizontal),
            LvArea::from_lv(vertical),
        };
    }
    return ScrollbarAreas{};
}

void scrollbar_invalidate(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_scrollbar_invalidate(raw);
    }
}

}  // namespace lvglpp
