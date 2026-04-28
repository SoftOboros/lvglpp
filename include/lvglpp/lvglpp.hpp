// lvglpp.hpp — top-level meta-umbrella.
//
// PARITY: rlvgl/src/lib.rs (workspace top-level re-exports), v0.2.0.
//
// Includes every module umbrella so a downstream consumer can write
// `#include <lvglpp/lvglpp.hpp>` and get the full surface. Per-module
// umbrellas remain available if a consumer wants a tighter dependency
// (e.g. just `<lvglpp/core/core.hpp>` for a custom backend).
//
// Ownership categories used throughout this project (see CLAUDE.md):
//   owns      — responsible for destruction/release.
//   borrows   — temporary non-owning access; must not outlive owner.
//   observes  — nullable/passive non-owning access; no mutation ownership.
//   shares    — reference-counted/shared lifecycle.
//   external  — lifecycle controlled outside this module.
//   mmio      — memory-mapped hardware; never freed.
//   dma       — buffer may be mutated by hardware/driver.

#ifndef LVGLPP_LVGLPP_HPP
#define LVGLPP_LVGLPP_HPP

#include "lvglpp/core/core.hpp"
#include "lvglpp/widgets/widgets.hpp"
#include "lvglpp/ui/ui.hpp"
#include "lvglpp/platform/platform.hpp"
#include "lvglpp/playit/playit.hpp"

#endif  // LVGLPP_LVGLPP_HPP
