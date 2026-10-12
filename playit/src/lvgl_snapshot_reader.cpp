// PARITY: rlvgl/playit/src/framebuffer.rs (v0.2.5 @ f999f75).
// LVGL:   lvgl/include/lvgl/draw/lv_snapshot.h and lv_draw_buf.h.
// DELTA:  ARGB8888 snapshot readback instead of physical framebuffer access.
#include "lvglpp/playit/lvgl_snapshot_reader.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

#if LV_USE_SNAPSHOT
namespace lvglpp::playit {

LvglSnapshotReader::LvglSnapshotReader(lv_display_t& display) noexcept
    : display_{display} {}

void LvglSnapshotReader::DrawBufferDeleter::operator()(
    lv_draw_buf_t* buffer) const noexcept {
    if (buffer != nullptr)
        lv_draw_buf_destroy(buffer);
}

bool LvglSnapshotReader::capture_presented_frame(
    std::uint32_t present) noexcept {
    if (snapshot_ && present == present_)
        return true;
    // borrows: active screen for this synchronous capture only; owned by
    // display.
    lv_obj_t* screen = lv_display_get_screen_active(&display_);
    if (screen == nullptr)
        return false;
    // Factory return visibly transfers ownership into RAII immediately.
    std::unique_ptr<lv_draw_buf_t, DrawBufferDeleter> next{
        lv_snapshot_take(screen, LV_COLOR_FORMAT_ARGB8888)};
    if (!next)
        return false;

    lv_area_t area{};
    lv_obj_get_coords(screen, &area);
    const auto margin =
        (static_cast<std::int32_t>(next->header.w) - lv_obj_get_width(screen)) /
        2;
    origin_x_ = area.x1 - margin;
    origin_y_ = area.y1 - margin;
    width_ = lv_display_get_horizontal_resolution(&display_);
    height_ = lv_display_get_vertical_resolution(&display_);
    snapshot_ = std::move(next);
    present_ = present;
    return true;
}

std::uint32_t LvglSnapshotReader::read_pixel(std::int32_t x,
                                             std::int32_t y) const {
    if (!snapshot_ || x < 0 || y < 0 || x >= width_ || y >= height_)
        return 0;
    const std::int64_t sx = static_cast<std::int64_t>(x) - origin_x_;
    const std::int64_t sy = static_cast<std::int64_t>(y) - origin_y_;
    if (sx < 0 || sy < 0 || sx >= snapshot_->header.w ||
        sy >= snapshot_->header.h)
        return 0;
    lv_color32_t pixel{};
    // borrows: goto_xy addresses owned snapshot bytes, respecting LVGL stride.
    // memcpy avoids alignment/aliasing assumptions; fields avoid host
    // endianness.
    std::memcpy(&pixel,
                lv_draw_buf_goto_xy(snapshot_.get(),
                                    static_cast<std::uint32_t>(sx),
                                    static_cast<std::uint32_t>(sy)),
                sizeof(pixel));
    return (static_cast<std::uint32_t>(pixel.alpha) << 24U) |
           (static_cast<std::uint32_t>(pixel.red) << 16U) |
           (static_cast<std::uint32_t>(pixel.green) << 8U) |
           static_cast<std::uint32_t>(pixel.blue);
}

std::size_t LvglSnapshotReader::read_row(std::int32_t x, std::int32_t y,
                                         std::uint16_t width,
                                         std::span<std::uint32_t> out) const {
    if (!snapshot_ || x < 0 || y < 0 || x >= width_ || y >= height_)
        return 0;
    const auto count = std::min({static_cast<std::size_t>(width), out.size(),
                                 static_cast<std::size_t>(width_ - x)});
    for (std::size_t i = 0; i < count; ++i) {
        out[i] = read_pixel(x + static_cast<std::int32_t>(i), y);
    }
    return count;
}

std::uint32_t LvglSnapshotReader::present_count() const { return present_; }

} // namespace lvglpp::playit
#endif // LV_USE_SNAPSHOT
