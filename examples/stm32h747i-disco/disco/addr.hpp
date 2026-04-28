// PARITY: rlvgl/platform/src/hwcore/addr.rs (MmioAddr<T> typed handle).
// LVGL:   N/A — bring-up plumbing below the LVGL surface.
// DELTA:  none.
//
// PLAT-02 §5.5 #2 + §3 "Address domain types". Typed handle for a
// memory-mapped register block. Bare reinterpret_cast of an MMIO
// address is permitted only inside this header (the regs layer);
// application code obtains a typed reference via `MmioAddr<T>::ref()`
// and reads/writes fields by name.
//
// `mmio: owned by RM0399 §<N.M>; never freed.` — the discipline
// comment lives on each MmioAddr<T> constant in the regs/*.hpp
// headers, not here.

#pragma once

#include <cstdint>

namespace lvglpp::disco {

template <typename Block>
class MmioAddr {
public:
    constexpr explicit MmioAddr(std::uintptr_t addr) noexcept
        : addr_{addr} {}

    // Typed reference to the underlying register block. The naked
    // reinterpret_cast lives here so caller code never needs to.
    [[nodiscard]] Block& ref() const noexcept {
        // SAFETY:
        //   addr_ is an MMIO base from RM0399; the Block layout is
        //   verified against documented offsets via static_assert in
        //   each regs/*.hpp header. No CPU RAM owns this region;
        //   alias-with-anything is correct because the hardware is
        //   the sole writer of read-only fields. No synchronization
        //   is required because every Block field is `volatile`.
        return *reinterpret_cast<Block*>(addr_);
    }

    [[nodiscard]] Block* operator->() const noexcept {
        return reinterpret_cast<Block*>(addr_);
    }

    [[nodiscard]] constexpr std::uintptr_t raw() const noexcept {
        return addr_;
    }

private:
    std::uintptr_t addr_;
};

} // namespace lvglpp::disco
