// PARITY: rlvgl/playit/src/framebuffer.rs (v0.2.5 @ f999f75).
// LVGL:   lvgl/include/lvgl/draw/lv_snapshot.h and lv_draw_buf.h.
// DELTA:  captures an LVGL active-screen tree outside Executor::poll().
#ifndef LVGLPP_PLAYIT_LVGL_SNAPSHOT_READER_HPP
#define LVGLPP_PLAYIT_LVGL_SNAPSHOT_READER_HPP

#include "lvgl.h"
#include "lvglpp/playit/framebuffer.hpp"

#include <memory>

#if LV_USE_SNAPSHOT
namespace lvglpp::playit {

// Snapshot-backed D dump source. All calls run on the LVGL application
// thread, or under its lock. The display and LVGL runtime outlive this
// reader; detach it from the Executor before destruction.
class LvglSnapshotReader final : public FramebufferReader {
public:
    // Args:
    //   display: borrows the display for this reader's lifetime.
    explicit LvglSnapshotReader(lv_display_t& display) noexcept;
    ~LvglSnapshotReader() override = default;
    LvglSnapshotReader(const LvglSnapshotReader&) = delete;
    LvglSnapshotReader& operator=(const LvglSnapshotReader&) = delete;
    LvglSnapshotReader(LvglSnapshotReader&&) = delete;
    LvglSnapshotReader& operator=(LvglSnapshotReader&&) = delete;

    // Call OUTSIDE poll(), after a completed present and before any scene
    // mutation for the next frame. This re-renders the active screen tree;
    // separate top/system/bottom layers and screen transitions are excluded.
    // Args:
    //   present: existing completed-present counter (including wraparound).
    // Returns: true if this present has a snapshot. Failure retains the
    // previous snapshot/count so the Executor never emits stale pixels as
    // a new frame. A repeated count is a no-op; skipped presents coalesce.
    // Allocates through LVGL, never throws. Old and new snapshots coexist
    // briefly during capture. Snapshotting does not advance the counter.
    [[nodiscard]] bool capture_presented_frame(std::uint32_t present) noexcept;

    [[nodiscard]] std::uint32_t read_pixel(std::int32_t x,
                                           std::int32_t y) const override;
    std::size_t read_row(std::int32_t x, std::int32_t y, std::uint16_t width,
                         std::span<std::uint32_t> out) const override;
    [[nodiscard]] std::uint32_t present_count() const override;

private:
    struct DrawBufferDeleter {
        // Args: buffer owns the snapshot allocation being released.
        void operator()(lv_draw_buf_t* buffer) const noexcept;
    };

    // borrows: external display; caller keeps it alive until destruction.
    lv_display_t& display_;
    // owns: LVGL snapshot; custom deleter releases through lv_draw_buf_destroy.
    std::unique_ptr<lv_draw_buf_t, DrawBufferDeleter> snapshot_{};
    std::uint32_t present_ = 0;
    std::int32_t origin_x_ = 0;
    std::int32_t origin_y_ = 0;
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
};

} // namespace lvglpp::playit
#endif // LV_USE_SNAPSHOT
#endif // LVGLPP_PLAYIT_LVGL_SNAPSHOT_READER_HPP
