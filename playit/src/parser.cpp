// parser.cpp — implementation of lvglpp::playit::parse_command.
//
// PARITY: rlvgl/playit/src/protocol.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A.
//
// All helpers are local (anonymous namespace). No allocations, no
// exceptions — the parser must compile and run cleanly under
// LVGLPP_EMBEDDED_POSTURE.

#include "lvglpp/playit/parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>

namespace lvglpp::playit {

namespace {

// ---------------------------------------------------------------------------
// Primitive parsers (return value + bytes consumed, or std::nullopt)
// ---------------------------------------------------------------------------

// Mirrors rlvgl/playit/src/protocol.rs:36 — parse a decimal integer
// with optional leading '-'. wrapping_mul/wrapping_add semantics are
// preserved by std::int32_t two's-complement wrap.
struct ParseInt {
    std::int32_t value;
    std::size_t  consumed;
};

[[nodiscard]] std::optional<ParseInt> parse_i32(std::string_view bytes) noexcept {
    if (bytes.empty()) return std::nullopt;

    bool        neg   = false;
    std::size_t start = 0;
    if (bytes[0] == '-') {
        neg   = true;
        start = 1;
    }
    if (start >= bytes.size() ||
        !std::isdigit(static_cast<unsigned char>(bytes[start]))) {
        return std::nullopt;
    }

    // Use unsigned accumulation to mirror Rust's wrapping_mul/add.
    std::uint32_t acc = 0;
    std::size_t   i   = start;
    while (i < bytes.size() &&
           std::isdigit(static_cast<unsigned char>(bytes[i]))) {
        acc = acc * 10U + static_cast<std::uint32_t>(bytes[i] - '0');
        ++i;
    }

    auto value = static_cast<std::int32_t>(acc);
    if (neg) value = -value;
    return ParseInt{value, i};
}

// Mirrors rlvgl/playit/src/protocol.rs:63 — parse "<x>,<y>".
struct XY {
    std::int32_t x;
    std::int32_t y;
};

[[nodiscard]] std::optional<XY> parse_xy(std::string_view bytes) noexcept {
    auto first = parse_i32(bytes);
    if (!first) return std::nullopt;
    if (first->consumed >= bytes.size() || bytes[first->consumed] != ',') {
        return std::nullopt;
    }
    auto second = parse_i32(bytes.substr(first->consumed + 1));
    if (!second) return std::nullopt;
    return XY{first->value, second->value};
}

// Mirrors rlvgl/playit/src/protocol.rs:74 — parse a key name.
[[nodiscard]] std::optional<KeySpec> parse_key(std::string_view name) noexcept {
    if (name.empty()) return std::nullopt;

    // Named keys (case-sensitive — matches rlvgl exactly).
    struct Named {
        std::string_view label;
        KeySpec::Kind    kind;
    };
    constexpr std::array<Named, 11> table{{
        {"Escape",     KeySpec::Kind::Escape},
        {"Esc",        KeySpec::Kind::Escape},
        {"Enter",      KeySpec::Kind::Enter},
        {"Return",     KeySpec::Kind::Enter},
        {"Space",      KeySpec::Kind::Space},
        {"ArrowUp",    KeySpec::Kind::ArrowUp},
        {"Up",         KeySpec::Kind::ArrowUp},
        {"ArrowDown",  KeySpec::Kind::ArrowDown},
        {"Down",       KeySpec::Kind::ArrowDown},
        {"ArrowLeft",  KeySpec::Kind::ArrowLeft},
        {"ArrowRight", KeySpec::Kind::ArrowRight},
    }};

    for (const auto& [label, kind] : table) {
        if (name == label) {
            return KeySpec{kind, 0};
        }
    }
    // ArrowLeft already handled above; "Left"/"Right" aliases:
    if (name == "Left")  return KeySpec{KeySpec::Kind::ArrowLeft,  0};
    if (name == "Right") return KeySpec{KeySpec::Kind::ArrowRight, 0};

    // Function keys: F1..F99 (the rlvgl source caps at u8 but does not
    // explicitly clamp to 12; we follow that source.)
    if (name.size() >= 2 && name[0] == 'F') {
        std::string_view digits = name.substr(1);
        bool             all_digits = !digits.empty();
        for (char c : digits) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                all_digits = false;
                break;
            }
        }
        if (all_digits) {
            std::uint32_t n = 0;
            for (char c : digits) {
                n = n * 10U + static_cast<std::uint32_t>(c - '0');
                if (n > 255U) return std::nullopt;  // u8 overflow
            }
            return KeySpec{KeySpec::Kind::Function, n};
        }
    }

