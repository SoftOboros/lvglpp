// playit.hpp — module umbrella for lvglpp::playit.
//
// PARITY: rlvgl/playit/src/lib.rs (v0.2.0 @ 79f730d).
// PROTOCOL: rlvgl/playit/README.md — single-line newline-terminated
//           commands, identical wire format. lvglpp re-implements the
//           parser in C++; it does not depend on the Rust crate.
//
// playit is the cross-language test harness: the same probe driver
// exercises rlvgl and lvglpp. Any new command MUST first land in the
// rlvgl playit protocol doc (cross-language change ordering — see
// CLAUDE.md § "Spec-Before-Code").

#ifndef LVGLPP_PLAYIT_PLAYIT_HPP
#define LVGLPP_PLAYIT_PLAYIT_HPP

#include "lvglpp/playit/command.hpp"
#include "lvglpp/playit/conversion.hpp"
#include "lvglpp/playit/dispatcher.hpp"
#include "lvglpp/playit/event_pipeline.hpp"
#include "lvglpp/playit/event_recorder.hpp"
#include "lvglpp/playit/event_spec.hpp"
#include "lvglpp/playit/gesture.hpp"
#include "lvglpp/playit/lvgl_input_bridge.hpp"
#include "lvglpp/playit/executor.hpp"
#include "lvglpp/playit/format.hpp"
#include "lvglpp/playit/parser.hpp"
#include "lvglpp/playit/response.hpp"
#include "lvglpp/playit/transport.hpp"

#endif  // LVGLPP_PLAYIT_PLAYIT_HPP
