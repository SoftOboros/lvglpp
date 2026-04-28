// host_sdl.cpp — host-side SDL2 backend implementation.
//
// PARITY: rlvgl/platform/src/simulator.rs (v0.2.0 @ 79f730d) — host
//         simulator. lvglpp uses SDL2; the cross-language contract
//         (Event ordering for the same physical input) is the §5.4
//         translation table in the chapter.

#include "lvglpp/platform/host_sdl.hpp"

#include <SDL.h>

#include <atomic>
#include <utility>

#include "lvglpp/core/fonts/font_6x10.hpp"

namespace lvglpp::platform {

namespace {

// Single-instance flag — at most one HostSdlBackend may be alive.
// Mirrors CORE-01 Runtime's pattern.
std::atomic<bool> g_backend_alive{false};

bool acquire_alive_slot() noexcept {
    bool expected = false;
    return g_backend_alive.compare_exchange_strong(expected, true);
}

void release_alive_slot() noexcept {
    g_backend_alive.store(false);
}

// Translate SDL_Keycode to an lvglpp::core::Key per concepts §5.4.
::lvglpp::core::Key translate_key(SDL_Keycode kc) noexcept {
    using ::lvglpp::core::Key;
    namespace k = ::lvglpp::core::key;

    switch (kc) {
        case SDLK_ESCAPE: return Key{k::Escape{}};
        case SDLK_RETURN: return Key{k::Enter{}};
        case SDLK_SPACE:  return Key{k::Space{}};
        case SDLK_UP:     return Key{k::ArrowUp{}};
        case SDLK_DOWN:   return Key{k::ArrowDown{}};
        case SDLK_LEFT:   return Key{k::ArrowLeft{}};
        case SDLK_RIGHT:  return Key{k::ArrowRight{}};
        default: break;
    }
    if (kc >= SDLK_F1 && kc <= SDLK_F12) {
        return Key{k::Function{static_cast<std::uint8_t>(kc - SDLK_F1 + 1)}};
    }
    if (kc >= 0x20 && kc <= 0x7E) {
        return Key{k::Character{static_cast<std::uint32_t>(kc)}};
    }
    return Key{k::Other{static_cast<std::uint32_t>(kc)}};
}

}  // namespace

// ===========================================================================
// SdlRenderer
// ===========================================================================

void SdlRenderer::fill_rect(::lvglpp::core::Rect rect,
                            ::lvglpp::core::Color color) {
    SDL_SetRenderDrawColor(raw_, color.r, color.g, color.b, color.a);
    SDL_Rect r{
        rect.x,
        rect.y,
        rect.width,
        rect.height,
    };
    SDL_RenderFillRect(raw_, &r);
}

void SdlRenderer::draw_text(std::int32_t x,
                            std::int32_t y,
                            std::string_view text,
                            ::lvglpp::core::Color color) {
    // The Label baseline convention (WID-01 §5.3) anchors `y` at the
    // bottom of the text; BitmapFont::draw_str takes the top-left
    // corner. Translate baseline → top-left via scaled_height().
    const auto& font = ::lvglpp::core::fonts::FONT_6X10;
    const auto top_y = y - font.scaled_height();
    font.draw_str(*this, x, top_y, text, color);
}

// ===========================================================================
// HostSdlBackend
// ===========================================================================

lvglpp::expected<HostSdlBackend, SdlError>
HostSdlBackend::try_make(std::string_view title,
                         std::int32_t width,
                         std::int32_t height) noexcept {
    if (!acquire_alive_slot()) {
        return lvglpp::unexpected<SdlError>{SdlError::AlreadyAlive};
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        release_alive_slot();
        return lvglpp::unexpected<SdlError>{SdlError::InitFailed};
    }

    // SDL_CreateWindow takes a NUL-terminated C string. string_view
    // is not guaranteed to be NUL-terminated, so copy into a
    // temporary std::string.
    const std::string title_buf{title};

    SDL_Window* window = SDL_CreateWindow(
        title_buf.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        SDL_Quit();
        release_alive_slot();
        return lvglpp::unexpected<SdlError>{SdlError::WindowFailed};
    }

    // Try the accelerated + vsync path first (concepts doc §5.2).
    // Fall back to whatever SDL provides (typically software) when
    // accelerated isn't available — the `SDL_VIDEODRIVER=dummy`
    // headless driver used for piped-fixture diagnostics is the
    // primary motivator for this fallback.
    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(
        window, /*index=*/-1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (sdl_renderer == nullptr) {
        sdl_renderer = SDL_CreateRenderer(window, /*index=*/-1, /*flags=*/0);
    }
    if (sdl_renderer == nullptr) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        release_alive_slot();
        return lvglpp::unexpected<SdlError>{SdlError::RendererFailed};
    }

    SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

    return HostSdlBackend{PrivateTag{}, window, sdl_renderer, width, height};
}

HostSdlBackend::HostSdlBackend(PrivateTag,
                               SDL_Window*   window,
                               SDL_Renderer* sdl_renderer,
                               std::int32_t  width,
                               std::int32_t  height) noexcept
    : window_{window},
      sdl_renderer_{sdl_renderer},
      renderer_{sdl_renderer},
      width_{width},
      height_{height} {}

HostSdlBackend::HostSdlBackend(HostSdlBackend&& other) noexcept
    : window_{other.window_},
      sdl_renderer_{other.sdl_renderer_},
      renderer_{other.sdl_renderer_},
      width_{other.width_},
      height_{other.height_},
      mouse_left_held_{other.mouse_left_held_},
      quit_{other.quit_},
      alive_owned_{other.alive_owned_} {
    other.window_       = nullptr;
    other.sdl_renderer_ = nullptr;
    other.alive_owned_  = false;
}

HostSdlBackend::~HostSdlBackend() {
    if (!alive_owned_) return;
    if (sdl_renderer_ != nullptr) {
        SDL_DestroyRenderer(sdl_renderer_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
    release_alive_slot();
}

std::optional<::lvglpp::core::Event> HostSdlBackend::poll_event() noexcept {
    using ::lvglpp::core::Event;
    namespace e = ::lvglpp::core::event;

    SDL_Event ev;
    while (SDL_PollEvent(&ev) != 0) {
        switch (ev.type) {
            case SDL_QUIT:
                quit_ = true;
                return std::nullopt;

            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
                    quit_ = true;
                    return std::nullopt;
                }
                continue;  // ignore other window events

            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    mouse_left_held_ = true;
                    return Event{e::PointerDown{ev.button.x, ev.button.y}};
                }
                continue;

            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    mouse_left_held_ = false;
                    return Event{e::PointerUp{ev.button.x, ev.button.y}};
                }
                continue;

            case SDL_MOUSEMOTION:
                if (mouse_left_held_) {
                    return Event{e::PointerMove{ev.motion.x, ev.motion.y}};
                }
                continue;

            case SDL_KEYDOWN:
                if (ev.key.repeat == 0) {
                    return Event{e::KeyDown{translate_key(ev.key.keysym.sym)}};
                }
                continue;

            case SDL_KEYUP:
                return Event{e::KeyUp{translate_key(ev.key.keysym.sym)}};

            default:
                continue;  // SDL_FINGER* and the rest — dropped.
        }
    }
    return std::nullopt;
}

void HostSdlBackend::clear(::lvglpp::core::Color color) noexcept {
    SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(sdl_renderer_);
}

void HostSdlBackend::present_frame() noexcept {
    SDL_RenderPresent(sdl_renderer_);
}

}  // namespace lvglpp::platform
