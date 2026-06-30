// rle_test.cpp — DEMO-04 acceptance for the consume-only RLEC decoder.
//
// Hand-builds tiny RLEC blobs with known pixels and asserts decode_into
// reproduces them, exercising every stream encoding (palette index, short
// repeat, single/double inline pixel, long repeat). Then the four frozen
// error paths, and finally a real lvglpp-owned icon asset if its path is provided.

#include "lvglpp/core/rle.hpp"

#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

#if defined(LVGLPP_RLE_ASSET_PATH)
#include <fstream>
#include <ios>
#include <iterator>
#endif

namespace lc  = lvglpp::core;
namespace rle = lvglpp::core::rle;

namespace {

// --- blob-builder helpers -------------------------------------------------

void push_u16_le(std::vector<std::uint8_t>& v, std::uint16_t x) {
    v.push_back(static_cast<std::uint8_t>(x & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
}

void push_u32_le(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(static_cast<std::uint8_t>(x & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 24) & 0xFF));
}

// Assemble a full RLEC blob: magic + w + h + palette + stream_len + stream.
std::vector<std::uint8_t> make_blob(std::uint16_t w,
                                    std::uint16_t h,
                                    const std::vector<std::uint16_t>& palette,
                                    const std::vector<std::uint8_t>& stream) {
    std::vector<std::uint8_t> blob = {'R', 'L', 'E', 'C'};
    push_u16_le(blob, w);
    push_u16_le(blob, h);
    push_u16_le(blob, static_cast<std::uint16_t>(palette.size()));
    for (std::uint16_t c : palette) push_u16_le(blob, c);
    push_u32_le(blob, static_cast<std::uint32_t>(stream.size()));
    for (std::uint8_t s : stream) blob.push_back(s);
    return blob;
}

std::span<const std::uint8_t> view(const std::vector<std::uint8_t>& v) {
    return std::span<const std::uint8_t>(v.data(), v.size());
}

// RGB565 constants used in the fixtures (5-bit-safe extremes).
constexpr std::uint16_t RED565   = 0xF800;  // -> (255,0,0)
constexpr std::uint16_t BLUE565  = 0x001F;  // -> (0,0,255)
constexpr std::uint16_t GREEN565 = 0x07E0;  // -> (0,255,0)

constexpr lc::Color RED   = lc::Color{255, 0, 0, 255};
constexpr lc::Color BLUE  = lc::Color{0, 0, 255, 255};
constexpr lc::Color GREEN = lc::Color{0, 255, 0, 255};

// --- positive decode paths ------------------------------------------------

// 2x2: palette index + short-repeat. Stream:
//   0x00 -> idx0 (red), 0x02 -> short repeat 1 (red),
//   0x01 -> idx1 (blue), 0x02 -> short repeat 1 (blue).
void test_decode_index_and_short_repeat() {
    const auto blob =
        make_blob(2, 2, {RED565, BLUE565}, {0x00, 0x02, 0x01, 0x02});
    auto parsed = rle::parse_blob(view(blob));
    assert(parsed.has_value());
    assert(parsed.value().width == 2);
    assert(parsed.value().height == 2);
    assert(parsed.value().palette_le.size() == 4);  // 2 entries * 2 bytes

    std::vector<lc::Color> out(4);
    auto r = rle::decode_into(parsed.value(), std::span<lc::Color>(out));
    assert(r.has_value());
    assert(out[0] == RED);
    assert(out[1] == RED);
    assert(out[2] == BLUE);
    assert(out[3] == BLUE);
}

// 3x1 with empty palette: single inline (0xFF) then double inline (0xFE).
void test_decode_inline_pixels() {
    std::vector<std::uint8_t> stream = {
        0xFF, 0xF8, 0x00,  // single inline red  (hi,lo)
        0xFE, 0x00, 0x1F,  // double inline blue (hi,lo)
    };
    const auto blob = make_blob(3, 1, {}, stream);
    auto parsed     = rle::parse_blob(view(blob));
    assert(parsed.has_value());

    std::vector<lc::Color> out(3);
    auto r = rle::decode_into(parsed.value(), std::span<lc::Color>(out));
    assert(r.has_value());
    assert(out[0] == RED);
    assert(out[1] == BLUE);
    assert(out[2] == BLUE);
}

// 62x1, all green: idx0 (1px) then long repeat (0xFD, add=0 -> 61px).
void test_decode_long_repeat() {
    const auto blob = make_blob(62, 1, {GREEN565}, {0x00, 0xFD, 0x00});
    auto parsed     = rle::parse_blob(view(blob));
    assert(parsed.has_value());

    std::vector<lc::Color> out(62);
    auto r = rle::decode_into(parsed.value(), std::span<lc::Color>(out));
    assert(r.has_value());
    for (const lc::Color& px : out) assert(px == GREEN);
}

// --- error paths ----------------------------------------------------------

void test_bad_magic() {
    const std::vector<std::uint8_t> data = {'N', 'O', 'P', 'E', 0,
                                            0,   0,   0,   0,   0};
    auto parsed = rle::parse_blob(view(data));
    assert(!parsed.has_value());
    assert(parsed.error() == rle::Error::BadMagic);
}

void test_truncated_header() {
    const std::vector<std::uint8_t> data = {'R', 'L', 'E', 'C', 0x01};
    auto parsed = rle::parse_blob(view(data));
    assert(!parsed.has_value());
    assert(parsed.error() == rle::Error::Truncated);
}

// Header declares a 10-byte stream but only 2 bytes follow.
void test_truncated_stream() {
    std::vector<std::uint8_t> data = {'R', 'L', 'E', 'C'};
    push_u16_le(data, 2);   // width
    push_u16_le(data, 2);   // height
    push_u16_le(data, 0);   // palette_len
    push_u32_le(data, 10);  // stream_len (lies)
    data.push_back(0x00);
    data.push_back(0x00);
    auto parsed = rle::parse_blob(view(data));
    assert(!parsed.has_value());
    assert(parsed.error() == rle::Error::Truncated);
}

// Inline-pixel key with a missing payload byte -> decode-time Truncated.
void test_decode_truncated_inline() {
    const auto blob = make_blob(1, 1, {}, {0xFF, 0xF8});  // missing lo byte
    auto parsed     = rle::parse_blob(view(blob));
    assert(parsed.has_value());
    std::vector<lc::Color> out(1);
    auto r = rle::decode_into(parsed.value(), std::span<lc::Color>(out));
    assert(!r.has_value());
    assert(r.error() == rle::Error::Truncated);
}

// out span length != width*height -> SizeMismatch.
void test_size_mismatch() {
    const auto blob =
        make_blob(2, 2, {RED565, BLUE565}, {0x00, 0x02, 0x01, 0x02});
    auto parsed = rle::parse_blob(view(blob));
    assert(parsed.has_value());
    std::vector<lc::Color> too_small(3);  // need 4
    auto r = rle::decode_into(parsed.value(), std::span<lc::Color>(too_small));
    assert(!r.has_value());
    assert(r.error() == rle::Error::SizeMismatch);
}

// --- real asset -----------------------------------------------------------

void test_real_asset() {
#if defined(LVGLPP_RLE_ASSET_PATH)
    std::ifstream f(LVGLPP_RLE_ASSET_PATH, std::ios::binary);
    if (!f) {
        return;  // asset unavailable in this checkout; skip gracefully.
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                                    std::istreambuf_iterator<char>());
    assert(bytes.size() >= rle::consts::BLOB_HEADER_SIZE);

    auto parsed = rle::parse_blob(view(bytes));
    assert(parsed.has_value());
    const auto& blob = parsed.value();
    assert(blob.width > 0 && blob.width <= 1024);
    assert(blob.height > 0 && blob.height <= 1024);

    const std::size_t total =
        static_cast<std::size_t>(blob.width) * blob.height;
    std::vector<lc::Color> out(total);
    auto r = rle::decode_into(blob, std::span<lc::Color>(out));
    assert(r.has_value());
    // Decoder fills the whole buffer with opaque pixels.
    for (const lc::Color& px : out) assert(px.a == 255);
#endif
}

}  // namespace

int main() {
    test_decode_index_and_short_repeat();
    test_decode_inline_pixels();
    test_decode_long_repeat();
    test_bad_magic();
    test_truncated_header();
    test_truncated_stream();
    test_decode_truncated_inline();
    test_size_mismatch();
    test_real_asset();
    return 0;
}
