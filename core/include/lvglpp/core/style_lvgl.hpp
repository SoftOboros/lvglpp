// style_lvgl.hpp - LVGL-backed style and theme wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-07-STYLE-THEME.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_style.h, lvgl/src/core/lv_obj_style.h,
//         and lvgl/src/themes/lv_theme.h.
// DELTA:  lvglpp delegates style cascade, inheritance, transitions, and
//         theme application to LVGL instead of porting rlvgl's StyleState.

#ifndef LVGLPP_CORE_STYLE_LVGL_HPP
#define LVGLPP_CORE_STYLE_LVGL_HPP

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/timer.hpp"
#include "lvglpp/core/widget.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace lvglpp {

enum class StylePart : std::uint32_t {
    Main        = static_cast<std::uint32_t>(LV_PART_MAIN),
    Scrollbar   = static_cast<std::uint32_t>(LV_PART_SCROLLBAR),
    Indicator   = static_cast<std::uint32_t>(LV_PART_INDICATOR),
    Knob        = static_cast<std::uint32_t>(LV_PART_KNOB),
    Selected    = static_cast<std::uint32_t>(LV_PART_SELECTED),
    Items       = static_cast<std::uint32_t>(LV_PART_ITEMS),
    Cursor      = static_cast<std::uint32_t>(LV_PART_CURSOR),
    CustomFirst = static_cast<std::uint32_t>(LV_PART_CUSTOM_FIRST),
    Any         = static_cast<std::uint32_t>(LV_PART_ANY),
};

[[nodiscard]] constexpr lv_part_t to_lv(StylePart part) noexcept {
    return static_cast<lv_part_t>(part);
}

[[nodiscard]] constexpr StylePart style_part_from_lv(lv_part_t part) noexcept {
    return static_cast<StylePart>(part);
}

[[nodiscard]] constexpr StylePart custom_style_part(
    std::uint16_t custom_index) noexcept {
    return static_cast<StylePart>(
        static_cast<std::uint32_t>(LV_PART_CUSTOM_FIRST) +
        (static_cast<std::uint32_t>(custom_index) << 16U));
}

enum class StyleState : std::uint16_t {
    Default  = static_cast<std::uint16_t>(LV_STATE_DEFAULT),
    Alt      = static_cast<std::uint16_t>(LV_STATE_ALT),
    Checked  = static_cast<std::uint16_t>(LV_STATE_CHECKED),
    Focused  = static_cast<std::uint16_t>(LV_STATE_FOCUSED),
    FocusKey = static_cast<std::uint16_t>(LV_STATE_FOCUS_KEY),
    Edited   = static_cast<std::uint16_t>(LV_STATE_EDITED),
    Hovered  = static_cast<std::uint16_t>(LV_STATE_HOVERED),
    Pressed  = static_cast<std::uint16_t>(LV_STATE_PRESSED),
    Scrolled = static_cast<std::uint16_t>(LV_STATE_SCROLLED),
    Disabled = static_cast<std::uint16_t>(LV_STATE_DISABLED),
    User1    = static_cast<std::uint16_t>(LV_STATE_USER_1),
    User2    = static_cast<std::uint16_t>(LV_STATE_USER_2),
    User3    = static_cast<std::uint16_t>(LV_STATE_USER_3),
    User4    = static_cast<std::uint16_t>(LV_STATE_USER_4),
    Any      = static_cast<std::uint16_t>(LV_STATE_ANY),
};

[[nodiscard]] constexpr lv_state_t to_lv(StyleState state) noexcept {
    return static_cast<lv_state_t>(state);
}

[[nodiscard]] constexpr StyleState style_state_from_lv(lv_state_t state) noexcept {
    return static_cast<StyleState>(static_cast<std::uint16_t>(state));
}

[[nodiscard]] constexpr StyleState style_state_from_object(
    ObjectState state) noexcept {
    return style_state_from_lv(to_lv(state));
}

