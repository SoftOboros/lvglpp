// image.hpp — image display widget, lv_obj-backed (LVGLPP-WRAP-06).
//
// PARITY: rlvgl/widgets/src/image.rs (v0.2.4 @ 343f596) — display an image.
// LVGL:   lvgl/src/widgets/image/lv_image.h (lv_image_create / set_src).
// DELTA:  Supersedes lvglpp::widgets::legacy::Image. core::Object subclass
//         over lv_image. The source descriptor is external/static (LVGL stores
//         the pointer) and MUST outlive the image.

#ifndef LVGLPP_WIDGETS_IMAGE_HPP
#define LVGLPP_WIDGETS_IMAGE_HPP

#include "lvglpp/core/object.hpp"
#include "lvglpp/std/expected.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::widgets {

// Image — displays an image from an lv_image_dsc_t, a file path, or a symbol.
//
// Ownership: owns its lv_image lv_obj (via core::Object). Move-only.
class Image : public ::lvglpp::core::Object {
public:
    [[nodiscard]] static lvglpp::expected<Image, ::lvglpp::core::ObjectError>
    try_make(::lvglpp::ObjectView parent) noexcept;
    [[nodiscard]] static Image make(::lvglpp::ObjectView parent);

    Image(Image&&) noexcept = default;

    // Set the image source (lv_image_set_src). `src` may be a pointer to a
    // const lv_image_dsc_t, a NUL-terminated file path, or an LVGL symbol.
    //   src: borrows; external/static — LVGL stores the pointer, so it MUST
    //        outlive this image. No-op when src is null or this Image is empty.
    void set_src(const void* src) noexcept;

private:
    explicit Image(lv_obj_t* obj) noexcept : ::lvglpp::core::Object{obj} {}
};

}  // namespace lvglpp::widgets

#endif  // LVGLPP_WIDGETS_IMAGE_HPP
