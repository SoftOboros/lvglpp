// container.hpp — passive grouping widget with background styling.
//
// PARITY: rlvgl/widgets/src/container.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/core/lv_obj.h — informative; Container does not
//         shadow lv_obj.
// DELTA:  none. `style` is a public member (parity with rlvgl's
//         `pub style` field); draw delegates to core::draw_widget_bg,
//         events are ignored, exactly as container.rs.
//
// docs/disco-demo/01-container-widget.md (DEMO-01).

#ifndef LVGLPP_WIDGETS_LEGACY_CONTAINER_HPP
#define LVGLPP_WIDGETS_LEGACY_CONTAINER_HPP

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets::legacy {

// Empty widget used to group child widgets (in the WidgetNode tree) and
// provide background/border styling. It owns nothing and observes
// nothing — its children are owned by the enclosing WidgetNode, never by
// the Container (DEMO-00 §5 O-1).
class Container final : public ::lvglpp::core::Widget {
public:
    explicit Container(::lvglpp::core::Rect bounds) noexcept
        : bounds_{bounds} {}

    // Public — application code styles directly (parity with rlvgl
    // `pub style`).
    ::lvglpp::core::Style style{};

    // ---- Widget overrides --------------------------------------------------

    [[nodiscard]] ::lvglpp::core::Rect bounds() const override {
        return bounds_;
    }

    void draw(::lvglpp::core::Renderer& renderer) const override;

    // Containers are passive and never consume input (parity:
    // container.rs `handle_event` -> false).
    [[nodiscard]] bool handle_event(
        const ::lvglpp::core::Event& /*event*/) override {
        return false;
    }

private:
    ::lvglpp::core::Rect bounds_;
};

}  // namespace lvglpp::widgets::legacy

#endif  // LVGLPP_WIDGETS_LEGACY_CONTAINER_HPP
