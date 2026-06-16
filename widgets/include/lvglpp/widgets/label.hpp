// label.hpp — text label widget, lv_obj-backed (LVGLPP-WRAP-01).
//
// PARITY: rlvgl/widgets/src/label.rs (v0.2.4 @ 343f596) — the Label surface
//         (text, long-mode). The ownership story matches rlvgl (a Label owns
//         its widget node); the mechanism wraps lv_label instead of the
//         hand-rolled core::Widget/Renderer path.
// LVGL:   lvgl/src/widgets/label/lv_label.h (lv_label_create / set_text /
//         set_text_static / get_text / set_long_mode).
// DELTA:  Supersedes the hand-rolled lvglpp::widgets::legacy::Label
//         (docs/widgets-label/00-label.md, WID-01). Label is a core::Object
//         subclass: it inherits the RAII ownership, move-only semantics, the
//         LV_EVENT_DELETE delete-safety hook, flags/state/scroll/style/font/
//         event surface, and the user-data back-pointer convention. Widget
//         state lives in LVGL (lv_label), not in C++ members.

#ifndef LVGLPP_WIDGETS_LABEL_HPP
#define LVGLPP_WIDGETS_LABEL_HPP

#include <cstdint>

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Label — single-line / multi-line text display over lv_label.
//
// Ownership: owns its lv_label lv_obj (via core::Object). Move-only; the
// inherited delete-safety callback makes parent-driven deletion safe.
class Label : public ::lvglpp::core::Object {
public:
    // Mirror of lv_label_long_mode_t (lvgl/src/widgets/label/lv_label.h).
    // Standards Action to diverge from LVGL.
    enum class LongMode : std::uint8_t {
        Wrap           = LV_LABEL_LONG_MODE_WRAP,
        Dots           = LV_LABEL_LONG_MODE_DOTS,
        Scroll         = LV_LABEL_LONG_MODE_SCROLL,
        ScrollCircular = LV_LABEL_LONG_MODE_SCROLL_CIRCULAR,
        Clip           = LV_LABEL_LONG_MODE_CLIP,
    };

    // Create a label as a child of `parent` (lv_label_create).
    //   parent: borrows an lv_obj that outlives the call; must be non-empty.
    // Returns: owns Label on success; ObjectError on failure.
    [[nodiscard]] static lvglpp::expected<Label, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;

    // Throwing convenience over try_make: aborts under embedded posture,
    // throws std::runtime_error on host, if creation fails.
    [[nodiscard]] static Label make(::lvglpp::ObjectView parent);

    // Move-only; inherits core::Object's move (rebinds the user-data
    // back-pointer and moves event handlers).
    Label(Label&&) noexcept = default;

    // Replace the text; LVGL copies it into the label (lv_label_set_text).
    //   text: borrows for the call only (copied). No-op when empty.
    void set_text(const char* text) noexcept;

    // Point the label at caller-owned static text (lv_label_set_text_static);
    // LVGL stores the pointer, so `text` MUST outlive the label.
    //   text: borrows; must outlive this label (no copy). No-op when empty.
    void set_text_static(const char* text) noexcept;

    // The label's current text (lv_label_get_text). Returns "" when empty.
    //   Returns: borrows; owned by the lv_label, valid until the next set_*.
    [[nodiscard]] const char* text() const noexcept;

    // Overflow behavior for text wider than the label (lv_label_set_long_mode).
    void set_long_mode(LongMode mode) noexcept;

private:
    // Adopt an already-created lv_label (used by try_make). Installs the
    // core::Object user-data back-pointer + delete-safety callback.
    explicit Label(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_LABEL_HPP
