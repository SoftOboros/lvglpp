// widgets.hpp — module umbrella for lvglpp::widgets.
//
// PARITY: rlvgl/widgets/src/lib.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/widgets/ (the upstream widget tree).
//
// LVGLPP-WRAP migration in progress: the canonical lvglpp::widgets names are
// being re-homed onto lv_obj-backed wrappers (core::Object subclasses). The
// hand-rolled implementations live under lvglpp::widgets::legacy (included
// below) until LVGLPP-WRAP-0N retires them. New lv_obj-backed widget headers
// are added here as each WRAP-0x sub-phase lands.

#ifndef LVGLPP_WIDGETS_WIDGETS_HPP
#define LVGLPP_WIDGETS_WIDGETS_HPP

// lv_obj-backed widgets (lvglpp::widgets, LVGLPP-WRAP-01..06):
#include "lvglpp/widgets/button.hpp"     // WRAP-02
#include "lvglpp/widgets/checkbox.hpp"   // WRAP-03
#include "lvglpp/widgets/container.hpp"  // WRAP-05
#include "lvglpp/widgets/image.hpp"      // WRAP-06
#include "lvglpp/widgets/label.hpp"      // WRAP-01
#include "lvglpp/widgets/list.hpp"       // WRAP-05
#include "lvglpp/widgets/slider.hpp"     // WRAP-04
#include "lvglpp/widgets/switch.hpp"     // WRAP-03

// Hand-rolled widgets (lvglpp::widgets::legacy) — retired by WRAP-0N:
#include "lvglpp/widgets/legacy/legacy.hpp"

#endif  // LVGLPP_WIDGETS_WIDGETS_HPP
