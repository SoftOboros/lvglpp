// PARITY: rlvgl/platform/src/hwcore/surface.rs (BackBuffer /
//         BorrowedForDma typed framebuffer handles, v0.2.0 @ 79f730d).
// LVGL:   N/A — sits below the LVGL draw-buffer surface.
// DELTA:  C++ has no borrow checker: exclusivity is enforced by move
//         semantics (the handle is moved into an InFlight token for
//         the duration of a DMA2D transfer) instead of lifetimes.
//
// PLAT-02e-2 §5.7 — move-only value handle over a framebuffer region.

#pragma once

#include <cstdint>

namespace lvglpp::disco {

// Move-only handle carrying mutation authority over an ARGB8888
// framebuffer region.
//
// Ownership: the handle does NOT own the memory — the region is
// `external:` SDRAM (lifecycle owned by the hardware mapping, never
// freed) and `dma:` (DMA2D writes it, the LTDC scans it). What the
// handle carries is the exclusive right to CPU-mutate the region;
// moving it into an InFlight token suspends that right for the
// duration of a transfer.
class FrameBuffer {
public:
    constexpr FrameBuffer() noexcept = default;  // invalid handle
    constexpr FrameBuffer(std::uintptr_t base,
                          std::uint32_t width,
                          std::uint32_t height,
                          std::uint32_t stride_px) noexcept
        : base_{base}, w_{width}, h_{height}, stride_{stride_px} {}

    FrameBuffer(const FrameBuffer&)            = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    constexpr FrameBuffer(FrameBuffer&& other) noexcept
        : base_{other.base_}, w_{other.w_}, h_{other.h_},
          stride_{other.stride_} {
        other.base_ = 0;
    }
    constexpr FrameBuffer& operator=(FrameBuffer&& other) noexcept {
        base_ = other.base_; w_ = other.w_; h_ = other.h_;
        stride_ = other.stride_;
        other.base_ = 0;
        return *this;
    }

    [[nodiscard]] constexpr bool valid() const noexcept { return base_ != 0; }
    [[nodiscard]] constexpr std::uint32_t width() const noexcept { return w_; }
    [[nodiscard]] constexpr std::uint32_t height() const noexcept { return h_; }
    [[nodiscard]] constexpr std::uint32_t stride_px() const noexcept { return stride_; }

    // dma: pointer into the region; valid()==true required. Caller
    // must not cache it across a move into an InFlight token.
    [[nodiscard]] volatile std::uint32_t* pixels() const noexcept {
        // SAFETY: base_ is the SDRAM framebuffer mapping (PLAT-02d
        // §5); integer→pointer cast is confined to this accessor.
        return reinterpret_cast<volatile std::uint32_t*>(base_);
    }

private:
    std::uintptr_t base_   = 0;  // external/dma: see class comment
    std::uint32_t  w_      = 0;
    std::uint32_t  h_      = 0;
    std::uint32_t  stride_ = 0;
};

} // namespace lvglpp::disco
