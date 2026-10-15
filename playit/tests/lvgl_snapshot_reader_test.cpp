// PARITY: rlvgl/playit/src/executor.rs (v0.2.5 @ f999f75).
// LVGL:   lvgl/include/lvgl/draw/lv_snapshot.h.
// DELTA:  test real LVGL snapshots through the unchanged D dump executor.
#include "lvglpp/core/display.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/playit/playit.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <deque>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

// Test-only allocator/stride fault injection; production uses public LVGL APIs.
#include "src/draw/lv_draw_buf_private.h"

#if LV_USE_SNAPSHOT
namespace lp = lvglpp::playit;
namespace {
struct MemoryTransport final : lp::Transport {
    std::deque<std::uint8_t> input;
    std::string output;
    std::optional<std::uint8_t> read_byte() noexcept override {
        if (input.empty())
            return std::nullopt;
        const auto byte = input.front();
        input.pop_front();
        return byte;
    }
    void write_bytes(std::span<const std::uint8_t> bytes) noexcept override {
        for (auto byte : bytes)
            output.push_back(static_cast<char>(byte));
    }
    void feed(std::string_view line) {
        for (char byte : line)
            input.push_back(static_cast<std::uint8_t>(byte));
    }
    std::string drain() { return std::exchange(output, {}); }
};

void noop_flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) {
    // borrows: callback-only access to the external LVGL display.
    lv_display_flush_ready(display);
}

struct Fixture {
    // owns: draw storage; display borrows it until destruction.
    std::array<std::uint8_t, 7 * 4 * 4> draw_buffer{};
    lvglpp::LvDisplay display{lvglpp::LvDisplay::make(7, 4)};
    lp::LvglSnapshotReader reader{*display.borrow_raw()};
    lvglpp::core::WidgetNode compatibility_root{};
    lp::Dispatcher dispatcher{compatibility_root};
    MemoryTransport transport;
    lp::Executor executor{transport, dispatcher};

    Fixture() {
        display.set_default();
        display.set_buffers(draw_buffer.data(), nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
        display.set_flush_callback(noop_flush);
        // borrows: active screen only during setup; display owns it.
        auto* screen = lv_display_get_screen_active(display.borrow_raw());
        lv_obj_remove_style_all(screen);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x112233), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        executor.set_framebuffer_reader(&reader);
    }
    ~Fixture() { executor.set_framebuffer_reader(nullptr); }

    void present(std::uint32_t count, std::uint32_t color = 0x112233) {
        // borrows: active screen for synchronous LVGL update/refresh only.
        auto* screen = lv_display_get_screen_active(display.borrow_raw());
        lv_obj_set_style_bg_color(screen, lv_color_hex(color), 0);
        lv_refr_now(display.borrow_raw());
        assert(reader.capture_presented_frame(count));
    }
};

// Scoped test override restores global LVGL callbacks before any later test.
class HandlerOverride {
public:
    HandlerOverride()
        : handlers_{*lv_draw_buf_get_handlers()}, saved_{handlers_} {}
    ~HandlerOverride() { handlers_ = saved_; }
    void fail_allocations() { handlers_.buf_malloc_cb = deny_allocation; }
    void pad_rows() { handlers_.width_to_stride_cb = padded_stride; }

private:
    // borrows: LVGL global handlers; Runtime outlives this scoped override.
    lv_draw_buf_handlers_t& handlers_;
    // external: copied callbacks have program lifetime; restoration owns no
    // buffers.
    lv_draw_buf_handlers_t saved_;
    static void* deny_allocation(std::size_t, lv_color_format_t) {
        return nullptr;
    }
    static std::uint32_t padded_stride(std::uint32_t width,
                                       lv_color_format_t format) {
        return ((width * lv_color_format_get_size(format) + 15U) / 16U) * 16U;
    }
};

void test_snapshot_pixels_and_immutable_rows() {
    Fixture fx;
    HandlerOverride handlers;
    handlers.pad_rows();
    assert(fx.reader.read_pixel(0, 0) == 0);
    std::array<std::uint32_t, 5> row{};
    assert(fx.reader.read_row(0, 0, 5, row) == 0);
    fx.present(1);
    assert(fx.reader.read_pixel(6, 3) == 0xff112233U);
    assert(fx.reader.read_row(5, 3, 5, row) == 2);
    assert(row[0] == 0xff112233U && row[1] == 0xff112233U);
    assert(fx.reader.read_row(0, 0, 5, std::span{row}.first(1)) == 1);
    assert(fx.reader.read_row(-1, 0, 5, row) == 0);
    assert(fx.reader.read_row(0, 4, 5, row) == 0);
    assert(fx.reader.read_pixel(std::numeric_limits<std::int32_t>::max(), 0) ==
           0);
    // borrows: screen for mutation only; captured pixels remain independent.
    auto* screen = lv_display_get_screen_active(fx.display.borrow_raw());
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x445566), 0);
    assert(fx.reader.capture_presented_frame(1));
    assert(fx.reader.read_pixel(0, 0) == 0xff112233U);
    fx.present(2, 0x445566);
    assert(fx.reader.read_pixel(0, 0) == 0xff445566U);
    // Different pixels on consecutive rows expose incorrect stride handling.
    auto child = lvglpp::LvObject::make_child(lvglpp::ObjectView{screen});
    lv_obj_remove_style_all(child.borrow_raw());
    lv_obj_set_pos(child.borrow_raw(), 5, 1);
    lv_obj_set_size(child.borrow_raw(), 2, 1);
    lv_obj_set_style_bg_color(child.borrow_raw(), lv_color_hex(0xabcdef), 0);
    lv_obj_set_style_bg_opa(child.borrow_raw(), LV_OPA_COVER, 0);
    fx.present(3, 0x445566);
    assert(fx.reader.read_pixel(5, 0) == 0xff445566U);
    assert(fx.reader.read_pixel(5, 1) == 0xffabcdefU);
    assert(fx.reader.read_pixel(6, 1) == 0xffabcdefU);
    assert(fx.reader.read_pixel(5, 2) == 0xff445566U);
}

