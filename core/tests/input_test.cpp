// input_test.cpp - LPAR-CPP-04 acceptance for LVGL event/input wrappers.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/input.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <utility>

namespace {

void noop_flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) {
    lv_display_flush_ready(display);
}

struct DisplayFixture {
    std::array<std::uint8_t, 64 * 64 * 4> draw_buffer{};
    lvglpp::LvDisplay display;

    DisplayFixture() : display{lvglpp::LvDisplay::make(64, 64)} {
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

struct EventTrace {
    int calls = 0;
    lvglpp::EventCode::Kind code[4]{};
    lv_obj_t* target[4]{};
    lv_obj_t* current[4]{};
    void* user_data[4]{};
    std::uint32_t key[4]{};
    std::int32_t rotary[4]{};
};

void record_event(lv_event_t* raw) {
    lvglpp::EventView event{raw};
    auto* trace = static_cast<EventTrace*>(event.user_data());
    if (trace == nullptr || trace->calls >= 4) {
        return;
    }

    const int index = trace->calls++;
    trace->code[index] = event.code().kind();
    trace->target[index] = event.target().borrow_raw();
    trace->current[index] = event.current_target().borrow_raw();
    trace->user_data[index] = event.user_data();
    trace->key[index] = event.key();
    trace->rotary[index] = event.rotary_diff();
}

void test_event_code_mapping_preserves_unknown() {
    assert(lvglpp::EventCode::from_lv(LV_EVENT_CLICKED).kind() ==
           lvglpp::EventCode::Kind::Clicked);
    assert(lvglpp::EventCode::from_lv(LV_EVENT_ROTARY).kind() ==
           lvglpp::EventCode::Kind::Rotary);

    const auto custom = static_cast<lv_event_code_t>(LV_EVENT_LAST + 42);
    const auto code = lvglpp::EventCode::from_lv(custom);
    assert(code.kind() == lvglpp::EventCode::Kind::Other);
    assert(code.raw() == custom);
    assert(!code.known());
}

void test_event_subscription_removes_descriptor() {
    DisplayFixture fixture;
    auto object = lvglpp::LvObject::make_child(fixture.active_screen());
    EventTrace trace;

    {
        auto subscription = lvglpp::add_event_callback(
            object.borrow(), lvglpp::EventCode::clicked(), record_event, &trace);
        assert(!subscription.empty());
        assert(subscription.borrow_descriptor() != nullptr);

        assert(lvglpp::send_event(object.borrow(), lvglpp::EventCode::clicked()) ==
               LV_RESULT_OK);
        assert(trace.calls == 1);
        assert(trace.code[0] == lvglpp::EventCode::Kind::Clicked);
        assert(trace.target[0] == object.borrow_raw());
        assert(trace.current[0] == object.borrow_raw());
        assert(trace.user_data[0] == &trace);
    }

    assert(lvglpp::send_event(object.borrow(), lvglpp::EventCode::clicked()) ==
           LV_RESULT_OK);
    assert(trace.calls == 1);
}

void test_event_bubbling_reports_target_and_current_target() {
    DisplayFixture fixture;
    auto parent = lvglpp::LvObject::make_child(fixture.active_screen());
    auto child = lvglpp::LvObject::make_child(parent.borrow());
    child.add_flag(lvglpp::ObjectFlag::EventBubble);

    EventTrace trace;
    auto parent_sub = lvglpp::add_event_callback(
        parent.borrow(), lvglpp::EventCode::clicked(), record_event, &trace);
    auto child_sub = lvglpp::add_event_callback(
        child.borrow(), lvglpp::EventCode::clicked(), record_event, &trace);
    assert(!parent_sub.empty());
    assert(!child_sub.empty());

    assert(lvglpp::send_event(child.borrow(), lvglpp::EventCode::clicked()) ==
           LV_RESULT_OK);
    assert(trace.calls == 2);
    assert(trace.target[0] == child.borrow_raw());
    assert(trace.current[0] == child.borrow_raw());
    assert(trace.target[1] == child.borrow_raw());
    assert(trace.current[1] == parent.borrow_raw());
}

void test_group_focus_traversal_and_move() {
    DisplayFixture fixture;
    auto first = lvglpp::LvObject::make_child(fixture.active_screen());
    auto second = lvglpp::LvObject::make_child(fixture.active_screen());

    auto group = lvglpp::LvGroup::make();
    assert(!group.empty());
    group.add_object(first.borrow());
    group.add_object(second.borrow());
    assert(group.object_count() == 2);
    assert(group.object_by_index(0).borrow_raw() == first.borrow_raw());

    group.focus_object(first.borrow());
    assert(group.focused().borrow_raw() == first.borrow_raw());
    group.focus_next();
    assert(group.focused().borrow_raw() == second.borrow_raw());
    group.focus_prev();
    assert(group.focused().borrow_raw() == first.borrow_raw());

    group.set_wrap(false);
    assert(!group.wrap());
    group.set_wrap(true);
    assert(group.wrap());
    group.set_editing(true);
    assert(group.editing());

    const lv_group_t* raw = group.borrow_raw();
    auto moved = lvglpp::LvGroup{std::move(group)};
    assert(group.empty());
    assert(moved.borrow_raw() == raw);
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

struct KeyState {
    lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
    std::uint32_t key = LV_KEY_ENTER;
    int reads = 0;
};

void read_key(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* state = static_cast<KeyState*>(lv_indev_get_user_data(indev));
    assert(state != nullptr);
    ++state->reads;
    data->state = state->state;
    data->key = state->key;
}

struct EncoderState {
    lv_indev_state_t state = LV_INDEV_STATE_RELEASED;
    std::int16_t diff = 0;
    int reads = 0;
};

void read_encoder(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* state = static_cast<EncoderState*>(lv_indev_get_user_data(indev));
    assert(state != nullptr);
    ++state->reads;
    data->state = state->state;
    data->enc_diff = state->diff;
}

void test_input_devices_read_synthetic_state() {
    DisplayFixture fixture;
    auto object = lvglpp::LvObject::make_child(fixture.active_screen());
    object.add_flag(lvglpp::ObjectFlag::Clickable);
    lv_obj_set_pos(object.borrow_raw(), 4, 4);
    lv_obj_set_size(object.borrow_raw(), 20, 20);
    lv_obj_update_layout(object.borrow_raw());

    EventTrace pointer_trace;
    auto pressed_sub = lvglpp::add_event_callback(
        object.borrow(), lvglpp::EventCode::pressed(), record_event, &pointer_trace);

    PointerState pointer_state{LV_INDEV_STATE_PRESSED, lv_point_t{8, 9}, 0};
    auto pointer = lvglpp::LvInputDevice::make(lvglpp::InputDeviceType::Pointer);
    assert(pointer.type() == lvglpp::InputDeviceType::Pointer);
    pointer.set_display(fixture.display.borrow());
    pointer.set_user_data(&pointer_state);
    pointer.set_read_callback(read_pointer);
    pointer.read();
    assert(pointer_state.reads > 0);
    assert(pointer.state() == lvglpp::InputDeviceState::Pressed);
    assert(pointer_trace.calls >= 1);

    pointer_state.state = LV_INDEV_STATE_RELEASED;
    pointer.read();
    assert(pointer.state() == lvglpp::InputDeviceState::Released);

    auto focus = lvglpp::LvObject::make_child(fixture.active_screen());
    auto group = lvglpp::LvGroup::make();
    group.add_object(focus.borrow());
    group.focus_object(focus.borrow());

    EventTrace key_trace;
    auto key_sub = lvglpp::add_event_callback(
        focus.borrow(), lvglpp::EventCode::key(), record_event, &key_trace);
    KeyState key_state{LV_INDEV_STATE_PRESSED, LV_KEY_ENTER, 0};
    auto keypad = lvglpp::LvInputDevice::make(lvglpp::InputDeviceType::Keypad);
    keypad.set_group(group.borrow());
    keypad.set_user_data(&key_state);
    keypad.set_read_callback(read_key);
    keypad.read();
    assert(key_state.reads > 0);
    assert(keypad.group().borrow_raw() == group.borrow_raw());
    assert(key_trace.calls >= 1);
    assert(key_trace.key[0] == LV_KEY_ENTER);

    EventTrace rotary_trace;
    auto rotary_sub = lvglpp::add_event_callback(
        focus.borrow(), lvglpp::EventCode::rotary(), record_event, &rotary_trace);
    EncoderState encoder_state{LV_INDEV_STATE_RELEASED, 3, 0};
    auto encoder = lvglpp::LvInputDevice::make(lvglpp::InputDeviceType::Encoder);
    encoder.set_group(group.borrow());
    encoder.set_user_data(&encoder_state);
    encoder.set_read_callback(read_encoder);
    encoder.read();
    assert(encoder_state.reads > 0);
    assert(encoder.type() == lvglpp::InputDeviceType::Encoder);
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_event_code_mapping_preserves_unknown();
    test_event_subscription_removes_descriptor();
    test_event_bubbling_reports_target_and_current_target();
    test_group_focus_traversal_and_move();
    test_input_devices_read_synthetic_state();

    return 0;
}
