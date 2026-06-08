// icon_strip.cpp — IconStrip bounds / draw / handle_event.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/icon_strip.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (app composite).
// DELTA:  per-draw decode into a stack-local std::vector scratch buffer;
//         focus highlight via core::detail::draw_border_straight.

#include "lvglpp/app/disco_demo/icon_strip.hpp"

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

// Decode an RLE icon into `buf` (sized to width*height). Returns the icon
// dimensions, or nullopt on parse/decode failure. Mirrors icon_strip.rs:84
// decode_into. `buf` is a caller-owned scratch buffer — never aliased with
// DMA (host has no DMA).
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

void IconStrip::set_slot(std::size_t index, IconSlot slot) {
    if (index < SLOT_COUNT) {
        slots_[index] = std::move(slot);
    }
}

core::Rect IconStrip::slot_bounds(std::size_t index) const noexcept {
    const std::int32_t y =
        margin_top_ + static_cast<std::int32_t>(index) * (icon_size_ + gap_);
    return core::Rect{x_, y, icon_size_, icon_size_};
}

core::Rect IconStrip::bounds() const {
    const std::int32_t n = static_cast<std::int32_t>(SLOT_COUNT);
    const std::int32_t total_h = n * icon_size_ + (n - 1) * gap_ + margin_top_;
    return core::Rect{x_, 0, icon_size_, total_h};
}

void IconStrip::draw(core::Renderer& renderer) const {
    std::vector<lc::Color> buf;
    for (std::size_t index = 0; index < SLOT_COUNT; ++index) {
        const auto& maybe = slots_[index];
        if (!maybe.has_value()) continue;
        const IconSlot& slot = *maybe;

        if (auto dims = decode_icon(slot.rle, buf)) {
            const auto [w, h]   = *dims;
            const core::Rect b  = slot_bounds(index);
            const std::int32_t x =
                b.x + (b.width - static_cast<std::int32_t>(w)) / 2;
            const std::int32_t y =
                b.y + (b.height - static_cast<std::int32_t>(h)) / 2;
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
            core::detail::draw_border_straight(renderer, slot_bounds(index),
                                               FOCUS_HIGHLIGHT_COLOR,
                                               FOCUS_BORDER_WIDTH);
        }
    }
}

bool IconStrip::handle_event(const core::Event& event) {
    const auto* pr = std::get_if<core::event::PressRelease>(&event);
    if (pr == nullptr) return false;

    const std::int32_t step = icon_size_ + gap_;
    for (std::size_t index = 0; index < SLOT_COUNT; ++index) {
        auto& maybe = slots_[index];
        if (!maybe.has_value() || !maybe->enabled) continue;

        const std::int32_t i = static_cast<std::int32_t>(index);
        const std::int32_t cell_top =
            (index == 0) ? 0 : margin_top_ + i * step - gap_ / 2;
        const std::int32_t cell_bottom =
            (index == SLOT_COUNT - 1)
                ? margin_top_ + static_cast<std::int32_t>(SLOT_COUNT) * step
                : margin_top_ + (i + 1) * step - gap_ / 2;

        if (pr->x >= x_ && pr->y >= cell_top && pr->y < cell_bottom) {
            if (maybe->on_tap) {
                maybe->on_tap(index);
            }
            return true;
        }
    }
    return false;
}

}  // namespace lvglpp::app::disco_demo
