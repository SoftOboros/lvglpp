// input_test.cpp — LPAR-04 acceptance: Object::on event callbacks, the
// lv_event_t ↔ CORE-02 Event seam (round-trip), FocusGroup, InputDevice.
//
// See docs/core-event/01-event-focus-input.md §12.

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/input.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"

#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

using lvglpp::ObjectView;
using lvglpp::Runtime;
using lvglpp::core::EventCode;
using lvglpp::core::FocusGroup;
using lvglpp::core::InputDevice;
using lvglpp::core::InputType;
using lvglpp::core::Object;
using lvglpp::core::Screen;
namespace ev = lvglpp::core::event;
namespace k = lvglpp::core::key;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

void noop_read(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
    data->state = LV_INDEV_STATE_RELEASED;
}

// Object::on (zero-arg overload) fires when the event is dispatched.
void test_on_zero_arg() {
    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());

    int hits = 0;
    obj.on(EventCode::Clicked, [&hits] { ++hits; });

    assert(lv_obj_send_event(obj.borrow_raw(), LV_EVENT_CLICKED, nullptr) == LV_RESULT_OK);
    assert(hits == 1);
    lv_obj_send_event(obj.borrow_raw(), LV_EVENT_CLICKED, nullptr);
    assert(hits == 2);
}

// Object::on (lv_event_t* overload) receives the live event; event_from_lv
// recovers the CORE-02 view.
void test_on_event_arg() {
    Screen screen = Screen::make();
    Object obj = Object::make(screen.view());

    int hits = 0;
    bool saw_press_down = false;
    obj.on(EventCode::Pressed, [&](lv_event_t* e) {
        ++hits;
        std::optional<lvglpp::core::Event> mapped = lvglpp::core::event_from_lv(e);
        saw_press_down = mapped.has_value() && std::holds_alternative<ev::PressDown>(*mapped);
    });

    lv_obj_send_event(obj.borrow_raw(), LV_EVENT_PRESSED, nullptr);
    assert(hits == 1);
    assert(saw_press_down);
}

// A registered handler survives moving the owning Object (the holder address
// is stable; the per-cb user_data still points at it).
void test_on_survives_move() {
    Screen screen = Screen::make();
    int hits = 0;

    Object o = Object::make(screen.view());
    o.on(EventCode::Clicked, [&hits] { ++hits; });

    Object moved = std::move(o);
    assert(o.empty());

    lv_obj_send_event(moved.borrow_raw(), LV_EVENT_CLICKED, nullptr);
    assert(hits == 1);
}

// on() on an empty (moved-from) Object is a no-op.
void test_on_empty_safe() {
    Object o = Object::make(ObjectView{lv_screen_active()});
    Object moved = std::move(o);
    (void)moved;
    assert(o.empty());
    o.on(EventCode::Clicked, [] {});                  // no-op, must not crash
    o.on(EventCode::Pressed, [](lv_event_t*) {});     // no-op
}

// The event-code seam round-trips for the representable subset.
void test_seam_code_roundtrip() {
    const lv_event_code_t codes[] = {LV_EVENT_PRESSED, LV_EVENT_RELEASED,
                                     LV_EVENT_PRESSING, LV_EVENT_KEY};
    for (lv_event_code_t code : codes) {
        std::optional<lvglpp::core::Event> mapped =
            lvglpp::core::event_of_code(code, 7, 9, LV_KEY_UP);
        assert(mapped.has_value());
        std::optional<lv_event_code_t> back = lvglpp::core::lv_code_of(*mapped);
        assert(back.has_value() && *back == code);
    }

    // Pointer payload survives the mapping.
    auto down = lvglpp::core::event_of_code(LV_EVENT_PRESSED, 7, 9, 0);
    assert(down.has_value());
    const auto* pd = std::get_if<ev::PressDown>(&*down);
    assert(pd != nullptr && pd->x == 7 && pd->y == 9);

    // A non-representable code yields nullopt.
    assert(!lvglpp::core::event_of_code(LV_EVENT_VALUE_CHANGED, 0, 0, 0).has_value());
    // A non-representable Event yields nullopt.
    assert(!lvglpp::core::lv_code_of(lvglpp::core::Event{ev::Tick{}}).has_value());
}

