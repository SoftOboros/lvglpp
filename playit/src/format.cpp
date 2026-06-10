// format.cpp — Response → ASCII line.
//
// PARITY: rlvgl/playit/src/protocol.rs:466 (format_response) and the
//         BufWriter helpers at lines 510+.
//
// All helpers are local; allocation-free, noexcept.

#include "lvglpp/playit/format.hpp"

#include <array>
#include <climits>
#include <cstdint>
#include <string_view>
#include <variant>

namespace lvglpp::playit {

namespace {

// Tiny no-alloc buffer writer — silently truncates beyond `buf.size()`.
// Mirrors rlvgl/playit/src/protocol.rs:514.
struct BufWriter {
    std::span<char> buf;
    std::size_t     pos = 0;

    constexpr void write_byte(char b) noexcept {
        if (pos < buf.size()) {
            buf[pos] = b;
            ++pos;
        }
    }

    constexpr void write_str(std::string_view s) noexcept {
        for (char b : s) {
            write_byte(b);
        }
    }

    void write_i32(std::int32_t v) noexcept {
        if (v < 0) {
            write_byte('-');
            // Handle INT32_MIN without UB: convert via uint32_t then
            // negate-as-unsigned.
            std::uint32_t u = static_cast<std::uint32_t>(
                                  -static_cast<std::int64_t>(v));
            write_uint(u);
        } else {
            write_uint(static_cast<std::uint32_t>(v));
        }
    }

    void write_uint(std::uint32_t v) noexcept {
        // Up to 10 decimal digits for u32. Fill backwards into a
        // scratch then flush.
        std::array<char, 10> scratch{};
        std::size_t          n = 0;
        if (v == 0) {
            write_byte('0');
            return;
        }
        while (v > 0 && n < scratch.size()) {
            scratch[n++] = static_cast<char>('0' + (v % 10U));
            v /= 10U;
        }
        // Emit in reverse — digits were pushed least-significant first.
        for (std::size_t i = n; i > 0; --i) {
            write_byte(scratch[i - 1]);
        }
    }
};

}  // namespace

std::size_t format_response(const Response& resp, std::span<char> buf) noexcept {
    BufWriter w{buf};

    std::visit([&w](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, response::Ok>) {
            w.write_str(std::string_view{"OK"});
        }
        else if constexpr (std::is_same_v<T, response::Error>) {
            w.write_str(std::string_view{"ERR: "});
            w.write_str(payload.reason);
        }
        else if constexpr (std::is_same_v<T, response::Bounds>) {
            w.write_str(std::string_view{"BOUNDS:"});
            w.write_i32(payload.x);      w.write_byte(',');
            w.write_i32(payload.y);      w.write_byte(',');
            w.write_i32(payload.width);  w.write_byte(',');
            w.write_i32(payload.height);
        }
        else if constexpr (std::is_same_v<T, response::Exists>) {
            w.write_str(std::string_view{"EXISTS:"});
            w.write_byte(payload.value ? '1' : '0');
        }
        else if constexpr (std::is_same_v<T, response::ChildCount>) {
            w.write_str(std::string_view{"CHILDREN:"});
            w.write_uint(static_cast<std::uint32_t>(payload.value));
        }
        else if constexpr (std::is_same_v<T, response::Status>) {
            w.write_str(std::string_view{"STAT:"});
            w.write_uint(payload.snapshot.tick_count);
            w.write_byte(',');
            w.write_uint(payload.snapshot.present_count);
        }
        else if constexpr (std::is_same_v<T, response::DumpEnd>) {
            w.write_str(std::string_view{"END"});
        }
        else {
            static_assert(sizeof(T) == 0,
                "format_response: unhandled Response variant — "
                "PLAYIT-04b §5.1 added a variant without updating "
                "this seam");
        }
    }, resp);

    w.write_str(std::string_view{"\r\n"});
    return w.pos;
}

// ---------------------------------------------------------------------------
// format_event_spec — PLAYIT-06 §5.2
// ---------------------------------------------------------------------------

