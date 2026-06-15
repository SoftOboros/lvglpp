// style_cascade.hpp — RAII over lv_style_t + part/state selectors + a theme
// handle (LPAR-07). See docs/core-style/01-style-cascade-theme.md.
//
// PARITY: rlvgl/core/src/{style_cascade,theme}.rs (v0.2.4 @ 343f596) — the
//         selector / part / state cascade and theme semantics. rlvgl
//         re-implements the cascade; lvglpp wraps LVGL's.
// LVGL:   lvgl/src/misc/lv_style.h, lvgl/src/core/lv_obj_style.h,
//         lvgl/src/themes/lv_theme.h (+ default/lv_theme_default.h).
// DELTA:  Style is NON-movable and NON-copyable. lv_obj_add_style records a
//         POINTER to the lv_style_t (not a copy), so the storage address
//         MUST stay stable for the Style's whole lifetime and the Style MUST
//         outlive every object it is added to (the load-bearing rule, chapter
//         §5.1). The cascade types live in nested namespace
//         lvglpp::core::style so the RAII `style::Style` coexists with the
//         CORE-05 value-type `lvglpp::core::Style` until the LVGLPP-WRAP
//         migration removes the value type and promotes this one to
//         lvglpp::core (CORE-05 reconciliation DELTA, chapter §10).

#ifndef LVGLPP_CORE_STYLE_CASCADE_HPP
#define LVGLPP_CORE_STYLE_CASCADE_HPP

#include <cstdint>

