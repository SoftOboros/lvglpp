// core.hpp — module umbrella for lvglpp::core.
//
// PARITY: rlvgl/core/src/lib.rs (v0.2.0 @ d99f793).
// LVGL:   lvgl/lvgl.h (the upstream public C header).
//
// This umbrella re-exports the per-concept headers under
// `include/lvglpp/core/`. It is the only header an external consumer
// should need to include for the core surface.

#ifndef LVGLPP_CORE_CORE_HPP
#define LVGLPP_CORE_CORE_HPP

#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/display.hpp"
#include "lvglpp/core/event.hpp"
#include "lvglpp/core/input.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/rle.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/scroll.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/style_lvgl.hpp"
#include "lvglpp/core/timer.hpp"
#include "lvglpp/core/widget.hpp"
#include "lvglpp/core/widget_node.hpp"
//   #include "lvglpp/core/renderer.hpp"   // PARITY: rlvgl/core/src/renderer.rs
//   #include "lvglpp/core/style.hpp"      // PARITY: rlvgl/core/src/style.rs
//   #include "lvglpp/core/theme.hpp"      // PARITY: rlvgl/core/src/theme.rs
//   #include "lvglpp/core/animation.hpp"  // PARITY: rlvgl/core/src/animation.rs
//   #include "lvglpp/core/font.hpp"       // PARITY: rlvgl/core/src/{bitmap,packed}_font.rs
//   #include "lvglpp/core/draw.hpp"       // PARITY: rlvgl/core/src/draw.rs

#endif  // LVGLPP_CORE_CORE_HPP
