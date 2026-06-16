// draw_lvgl.hpp - LVGL-backed text, font, image, and draw wrappers.
//
// PARITY: rlvgl/docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md
//         (v0.2.5 @ f999f75).
// LVGL:   lvgl/src/font/lv_font.h, lvgl/src/widgets/label/lv_label.h,
//         lvgl/src/widgets/image/lv_image.h, lvgl/src/draw/lv_draw_label.h,
//         lvgl/src/draw/lv_draw_image.h, lvgl/src/draw/lv_draw_rect.h,
//         and lvgl/src/draw/lv_draw_mask.h.
// DELTA:  lvglpp delegates fonts, labels, image sources, draw descriptors,
//         and masks to LVGL public APIs instead of porting rlvgl's software
//         renderer and font traits.

#ifndef LVGLPP_CORE_DRAW_LVGL_HPP
#define LVGLPP_CORE_DRAW_LVGL_HPP

#include "lvglpp/core/display.hpp"
#include "lvglpp/core/object.hpp"
#include "lvglpp/core/style_lvgl.hpp"

#include <cstdint>
#include <span>
#include <vector>

extern "C" {
#include "src/draw/lv_draw_image.h"
#include "src/draw/lv_draw_label.h"
#include "src/draw/lv_draw_mask.h"
#include "src/draw/lv_draw_rect.h"
#include "src/widgets/image/lv_image.h"
#include "src/widgets/label/lv_label.h"
}

namespace lvglpp {

enum class BuiltinFont : std::uint8_t {
    Montserrat12,
    Montserrat14,
    Montserrat16,
    Montserrat18,
    Montserrat24,
    Montserrat28,
    Montserrat48,
};

struct GlyphMetrics {
    std::uint32_t advance_width = 0;
    std::uint32_t box_width = 0;
    std::uint32_t box_height = 0;
    std::int32_t offset_x = 0;
    std::int32_t offset_y = 0;

    [[nodiscard]] constexpr bool operator==(
        const GlyphMetrics&) const noexcept = default;
};

class LvFontView {
public:
    // Args:
    //   raw: observes external/static LVGL font storage. This view never
    //        deletes or mutates it.
    explicit LvFontView(const lv_font_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] static LvFontView default_font() noexcept;
    [[nodiscard]] static LvFontView builtin(BuiltinFont font) noexcept;

    [[nodiscard]] const lv_font_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool             empty() const noexcept { return raw_ == nullptr; }

    [[nodiscard]] std::int32_t line_height() const noexcept;
    [[nodiscard]] std::int32_t base_line() const noexcept;
    [[nodiscard]] std::uint16_t glyph_width(char32_t letter,
                                            char32_t next = 0) const noexcept;
    [[nodiscard]] GlyphMetrics glyph_metrics(char32_t letter,
                                             char32_t next = 0) const noexcept;
    [[nodiscard]] bool glyph_descriptor(lv_font_glyph_dsc_t& out,
                                        char32_t letter,
                                        char32_t next = 0) const noexcept;
    [[nodiscard]] bool is_anti_aliased() const noexcept;

private:
    // observes: LVGL/static font; never released by this view.
    const lv_font_t* raw_ = nullptr;
};

class GlyphBitmapView {
public:
    GlyphBitmapView() noexcept = default;

    GlyphBitmapView(const GlyphBitmapView&)            = delete;
    GlyphBitmapView& operator=(const GlyphBitmapView&) = delete;

    GlyphBitmapView(GlyphBitmapView&& other) noexcept;
    GlyphBitmapView& operator=(GlyphBitmapView&& other) noexcept;

    ~GlyphBitmapView();

    [[nodiscard]] static GlyphBitmapView make(LvFontView font,
                                              char32_t letter,
                                              char32_t next,
                                              lv_draw_buf_t* scratch) noexcept;
    [[nodiscard]] static GlyphBitmapView make_static(LvFontView font,
                                                     char32_t letter,
                                                     char32_t next = 0) noexcept;

    [[nodiscard]] const void* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] const lv_font_glyph_dsc_t& descriptor() const noexcept {
        return descriptor_;
    }
    [[nodiscard]] bool empty() const noexcept { return raw_ == nullptr; }

