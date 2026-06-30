// style_lvgl.cpp - LVGL-backed style and theme wrapper implementation.
//
// PARITY: rlvgl/docs/concepts/LPAR-07-STYLE-THEME.md (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/misc/lv_style.c, lvgl/src/core/lv_obj_style.c,
//         and lvgl/src/themes/.
// DELTA:  delegates cascade, inheritance, transitions, and themes to LVGL.

#include "lvglpp/core/style_lvgl.hpp"

namespace lvglpp {

namespace {

[[nodiscard]] lv_obj_t* raw_or_null(ObjectView object) noexcept {
    return object.empty() ? nullptr : object.borrow_raw();
}

[[nodiscard]] lv_display_t* raw_or_null(DisplayView display) noexcept {
    return display.empty() ? nullptr : display.borrow_raw();
}

}  // namespace

StylePart StyleSelector::part() const noexcept {
    return style_part_from_lv(lv_obj_style_get_selector_part(raw_));
}

StyleState StyleSelector::state() const noexcept {
    return style_state_from_lv(lv_obj_style_get_selector_state(raw_));
}

lv_color_t to_lv(core::Color color) noexcept {
    return lv_color_make(color.r, color.g, color.b);
}

core::Color color_from_lv(lv_color_t color, std::uint8_t alpha) noexcept {
    return core::Color{color.red, color.green, color.blue, alpha};
}

lv_style_res_t StyleView::get_prop(lv_style_prop_t prop,
                                   lv_style_value_t& value) const noexcept {
    if (raw_ == nullptr) {
        return LV_STYLE_RES_NOT_FOUND;
    }
    return lv_style_get_prop(raw_, prop, &value);
}

LvStyle::LvStyle() : raw_{std::make_unique<lv_style_t>()} {
    lv_style_init(raw_.get());
}

LvStyle::~LvStyle() {
    reset();
}

StyleView LvStyle::borrow() const noexcept {
    return StyleView{raw_.get()};
}

lv_style_t* LvStyle::borrow_raw() noexcept {
    return raw_.get();
}

const lv_style_t* LvStyle::borrow_raw() const noexcept {
    return raw_.get();
}

bool LvStyle::empty() const noexcept {
    return raw_ == nullptr;
}

void LvStyle::reset() noexcept {
    if (raw_ != nullptr) {
        lv_style_reset(raw_.get());
        raw_.reset();
    }
}

void LvStyle::copy_from(StyleView source) noexcept {
    if (raw_ != nullptr && !source.empty()) {
        lv_style_copy(raw_.get(), source.borrow_raw());
    }
}

void LvStyle::merge_from(StyleView source) noexcept {
    if (raw_ != nullptr && !source.empty()) {
        lv_style_merge(raw_.get(), source.borrow_raw());
    }
}

void LvStyle::set_prop(lv_style_prop_t prop, lv_style_value_t value) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_prop(raw_.get(), prop, value);
    }
}

lv_style_res_t LvStyle::get_prop(lv_style_prop_t prop,
                                 lv_style_value_t& value) const noexcept {
    return borrow().get_prop(prop, value);
}

bool LvStyle::remove_prop(lv_style_prop_t prop) noexcept {
    return raw_ != nullptr && lv_style_remove_prop(raw_.get(), prop);
}

void LvStyle::set_bg_color(core::Color color) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_bg_color(raw_.get(), to_lv(color));
    }
}

void LvStyle::set_bg_opa(std::uint8_t opa) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_bg_opa(raw_.get(), opa);
    }
}

void LvStyle::set_border_color(core::Color color) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_border_color(raw_.get(), to_lv(color));
    }
}

void LvStyle::set_border_opa(std::uint8_t opa) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_border_opa(raw_.get(), opa);
    }
}

void LvStyle::set_border_width(std::int32_t width) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_border_width(raw_.get(), width);
    }
}

void LvStyle::set_radius(std::int32_t radius) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_radius(raw_.get(), radius);
    }
}

void LvStyle::set_text_color(core::Color color) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_text_color(raw_.get(), to_lv(color));
    }
}

void LvStyle::set_text_opa(std::uint8_t opa) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_text_opa(raw_.get(), opa);
    }
}

void LvStyle::set_text_font(const lv_font_t* font) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_text_font(raw_.get(), font);
    }
}

void LvStyle::set_pad_all(std::int32_t value) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_pad_all(raw_.get(), value);
    }
}

void LvStyle::set_margin_all(std::int32_t value) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_margin_all(raw_.get(), value);
    }
}

void LvStyle::set_size(std::int32_t width, std::int32_t height) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_size(raw_.get(), width, height);
    }
}

void LvStyle::set_transition(const StyleTransition& transition) noexcept {
    if (raw_ != nullptr) {
        lv_style_set_transition(raw_.get(), transition.borrow_raw());
    }
}

