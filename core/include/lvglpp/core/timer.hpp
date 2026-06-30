// timer.hpp - LVGL-backed timers and animations.
//
// PARITY: rlvgl/docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_timer.h and lvgl/src/misc/lv_anim.h.
// DELTA:  lvglpp delegates timer scheduling and animation execution to LVGL
//         instead of porting rlvgl's Timers registry or ObjectAnims walker.

#ifndef LVGLPP_CORE_TIMER_HPP
#define LVGLPP_CORE_TIMER_HPP

#include "lvglpp/core/object.hpp"

#include <cstdint>

namespace lvglpp {

class TimerView {
public:
    // Args:
    //   raw: observes LVGL timer. The pointed-to timer is owned by LVGL
    //        or an LvTimer owner and must outlive this view.
    explicit TimerView(lv_timer_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_timer_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool        empty() const noexcept { return raw_ == nullptr; }

    // Args:
    //   callback: external function pointer; LVGL stores and calls it.
    void set_callback(lv_timer_cb_t callback) const noexcept;
    void set_period(std::uint32_t period_ms) const noexcept;
    void ready() const noexcept;
    void reset() const noexcept;
    void pause() const noexcept;
    void resume() const noexcept;
    void set_repeat_count(std::int32_t repeat_count) const noexcept;
    void set_auto_delete(bool auto_delete) const noexcept;

    // Args:
    //   user_data: external/observes; caller guarantees it outlives LVGL
    //              timer callbacks that read it.
    void set_user_data(void* user_data) const noexcept;

    [[nodiscard]] bool  is_paused() const noexcept;
    [[nodiscard]] void* user_data() const noexcept;

private:
    // observes: owned by LVGL or an LvTimer; never deleted by this view.
    lv_timer_t* raw_ = nullptr;
};

// Move-only RAII owner for an LVGL timer.
//
// Ownership: owns raw_ while non-null. Destruction calls lv_timer_delete().
// release() and release_with_auto_delete() transfer lifecycle authority to
// the caller or LVGL and disarm deletion.
class LvTimer {
public:
    LvTimer() noexcept = default;

    // Args:
    //   callback: external function pointer stored by LVGL.
    //   user_data: external/observes; caller guarantees callback lifetime.
    [[nodiscard]] static LvTimer make(lv_timer_cb_t callback,
                                      std::uint32_t period_ms,
                                      void* user_data = nullptr) noexcept;
    [[nodiscard]] static LvTimer make_basic() noexcept;

    LvTimer(const LvTimer&)            = delete;
    LvTimer& operator=(const LvTimer&) = delete;

    LvTimer(LvTimer&& other) noexcept;
    LvTimer& operator=(LvTimer&& other) noexcept;

    ~LvTimer();

    [[nodiscard]] TimerView   borrow() const noexcept;
    [[nodiscard]] lv_timer_t* borrow_raw() const noexcept;
    [[nodiscard]] bool        empty() const noexcept;

    // Returns: owns raw LVGL timer handle; caller is responsible for
    // deleting or otherwise transferring lifecycle authority.
    [[nodiscard]] lv_timer_t* release() noexcept;

    // Enables LVGL auto-delete and releases RAII ownership.
    // Returns: external raw handle; LVGL may delete it once repeat_count
    // reaches zero, so the caller must not keep long-lived observations.
    [[nodiscard]] lv_timer_t* release_with_auto_delete() noexcept;

    void reset() noexcept;

    void set_callback(lv_timer_cb_t callback) noexcept;
    void set_period(std::uint32_t period_ms) noexcept;
    void ready() noexcept;
    void reset_period() noexcept;
    void pause() noexcept;
    void resume() noexcept;
    void set_repeat_count(std::int32_t repeat_count) noexcept;

    // Owned timers keep LVGL auto-delete disabled. Passing true is a no-op;
    // use release_with_auto_delete() for explicit LVGL ownership transfer.
    void set_auto_delete(bool auto_delete) noexcept;
    void set_user_data(void* user_data) noexcept;

    [[nodiscard]] bool  is_paused() const noexcept;
    [[nodiscard]] void* user_data() const noexcept;

private:
    explicit LvTimer(lv_timer_t* raw) noexcept;

    // owns: deleted with lv_timer_delete() when non-null.
    lv_timer_t* raw_ = nullptr;
};

[[nodiscard]] TimerView view_timer(lv_timer_t* raw) noexcept;
[[nodiscard]] TimerView borrow_timer(const LvTimer& timer) noexcept;

[[nodiscard]] std::uint32_t run_timers() noexcept;
[[nodiscard]] std::uint32_t time_until_next_timer() noexcept;
[[nodiscard]] TimerView next_timer(TimerView previous = TimerView{nullptr}) noexcept;

class AnimationView;

enum class AnimationCallbackFamily : std::uint8_t {
    Exec,
    Custom,
};

class AnimationKey {
public:
    [[nodiscard]] static AnimationKey exec(void* var,
                                           lv_anim_exec_xcb_t callback) noexcept;
    [[nodiscard]] static AnimationKey custom(void* var,
                                             lv_anim_custom_exec_cb_t callback) noexcept;
    [[nodiscard]] static AnimationKey all_for_var(void* var) noexcept;

