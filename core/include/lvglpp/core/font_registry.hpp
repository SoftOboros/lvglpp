// font_registry.hpp - FontId identifier and heap-free FontId-to-font registry.
//
// PARITY: rlvgl/core/src/font.rs and
//         rlvgl/docs/concepts/FONT-05-FONT-REGISTRY.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/core/lv_obj_style_gen.h (lv_obj_set_style_text_font).
// DELTA:  LVGL stores resolved lv_font_t pointers directly in the style
//         cascade, so lvglpp only needs a small id-to-font lookup and an
//         apply helper over set_local_text_font().

#ifndef LVGLPP_CORE_FONT_REGISTRY_HPP
#define LVGLPP_CORE_FONT_REGISTRY_HPP

#include "lvglpp/core/draw_lvgl.hpp"
#include "lvglpp/core/style_lvgl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace lvglpp {

class FontId {
public:
    constexpr FontId() noexcept = default;
    explicit constexpr FontId(std::uint16_t value) noexcept : value_{value} {}

    [[nodiscard]] static constexpr FontId default_id() noexcept {
        return FontId{0};
    }

    [[nodiscard]] constexpr std::uint16_t value() const noexcept { return value_; }

    [[nodiscard]] constexpr bool operator==(const FontId&) const noexcept = default;

private:
    std::uint16_t value_ = 0;
};

template <std::size_t Capacity = 16>
class FontRegistry {
public:
    static_assert(Capacity > 0, "FontRegistry needs at least one slot");

    // Args:
    //   font: observes external/static LVGL font storage. The raw lv_font_t
    //         must outlive every object this registry applies it to.
    [[nodiscard]] bool register_font(FontId id, LvFontView font) noexcept {
        for (std::size_t i = 0; i < count_; ++i) {
            if (slots_[i].id == id) {
                slots_[i].font = font;
                return true;
            }
        }
        if (count_ >= Capacity) {
            return false;
        }
        slots_[count_] = Slot{id, font};
        ++count_;
        return true;
    }

    [[nodiscard]] LvFontView lookup(FontId id) const noexcept {
        if (id == FontId::default_id()) {
            return LvFontView::default_font();
        }
        for (std::size_t i = 0; i < count_; ++i) {
            if (slots_[i].id == id) {
                return slots_[i].font;
            }
        }
        return LvFontView{nullptr};
    }

    [[nodiscard]] bool apply(ObjectView object,
                             FontId id,
                             StyleSelector selector) const noexcept {
        const LvFontView font = lookup(id);
        if (font.empty()) {
            return false;
        }
        set_local_text_font(object, font.borrow_raw(), selector);
        return true;
    }

    [[nodiscard]] std::size_t count() const noexcept { return count_; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return Capacity;
    }

private:
    struct Slot {
        FontId id{};
        // observes: external/static LVGL font storage; never released here.
        LvFontView font{nullptr};
    };

    std::array<Slot, Capacity> slots_{};
    std::size_t count_ = 0;
};

}  // namespace lvglpp

#endif  // LVGLPP_CORE_FONT_REGISTRY_HPP
