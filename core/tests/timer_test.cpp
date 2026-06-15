// timer_test.cpp — LPAR-06 acceptance: RAII Timer over lv_timer_t and the
// Animation builder over lv_anim_t, exercised deterministically by feeding
// lv_tick_inc + lv_timer_handler (no wall clock).
//
// See docs/core-timer/00-timers-object-anim.md §12.

#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/timer.hpp"

#include <cassert>
#include <cstdint>
#include <utility>

using lvglpp::Runtime;
using lvglpp::core::Animation;
using lvglpp::core::Timer;

namespace {

void noop_flush(lv_display_t* disp, const lv_area_t* /*area*/, std::uint8_t* /*px*/) {
    lv_display_flush_ready(disp);
}

// Advance LVGL time by `total_ms` in `step_ms` slices, running the timer
// handler after each slice so periodic timers and the animation refresh
// timer fire deterministically.
void advance(std::uint32_t total_ms, std::uint32_t step_ms) {
    for (std::uint32_t elapsed = 0; elapsed < total_ms; elapsed += step_ms) {
        lv_tick_inc(step_ms);
        lv_timer_handler();
    }
}

// (a) Timer: a 10ms-period Timer whose callback increments a counter must
// fire several times across ~30ms of advanced ticks; destroying it stops it.
void test_timer_fires_and_tears_down() {
    int counter = 0;
    {
        Timer t = Timer::make([&counter]() { ++counter; }, 10U);
        assert(!t.empty());
        advance(35U, 5U);
        assert(counter >= 3);  // ~3 fires across 35ms at 10ms period

        // Move must keep the callback alive and bound to the live Timer.
        Timer moved = std::move(t);
        assert(t.empty());
        assert(!moved.empty());
        const int before = counter;
        advance(20U, 5U);
        assert(counter > before);  // still firing after move

        // pause/resume gate firing.
        moved.pause();
        const int paused_at = counter;
        advance(30U, 5U);
        assert(counter == paused_at);  // no fires while paused
        moved.resume();
        advance(20U, 5U);
        assert(counter > paused_at);   // fires resume
    }  // moved destroyed -> lv_timer_delete; no further fires.
    const int after_destroy = counter;
    advance(30U, 5U);
    assert(counter == after_destroy);
}

// Empty-Timer safety: every accessor is a no-op and never crashes.
void test_empty_timer_safe() {
    Timer t = Timer::make([]() {}, 10U);
    Timer moved = std::move(t);
    (void)moved;
    assert(t.empty());
    t.set_period(5U);  // no-ops below must not crash
    t.pause();
    t.resume();
    assert(t.borrow_raw() == nullptr);
}

// exec_cb writes the animated value into a captured-by-pointer int. lv_anim
// passes the var as the first arg, so we route through it (no globals).
void anim_exec(void* var, std::int32_t value) {
    *static_cast<std::int32_t*>(var) = value;
}

// (b) Animation: a 50ms 0->100 tween must move the target toward 100 as time
// advances. lv_anim_start copies the descriptor; the target is the key.
void test_animation_moves_value() {
    std::int32_t tracked = 0;

    Animation a;
    a.set_var(&tracked)
        .set_values(0, 100)
        .set_time(50U)
        .set_exec_cb(&anim_exec)
        .set_path_cb(lv_anim_path_linear);
    a.start();

    // Part-way through: value should have moved off 0 but not reached 100.
    advance(20U, 5U);
    assert(tracked > 0);
    assert(tracked < 100);

    // Past the end: linear tween lands on the end value.
    advance(60U, 5U);
    assert(tracked == 100);
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

    test_timer_fires_and_tears_down();
    test_empty_timer_safe();
    test_animation_moves_value();

    lv_display_delete(disp);
    return 0;
}
