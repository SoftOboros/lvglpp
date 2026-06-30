// gallery_scene_obj.hpp — the WID-05/WID-06 gallery composition on lv_obj
// (LVGLPP-WRAP-0N example migration).
//
// PARITY: composition only — same navy container + 3-item List + RLE icon as
//         the legacy gallery_scene.hpp, rebuilt on lv_obj widgets.
// LVGL:   lv_obj / lv_list / lv_image; the icon is an lv_image_dsc_t.
// DELTA:  ObjScene is NON-movable: the Image stores a pointer to the member
//         lv_image_dsc_t (and the dsc points at the member pixel buffer), so
//         the scene's address must stay stable. Build it in place via
//         build_obj_scene(); never move or copy it. Styling uses LVGL local
//         style setters directly while the lvglpp wrapper surface catches up.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "lvglpp/core/object.hpp"
#include "lvglpp/core/plugins/rle.hpp"  // CORE-07n (LVGLPP_CORE_RLE)
#include "lvglpp/core/runtime.hpp"
#include "lvglpp/widgets/container.hpp"
#include "lvglpp/widgets/image.hpp"
#include "lvglpp/widgets/list.hpp"

extern "C" {
#include "lvgl.h"
}

namespace lvglpp::gallery {

// The lv_obj gallery scene. Default member initializers create the tree on the
// default display (the caller MUST create an LvFramebuffer first). Non-movable
// so the lv_image_dsc_t address LVGL stores stays valid.
struct ObjScene {
    core::Screen      screen = core::Screen::make();
    widgets::Container root   = widgets::Container::make(screen.view());
    widgets::List      list   = widgets::List::make(root.view());
    widgets::Image     icon   = widgets::Image::make(root.view());

    // owns: decoded icon pixels (XRGB8888) + the descriptor LVGL points at.
    std::vector<std::uint32_t> icon_pixels;
    lv_image_dsc_t             icon_dsc{};

    ObjScene()                           = default;
    ObjScene(const ObjScene&)            = delete;
    ObjScene& operator=(const ObjScene&) = delete;
    ObjScene(ObjScene&&)                 = delete;  // pinned (see file header).
    ObjScene& operator=(ObjScene&&)      = delete;
};

// Populate `scene` in place: tags, styling, list items, and the RLE-decoded
// icon. Returns false if the icon blob fails to decode (scene left partial).
//   icon_blob: borrows an RLEC blob for the call (decoded into scene.icon_pixels).
inline bool build_obj_scene(ObjScene& scene, std::span<const std::uint8_t> icon_blob) {
    lv_screen_load(scene.screen.borrow_raw());

    scene.root.set_tag("gallery.root");
    scene.root.set_size(800, 480);
    lv_obj_set_style_bg_color(scene.root.borrow_raw(), lv_color_make(16, 24, 48),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scene.root.borrow_raw(), LV_OPA_COVER, LV_PART_MAIN);

    scene.list.set_tag("gallery.list");
    lv_obj_set_pos(scene.list.borrow_raw(), 40, 40);
    scene.list.set_size(200, 80);
    lv_obj_set_style_bg_color(scene.list.borrow_raw(), lv_color_make(16, 24, 48),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scene.list.borrow_raw(), LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(scene.list.borrow_raw(), 2, LV_PART_MAIN);
    lv_obj_set_style_text_color(scene.list.borrow_raw(), lv_color_make(255, 255, 255),
                                LV_PART_MAIN);
    lv_obj_set_style_border_color(scene.list.borrow_raw(), lv_color_make(255, 200, 0),
                                  LV_PART_MAIN);
    static_cast<void>(scene.list.add_text("alpha"));
    static_cast<void>(scene.list.add_text("beta"));
    static_cast<void>(scene.list.add_text("gamma"));

    auto parsed = core::rle::parse_blob(icon_blob);
    if (!parsed.has_value()) {
        return false;
    }
    const auto& view = parsed.value();
    const std::size_t count = static_cast<std::size_t>(view.width) * view.height;

    std::vector<core::Color> rgba(count, core::Color{});
    if (!core::rle::decode_into(view, std::span<core::Color>(rgba)).has_value()) {
        return false;
    }

    // Pack core::Color {r,g,b,a} into LVGL ARGB8888 (memory B,G,R,A; LE uint32).
    scene.icon_pixels.assign(count, 0u);
    for (std::size_t i = 0; i < count; ++i) {
        const core::Color c = rgba[i];
        scene.icon_pixels[i] = (static_cast<std::uint32_t>(c.a) << 24) |
                               (static_cast<std::uint32_t>(c.r) << 16) |
                               (static_cast<std::uint32_t>(c.g) << 8) |
                               static_cast<std::uint32_t>(c.b);
    }

    scene.icon_dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
    scene.icon_dsc.header.cf     = LV_COLOR_FORMAT_ARGB8888;
    scene.icon_dsc.header.w      = view.width;
    scene.icon_dsc.header.h      = view.height;
    scene.icon_dsc.header.stride = static_cast<std::uint32_t>(view.width) * 4u;
    scene.icon_dsc.data_size     = static_cast<std::uint32_t>(count * sizeof(std::uint32_t));
    scene.icon_dsc.data =
        reinterpret_cast<const std::uint8_t*>(scene.icon_pixels.data());

    scene.icon.set_tag("gallery.icon");
    lv_obj_set_pos(scene.icon.borrow_raw(), 300, 40);
    scene.icon.set_src(&scene.icon_dsc);

    return true;
}

}  // namespace lvglpp::gallery
