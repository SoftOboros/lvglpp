// gallery_scene.hpp — the WID-05/WID-06 conformance composition.
//
// PARITY: composition only — the widgets it instantiates carry their
//         own rlvgl parity (list.rs / image.rs / container.rs).
// LVGL:   N/A.
// DELTA:  N/A (example scene, shared by the gallery sim and the
//         PLAT-LNX fbdev smoke so both render the identical tree).
//
// docs/widgets-list/00-list.md §12 + docs/widgets-image/00-image.md
// §12 + docs/platform-linux/00-fbdev-evdev.md §5.3.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "lvglpp/core/plugins/rle.hpp"  // CORE-07n (LVGLPP_CORE_RLE)
#include "lvglpp/core/widget_node.hpp"
#include "lvglpp/widgets/container.hpp"
#include "lvglpp/widgets/image.hpp"
#include "lvglpp/widgets/list.hpp"

namespace lvglpp::gallery {

struct Scene {
    core::WidgetNode root;
    // owns: decoded icon pixels; the Image widget borrows this
    // buffer, so the Scene must outlive the tree traversals.
    std::vector<core::Color> icon_pixels;
    std::uint32_t icon_w = 0, icon_h = 0;
};

// Build the conformance tree: navy container + a 3-item List
// ("alpha"/"beta"/"gamma", tag gallery.list) + an RLE-decoded Image
// (tag gallery.icon) at (300, 40).
//
// Args:
//   icon_blob: borrows an RLEC blob for the duration of the call
//              (decoded into the returned Scene's own buffer).
// Returns:
//   owns the Scene, or nullopt when the blob fails to decode.
inline std::optional<Scene>
build_scene(std::span<const std::uint8_t> icon_blob) {
    Scene scene{core::WidgetNode{
                    std::make_unique<widgets::Container>(
                        core::Rect{0, 0, 800, 480}),
                    "gallery.root"},
                {}, 0, 0};
    if (auto* cont = static_cast<widgets::Container*>(
            scene.root.widget.get())) {
        cont->style.bg_color = core::Color{16, 24, 48, 255};
    }

    auto parsed = core::rle::parse_blob(icon_blob);
    if (!parsed.has_value()) return std::nullopt;
    const auto& view = parsed.value();
    scene.icon_w = view.width;
    scene.icon_h = view.height;
    scene.icon_pixels.assign(
        static_cast<std::size_t>(view.width) * view.height,
        core::Color{});
    if (!core::rle::decode_into(
            view, std::span<core::Color>(scene.icon_pixels))
             .has_value()) {
        return std::nullopt;
    }

    auto list = std::make_unique<widgets::List>(
        core::Rect{40, 40, 200, 80});
    list->style.bg_color     = core::Color{16, 24, 48, 255};
    list->style.border_color = core::Color{255, 200, 0, 255};
    list->text_color         = core::Color{255, 255, 255, 255};
    list->add_item("alpha");
    list->add_item("beta");
    list->add_item("gamma");
    scene.root.add_child(core::WidgetNode{std::move(list),
                                          "gallery.list"});

    auto image = std::make_unique<widgets::Image>(
        core::Rect{300, 40, static_cast<std::int32_t>(scene.icon_w),
                   static_cast<std::int32_t>(scene.icon_h)},
        static_cast<std::int32_t>(scene.icon_w),
        static_cast<std::int32_t>(scene.icon_h),
        std::span<const core::Color>(scene.icon_pixels));
    image->style.alpha = 0;  // pure blit; no bg behind the icon
    scene.root.add_child(core::WidgetNode{std::move(image),
                                          "gallery.icon"});
    return scene;
}

}  // namespace lvglpp::gallery
