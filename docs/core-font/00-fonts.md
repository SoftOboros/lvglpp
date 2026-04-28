# 00 — Fonts

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-06**.

The key words **MUST**, **SHOULD**, **MAY** in this chapter are
interpreted per RFC 2119 and RFC 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `BitmapFont` field set, glyph storage layout, draw semantics | `rlvgl/core/src/bitmap_font.rs` (v0.2.0 @ b178cbc) | Canonical. |
| `PackedFont` + `GlyphMetric` field set, fixed-point advance, glyph lookup | `rlvgl/core/src/packed_font.rs` (v0.2.0 @ b178cbc) | Canonical. |
| `rlvgl-creator` font emission format | `rlvgl-creator fonts pack` output | External authority. lvglpp consumes these artifacts; do not redefine the binary layout here. |
| C++ surface (storage discipline, lifetime tags) | this chapter | Normative for lvglpp. |

## §1 Purpose

Define the font types every text-rendering call site uses and the
seam through which `rlvgl-creator`-emitted font data lands in lvglpp.

## §2 Problem statement

Two font shapes coexist in rlvgl:

- `BitmapFont` — fixed-width, 1-bit packed, hand-crafted ASCII font
  (the bring-up font shipped at `bitmap_font_6x10.bin`).
- `PackedFont` — variable-width, 8-bit grayscale, Unicode-capable,
  produced by `rlvgl-creator fonts pack`.

The widgets crate, the renderer, and any application code that draws
text must compile against these two shapes. They also represent the
**first rlvgl-creator → lvglpp asset path** the project will exercise
(per CLAUDE.md § "Doc Co-Location Policy" — playit-first / creator
deferred seam).

## §3 Canonical glossary

- **`BitmapFont`** — As defined in `rlvgl/core/src/bitmap_font.rs:16`.
  Mirrored as `lvglpp::core::BitmapFont`. Four fields (§5.1).
- **`PackedFont`** — As defined in `rlvgl/core/src/packed_font.rs:32`.
  Mirrored as `lvglpp::core::PackedFont`. Four fields (§5.2).
- **`GlyphMetric`** — As defined in
  `rlvgl/core/src/packed_font.rs:16`. Mirrored as
  `lvglpp::core::GlyphMetric`. Six fields (§5.2).
- **`advance_fp16`** — As defined in
  `rlvgl/core/src/packed_font.rs:24`. Horizontal advance in
  **1/16 pixels** (Q12.4 fixed-point). Used without modification.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `BitmapFont` storage layout (row-major, MSB-first 1-bpp) | `rlvgl/core/src/bitmap_font.rs:16` (canonical) | `lvglpp::core::BitmapFont`. |
| `PackedFont` glyph layout (8-bpp grayscale, sorted by codepoint for binary search) | `rlvgl/core/src/packed_font.rs:32` (canonical) | `lvglpp::core::PackedFont`. |
| Built-in fonts (`FONT_6X10` etc.) | `rlvgl/core/src/bitmap_font.rs:109` | lvglpp ships the same bring-up font referenced from the same `.bin` blob (or a copy). |
| `rlvgl-creator` emission format | `rlvgl-creator` source (informative) | lvglpp imports unchanged; layout authority lives in the creator. |

## §5 Frozen decisions

### §5.1 `BitmapFont` field set — **Standards Action**

| Field | Type | Notes |
| --- | --- | --- |
| `glyph_width` | `uint8_t` | Pixels per glyph. |
| `glyph_height` | `uint8_t` | Pixels per glyph. |
| `scale` | `uint8_t` | `1` = native; rendered glyphs scale by this factor. |
| `data` | `std::span<const std::uint8_t>` | **borrows** the glyph blob; lifetime is the application's. Mirrors rlvgl's `&'static [u8]`. |

Glyph storage: row-major, MSB-first 1-bit packing, ASCII 0x20..=0x7E
(95 glyphs).

### §5.2 `PackedFont` + `GlyphMetric` — **Standards Action**

`GlyphMetric`:

| Field | Type | Notes |
| --- | --- | --- |
| `ch` | `char32_t` | Unicode codepoint. |
| `width` | `uint16_t` | Glyph bitmap width pixels. |
| `height` | `uint16_t` | Glyph bitmap height pixels. |
| `advance_fp16` | `uint16_t` | Q12.4 fixed-point pixel advance. |
| `offset` | `uint32_t` | Byte offset into the glyph data blob. |
| `ymin` | `int16_t` | Vertical offset from baseline (positive = above). |

`PackedFont`:

| Field | Type | Notes |
| --- | --- | --- |
| `height` | `uint16_t` | Line height for layout. |
| `ascent` | `int16_t` | Distance from line top to baseline. |
| `glyphs` | `std::span<const GlyphMetric>` | **borrows**. Sorted by codepoint for binary search. |
| `data` | `std::span<const std::uint8_t>` | **borrows**. Raw 8-bpp grayscale glyph data. |

### §5.3 Storage / lifetime discipline

`BitmapFont` and `PackedFont` are **non-owning views** over font data
that lives elsewhere — typically a static array in a generated
translation unit, or a memory-mapped flash region on embedded
targets. Both struct types MUST be cheap to copy; they MUST NOT take
ownership of the glyph blob.

This means:

- The `data` / `glyphs` fields use `std::span` (the lvglpp `borrows`
  tag).
- Constructing a `PackedFont` from heap-allocated data is a caller
  responsibility; the font type does not free.
