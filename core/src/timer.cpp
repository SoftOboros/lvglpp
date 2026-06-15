// timer.cpp — Timer + Animation implementation (LPAR-06).
//
// PARITY: rlvgl/core/src/timer.rs + core::anim (v0.2.4 @ 343f596).
// LVGL:   lvgl/src/misc/lv_timer.c, lvgl/src/misc/lv_anim.c.
// DELTA:  see timer.hpp — Timer is move-only + rebinds user_data on move;
//         Animation is a builder, not an RAII owner of the running anim.

#include "lvglpp/core/timer.hpp"

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::core {

// --- Timer ---

Timer::Timer(lv_timer_t* timer, std::function<void()> on_tick) noexcept
    : timer_{timer}, on_tick_fn_{std::move(on_tick)} {
    if (timer_ != nullptr) {
        // SAFETY: user_data is owned by this wrapper. It holds a back-pointer
        //   to this Timer, kept current across moves (see move ctor). Valid
        //   until lv_timer_delete in the destructor.
        lv_timer_set_user_data(timer_, this);
    }
}

Timer::Timer(Timer&& other) noexcept
    : timer_{other.timer_}, on_tick_fn_{std::move(other.on_tick_fn_)} {
    other.timer_ = nullptr;
    if (timer_ != nullptr) {
        // Rebind the back-pointer to this (new) location. The trampoline
        // reads user_data dynamically, so it picks up the moved-to Timer.
        lv_timer_set_user_data(timer_, this);
    }
}

Timer::~Timer() {
    if (timer_ != nullptr) {
        lv_timer_delete(timer_);
    }
}

void Timer::on_tick_(lv_timer_t* timer) {
    if (timer == nullptr) {
        return;
    }
    // SAFETY: user_data is the owning Timer's `this`, installed at construction
    //   and on every move. Borrowed for the duration of this call only.
    auto* self = static_cast<Timer*>(lv_timer_get_user_data(timer));
    if (self != nullptr && self->on_tick_fn_) {
        self->on_tick_fn_();
    }
}

lvglpp::expected<Timer, TimerError>
Timer::try_make(std::function<void()> on_tick, std::uint32_t period_ms) noexcept {
    // user_data is set to `this` in the adopting constructor, not here:
    // lv_timer_create needs a non-null user_data only if the trampoline runs
    // before adoption, which cannot happen (no tick is processed mid-create).
    lv_timer_t* timer = lv_timer_create(&Timer::on_tick_, period_ms, nullptr);
    if (timer == nullptr) {
        return lvglpp::unexpected<TimerError>{TimerError::CreateFailed};
    }
    return Timer{timer, std::move(on_tick)};
}

Timer Timer::make(std::function<void()> on_tick, std::uint32_t period_ms) {
    auto result = try_make(std::move(on_tick), period_ms);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::Timer::make: lv_timer_create failed");
#endif
    }
    return std::move(result.value());
}

void Timer::set_period(std::uint32_t period_ms) noexcept {
    if (timer_ != nullptr) {
        lv_timer_set_period(timer_, period_ms);
    }
}

void Timer::pause() noexcept {
    if (timer_ != nullptr) {
        lv_timer_pause(timer_);
    }
}

void Timer::resume() noexcept {
    if (timer_ != nullptr) {
        lv_timer_resume(timer_);
    }
}

// --- Animation ---

Animation::Animation() noexcept {
    lv_anim_init(&anim_);
}

Animation& Animation::set_var(void* var) noexcept {
    lv_anim_set_var(&anim_, var);
    return *this;
}

Animation& Animation::set_values(std::int32_t start, std::int32_t end) noexcept {
    lv_anim_set_values(&anim_, start, end);
    return *this;
}

Animation& Animation::set_time(std::uint32_t duration_ms) noexcept {
    lv_anim_set_duration(&anim_, duration_ms);
    return *this;
}

Animation& Animation::set_exec_cb(lv_anim_exec_xcb_t exec_cb) noexcept {
    lv_anim_set_exec_cb(&anim_, exec_cb);
    return *this;
}

Animation& Animation::set_path_cb(lv_anim_path_cb_t path_cb) noexcept {
    lv_anim_set_path_cb(&anim_, path_cb);
    return *this;
}

void Animation::start() noexcept {
    // lv_anim_start COPIES anim_ into the LVGL registry; the returned pointer
    // is the registry-owned instance (keyed by var+exec_cb). We intentionally
    // discard it — Animation does not own the running animation (see DELTA).
    static_cast<void>(lv_anim_start(&anim_));
}

}  // namespace lvglpp::core
