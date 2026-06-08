// hotspot.cpp — ActionHotspot::handle_event.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/hotspot.rs:59 (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  std::function callbacks; PressRelease fires the activation closure.

#include "lvglpp/app/disco_demo/hotspot.hpp"

#include <variant>

namespace lvglpp::app::disco_demo {

bool ActionHotspot::handle_event(const core::Event& event) {
    if (is_visible_ && !is_visible_()) return false;
    if (std::holds_alternative<core::event::PressRelease>(event)) {
        if (on_tap_) on_tap_();
        return true;
    }
    return false;
}

}  // namespace lvglpp::app::disco_demo