- An asset loader that produces a font from a creator-emitted blob
  returns a `(font, owning_buffer)` pair; the application MUST keep
  the buffer alive for the font's lifetime.

### §5.4 `rlvgl-creator` interop seam

A creator-emitted font lands as:

- A static array of `GlyphMetric` (sorted by `ch`).
- A static array of bytes (the `data` blob).
- A `PackedFont` aggregate referencing both via spans.

The **same generated translation unit** SHOULD work for rlvgl (which
imports it as `&'static [u8]`) and for lvglpp (which imports it as
`std::span<const std::uint8_t>`). Implementation note: rlvgl-creator
emits Rust today; the C-compatible / dual-language emission is part
of the rlvgl-creator → lvglpp asset path that warms up after CORE-06
execution lands. **Out of scope** for this chapter; sub-phase
CORE-06a / creator follow-up.

## §10 Reconciliation vs. adjacent primitives

- **LVGL `lv_font_t`.** LVGL's font system uses callbacks for
  per-glyph dispatch. lvglpp's `BitmapFont` / `PackedFont` are
  value-type views suitable for the lvglpp `Renderer`-based draw
  path. They do NOT replace `lv_font_t` in LVGL-internal contexts;
  bridges live in the platform translation unit on demand.
- **`Style`.** `Style` does NOT carry a font (CORE-05 §11). Font
  selection is per-call-site in `Widget::draw(Renderer&)`.
- **`Renderer::draw_text` (CORE-04).** The renderer's primitive is
  one signature; concrete fonts call back into `Renderer::fill_rect`
  (BitmapFont) or `Renderer::draw_pixels` (PackedFont's grayscale).

## §11 Non-goals

- **Font fallback / CSS-style font stacks.** Out of scope.
- **Subpixel anti-aliasing.** PackedFont is 8-bpp grayscale; no LCD
  subpixel support in this chapter.
- **Right-to-left text shaping.** Out of scope.
- **Font caching / glyph atlas caching.** Plain look-up only;
  caching is a CORE-05 sibling concern if it lands.
- **Live font loading from disk.** Asset loading is a creator /
  application concern; this chapter only defines the *consumed*
  shape.

## §12 Acceptance checklist

A conforming CORE-06 execution PR MUST satisfy:

- [ ] `lvglpp::core::BitmapFont` exposes the four fields in §5.1
      with `data` typed as `std::span<const std::uint8_t>`.
- [ ] `lvglpp::core::PackedFont` and `lvglpp::core::GlyphMetric`
      expose the field sets in §5.2.
- [ ] Both font types are **non-owning** (§5.3) — the `data` and
      `glyphs` fields use `std::span` and the structs hold no other
      ownership.
- [ ] `BitmapFont::draw_char(Renderer&, x, y, codepoint, color)` and
      `BitmapFont::draw_str(Renderer&, x, y, text, color)` decompose
      into `Renderer::fill_rect` calls identical to
      `rlvgl/core/src/bitmap_font.rs:39, :70`.
- [ ] `PackedFont::draw_str` walks UTF-8 input, looks up glyphs
      via binary search on `glyphs[].ch`, and renders grayscale
      glyph bitmaps via `Renderer::draw_pixels` or alpha-modulated
      `fill_rect` (matching rlvgl's `packed_font.rs` choice).
- [ ] The lvglpp build ships at least one bring-up font (mirrors
      rlvgl's `FONT_6X10`) so the smoke test can render a string
      without external assets.
- [ ] PARITY/LVGL/DELTA cite block on each public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] `core/STATUS.md` change log records ratification of CORE-06
      execution.

A conforming PR MAY:

- Inline the bring-up font's binary blob via `#embed` (C++23) or
  `unsigned char data[]` initialiser; both are conformant.

## §13 Files cited

- `rlvgl/core/src/bitmap_font.rs` (v0.2.0 @ b178cbc)
- `rlvgl/core/src/packed_font.rs` (v0.2.0 @ b178cbc)
- `rlvgl/core/src/bitmap_font_6x10.bin` (binary — bring-up font blob)
- `lvglpp/docs/core-renderer/00-renderer-trait.md`,
  `lvglpp/docs/core-widget/00-widget-tree.md`,
  `lvglpp/CLAUDE.md` § "Doc Co-Location Policy"

## §14 Unblocks

- **WID-01** (`Label`) — first widget that needs a font.
- **WID-02+** — buttons / lists with text.
- **CORE-06a** (creator interop) — once execution lands, the
  rlvgl-creator → lvglpp asset path can be exercised.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. `BitmapFont` (§5.1),
  `PackedFont` + `GlyphMetric` (§5.2), storage/lifetime discipline
  (§5.3), creator interop seam (§5.4) frozen with **Standards
  Action** registration. Execution unblocked.
- 2026-04-27 — CORE-06 execution landed. `BitmapFont`, `PackedFont`,
  `GlyphMetric` defined in `core/include/lvglpp/core/font.hpp`.
  Bring-up font `FONT_6X10` shipped at
  `core/include/lvglpp/core/fonts/font_6x10.hpp` (declaration) +
  `core/src/fonts/font_6x10.cpp` (definition) backed by
  `core/src/fonts/font_6x10.bin` (713 bytes, byte-identical to the
  rlvgl source). CMake-time hex conversion (no xxd/objcopy
  dependency). Test target `lvglpp_core_font` with 7 fixtures
  passing. Compiles clean under `LVGLPP_EMBEDDED_POSTURE=ON`.
