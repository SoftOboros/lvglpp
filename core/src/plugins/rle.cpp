// rle.cpp — consume-only RLEC icon decoder implementation.
//
// PARITY: rlvgl/rlvgl-decomp/src/lib.rs (v0.2.0 @ 79f730d) —
//         `parse_rle_blob` (lib.rs:341), `decode_argb_into` (lib.rs:382),
//         `rgb565_to_argb_u32` (lib.rs:101).
// LVGL:   N/A (asset codec).
// DELTA:  decode into core::Color span, consume-only (no encoder).

#include "lvglpp/core/rle.hpp"

#include <array>

namespace lvglpp::core::rle {
namespace {

// Read a little-endian u16 at byte offset `off`. Mirrors the
// `u16::from_le_bytes` reads in parse_rle_blob.
[[nodiscard]] std::uint16_t read_u16_le(std::span<const std::uint8_t> d,
                                        std::size_t off) noexcept {
    return static_cast<std::uint16_t>(static_cast<unsigned>(d[off]) |
                                      (static_cast<unsigned>(d[off + 1]) << 8));
}

// Read a little-endian u32 at byte offset `off` (the stream_len field).
[[nodiscard]] std::uint32_t read_u32_le(std::span<const std::uint8_t> d,
                                        std::size_t off) noexcept {
    return static_cast<std::uint32_t>(d[off]) |
           (static_cast<std::uint32_t>(d[off + 1]) << 8) |
           (static_cast<std::uint32_t>(d[off + 2]) << 16) |
           (static_cast<std::uint32_t>(d[off + 3]) << 24);
}

// Read a big-endian u16 from the stream (inline-pixel payload: hi then lo).
// Mirrors `((stream[i] as u16) << 8) | stream[i + 1]`.
[[nodiscard]] std::uint16_t read_u16_be(std::span<const std::uint8_t> d,
                                        std::size_t off) noexcept {
    return static_cast<std::uint16_t>((static_cast<unsigned>(d[off]) << 8) |
                                      static_cast<unsigned>(d[off + 1]));
}

// RGB565 -> core::Color (a=255). Mirrors rgb565_to_argb_u32 (lib.rs:101);
// the 5/6-bit channel expansion (x<<3|x>>2, x<<2|x>>4) is preserved exactly.
[[nodiscard]] core::Color rgb565_to_color(std::uint16_t c) noexcept {
    const std::uint8_t r5 = static_cast<std::uint8_t>((c >> 11) & 0x1F);
    const std::uint8_t g6 = static_cast<std::uint8_t>((c >> 5) & 0x3F);
    const std::uint8_t b5 = static_cast<std::uint8_t>(c & 0x1F);
    const std::uint8_t r  = static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2));
    const std::uint8_t g  = static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4));
    const std::uint8_t b  = static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2));
    return core::Color{r, g, b, 0xFF};
}

}  // namespace

lvglpp::expected<ParsedBlob, Error>
parse_blob(std::span<const std::uint8_t> data) noexcept {
    using namespace consts;

    // Size check before magic (matches rlvgl ordering: a 5-byte "RLEC\x01"
    // is Truncated, not BadMagic).
    if (data.size() < BLOB_HEADER_SIZE) {
        return lvglpp::unexpected(Error::Truncated);
    }
    if (data[0] != BLOB_MAGIC[0] || data[1] != BLOB_MAGIC[1] ||
        data[2] != BLOB_MAGIC[2] || data[3] != BLOB_MAGIC[3]) {
        return lvglpp::unexpected(Error::BadMagic);
    }

    const std::uint16_t width     = read_u16_le(data, 4);
    const std::uint16_t height    = read_u16_le(data, 6);
    const std::size_t palette_len = read_u16_le(data, 8);
    const std::size_t pal_bytes   = palette_len * 2;
    const std::size_t pal_end     = BLOB_HEADER_SIZE + pal_bytes;

    if (data.size() < pal_end + 4) {
        return lvglpp::unexpected(Error::Truncated);
    }
    const std::size_t stream_len   = read_u32_le(data, pal_end);
    const std::size_t stream_start = pal_end + 4;
    if (data.size() < stream_start + stream_len) {
        return lvglpp::unexpected(Error::Truncated);
    }

    ParsedBlob blob;
    blob.width      = width;
    blob.height     = height;
    blob.palette_le = data.subspan(BLOB_HEADER_SIZE, pal_bytes);
    blob.stream     = data.subspan(stream_start, stream_len);
    return blob;
}

