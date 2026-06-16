// container.hpp — plain panel/container widget, lv_obj-backed (LVGLPP-WRAP-05).
//
// PARITY: rlvgl/widgets/src/container.rs (v0.2.4 @ 343f596) — a styled panel
//         that holds children.
// LVGL:   lvgl/src/core/lv_obj.h (lv_obj_create) — a base object IS LVGL's
//         container/panel; there is no separate lv_container widget.
// DELTA:  Supersedes lvglpp::widgets::legacy::Container. A named widget over
//         lv_obj_create; it adds no API beyond core::Object (layout/scroll/
//         style/flags come from the inherited surface), giving callers a
//         self-documenting panel type.

#ifndef LVGLPP_WIDGETS_CONTAINER_HPP
#define LVGLPP_WIDGETS_CONTAINER_HPP

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Container — a plain object that holds and lays out children.
//
// Ownership: owns its lv_obj (via core::Object). Move-only. All behavior is
// the inherited core::Object surface (set_size, set_flex_flow, add_style, …).
class Container : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<Container, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static Container make(::lvglpp::ObjectView parent);

    Container(Container&&) noexcept = default;

private:
    explicit Container(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_CONTAINER_HPP
