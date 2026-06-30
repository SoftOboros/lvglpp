// style_lvgl_test.cpp - LPAR-CPP-07 acceptance for LVGL style wrappers.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style.hpp"
#include "lvglpp/core/style_lvgl.hpp"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

void drive_lvgl(std::uint32_t ms) {
    lv_tick_inc(ms);
    static_cast<void>(lvglpp::run_timers());
}

struct DisplayFixture {
    std::array<std::uint8_t, 80 * 80 * 4> draw_buffer{};
    lvglpp::LvDisplay display;

    DisplayFixture() : display{lvglpp::LvDisplay::make(80, 80)} {
        assert(!display.empty());
        display.set_default();
        display.set_buffers(draw_buffer.data(),
                            nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
    }
};

lvglpp::core::Color red() {
    return lvglpp::core::Color{200, 10, 20, 255};
}

lvglpp::core::Color blue() {
    return lvglpp::core::Color{10, 20, 200, 255};
}

void test_selector_mapping() {
    static_assert(lvglpp::to_lv(lvglpp::StylePart::Main) == LV_PART_MAIN);
    static_assert(lvglpp::to_lv(lvglpp::StylePart::Scrollbar) ==
                  LV_PART_SCROLLBAR);
    static_assert(lvglpp::to_lv(lvglpp::StyleState::Pressed) ==
                  LV_STATE_PRESSED);
    static_assert(lvglpp::to_lv(lvglpp::StyleState::Focused |
                                lvglpp::StyleState::Pressed) ==
                  static_cast<lv_state_t>(LV_STATE_FOCUSED | LV_STATE_PRESSED));

    const lvglpp::StyleSelector selector{
        lvglpp::StylePart::Knob,
        lvglpp::StyleState::Pressed | lvglpp::StyleState::Checked};
    assert(lvglpp::to_lv(selector) ==
           static_cast<lv_style_selector_t>(LV_PART_KNOB |
                                            LV_STATE_PRESSED |
                                            LV_STATE_CHECKED));
    assert(selector.part() == lvglpp::StylePart::Knob);
    assert(selector.state() ==
           (lvglpp::StyleState::Pressed | lvglpp::StyleState::Checked));
}

void test_style_owner_property_helpers_and_copy() {
    lvglpp::LvStyle style;
    assert(!style.empty());
    style.set_bg_color(red());
    style.set_bg_opa(128);
    style.set_border_width(3);
    style.set_radius(7);
    style.set_text_color(blue());
    style.set_pad_all(4);
    style.set_margin_all(5);
    style.set_size(20, 30);

    lv_style_value_t value{};
    assert(style.get_prop(LV_STYLE_BG_COLOR, value) == LV_STYLE_RES_FOUND);
    assert(lvglpp::color_from_lv(value.color) == red());
    assert(style.get_prop(LV_STYLE_BG_OPA, value) == LV_STYLE_RES_FOUND);
    assert(value.num == 128);
    assert(style.get_prop(LV_STYLE_RADIUS, value) == LV_STYLE_RES_FOUND);
    assert(value.num == 7);
    assert(lvglpp::style_prop_has_flag(LV_STYLE_TEXT_COLOR,
                                       LV_STYLE_PROP_FLAG_INHERITABLE));

    lvglpp::LvStyle copied;
    copied.copy_from(style.borrow());
    assert(copied.get_prop(LV_STYLE_BORDER_WIDTH, value) == LV_STYLE_RES_FOUND);
    assert(value.num == 3);
    assert(copied.remove_prop(LV_STYLE_BORDER_WIDTH));
    assert(copied.get_prop(LV_STYLE_BORDER_WIDTH, value) ==
           LV_STYLE_RES_NOT_FOUND);

    lvglpp::LvStyle merged;
    merged.merge_from(style.borrow());
    assert(merged.get_prop(LV_STYLE_TEXT_COLOR, value) == LV_STYLE_RES_FOUND);
    assert(lvglpp::color_from_lv(value.color) == blue());
}

void test_object_style_stack_and_resolved_properties() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    lv_screen_load(screen.borrow_raw());
    auto object = lvglpp::LvObject::make_child(screen.borrow());
    lvglpp::remove_all_styles(object.borrow());

    lvglpp::LvStyle base;
    base.set_bg_color(red());
    base.set_bg_opa(255);
    base.set_radius(2);
    lvglpp::add_style(object.borrow(), base.borrow(), lvglpp::StyleSelector{});

    assert(lvglpp::resolved_bg_color(object.borrow(), lvglpp::StylePart::Main) ==
           red());
    assert(lvglpp::resolved_bg_opa(object.borrow(), lvglpp::StylePart::Main) ==
           255);
    assert(lvglpp::resolved_radius(object.borrow(), lvglpp::StylePart::Main) ==
           2);
    assert(lvglpp::has_style_prop(object.borrow(),
                                  lvglpp::StyleSelector{},
                                  LV_STYLE_BG_COLOR));

