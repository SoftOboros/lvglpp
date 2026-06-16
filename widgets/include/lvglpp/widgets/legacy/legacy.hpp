// legacy.hpp — umbrella for the hand-rolled (pre-LVGLPP-WRAP) widgets.
//
// PARITY: rlvgl/widgets/src/lib.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A — these widgets sit on the hand-rolled core::Widget/Renderer
//         layer and do NOT wrap lv_*.
// DELTA:  Moved to lvglpp::widgets::legacy by LVGLPP-WRAP-01 so the lv_obj-
//         backed widgets can take the canonical lvglpp::widgets names while
//         the hand-rolled stack (these + core::WidgetNode + the WidgetNode
//         playit Dispatcher) keeps compiling. Retired by LVGLPP-WRAP-0N once
//         nothing references it.

#ifndef LVGLPP_WIDGETS_LEGACY_LEGACY_HPP
#define LVGLPP_WIDGETS_LEGACY_LEGACY_HPP

#include "lvglpp/widgets/legacy/button.hpp"
#include "lvglpp/widgets/legacy/checkbox.hpp"
#include "lvglpp/widgets/legacy/container.hpp"
#include "lvglpp/widgets/legacy/image.hpp"
#include "lvglpp/widgets/legacy/label.hpp"
#include "lvglpp/widgets/legacy/list.hpp"
#include "lvglpp/widgets/legacy/slider.hpp"
#include "lvglpp/widgets/legacy/switch.hpp"

#endif  // LVGLPP_WIDGETS_LEGACY_LEGACY_HPP
