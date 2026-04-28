// response.hpp — Response value type produced by the Dispatcher.
//
// PARITY: rlvgl/playit/src/response.rs (v0.2.0 @ b178cbc).
// LVGL:   N/A.
// DELTA:  rlvgl uses an enum with `Error(&'a str)`; lvglpp uses
//         std::variant + per-variant POD structs with
//         std::string_view for the error reason (`borrows`).

#ifndef LVGLPP_PLAYIT_RESPONSE_HPP
#define LVGLPP_PLAYIT_RESPONSE_HPP

#include <cstdint>
#include <string_view>
#include <variant>

namespace lvglpp::playit {

// Mirrors rlvgl/playit/src/response.rs:5.
struct StatusData {
    std::uint32_t tick_count    = 0;
    std::uint32_t present_count = 0;

    constexpr bool operator==(const StatusData&) const noexcept = default;
};

namespace response {

struct Ok {
    constexpr bool operator==(const Ok&) const noexcept = default;
};

struct Error {
    // borrows: lifetime is the caller's. Typical reasons are
    // string-literal constants in the Dispatcher; any non-static
    // source must outlive the Response value.
    std::string_view reason{};
    constexpr bool operator==(const Error&) const noexcept = default;
};

struct Bounds {
    std::int32_t x      = 0;
    std::int32_t y      = 0;
    std::int32_t width  = 0;
    std::int32_t height = 0;
    constexpr bool operator==(const Bounds&) const noexcept = default;
};

struct Exists {
    bool value = false;
    constexpr bool operator==(const Exists&) const noexcept = default;
};

struct ChildCount {
    std::uint16_t value = 0;
    constexpr bool operator==(const ChildCount&) const noexcept = default;
};

struct Status {
    StatusData snapshot{};
    constexpr bool operator==(const Status&) const noexcept = default;
};

struct DumpEnd {
    constexpr bool operator==(const DumpEnd&) const noexcept = default;
};

}  // namespace response

// Variant ordering matches rlvgl/playit/src/response.rs:14 for
// parity with any future serialization that depends on tag indices.
using Response = std::variant<
    response::Ok,
    response::Error,
    response::Bounds,
    response::Exists,
    response::ChildCount,
    response::Status,
    response::DumpEnd
>;

}  // namespace lvglpp::playit

#endif  // LVGLPP_PLAYIT_RESPONSE_HPP
