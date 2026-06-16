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

// lv_obj-backed widgets (lvglpp::widgets, LVGLPP-WRAP-01..06) — added as they
// land. (none yet — the scaffold commit only re-homes the legacy stack.)

// Hand-rolled widgets (lvglpp::widgets::legacy) — retired by WRAP-0N:
#include "lvglpp/widgets/legacy/legacy.hpp"

#endif  // LVGLPP_WIDGETS_WIDGETS_HPP
