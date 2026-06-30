// draw_lvgl.cpp - LVGL-backed text, font, image, and draw wrapper implementation.
//
// PARITY: rlvgl/docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md
//         (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/font/lv_font.c, lvgl/src/widgets/label/lv_label.c,
//         lvgl/src/widgets/image/lv_image.c, and public draw descriptor APIs.
// DELTA:  delegates all runtime behavior to LVGL public APIs.

#include "lvglpp/core/draw_lvgl.hpp"

namespace lvglpp {

namespace {

[[nodiscard]] bool is_live(lv_obj_t* raw) noexcept {
    return raw != nullptr && lv_obj_is_valid(raw);
}

[[nodiscard]] std::uint32_t normalized_stride(std::uint16_t width,
                                              lv_color_format_t color_format,
                                              std::uint32_t stride) noexcept {
    if (stride != 0U) {
        return stride;
    }
    return lv_draw_buf_width_to_stride(width, color_format);
}

[[nodiscard]] lv_image_dsc_t make_descriptor(std::uint16_t width,
                                             std::uint16_t height,
                                             lv_color_format_t color_format,
                                             std::span<const std::uint8_t> data,
                                             std::uint32_t stride,
                                             std::uint16_t flags) noexcept {
    lv_image_dsc_t descriptor{};
    descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
    descriptor.header.cf = static_cast<std::uint8_t>(color_format);
    descriptor.header.flags = flags;
    descriptor.header.w = width;
    descriptor.header.h = height;
    descriptor.header.stride = normalized_stride(width, color_format, stride);
    descriptor.data_size = static_cast<std::uint32_t>(data.size());
    descriptor.data = data.data();
    return descriptor;
}

}  // namespace

LvFontView LvFontView::default_font() noexcept {
    return LvFontView{lv_font_get_default()};
}

std::int32_t LvFontView::line_height() const noexcept {
    if (raw_ == nullptr) {
        return 0;
    }
    return lv_font_get_line_height(raw_);
}

std::uint16_t LvFontView::glyph_width(char32_t letter,
                                      char32_t next) const noexcept {
    if (raw_ == nullptr) {
        return 0;
    }
    return lv_font_get_glyph_width(raw_,
                                   static_cast<std::uint32_t>(letter),
                                   static_cast<std::uint32_t>(next));
}

bool LvFontView::glyph_descriptor(lv_font_glyph_dsc_t& out,
                                  char32_t letter,
                                  char32_t next) const noexcept {
    if (raw_ == nullptr) {
        out = lv_font_glyph_dsc_t{};
        return false;
    }
    return lv_font_get_glyph_dsc(raw_,
                                 &out,
                                 static_cast<std::uint32_t>(letter),
                                 static_cast<std::uint32_t>(next));
}

GlyphBitmapView::GlyphBitmapView(GlyphBitmapView&& other) noexcept
    : descriptor_{other.descriptor_},
      raw_{other.raw_},
      release_required_{other.release_required_} {
    other.raw_ = nullptr;
    other.release_required_ = false;
    other.descriptor_ = lv_font_glyph_dsc_t{};
}

GlyphBitmapView& GlyphBitmapView::operator=(GlyphBitmapView&& other) noexcept {
    if (this != &other) {
        reset();
        descriptor_ = other.descriptor_;
        raw_ = other.raw_;
        release_required_ = other.release_required_;
        other.raw_ = nullptr;
        other.release_required_ = false;
        other.descriptor_ = lv_font_glyph_dsc_t{};
    }
    return *this;
}

GlyphBitmapView::~GlyphBitmapView() {
    reset();
}

GlyphBitmapView GlyphBitmapView::make(LvFontView font,
                                      char32_t letter,
                                      char32_t next,
                                      lv_draw_buf_t* scratch) noexcept {
    lv_font_glyph_dsc_t descriptor{};
    if (!font.glyph_descriptor(descriptor, letter, next)) {
        return GlyphBitmapView{};
    }
    const void* raw = lv_font_get_glyph_bitmap(&descriptor, scratch);
    return GlyphBitmapView{descriptor, raw, raw != nullptr};
}

GlyphBitmapView GlyphBitmapView::make_static(LvFontView font,
                                             char32_t letter,
                                             char32_t next) noexcept {
    lv_font_glyph_dsc_t descriptor{};
    if (!font.glyph_descriptor(descriptor, letter, next)) {
        return GlyphBitmapView{};
    }
    const void* raw = lv_font_get_glyph_static_bitmap(&descriptor);
    return GlyphBitmapView{descriptor, raw, false};
}

void GlyphBitmapView::reset() noexcept {
    if (release_required_) {
        lv_font_glyph_release_draw_data(&descriptor_);
    }
    descriptor_ = lv_font_glyph_dsc_t{};
    raw_ = nullptr;
    release_required_ = false;
}

GlyphBitmapView::GlyphBitmapView(lv_font_glyph_dsc_t descriptor,
                                 const void* raw,
                                 bool release_required) noexcept
    : descriptor_{descriptor}, raw_{raw}, release_required_{release_required} {}

LvLabel LvLabel::make(ObjectView parent) noexcept {
    if (parent.empty()) {
        return LvLabel{};
    }
    return LvLabel{lv_label_create(parent.borrow_raw())};
}

LvLabel::LvLabel(LvLabel&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvLabel& LvLabel::operator=(LvLabel&& other) noexcept {
    if (this != &other) {
        reset();
        raw_ = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvLabel::~LvLabel() {
    reset();
}

ObjectView LvLabel::borrow() const noexcept {
    return ObjectView{raw_};
}

bool LvLabel::valid() const noexcept {
    return is_live(raw_);
}

lv_obj_t* LvLabel::release() noexcept {
    lv_obj_t* released = raw_;
    raw_ = nullptr;
    return released;
}

void LvLabel::reset() noexcept {
    if (is_live(raw_)) {
        lv_obj_delete(raw_);
    }
    raw_ = nullptr;
}

void LvLabel::set_text(const char* text) noexcept {
    if (is_live(raw_)) {
        lv_label_set_text(raw_, text);
    }
}

void LvLabel::set_text_static(const char* text) noexcept {
    if (is_live(raw_)) {
        lv_label_set_text_static(raw_, text);
    }
}

const char* LvLabel::text() const noexcept {
    if (!is_live(raw_)) {
        return "";
    }
    return lv_label_get_text(raw_);
}

void LvLabel::set_long_mode(LabelLongMode mode) noexcept {
    if (is_live(raw_)) {
        lv_label_set_long_mode(raw_, to_lv(mode));
    }
}

LabelLongMode LvLabel::long_mode() const noexcept {
    if (!is_live(raw_)) {
        return LabelLongMode::Wrap;
    }
    return label_long_mode_from_lv(lv_label_get_long_mode(raw_));
}

void LvLabel::set_recolor(bool enabled) noexcept {
    if (is_live(raw_)) {
        lv_label_set_recolor(raw_, enabled);
    }
}

bool LvLabel::recolor() const noexcept {
    return is_live(raw_) && lv_label_get_recolor(raw_);
}

void LvLabel::set_text_selection_start(std::uint32_t index) noexcept {
    if (is_live(raw_)) {
        lv_label_set_text_selection_start(raw_, index);
    }
}

void LvLabel::set_text_selection_end(std::uint32_t index) noexcept {
    if (is_live(raw_)) {
        lv_label_set_text_selection_end(raw_, index);
    }
}

std::uint32_t LvLabel::text_selection_start() const noexcept {
    if (!is_live(raw_)) {
        return LV_LABEL_TEXT_SELECTION_OFF;
    }
    return lv_label_get_text_selection_start(raw_);
}

std::uint32_t LvLabel::text_selection_end() const noexcept {
    if (!is_live(raw_)) {
        return LV_LABEL_TEXT_SELECTION_OFF;
    }
    return lv_label_get_text_selection_end(raw_);
}

lv_point_t LvLabel::letter_pos(std::uint32_t char_id) const noexcept {
    lv_point_t point{};
    if (is_live(raw_)) {
        lv_label_get_letter_pos(raw_, char_id, &point);
    }
    return point;
}

std::uint32_t LvLabel::letter_on(lv_point_t point, bool bidi) const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_label_get_letter_on(raw_, &point, bidi);
}

bool LvLabel::char_under_pos(lv_point_t point) const noexcept {
    if (!is_live(raw_)) {
        return false;
    }
    return lv_label_is_char_under_pos(raw_, &point);
}

LvImageDescriptor LvImageDescriptor::borrowed(
    std::uint16_t width,
    std::uint16_t height,
    lv_color_format_t color_format,
    std::span<const std::uint8_t> data,
    std::uint32_t stride,
    std::uint16_t flags) noexcept {
    LvImageDescriptor descriptor;
    descriptor.raw_ = make_descriptor(width, height, color_format, data, stride, flags);
    return descriptor;
}

LvImageDescriptor LvImageDescriptor::owned(std::uint16_t width,
                                           std::uint16_t height,
                                           lv_color_format_t color_format,
                                           std::vector<std::uint8_t> data,
                                           std::uint32_t stride,
                                           std::uint16_t flags) {
    LvImageDescriptor descriptor;
    descriptor.owned_data_ = std::move(data);
    descriptor.raw_ = make_descriptor(width,
                                      height,
                                      color_format,
                                      descriptor.owned_data_,
                                      stride,
                                      flags);
    return descriptor;
}

LvImageDescriptor::LvImageDescriptor(LvImageDescriptor&& other) noexcept
    : raw_{other.raw_}, owned_data_{std::move(other.owned_data_)} {
    point_to_owned_data();
    other.raw_ = lv_image_dsc_t{};
}

LvImageDescriptor& LvImageDescriptor::operator=(
    LvImageDescriptor&& other) noexcept {
    if (this != &other) {
        raw_ = other.raw_;
        owned_data_ = std::move(other.owned_data_);
        point_to_owned_data();
        other.raw_ = lv_image_dsc_t{};
    }
    return *this;
}

ImageDescriptorView LvImageDescriptor::borrow() const noexcept {
    return ImageDescriptorView{&raw_};
}

void LvImageDescriptor::point_to_owned_data() noexcept {
    if (!owned_data_.empty()) {
        raw_.data = owned_data_.data();
        raw_.data_size = static_cast<std::uint32_t>(owned_data_.size());
    }
}

lv_image_src_t image_source_type(const void* source) noexcept {
    return lv_image_src_get_type(source);
}

lv_image_src_t image_source_type(ImageDescriptorView descriptor) noexcept {
    return image_source_type(descriptor.borrow_raw());
}

LvImage LvImage::make(ObjectView parent) noexcept {
    if (parent.empty()) {
        return LvImage{};
    }
    return LvImage{lv_image_create(parent.borrow_raw())};
}

LvImage::LvImage(LvImage&& other) noexcept : raw_{other.raw_} {
    other.raw_ = nullptr;
}

LvImage& LvImage::operator=(LvImage&& other) noexcept {
    if (this != &other) {
        reset();
        raw_ = other.raw_;
        other.raw_ = nullptr;
    }
    return *this;
}

LvImage::~LvImage() {
    reset();
}

ObjectView LvImage::borrow() const noexcept {
    return ObjectView{raw_};
}

bool LvImage::valid() const noexcept {
    return is_live(raw_);
}

lv_obj_t* LvImage::release() noexcept {
    lv_obj_t* released = raw_;
    raw_ = nullptr;
    return released;
}

void LvImage::reset() noexcept {
    if (is_live(raw_)) {
        lv_obj_delete(raw_);
    }
    raw_ = nullptr;
}

void LvImage::set_source(ImageDescriptorView descriptor) noexcept {
    if (is_live(raw_)) {
        lv_image_set_src(raw_, descriptor.borrow_raw());
    }
}

void LvImage::set_source(const char* source) noexcept {
    if (is_live(raw_)) {
        lv_image_set_src(raw_, source);
    }
}

const void* LvImage::source() const noexcept {
    if (!is_live(raw_)) {
        return nullptr;
    }
    return lv_image_get_src(raw_);
}

lv_image_src_t LvImage::source_type() const noexcept {
    return image_source_type(source());
}

std::int32_t LvImage::source_width() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_src_width(raw_);
}

std::int32_t LvImage::source_height() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_src_height(raw_);
}

void LvImage::set_offset_x(std::int32_t x) noexcept {
    if (is_live(raw_)) {
        lv_image_set_offset_x(raw_, x);
    }
}

void LvImage::set_offset_y(std::int32_t y) noexcept {
    if (is_live(raw_)) {
        lv_image_set_offset_y(raw_, y);
    }
}

std::int32_t LvImage::offset_x() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_offset_x(raw_);
}

