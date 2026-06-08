// wing.cpp — Wing ctor / bounds / draw / handle_event / clear_region.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/wing.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  per-draw decode into a stack-local scratch buffer; rounded border
//         rendered as a straight border (core lacks rounded-border path).

#include "lvglpp/app/disco_demo/wing.hpp"

#include <algorithm>
#include <utility>
#include <variant>
#include <vector>

#include "lvglpp/app/disco_demo/assets.hpp"
#include "lvglpp/core/draw_helpers.hpp"
#include "lvglpp/core/rle.hpp"

namespace lvglpp::app::disco_demo {

namespace {

namespace lc  = ::lvglpp::core;
namespace rle = ::lvglpp::core::rle;

// FROZEN geometry/colors — mirror wing.rs:18-26.
constexpr std::int32_t kIconSize    = 60;
constexpr std::int32_t kGap         = 10;
constexpr std::int32_t kMarginTop   = 17;
constexpr std::int32_t kWingX       = 10;
constexpr std::uint8_t kRadius      = 18;
constexpr std::uint8_t kBorderWidth = 2;
constexpr lc::Color    kBgColor{30, 30, 30, 240};
constexpr lc::Color    kBorderColor{80, 80, 80, 255};

// Mirror icon_strip.cpp decode_icon (wing.rs:125 decode_into).
std::optional<std::pair<std::uint32_t, std::uint32_t>>
decode_icon(std::span<const std::uint8_t> blob, std::vector<lc::Color>& buf) {
    auto parsed = rle::parse_blob(blob);
    if (!parsed.has_value()) return std::nullopt;
    const auto& view = parsed.value();
    const std::size_t total =
        static_cast<std::size_t>(view.width) * static_cast<std::size_t>(view.height);
    buf.assign(total, lc::Color{});
    if (!rle::decode_into(view, std::span<lc::Color>(buf)).has_value()) {
        return std::nullopt;
    }
    return std::pair{static_cast<std::uint32_t>(view.width),
                     static_cast<std::uint32_t>(view.height)};
}

}  // namespace

Wing::Wing(
    std::span<const std::pair<std::span<const std::uint8_t>, bool>> icons) {
    slot_count_ = std::min(icons.size(), MAX_SLOTS);
    const std::int32_t count = static_cast<std::int32_t>(slot_count_);
    const std::int32_t total_height =
        kMarginTop + count * kIconSize +
        std::max(count - 1, 0) * kGap + kMarginTop;

    for (std::size_t i = 0; i < slot_count_; ++i) {
        slots_[i] = WingSlot{icons[i].first, icons[i].second, {}};
    }

    bounds_ = core::Rect{
        kWingX - static_cast<std::int32_t>(kBorderWidth),
        0,
        kIconSize + static_cast<std::int32_t>(kBorderWidth) * 2,
        total_height,
    };
}

bool Wing::toggle_visible() noexcept {
    if (visible_) {
        close();
    } else {
        visible_ = true;
    }
    return visible_;
}

void Wing::close() noexcept {
    if (visible_) {
        clear_countdown_ = CLEAR_FRAMES;
        visible_         = false;
    }
}

core::Rect Wing::icon_rect(std::size_t index) const noexcept {
    return core::Rect{
        kWingX,
        kMarginTop + static_cast<std::int32_t>(index) * (kIconSize + kGap),
        kIconSize,
        kIconSize,
    };
}

void Wing::draw(core::Renderer& renderer) const {
    if (!visible_) return;

    const core::Rect bg_rect{kWingX - 2, 0, kIconSize + 4, bounds_.height};
    core::fill_rounded_rect(renderer, bg_rect, kBgColor, kRadius);
    // DELTA: lvglpp core has no rounded-border path; the straight border is
    // the radius==0 fallback (visually exact for the demo's purposes).
    core::detail::draw_border_straight(renderer, bg_rect, kBorderColor,
                                       kBorderWidth);

    std::vector<lc::Color> buf;
    for (std::size_t index = 0; index < slot_count_; ++index) {
        const auto& maybe = slots_[index];
        if (!maybe.has_value()) continue;
        const WingSlot& slot = *maybe;

        if (auto dims = decode_icon(slot.rle, buf)) {
            const auto [w, h]   = *dims;
            const core::Rect r  = icon_rect(index);
            const std::int32_t x =
                r.x + (r.width - static_cast<std::int32_t>(w)) / 2;
            const std::int32_t y =
                r.y + (r.height - static_cast<std::int32_t>(h)) / 2;
            if (!slot.enabled) {
                for (lc::Color& c : buf) {
                    c.r = static_cast<std::uint8_t>(c.r / 2);
                    c.g = static_cast<std::uint8_t>(c.g / 2);
                    c.b = static_cast<std::uint8_t>(c.b / 2);
                }
            }
            renderer.draw_pixels(x, y, std::span<const lc::Color>(buf), w, h);
        }
        if (focused_slot_ == index) {
            core::detail::draw_border_straight(renderer, icon_rect(index),
                                               FOCUS_HIGHLIGHT_COLOR,
                                               FOCUS_BORDER_WIDTH);
        }
    }
}

bool Wing::handle_event(const core::Event& event) {
    if (!visible_) return false;

    const auto* pr = std::get_if<core::event::PressRelease>(&event);
    if (pr == nullptr) return false;

    const std::int32_t step = kIconSize + kGap;
    for (std::size_t index = 0; index < slot_count_; ++index) {
        const std::int32_t i = static_cast<std::int32_t>(index);
        const std::int32_t cell_top =
            (index == 0) ? 0 : kMarginTop + i * step - kGap / 2;
        const std::int32_t cell_bottom =
            (index == slot_count_ - 1)
                ? kMarginTop + static_cast<std::int32_t>(slot_count_) * step
                : kMarginTop + (i + 1) * step - kGap / 2;

        if (pr->x <= kWingX + kIconSize && pr->y >= cell_top &&
            pr->y < cell_bottom) {
            auto& maybe = slots_[index];
            if (maybe.has_value() && maybe->enabled) {
                if (maybe->on_tap) {
                    maybe->on_tap(index);
                }
                close();
                return true;
            }
            return true;
        }
    }

    if (pr->x < 720) {
        close();
    }
    return false;
}

std::optional<core::Rect> Wing::clear_region() {
    if (clear_countdown_ > 0 && !visible_) {
        --clear_countdown_;
        return core::Rect{kWingX - 2, 0, kIconSize + 4, bounds_.height};
    }
    return std::nullopt;
}

}  // namespace lvglpp::app::disco_demo