    void reset() noexcept;

private:
    GlyphBitmapView(lv_font_glyph_dsc_t descriptor,
                    const void* raw,
                    bool release_required) noexcept;

    // owns: descriptor value. It may contain an LVGL cache entry released
    // by lv_font_glyph_release_draw_data() when release_required_ is true.
    lv_font_glyph_dsc_t descriptor_{};
    // borrows: LVGL/font-owned glyph draw data; valid until reset/destructor
    // when release_required_ is true, otherwise static font storage.
    const void* raw_ = nullptr;
    bool release_required_ = false;
};

enum class LabelLongMode : std::uint8_t {
    Wrap = static_cast<std::uint8_t>(LV_LABEL_LONG_MODE_WRAP),
    Dots = static_cast<std::uint8_t>(LV_LABEL_LONG_MODE_DOTS),
    Scroll = static_cast<std::uint8_t>(LV_LABEL_LONG_MODE_SCROLL),
    ScrollCircular = static_cast<std::uint8_t>(LV_LABEL_LONG_MODE_SCROLL_CIRCULAR),
    Clip = static_cast<std::uint8_t>(LV_LABEL_LONG_MODE_CLIP),
};

[[nodiscard]] constexpr lv_label_long_mode_t to_lv(LabelLongMode mode) noexcept {
    return static_cast<lv_label_long_mode_t>(mode);
}

[[nodiscard]] constexpr LabelLongMode label_long_mode_from_lv(
    lv_label_long_mode_t mode) noexcept {
    return static_cast<LabelLongMode>(mode);
}

class LvLabel {
public:
    LvLabel() noexcept = default;

    [[nodiscard]] static LvLabel make(ObjectView parent) noexcept;

    LvLabel(const LvLabel&)            = delete;
    LvLabel& operator=(const LvLabel&) = delete;

    LvLabel(LvLabel&& other) noexcept;
    LvLabel& operator=(LvLabel&& other) noexcept;

    ~LvLabel();

    [[nodiscard]] ObjectView borrow() const noexcept;
    [[nodiscard]] lv_obj_t*  borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool       empty() const noexcept { return raw_ == nullptr; }
    [[nodiscard]] bool       valid() const noexcept;

    // Returns: owns raw LVGL label object; caller is responsible for deletion.
    [[nodiscard]] lv_obj_t* release() noexcept;
    void reset() noexcept;

    // Args:
    //   text: borrows a null-terminated string for the duration of the call.
    //         LVGL copies it into label-owned storage.
    void set_text(const char* text) noexcept;
    // Args:
    //   text: external; LVGL observes this null-terminated storage until the
    //         next text assignment or label deletion.
    void set_text_static(const char* text) noexcept;
    // Returns: borrows LVGL label storage; invalid after text mutation/deletion.
    [[nodiscard]] const char* text() const noexcept;

    void set_long_mode(LabelLongMode mode) noexcept;
    [[nodiscard]] LabelLongMode long_mode() const noexcept;

    void set_recolor(bool enabled) noexcept;
    [[nodiscard]] bool recolor() const noexcept;

    void set_text_selection_start(std::uint32_t index) noexcept;
    void set_text_selection_end(std::uint32_t index) noexcept;
    [[nodiscard]] std::uint32_t text_selection_start() const noexcept;
    [[nodiscard]] std::uint32_t text_selection_end() const noexcept;

    [[nodiscard]] lv_point_t letter_pos(std::uint32_t char_id) const noexcept;
    [[nodiscard]] std::uint32_t letter_on(lv_point_t point,
                                          bool bidi = false) const noexcept;
    [[nodiscard]] bool char_under_pos(lv_point_t point) const noexcept;

private:
    explicit LvLabel(lv_obj_t* raw) noexcept : raw_{raw} {}

    // owns: deleted with lv_obj_delete() when non-null and still valid.
    lv_obj_t* raw_ = nullptr;
};

