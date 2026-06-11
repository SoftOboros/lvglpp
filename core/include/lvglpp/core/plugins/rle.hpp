// rle.hpp — consume-only RLEC icon decoder.
//
// PARITY: rlvgl/rlvgl-decomp/src/lib.rs (v0.2.0 @ 79f730d).
// LVGL:   N/A (asset codec).
// DELTA:  decode into core::Color span, consume-only.
//
// This file implements docs/disco-demo/04-rle-icons.md (DEMO-04),
// gated per docs/core-plugins/01-rle.md (CORE-07n). It
// mirrors the rlvgl-decomp parser (`parse_rle_blob`, lib.rs:341) and the
// ARGB decode loop (`decode_argb_into`, lib.rs:382), and ONLY those. The
// encoder side (`write_rle_blob`, `encode_rgba`) is deliberately NOT
// ported: lvglpp consumes rlvgl-produced assets at runtime, it does not
// generate them (CLAUDE.md § "`creator-cpp` is deferred"). Frozen
// constants and the error set are Standards Action (must match rlvgl).

#ifndef LVGLPP_CORE_RLE_HPP
#define LVGLPP_CORE_RLE_HPP

// CORE-07 Â§5.2 gating guard: this is a plugin header (CORE-07n);
// consumers must enable LVGLPP_CORE_RLE.
#if !defined(LVGLPP_CORE_RLE) || LVGLPP_CORE_RLE == 0
#error "plugins/rle.hpp included without LVGLPP_CORE_RLE enabled"
#endif

#include <cstddef>
#include <cstdint>
#include <span>

#include "lvglpp/core/widget.hpp"   // core::Color
#include "lvglpp/std/expected.hpp"  // lvglpp::expected / lvglpp::unexpected

namespace lvglpp::core::rle {

// Frozen encoding constants. Mirror rlvgl/rlvgl-decomp/src/lib.rs:27-39
// (`mod consts`). These are part of a cross-language contract; adding or
// changing a value is Standards Action (must agree with rlvgl).
namespace consts {

// Control keys in the RLE byte stream.
inline constexpr std::uint8_t ENCODE_KEY_SINGLE_INLINE_PIXEL = 0xFF;
inline constexpr std::uint8_t ENCODE_KEY_DOUBLE_INLINE_PIXEL = 0xFE;
inline constexpr std::uint8_t ENCODE_KEY_LONG_REPEAT         = 0xFD;

// Short-repeat data bytes encode runs of 1..=60 pixels.
inline constexpr std::uint8_t SHORT_REPEAT_MAX = 60;
// Long repeats cover 61..=316 pixels (kept for contract completeness; the
// decoder derives counts directly from SHORT_REPEAT_MAX).
inline constexpr std::uint16_t LONG_REPEAT_MIN = SHORT_REPEAT_MAX + 1U;  // 61
inline constexpr std::uint16_t LONG_REPEAT_MAX = 316;
// Palette cap keeps the short-repeat range below ENCODE_KEY_LONG_REPEAT.
inline constexpr std::size_t MAX_PALETTE = 192;

// RLEC blob magic and fixed header size:
//   magic(4) + width(2) + height(2) + palette_len(2) = 10 bytes.
inline constexpr std::uint8_t BLOB_MAGIC[4]    = {'R', 'L', 'E', 'C'};
inline constexpr std::size_t  BLOB_HEADER_SIZE = 10;

}  // namespace consts

// Error set for parse/decode. FROZEN (Standards Action) — mirrors
// rlvgl/rlvgl-decomp/src/lib.rs:42 `enum Error`. PaletteTooLarge is raised
// by the decoder when a blob's palette exceeds MAX_PALETTE (the encoder,
// which is the other rlvgl raiser, is not ported here).
enum class Error : std::uint8_t {
    SizeMismatch,     // out.size() != width*height (or stream under-fills)
    Truncated,        // header/stream/inline-pixel terminated prematurely
    PaletteTooLarge,  // palette entry count exceeds MAX_PALETTE
    BadMagic,         // first four bytes are not "RLEC"
};

// Zero-copy view of a parsed RLEC blob.
//
// Ownership (DEMO-04 §4 / DEMO-00 §5): ParsedBlob *borrows* into the
// caller's input span. `palette_le` and `stream` are non-owning views into
// the original blob storage and MUST NOT outlive it. The decoder owns
// nothing and allocates nothing.
struct ParsedBlob {
    std::uint16_t width  = 0;
    std::uint16_t height = 0;
    // borrows: palette as consecutive little-endian u16 RGB565 pairs.
    std::span<const std::uint8_t> palette_le;
    // borrows: the RLE byte stream.
    std::span<const std::uint8_t> stream;
};

// Parse an RLEC blob header into a zero-copy ParsedBlob.
//
// Mirrors rlvgl/rlvgl-decomp/src/lib.rs:341 `parse_rle_blob`.
//
// Args:
//   data: borrows the full blob bytes for the duration of the call; the
//         returned ParsedBlob continues to borrow from it (caller keeps it
//         alive).
// Returns:
//   ParsedBlob borrowing into `data`, or an Error (BadMagic / Truncated).
[[nodiscard]] lvglpp::expected<ParsedBlob, Error>
parse_blob(std::span<const std::uint8_t> data) noexcept;

// Decode a parsed blob into a caller-owned core::Color buffer (ARGB, a=255).
//
// Mirrors rlvgl/rlvgl-decomp/src/lib.rs:382 `decode_argb_into`. Palette
// entries are read as little-endian u16 RGB565 and converted to core::Color
// at decode time (the DELTA vs rlvgl's native-u32 target). Zero allocation;
// safe under -fno-exceptions (never throws).
//
// Args:
//   blob: borrows the parsed view; its palette/stream spans must still be
//         valid (i.e. the original blob storage is alive).
//   out:  borrows a caller-owned, writable pixel buffer. out.size() MUST
//         equal blob.width*blob.height, else SizeMismatch.
// Returns:
//   empty success, or an Error (SizeMismatch / Truncated / PaletteTooLarge).
[[nodiscard]] lvglpp::expected<void, Error>
decode_into(const ParsedBlob& blob, std::span<core::Color> out) noexcept;

}  // namespace lvglpp::core::rle

#endif  // LVGLPP_CORE_RLE_HPP
