// format.hpp — wire-format Response → ASCII serialiser.
//
// PARITY: rlvgl/playit/src/protocol.rs:466 (format_response).
//         v0.2.0 @ 79f730d.
// LVGL:   N/A.
// DELTA:  rlvgl uses an internal BufWriter that silently truncates;
//         lvglpp uses std::span<char> directly with the same
//         truncation semantics.
//
// docs/playit-tagged/01-response-formatter.md (PLAYIT-04b) freezes
// the wire format under Standards Action.

#ifndef LVGLPP_PLAYIT_FORMAT_HPP
#define LVGLPP_PLAYIT_FORMAT_HPP

#include <cstddef>
#include <span>

#include "lvglpp/playit/event_spec.hpp"
#include "lvglpp/playit/response.hpp"

namespace lvglpp::playit {

// Format a Response as a CRLF-terminated ASCII line into `buf`.
[[nodiscard]] std::size_t format_response(const Response& resp,
                                          std::span<char> buf) noexcept;

// Format an EventSpec as the wire form per
// docs/playit-recorder/00-event-recorder.md §5.2 (mirrors
// rlvgl/playit/src/protocol.rs:359). NO trailing newline.
[[nodiscard]] std::size_t format_event_spec(const EventSpec& spec,
                                            std::span<char> buf) noexcept;

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_FORMAT_HPP
