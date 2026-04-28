// parser.hpp — text wire-protocol parser for lvglpp::playit.
//
// PARITY: rlvgl/playit/src/protocol.rs (v0.2.0 @ 79f730d) —
//         parse_command() and the per-prefix parse_t/p/k/d/q/m/r
//         helpers.
// LVGL:   N/A.
// DELTA:  rlvgl uses Option<Command<'a>>; lvglpp uses std::optional —
//         identical semantics, no behavioural drift. Both are
//         allocation-free (string_view borrows from the input).
//
// The wire format is a single line per command, no trailing newline.
// See rlvgl/playit/src/protocol.rs:1-25 for the canonical grammar
// table; lvglpp implements the same set.

#ifndef LVGLPP_PLAYIT_PARSER_HPP
#define LVGLPP_PLAYIT_PARSER_HPP

#include <optional>
#include <string_view>

#include "lvglpp/playit/command.hpp"

namespace lvglpp::playit {

// Parse a single command line (no trailing newline).
//
// Args:
//   line: borrows the input bytes for the duration of the call. The
//         returned Command may carry std::string_view fields that
//         continue to borrow from the same buffer; the caller MUST
//         keep `line`'s underlying storage alive for as long as the
//         Command is used.
//
// Returns:
//   `std::nullopt` when the line is empty or malformed (parity with
//   rlvgl's `Option<Command<'a>>::None`); a populated Command
//   otherwise. A line whose first byte is unrecognized falls through
//   to `Command::Extension(line)` per rlvgl/playit/src/protocol.rs:134.
[[nodiscard]] std::optional<Command>
parse_command(std::string_view line) noexcept;

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_PARSER_HPP
