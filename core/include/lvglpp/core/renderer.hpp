// renderer.hpp — target-agnostic drawing interface.
//
// PARITY: rlvgl/core/src/renderer.rs (v0.2.0 @ d99f793) — Renderer
//         trait (line 12), default blend_rect (line 25) and
//         draw_pixels (line 33) implementations.
// LVGL:   lvgl/src/draw/lv_draw.h — informative only. Each lvglpp
//         backend translates between the Renderer abstract base and
//         LVGL's display-driver callbacks in its own translation
//         unit.
// DELTA:  rlvgl uses Rust trait with default-method fallback; lvglpp
//         uses a C++ abstract base class. The default bodies are
//         IDENTICAL in semantics — see §5.2 of the concepts doc.
//
// This file implements docs/core-renderer/00-renderer-trait.md
// (CORE-04). Method set and default implementations are FROZEN under
// Standards Action.

#ifndef LVGLPP_CORE_RENDERER_HPP
#define LVGLPP_CORE_RENDERER_HPP

#include <cstdint>
#include <span>
#include <string_view>

#include "lvglpp/core/widget.hpp"  // Color, Rect

namespace lvglpp::core {

// Renderer abstract base. Backends subclass and override at minimum
// fill_rect and draw_text; blend_rect and draw_pixels have default
// bodies that fall back to fill_rect (per concepts doc §5.2).
//
// Ownership: Renderer is a non-owning *interface*. Concrete backends
// hold their own framebuffer / display-driver state; widgets receive
// the renderer by mutable reference (`borrows`) for the duration of
// a single draw() call and MUST NOT retain a reference past return.
class Renderer {
public:
    Renderer()                                   = default;
    Renderer(const Renderer&)                    = default;
    Renderer(Renderer&&) noexcept                = default;
    Renderer& operator=(const Renderer&)         = default;
    Renderer& operator=(Renderer&&) noexcept     = default;
    virtual ~Renderer()                          = default;

    // Fill a rectangle with a solid color. Pure virtual.
    virtual void fill_rect(Rect rect, Color color) = 0;

    // Draw UTF-8 text with its baseline at (x, y). The renderer's
    // built-in font is implementation-defined; widgets that need a
    // specific font use the CORE-06 font helpers (BitmapFont,
    // PackedFont) which call back into fill_rect / draw_pixels.
    //
    // Args:
    //   text: borrows; must outlive the call. Not retained.
    virtual void draw_text(std::int32_t x,
                           std::int32_t y,
                           std::string_view text,
                           Color color) = 0;

    // Alpha-blended rectangle fill. Default: ignore alpha and call
    // fill_rect. Backends with hardware blending SHOULD override.
    // Mirrors rlvgl/core/src/renderer.rs:25.
    virtual void blend_rect(Rect rect, Color color) {
        fill_rect(rect, color);
    }

    // Bulk pixel blit. Default: per-pixel fill_rect loop. Backends
    // with bulk-copy / DMA support SHOULD override.
    //
    // Args:
    //   pixels: borrows; row-major, top-to-bottom, left-to-right.
    //           length MUST be at least width*height; trailing
    //           pixels beyond `pixels.size()` are skipped per
    //           rlvgl/core/src/renderer.rs:36.
    //
    // Mirrors rlvgl/core/src/renderer.rs:33 — same loop shape, same
    // out-of-bounds tolerance.
    virtual void draw_pixels(std::int32_t x,
                             std::int32_t y,
                             std::span<const Color> pixels,
                             std::uint32_t width,
                             std::uint32_t height) {
        for (std::uint32_t py = 0; py < height; ++py) {
            for (std::uint32_t px = 0; px < width; ++px) {
                const std::size_t idx = static_cast<std::size_t>(py) * width + px;
                if (idx < pixels.size()) {
                    fill_rect(
                        Rect{
                            x + static_cast<std::int32_t>(px),
                            y + static_cast<std::int32_t>(py),
                            1,
                            1,
                        },
                        pixels[idx]);
                }
            }
        }
    }
};

}  // namespace lvglpp::core

#endif  // LVGLPP_CORE_RENDERER_HPP
