// button.cpp — Button::handle_event.
//
// PARITY: rlvgl/widgets/src/button.rs:78 — exact two-line semantics:
//         consume PressRelease inside bounds (firing on_click) and
//         return true; ignore everything else and return false.
//
// docs/widgets-button/00-button.md §5.3 freezes this body under
// Standards Action.

#include "lvglpp/widgets/legacy/button.hpp"

#include <variant>

namespace lvglpp::widgets::legacy {

bool Button::handle_event(const ::lvglpp::core::Event& event) {
    if (const auto* pr = std::get_if<::lvglpp::core::event::PressRelease>(&event)) {
        if (inside_bounds(pr->x, pr->y)) {
            if (on_click_) {
                on_click_(*this);
            }
            return true;
        }
    }
    return false;
}

}  // namespace lvglpp::widgets::legacy
