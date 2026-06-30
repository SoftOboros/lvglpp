// scroll_test.cpp - LPAR-CPP-05 acceptance for LVGL scroll wrappers.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/input.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/scroll.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

void noop_flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) {
    lv_display_flush_ready(display);
}

struct DisplayFixture {
    std::array<std::uint8_t, 96 * 96 * 4> draw_buffer{};
    lvglpp::LvDisplay display;

    DisplayFixture() : display{lvglpp::LvDisplay::make(96, 96)} {
        assert(!display.empty());
        display.set_default();
        display.set_buffers(draw_buffer.data(),
                            nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
        display.set_flush_callback(noop_flush);
    }

    [[nodiscard]] lvglpp::ObjectView active_screen() const noexcept {
        return lvglpp::ObjectView{lv_display_get_screen_active(display.borrow_raw())};
    }
};

struct ScrollFixture {
    DisplayFixture display;
    lvglpp::LvObject container;
    lvglpp::LvObject child;

    ScrollFixture()
        : display{},
          container{lvglpp::LvObject::make_child(display.active_screen())},
          child{lvglpp::LvObject::make_child(container.borrow())} {
        container.add_flag(lvglpp::ObjectFlag::Scrollable);
        lv_obj_set_pos(container.borrow_raw(), 8, 8);
        lv_obj_set_size(container.borrow_raw(), 48, 40);

        child.add_flag(lvglpp::ObjectFlag::Snappable);
        lv_obj_set_pos(child.borrow_raw(), 0, 0);
        lv_obj_set_size(child.borrow_raw(), 48, 140);
        lv_obj_update_layout(container.borrow_raw());
    }
};

struct ScrollTrace {
    int begin = 0;
    int scroll = 0;
    int end = 0;
    lvglpp::EventCode::Kind seen[8]{};
    int count = 0;
};

void record_scroll(lv_event_t* raw) {
    lvglpp::EventView event{raw};
    auto* trace = static_cast<ScrollTrace*>(event.user_data());
    if (trace == nullptr) {
        return;
    }

    if (trace->count < 8) {
        trace->seen[trace->count++] = event.code().kind();
    }

    switch (event.code().kind()) {
        case lvglpp::EventCode::Kind::ScrollBegin: ++trace->begin; break;
        case lvglpp::EventCode::Kind::Scroll:      ++trace->scroll; break;
        case lvglpp::EventCode::Kind::ScrollEnd:   ++trace->end; break;
        default: break;
    }
}

void test_scroll_flag_and_event_code_mapping() {
    static_assert(lvglpp::to_lv(lvglpp::ObjectFlag::ScrollMomentum) ==
                  LV_OBJ_FLAG_SCROLL_MOMENTUM);
    static_assert(lvglpp::to_lv(lvglpp::ObjectFlag::Snappable) ==
                  LV_OBJ_FLAG_SNAPPABLE);

    assert(lvglpp::EventCode::from_lv(LV_EVENT_SCROLL_BEGIN).kind() ==
           lvglpp::EventCode::Kind::ScrollBegin);
    assert(lvglpp::EventCode::from_lv(LV_EVENT_SCROLL).kind() ==
           lvglpp::EventCode::Kind::Scroll);
    assert(lvglpp::EventCode::from_lv(LV_EVENT_SCROLL_END).kind() ==
           lvglpp::EventCode::Kind::ScrollEnd);
    assert(lvglpp::EventCode::from_lv(LV_EVENT_SCROLL_THROW_BEGIN).kind() ==
           lvglpp::EventCode::Kind::ScrollThrowBegin);
}

void test_modes_and_snap_wrappers() {
    ScrollFixture fixture;

    lvglpp::set_scroll_direction(fixture.container.borrow(),
                                 lvglpp::ScrollDirection::Vertical);
    assert(lvglpp::scroll_direction(fixture.container.borrow()) ==
           lvglpp::ScrollDirection::Vertical);

    lvglpp::set_scrollbar_mode(fixture.container.borrow(), lvglpp::ScrollbarMode::On);
    assert(lvglpp::scrollbar_mode(fixture.container.borrow()) ==
           lvglpp::ScrollbarMode::On);

    lvglpp::set_scroll_snap_y(fixture.container.borrow(), lvglpp::ScrollSnap::Start);
    assert(lvglpp::scroll_snap_y(fixture.container.borrow()) ==
           lvglpp::ScrollSnap::Start);
    lvglpp::set_scroll_snap_x(fixture.container.borrow(), lvglpp::ScrollSnap::Center);
    assert(lvglpp::scroll_snap_x(fixture.container.borrow()) ==
           lvglpp::ScrollSnap::Center);

    const auto areas = lvglpp::scrollbar_areas(fixture.container.borrow());
    assert(areas.vertical.y2 >= areas.vertical.y1);
    lvglpp::scrollbar_invalidate(fixture.container.borrow());
}

void test_programmatic_scroll_offsets_and_events() {
    ScrollFixture fixture;
    ScrollTrace trace;
    auto all_events = lvglpp::add_event_callback(
        fixture.container.borrow(), lvglpp::EventCode::all(), record_scroll, &trace);

    const auto initial_offset = lvglpp::scroll_offset(fixture.container.borrow());
    const lvglpp::ScrollOffset zero_offset{0, 0};
    assert(initial_offset == zero_offset);
    const auto extents = lvglpp::scroll_extents(fixture.container.borrow());
    assert(extents.bottom > 0);

    lvglpp::scroll_to_y(fixture.container.borrow(), 24, lvglpp::AnimationMode::Off);
    assert(lvglpp::scroll_offset(fixture.container.borrow()).y == 24);
    assert(trace.begin >= 1);
    assert(trace.scroll >= 1);
    assert(trace.end >= 1);

    lvglpp::scroll_by_bounded(
        fixture.container.borrow(), 0, -10, lvglpp::AnimationMode::Off);
    assert(lvglpp::scroll_offset(fixture.container.borrow()).y >= 24);

    lvglpp::scroll_to(fixture.container.borrow(), 0, 12, lvglpp::AnimationMode::Off);
    assert(lvglpp::scroll_offset(fixture.container.borrow()).y == 12);

    const auto end = lvglpp::scroll_end(fixture.container.borrow());
    assert(end.y == 12);
    assert(!lvglpp::is_scrolling(fixture.container.borrow()));
    lvglpp::stop_scroll_anim(fixture.container.borrow());
    lvglpp::readjust_scroll(fixture.container.borrow(), lvglpp::AnimationMode::Off);
    lvglpp::update_snap(fixture.container.borrow(), lvglpp::AnimationMode::Off);
}

void test_scroll_to_view_helpers() {
    ScrollFixture fixture;
    auto low_child = lvglpp::LvObject::make_child(fixture.container.borrow());
    lv_obj_set_pos(low_child.borrow_raw(), 0, 110);
    lv_obj_set_size(low_child.borrow_raw(), 20, 20);
    lv_obj_update_layout(fixture.container.borrow_raw());

    lvglpp::scroll_to_view(low_child.borrow(), lvglpp::AnimationMode::Off);
    assert(lvglpp::scroll_offset(fixture.container.borrow()).y > 0);

    lvglpp::scroll_to_y(fixture.container.borrow(), 0, lvglpp::AnimationMode::Off);
    assert(lvglpp::scroll_offset(fixture.container.borrow()).y == 0);
    lvglpp::scroll_to_view_recursive(low_child.borrow(), lvglpp::AnimationMode::Off);
    assert(lvglpp::scroll_offset(fixture.container.borrow()).y > 0);
}

struct PointerState {
    lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
    lv_point_t point{0, 0};
    int reads = 0;
};

void read_pointer(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* state = static_cast<PointerState*>(lv_indev_get_user_data(indev));
    assert(state != nullptr);
    ++state->reads;
    data->state = state->state;
    data->point = state->point;
}

void test_synthetic_input_can_drive_lvgl_scroll_path() {
    ScrollFixture fixture;
    ScrollTrace trace;
    auto all_events = lvglpp::add_event_callback(
        fixture.container.borrow(), lvglpp::EventCode::all(), record_scroll, &trace);

    PointerState pointer_state;
    auto pointer = lvglpp::LvInputDevice::make(lvglpp::InputDeviceType::Pointer);
    pointer.set_display(fixture.display.display.borrow());
    pointer.set_user_data(&pointer_state);
    pointer.set_read_callback(read_pointer);

    pointer_state = PointerState{LV_INDEV_STATE_PRESSED, lv_point_t{20, 34}, 0};
    pointer.read();
    pointer_state.point = lv_point_t{20, 8};
    pointer.read();
    pointer_state.state = LV_INDEV_STATE_RELEASED;
    pointer.read();

    assert(pointer_state.reads >= 3);
    assert(trace.begin >= 1 || trace.scroll >= 1);
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_scroll_flag_and_event_code_mapping();
    test_modes_and_snap_wrappers();
    test_programmatic_scroll_offsets_and_events();
    test_scroll_to_view_helpers();
    test_synthetic_input_can_drive_lvgl_scroll_path();

    return 0;
}