void test_exact_wire_priming_and_present_gating() {
    Fixture fx;
    fx.present(1);
    fx.transport.feed("D0,0,2,2,2\r\n");
    assert(fx.executor.poll() == 1);
    assert(fx.transport.drain() == "DUMP:queued\r\n");
    assert(fx.executor.poll() == 0);
    assert(fx.transport.drain().empty());
    fx.present(2);
    (void)fx.executor.poll();
    assert(fx.transport.drain() ==
           "F\r\nFF112233 FF112233\r\nFF112233 FF112233\r\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain().empty());
    // A skipped count emits one frame, without synthesizing missing presents.
    fx.present(5, 0x445566);
    (void)fx.executor.poll();
    assert(fx.transport.drain() ==
           "F\r\nFF445566 FF445566\r\nFF445566 FF445566\r\nEND\r\n");
    fx.present(6);
    (void)fx.executor.poll();
    assert(fx.transport.drain().empty());
}

void test_zero_count_priming_is_unchanged() {
    Fixture fx;
    fx.present(0);
    fx.transport.feed("D0,0,1,1\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "DUMP:queued\r\n");
    // Preserve the existing zero-sentinel priming quirk, without changing it.
    fx.present(1);
    (void)fx.executor.poll();
    assert(fx.transport.drain().empty());
    fx.present(2);
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "F\r\nFF112233\r\nEND\r\n");
}

void test_failed_capture_retains_frame_and_gate() {
    Fixture fx;
    fx.present(1);
    fx.transport.feed("D0,0,1,1\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "DUMP:queued\r\n");
    {
        HandlerOverride handlers;
        handlers.fail_allocations();
        assert(!fx.reader.capture_presented_frame(2));
        assert(fx.reader.present_count() == 1);
        assert(fx.reader.read_pixel(0, 0) == 0xff112233U);
        (void)fx.executor.poll();
        assert(fx.transport.drain().empty());
    }
    fx.present(2, 0x445566);
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "F\r\nFF445566\r\nEND\r\n");
}

void test_crop_replacement_detach_and_rollover() {
    Fixture fx;
    fx.present(1);
    fx.transport.feed("D0,0,1,1,4\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "DUMP:queued\r\n");
    fx.present(2);
    fx.transport.feed("D6,3,4,2\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "DUMP:queued\r\n");
    fx.present(3);
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "F\r\nFF112233\r\n\r\nEND\r\n");
    fx.transport.feed("D0,0,1,1\n");
    (void)fx.executor.poll();
    (void)fx.transport.drain();
    fx.executor.set_framebuffer_reader(nullptr);
    fx.present(4);
    (void)fx.executor.poll();
    assert(fx.transport.drain().empty());
    fx.transport.feed("D0,0,1,1\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "ERR: not implemented\r\n");
    fx.executor.set_framebuffer_reader(&fx.reader);
    fx.present(std::numeric_limits<std::uint32_t>::max());
    fx.transport.feed("D0,0,1,1\n");
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "DUMP:queued\r\n");
    fx.present(0);
    (void)fx.executor.poll();
    assert(fx.transport.drain() == "F\r\nFF112233\r\nEND\r\n");
}
} // namespace
#endif

int main() {
#if LV_USE_SNAPSHOT
    static_assert(!std::is_copy_constructible_v<lp::LvglSnapshotReader>);
    static_assert(!std::is_move_constructible_v<lp::LvglSnapshotReader>);
    lvglpp::Runtime runtime;
    test_snapshot_pixels_and_immutable_rows();
    test_exact_wire_priming_and_present_gating();
    test_zero_count_priming_is_unchanged();
    test_failed_capture_retains_frame_and_gate();
    test_crop_replacement_detach_and_rollover();
    return 0;
#else
    std::puts("LV_USE_SNAPSHOT is disabled in the consumer configuration");
    return 77;
#endif
}