StyleTransition::StyleTransition(std::span<const lv_style_prop_t> props,
                                 lv_anim_path_cb_t path_callback,
                                 std::uint32_t duration_ms,
                                 std::uint32_t delay_ms,
                                 void* user_data)
    : props_{props.begin(), props.end()},
      path_callback_{path_callback},
      duration_ms_{duration_ms},
      delay_ms_{delay_ms},
      user_data_{user_data} {
    if (props_.empty() || props_.back() != LV_STYLE_PROP_INV) {
        props_.push_back(LV_STYLE_PROP_INV);
    }
    refresh_descriptor();
}

StyleTransition::StyleTransition(StyleTransition&& other) noexcept
    : props_{std::move(other.props_)},
      path_callback_{other.path_callback_},
      duration_ms_{other.duration_ms_},
      delay_ms_{other.delay_ms_},
      user_data_{other.user_data_} {
    refresh_descriptor();
    other.refresh_descriptor();
}

StyleTransition& StyleTransition::operator=(StyleTransition&& other) noexcept {
    if (this != &other) {
        props_ = std::move(other.props_);
        path_callback_ = other.path_callback_;
        duration_ms_ = other.duration_ms_;
        delay_ms_ = other.delay_ms_;
        user_data_ = other.user_data_;
        refresh_descriptor();
        other.refresh_descriptor();
    }
    return *this;
}

void StyleTransition::refresh_descriptor() noexcept {
    lv_style_transition_dsc_init(&raw_,
                                 props_.empty() ? nullptr : props_.data(),
                                 path_callback_,
                                 duration_ms_,
                                 delay_ms_,
                                 user_data_);
}

lv_style_value_t style_prop_default(lv_style_prop_t prop) noexcept {
    return lv_style_prop_get_default(prop);
}

std::uint8_t style_prop_flags(lv_style_prop_t prop) noexcept {
    return lv_style_prop_lookup_flags(prop);
}

bool style_prop_has_flag(lv_style_prop_t prop, std::uint8_t flag) noexcept {
    return (style_prop_flags(prop) & flag) != 0;
}

void add_style(ObjectView object, StyleView style, StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        if (!style.empty()) {
            lv_obj_add_style(raw, style.borrow_raw(), to_lv(selector));
        }
    }
}

bool replace_style(ObjectView object,
                   StyleView old_style,
                   StyleView new_style,
                   StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        if (!old_style.empty() && !new_style.empty()) {
            return lv_obj_replace_style(
                raw, old_style.borrow_raw(), new_style.borrow_raw(), to_lv(selector));
        }
    }
    return false;
}

void remove_style(ObjectView object,
                  StyleView style,
                  StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        if (!style.empty()) {
            lv_obj_remove_style(raw, style.borrow_raw(), to_lv(selector));
        }
    }
}

void remove_styles(ObjectView object, StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_remove_style(raw, nullptr, to_lv(selector));
    }
}

void remove_theme_styles(ObjectView object, StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_remove_theme(raw, to_lv(selector));
    }
}

void remove_all_styles(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_remove_style_all(raw);
    }
}

void set_style_disabled(ObjectView object,
                        StyleView style,
                        StyleSelector selector,
                        bool disabled) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        if (!style.empty()) {
            lv_obj_style_set_disabled(raw, style.borrow_raw(), to_lv(selector), disabled);
        }
    }
}

bool style_disabled(ObjectView object,
                    StyleView style,
                    StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        if (!style.empty()) {
            return lv_obj_style_get_disabled(raw, style.borrow_raw(), to_lv(selector));
        }
    }
    return false;
}

void set_local_style_prop(ObjectView object,
                          lv_style_prop_t prop,
                          lv_style_value_t value,
                          StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_set_local_style_prop(raw, prop, value, to_lv(selector));
    }
}

lv_style_res_t local_style_prop(ObjectView object,
                                lv_style_prop_t prop,
                                lv_style_value_t& value,
                                StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_local_style_prop(raw, prop, &value, to_lv(selector));
    }
    return LV_STYLE_RES_NOT_FOUND;
}

bool remove_local_style_prop(ObjectView object,
                             lv_style_prop_t prop,
                             StyleSelector selector) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_remove_local_style_prop(raw, prop, to_lv(selector));
    }
    return false;
}

lv_style_value_t resolved_style_prop(ObjectView object,
                                     StylePart part,
                                     lv_style_prop_t prop) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_style_prop(raw, to_lv(part), prop);
    }
    return style_prop_default(prop);
}

bool has_style_prop(ObjectView object,
                    StyleSelector selector,
                    lv_style_prop_t prop) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_has_style_prop(raw, to_lv(selector), prop);
    }
    return false;
}

core::Color resolved_bg_color(ObjectView object, StylePart part) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return color_from_lv(lv_obj_get_style_bg_color(raw, to_lv(part)));
    }
    return color_from_lv(style_prop_default(LV_STYLE_BG_COLOR).color);
}

std::uint8_t resolved_bg_opa(ObjectView object, StylePart part) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_style_bg_opa(raw, to_lv(part));
    }
    return static_cast<std::uint8_t>(style_prop_default(LV_STYLE_BG_OPA).num);
}

core::Color resolved_text_color(ObjectView object, StylePart part) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return color_from_lv(lv_obj_get_style_text_color(raw, to_lv(part)));
    }
    return color_from_lv(style_prop_default(LV_STYLE_TEXT_COLOR).color);
}

