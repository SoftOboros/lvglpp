// command.hpp — controller→runtime command queue surface.
//
// PARITY: rlvgl/examples/apps/disco-demo/src/lib.rs (v0.2.0 @ 79f730d)
//         — DiscoEffect (:133), DiscoCommand (:142).
// LVGL:   N/A (app controller).
// DELTA:  Rust enum-with-payload becomes a std::variant over per-variant
//         aggregate structs in the `cmd` namespace (parity with the
//         core::Event encoding). Variant set FROZEN (Standards Action,
//         DEMO-06 §2).
//
// docs/disco-demo/06-controller-and-host-target.md (DEMO-06) §2.

#ifndef LVGLPP_APP_DISCO_DEMO_COMMAND_HPP
#define LVGLPP_APP_DISCO_DEMO_COMMAND_HPP

#include <cstdint>
#include <string>
#include <variant>

namespace lvglpp::app::disco_demo {

// Effect hooks requested by the shared demo controller. FROZEN.
enum class DiscoEffect : std::uint8_t {
    AudioScope,  // audio-scope / audio-reactive visualizations.
    StarCrawl,   // Star Wars style crawl used by the STM32 demo.
};

// Command payloads. One aggregate per DiscoCommand variant so std::visit
// dispatches by type. Ordering mirrors lib.rs:142.
namespace cmd {

// Request a backlight change using an abstract 0..=100 level.
struct SetBacklight {
    std::uint8_t level = 0;
    [[nodiscard]] bool operator==(const SetBacklight&) const noexcept = default;
};

// Request that the runtime populate or refresh storage details.
struct LoadStorageSummary {
    [[nodiscard]] bool operator==(const LoadStorageSummary&) const noexcept = default;
};

// Request that a runtime-specific visual effect should start.
struct StartEffect {
    DiscoEffect effect = DiscoEffect::AudioScope;
    [[nodiscard]] bool operator==(const StartEffect&) const noexcept = default;
};

// Request that a runtime-specific visual effect should stop.
struct StopEffect {
    DiscoEffect effect = DiscoEffect::AudioScope;
    [[nodiscard]] bool operator==(const StopEffect&) const noexcept = default;
};

// Inform the runtime that the controller wants a status line surfaced.
struct ShowStatus {
    std::string text;
    [[nodiscard]] bool operator==(const ShowStatus&) const noexcept = default;
};

// Explicitly record that an action was intentionally ignored.
struct NoOp {
    [[nodiscard]] bool operator==(const NoOp&) const noexcept = default;
};

}  // namespace cmd

// Commands emitted by the shared demo controller for a runtime adapter.
// FROZEN variant set (Standards Action, DEMO-06 §2).
using DiscoCommand = std::variant<
    cmd::SetBacklight,
    cmd::LoadStorageSummary,
    cmd::StartEffect,
    cmd::StopEffect,
    cmd::ShowStatus,
    cmd::NoOp>;

}  // namespace lvglpp::app::disco_demo

#endif  // LVGLPP_APP_DISCO_DEMO_COMMAND_HPP