lvglpp::expected<void, Error>
decode_into(const ParsedBlob& blob, std::span<core::Color> out) noexcept {
    using namespace consts;

    const std::size_t total = static_cast<std::size_t>(blob.width) * blob.height;
    // DELTA vs rlvgl (`out.len() < total*4`): the chapter freezes an exact
    // length contract — out.size() MUST equal width*height.
    if (out.size() != total) {
        return lvglpp::unexpected(Error::SizeMismatch);
    }

    const std::size_t palette_count = blob.palette_le.size() / 2;
    if (palette_count > MAX_PALETTE) {
        return lvglpp::unexpected(Error::PaletteTooLarge);
    }

    // Precompute the RGB565 palette as core::Color (mirrors the pal_argb
    // precompute in decode_argb_into; entries are LE u16).
    std::array<core::Color, MAX_PALETTE> pal{};
    for (std::size_t k = 0; k < palette_count; ++k) {
        pal[k] = rgb565_to_color(read_u16_le(blob.palette_le, k * 2));
    }

    const std::span<const std::uint8_t> stream = blob.stream;
    std::size_t  pos        = 0;  // pixel position
    std::uint8_t recent_idx = 0;
    std::size_t  i          = 0;

    while (i < stream.size() && pos < total) {
        const std::uint8_t b = stream[i];
        ++i;
        switch (b) {
            case ENCODE_KEY_SINGLE_INLINE_PIXEL: {
                if (i + 1 >= stream.size()) {
                    return lvglpp::unexpected(Error::Truncated);
                }
                const std::uint16_t c = read_u16_be(stream, i);
                i += 2;
                out[pos] = rgb565_to_color(c);
                ++pos;
                break;
            }
            case ENCODE_KEY_DOUBLE_INLINE_PIXEL: {
                if (i + 1 >= stream.size()) {
                    return lvglpp::unexpected(Error::Truncated);
                }
                const std::uint16_t c = read_u16_be(stream, i);
                i += 2;
                const core::Color px = rgb565_to_color(c);
                out[pos]             = px;
                ++pos;
                if (pos < total) {
                    out[pos] = px;
                    ++pos;
                }
                break;
            }
            case ENCODE_KEY_LONG_REPEAT: {
                if (i >= stream.size()) {
                    return lvglpp::unexpected(Error::Truncated);
                }
                const std::size_t add = stream[i];
                ++i;
                const std::size_t count =
                    static_cast<std::size_t>(SHORT_REPEAT_MAX) + 1U + add;
                if (static_cast<std::size_t>(recent_idx) >= palette_count) {
                    return lvglpp::unexpected(Error::Truncated);
                }
                const core::Color px = pal[recent_idx];
                for (std::size_t k = 0; k < count && pos < total; ++k) {
                    out[pos] = px;
                    ++pos;
                }
                break;
            }
            default: {
                if (static_cast<std::size_t>(b) < palette_count) {
                    recent_idx = b;
                    out[pos]   = pal[b];
                    ++pos;
                } else {
                    // short repeat: (b - palette_len + 1) copies of recent.
                    const std::size_t count =
                        static_cast<std::size_t>(b) - palette_count + 1U;
                    if (static_cast<std::size_t>(recent_idx) >= palette_count) {
                        return lvglpp::unexpected(Error::Truncated);
                    }
                    const core::Color px = pal[recent_idx];
                    for (std::size_t k = 0; k < count && pos < total; ++k) {
                        out[pos] = px;
                        ++pos;
                    }
                }
                break;
            }
        }
    }

    if (pos != total) {
        return lvglpp::unexpected(Error::SizeMismatch);
    }
    return {};
}

}  // namespace lvglpp::core::rle
