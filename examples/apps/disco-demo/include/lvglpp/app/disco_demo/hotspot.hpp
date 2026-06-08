// hotspot.hpp — invisible tap-target composite widget for automation.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/hotspot.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  single-owner core::Widget; Box<dyn FnMut>/Box<dyn Fn> become
//         std::function (DEMO-00 §5 — captures observe the controller when
//         bound in DEMO-06). with_visibility returns *this& for fluent use.
//
// docs/disco-demo/05-composite-widgets.md (DEMO-05) §4.

#ifndef LVGLPP_APP_DISCO_DEMO_HOTSPOT_HPP
#define LVGLPP_APP_DISCO_DEMO_HOTSPOT_HPP

#include <functional>
#include <utility>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::app::disco_demo {

// Transparent widget that triggers an action when tapped. Always visible
// unless a visibility predicate is attached via with_visibility.
class ActionHotspot final : public core::Widget {
public:
    // Args:
    //   bounds: hit rectangle.
    //   on_tap: owned activation callback (observes the controller when
    //           bound; lifetime documented at the bind site, DEMO-06).
    ActionHotspot(core::Rect bounds, std::function<void()> on_tap)
        : bounds_{bounds},
          on_tap_{std::move(on_tap)},
          is_visible_{[] { return true; }} {}

    // Attach a dynamic visibility predicate (observes a const Wing* when
    // bound). Fluent: returns *this for chaining at the construction site.
    ActionHotspot& with_visibility(std::function<bool()> is_visible) {
        is_visible_ = std::move(is_visible);
        return *this;
    }

    // ---- Widget overrides --------------------------------------------------
    [[nodiscard]] core::Rect bounds() const override {
        return is_visible_ && is_visible_() ? bounds_ : core::Rect{0, 0, 0, 0};
    }
    void draw(core::Renderer& /*renderer*/) const override {}
    [[nodiscard]] bool handle_event(const core::Event& event) override;

private:
    core::Rect            bounds_{};
    std::function<void()> on_tap_{};
    std::function<bool()> is_visible_{};
};

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_HOTSPOT_HPP
