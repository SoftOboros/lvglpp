// list.hpp — vertical text list with single selection.
//
// PARITY: rlvgl/widgets/src/list.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/widgets/list/lv_list.h — informative; List does
//         not shadow lv_list.
// DELTA:  items() returns std::span<const std::string> (borrows)
//         rather than rlvgl's &[String]; otherwise none. No scroll
//         state — rlvgl parity (WID-05 §5.2; scrolling is an
//         rlvgl-first amendment).
//
// docs/widgets-list/00-list.md (WID-05).

#ifndef LVGLPP_WIDGETS_LEGACY_LIST_HPP
#define LVGLPP_WIDGETS_LEGACY_LIST_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets::legacy {

class List final : public ::lvglpp::core::Widget {
public:
    // WID-05 §5.2 FROZEN — list.rs:49/:74 hardcoded row height.
    static constexpr std::int32_t ROW_HEIGHT = 16;

    explicit List(::lvglpp::core::Rect bounds) noexcept
        : bounds_{bounds} {}

    // Public — parity with rlvgl `pub style` / `pub text_color`.
    ::lvglpp::core::Style style{};
    ::lvglpp::core::Color text_color{0, 0, 0, 255};

    // Args:
    //   text: owns std::string — consumed via move (list.rs:33
    //         add_item(impl Into<String>)).
    void add_item(std::string text) {
        items_.push_back(std::move(text));
    }

    // borrows: views into this List's storage; invalidated by
    // add_item (vector growth).
    [[nodiscard]] std::span<const std::string> items() const noexcept {
        return {items_.data(), items_.size()};
    }

    [[nodiscard]] std::optional<std::size_t> selected() const noexcept {
        return selected_;
    }

    // ---- Widget overrides ----------------------------------------

    [[nodiscard]] ::lvglpp::core::Rect bounds() const override {
        return bounds_;
    }

    void draw(::lvglpp::core::Renderer& renderer) const override;

    // PressRelease inside an item row selects it and consumes the
    // event; everything else is ignored (list.rs:88–103).
    [[nodiscard]] bool handle_event(
        const ::lvglpp::core::Event& event) override;

private:
    // Mirrors list.rs:48 index_at: y → item index, or nullopt when
    // above/below the populated rows.
    [[nodiscard]] std::optional<std::size_t>
    index_at(std::int32_t y) const noexcept;

    ::lvglpp::core::Rect bounds_{};
    std::vector<std::string> items_{};            // owns
    std::optional<std::size_t> selected_{};
};

}  // namespace lvglpp::widgets::legacy

#endif  // LVGLPP_WIDGETS_LEGACY_LIST_HPP
