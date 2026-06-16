// font_registry.hpp — FontId identifier + a heap-free FontId→Font registry
// (FONT-05). See docs/font/05-font-registry.md.
//
// PARITY: rlvgl/core/src/font.rs:15 (FontId(u16), FontId::DEFAULT) +
//         rlvgl/docs/concepts/FONT-05-FONT-REGISTRY.md (v0.2.4 @ 343f596).
// LVGL:   lv_obj_set_style_text_font is the cascade→object bridge — NATIVE; a
//         style stores the lv_font_t* and LVGL resolves it per object, so the
//         rlvgl "walk the tree and feed each node's resolved font into its
//         slot" pass has no lvglpp analog (DELTA, chapter §10).
// DELTA:  FontId mirrors rlvgl's FontId(u16). FontRegistry is a fixed-capacity,
//         allocator-free array (usable under LVGLPP_EMBEDDED_POSTURE). It
//         OBSERVES fonts (transfers no ownership); every registered lv_font_t
//         MUST outlive the objects it is applied to (FONT-00 §5.2).

#ifndef LVGLPP_CORE_FONT_REGISTRY_HPP
#define LVGLPP_CORE_FONT_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#include "lvglpp/core/draw.hpp"           // Font
#include "lvglpp/core/object.hpp"         // Object (apply target)
#include "lvglpp/core/style_cascade.hpp"  // style::Selector

namespace lvglpp::core {

// A stable, by-value font identifier. Mirrors rlvgl FontId(u16); the registry
// key. Standards Action: any new well-known id is amended in rlvgl
// core/src/font.rs first, then mirrored here. See docs/font/05-...md §5.1.
class FontId {
public:
    // Default-constructs to the default id (0 == rlvgl FontId::DEFAULT).
    constexpr FontId() noexcept = default;
    constexpr explicit FontId(std::uint16_t value) noexcept : value_{value} {}

    // The well-known default id (0); always resolves to Font::default_font().
    [[nodiscard]] static constexpr FontId default_id() noexcept { return FontId{0}; }

    [[nodiscard]] constexpr std::uint16_t value() const noexcept { return value_; }

    constexpr bool operator==(const FontId&) const noexcept = default;

private:
    std::uint16_t value_ = 0;
};

// A heap-free FontId→Font map. `Capacity` slots live inline (no allocator), so
// the registry is usable under embedded posture. It observes fonts; it owns
// nothing and frees nothing. Default capacity 16 (chapter §5.2).
//
// Ownership: each slot observes an external/static lv_font_t (via the Font
// handle). Every registered font MUST outlive the objects it is applied to.
template <std::size_t Capacity = 16>
class FontRegistry {
public:
    static_assert(Capacity > 0, "FontRegistry needs at least one slot");

    // Map `id` to `font`. Replaces an existing entry for `id`; otherwise
    // appends. Returns false (registering nothing) only when the registry is
    // full and `id` is new — never a silent drop (chapter §5.2).
    //   font: observes; must outlive every object it is later applied to.
    [[nodiscard]] bool register_font(FontId id, Font font) noexcept {
        for (std::size_t i = 0; i < count_; ++i) {
            if (slots_[i].id == id) {
                slots_[i].font = font;
                return true;
            }
        }
        if (count_ >= Capacity) {
            return false;  // full and id is new — caller must widen Capacity.
        }
        slots_[count_].id   = id;
        slots_[count_].font = font;
        ++count_;
        return true;
    }

    // Resolve `id` to a Font. The default id always resolves to
    // Font::default_font() (even unregistered, chapter §5.4); any other
    // unregistered id resolves to an empty Font.
    [[nodiscard]] Font lookup(FontId id) const noexcept {
        if (id == FontId::default_id()) {
            return Font::default_font();
        }
        for (std::size_t i = 0; i < count_; ++i) {
            if (slots_[i].id == id) {
                return slots_[i].font;
            }
        }
        return Font{};
    }

    // Resolve `id` and apply it to `obj` for `sel` via the FONT-00 selection
    // setter (Object::set_local_text_font). Returns false (applying nothing)
    // if `id` resolves to an empty Font.
    //   obj: borrows mut; receives the resolved font.
    [[nodiscard]] bool apply(Object& obj, FontId id, style::Selector sel) noexcept {
        const Font font = lookup(id);
        if (font.empty()) {
            return false;
        }
        obj.set_local_text_font(font, sel);
        return true;
    }

    [[nodiscard]] std::size_t           count()    const noexcept { return count_; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    struct Slot {
        FontId id{};
        Font   font{};  // observes: external/static lv_font_t.
    };

    std::array<Slot, Capacity> slots_{};
    std::size_t                count_ = 0;
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_FONT_REGISTRY_HPP