std::int32_t resolved_radius(ObjectView object, StylePart part) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_obj_get_style_radius(raw, to_lv(part));
    }
    return style_prop_default(LV_STYLE_RADIUS).num;
}

void report_style_change(StyleView style) noexcept {
    lv_obj_report_style_change(const_cast<lv_style_t*>(style.borrow_raw()));
}

void refresh_style(ObjectView object,
                   StylePart part,
                   lv_style_prop_t prop) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_obj_refresh_style(raw, to_lv(part), prop);
    }
}

void set_style_refresh_enabled(bool enabled) noexcept {
    lv_obj_enable_style_refresh(enabled);
}

void ThemeView::set_parent(ThemeView parent) const noexcept {
    if (raw_ != nullptr) {
        lv_theme_set_parent(raw_, parent.borrow_raw());
    }
}

void ThemeView::set_apply_callback(lv_theme_apply_cb_t callback) const noexcept {
    if (raw_ != nullptr) {
        lv_theme_set_apply_cb(raw_, callback);
    }
}

LvTheme LvTheme::make() noexcept {
    return LvTheme{lv_theme_create()};
}

LvTheme::LvTheme(lv_theme_t* raw) noexcept : raw_{raw} {}

LvTheme::LvTheme(LvTheme&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvTheme& LvTheme::operator=(LvTheme&& other) noexcept {
    if (this != &other) {
        reset();
        raw_ = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvTheme::~LvTheme() {
    reset();
}

ThemeView LvTheme::borrow() const noexcept {
    return ThemeView{raw_};
}

lv_theme_t* LvTheme::borrow_raw() const noexcept {
    return raw_;
}

bool LvTheme::empty() const noexcept {
    return raw_ == nullptr;
}

lv_theme_t* LvTheme::release() noexcept {
    lv_theme_t* released = raw_;
    raw_ = nullptr;
    return released;
}

void LvTheme::reset() noexcept {
    if (raw_ != nullptr) {
        lv_theme_delete(raw_);
        raw_ = nullptr;
    }
}

void LvTheme::set_parent(ThemeView parent) noexcept {
    borrow().set_parent(parent);
}

void LvTheme::set_apply_callback(lv_theme_apply_cb_t callback) noexcept {
    borrow().set_apply_callback(callback);
}

ThemeView theme_from_object(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return ThemeView{lv_theme_get_from_obj(raw)};
    }
    return ThemeView{nullptr};
}

ThemeView display_theme(DisplayView display) noexcept {
    if (lv_display_t* raw = raw_or_null(display)) {
        return ThemeView{lv_display_get_theme(raw)};
    }
    return ThemeView{nullptr};
}

void set_display_theme(DisplayView display, ThemeView theme) noexcept {
    if (lv_display_t* raw = raw_or_null(display)) {
        lv_display_set_theme(raw, theme.borrow_raw());
    }
}

void apply_theme(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        lv_theme_apply(raw);
    }
}

const lv_font_t* theme_font_small(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_theme_get_font_small(raw);
    }
    return nullptr;
}

const lv_font_t* theme_font_normal(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_theme_get_font_normal(raw);
    }
    return nullptr;
}

const lv_font_t* theme_font_large(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return lv_theme_get_font_large(raw);
    }
    return nullptr;
}

core::Color theme_color_primary(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return color_from_lv(lv_theme_get_color_primary(raw));
    }
    return core::Color{};
}

core::Color theme_color_secondary(ObjectView object) noexcept {
    if (lv_obj_t* raw = raw_or_null(object)) {
        return color_from_lv(lv_theme_get_color_secondary(raw));
    }
    return core::Color{};
}

#if LV_USE_THEME_DEFAULT
ThemeView default_theme_init(DisplayView display,
                             core::Color primary,
                             core::Color secondary,
                             bool dark,
                             const lv_font_t* font) noexcept {
    return ThemeView{lv_theme_default_init(
        raw_or_null(display), to_lv(primary), to_lv(secondary), dark, font)};
}

ThemeView default_theme() noexcept {
    return ThemeView{lv_theme_default_get()};
}

void default_theme_deinit() noexcept {
    lv_theme_default_deinit();
}
#endif

#if LV_USE_THEME_SIMPLE
ThemeView simple_theme_init(DisplayView display) noexcept {
    return ThemeView{lv_theme_simple_init(raw_or_null(display))};
}

ThemeView simple_theme() noexcept {
    return ThemeView{lv_theme_simple_get()};
}

void simple_theme_deinit() noexcept {
    lv_theme_simple_deinit();
}
#endif

#if LV_USE_THEME_MONO
ThemeView mono_theme_init(DisplayView display,
                          bool dark_background,
                          const lv_font_t* font) noexcept {
    return ThemeView{lv_theme_mono_init(raw_or_null(display), dark_background, font)};
}

ThemeView mono_theme() noexcept {
    return ThemeView{lv_theme_mono_get()};
}

void mono_theme_deinit() noexcept {
    lv_theme_mono_deinit();
}
#endif

}  // namespace lvglpp