enum class ImageAlign : std::uint8_t {
    Default = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_DEFAULT),
    TopLeft = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_TOP_LEFT),
    TopMid = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_TOP_MID),
    TopRight = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_TOP_RIGHT),
    BottomLeft = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_BOTTOM_LEFT),
    BottomMid = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_BOTTOM_MID),
    BottomRight = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_BOTTOM_RIGHT),
    LeftMid = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_LEFT_MID),
    RightMid = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_RIGHT_MID),
    Center = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_CENTER),
    Stretch = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_STRETCH),
    Tile = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_TILE),
    Contain = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_CONTAIN),
    Cover = static_cast<std::uint8_t>(LV_IMAGE_ALIGN_COVER),
};

[[nodiscard]] constexpr lv_image_align_t to_lv(ImageAlign align) noexcept {
    return static_cast<lv_image_align_t>(align);
}

[[nodiscard]] constexpr ImageAlign image_align_from_lv(
    lv_image_align_t align) noexcept {
    return static_cast<ImageAlign>(align);
}

class ImageDescriptorView {
public:
    // Args:
    //   raw: observes external LVGL image descriptor storage.
    explicit ImageDescriptorView(const lv_image_dsc_t* raw) noexcept : raw_{raw} {}

    [[nodiscard]] const lv_image_dsc_t* borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool empty() const noexcept { return raw_ == nullptr; }

private:
    // observes: descriptor owned outside this view; never freed here.
    const lv_image_dsc_t* raw_ = nullptr;
};

class LvImageDescriptor {
public:
    LvImageDescriptor() noexcept = default;

    [[nodiscard]] static LvImageDescriptor borrowed(
        std::uint16_t width,
        std::uint16_t height,
        lv_color_format_t color_format,
        std::span<const std::uint8_t> data,
        std::uint32_t stride = 0,
        std::uint16_t flags = 0) noexcept;

    [[nodiscard]] static LvImageDescriptor owned(
        std::uint16_t width,
        std::uint16_t height,
        lv_color_format_t color_format,
        std::vector<std::uint8_t> data,
        std::uint32_t stride = 0,
        std::uint16_t flags = 0);

    LvImageDescriptor(const LvImageDescriptor&)            = delete;
    LvImageDescriptor& operator=(const LvImageDescriptor&) = delete;

    LvImageDescriptor(LvImageDescriptor&& other) noexcept;
    LvImageDescriptor& operator=(LvImageDescriptor&& other) noexcept;

    [[nodiscard]] ImageDescriptorView borrow() const noexcept;
    [[nodiscard]] const lv_image_dsc_t* borrow_raw() const noexcept { return &raw_; }
    [[nodiscard]] lv_image_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] bool owns_data() const noexcept { return !owned_data_.empty(); }

private:
    void point_to_owned_data() noexcept;

    // owns: descriptor value. raw_.data borrows external bytes or points into
    // owned_data_ depending on constructor.
    lv_image_dsc_t raw_{};
    // owns: optional image bytes; stable after moves via point_to_owned_data().
    std::vector<std::uint8_t> owned_data_{};
};

[[nodiscard]] lv_image_src_t image_source_type(const void* source) noexcept;
[[nodiscard]] lv_image_src_t image_source_type(ImageDescriptorView descriptor) noexcept;

class LvImage {
public:
    LvImage() noexcept = default;

    [[nodiscard]] static LvImage make(ObjectView parent) noexcept;

    LvImage(const LvImage&)            = delete;
    LvImage& operator=(const LvImage&) = delete;

    LvImage(LvImage&& other) noexcept;
    LvImage& operator=(LvImage&& other) noexcept;

    ~LvImage();

    [[nodiscard]] ObjectView borrow() const noexcept;
    [[nodiscard]] lv_obj_t*  borrow_raw() const noexcept { return raw_; }
    [[nodiscard]] bool       empty() const noexcept { return raw_ == nullptr; }
    [[nodiscard]] bool       valid() const noexcept;

    // Returns: owns raw LVGL image object; caller is responsible for deletion.
    [[nodiscard]] lv_obj_t* release() noexcept;
    void reset() noexcept;

    void set_source(ImageDescriptorView descriptor) noexcept;
    // Args:
    //   source: external null-terminated LVGL file path or symbol string.
    void set_source(const char* source) noexcept;
    [[nodiscard]] const void* source() const noexcept;
    [[nodiscard]] lv_image_src_t source_type() const noexcept;
    [[nodiscard]] std::int32_t source_width() const noexcept;
    [[nodiscard]] std::int32_t source_height() const noexcept;

