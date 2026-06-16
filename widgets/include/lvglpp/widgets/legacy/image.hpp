// image.hpp — non-interactive pixel-blit widget.
//
// PARITY: rlvgl/widgets/src/image.rs (v0.2.0 @ 79f730d).
// LVGL:   lvgl/src/widgets/image/lv_image.h — informative; Image
//         does not shadow lv_image.
// DELTA:  pixels_ is std::span<const Color> rather than rlvgl's
//         &'a [Color]; the lifetime contract is identical and is
//         the caller's to uphold (no borrow checker — see WID-06
//         §5.1).
//
// docs/widgets-image/00-image.md (WID-06).

#ifndef LVGLPP_WIDGETS_LEGACY_IMAGE_HPP
#define LVGLPP_WIDGETS_LEGACY_IMAGE_HPP

#include <cstdint>
#include <span>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/widget.hpp"

namespace lvglpp::widgets::legacy {

class Image final : public ::lvglpp::core::Widget {
public:
    // Args:
    //   pixels: borrows — row-major width×height ARGB pixels; the
    //           storage MUST outlive this widget (rlvgl &'a [Color]
    //           parity). Typically the decoded output of a CORE-07
    //           plugin owned by the enclosing composition.
    Image(::lvglpp::core::Rect bounds,
          std::int32_t width,
          std::int32_t height,
          std::span<const ::lvglpp::core::Color> pixels) noexcept
        : bounds_{bounds}, width_{width}, height_{height},
          pixels_{pixels} {}

    // Public — parity with rlvgl `pub style`.
    ::lvglpp::core::Style style{};

    [[nodiscard]] std::int32_t width() const noexcept { return width_; }
    [[nodiscard]] std::int32_t height() const noexcept { return height_; }
    // borrows: see constructor contract.
    [[nodiscard]] std::span<const ::lvglpp::core::Color>
    pixels() const noexcept { return pixels_; }

    // ---- Widget overrides ----------------------------------------

    [[nodiscard]] ::lvglpp::core::Rect bounds() const override {
        return bounds_;
    }

    void draw(::lvglpp::core::Renderer& renderer) const override;

    // Images are non-interactive (image.rs:46–49).
    [[nodiscard]] bool handle_event(
        const ::lvglpp::core::Event& /*event*/) override {
        return false;
    }

private:
    ::lvglpp::core::Rect bounds_{};
    std::int32_t width_  = 0;
    std::int32_t height_ = 0;
    // borrows: caller-owned decoded pixels (constructor contract).
    std::span<const ::lvglpp::core::Color> pixels_{};
};

}  // namespace lvglpp::widgets::legacy

#endif  // LVGLPP_WIDGETS_LEGACY_IMAGE_HPP
