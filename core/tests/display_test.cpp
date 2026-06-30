// display_test.cpp - LPAR-CPP-03 acceptance for LVGL display wrappers.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

struct Recorder {
    std::vector<lvglpp::LvArea> invalidated;
    std::vector<lvglpp::LvArea> flushed;
    bool saw_px_map = false;
};

void on_invalidate(lv_event_t* event) {
    auto* recorder = static_cast<Recorder*>(lv_event_get_user_data(event));
    lv_area_t* area = lv_event_get_invalidated_area(event);
    if (recorder != nullptr && area != nullptr) {
        recorder->invalidated.push_back(lvglpp::LvArea::from_lv(*area));
    }
}

void on_flush(lv_display_t* display, const lv_area_t* area, std::uint8_t* px_map) {
    auto* recorder = static_cast<Recorder*>(lv_display_get_user_data(display));
    if (recorder != nullptr && area != nullptr) {
        recorder->flushed.push_back(lvglpp::LvArea::from_lv(*area));
        recorder->saw_px_map = recorder->saw_px_map || px_map != nullptr;
    }
    lv_display_flush_ready(display);
}

struct DisplayFixture {
    Recorder recorder;
    std::array<std::uint8_t, 64 * 64 * 4> draw_buffer{};
    lvglpp::LvDisplay display;

    DisplayFixture() : display{lvglpp::LvDisplay::make(64, 64)} {
        assert(!display.empty());
        display.set_default();
        // observes: recorder outlives display and callbacks in this fixture.
        lv_display_set_user_data(display.borrow_raw(), &recorder);
        lv_display_add_event_cb(
            display.borrow_raw(), on_invalidate, LV_EVENT_INVALIDATE_AREA, &recorder);
        display.set_buffers(draw_buffer.data(),
                            nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
        display.set_flush_callback(on_flush);
    }

    [[nodiscard]] lvglpp::ObjectView active_screen() const noexcept {
        return lvglpp::ObjectView{lv_display_get_screen_active(display.borrow_raw())};
    }
};

void test_area_conversion_is_inclusive() {
    constexpr auto area = lvglpp::LvArea::from_xywh(3, 4, 10, 20);
    static_assert(area == lvglpp::LvArea::from_inclusive(3, 4, 12, 23));

    const lv_area_t lv = area.to_lv();
    assert(lv.x1 == 3);
    assert(lv.y1 == 4);
    assert(lv.x2 == 12);
    assert(lv.y2 == 23);
    assert(lvglpp::LvArea::from_lv(lv) == area);
}

void test_display_move_and_release() {
    auto display = lvglpp::LvDisplay::make(16, 16);
    assert(!display.empty());

    lv_display_t* raw = display.borrow_raw();
    lvglpp::LvDisplay moved{std::move(display)};
    assert(display.empty());
    assert(moved.borrow_raw() == raw);

    raw = moved.release();
    assert(moved.empty());
    assert(raw != nullptr);
    lv_display_delete(raw);
}

void test_default_display_selection() {
    auto first = lvglpp::LvDisplay::make(20, 20);
    auto next  = lvglpp::LvDisplay::make(30, 30);

    first.set_default();
    assert(lv_display_get_default() == first.borrow_raw());
    next.set_default();
    assert(lv_display_get_default() == next.borrow_raw());
}

void test_object_invalidation_records_area() {
    DisplayFixture fixture;
    auto child = lvglpp::LvObject::make_child(fixture.active_screen());
    lv_obj_set_pos(child.borrow_raw(), 5, 6);
    lv_obj_set_size(child.borrow_raw(), 20, 20);
    lv_obj_update_layout(child.borrow_raw());

    fixture.recorder.invalidated.clear();
    lv_area_t coords{};
    lv_obj_get_coords(child.borrow_raw(), &coords);
    const auto area = lvglpp::LvArea::from_lv(coords);
    assert(lvglpp::invalidate_area(child.borrow(), area) == LV_RESULT_OK);
    assert(!fixture.recorder.invalidated.empty());
    assert(fixture.recorder.invalidated.back() == area);

    assert(lvglpp::invalidate(lvglpp::ObjectView{nullptr}) == LV_RESULT_INVALID);
    assert(lvglpp::invalidate_area(lvglpp::ObjectView{nullptr}, area) == LV_RESULT_INVALID);
}

void test_refresh_flush_callback_records_area() {
    DisplayFixture fixture;
    auto child = lvglpp::LvObject::make_child(fixture.active_screen());
    lv_obj_set_pos(child.borrow_raw(), 10, 11);
    lv_obj_set_size(child.borrow_raw(), 8, 9);
    lv_obj_update_layout(child.borrow_raw());

    fixture.recorder.flushed.clear();
    fixture.recorder.saw_px_map = false;

    assert(lvglpp::invalidate(child.borrow()) == LV_RESULT_OK);
    lv_refr_now(fixture.display.borrow_raw());

    assert(!fixture.recorder.flushed.empty());
    assert(fixture.recorder.saw_px_map);
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_area_conversion_is_inclusive();
    test_display_move_and_release();
    test_default_display_selection();
    test_object_invalidation_records_area();
    test_refresh_flush_callback_records_area();

    return 0;
}