    void set_offset_x(std::int32_t x) noexcept;
    void set_offset_y(std::int32_t y) noexcept;
    [[nodiscard]] std::int32_t offset_x() const noexcept;
    [[nodiscard]] std::int32_t offset_y() const noexcept;

    void set_rotation(std::int32_t angle_tenths) noexcept;
    [[nodiscard]] std::int32_t rotation() const noexcept;
    void set_pivot(std::int32_t x, std::int32_t y) noexcept;
    [[nodiscard]] lv_point_t pivot() const noexcept;
    void set_scale(std::uint32_t scale) noexcept;
    void set_scale_x(std::uint32_t scale) noexcept;
    void set_scale_y(std::uint32_t scale) noexcept;
    [[nodiscard]] std::int32_t scale() const noexcept;
    [[nodiscard]] std::int32_t scale_x() const noexcept;
    [[nodiscard]] std::int32_t scale_y() const noexcept;

    void set_blend_mode(lv_blend_mode_t mode) noexcept;
    void set_antialias(bool enabled) noexcept;
    void set_inner_align(ImageAlign align) noexcept;
    [[nodiscard]] ImageAlign inner_align() const noexcept;

    void set_bitmap_mask(ImageDescriptorView descriptor) noexcept;

private:
    explicit LvImage(lv_obj_t* raw) noexcept : raw_{raw} {}

    // owns: deleted with lv_obj_delete() when non-null and still valid.
    lv_obj_t* raw_ = nullptr;
};

class LvDrawLabelDescriptor {
public:
    LvDrawLabelDescriptor() noexcept;

    [[nodiscard]] lv_draw_label_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_label_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

private:
    // owns: descriptor value. Pointer fields, when set by caller, are external.
    lv_draw_label_dsc_t raw_{};
};

class LvDrawImageDescriptor {
public:
    LvDrawImageDescriptor() noexcept;

    [[nodiscard]] lv_draw_image_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_image_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

private:
    // owns: descriptor value. src/mask pointers, when set, are external.
    lv_draw_image_dsc_t raw_{};
};

class LvDrawFillDescriptor {
public:
    LvDrawFillDescriptor() noexcept;

    [[nodiscard]] lv_draw_fill_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_fill_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

    void set_color(core::Color color) noexcept;
    void set_opa(std::uint8_t opa) noexcept;
    void set_radius(std::int32_t radius) noexcept;

private:
    // owns: descriptor value; no retained external pointers in v1 helpers.
    lv_draw_fill_dsc_t raw_{};
};

class LvDrawBorderDescriptor {
public:
    LvDrawBorderDescriptor() noexcept;

    [[nodiscard]] lv_draw_border_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_border_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

private:
    // owns: descriptor value; no retained external pointers in v1 helpers.
    lv_draw_border_dsc_t raw_{};
};

class LvDrawBoxShadowDescriptor {
public:
    LvDrawBoxShadowDescriptor() noexcept;

    [[nodiscard]] lv_draw_box_shadow_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_box_shadow_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

private:
    // owns: descriptor value; no retained external pointers in v1 helpers.
    lv_draw_box_shadow_dsc_t raw_{};
};

class LvDrawRectDescriptor {
public:
    LvDrawRectDescriptor() noexcept;

    [[nodiscard]] lv_draw_rect_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_rect_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

private:
    // owns: descriptor value. bg image pointers, when set by caller, are external.
    lv_draw_rect_dsc_t raw_{};
};

class LvDrawMaskRectDescriptor {
public:
    LvDrawMaskRectDescriptor() noexcept;

    [[nodiscard]] lv_draw_mask_rect_dsc_t* borrow_raw() noexcept { return &raw_; }
    [[nodiscard]] const lv_draw_mask_rect_dsc_t* borrow_raw() const noexcept {
        return &raw_;
    }

    void set_area(LvArea area) noexcept;
    void set_radius(std::int32_t radius) noexcept;
    void set_keep_outside(bool keep_outside) noexcept;

private:
    // owns: descriptor value; no retained external pointers in v1 helpers.
    lv_draw_mask_rect_dsc_t raw_{};
};

}  // namespace lvglpp

#endif  // LVGLPP_CORE_DRAW_LVGL_HPP
