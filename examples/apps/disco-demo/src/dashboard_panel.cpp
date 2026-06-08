// dashboard_panel.cpp — DashboardPanel impl + greedy word-wrap.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/dashboard_panel.rs
//         (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  rounded bg/border via the core radius==0 fallback; wrap_text is a
//         byte-wise port of the rlvgl char-wise wrapper (ASCII-only fonts).

#include "lvglpp/app/disco_demo/dashboard_panel.hpp"

#include <algorithm>
#include <string_view>
#include <utility>
#include <variant>

#include "lvglpp/app/disco_demo/detail/text_wrap.hpp"
#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/fonts/font_6x10.hpp"
#include "lvglpp/ui/draw_helpers.hpp"

namespace lvglpp::app::disco_demo {

namespace {

namespace lc = ::lvglpp::core;

using detail::join_newline;
using detail::wrap_text;

// FROZEN palette — mirror dashboard_panel.rs:17-22.
constexpr lc::Color  kPanelBg{22, 29, 41, 255};
constexpr lc::Color  kPanelBorder{75, 94, 122, 255};
constexpr lc::Color  kTitleColor{240, 244, 248, 255};
constexpr lc::Color  kBodyColor{188, 201, 214, 255};
constexpr lc::Color  kGridColor{44, 58, 79, 255};
constexpr lc::Color  kCloseColor{255, 80, 80, 255};
constexpr std::int32_t kPadding      = 20;
constexpr std::uint8_t kPanelRadius  = 18;  // mirrors rlvgl_ui::PANEL_RADIUS

}  // namespace

DashboardPanel::DashboardPanel(core::Rect bounds, std::string title,
                               std::string caption)
    : bounds_{bounds},
      title_{std::move(title)},
      font_{&core::fonts::FONT_6X10} {
    set_caption(std::move(caption));
}

std::size_t DashboardPanel::text_cols() const noexcept {
    const std::int32_t inner = std::max(bounds_.width - kPadding * 2, 0);
    // advance per char = scaled_width + scale (see BitmapFont::draw_str).
    const std::int32_t advance =
        std::max(font_->scaled_width() + static_cast<std::int32_t>(font_->scale), 1);
    return static_cast<std::size_t>(inner / advance);
}

void DashboardPanel::set_title(std::string title) { title_ = std::move(title); }

void DashboardPanel::set_caption(std::string caption) {
    caption_ = join_newline(wrap_text(caption, text_cols()));
}

void DashboardPanel::set_lines(std::span<const std::string> lines) {
    const std::size_t cols = text_cols();
    lines_.clear();
    for (const std::string& line : lines) {
        for (std::string& wrapped : wrap_text(line, cols)) {
            lines_.push_back(std::move(wrapped));
        }
    }
}

core::Rect DashboardPanel::bounds() const {
    return visible_ ? bounds_ : core::Rect{0, 0, 0, 0};
}

void DashboardPanel::draw(core::Renderer& renderer) const {
    if (!visible_) return;

    core::fill_rounded_rect(renderer, bounds_, kPanelBg, kPanelRadius);
    // DELTA: straight border (core lacks rounded-border path).
    core::detail::draw_border_straight(renderer, bounds_, kPanelBorder, 2);

    const std::int32_t body_y = ui::draw_panel_header(
        renderer, bounds_, accent_, title_, *font_, kTitleColor, kCloseColor,
        kGridColor);

    // Caption below header.
    const std::int32_t caption_line_h = font_->scaled_height() + 4;
    std::int32_t caption_y = body_y;
    std::size_t cstart = 0;
    while (true) {
        const std::size_t nl = caption_.find('\n', cstart);
        const std::size_t end =
            (nl == std::string::npos) ? caption_.size() : nl;
        font_->draw_str(renderer, bounds_.x + ui::kPanelPadding, caption_y,
                        std::string_view{caption_}.substr(cstart, end - cstart),
                        kBodyColor);
        caption_y += caption_line_h;
        if (nl == std::string::npos) break;
        cstart = nl + 1;
    }

    // Secondary divider below caption.
    const std::int32_t grid_top = std::max(caption_y + 8, bounds_.y + 108);
    renderer.fill_rect(
        core::Rect{bounds_.x + ui::kPanelPadding, grid_top,
                   bounds_.width - ui::kPanelPadding * 2, 1},
        kGridColor);

    // Tight body line spacing — matches the original 26px pitch for the
    // scale-2 FONT_6X10 so wrapped content fits the 312-tall panel.
    const std::int32_t body_line_h  = font_->scaled_height() + 6;
    const std::int32_t body_bottom  = bounds_.y + bounds_.height - kPadding;
    for (std::size_t index = 0; index < lines_.size(); ++index) {
        const std::int32_t y =
            grid_top + static_cast<std::int32_t>(index) * body_line_h;
        if (y + font_->scaled_height() > body_bottom) break;
        font_->draw_str(renderer, bounds_.x + ui::kPanelPadding, y,
                        lines_[index], kBodyColor);
    }
}

bool DashboardPanel::handle_event(const core::Event& event) {
    if (!visible_) return false;
    const auto* pr = std::get_if<core::event::PressRelease>(&event);
    if (pr == nullptr) return false;
    if (ui::panel_close_hit(bounds_, pr->x, pr->y)) {
        hide();
        return true;
    }
    return false;
}

}  // namespace lvglpp::app::disco_demo
