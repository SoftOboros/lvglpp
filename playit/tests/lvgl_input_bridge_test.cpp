// lvgl_input_bridge_test.cpp - playit EventSpec injection into LVGL indevs.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/input.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/playit/playit.hpp"

#include <array>
#include <cassert>
#include <cstdint>

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

struct Trace {
    int clicked = 0;
    int keyed = 0;
    std::uint32_t key = 0;
};

void record_clicked(lv_event_t* raw) {
    lvglpp::EventView event{raw};
    auto* trace = static_cast<Trace*>(event.user_data());
    if (trace != nullptr) {
        ++trace->clicked;
    }
}

void record_key(lv_event_t* raw) {
    lvglpp::EventView event{raw};
    auto* trace = static_cast<Trace*>(event.user_data());
    if (trace != nullptr) {
        ++trace->keyed;
        trace->key = event.key();
    }
}

void test_playit_pointer_and_key_specs_feed_lvgl_input() {
    DisplayFixture fixture;
    Trace trace;

    auto clickable = lvglpp::LvObject::make_child(fixture.active_screen());
    clickable.add_flag(lvglpp::ObjectFlag::Clickable);
    lv_obj_set_pos(clickable.borrow_raw(), 4, 4);
    lv_obj_set_size(clickable.borrow_raw(), 20, 20);
    lv_obj_update_layout(clickable.borrow_raw());

    auto clicked_sub = lvglpp::add_event_callback(
        clickable.borrow(), lvglpp::EventCode::clicked(), record_clicked, &trace);

    auto pointer = lvglpp::LvInputDevice::make(lvglpp::InputDeviceType::Pointer);
    pointer.set_display(fixture.display.borrow());

    auto focus = lvglpp::LvObject::make_child(fixture.active_screen());
    auto group = lvglpp::LvGroup::make();
    group.add_object(focus.borrow());
    group.focus_object(focus.borrow());
    auto key_sub = lvglpp::add_event_callback(
        focus.borrow(), lvglpp::EventCode::key(), record_key, &trace);

    auto keypad = lvglpp::LvInputDevice::make(lvglpp::InputDeviceType::Keypad);
    keypad.set_group(group.borrow());

    lvglpp::playit::LvglInputBridge bridge;
    bridge.bind_pointer(pointer.borrow());
    bridge.bind_keypad(keypad.borrow());

    assert(bridge.inject(lvglpp::playit::EventSpec{
        lvglpp::playit::event_spec::PressRelease{8, 9}}));
    assert(bridge.pointer_reads() >= 2);
    assert(trace.clicked >= 1);

    assert(bridge.inject(lvglpp::playit::EventSpec{
        lvglpp::playit::event_spec::KeyDown{
            lvglpp::playit::KeySpec{lvglpp::playit::KeySpec::Kind::Enter, 0}}}));
    assert(bridge.keypad_reads() >= 1);
    assert(trace.keyed >= 1);
    assert(trace.key == LV_KEY_ENTER);

    assert(!bridge.inject(lvglpp::playit::EventSpec{
        lvglpp::playit::event_spec::Tick{}}));

    bridge.detach_keypad();
    bridge.detach_pointer();
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;
    test_playit_pointer_and_key_specs_feed_lvgl_input();
    return 0;
}