// Key mapping round-trips for the named + character set.
void test_seam_key_roundtrip() {
    const lvglpp::core::Key keys[] = {
        k::ArrowUp{},   k::ArrowDown{}, k::ArrowLeft{}, k::ArrowRight{},
        k::Escape{},    k::Enter{},     k::Space{},     k::Character{0x41},
    };
    for (const lvglpp::core::Key& key : keys) {
        const lvglpp::core::Key back = lvglpp::core::key_from_lv(lvglpp::core::lv_key_of(key));
        assert(back == key);
    }
}

// event_to_indev fills the indev data for the replayable subset.
void test_event_to_indev() {
    lv_indev_data_t data{};

    assert(lvglpp::core::event_to_indev(lvglpp::core::Event{ev::PressDown{3, 4}}, &data));
    assert(data.point.x == 3 && data.point.y == 4 && data.state == LV_INDEV_STATE_PRESSED);

    assert(lvglpp::core::event_to_indev(lvglpp::core::Event{ev::PressRelease{5, 6}}, &data));
    assert(data.point.x == 5 && data.point.y == 6 && data.state == LV_INDEV_STATE_RELEASED);

    assert(lvglpp::core::event_to_indev(lvglpp::core::Event{ev::KeyDown{k::Enter{}}}, &data));
    assert(data.key == static_cast<std::uint32_t>(LV_KEY_ENTER) &&
           data.state == LV_INDEV_STATE_PRESSED);

    // Non-representable Event leaves the result false.
    assert(!lvglpp::core::event_to_indev(lvglpp::core::Event{ev::Tick{}}, &data));
    assert(!lvglpp::core::event_to_indev(lvglpp::core::Event{ev::PressDown{0, 0}}, nullptr));
}

// FocusGroup RAII + navigation + empty-safety.
void test_focus_group() {
    Screen screen = Screen::make();
    Object a = Object::make(screen.view());
    Object b = Object::make(screen.view());

    FocusGroup group = FocusGroup::make();
    assert(!group.empty());
    group.add(a.view());
    group.add(b.view());
    FocusGroup::focus(a.view());
    group.focus_next();
    group.focus_prev();
    group.set_editing(true);
    group.set_editing(false);
    FocusGroup::remove(b.view());

    FocusGroup moved = std::move(group);
    assert(group.empty());
    assert(!moved.empty());
    group.add(a.view());     // no-op on empty
    group.focus_next();      // no-op
    group.set_editing(true); // no-op
}

// InputDevice RAII + setters + empty-safety.
void test_input_device(lv_display_t* disp) {
    FocusGroup group = FocusGroup::make();

    InputDevice indev = InputDevice::make();
    assert(!indev.empty());
    indev.set_type(InputType::Keypad);
    indev.set_read_cb(noop_read);
    indev.set_group(group);
    indev.set_display(disp);
    indev.set_user_data(nullptr);

    InputDevice moved = std::move(indev);
    assert(indev.empty());
    assert(!moved.empty());
    indev.set_type(InputType::Pointer);  // no-op on empty
    indev.set_read_cb(noop_read);        // no-op
    indev.set_group(group);              // no-op
}

}  // namespace

int main() {
    auto runtime = Runtime::try_make();
    assert(runtime.has_value());

    static std::uint8_t draw_buf[100 * 20 * 4];
    lv_display_t* disp = lv_display_create(100, 100);
    assert(disp != nullptr);
    lv_display_set_flush_cb(disp, noop_flush);
    lv_display_set_buffers(disp, draw_buf, nullptr,
                           static_cast<std::uint32_t>(sizeof(draw_buf)),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    test_on_zero_arg();
    test_on_event_arg();
    test_on_survives_move();
    test_on_empty_safe();
    test_seam_code_roundtrip();
    test_seam_key_roundtrip();
    test_event_to_indev();
    test_focus_group();
    test_input_device(disp);

    lv_display_delete(disp);
    return 0;
}