#include "lvglpp/core/object.hpp"  // ObjectState (mirrors lv_state_t), ObjectView

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::core::style {

// Mirror of LVGL widget parts (lv_part_t, lvgl/src/core/lv_obj_style.h).
// Standards Action to add a value that must agree with rlvgl. See
// docs/core-style/01-style-cascade-theme.md §5.2.
enum class Part : std::uint32_t {
    Main      = LV_PART_MAIN,
    Scrollbar = LV_PART_SCROLLBAR,
    Indicator = LV_PART_INDICATOR,
    Knob      = LV_PART_KNOB,
    Selected  = LV_PART_SELECTED,
    Items     = LV_PART_ITEMS,
    Cursor    = LV_PART_CURSOR,
    Any       = LV_PART_ANY,
};

// A (Part, State) pair packed into lv_style_selector_t (= part | state).
//
// The state half reuses ObjectState (object.hpp), which already mirrors
// lv_state_t — defining a second State enum here would fork the lv_state_t
// vocabulary, which the discipline forbids. LV_PART_MAIN and
// LV_STATE_DEFAULT are both 0, so a default-constructed Selector is
// "main part, default state" — exactly LVGL's `0` selector.
class Selector {
public:
    constexpr Selector() noexcept = default;

    // Part (+ optional state). `Selector(Part::Knob)` targets the knob in
    // every state; `Selector(Part::Knob, ObjectState::Pressed)` narrows it.
    constexpr explicit Selector(Part part, ObjectState state = ObjectState::Default) noexcept
        : sel_{static_cast<lv_style_selector_t>(part) |
               static_cast<lv_style_selector_t>(state)} {}

    // State-only selector (part defaults to Main, which is 0).
    constexpr explicit Selector(ObjectState state) noexcept
        : sel_{static_cast<lv_style_selector_t>(state)} {}

    [[nodiscard]] constexpr lv_style_selector_t raw() const noexcept { return sel_; }

private:
    lv_style_selector_t sel_ = 0;  // LV_PART_MAIN | LV_STATE_DEFAULT
};

// RAII owner of an lv_style_t. See chapter §3, §5.1 (the outlives-objects
// rule). Configure it once with the setters, then attach it to one or more
// objects via Object::add_style.
//
// Ownership: owns the lv_style_t storage (lv_style_init / lv_style_reset).
// NON-movable + NON-copyable so the storage address is stable for life:
// Object::add_style hands LVGL `&style_`, and that pointer MUST stay valid
// until every object using the Style is deleted (or removes it).
// borrows-into-LVGL: a live object borrows this Style; destroying a Style
// still referenced by a live object is undefined behaviour and forbidden.
class Style {
public:
    Style() noexcept { lv_style_init(&style_); }
    ~Style() { lv_style_reset(&style_); }

    Style(const Style&)            = delete;
    Style& operator=(const Style&) = delete;
    Style(Style&&)                 = delete;  // address must stay stable (§5.1).
    Style& operator=(Style&&)      = delete;

    // Drop all properties. lv_style_reset re-zeroes the storage in place, so
    // the address is unchanged and the Style stays reusable. Objects that
    // still reference it pick up the cleared cascade on the next refresh.
    void reset() noexcept { lv_style_reset(&style_); }

    // Property setters — thin wrappers over lv_style_set_*; return *this for
    // a fluent builder shape (the Style stays in place; only its contents
    // change). A representative subset; reach lv_style_set_* directly via
    // borrow_raw() for properties not surfaced here.
    Style& set_bg_color(lv_color_t c) noexcept {
        lv_style_set_bg_color(&style_, c);
        return *this;
    }
    Style& set_bg_opa(lv_opa_t opa) noexcept {
        lv_style_set_bg_opa(&style_, opa);
        return *this;
    }
    Style& set_border_color(lv_color_t c) noexcept {
        lv_style_set_border_color(&style_, c);
        return *this;
    }
    Style& set_border_width(std::int32_t w) noexcept {
        lv_style_set_border_width(&style_, w);
        return *this;
    }
    Style& set_radius(std::int32_t r) noexcept {
        lv_style_set_radius(&style_, r);
        return *this;
    }
    Style& set_text_color(lv_color_t c) noexcept {
        lv_style_set_text_color(&style_, c);
        return *this;
    }
    Style& set_pad_all(std::int32_t p) noexcept {
        lv_style_set_pad_all(&style_, p);
        return *this;
    }
    Style& set_width(std::int32_t w) noexcept {
        lv_style_set_width(&style_, w);
        return *this;
    }
    Style& set_height(std::int32_t h) noexcept {
        lv_style_set_height(&style_, h);
        return *this;
    }

    // borrows: the lv_style_t storage, address-stable for this Style's whole
    // lifetime. Object::add_style stores this pointer; the caller must keep
    // the Style alive at least as long as every object using it.
    [[nodiscard]] lv_style_t* borrow_raw() noexcept { return &style_; }

private:
    // owns: the style storage. Address-stable because Style is non-movable.
    lv_style_t style_{};
};

// Non-owning handle over an lv_theme_t. LVGL's default theme lives in the
// library's internal static storage (lv_theme_default), never on the user's
// heap — so Theme observes; it never frees.
//
// Ownership: observes an LVGL-owned lv_theme_t. external lifetime — valid
// until the display it belongs to is deleted or re-themed.
class Theme {
public:
    constexpr Theme() noexcept = default;
    constexpr explicit Theme(lv_theme_t* t) noexcept : theme_{t} {}  // observes.

    // Initialise (or re-fetch) LVGL's default theme on `disp` and return a
    // handle observing the LVGL-owned static storage.
    //   disp:      borrows; the display the theme is configured for.
    //   font:      borrows; the theme's base font (e.g. lv_font_get_default()).
    //              Must outlive the theme's use.
    [[nodiscard]] static Theme default_init(lv_display_t* disp, lv_color_t primary,
                                            lv_color_t secondary, bool dark,
                                            const lv_font_t* font) noexcept {
        return Theme{lv_theme_default_init(disp, primary, secondary, dark, font)};
    }

    // The theme currently applied to `obj` (lv_theme_get_from_obj). observes.
    [[nodiscard]] static Theme from(ObjectView obj) noexcept {
        lv_obj_t* raw = obj.borrow_raw();
        return Theme{raw != nullptr ? lv_theme_get_from_obj(raw) : nullptr};
    }

    // Re-apply the active display theme to `obj` and its tree
    // (lv_theme_apply). No-op when `obj` is empty.
    static void apply_to(ObjectView obj) noexcept {
        lv_obj_t* raw = obj.borrow_raw();
        if (raw != nullptr) {
            lv_theme_apply(raw);
        }
    }

    // Chain `parent` below this theme so unset properties fall through to it
    // (lv_theme_set_parent). No-op if this handle is empty.
    void set_parent(Theme parent) noexcept {
        if (theme_ != nullptr) {
            lv_theme_set_parent(theme_, parent.theme_);
        }
    }

    // Bind this theme to a display (lv_display_set_theme). No-op if empty.
    //   disp: borrows; the display to re-theme.
    void bind_to_display(lv_display_t* disp) noexcept {
        if (theme_ != nullptr && disp != nullptr) {
            lv_display_set_theme(disp, theme_);
        }
    }

    [[nodiscard]] lv_theme_t* borrow_raw() const noexcept { return theme_; }
    [[nodiscard]] bool        empty()      const noexcept { return theme_ == nullptr; }

private:
    // observes: LVGL-owned (lv_theme_default static storage). Never freed here.
    lv_theme_t* theme_ = nullptr;
};

}  // namespace lvglpp::core::style

#endif  // LVGLPP_CORE_STYLE_CASCADE_HPP
