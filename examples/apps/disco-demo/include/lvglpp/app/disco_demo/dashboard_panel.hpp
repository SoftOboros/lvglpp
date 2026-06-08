// dashboard_panel.hpp — centered text-rich detail panel composite widget.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/dashboard_panel.rs
//         (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  single-owner core::Widget; title/caption/lines owned by value
//         (std::string / std::vector). Borrows the BitmapFont via an
//         observing pointer (DEMO-00 §5). Rounded bg/border use the core
//         radius==0 fallback. set_lines takes a std::span instead of a Rust
//         IntoIterator.
//
// docs/disco-demo/05-composite-widgets.md (DEMO-05) §4.

#ifndef LVGLPP_APP_DISCO_DEMO_DASHBOARD_PANEL_HPP
#define LVGLPP_APP_DISCO_DEMO_DASHBOARD_PANEL_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/font.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::app::disco_demo {

// A reusable text-rich panel for the shared 747-style demo. Starts hidden.
class DashboardPanel final : public core::Widget {
public:
    // Args:
    //   bounds:  panel outer rectangle.
    //   title:   header title text (owned by value).
    //   caption: caption text, wrapped to the inner width on construction.
    DashboardPanel(core::Rect bounds, std::string title, std::string caption);

    void set_title(std::string title);
    // Replace the caption text, wrapping it to the panel's inner width.
    void set_caption(std::string caption);
    // Replace the body lines, word-wrapping each to fit the panel width.
    void set_lines(std::span<const std::string> lines);
    void set_accent(core::Color color) noexcept { accent_ = color; }

    void show() noexcept { visible_ = true; }
    void hide() noexcept { visible_ = false; }
    [[nodiscard]] bool is_visible() const noexcept { return visible_; }

    // ---- Widget overrides --------------------------------------------------
    [[nodiscard]] core::Rect bounds() const override;
    void draw(core::Renderer& renderer) const override;
    [[nodiscard]] bool handle_event(const core::Event& event) override;

private:
    // Maximum character columns that fit inside the panel's inner width.
    [[nodiscard]] std::size_t text_cols() const noexcept;

    core::Rect               bounds_{};
    std::string              title_;
    std::string              caption_;  // pre-wrapped, '\n'-joined
    std::vector<std::string> lines_;
    core::Color              accent_{0x58, 0xB3, 0xF5, 0xFF};
    // observes: borrows the immortal static FONT_6X10; never freed here.
    const core::BitmapFont*  font_   = nullptr;
    bool                     visible_ = false;
};

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_DASHBOARD_PANEL_HPP
