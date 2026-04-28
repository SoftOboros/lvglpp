// host_sdl.hpp — host-side SDL2 backend.
//
// PARITY: rlvgl/platform/src/simulator.rs (v0.2.0 @ b178cbc) — host
//         simulator window + event translation. lvglpp uses SDL2
//         instead of winit/wgpu/eframe per docs/platform-host-sdl
//         §10.
// LVGL:   N/A — SdlRenderer bypasses LVGL's display-driver pipeline
//         and writes directly through lvglpp::core::Renderer.
// DELTA:  rlvgl emits raw pointer events from winit then runs them
//         through a recogniser; lvglpp does the same — gestures
//         (PressDown / PressRelease / DoubleTap) are produced
//         downstream by PLAYIT-02 EventPipeline (or a future CORE
//         sub-phase), not by this backend.
//
// This file implements docs/platform-host-sdl/00-host-sdl-backend.md
// (PLAT-01).

#ifndef LVGLPP_PLATFORM_HOST_SDL_HPP
#define LVGLPP_PLATFORM_HOST_SDL_HPP

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  error "lvglpp::platform::HostSdlBackend is host-only; do not include under LVGLPP_EMBEDDED_POSTURE."
#endif

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "lvglpp/core/event.hpp"
#include "lvglpp/core/renderer.hpp"
#include "lvglpp/core/widget.hpp"
#include "lvglpp/std/expected.hpp"

// SDL2 forward declarations to keep the header light. The .cpp pulls
// the full <SDL.h>.
struct SDL_Window;
struct SDL_Renderer;

namespace lvglpp::platform {

// Errors that HostSdlBackend::try_make can produce. Mirrors the
// `Result<HostSdlBackend, _>` ergonomics of CORE-01 Runtime.
enum class SdlError : std::uint8_t {
    AlreadyAlive   = 1,  // a HostSdlBackend is already constructed
    InitFailed     = 2,  // SDL_Init failed
    WindowFailed   = 3,  // SDL_CreateWindow failed
    RendererFailed = 4,  // SDL_CreateRenderer failed
};

// Concrete Renderer subclass wrapping an SDL_Renderer*. Non-owning
// over the SDL handle (the HostSdlBackend owns it).
//
// Ownership: the SDL_Renderer* member is `external` — owned by the
// HostSdlBackend. SdlRenderer instances are created and destroyed
// alongside the backend; do not store one outside that lifetime.
class SdlRenderer final : public ::lvglpp::core::Renderer {
public:
    // Args:
    //   raw: external; owned by the surrounding HostSdlBackend.
    //        Must remain valid for the lifetime of this view.
    explicit SdlRenderer(SDL_Renderer* raw) noexcept : raw_{raw} {}

    void fill_rect(::lvglpp::core::Rect rect,
                   ::lvglpp::core::Color color) override;

    void draw_text(std::int32_t x,
                   std::int32_t y,
                   std::string_view text,
                   ::lvglpp::core::Color color) override;

    // Inherits the default Renderer::blend_rect (delegates to
    // fill_rect, alpha-aware via SDL_BLENDMODE_BLEND set on the
    // backend's renderer) and Renderer::draw_pixels (per-pixel
    // fill_rect loop).

private:
    // external: SDL_Renderer owned by the HostSdlBackend.
    SDL_Renderer* raw_;
};

// Owns the SDL_Window + SDL_Renderer + the SdlRenderer view.
//
// Ownership: at most one HostSdlBackend may be alive at a time
// (single-instance flag mirrors CORE-01 Runtime). Move-construct
// allowed; move-assign deleted.
class HostSdlBackend {
public:
    // Args:
    //   title:   window title, copied into an internal std::string.
    //   width:   window width in pixels (>= 1).
    //   height:  window height in pixels (>= 1).
    // Returns: owns HostSdlBackend on success; SdlError on failure.
    [[nodiscard]] static lvglpp::expected<HostSdlBackend, SdlError>
    try_make(std::string_view title,
             std::int32_t width,
             std::int32_t height) noexcept;

    HostSdlBackend(const HostSdlBackend&)            = delete;
    HostSdlBackend& operator=(const HostSdlBackend&) = delete;
    HostSdlBackend(HostSdlBackend&& other) noexcept;
    HostSdlBackend& operator=(HostSdlBackend&&) = delete;
    ~HostSdlBackend();

    // The renderer for the current frame. Borrows: caller must not
    // retain a reference past the next present_frame() / clear() /
    // destructor call.
    [[nodiscard]] ::lvglpp::core::Renderer& renderer() noexcept { return renderer_; }

    // Pump pending SDL events. Returns the next translated event, or
    // std::nullopt when the queue is drained for this frame.
    // SDL_QUIT / SDL_WINDOWEVENT_CLOSE latch the quit_requested()
    // flag and return std::nullopt to the caller.
    [[nodiscard]] std::optional<::lvglpp::core::Event> poll_event() noexcept;

    [[nodiscard]] bool quit_requested() const noexcept { return quit_; }

    // Clear the back buffer to a solid color. Call once at the
    // start of each frame before drawing the widget tree.
    void clear(::lvglpp::core::Color color) noexcept;

    // Flush the back buffer to the screen.
    void present_frame() noexcept;

    [[nodiscard]] std::int32_t width()  const noexcept { return width_; }
    [[nodiscard]] std::int32_t height() const noexcept { return height_; }

private:
    struct PrivateTag {};
    HostSdlBackend(PrivateTag,
                   SDL_Window* window,
                   SDL_Renderer* sdl_renderer,
                   std::int32_t width,
                   std::int32_t height) noexcept;

    // owns the SDL_Window lifetime when alive_owned_ == true; freed
    // by SDL_DestroyWindow in the destructor.
    SDL_Window*   window_       = nullptr;
    // owns the SDL_Renderer lifetime when alive_owned_ == true.
    SDL_Renderer* sdl_renderer_ = nullptr;

    SdlRenderer  renderer_;
    std::int32_t width_  = 0;
    std::int32_t height_ = 0;
    // Tracks the LEFT mouse button so SDL_MOUSEMOTION emits
    // PointerMove only while pressed (per concepts §5.4).
    bool         mouse_left_held_ = false;
    bool         quit_            = false;
    // owns the alive-slot when true; moved-from instances set this
    // to false so the destructor releases nothing.
    bool         alive_owned_     = true;
};

}  // namespace lvglpp::platform

#endif  // LVGLPP_PLATFORM_HOST_SDL_HPP