    [[nodiscard]] void*                   var() const noexcept { return var_; }
    [[nodiscard]] AnimationCallbackFamily family() const noexcept { return family_; }
    [[nodiscard]] lv_anim_exec_xcb_t      exec_callback() const noexcept;
    [[nodiscard]] lv_anim_custom_exec_cb_t custom_callback() const noexcept;

    [[nodiscard]] AnimationView find() const noexcept;
    [[nodiscard]] bool          cancel() const noexcept;

private:
    AnimationKey(void* var,
                 AnimationCallbackFamily family,
                 lv_anim_exec_xcb_t exec_callback,
                 lv_anim_custom_exec_cb_t custom_callback) noexcept;

    // external: animation target variable; owned by caller/LVGL object tree.
    void* var_ = nullptr;
    AnimationCallbackFamily family_ = AnimationCallbackFamily::Exec;
    // external: function pointer stored by LVGL; not owned.
    lv_anim_exec_xcb_t exec_callback_ = nullptr;
    // external: function pointer stored by LVGL; not owned.
    lv_anim_custom_exec_cb_t custom_callback_ = nullptr;
};

class AnimationView {
public:
    // Args:
    //   raw: observes a running LVGL animation. LVGL owns and may delete it.
    explicit AnimationView(lv_anim_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_anim_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool       empty() const noexcept { return raw_ == nullptr; }

    void pause() const noexcept;
    void pause_for(std::uint32_t ms) const noexcept;
    void resume() const noexcept;

    [[nodiscard]] bool          is_paused() const noexcept;
    [[nodiscard]] std::uint32_t delay() const noexcept;
    [[nodiscard]] std::uint32_t playtime() const noexcept;
    [[nodiscard]] std::uint32_t duration() const noexcept;
    [[nodiscard]] std::uint32_t repeat_count() const noexcept;
    [[nodiscard]] void*         user_data() const noexcept;
    [[nodiscard]] AnimationKey  key() const noexcept;

private:
    // observes: owned by LVGL's animation list; never freed by this view.
    lv_anim_t* raw_ = nullptr;
};

class AnimationTemplate {
public:
    AnimationTemplate() noexcept;

    [[nodiscard]] lv_anim_t*       borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_anim_t* borrow_raw() const noexcept { return &raw_; }

    // Args:
    //   var: external target; must outlive the running animation or be
    //        cancelled before teardown.
    AnimationTemplate& set_var(void* var) noexcept;
    AnimationTemplate& set_target(ObjectView object) noexcept;

    // Args:
    //   callback: external function pointer; LVGL stores and calls it.
    AnimationTemplate& set_exec_callback(lv_anim_exec_xcb_t callback) noexcept;
    AnimationTemplate& set_custom_exec_callback(
        lv_anim_custom_exec_cb_t callback) noexcept;
    AnimationTemplate& set_values(std::int32_t start, std::int32_t end) noexcept;
    AnimationTemplate& set_duration(std::uint32_t duration_ms) noexcept;
    AnimationTemplate& set_delay(std::uint32_t delay_ms) noexcept;
    AnimationTemplate& set_path_callback(lv_anim_path_cb_t callback) noexcept;
    AnimationTemplate& set_start_callback(lv_anim_start_cb_t callback) noexcept;
    AnimationTemplate& set_completed_callback(
        lv_anim_completed_cb_t callback) noexcept;
    AnimationTemplate& set_deleted_callback(lv_anim_deleted_cb_t callback) noexcept;
    AnimationTemplate& set_get_value_callback(
        lv_anim_get_value_cb_t callback) noexcept;
    AnimationTemplate& set_reverse_duration(std::uint32_t duration_ms) noexcept;
    AnimationTemplate& set_reverse_delay(std::uint32_t delay_ms) noexcept;
    AnimationTemplate& set_repeat_count(std::uint32_t count) noexcept;
    AnimationTemplate& set_repeat_delay(std::uint32_t delay_ms) noexcept;
    AnimationTemplate& set_early_apply(bool enabled) noexcept;

    // Args:
    //   user_data: external/observes; caller guarantees callback lifetime.
    AnimationTemplate& set_user_data(void* user_data) noexcept;
    AnimationTemplate& set_bezier3_param(std::int16_t x1,
                                         std::int16_t y1,
                                         std::int16_t x2,
                                         std::int16_t y2) noexcept;

    [[nodiscard]] AnimationKey key() const noexcept;
    [[nodiscard]] AnimationView start() const noexcept;
    [[nodiscard]] bool cancel_matching() const noexcept;
    [[nodiscard]] AnimationView find_matching() const noexcept;

private:
    // owns: local animation configuration value copied by lv_anim_start().
    lv_anim_t raw_{};
};

[[nodiscard]] std::uint16_t running_animation_count() noexcept;
void delete_all_animations() noexcept;

}  // namespace lvglpp

#endif  // LVGLPP_CORE_TIMER_HPP
