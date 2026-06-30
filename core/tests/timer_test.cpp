// timer_test.cpp - LPAR-CPP-06 acceptance for LVGL timers/animations.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/timer.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

namespace {

void drive_lvgl(std::uint32_t ms) {
    lv_tick_inc(ms);
    static_cast<void>(lvglpp::run_timers());
}

struct TimerTrace {
    int calls = 0;
    int last_value = 0;
};

void timer_callback(lv_timer_t* timer) {
    auto* trace = static_cast<TimerTrace*>(lv_timer_get_user_data(timer));
    assert(trace != nullptr);
    ++trace->calls;
    trace->last_value = trace->calls * 10;
}

void alternate_timer_callback(lv_timer_t* timer) {
    auto* trace = static_cast<TimerTrace*>(lv_timer_get_user_data(timer));
    assert(trace != nullptr);
    ++trace->calls;
    trace->last_value = 99;
}

void test_timer_owner_view_and_release() {
    TimerTrace trace;
    auto timer = lvglpp::LvTimer::make(timer_callback, 20, &trace);
    assert(!timer.empty());
    assert(timer.borrow_raw() != nullptr);
    assert(timer.user_data() == &trace);

    auto view = timer.borrow();
    assert(view.borrow_raw() == timer.borrow_raw());
    view.set_period(10);
    view.ready();
    drive_lvgl(0);
    assert(trace.calls == 1);
    assert(trace.last_value == 10);

    timer.set_callback(alternate_timer_callback);
    timer.ready();
    drive_lvgl(0);
    assert(trace.calls == 2);
    assert(trace.last_value == 99);

    lv_timer_t* raw = timer.release();
    assert(timer.empty());
    assert(raw != nullptr);
    lv_timer_delete(raw);
}

void test_timer_pause_reset_repeat_and_move() {
    TimerTrace trace;
    auto timer = lvglpp::LvTimer::make(timer_callback, 30, &trace);
    timer.pause();
    assert(timer.is_paused());
    drive_lvgl(40);
    assert(trace.calls == 0);

    timer.resume();
    timer.reset_period();
    drive_lvgl(29);
    assert(trace.calls == 0);
    drive_lvgl(1);
    assert(trace.calls == 1);

    timer.set_repeat_count(1);
    timer.ready();
    drive_lvgl(0);
    assert(trace.calls == 2);
    drive_lvgl(40);
    assert(trace.calls == 2);
    assert(timer.is_paused());

    auto moved = std::move(timer);
    assert(timer.empty());
    assert(!moved.empty());
}

void test_timer_release_with_lvgl_auto_delete() {
    TimerTrace trace;
    auto timer = lvglpp::LvTimer::make(timer_callback, 10, &trace);
    timer.set_repeat_count(1);
    lv_timer_t* raw = timer.release_with_auto_delete();
    assert(timer.empty());
    assert(raw != nullptr);

    drive_lvgl(11);
    assert(trace.calls == 1);
}

struct AnimationTrace {
    int32_t value = -1;
    int starts = 0;
    int completed = 0;
    int deleted = 0;
};

void exec_value(void* var, std::int32_t value) {
    auto* trace = static_cast<AnimationTrace*>(var);
    assert(trace != nullptr);
    trace->value = value;
}

void custom_exec_value(lv_anim_t* animation, std::int32_t value) {
    auto* trace = static_cast<AnimationTrace*>(animation->var);
    assert(trace != nullptr);
    trace->value = value;
}

void anim_started(lv_anim_t* animation) {
    auto* trace = static_cast<AnimationTrace*>(lv_anim_get_user_data(animation));
    assert(trace != nullptr);
    ++trace->starts;
}

void anim_completed(lv_anim_t* animation) {
    auto* trace = static_cast<AnimationTrace*>(lv_anim_get_user_data(animation));
    assert(trace != nullptr);
    ++trace->completed;
}

void anim_deleted(lv_anim_t* animation) {
    auto* trace = static_cast<AnimationTrace*>(lv_anim_get_user_data(animation));
    assert(trace != nullptr);
    ++trace->deleted;
}

void test_animation_start_find_pause_resume_and_callbacks() {
    AnimationTrace trace;
    lvglpp::AnimationTemplate animation;
    animation.set_var(&trace)
        .set_values(0, 100)
        .set_exec_callback(exec_value)
        .set_duration(100)
        .set_start_callback(anim_started)
        .set_completed_callback(anim_completed)
        .set_deleted_callback(anim_deleted)
        .set_user_data(&trace);

    auto running = animation.start();
    assert(!running.empty());
    assert(running.user_data() == &trace);
    assert(running.duration() == 100);
    assert(running.key().find().borrow_raw() == running.borrow_raw());
    assert(animation.find_matching().borrow_raw() == running.borrow_raw());
    assert(lvglpp::running_animation_count() >= 1);

    drive_lvgl(40);
    assert(trace.starts == 1);
    assert(trace.value > 0);
    const auto paused_value = trace.value;
    running.pause();
    assert(running.is_paused());
    drive_lvgl(40);
    assert(trace.value == paused_value);
    running.resume();
    drive_lvgl(70);
    assert(trace.value == 100);
    assert(trace.completed == 1);
    assert(trace.deleted == 1);
}

void test_animation_cancel_repeat_reverse_delay_and_custom_callback() {
    AnimationTrace trace;
    lvglpp::AnimationTemplate animation;
    animation.set_var(&trace)
        .set_values(0, 50)
        .set_custom_exec_callback(custom_exec_value)
        .set_duration(100)
        .set_delay(5)
        .set_repeat_count(1)
        .set_repeat_delay(5)
        .set_reverse_duration(20)
        .set_reverse_delay(5)
        .set_early_apply(false)
        .set_deleted_callback(anim_deleted)
        .set_user_data(&trace);

    auto running = animation.start();
    assert(!running.empty());
    assert(running.delay() == 5);
    assert(running.repeat_count() == 1);
    assert(animation.find_matching().borrow_raw() == running.borrow_raw());

    drive_lvgl(50);
    assert(trace.value >= 0);
    assert(animation.cancel_matching());
    const auto cancelled_value = trace.value;
    drive_lvgl(80);
    assert(trace.value == cancelled_value);
    assert(trace.deleted == 1);
    assert(animation.find_matching().empty());
    lvglpp::delete_all_animations();
}

void set_object_x(void* var, std::int32_t value) {
    lv_obj_set_x(static_cast<lv_obj_t*>(var), value);
}

void test_object_property_animation_uses_lvgl_object_target() {
    auto display = lvglpp::LvDisplay::make(80, 80);
    display.set_default();

    auto screen = lvglpp::LvObject::make_screen();
    auto child = lvglpp::LvObject::make_child(screen.borrow());
    lv_obj_set_x(child.borrow_raw(), 0);

    lvglpp::AnimationTemplate animation;
    animation.set_target(child.borrow())
        .set_values(0, 30)
        .set_exec_callback(set_object_x)
        .set_duration(60);

    auto running = animation.start();
    assert(!running.empty());
    drive_lvgl(80);
    lv_obj_update_layout(screen.borrow_raw());
    assert(lv_obj_get_x(child.borrow_raw()) == 30);
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_object_property_animation_uses_lvgl_object_target();
    test_timer_owner_view_and_release();
    test_timer_pause_reset_repeat_and_move();
    test_timer_release_with_lvgl_auto_delete();
    test_animation_start_find_pause_resume_and_callbacks();
    test_animation_cancel_repeat_reverse_delay_and_custom_callback();
    lvglpp::delete_all_animations();

    return 0;
}
