// command.hpp — Command sum type for the playit wire protocol.
//
// PARITY: rlvgl/playit/src/command.rs (v0.2.0 @ 79f730d) — Command<'a>,
//         QuerySpec<'a>, DumpSpec.
// LVGL:   N/A.
// DELTA:  rlvgl's Command<'a> borrows tag strings as &'a str from the
//         input line; lvglpp uses std::string_view with the same
//         lifetime contract — the Command must not outlive the input
//         buffer that produced it. Tagged with `borrows` per
//         CLAUDE.md § "Strict and Explicit Ownership".

#ifndef LVGLPP_PLAYIT_COMMAND_HPP
#define LVGLPP_PLAYIT_COMMAND_HPP

#include <cstdint>
#include <string_view>
#include <variant>

#include "lvglpp/playit/event_spec.hpp"

namespace lvglpp::playit {

// Mirrors rlvgl/playit/src/command.rs:138.
struct DumpSpec {
    std::int32_t  x      = 0;
    std::int32_t  y      = 0;
    std::uint16_t width  = 1;   // clamped 1..=40 by the parser
    std::uint16_t height = 1;   // clamped 1..=40 by the parser
    std::uint8_t  frames = 1;   // clamped 1..=4 by the parser

    constexpr bool operator==(const DumpSpec&) const noexcept = default;
};

namespace command {

// `?` — runtime status query.
struct Status {
    constexpr bool operator==(const Status&) const noexcept = default;
};

// `RS` / `RE` / `RD`.
struct RecordStart {
    constexpr bool operator==(const RecordStart&) const noexcept = default;
};
struct RecordStop {
    constexpr bool operator==(const RecordStop&) const noexcept = default;
};
struct RecordDump {
    constexpr bool operator==(const RecordDump&) const noexcept = default;
};

// Tx,y / TKn / TDx,y / TTx,y / PDx,y / PUx,y / PMx,y / KD:k / KU:k /
// MTn:... — every form that injects an event into the widget tree
// root.
struct Inject {
    EventSpec event{};
    constexpr bool operator==(const Inject&) const noexcept = default;
};

// `T@<tag>:<x>,<y>` — tagged inject. tag borrows from the input line.
struct InjectTagged {
    // borrows: lifetime tied to the input line that fed the parser.
    // Must not outlive that buffer.
    std::string_view tag{};
    EventSpec        event{};
    constexpr bool operator==(const InjectTagged&) const noexcept = default;
};

// `QB:<tag>` / `QE:<tag>` / `QC:<tag>`. tag borrows from the input.
struct QueryBounds {
    std::string_view tag{};
    constexpr bool operator==(const QueryBounds&) const noexcept = default;
};
struct QueryExists {
    std::string_view tag{};
    constexpr bool operator==(const QueryExists&) const noexcept = default;
};
struct QueryChildCount {
    std::string_view tag{};
    constexpr bool operator==(const QueryChildCount&) const noexcept = default;
};

// `D<x>,<y>,<w>,<h>[,<frames>]`.
struct DumpPixels {
    DumpSpec spec{};
    constexpr bool operator==(const DumpPixels&) const noexcept = default;
};

// `X<payload>` — application-defined extension. payload borrows.
struct Extension {
    std::string_view payload{};
    constexpr bool operator==(const Extension&) const noexcept = default;
};

}  // namespace command

// Sum type over every wire-protocol command. Mirrors the variant set
// in rlvgl/playit/src/command.rs:7.
using Command = std::variant<
    command::Inject,
    command::InjectTagged,
    command::QueryBounds,
    command::QueryExists,
    command::QueryChildCount,
    command::DumpPixels,
    command::Status,
    command::RecordStart,
    command::RecordStop,
    command::RecordDump,
    command::Extension
>;

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_COMMAND_HPP
