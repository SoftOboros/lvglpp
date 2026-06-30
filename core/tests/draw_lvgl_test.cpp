// draw_lvgl_test.cpp - LPAR-CPP-08 acceptance for LVGL-backed content wrappers.

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/draw_lvgl.hpp"
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/core/style_lvgl.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

struct Recorder {
    std::vector<lvglpp::LvArea> flushed;
};

void on_flush(lv_display_t* display, const lv_area_t* area, std::uint8_t* /*px_map*/) {
    auto* recorder = static_cast<Recorder*>(lv_display_get_user_data(display));
    if (recorder != nullptr && area != nullptr) {
        recorder->flushed.push_back(lvglpp::LvArea::from_lv(*area));
    }
    lv_display_flush_ready(display);
}

struct Fixture {
    Recorder recorder;
    std::array<std::uint8_t, 96 * 64 * 4> draw_buffer{};
    lvglpp::LvDisplay display;

    Fixture() : display{lvglpp::LvDisplay::make(96, 64)} {
        display.set_default();
        lv_display_set_user_data(display.borrow_raw(), &recorder);
        display.set_buffers(draw_buffer.data(),
                            nullptr,
                            static_cast<std::uint32_t>(draw_buffer.size()),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
        display.set_flush_callback(on_flush);
    }

    [[nodiscard]] lvglpp::ObjectView active_screen() const noexcept {
        return lvglpp::ObjectView{lv_display_get_screen_active(display.borrow_raw())};
    }
};

std::array<std::uint8_t, 16> make_argb8888_pixels() {
    return std::array<std::uint8_t, 16>{
        0xFF, 0xFF, 0x00, 0x00,
        0xFF, 0x00, 0xFF, 0x00,
        0xFF, 0x00, 0x00, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
    };
}

void test_font_view_uses_lvgl_metrics() {
    const auto font = lvglpp::LvFontView::default_font();
    assert(!font.empty());
    assert(font.line_height() > 0);
    assert(font.glyph_width(U'A') > 0);

    lv_font_glyph_dsc_t descriptor{};
    assert(font.glyph_descriptor(descriptor, U'A'));
    assert(descriptor.adv_w > 0);

    auto static_bitmap = lvglpp::GlyphBitmapView::make_static(font, U'A');
    if (!static_bitmap.empty()) {
        assert(static_bitmap.descriptor().adv_w == descriptor.adv_w);
    }
}

void test_label_wrapper_state_and_geometry(Fixture& fixture) {
    auto label = lvglpp::LvLabel::make(fixture.active_screen());
    assert(label.valid());

    label.set_text("Hello");
    assert(std::strcmp(label.text(), "Hello") == 0);

    static const char kStaticText[] = "Static";
    label.set_text_static(kStaticText);
    assert(std::strcmp(label.text(), kStaticText) == 0);

    label.set_long_mode(lvglpp::LabelLongMode::Clip);
    assert(label.long_mode() == lvglpp::LabelLongMode::Clip);

    label.set_recolor(true);
    assert(label.recolor());

    label.set_text_selection_start(1);
    label.set_text_selection_end(3);
    assert(label.text_selection_start() == 1);
    assert(label.text_selection_end() == 3);

    lv_obj_set_pos(label.borrow_raw(), 4, 5);
    lv_obj_set_size(label.borrow_raw(), 60, 20);
    lv_obj_update_layout(label.borrow_raw());

    const lv_point_t first = label.letter_pos(0);
    assert(first.x >= 0);
    const auto idx = label.letter_on(lv_point_t{first.x, first.y}, false);
    assert(idx <= 1);

    fixture.recorder.flushed.clear();
    assert(lvglpp::invalidate(label.borrow()) == LV_RESULT_OK);
    lv_refr_now(fixture.display.borrow_raw());
    assert(!fixture.recorder.flushed.empty());
}

void test_image_descriptor_and_widget(Fixture& fixture) {
    auto pixels = make_argb8888_pixels();
    auto descriptor = lvglpp::LvImageDescriptor::borrowed(
        2, 2, LV_COLOR_FORMAT_ARGB8888, pixels);
    assert(!descriptor.owns_data());
    assert(descriptor.borrow_raw()->header.magic == LV_IMAGE_HEADER_MAGIC);
    assert(descriptor.borrow_raw()->header.w == 2);
    assert(descriptor.borrow_raw()->header.h == 2);
    assert(descriptor.borrow_raw()->header.stride ==
           lv_draw_buf_width_to_stride(2, LV_COLOR_FORMAT_ARGB8888));
    assert(lvglpp::image_source_type(descriptor.borrow()) == LV_IMAGE_SRC_VARIABLE);

    auto owned_pixels = make_argb8888_pixels();
    std::vector<std::uint8_t> owned_data(owned_pixels.begin(), owned_pixels.end());
    auto owned_descriptor = lvglpp::LvImageDescriptor::owned(
        2, 2, LV_COLOR_FORMAT_ARGB8888, std::move(owned_data));
    assert(owned_descriptor.owns_data());
    auto moved_descriptor = std::move(owned_descriptor);
    assert(moved_descriptor.borrow_raw()->data != nullptr);

    auto image = lvglpp::LvImage::make(fixture.active_screen());
    assert(image.valid());
    image.set_source(descriptor.borrow());
    assert(image.source() == descriptor.borrow_raw());
    assert(image.source_type() == LV_IMAGE_SRC_VARIABLE);
    assert(image.source_width() == 2);
    assert(image.source_height() == 2);

    image.set_offset_x(1);
    image.set_offset_y(2);
    assert(image.offset_x() == 1);
    assert(image.offset_y() == 2);

    image.set_pivot(1, 1);
    const lv_point_t pivot = image.pivot();
    assert(pivot.x == 1);
    assert(pivot.y == 1);

    image.set_scale(256);
    image.set_scale_x(512);
    image.set_scale_y(128);
    assert(image.scale() == 512);
    assert(image.scale_x() == 512);
    assert(image.scale_y() == 128);

    image.set_rotation(90);
    assert(image.rotation() == 90);
    image.set_blend_mode(LV_BLEND_MODE_NORMAL);
    image.set_antialias(true);
    image.set_inner_align(lvglpp::ImageAlign::Center);
    assert(image.inner_align() == lvglpp::ImageAlign::Center);
    image.set_bitmap_mask(moved_descriptor.borrow());

    lv_obj_set_pos(image.borrow_raw(), 18, 8);
    lv_obj_update_layout(image.borrow_raw());
    fixture.recorder.flushed.clear();
    assert(lvglpp::invalidate(image.borrow()) == LV_RESULT_OK);
    lv_refr_now(fixture.display.borrow_raw());
    assert(!fixture.recorder.flushed.empty());
}

void test_draw_descriptor_initializers() {
    lvglpp::LvDrawLabelDescriptor label;
    assert(label.borrow_raw()->opa == LV_OPA_COVER);

    lvglpp::LvDrawImageDescriptor image;
    assert(image.borrow_raw()->opa == LV_OPA_COVER);

    lvglpp::LvDrawFillDescriptor fill;
    fill.set_color(lvglpp::core::Color{1, 2, 3, 255});
    fill.set_opa(123);
    fill.set_radius(7);
    assert(fill.borrow_raw()->opa == 123);
    assert(fill.borrow_raw()->radius == 7);

    lvglpp::LvDrawBorderDescriptor border;
    assert(border.borrow_raw() != nullptr);

    lvglpp::LvDrawBoxShadowDescriptor shadow;
    assert(shadow.borrow_raw() != nullptr);

    lvglpp::LvDrawRectDescriptor rect;
    assert(rect.borrow_raw() != nullptr);

    lvglpp::LvDrawMaskRectDescriptor mask;
    mask.set_area(lvglpp::LvArea::from_xywh(1, 2, 3, 4));
    mask.set_radius(5);
    mask.set_keep_outside(true);
    assert(mask.borrow_raw()->area.x1 == 1);
    assert(mask.borrow_raw()->area.y1 == 2);
    assert(mask.borrow_raw()->area.x2 == 3);
    assert(mask.borrow_raw()->area.y2 == 5);
    assert(mask.borrow_raw()->radius == 5);
    assert(mask.borrow_raw()->keep_outside == 1U);
}

void test_move_and_release(Fixture& fixture) {
    auto label = lvglpp::LvLabel::make(fixture.active_screen());
    lv_obj_t* raw_label = label.borrow_raw();
    lvglpp::LvLabel moved_label{std::move(label)};
    assert(label.empty());
    assert(moved_label.borrow_raw() == raw_label);

    raw_label = moved_label.release();
    assert(moved_label.empty());
    assert(raw_label != nullptr);
    lv_obj_delete(raw_label);

    auto image = lvglpp::LvImage::make(fixture.active_screen());
    lv_obj_t* raw_image = image.borrow_raw();
    lvglpp::LvImage moved_image{std::move(image)};
    assert(image.empty());
    assert(moved_image.borrow_raw() == raw_image);

    raw_image = moved_image.release();
    assert(moved_image.empty());
    assert(raw_image != nullptr);
    lv_obj_delete(raw_image);
}

}  // namespace

int main() {
    lvglpp::Runtime runtime;
    Fixture fixture;

    test_font_view_uses_lvgl_metrics();
    test_label_wrapper_state_and_geometry(fixture);
    test_image_descriptor_and_widget(fixture);
    test_draw_descriptor_initializers();
    test_move_and_release(fixture);

    return 0;
}
