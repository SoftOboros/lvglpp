// timer.hpp — RAII over lv_timer_t and a value builder over lv_anim_t
// (LPAR-06). See docs/core-timer/00-timers-object-anim.md.
//
// PARITY: rlvgl/core/src/timer.rs + core::anim (ANIM-00) (v0.2.4 @ 343f596).
//         The ownership story matches rlvgl: a Timer exclusively owns its
//         callback and tears it down on drop; an Animation is a deterministic
//         tick-driven tween descriptor whose running instance is owned by the
//         framework registry once started.
// LVGL:   lvgl/src/misc/lv_timer.h (lv_timer_create / lv_timer_delete /
//         lv_timer_get_user_data), lvgl/src/misc/lv_anim.h (lv_anim_init /
//         lv_anim_start), lvgl/src/tick/lv_tick.h (lv_tick_inc).
// DELTA:  Timer is move-only and rebinds the lv_timer user_data back-pointer
//         on move (mirrors Object) so the static trampoline always recovers
//         the live Timer and its owned std::function. Animation is a
//         lightweight builder: lv_anim_start COPIES the descriptor, so the
//         running animation is LVGL-owned and keyed by (var, exec_cb) — the
//         Animation value does NOT own or stop the running instance.

#ifndef LVGLPP_CORE_TIMER_HPP
#define LVGLPP_CORE_TIMER_HPP

#include <cstdint>
#include <functional>

#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core {

// Errors that the Timer factory can produce.
enum class TimerError : std::uint8_t {
    CreateFailed = 1,  // lv_timer_create returned nullptr (e.g. OOM).
};

// RAII owner of a single lv_timer_t and the C++ callback it drives.
//
// Ownership: owns its lv_timer_t and the std::function callback. The
// destructor calls lv_timer_delete iff it still owns a live timer. A static
// trampoline of type lv_timer_cb_t (void(lv_timer_t*)) recovers `this` from
// the timer's user_data and invokes the owned function. The user_data
// back-pointer is set at create and rebound on move so the trampoline always
// finds the live Timer (mirrors core::Object's LV_EVENT_DELETE pattern).
class Timer {
public:
    // Create a periodic timer.
    // Args:
    //   on_tick:   owns; the callback is moved into this Timer and invoked
    //              every period until the Timer is destroyed.
    //   period_ms: call period in milliseconds.
    // Returns: owns Timer on success; TimerError on failure.
    [[nodiscard]] static lvglpp::expected<Timer, TimerError>
    try_make(std::function<void()> on_tick, std::uint32_t period_ms) noexcept;

    // Throwing convenience over try_make: aborts under embedded posture,
    // throws std::runtime_error on host, if creation fails. Mirrors the
    // Object factory pattern.
    [[nodiscard]] static Timer make(std::function<void()> on_tick,
                                    std::uint32_t period_ms);

    Timer(const Timer&)            = delete;
    Timer& operator=(const Timer&) = delete;

    // Move-construct only: transfers the handle and the owned callback, then
    // rebinds the LVGL user_data back-pointer to the new location. The
    // moved-from Timer is left empty so its destructor is a no-op.
    // Move-assignment is deleted (mirrors Object).
    Timer(Timer&& other) noexcept;
    Timer& operator=(Timer&&) = delete;

    ~Timer();

    // All accessors below are empty-safe no-ops when this Timer is empty.
    void set_period(std::uint32_t period_ms) noexcept;
    void pause() noexcept;
    void resume() noexcept;

    // borrows: valid only while this Timer owns a live lv_timer.
    [[nodiscard]] lv_timer_t* borrow_raw() const noexcept { return timer_; }
    [[nodiscard]] bool        empty()      const noexcept { return timer_ == nullptr; }

private:
    // Adopt an already-created lv_timer and install the back-pointer.
    // `timer` may be nullptr (yields an empty Timer). `on_tick` is moved in.
    Timer(lv_timer_t* timer, std::function<void()> on_tick) noexcept;

    // lv_timer_cb_t trampoline: recovers the owning Timer via the timer's
    // user_data and calls the stored function. Must not throw.
    static void on_tick_(lv_timer_t* timer);

    // owns: the LVGL timer. Nulled by the move-ctor on the source.
    lv_timer_t* timer_ = nullptr;
    // owns: the user callback invoked by on_tick_. Lives as long as the Timer.
    std::function<void()> on_tick_fn_;
};

// Value builder over lv_anim_t. Configure with the setters, then start().
//
// Ownership DELTA (§5.2): lv_anim_start COPIES the descriptor, so the running
// animation is owned by LVGL's animation registry and identified by
// (var, exec_cb). This Animation is a lightweight, copyable builder — it does
// NOT own, track, or stop the running instance. Use lv_anim_delete(var,
// exec_cb) to stop a running animation out-of-band.
class Animation {
public:
    // Returns: an Animation with lv_anim defaults applied (lv_anim_init).
    Animation() noexcept;

    // var: borrows; the animated target. LVGL passes it to exec_cb and uses
    // it (with exec_cb) as the running-animation key. Must outlive the run.
    Animation& set_var(void* var) noexcept;
    Animation& set_values(std::int32_t start, std::int32_t end) noexcept;
    // Animation duration in milliseconds (lv_anim_set_duration; LVGL <= 9.0
    // spelled this lv_anim_set_time).
    Animation& set_time(std::uint32_t duration_ms) noexcept;
    // exec_cb: borrows; void(void* var, int32_t value). Stored in the
    // descriptor and called each tick. Must outlive the run.
    Animation& set_exec_cb(lv_anim_exec_xcb_t exec_cb) noexcept;
    // path_cb: borrows; the easing curve (e.g. lv_anim_path_ease_in_out).
    // CORE-05 Easing maps onto these. Must outlive the run.
    Animation& set_path_cb(lv_anim_path_cb_t path_cb) noexcept;

    // Hand the configured descriptor to LVGL. LVGL copies it; the running
    // animation is thereafter LVGL-owned (see class DELTA).
    void start() noexcept;

    // borrows: the in-progress descriptor (pre-start configuration).
    [[nodiscard]] lv_anim_t* borrow_raw() noexcept { return &anim_; }

private:
    // The descriptor being built. A value, not an owner of any running anim.
    lv_anim_t anim_{};
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_TIMER_HPP
