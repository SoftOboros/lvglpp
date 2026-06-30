// scroll.hpp - LVGL-backed scroll helpers.
//
// PARITY: rlvgl/docs/concepts/LPAR-05-SCROLL-RUNTIME.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj_scroll.h and lvgl/src/core/lv_obj.h.
// DELTA:  lvglpp delegates scroll state, snap, scrollbars, chaining, and
//         momentum to LVGL instead of porting rlvgl's ScrollController.

#ifndef LVGLPP_CORE_SCROLL_HPP
#define LVGLPP_CORE_SCROLL_HPP

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"

#include <cstdint>

namespace lvglpp {

enum class AnimationMode : std::uint8_t {
    Off,
    On,
};

[[nodiscard]] constexpr lv_anim_enable_t to_lv(AnimationMode mode) noexcept {
    return mode == AnimationMode::On ? LV_ANIM_ON : LV_ANIM_OFF;
}

enum class ScrollbarMode : std::uint8_t {
    Off,
    On,
    Active,
    Auto,
};

[[nodiscard]] constexpr lv_scrollbar_mode_t to_lv(ScrollbarMode mode) noexcept {
    switch (mode) {
        case ScrollbarMode::Off:    return LV_SCROLLBAR_MODE_OFF;
        case ScrollbarMode::On:     return LV_SCROLLBAR_MODE_ON;
        case ScrollbarMode::Active: return LV_SCROLLBAR_MODE_ACTIVE;
        case ScrollbarMode::Auto:   return LV_SCROLLBAR_MODE_AUTO;
    }
    return LV_SCROLLBAR_MODE_AUTO;
}

[[nodiscard]] constexpr ScrollbarMode scrollbar_mode_from_lv(
    lv_scrollbar_mode_t mode) noexcept {
    switch (mode) {
        case LV_SCROLLBAR_MODE_OFF:    return ScrollbarMode::Off;
        case LV_SCROLLBAR_MODE_ON:     return ScrollbarMode::On;
        case LV_SCROLLBAR_MODE_ACTIVE: return ScrollbarMode::Active;
        case LV_SCROLLBAR_MODE_AUTO:
        default:                       return ScrollbarMode::Auto;
    }
}

enum class ScrollSnap : std::uint8_t {
    None,
    Start,
    End,
    Center,
};

[[nodiscard]] constexpr lv_scroll_snap_t to_lv(ScrollSnap snap) noexcept {
    switch (snap) {
        case ScrollSnap::None:   return LV_SCROLL_SNAP_NONE;
        case ScrollSnap::Start:  return LV_SCROLL_SNAP_START;
        case ScrollSnap::End:    return LV_SCROLL_SNAP_END;
        case ScrollSnap::Center: return LV_SCROLL_SNAP_CENTER;
    }
    return LV_SCROLL_SNAP_NONE;
}

[[nodiscard]] constexpr ScrollSnap scroll_snap_from_lv(
    lv_scroll_snap_t snap) noexcept {
    switch (snap) {
        case LV_SCROLL_SNAP_START:  return ScrollSnap::Start;
        case LV_SCROLL_SNAP_END:    return ScrollSnap::End;
        case LV_SCROLL_SNAP_CENTER: return ScrollSnap::Center;
        case LV_SCROLL_SNAP_NONE:
        default:                    return ScrollSnap::None;
    }
}

enum class ScrollDirection : std::uint8_t {
    None       = static_cast<std::uint8_t>(LV_DIR_NONE),
    Left       = static_cast<std::uint8_t>(LV_DIR_LEFT),
    Right      = static_cast<std::uint8_t>(LV_DIR_RIGHT),
    Top        = static_cast<std::uint8_t>(LV_DIR_TOP),
    Bottom     = static_cast<std::uint8_t>(LV_DIR_BOTTOM),
    Horizontal = static_cast<std::uint8_t>(LV_DIR_HOR),
    Vertical   = static_cast<std::uint8_t>(LV_DIR_VER),
    All        = static_cast<std::uint8_t>(LV_DIR_ALL),
};

[[nodiscard]] constexpr lv_dir_t to_lv(ScrollDirection direction) noexcept {
    return static_cast<lv_dir_t>(direction);
}

[[nodiscard]] constexpr ScrollDirection scroll_direction_from_lv(
    lv_dir_t direction) noexcept {
    return static_cast<ScrollDirection>(
        static_cast<std::uint8_t>(direction & LV_DIR_ALL));
}

[[nodiscard]] constexpr ScrollDirection operator|(
    ScrollDirection lhs, ScrollDirection rhs) noexcept {
    return static_cast<ScrollDirection>(
        static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

struct ScrollOffset {
    std::int32_t x = 0;
    std::int32_t y = 0;

    [[nodiscard]] constexpr bool operator==(const ScrollOffset&) const noexcept = default;
};

struct ScrollExtents {
    std::int32_t top    = 0;
    std::int32_t bottom = 0;
    std::int32_t left   = 0;
    std::int32_t right  = 0;

    [[nodiscard]] constexpr bool operator==(const ScrollExtents&) const noexcept = default;
};

struct ScrollbarAreas {
    LvArea horizontal{};
    LvArea vertical{};

    [[nodiscard]] constexpr bool operator==(const ScrollbarAreas&) const noexcept = default;
};

void set_scrollbar_mode(ObjectView object, ScrollbarMode mode) noexcept;
[[nodiscard]] ScrollbarMode scrollbar_mode(ObjectView object) noexcept;

void set_scroll_direction(ObjectView object, ScrollDirection direction) noexcept;
[[nodiscard]] ScrollDirection scroll_direction(ObjectView object) noexcept;

void set_scroll_snap_x(ObjectView object, ScrollSnap snap) noexcept;
void set_scroll_snap_y(ObjectView object, ScrollSnap snap) noexcept;
[[nodiscard]] ScrollSnap scroll_snap_x(ObjectView object) noexcept;
[[nodiscard]] ScrollSnap scroll_snap_y(ObjectView object) noexcept;

[[nodiscard]] ScrollOffset scroll_offset(ObjectView object) noexcept;
[[nodiscard]] ScrollExtents scroll_extents(ObjectView object) noexcept;
[[nodiscard]] ScrollOffset scroll_end(ObjectView object) noexcept;

void scroll_by(ObjectView object,
               std::int32_t dx,
               std::int32_t dy,
               AnimationMode animation) noexcept;
void scroll_by_bounded(ObjectView object,
                       std::int32_t dx,
                       std::int32_t dy,
                       AnimationMode animation) noexcept;
void scroll_to(ObjectView object,
               std::int32_t x,
               std::int32_t y,
               AnimationMode animation) noexcept;
void scroll_to_x(ObjectView object,
                 std::int32_t x,
                 AnimationMode animation) noexcept;
void scroll_to_y(ObjectView object,
                 std::int32_t y,
                 AnimationMode animation) noexcept;

void scroll_to_view(ObjectView object, AnimationMode animation) noexcept;
void scroll_to_view_recursive(ObjectView object, AnimationMode animation) noexcept;

[[nodiscard]] bool is_scrolling(ObjectView object) noexcept;
void stop_scroll_anim(ObjectView object) noexcept;
void update_snap(ObjectView object, AnimationMode animation) noexcept;
void readjust_scroll(ObjectView object, AnimationMode animation) noexcept;

[[nodiscard]] ScrollbarAreas scrollbar_areas(ObjectView object) noexcept;
void scrollbar_invalidate(ObjectView object) noexcept;

}  // namespace lvglpp

#endif  // LVGLPP_CORE_SCROLL_HPP