std::int32_t LvImage::offset_y() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_offset_y(raw_);
}

void LvImage::set_rotation(std::int32_t angle_tenths) noexcept {
    if (is_live(raw_)) {
        lv_image_set_rotation(raw_, angle_tenths);
    }
}

std::int32_t LvImage::rotation() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_rotation(raw_);
}

void LvImage::set_pivot(std::int32_t x, std::int32_t y) noexcept {
    if (is_live(raw_)) {
        lv_image_set_pivot(raw_, x, y);
    }
}

lv_point_t LvImage::pivot() const noexcept {
    lv_point_t point{};
    if (is_live(raw_)) {
        lv_image_get_pivot(raw_, &point);
    }
    return point;
}

void LvImage::set_scale(std::uint32_t scale) noexcept {
    if (is_live(raw_)) {
        lv_image_set_scale(raw_, scale);
    }
}

void LvImage::set_scale_x(std::uint32_t scale) noexcept {
    if (is_live(raw_)) {
        lv_image_set_scale_x(raw_, scale);
    }
}

void LvImage::set_scale_y(std::uint32_t scale) noexcept {
    if (is_live(raw_)) {
        lv_image_set_scale_y(raw_, scale);
    }
}

std::int32_t LvImage::scale() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_scale(raw_);
}

