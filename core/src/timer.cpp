// timer.cpp - LVGL-backed timer and animation wrapper implementation.
//
// PARITY: rlvgl/docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_timer.c and lvgl/src/misc/lv_anim.c.
// DELTA:  delegates scheduling and animation runtime to LVGL.

#include "lvglpp/core/timer.hpp"

namespace lvglpp {

namespace {

void disable_lvgl_auto_delete(lv_timer_t* raw) noexcept {
    if (raw != nullptr) {
        lv_timer_set_auto_delete(raw, false);
    }
}

}  // namespace

void TimerView::set_callback(lv_timer_cb_t callback) const noexcept {
    if (raw_ != nullptr) {
        lv_timer_set_cb(raw_, callback);
    }
}

void TimerView::set_period(std::uint32_t period_ms) const noexcept {
    if (raw_ != nullptr) {
        lv_timer_set_period(raw_, period_ms);
    }
}

void TimerView::ready() const noexcept {
    if (raw_ != nullptr) {
        lv_timer_ready(raw_);
    }
}

void TimerView::reset() const noexcept {
    if (raw_ != nullptr) {
        lv_timer_reset(raw_);
    }
}

void TimerView::pause() const noexcept {
    if (raw_ != nullptr) {
        lv_timer_pause(raw_);
    }
}

void TimerView::resume() const noexcept {
    if (raw_ != nullptr) {
        lv_timer_resume(raw_);
    }
}

void TimerView::set_repeat_count(std::int32_t repeat_count) const noexcept {
    if (raw_ != nullptr) {
        lv_timer_set_repeat_count(raw_, repeat_count);
    }
}

void TimerView::set_auto_delete(bool auto_delete) const noexcept {
    if (raw_ != nullptr) {
        lv_timer_set_auto_delete(raw_, auto_delete);
    }
}

void TimerView::set_user_data(void* user_data) const noexcept {
    if (raw_ != nullptr) {
        lv_timer_set_user_data(raw_, user_data);
    }
}

bool TimerView::is_paused() const noexcept {
    return raw_ != nullptr && lv_timer_get_paused(raw_);
}

void* TimerView::user_data() const noexcept {
    return raw_ == nullptr ? nullptr : lv_timer_get_user_data(raw_);
}

LvTimer::LvTimer(lv_timer_t* raw) noexcept : raw_{raw} {
    disable_lvgl_auto_delete(raw_);
}

LvTimer LvTimer::make(lv_timer_cb_t callback,
                      std::uint32_t period_ms,
                      void* user_data) noexcept {
    return LvTimer{lv_timer_create(callback, period_ms, user_data)};
}

LvTimer LvTimer::make_basic() noexcept {
    return LvTimer{lv_timer_create_basic()};
}

LvTimer::LvTimer(LvTimer&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvTimer& LvTimer::operator=(LvTimer&& other) noexcept {
    if (this != &other) {
        reset();
        raw_       = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvTimer::~LvTimer() {
    reset();
}

TimerView LvTimer::borrow() const noexcept {
    return TimerView{raw_};
}

lv_timer_t* LvTimer::borrow_raw() const noexcept {
    return raw_;
}

bool LvTimer::empty() const noexcept {
    return raw_ == nullptr;
}

lv_timer_t* LvTimer::release() noexcept {
    lv_timer_t* released = raw_;
    raw_                 = nullptr;
    return released;
}

lv_timer_t* LvTimer::release_with_auto_delete() noexcept {
    if (raw_ != nullptr) {
        lv_timer_set_auto_delete(raw_, true);
    }
    return release();
}

void LvTimer::reset() noexcept {
    if (raw_ != nullptr) {
        lv_timer_delete(raw_);
        raw_ = nullptr;
    }
}

void LvTimer::set_callback(lv_timer_cb_t callback) noexcept {
    borrow().set_callback(callback);
}

void LvTimer::set_period(std::uint32_t period_ms) noexcept {
    borrow().set_period(period_ms);
}

void LvTimer::ready() noexcept {
    borrow().ready();
}

void LvTimer::reset_period() noexcept {
    borrow().reset();
}

void LvTimer::pause() noexcept {
    borrow().pause();
}

void LvTimer::resume() noexcept {
    borrow().resume();
}

void LvTimer::set_repeat_count(std::int32_t repeat_count) noexcept {
    borrow().set_repeat_count(repeat_count);
}

void LvTimer::set_auto_delete(bool auto_delete) noexcept {
    if (raw_ != nullptr && !auto_delete) {
        lv_timer_set_auto_delete(raw_, false);
    }
}

void LvTimer::set_user_data(void* user_data) noexcept {
    borrow().set_user_data(user_data);
}

bool LvTimer::is_paused() const noexcept {
    return borrow().is_paused();
}

void* LvTimer::user_data() const noexcept {
    return borrow().user_data();
}

TimerView view_timer(lv_timer_t* raw) noexcept {
    return TimerView{raw};
}

TimerView borrow_timer(const LvTimer& timer) noexcept {
    return timer.borrow();
}

std::uint32_t run_timers() noexcept {
    return lv_timer_handler();
}

std::uint32_t time_until_next_timer() noexcept {
    return lv_timer_get_time_until_next();
}

TimerView next_timer(TimerView previous) noexcept {
    return TimerView{lv_timer_get_next(previous.borrow_raw())};
}

AnimationKey AnimationKey::exec(void* var,
                                lv_anim_exec_xcb_t callback) noexcept {
    return AnimationKey{var, AnimationCallbackFamily::Exec, callback, nullptr};
}

AnimationKey AnimationKey::custom(void* var,
                                  lv_anim_custom_exec_cb_t callback) noexcept {
    return AnimationKey{var, AnimationCallbackFamily::Custom, nullptr, callback};
}

AnimationKey AnimationKey::all_for_var(void* var) noexcept {
    return AnimationKey{var, AnimationCallbackFamily::Exec, nullptr, nullptr};
}

lv_anim_exec_xcb_t AnimationKey::exec_callback() const noexcept {
    return exec_callback_;
}

lv_anim_custom_exec_cb_t AnimationKey::custom_callback() const noexcept {
    return custom_callback_;
}

AnimationView AnimationKey::find() const noexcept {
    if (family_ == AnimationCallbackFamily::Custom) {
        lv_anim_t* candidate = lv_anim_get(var_, nullptr);
        if (candidate != nullptr && candidate->custom_exec_cb == custom_callback_) {
            return AnimationView{candidate};
        }
        return AnimationView{nullptr};
    }
    return AnimationView{lv_anim_get(var_, exec_callback_)};
}

bool AnimationKey::cancel() const noexcept {
    if (family_ == AnimationCallbackFamily::Custom) {
        AnimationView candidate = find();
        if (candidate.empty()) {
            return false;
        }
        return lv_anim_delete(var_, nullptr);
    }
    return lv_anim_delete(var_, exec_callback_);
}

AnimationKey::AnimationKey(void* var,
                           AnimationCallbackFamily family,
                           lv_anim_exec_xcb_t exec_callback,
                           lv_anim_custom_exec_cb_t custom_callback) noexcept
    : var_{var},
      family_{family},
      exec_callback_{exec_callback},
      custom_callback_{custom_callback} {}

void AnimationView::pause() const noexcept {
    if (raw_ != nullptr) {
        lv_anim_pause(raw_);
    }
}

void AnimationView::pause_for(std::uint32_t ms) const noexcept {
    if (raw_ != nullptr) {
        lv_anim_pause_for(raw_, ms);
    }
}

void AnimationView::resume() const noexcept {
    if (raw_ != nullptr) {
        lv_anim_resume(raw_);
    }
}

bool AnimationView::is_paused() const noexcept {
    return raw_ != nullptr && lv_anim_is_paused(raw_);
}

std::uint32_t AnimationView::delay() const noexcept {
    return raw_ == nullptr ? 0U : lv_anim_get_delay(raw_);
}

std::uint32_t AnimationView::playtime() const noexcept {
    return raw_ == nullptr ? 0U : lv_anim_get_playtime(raw_);
}

std::uint32_t AnimationView::duration() const noexcept {
    return raw_ == nullptr ? 0U : lv_anim_get_time(raw_);
}

std::uint32_t AnimationView::repeat_count() const noexcept {
    return raw_ == nullptr ? 0U : lv_anim_get_repeat_count(raw_);
}

void* AnimationView::user_data() const noexcept {
    return raw_ == nullptr ? nullptr : lv_anim_get_user_data(raw_);
}

AnimationKey AnimationView::key() const noexcept {
    if (raw_ == nullptr) {
        return AnimationKey::all_for_var(nullptr);
    }
    if (raw_->custom_exec_cb != nullptr) {
        return AnimationKey::custom(raw_->var, raw_->custom_exec_cb);
    }
    return AnimationKey::exec(raw_->var, raw_->exec_cb);
}

AnimationTemplate::AnimationTemplate() noexcept {
    lv_anim_init(&raw_);
}

AnimationTemplate& AnimationTemplate::set_var(void* var) noexcept {
    lv_anim_set_var(&raw_, var);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_target(ObjectView object) noexcept {
    lv_anim_set_var(&raw_, object.empty() ? nullptr : object.borrow_raw());
    return *this;
}

AnimationTemplate& AnimationTemplate::set_exec_callback(
    lv_anim_exec_xcb_t callback) noexcept {
    lv_anim_set_exec_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_custom_exec_callback(
    lv_anim_custom_exec_cb_t callback) noexcept {
    lv_anim_set_custom_exec_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_values(std::int32_t start,
                                                 std::int32_t end) noexcept {
    lv_anim_set_values(&raw_, start, end);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_duration(
    std::uint32_t duration_ms) noexcept {
    lv_anim_set_duration(&raw_, duration_ms);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_delay(std::uint32_t delay_ms) noexcept {
    lv_anim_set_delay(&raw_, delay_ms);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_path_callback(
    lv_anim_path_cb_t callback) noexcept {
    lv_anim_set_path_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_start_callback(
    lv_anim_start_cb_t callback) noexcept {
    lv_anim_set_start_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_completed_callback(
    lv_anim_completed_cb_t callback) noexcept {
    lv_anim_set_completed_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_deleted_callback(
    lv_anim_deleted_cb_t callback) noexcept {
    lv_anim_set_deleted_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_get_value_callback(
    lv_anim_get_value_cb_t callback) noexcept {
    lv_anim_set_get_value_cb(&raw_, callback);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_reverse_duration(
    std::uint32_t duration_ms) noexcept {
    lv_anim_set_reverse_duration(&raw_, duration_ms);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_reverse_delay(
    std::uint32_t delay_ms) noexcept {
    lv_anim_set_reverse_delay(&raw_, delay_ms);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_repeat_count(
    std::uint32_t count) noexcept {
    lv_anim_set_repeat_count(&raw_, count);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_repeat_delay(
    std::uint32_t delay_ms) noexcept {
    lv_anim_set_repeat_delay(&raw_, delay_ms);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_early_apply(bool enabled) noexcept {
    lv_anim_set_early_apply(&raw_, enabled);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_user_data(void* user_data) noexcept {
    lv_anim_set_user_data(&raw_, user_data);
    return *this;
}

AnimationTemplate& AnimationTemplate::set_bezier3_param(std::int16_t x1,
                                                        std::int16_t y1,
                                                        std::int16_t x2,
                                                        std::int16_t y2) noexcept {
    lv_anim_set_bezier3_param(&raw_, x1, y1, x2, y2);
    return *this;
}

AnimationKey AnimationTemplate::key() const noexcept {
    if (raw_.custom_exec_cb != nullptr) {
        return AnimationKey::custom(raw_.var, raw_.custom_exec_cb);
    }
    return AnimationKey::exec(raw_.var, raw_.exec_cb);
}

AnimationView AnimationTemplate::start() const noexcept {
    return AnimationView{lv_anim_start(&raw_)};
}

bool AnimationTemplate::cancel_matching() const noexcept {
    return key().cancel();
}

AnimationView AnimationTemplate::find_matching() const noexcept {
    return key().find();
}

std::uint16_t running_animation_count() noexcept {
    return lv_anim_count_running();
}

void delete_all_animations() noexcept {
    lv_anim_delete_all();
}

}  // namespace lvglpp
