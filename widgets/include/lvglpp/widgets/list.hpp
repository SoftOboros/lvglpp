// list.hpp — vertical list widget, lv_obj-backed (LVGLPP-WRAP-05).
//
// PARITY: rlvgl/widgets/src/list.rs (v0.2.4 @ 343f596) — a scrollable list of
//         text/button rows.
// LVGL:   lvgl/src/widgets/list/lv_list.h (create / add_text / add_button /
//         get_button_text).
// DELTA:  Supersedes lvglpp::widgets::legacy::List. core::Object subclass over
//         lv_list. Rows are LVGL-owned children; add_* return a non-owning
//         ObjectView of the created row (its lifetime is the list's).

#ifndef LVGLPP_WIDGETS_LIST_HPP
#define LVGLPP_WIDGETS_LIST_HPP

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// List — a vertically scrolling list of text labels and clickable buttons.
//
// Ownership: owns its lv_list lv_obj (via core::Object). Move-only. Rows added
// via add_* are owned by the list's object tree, not by the caller.
class List : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<List, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static List make(::lvglpp::ObjectView parent);

    List(List&&) noexcept = default;

    // Append a non-interactive text row (lv_list_add_text).
    //   text: borrows for the call only (copied).
    // Returns: observes the new row label (owned by the list tree); empty
    //          ObjectView when this List is empty.
    [[nodiscard]] ::lvglpp::ObjectView add_text(const char* text) noexcept;

    // Append a clickable button row with `text` (lv_list_add_button, no icon).
    //   text: borrows for the call only (copied).
    // Returns: observes the new row button (owned by the list tree); empty
    //          ObjectView when this List is empty.
    [[nodiscard]] ::lvglpp::ObjectView add_button(const char* text) noexcept;

    // The text of a button row created by add_button (lv_list_get_button_text).
    //   row: borrows; a button row from this list. Returns "" if not resolvable.
    [[nodiscard]] const char* button_text(::lvglpp::ObjectView row) const noexcept;

private:
    explicit List(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_LIST_HPP
