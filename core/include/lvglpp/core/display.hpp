// display.hpp — RAII owner of an lv_display_t* and the flush/draw-buffer
// surface the platform backends drive (LVGLPP LPAR-03).
//
// PARITY: rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md (v0.2.4 @
//         343f596). rlvgl owns a dirty-rect planner + present plan; lvglpp
//         delegates that to LVGL and only wraps the lv_display_* flush
//         contract. The ownership story (Display owns the device; draw
//         buffers are caller-owned/dma) matches the rlvgl present model.
// LVGL:   lvgl/src/display/lv_display.h (lv_display_create /
//         lv_display_delete / lv_display_set_flush_cb /
//         lv_display_set_buffers / lv_display_set_render_mode /
//         lv_display_get_screen_active).
// DELTA:  Display is move-only RAII; the destructor calls lv_display_delete
//         iff it still owns a live display. Draw buffers stay caller-owned
//         and are tagged dma/external (CLAUDE.md ownership rule 10). The
//         flush-cb body itself is deferred to the platform layer (WRAP-0N).

#ifndef LVGLPP_CORE_DISPLAY_HPP
#define LVGLPP_CORE_DISPLAY_HPP

#include <cstdint>

#include "lvglpp/core/runtime.hpp"  // lvglpp::ObjectView
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// Errors that the Display factory can produce.
enum class DisplayError : std::uint8_t {
    CreateFailed = 1,  // lv_display_create returned nullptr (e.g. OOM).
};

// LPAR-03 — mirror of lv_display_render_mode_t
// (lvgl/src/display/lv_display.h). Standards Action to diverge: this enum
// must agree with the LVGL constants and with rlvgl's render-mode model.
// See docs/core-object/01-invalidation-display.md §3/§5.
enum class RenderMode : std::uint8_t {
    Partial = LV_DISPLAY_RENDER_MODE_PARTIAL,
    Full    = LV_DISPLAY_RENDER_MODE_FULL,
    Direct  = LV_DISPLAY_RENDER_MODE_DIRECT,
};

// RAII owner of a single lv_display_t. See
// docs/core-object/01-invalidation-display.md §5.
//
// Ownership: owns its lv_display_t. The destructor calls lv_display_delete
// iff it still owns a live display. The flush callback and draw buffers are
// caller-provided and non-owning from Display's perspective (dma/external);
// they MUST outlive the Display and MUST NOT be CPU-mutated during an
// active flush (CLAUDE.md ownership rule 10).
class Display {
public:
    // Create a display device of the given pixel resolution.
    // Args:
    //   hor: horizontal resolution in pixels.
    //   ver: vertical resolution in pixels.
    // Returns: owns Display on success; DisplayError on failure.
    [[nodiscard]] static lvglpp::expected<Display, DisplayError>
    try_make(std::int32_t hor, std::int32_t ver) noexcept;

    // Throwing convenience over try_make: aborts under embedded posture,
    // throws std::runtime_error on host, if creation fails. Mirrors the
    // Object/Runtime factory pattern.
    [[nodiscard]] static Display make(std::int32_t hor, std::int32_t ver);

    Display(const Display&)            = delete;
    Display& operator=(const Display&) = delete;

    // Move-construct only: transfers the handle and leaves the moved-from
    // Display empty so its destructor is a no-op. Move-assignment is
    // deleted (mirrors Object/Runtime).
    Display(Display&& other) noexcept;
    Display& operator=(Display&&) = delete;

    ~Display();

    // Non-owning observation of ownership state.
    // borrows: valid only while this Display owns a live lv_display.
    [[nodiscard]] lv_display_t* borrow_raw() const noexcept { return disp_; }
    [[nodiscard]] bool          empty()      const noexcept { return disp_ == nullptr; }

    // Install the flush callback. The callback is invoked by LVGL's
    // refresh timer with a dirty area and a rendered pixel buffer; it must
    // end with lv_display_flush_ready. No-op when this Display is empty.
    //   cb: observes; an externally-owned function pointer. Its body and
    //       any captured target lifetime are the platform layer's concern
    //       (WRAP-0N).
    void set_flush_cb(lv_display_flush_cb_t cb) noexcept;

    // Hand LVGL the draw buffer(s) and select the render mode.
    //   buf1: dma/external; caller-owned render buffer. MUST outlive this
    //         Display and MUST NOT be CPU-mutated while a flush of its
    //         contents is in flight (CLAUDE.md ownership rule 10).
    //   buf2: dma/external; optional second buffer for double-buffering,
    //         or nullptr. Same lifetime/mutation contract as buf1.
    //   size: byte size of each buffer.
    //   mode: how LVGL renders into the buffers (see RenderMode).
    // No-op when this Display is empty.
    void set_buffers(void* buf1, void* buf2, std::uint32_t size, RenderMode mode) noexcept;

    // Change the render mode without re-supplying buffers. No-op when
    // empty.
    void set_render_mode(RenderMode mode) noexcept;

    // Non-owning view of this display's active screen (observes; never
    // frees). Empty view when this Display is empty.
    [[nodiscard]] ObjectView active_screen() const noexcept;

private:
    // Adopt an already-created lv_display. `disp` may be nullptr (yields an
    // empty Display).
    explicit Display(lv_display_t* disp) noexcept : disp_{disp} {}

    // owns: the LVGL display device. Nulled on move-out so the destructor
    // does not double-delete.
    lv_display_t* disp_ = nullptr;
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_DISPLAY_HPP