    // Single ASCII character — single byte single-codepoint case. The
    // rlvgl source accepts any single Unicode scalar value via .chars();
    // the parser here accepts ASCII single-byte chars and rejects
    // multi-byte UTF-8 sequences (no playit fixture exercises them).
    if (name.size() == 1) {
        return KeySpec{
            KeySpec::Kind::Character,
            static_cast<std::uint32_t>(static_cast<unsigned char>(name[0])),
        };
    }

    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Per-prefix command parsers
// ---------------------------------------------------------------------------

[[nodiscard]] std::optional<Command> parse_t(std::string_view line) noexcept {
    if (line.size() < 2) return std::nullopt;
    const char c = line[1];

    switch (c) {
        case 'K': case 'k':
            return Command{command::Inject{EventSpec{event_spec::Tick{}}}};

        case 'D': case 'd': {
            auto xy = parse_xy(line.substr(2));
            if (!xy) return std::nullopt;
            return Command{command::Inject{
                EventSpec{event_spec::PressDown{xy->x, xy->y}}}};
        }

        case 'T': case 't': {
            auto xy = parse_xy(line.substr(2));
            if (!xy) return std::nullopt;
            return Command{command::Inject{
                EventSpec{event_spec::DoubleTap{xy->x, xy->y}}}};
        }

        case '@': {
            std::string_view rest  = line.substr(2);
            auto             colon = rest.find(':');
            if (colon == std::string_view::npos) return std::nullopt;
            std::string_view tag      = rest.substr(0, colon);
            auto             xy       = parse_xy(rest.substr(colon + 1));
            if (!xy) return std::nullopt;
            return Command{command::InjectTagged{
                tag,
                EventSpec{event_spec::PressRelease{xy->x, xy->y}}}};
        }

        default:
            // T<digit>... or T-... — bare PressRelease.
            if (c == '-' || (c >= '0' && c <= '9')) {
                auto xy = parse_xy(line.substr(1));
                if (!xy) return std::nullopt;
                return Command{command::Inject{
                    EventSpec{event_spec::PressRelease{xy->x, xy->y}}}};
            }
            return std::nullopt;
    }
}

[[nodiscard]] std::optional<Command> parse_p(std::string_view line) noexcept {
    if (line.size() < 2) return std::nullopt;
    auto xy = parse_xy(line.substr(2));
    if (!xy) return std::nullopt;

    switch (line[1]) {
        case 'D': case 'd':
            return Command{command::Inject{
                EventSpec{event_spec::PointerDown{xy->x, xy->y}}}};
        case 'U': case 'u':
            return Command{command::Inject{
                EventSpec{event_spec::PointerUp{xy->x, xy->y}}}};
        case 'M': case 'm':
            return Command{command::Inject{
                EventSpec{event_spec::PointerMove{xy->x, xy->y}}}};
        default:
            return std::nullopt;
    }
}

[[nodiscard]] std::optional<Command> parse_k(std::string_view line) noexcept {
    if (line.size() < 3 || line[2] != ':') return std::nullopt;
    auto key = parse_key(line.substr(3));
    if (!key) return std::nullopt;

    switch (line[1]) {
        case 'D': case 'd':
            return Command{command::Inject{
                EventSpec{event_spec::KeyDown{*key}}}};
        case 'U': case 'u':
            return Command{command::Inject{
                EventSpec{event_spec::KeyUp{*key}}}};
        default:
            return std::nullopt;
    }
}

[[nodiscard]] std::optional<Command> parse_d(std::string_view line) noexcept {
    // D<x>,<y>,<w>,<h>[,<frames>] — mirrors rlvgl/playit/src/protocol.rs:210.
    std::array<std::int32_t, 5> parts{0, 0, 0, 0, 1};  // frames default = 1
    std::size_t  idx         = 0;
    std::int32_t value       = 0;
    bool         have_digits = false;

    for (std::size_t i = 1; i < line.size(); ++i) {
        const char b = line[i];
        if (b == ',') {
            if (have_digits && idx < parts.size()) {
                parts[idx++] = value;
            }
            value       = 0;
            have_digits = false;
        } else if (std::isdigit(static_cast<unsigned char>(b))) {
            value = static_cast<std::int32_t>(
                static_cast<std::uint32_t>(value) * 10U +
                static_cast<std::uint32_t>(b - '0'));
            have_digits = true;
        } else {
            return std::nullopt;
        }
    }
    if (have_digits && idx < parts.size()) {
        parts[idx] = value;
    }

    DumpSpec spec{};
    spec.x      = parts[0];
    spec.y      = parts[1];
    spec.width  = static_cast<std::uint16_t>(std::clamp(parts[2], 1, 40));
    spec.height = static_cast<std::uint16_t>(std::clamp(parts[3], 1, 40));
    spec.frames = static_cast<std::uint8_t>(std::clamp(parts[4], 1, 4));
    return Command{command::DumpPixels{spec}};
}

[[nodiscard]] std::optional<Command> parse_q(std::string_view line) noexcept {
    if (line.size() < 3 || line[2] != ':') return std::nullopt;
    std::string_view tag = line.substr(3);
    switch (line[1]) {
        case 'B': case 'b':
            return Command{command::QueryBounds{tag}};
        case 'E': case 'e':
            return Command{command::QueryExists{tag}};
        case 'C': case 'c':
            return Command{command::QueryChildCount{tag}};
        default:
            return std::nullopt;
    }
}

[[nodiscard]] std::optional<Command> parse_mt(std::string_view data) noexcept {
    auto count_int = parse_i32(data);
    if (!count_int) return std::nullopt;
    if (count_int->value < 1 ||
        count_int->value > static_cast<std::int32_t>(MAX_TOUCH_POINTS)) {
        return std::nullopt;
    }
    if (count_int->consumed >= data.size() ||
        data[count_int->consumed] != ':') {
        return std::nullopt;
    }
    std::string_view rest = data.substr(count_int->consumed + 1);

    event_spec::Touch touch{};
    touch.count = static_cast<std::uint8_t>(count_int->value);

    for (std::size_t i = 0; i < static_cast<std::size_t>(count_int->value); ++i) {
        if (i > 0) {
            if (rest.empty() || rest[0] != ';') return std::nullopt;
            rest.remove_prefix(1);
        }
        // id
        auto id_int = parse_i32(rest);
        if (!id_int) return std::nullopt;
        rest.remove_prefix(id_int->consumed);
        if (rest.empty() || rest[0] != ',') return std::nullopt;
        rest.remove_prefix(1);

        // state: D / U / C
        if (rest.empty()) return std::nullopt;
        TouchStateSpec state;
        switch (rest[0]) {
            case 'D': case 'd': state = TouchStateSpec::Down;    break;
            case 'U': case 'u': state = TouchStateSpec::Up;      break;
            case 'C': case 'c': state = TouchStateSpec::Contact; break;
            default: return std::nullopt;
        }
        rest.remove_prefix(1);
        if (rest.empty() || rest[0] != ',') return std::nullopt;
        rest.remove_prefix(1);

        // x,y
        auto x_int = parse_i32(rest);
        if (!x_int) return std::nullopt;
        rest.remove_prefix(x_int->consumed);
        if (rest.empty() || rest[0] != ',') return std::nullopt;
        rest.remove_prefix(1);
        auto y_int = parse_i32(rest);
        if (!y_int) return std::nullopt;
        rest.remove_prefix(y_int->consumed);

        touch.points[i] = TouchPointSpec{
            static_cast<std::uint8_t>(id_int->value),
            state,
            x_int->value,
            y_int->value,
        };
    }
    return Command{command::Inject{EventSpec{touch}}};
}

[[nodiscard]] std::optional<Command> parse_m(std::string_view line) noexcept {
    if (line.size() < 2) return std::nullopt;
    if (line[1] == 'T' || line[1] == 't') {
        return parse_mt(line.substr(2));
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<Command> parse_r(std::string_view line) noexcept {
    if (line.size() < 2) return std::nullopt;
    switch (line[1]) {
        case 'S': case 's': return Command{command::RecordStart{}};
        case 'E': case 'e': return Command{command::RecordStop{}};
        case 'D': case 'd': return Command{command::RecordDump{}};
        default:            return std::nullopt;
    }
}

}  // namespace

std::optional<Command> parse_command(std::string_view line) noexcept {
    if (line.empty()) return std::nullopt;

    switch (line[0]) {
        case '?':
            return Command{command::Status{}};
        case 'T': case 't':
            return parse_t(line);
        case 'P': case 'p':
            return parse_p(line);
        case 'K': case 'k':
            return parse_k(line);
        case 'D': case 'd':
            return parse_d(line);
        case 'Q': case 'q':
            return parse_q(line);
        case 'M': case 'm':
            return parse_m(line);
        case 'R': case 'r':
            return parse_r(line);
        case 'X': case 'x':
            return Command{command::Extension{line.substr(1)}};
        default:
            // Unknown prefix: pass through as Extension(line) — parity
            // with rlvgl/playit/src/protocol.rs:134.
            return Command{command::Extension{line}};
    }
}

}  // namespace lvglpp::playit
