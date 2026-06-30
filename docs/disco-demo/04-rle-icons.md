<!-- 04-rle-icons.md — DEMO-04 concepts doc (normative, thin). -->

# DEMO-04 — RLE icon decode (consume-only)

Status: **ratified** (2026-06-07). Thin chapter; contract inherited from
DEMO-00 (D2). RFC 2119 keywords per DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical: `rlvgl/rlvgl-decomp/src/lib.rs`
(rlvgl `v0.2.0`), the RLEC codec. Assets:
`examples/apps/disco-demo/assets/icons/*.rle`, mirrored from the rlvgl
STM32H747I-DISCO icon set.

**Scope boundary (load-bearing).** D2 resolved to port RLE *decode* up
front but **consume-only**: mirror the parser + ARGB decoder, **not** the
encoder (`write_rle_blob`/`encode`). This keeps the CLAUDE.md §
"`creator-cpp` is deferred" boundary intact - lvglpp decodes checked-in
RLE assets at runtime; it does not generate them.

## §1 Purpose

Decode the demo's RLE icon blobs to pixels so `IconStrip`/`Wing`
(DEMO-05) render pixel-faithful icons via `core::Renderer::draw_pixels`.
Wave-A, independent; unblocks DEMO-05.

## §2 Frozen contract (mirror)

RLEC blob layout (FROZEN — mirror `rlvgl-decomp/src/lib.rs:30`–`60`,
`:341`):

```
magic "RLEC" (4) | width u16 LE (2) | height u16 LE (2) |
palette_len u16 LE (2)            = 10-byte header
palette: palette_len × u16 LE (RGB565)
stream_len u32 LE (4) | RLE stream (stream_len bytes)
```

Encoding constants (FROZEN): `SHORT_REPEAT_MAX = 60`,
`ENCODE_KEY_LONG_REPEAT = 0xFD`, `LONG_REPEAT_MIN = 61`,
`LONG_REPEAT_MAX = 316`, `MAX_PALETTE = 192`. Errors (FROZEN set):
`SizeMismatch | Truncated | PaletteTooLarge | BadMagic`.

Mirrored functions (`:341`, `:382`):

```rust
pub fn parse_rle_blob(data) -> Result<(w: u16, h: u16, palette_bytes, stream), Error>
pub fn decode_argb_into(w, h, palette_rgb565: &[u16], stream, out: &mut [u8]) -> Result<(), Error>
// NOT ported: write_rle_blob, encode (generation side)
```

C++ mirror (`core/`, consume-only):

```cpp
// core/include/lvglpp/core/rle.hpp
namespace lvglpp::core::rle {
enum class Error : std::uint8_t { SizeMismatch, Truncated, PaletteTooLarge, BadMagic };

struct ParsedBlob {                  // borrows into the input span (zero-copy)
  std::uint16_t width = 0, height = 0;
  std::span<const std::uint8_t> palette_le;   // pairs of LE u16 (RGB565)
  std::span<const std::uint8_t> stream;
};
[[nodiscard]] lvglpp::expected<ParsedBlob, Error>
parse_blob(std::span<const std::uint8_t> data) noexcept;

// Decode directly into a core::Color buffer (ARGB) for Renderer::draw_pixels.
[[nodiscard]] lvglpp::expected<void, Error>
decode_into(const ParsedBlob& blob, std::span<core::Color> out) noexcept;  // out.size()==w*h
}
```

DELTA vs rlvgl: decode target is `std::span<core::Color>` (what
`Renderer::draw_pixels` consumes, `renderer.hpp`) rather than a raw
`&mut [u8]` of native `u32`; RGB565 palette → `core::Color` at decode
time. `Result`→`lvglpp::expected`. Zero-alloc, embedded-posture clean
(matches the rlvgl `no_std`/no-`alloc` decoder). No format/semantic
delta.

## §3 Files

- `core/include/lvglpp/core/rle.hpp` (new) + `core/src/rle.cpp` (new)
- `core/tests/rle_test.cpp` (new) — round-trip against a checked-in
  fixture blob: parse header, decode, compare to expected ARGB; plus the
  four error paths (bad magic, truncated header, truncated stream,
  size mismatch)
- `core/include/lvglpp/core/core.hpp` — add the include
- `core/STATUS.md` — append change-log line
- Icon assets: consumed from
  `examples/apps/disco-demo/assets/icons/*.rle`; the demo module
  (DEMO-05/06) wires the specific blobs named in
  `rlvgl/examples/apps/disco-demo/src/assets.rs` (settings/file/info +
  the 48px set).

Triangulation cite block per file
(`// PARITY: rlvgl/rlvgl-decomp/src/lib.rs` / `// LVGL: N/A (asset codec)`
/ `// DELTA: decode into core::Color span; consume-only`).

## §4 Ownership

`ParsedBlob` **borrows** into the caller's input span (zero-copy, like
the rlvgl zero-copy `ParsedRleBlob`); it MUST NOT outlive the blob
storage — documented at the type. `decode_into` writes into a
caller-owned `core::Color` span; the decoder owns nothing and allocates
nothing (DEMO-00 §5: value/borrow discipline; no handles).

## §5 Acceptance

- [ ] `parse_blob` returns correct `w/h` and zero-copy palette/stream
      spans for a known `.rle`; rejects bad magic / truncation.
- [ ] `decode_into` reproduces the reference ARGB for a fixture blob;
      `SizeMismatch` when `out.size() != w*h`.
- [ ] No heap allocation (verify under embedded-ON posture; decoder is
      `-fno-exceptions` clean).
- [ ] Encoder is **absent** (consume-only boundary held).
- [ ] `core.hpp` re-exports; Pre-Publish 0–3 green.

## §6 Change log

- _drafted_ — DEMO-04 restated from DEMO-00 §8 D2 / §14. Consume-only
  decoder; encoder explicitly out of scope.
- **2026-06-07 — ratified.** Owner directed Wave-A ratification;
  consume-only boundary (no encoder) reaffirmed. Execution may proceed.