[[nodiscard]] constexpr StyleState operator|(StyleState lhs,
                                             StyleState rhs) noexcept {
    return static_cast<StyleState>(
        static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

class StyleSelector {
public:
    constexpr StyleSelector() noexcept = default;
    constexpr StyleSelector(StylePart part, StyleState state) noexcept
        : raw_{static_cast<lv_style_selector_t>(to_lv(part)) |
               static_cast<lv_style_selector_t>(to_lv(state))} {}

    [[nodiscard]] static constexpr StyleSelector raw(
        lv_style_selector_t selector) noexcept {
        return StyleSelector{selector};
    }

    [[nodiscard]] constexpr lv_style_selector_t to_lv_selector() const noexcept {
        return raw_;
    }

    [[nodiscard]] StylePart part() const noexcept;
    [[nodiscard]] StyleState state() const noexcept;

    [[nodiscard]] constexpr bool operator==(
        const StyleSelector&) const noexcept = default;

private:
    explicit constexpr StyleSelector(lv_style_selector_t raw) noexcept : raw_{raw} {}

    lv_style_selector_t raw_ = 0;
};

[[nodiscard]] constexpr lv_style_selector_t to_lv(
    StyleSelector selector) noexcept {
    return selector.to_lv_selector();
}

[[nodiscard]] lv_color_t to_lv(core::Color color) noexcept;
[[nodiscard]] core::Color color_from_lv(lv_color_t color,
                                        std::uint8_t alpha = 255) noexcept;

class StyleView {
public:
    // Args:
    //   raw: observes LVGL style. The pointed-to style is owned by LVGL,
    //        a constant object, or an LvStyle owner and must outlive this view.
    explicit StyleView(const lv_style_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] const lv_style_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool              empty() const noexcept { return raw_ == nullptr; }

    [[nodiscard]] lv_style_res_t get_prop(lv_style_prop_t prop,
                                          lv_style_value_t& value) const noexcept;

private:
    // observes: owned outside this view; never reset by this view.
    const lv_style_t* raw_ = nullptr;
};

class StyleTransition;

// Move-only RAII owner for an LVGL style.
//
// Ownership: owns raw_ while non-null. Destruction calls lv_style_reset()
// and then releases the C++ storage. LVGL observes attached styles; remove
// the style from objects before destroying the owner.
class LvStyle {
public:
    LvStyle();

    LvStyle(const LvStyle&)            = delete;
    LvStyle& operator=(const LvStyle&) = delete;

    LvStyle(LvStyle&&) noexcept = default;
    LvStyle& operator=(LvStyle&&) noexcept = default;

    ~LvStyle();

    [[nodiscard]] StyleView        borrow() const noexcept;
    [[nodiscard]] lv_style_t*      borrow_raw() noexcept;
    [[nodiscard]] const lv_style_t* borrow_raw() const noexcept;
    [[nodiscard]] bool             empty() const noexcept;

    void reset() noexcept;
    void copy_from(StyleView source) noexcept;
    void merge_from(StyleView source) noexcept;

    void set_prop(lv_style_prop_t prop, lv_style_value_t value) noexcept;
    [[nodiscard]] lv_style_res_t get_prop(lv_style_prop_t prop,
                                          lv_style_value_t& value) const noexcept;
    [[nodiscard]] bool remove_prop(lv_style_prop_t prop) noexcept;

    void set_bg_color(core::Color color) noexcept;
    void set_bg_opa(std::uint8_t opa) noexcept;
    void set_border_color(core::Color color) noexcept;
    void set_border_opa(std::uint8_t opa) noexcept;
    void set_border_width(std::int32_t width) noexcept;
    void set_radius(std::int32_t radius) noexcept;
    void set_text_color(core::Color color) noexcept;
    void set_text_opa(std::uint8_t opa) noexcept;
    void set_text_font(const lv_font_t* font) noexcept;
    void set_pad_all(std::int32_t value) noexcept;
    void set_margin_all(std::int32_t value) noexcept;
    void set_size(std::int32_t width, std::int32_t height) noexcept;
    void set_transition(const StyleTransition& transition) noexcept;

private:
    // owns: initialized with lv_style_init(); reset with lv_style_reset().
    std::unique_ptr<lv_style_t> raw_;
};

class StyleTransition {
public:
    StyleTransition(std::span<const lv_style_prop_t> props,
                    lv_anim_path_cb_t path_callback,
                    std::uint32_t duration_ms,
                    std::uint32_t delay_ms,
                    void* user_data = nullptr);

    StyleTransition(const StyleTransition&)            = delete;
    StyleTransition& operator=(const StyleTransition&) = delete;

    StyleTransition(StyleTransition&& other) noexcept;
    StyleTransition& operator=(StyleTransition&& other) noexcept;

    [[nodiscard]] const lv_style_transition_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }
    [[nodiscard]] bool empty() const noexcept { return props_.empty(); }

private:
    void refresh_descriptor() noexcept;

    // owns: property id array including LV_STYLE_PROP_INV sentinel.
    std::vector<lv_style_prop_t> props_;
    lv_anim_path_cb_t path_callback_ = nullptr;
    std::uint32_t duration_ms_ = 0;
    std::uint32_t delay_ms_ = 0;
    // external: LVGL passes through to transition animation user_data.
    void* user_data_ = nullptr;
    // owns: descriptor value; props points into props_.
    lv_style_transition_dsc_t raw_{};
};

[[nodiscard]] lv_style_value_t style_prop_default(lv_style_prop_t prop) noexcept;
[[nodiscard]] std::uint8_t style_prop_flags(lv_style_prop_t prop) noexcept;
[[nodiscard]] bool style_prop_has_flag(lv_style_prop_t prop,
                                       std::uint8_t flag) noexcept;

void add_style(ObjectView object, StyleView style, StyleSelector selector) noexcept;
[[nodiscard]] bool replace_style(ObjectView object,
                                 StyleView old_style,
                                 StyleView new_style,
                                 StyleSelector selector) noexcept;
void remove_style(ObjectView object, StyleView style, StyleSelector selector) noexcept;
void remove_styles(ObjectView object, StyleSelector selector) noexcept;
void remove_theme_styles(ObjectView object, StyleSelector selector) noexcept;
void remove_all_styles(ObjectView object) noexcept;
void set_style_disabled(ObjectView object,
                        StyleView style,
                        StyleSelector selector,
                        bool disabled) noexcept;
[[nodiscard]] bool style_disabled(ObjectView object,
                                  StyleView style,
                                  StyleSelector selector) noexcept;

void set_local_style_prop(ObjectView object,
                          lv_style_prop_t prop,
                          lv_style_value_t value,
                          StyleSelector selector) noexcept;
[[nodiscard]] lv_style_res_t local_style_prop(ObjectView object,
                                              lv_style_prop_t prop,
                                              lv_style_value_t& value,
                                              StyleSelector selector) noexcept;
[[nodiscard]] bool remove_local_style_prop(ObjectView object,
                                           lv_style_prop_t prop,
                                           StyleSelector selector) noexcept;
[[nodiscard]] lv_style_value_t resolved_style_prop(ObjectView object,
                                                   StylePart part,
                                                   lv_style_prop_t prop) noexcept;
[[nodiscard]] bool has_style_prop(ObjectView object,
                                  StyleSelector selector,
                                  lv_style_prop_t prop) noexcept;

[[nodiscard]] core::Color resolved_bg_color(ObjectView object,
                                            StylePart part) noexcept;
[[nodiscard]] std::uint8_t resolved_bg_opa(ObjectView object,
                                           StylePart part) noexcept;
[[nodiscard]] core::Color resolved_text_color(ObjectView object,
                                              StylePart part) noexcept;
[[nodiscard]] std::int32_t resolved_radius(ObjectView object,
                                           StylePart part) noexcept;

void report_style_change(StyleView style) noexcept;
void refresh_style(ObjectView object,
                   StylePart part,
                   lv_style_prop_t prop = LV_STYLE_PROP_ANY) noexcept;
void set_style_refresh_enabled(bool enabled) noexcept;

class ThemeView {
public:
    // Args:
    //   raw: observes LVGL theme. The pointed-to theme is owned by LVGL,
    //        a built-in theme singleton, or an LvTheme owner.
    explicit ThemeView(lv_theme_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] lv_theme_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool        empty() const noexcept { return raw_ == nullptr; }

    void set_parent(ThemeView parent) const noexcept;
    void set_apply_callback(lv_theme_apply_cb_t callback) const noexcept;

private:
    // observes: owned outside this view; never deleted by this view.
    lv_theme_t* raw_ = nullptr;
};

class LvTheme {
public:
    [[nodiscard]] static LvTheme make() noexcept;

    LvTheme() noexcept = default;
    LvTheme(const LvTheme&)            = delete;
    LvTheme& operator=(const LvTheme&) = delete;

    LvTheme(LvTheme&& other) noexcept;
    LvTheme& operator=(LvTheme&& other) noexcept;

    ~LvTheme();

    [[nodiscard]] ThemeView  borrow() const noexcept;
    [[nodiscard]] lv_theme_t* borrow_raw() const noexcept;
    [[nodiscard]] bool       empty() const noexcept;

    // Returns: owns raw LVGL theme handle; caller must delete it or
    // transfer lifecycle authority elsewhere.
    [[nodiscard]] lv_theme_t* release() noexcept;

    void reset() noexcept;
    void set_parent(ThemeView parent) noexcept;
    void set_apply_callback(lv_theme_apply_cb_t callback) noexcept;

private:
    explicit LvTheme(lv_theme_t* raw) noexcept;

    // owns: deleted with lv_theme_delete() when non-null.
    lv_theme_t* raw_ = nullptr;
};

[[nodiscard]] ThemeView theme_from_object(ObjectView object) noexcept;
[[nodiscard]] ThemeView display_theme(DisplayView display) noexcept;
void set_display_theme(DisplayView display, ThemeView theme) noexcept;
void apply_theme(ObjectView object) noexcept;
[[nodiscard]] const lv_font_t* theme_font_small(ObjectView object) noexcept;
[[nodiscard]] const lv_font_t* theme_font_normal(ObjectView object) noexcept;
[[nodiscard]] const lv_font_t* theme_font_large(ObjectView object) noexcept;
[[nodiscard]] core::Color theme_color_primary(ObjectView object) noexcept;
[[nodiscard]] core::Color theme_color_secondary(ObjectView object) noexcept;

#if LV_USE_THEME_DEFAULT
[[nodiscard]] ThemeView default_theme_init(DisplayView display,
                                           core::Color primary,
                                           core::Color secondary,
                                           bool dark,
                                           const lv_font_t* font) noexcept;
[[nodiscard]] ThemeView default_theme() noexcept;
void default_theme_deinit() noexcept;
#endif

#if LV_USE_THEME_SIMPLE
[[nodiscard]] ThemeView simple_theme_init(DisplayView display) noexcept;
[[nodiscard]] ThemeView simple_theme() noexcept;
void simple_theme_deinit() noexcept;
#endif

#if LV_USE_THEME_MONO
[[nodiscard]] ThemeView mono_theme_init(DisplayView display,
                                        bool dark_background,
                                        const lv_font_t* font) noexcept;
[[nodiscard]] ThemeView mono_theme() noexcept;
void mono_theme_deinit() noexcept;
#endif

}  // namespace lvglpp

#endif  // LVGLPP_CORE_STYLE_LVGL_HPP
