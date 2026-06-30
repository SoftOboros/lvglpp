// display.hpp - move-only owner for LVGL displays and invalidation helpers.
//
// PARITY: rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/display/lv_display.h and lvgl/src/core/lv_obj_pos.h.
// DELTA:  lvglpp delegates dirty planning and refresh to LVGL instead of
//         porting rlvgl's retained dirty planner.

#ifndef LVGLPP_CORE_DISPLAY_HPP
#define LVGLPP_CORE_DISPLAY_HPP

#include "lvglpp/core/object.hpp"

#include <cstdint>

namespace lvglpp {

// Value wrapper for LVGL's inclusive-coordinate lv_area_t.
//
// LVGL: x2/y2 are inclusive. A 10x20 rect at (3,4) is
// {x1=3, y1=4, x2=12, y2=23}.
struct LvArea {
    std::int32_t x1 = 0;
    std::int32_t y1 = 0;
    std::int32_t x2 = 0;
    std::int32_t y2 = 0;

    [[nodiscard]] static constexpr LvArea from_inclusive(
        std::int32_t left,
        std::int32_t top,
        std::int32_t right,
        std::int32_t bottom) noexcept {
        return LvArea{left, top, right, bottom};
    }

    [[nodiscard]] static constexpr LvArea from_xywh(
        std::int32_t x,
        std::int32_t y,
        std::int32_t width,
        std::int32_t height) noexcept {
        return LvArea{x, y, x + width - 1, y + height - 1};
    }

    [[nodiscard]] static constexpr LvArea from_lv(const lv_area_t& area) noexcept {
        return LvArea{area.x1, area.y1, area.x2, area.y2};
    }

    [[nodiscard]] constexpr lv_area_t to_lv() const noexcept {
        return lv_area_t{x1, y1, x2, y2};
    }

    [[nodiscard]] constexpr bool operator==(const LvArea&) const noexcept = default;
};

// Thin non-owning view of an lv_display_t*.
//
// Ownership: external. The underlying display is owned by LVGL or by an
// LvDisplay owner. This view never deletes it.
class DisplayView {
public:
    // Args:
    //   raw: external; owned outside this view. Must remain valid for
    //        the lifetime of this view.
    explicit DisplayView(lv_display_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_display_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool          empty() const noexcept { return raw_ == nullptr; }

private:
    // external: owned by LVGL/LvDisplay; this view never frees it.
    lv_display_t* raw_;
};

// Move-only RAII owner for an LVGL display.
//
// Ownership: owns raw_ while non-null. Destruction calls
// lv_display_delete(). release() transfers the raw handle to the caller
// and disarms deletion.
class LvDisplay {
public:
    LvDisplay() noexcept = default;

    [[nodiscard]] static LvDisplay make(std::int32_t width,
                                        std::int32_t height) noexcept;

    LvDisplay(const LvDisplay&)            = delete;
    LvDisplay& operator=(const LvDisplay&) = delete;

    LvDisplay(LvDisplay&& other) noexcept;
    LvDisplay& operator=(LvDisplay&& other) noexcept;

    ~LvDisplay();

    [[nodiscard]] DisplayView   borrow() const noexcept;
    [[nodiscard]] lv_display_t* borrow_raw() const noexcept;
    [[nodiscard]] bool          empty() const noexcept;

    // Returns: owns raw LVGL display handle; caller is responsible for
    // its lifecycle.
    [[nodiscard]] lv_display_t* release() noexcept;

    void reset() noexcept;
    void set_default() noexcept;

    // Args:
    //   buffer1: external/dma; LVGL borrows for render/flush use.
    //   buffer2: external/dma nullable second buffer; same lifetime as
    //            buffer1 when non-null.
    //   byte_size: size of each buffer in bytes.
    //   render_mode: LVGL render-mode enum.
    void set_buffers(void* buffer1,
                     void* buffer2,
                     std::uint32_t byte_size,
                     lv_display_render_mode_t render_mode) noexcept;

    // Args:
    //   callback: external function pointer. Callback userdata, if any,
    //             is managed by LVGL display events or external storage.
    void set_flush_callback(lv_display_flush_cb_t callback) noexcept;

private:
    explicit LvDisplay(lv_display_t* raw) noexcept;

    // owns: deleted with lv_display_delete() when non-null.
    lv_display_t* raw_ = nullptr;
};

[[nodiscard]] lv_display_t* borrow_raw(DisplayView display) noexcept;

[[nodiscard]] lv_result_t invalidate(ObjectView object) noexcept;
[[nodiscard]] lv_result_t invalidate_area(ObjectView object, LvArea area) noexcept;

}  // namespace lvglpp

#endif  // LVGLPP_CORE_DISPLAY_HPP