std::int32_t LvImage::scale_x() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_scale_x(raw_);
}

std::int32_t LvImage::scale_y() const noexcept {
    if (!is_live(raw_)) {
        return 0;
    }
    return lv_image_get_scale_y(raw_);
}

void LvImage::set_blend_mode(lv_blend_mode_t mode) noexcept {
    if (is_live(raw_)) {
        lv_image_set_blend_mode(raw_, mode);
    }
}

void LvImage::set_antialias(bool enabled) noexcept {
    if (is_live(raw_)) {
        lv_image_set_antialias(raw_, enabled);
    }
}

void LvImage::set_inner_align(ImageAlign align) noexcept {
    if (is_live(raw_)) {
        lv_image_set_inner_align(raw_, to_lv(align));
    }
}

ImageAlign LvImage::inner_align() const noexcept {
    if (!is_live(raw_)) {
        return ImageAlign::Default;
    }
    return image_align_from_lv(lv_image_get_inner_align(raw_));
}

void LvImage::set_bitmap_mask(ImageDescriptorView descriptor) noexcept {
    if (is_live(raw_)) {
        lv_image_set_bitmap_map_src(raw_, descriptor.borrow_raw());
    }
}

LvDrawLabelDescriptor::LvDrawLabelDescriptor() noexcept {
    lv_draw_label_dsc_init(&raw_);
}