    lvglpp::LvStyle pressed;
    pressed.set_bg_color(blue());
    lvglpp::add_style(
        object.borrow(),
        pressed.borrow(),
        lvglpp::StyleSelector{lvglpp::StylePart::Main,
                              lvglpp::StyleState::Pressed});
    object.add_state(lvglpp::ObjectState::Pressed);
    assert(lvglpp::resolved_bg_color(object.borrow(), lvglpp::StylePart::Main) ==
           blue());

    lvglpp::set_style_disabled(
        object.borrow(),
        pressed.borrow(),
        lvglpp::StyleSelector{lvglpp::StylePart::Main,
                              lvglpp::StyleState::Pressed},
        true);
    assert(lvglpp::style_disabled(
        object.borrow(),
        pressed.borrow(),
        lvglpp::StyleSelector{lvglpp::StylePart::Main,
                              lvglpp::StyleState::Pressed}));
    assert(lvglpp::resolved_bg_color(object.borrow(), lvglpp::StylePart::Main) ==
           red());

    lvglpp::remove_style(
        object.borrow(),
        base.borrow(),
        lvglpp::StyleSelector{lvglpp::StylePart::Any, lvglpp::StyleState::Any});
    assert(!lvglpp::has_style_prop(object.borrow(),
                                   lvglpp::StyleSelector{},
                                   LV_STYLE_BG_COLOR));
}

void test_local_style_props_refresh_and_transition_descriptor() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    lv_screen_load(screen.borrow_raw());
    auto object = lvglpp::LvObject::make_child(screen.borrow());

    lv_style_value_t value{};
    value.color = lvglpp::to_lv(red());
    lvglpp::set_local_style_prop(object.borrow(),
                                 LV_STYLE_BG_COLOR,
                                 value,
                                 lvglpp::StyleSelector{});
    lv_style_value_t found{};
    assert(lvglpp::local_style_prop(object.borrow(),
                                    LV_STYLE_BG_COLOR,
                                    found,
                                    lvglpp::StyleSelector{}) ==
           LV_STYLE_RES_FOUND);
    assert(lvglpp::color_from_lv(found.color) == red());
    assert(lvglpp::resolved_style_prop(
               object.borrow(), lvglpp::StylePart::Main, LV_STYLE_BG_COLOR)
               .color.red == red().r);

    assert(lvglpp::remove_local_style_prop(
        object.borrow(), LV_STYLE_BG_COLOR, lvglpp::StyleSelector{}));

    const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, LV_STYLE_BG_OPA};
    int transition_user_data = 7;
    lvglpp::StyleTransition transition{
        std::span<const lv_style_prop_t>{props}, nullptr, 40, 5,
        &transition_user_data};
    lvglpp::LvStyle style;
    style.set_transition(transition);
    assert(style.get_prop(LV_STYLE_TRANSITION, found) == LV_STYLE_RES_FOUND);
    assert(found.ptr == transition.borrow_raw());

    lvglpp::add_style(object.borrow(), style.borrow(), lvglpp::StyleSelector{});
    lvglpp::refresh_style(object.borrow(), lvglpp::StylePart::Main);
    lvglpp::report_style_change(style.borrow());
    drive_lvgl(50);
}

int theme_apply_count = 0;

void theme_apply_callback(lv_theme_t*, lv_obj_t* object) {
    ++theme_apply_count;
    lv_obj_set_style_bg_color(object, lvglpp::to_lv(blue()), LV_PART_MAIN);
}

void test_theme_owner_view_and_apply() {
    DisplayFixture display;
    auto screen = lvglpp::LvObject::make_screen();
    lv_screen_load(screen.borrow_raw());
    auto object = lvglpp::LvObject::make_child(screen.borrow());

    auto theme = lvglpp::LvTheme::make();
    assert(!theme.empty());
    theme.set_apply_callback(theme_apply_callback);
    lvglpp::set_display_theme(display.display.borrow(), theme.borrow());
    assert(lvglpp::display_theme(display.display.borrow()).borrow_raw() ==
           theme.borrow_raw());
    assert(lvglpp::theme_from_object(object.borrow()).borrow_raw() ==
           theme.borrow_raw());

    theme_apply_count = 0;
    lvglpp::apply_theme(object.borrow());
    assert(theme_apply_count == 1);
    assert(lvglpp::resolved_bg_color(object.borrow(), lvglpp::StylePart::Main) ==
           blue());

    lvglpp::set_display_theme(display.display.borrow(), lvglpp::ThemeView{nullptr});
}

void test_compatibility_style_still_compiles() {
    lvglpp::core::Style style{};
    lvglpp::core::LightTheme light;
    light.apply(style);
    assert(style.bg_color == (lvglpp::core::Color{255, 255, 255, 255}));
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;

    test_selector_mapping();
    test_style_owner_property_helpers_and_copy();
    test_object_style_stack_and_resolved_properties();
    test_local_style_props_refresh_and_transition_descriptor();
    test_theme_owner_view_and_apply();
    test_compatibility_style_still_compiles();

    return 0;
}