namespace {

void write_key_spec(BufWriter& w, const KeySpec& key) noexcept {
    switch (key.kind) {
        case KeySpec::Kind::Escape:     w.write_str(std::string_view{"Escape"});     return;
        case KeySpec::Kind::Enter:      w.write_str(std::string_view{"Enter"});      return;
        case KeySpec::Kind::Space:      w.write_str(std::string_view{"Space"});      return;
        case KeySpec::Kind::ArrowUp:    w.write_str(std::string_view{"ArrowUp"});    return;
        case KeySpec::Kind::ArrowDown:  w.write_str(std::string_view{"ArrowDown"});  return;
        case KeySpec::Kind::ArrowLeft:  w.write_str(std::string_view{"ArrowLeft"});  return;
        case KeySpec::Kind::ArrowRight: w.write_str(std::string_view{"ArrowRight"}); return;
        case KeySpec::Kind::Function:
            w.write_byte('F');
            w.write_uint(key.value);
            return;
        case KeySpec::Kind::Character:
            // ASCII single-byte fast path. Multi-byte UTF-8 deferred
            // (PLAYIT-06 §5.2 — out-of-range Character falls back to
            // the Other-style decimal path so the dump is parseable).
            if (key.value >= 0x20 && key.value <= 0x7E) {
                w.write_byte(static_cast<char>(key.value));
            } else {
                w.write_uint(key.value);
            }
            return;
        case KeySpec::Kind::Other:
            w.write_uint(key.value);
            return;
    }
}

}  // namespace

std::size_t format_event_spec(const EventSpec& spec,
                              std::span<char> buf) noexcept {
    BufWriter w{buf};

    std::visit([&w](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;

        if constexpr (std::is_same_v<T, event_spec::Tick>) {
            w.write_str(std::string_view{"TK"});
        }
        else if constexpr (std::is_same_v<T, event_spec::PressRelease>) {
            w.write_byte('T');
            w.write_i32(payload.x); w.write_byte(','); w.write_i32(payload.y);
        }
        else if constexpr (std::is_same_v<T, event_spec::PressDown>) {
            w.write_str(std::string_view{"TD"});
            w.write_i32(payload.x); w.write_byte(','); w.write_i32(payload.y);
        }
        else if constexpr (std::is_same_v<T, event_spec::DoubleTap>) {
            w.write_str(std::string_view{"TT"});
            w.write_i32(payload.x); w.write_byte(','); w.write_i32(payload.y);
        }
        else if constexpr (std::is_same_v<T, event_spec::PointerDown>) {
            w.write_str(std::string_view{"PD"});
            w.write_i32(payload.x); w.write_byte(','); w.write_i32(payload.y);
        }
        else if constexpr (std::is_same_v<T, event_spec::PointerUp>) {
            w.write_str(std::string_view{"PU"});
            w.write_i32(payload.x); w.write_byte(','); w.write_i32(payload.y);
        }
        else if constexpr (std::is_same_v<T, event_spec::PointerMove>) {
            w.write_str(std::string_view{"PM"});
            w.write_i32(payload.x); w.write_byte(','); w.write_i32(payload.y);
        }
        else if constexpr (std::is_same_v<T, event_spec::KeyDown>) {
            w.write_str(std::string_view{"KD:"});
            write_key_spec(w, payload.key);
        }
        else if constexpr (std::is_same_v<T, event_spec::KeyUp>) {
            w.write_str(std::string_view{"KU:"});
            write_key_spec(w, payload.key);
        }
        else if constexpr (std::is_same_v<T, event_spec::Touch>) {
            w.write_str(std::string_view{"MT"});
            w.write_uint(static_cast<std::uint32_t>(payload.count));
            w.write_byte(':');
            for (std::size_t i = 0; i < payload.count && i < MAX_TOUCH_POINTS; ++i) {
                if (i > 0) w.write_byte(';');
                const auto& p = payload.points[i];
                w.write_uint(static_cast<std::uint32_t>(p.id));
                w.write_byte(',');
                switch (p.state) {
                    case TouchStateSpec::Down:    w.write_byte('D'); break;
                    case TouchStateSpec::Up:      w.write_byte('U'); break;
                    case TouchStateSpec::Contact: w.write_byte('C'); break;
                }
                w.write_byte(',');
                w.write_i32(p.x);
                w.write_byte(',');
                w.write_i32(p.y);
            }
        }
        else {
            static_assert(sizeof(T) == 0,
                "format_event_spec: unhandled EventSpec variant");
        }
    }, spec);

    return w.pos;
}

}  // namespace lvglpp::playit

namespace lvglpp::playit {

std::size_t format_hex_u32(std::uint32_t value,
                           std::span<char> buf) noexcept {
    // PARITY: rlvgl/playit/src/protocol.rs:566 — 8 uppercase digits,
    // MSB first, truncate when the buffer is short.
    constexpr char HEX[] = "0123456789ABCDEF";
    std::size_t n = 0;
    for (int i = 7; i >= 0; --i) {
        if (n >= buf.size()) break;
        buf[n++] = HEX[(value >> (static_cast<unsigned>(i) * 4u)) & 0xFu];
    }
    return n;
}

}  // namespace lvglpp::playit