LvDrawImageDescriptor::LvDrawImageDescriptor() noexcept {
    lv_draw_image_dsc_init(&raw_);
}

LvDrawFillDescriptor::LvDrawFillDescriptor() noexcept {
    lv_draw_fill_dsc_init(&raw_);
}

void LvDrawFillDescriptor::set_color(core::Color color) noexcept {
    raw_.color = to_lv(color);
}

void LvDrawFillDescriptor::set_opa(std::uint8_t opa) noexcept {
    raw_.opa = opa;
}

void LvDrawFillDescriptor::set_radius(std::int32_t radius) noexcept {
    raw_.radius = radius;
}

LvDrawBorderDescriptor::LvDrawBorderDescriptor() noexcept {
    lv_draw_border_dsc_init(&raw_);
}

LvDrawBoxShadowDescriptor::LvDrawBoxShadowDescriptor() noexcept {
    lv_draw_box_shadow_dsc_init(&raw_);
}

LvDrawRectDescriptor::LvDrawRectDescriptor() noexcept {
    lv_draw_rect_dsc_init(&raw_);
}

LvDrawMaskRectDescriptor::LvDrawMaskRectDescriptor() noexcept {
    lv_draw_mask_rect_dsc_init(&raw_);
}

void LvDrawMaskRectDescriptor::set_area(LvArea area) noexcept {
    raw_.area = area.to_lv();
}

void LvDrawMaskRectDescriptor::set_radius(std::int32_t radius) noexcept {
    raw_.radius = radius;
}

void LvDrawMaskRectDescriptor::set_keep_outside(bool keep_outside) noexcept {
    raw_.keep_outside = keep_outside ? 1U : 0U;
}

}  // namespace lvglpp
