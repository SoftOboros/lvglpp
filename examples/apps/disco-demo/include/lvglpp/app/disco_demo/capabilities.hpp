// capabilities.hpp — runtime capability descriptor + presets.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/lib.rs (v0.2.0 @ 79f730d)
//         — DiscoCapabilities (:38) and the five presets (:61-122).
// LVGL:   N/A (app controller).
// DELTA:  Rust `&'static str platform` becomes std::string_view; presets
//         are `static constexpr` factory methods. Field set and preset
//         values byte-match rlvgl. FROZEN (Standards Action, DEMO-06 §2).
//
// docs/disco-demo/06-controller-and-host-target.md (DEMO-06) §2.

#ifndef LVGLPP_APP_DISCO_DEMO_CAPABILITIES_HPP
#define LVGLPP_APP_DISCO_DEMO_CAPABILITIES_HPP

#include <string_view>

namespace lvglpp::app::disco_demo {

// Runtime capabilities that shape which portions of the disco demo are
// active. Trivially-copyable value type — no ownership concerns.
struct DiscoCapabilities {
    bool audio       = false;  // platform audio demos can run.
    bool storage     = false;  // runtime can surface storage summaries.
    bool diagnostics = false;  // diagnostic widgets expose platform probes.
    bool effects     = false;  // animation/effect demos (star crawl) available.
    bool pointer     = false;  // pointer or touch input supported.
    // observes: immortal string literal; lifetime is the program's.
    std::string_view platform = "";

    [[nodiscard]] constexpr bool operator==(
        const DiscoCapabilities&) const noexcept = default;

    // ---- Presets (mirror lib.rs:61-122) -----------------------------------

    // Simulator hosts. effects=true so the simulator can run motion demos.
    [[nodiscard]] static constexpr DiscoCapabilities simulator() noexcept {
        return DiscoCapabilities{
            /*audio=*/false, /*storage=*/true, /*diagnostics=*/true,
            /*effects=*/true, /*pointer=*/true, /*platform=*/"simulator"};
    }

    // STM32H747I-DISCO bare-metal hardware runtime.
    [[nodiscard]] static constexpr DiscoCapabilities stm32h747i_disco() noexcept {
        return DiscoCapabilities{
            /*audio=*/true, /*storage=*/true, /*diagnostics=*/true,
            /*effects=*/true, /*pointer=*/true,
            /*platform=*/"STM32H747I-DISCO bare-metal"};
    }

    // First AArch64 UEFI milestone.
    [[nodiscard]] static constexpr DiscoCapabilities uefi() noexcept {
        return DiscoCapabilities{
            /*audio=*/false, /*storage=*/false, /*diagnostics=*/true,
            /*effects=*/false, /*pointer=*/false, /*platform=*/"UEFI"};
    }

    // Zephyr RTOS runtime.
    [[nodiscard]] static constexpr DiscoCapabilities zephyr() noexcept {
        return DiscoCapabilities{
            /*audio=*/false, /*storage=*/true, /*diagnostics=*/true,
            /*effects=*/true, /*pointer=*/true, /*platform=*/"Zephyr RTOS"};
    }

    // BeagleBone Black with NHD-7.0CTP-CAPE-P. No on-board audio codec.
    [[nodiscard]] static constexpr DiscoCapabilities beaglebone_black() noexcept {
        return DiscoCapabilities{
            /*audio=*/false, /*storage=*/true, /*diagnostics=*/true,
            /*effects=*/true, /*pointer=*/true, /*platform=*/"BeagleBone Black"};
    }

    // Default preset (mirror lib.rs:125 — Default = simulator()).
    [[nodiscard]] static constexpr DiscoCapabilities make_default() noexcept {
        return simulator();
    }
};

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_CAPABILITIES_HPP
