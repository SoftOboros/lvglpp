// display.cpp — Display implementation (LVGLPP LPAR-03).
//
// PARITY: rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md (v0.2.4 @
//         343f596).
// LVGL:   lvgl/src/display/lv_display.c.
// DELTA:  see display.hpp — move-only RAII; draw buffers are caller-owned
//         (dma/external).

#include "lvglpp/core/display.hpp"

#include <utility>  // std::move

#if defined(LVGLPP_EMBEDDED_POSTURE)
#  include <cstdlib>  // std::abort
#else
#  include <stdexcept>  // std::runtime_error
#endif

namespace lvglpp::core {

Display::Display(Display&& other) noexcept : disp_{other.disp_} {
    other.disp_ = nullptr;
}

Display::~Display() {
    if (disp_ != nullptr) {
        lv_display_delete(disp_);
    }
}

lvglpp::expected<Display, DisplayError> Display::try_make(std::int32_t hor,
                                                         std::int32_t ver) noexcept {
    lv_display_t* disp = lv_display_create(hor, ver);
    if (disp == nullptr) {
        return lvglpp::unexpected<DisplayError>{DisplayError::CreateFailed};
    }
    return Display{disp};
}

Display Display::make(std::int32_t hor, std::int32_t ver) {
    auto result = try_make(hor, ver);
    if (!result.has_value()) {
#if defined(LVGLPP_EMBEDDED_POSTURE)
        std::abort();
#else
        throw std::runtime_error("lvglpp::core::Display::make: lv_display_create failed");
#endif
    }
    return std::move(result.value());
}

void Display::set_flush_cb(lv_display_flush_cb_t cb) noexcept {
    if (disp_ != nullptr) {
        // SAFETY: cb is external; its body and any captured target lifetime
        //   are owned by the platform layer (WRAP-0N). LVGL stores the
        //   pointer and invokes it from the refresh timer.
        lv_display_set_flush_cb(disp_, cb);
    }
}

void Display::set_buffers(void* buf1, void* buf2, std::uint32_t size, RenderMode mode) noexcept {
    if (disp_ != nullptr) {
        // SAFETY: buf1/buf2 are dma/external; caller-owned. LVGL renders
        //   into them and the flush_cb ships them to the panel. They must
        //   outlive this Display and must not be CPU-mutated during an
        //   active flush (CLAUDE.md ownership rule 10).
        lv_display_set_buffers(disp_, buf1, buf2, size,
                               static_cast<lv_display_render_mode_t>(mode));
    }
}

void Display::set_render_mode(RenderMode mode) noexcept {
    if (disp_ != nullptr) {
        lv_display_set_render_mode(disp_, static_cast<lv_display_render_mode_t>(mode));
    }
}

ObjectView Display::active_screen() const noexcept {
    if (disp_ == nullptr) {
        return ObjectView{nullptr};
    }
    return ObjectView{lv_display_get_screen_active(disp_)};
}

}  // namespace lvglpp::core
