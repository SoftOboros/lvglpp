// list.cpp — List draw + selection event handling.
//
// PARITY: rlvgl/widgets/src/list.rs:48–103 (v0.2.0 @ 79f730d) —
//         index_at, the per-item draw loop (baseline anchor
//         y + i*16 + 16, x + 2), selected-item colour from
//         style.border_color, PressRelease-only selection.
//
// docs/widgets-list/00-list.md §5.2/§5.3.

#include "lvglpp/widgets/legacy/list.hpp"

#include <variant>

#include "lvglpp/core/draw_helpers.hpp"

namespace lvglpp::widgets::legacy {

void List::draw(::lvglpp::core::Renderer& renderer) const {
    ::lvglpp::core::draw_widget_bg(renderer, bounds_, style);

    std::int32_t i = 0;
    for (const std::string& item : items_) {
        const bool is_selected =
            selected_.has_value() &&
            *selected_ == static_cast<std::size_t>(i);
        const ::lvglpp::core::Color base =
            is_selected ? style.border_color : text_color;
        renderer.draw_text(bounds_.x + 2,
                           bounds_.y + i * ROW_HEIGHT + ROW_HEIGHT,
                           item,
                           base.with_alpha(style.alpha));
        ++i;
    }
}

std::optional<std::size_t> List::index_at(std::int32_t y) const noexcept {
    if (y < bounds_.y) return std::nullopt;
    const std::int32_t row = (y - bounds_.y) / ROW_HEIGHT;
    if (row < 0 ||
        static_cast<std::size_t>(row) >= items_.size()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(row);
}

bool List::handle_event(const ::lvglpp::core::Event& event) {
    const auto* pr =
        std::get_if<::lvglpp::core::event::PressRelease>(&event);
    if (pr == nullptr) return false;
    if (pr->x < bounds_.x || pr->x >= bounds_.x + bounds_.width) {
        return false;
    }
    const auto idx = index_at(pr->y);
    if (!idx.has_value()) return false;
    selected_ = idx;
    return true;
}

}  // namespace lvglpp::widgets::legacy
