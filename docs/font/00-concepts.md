# 00 — Font selection & anti-aliased text

Chapter status: **ratified 2026-06-15**.
Phase code: **FONT-00**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD NOT**,
**MAY**, and **RECOMMENDED** in this chapter are interpreted per RFC 2119 and
RFC 8174.

This chapter is the normative artifact for lvglpp font *selection*. It builds
on the LPAR-08 `Font` handle ([`../core-draw/00-text-draw-image-mask.md`](../core-draw/00-text-draw-image-mask.md))
and the LPAR-07 style cascade ([`../core-style/01-style-cascade-theme.md`](../core-style/01-style-cascade-theme.md)).
The [initiative README](./README.md) is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Font-handle selection model (`WidgetFont`/`set_font` **intent**), AA-coverage contract | rlvgl `v0.2.4` `docs/concepts/FONT-00-CONCEPTS.md` (@ `343f596`) |
| The font **primitive** + glyph rasterization | `lvgl/src/font/lv_font.h` (`lv_font_t`, `lv_font_get_glyph_dsc`, `lv_font_get_default`), built-in `lv_font_montserrat_*` |
| Font as a **style/cascade property** | `lvgl/src/core/lv_obj_style.h` + `lv_obj_style_gen.h` (`lv_obj_set_style_text_font`, `lv_obj_get_style_text_font`), `lv_style_gen.h` (`lv_style_set_text_font`) |
| Existing `Font` handle | `core/include/lvglpp/core/draw.hpp` (LPAR-08) — extended here, not replaced |
| Existing value-type fonts (`BitmapFont`/`PackedFont`) | `docs/core-font/00-fonts.md` (CORE-06) — reconciled in §10 |

If FONT-00 changes a frozen decision below, §15 MUST be amended first in a
separate docs change. Where the lvglpp behavior cannot mirror rlvgl because
LVGL owns the path natively, that is recorded as a DELTA, not a fork.

## §1 Purpose

Make any lvglpp `Object` render **selectable, anti-aliased** text. The glyph
*pipeline* already works in LVGL (it rasterizes A1/A2/A4/A8 coverage and
blends it), so unlike rlvgl, lvglpp does **not** add a coverage path. It adds:

1. **Font selection** (§5): assign any `lv_font_t` to an object or style via
   the cascade, and resolve the effective font of an object.
2. **AA built-ins** (§6): expose LVGL's built-in anti-aliased
   `lv_font_montserrat_*` fonts and an AA/glyph-format query on the `Font`
   handle, so a caller can pick a real AA font and confirm it is AA.

## §2 Problem statement

State as of 2026-06-15:

- The LPAR-08 `Font` handle (`core/include/lvglpp/core/draw.hpp:38`) wraps
  `const lv_font_t*` with `default_font()`, `glyph_advance()`,
  `line_height()` only. There is **no** way to (a) name a built-in AA font,
  (b) ask whether a font is anti-aliased, or (c) read glyph metrics beyond
  advance.
- The LPAR-07 cascade (`Object::set_local_*`, `style::Style::set_*`) wraps
  `bg`/`border`/`radius`/`text_color` but **not** `text_font`. There is no
  `set_text_font` on `Style` and no `set_local_text_font` /
  `text_font()` on `Object`, so no object can be given a font through the
  wrapper today.
- rlvgl's FONT-00 §2.1 problem ("every widget hard-codes 1-bit `FONT_6X10`,
  no `set_font`") does not exist in LVGL form: LVGL widgets default to
  `LV_FONT_DEFAULT` (a 4-bit AA montserrat) and already read the cascade
  font. The lvglpp gap is purely that the **wrapper** does not expose the
  selection knob. This chapter closes that.

## §3 Canonical glossary

- **`Font`** — As defined in `core/include/lvglpp/core/draw.hpp:38` (LPAR-08);
  **adapted: extended** with `builtin()`, `glyph_metrics()`, `is_anti_aliased()`,
  and `base_line()`. Still a non-owning handle over `const lv_font_t*`
  (observes; the font is external/static and MUST outlive the handle).
- **`BuiltinFont`** — Owned by this chapter; an enum naming LVGL's built-in
  `lv_font_montserrat_*` sizes. `Font::builtin(BuiltinFont)` returns the font
  if its `LV_FONT_MONTSERRAT_<n>` build flag is enabled, else an **empty
  `Font`** (so the caller falls back to `default_font()`). **Specification
  Required** (local enum; no cross-language contract — rlvgl has no built-in
  size enum).
- **`GlyphMetrics`** — Owned by this chapter; the subset of
  `lv_font_glyph_dsc_t` a caller needs for layout: `adv_w`, `box_w`, `box_h`,
  `ofs_x`, `ofs_y`. Mirrors the *intent* of rlvgl `FontMetrics::glyph_metrics`
  (`rlvgl/core/src/font.rs`) but is populated by `lv_font_get_glyph_dsc`.
- **`Style::set_text_font` / `Object::set_local_text_font`** — wrap
  `lv_style_set_text_font` / `lv_obj_set_style_text_font`; the `Font` is
  borrowed (LVGL stores the `lv_font_t*`, so it MUST outlive every object
  using it — the same outlives-objects rule as a shared `style::Style`).
- **`Object::text_font()`** — wraps `lv_obj_get_style_text_font(obj,
  LV_PART_MAIN)`; returns the **resolved** effective font (cascade + theme +
  inheritance), i.e. the answer to "which font does this object actually
  draw with". This is the LVGL-native realization of rlvgl's FONT-00 §5
  resolution step.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Glyph rasterization / AA coverage / blend | `lvgl` (`lv_font_t` + draw pipeline) — **native; lvglpp wraps, never reimplements** |
| Which `lv_font_t` an object uses | `lvgl` cascade (`lv_obj_set/get_style_text_font`) — lvglpp wraps |
| `BuiltinFont` size set | this chapter (Specification Required) |
| `GlyphMetrics` field subset | this chapter; values from `lv_font_get_glyph_dsc` |
| AA / glyph-format meaning (A1/A2/A4/A8) | `lvgl` `lv_font_glyph_format_t` |

## §5 Frozen decisions — selection

1. Font is selected through the **cascade**, never by mutating a widget field:
   `Style::set_text_font(const Font&)` and
   `Object::set_local_text_font(const Font&, Selector)` are the only write
   paths; `Object::text_font()` is the resolve/read path.
2. The selected `Font` is **borrowed into LVGL** (the `lv_font_t*` is stored,
   not copied): a `Font` set on a live object MUST outlive that object. This
   is the same load-bearing rule as a shared `style::Style` (LPAR-07 §5.1);
   built-in fonts (static storage) and a `Font` held by the caller both
   satisfy it.
3. `default_font()` (`lv_font_get_default`, = `LV_FONT_DEFAULT`) is the
   fallback whenever no font is selected and whenever `builtin()` is asked for
   a size that is not compiled in.

## §6 Frozen decisions — anti-aliased text

4. lvglpp adds **no** coverage/AA rasterization. AA is a property of the font
   asset: LVGL's built-in `lv_font_montserrat_*` are 4-bit (A4) AA; the
   default font is AA. `Font::is_anti_aliased()` reports whether a
   representative glyph's format is a multi-bit coverage format (A2/A3/A4/A8),
   i.e. not 1-bit A1. This is a **query**, not a renderer switch.
5. `BuiltinFont` enumerates the montserrat sizes LVGL can ship
   (12/14/16/18/24/28/48 — the common set); `builtin()` returns an empty
   `Font` for a size whose `LV_FONT_MONTSERRAT_<n>` flag is off. Which sizes
   are enabled is a build-flag concern documented in
   `core/OPTIONS.md` / `lv_conf.h`; the enum is stable regardless.

## §10 Reconciliation vs. adjacent primitives

- **CORE-06 `BitmapFont`/`PackedFont`/`FONT_6X10`** — the hand-rolled value
  types remain for the not-yet-migrated widgets/renderer (same posture as the
  CORE-05 `Style` reconciliation, LPAR-07 §10). They are **not** deleted here.
  When `LVGLPP-WRAP` migrates the widgets onto `Object`, the bring-up
  `FONT_6X10` either becomes an `lv_font_t` or is retired (LPAR-00 §8 conflict
  row). FONT-00 owns the `lv_font_t` selection path; it does not touch the
  value types.
- **LPAR-08 `Font`** — extended in place (new accessors), not replaced; the
  `glyph_advance`/`line_height` surface is unchanged.
- **rlvgl FONT-00 §7/§8/§9 (ArcLabel, rotated renderer, AA fixture)** — no
  lvglpp execution phase: LVGL has no `ArcLabel`; the rotated path is
  platform/verification-blocked (disco); the AA fixture is LPAR-16
  conformance. Recorded in the README reframe table.

## §11 Non-goals

- FreeType / TinyTTF dynamic font loading (a later `lv_freetype`/`lv_tiny_ttf`
  seam, LPAR-00 §7 FONT row) — not in this wave.
- Per-widget convenience `set_font` on concrete widget wrappers — those land
  with each widget phase; FONT-00 provides the `Object`-level primitive they
  call.
- The `FontId` registry — owned by FONT-05.

## §12 Acceptance checklist

- [x] `Font` gains `builtin(BuiltinFont)`, `glyph_metrics(codepoint)`,
      `is_anti_aliased()`, `base_line()`; empty-safe (return empty/0).
      `core/include/lvglpp/core/draw.hpp` + `core/src/draw.cpp`.
- [x] `BuiltinFont` enum + `builtin()` returns the compiled-in font or an
      empty `Font` (per-size `#if LV_FONT_MONTSERRAT_<n>`); `default_font()`
      fallback documented in the header + §5.3.
- [x] `Style::set_text_font(const Font&)`
      (`style_cascade.hpp`), `Object::set_local_text_font(const Font&,
      Selector)` + `Object::text_font()` cascade resolution
      (`object.{hpp,cpp}`, over `lv_obj_set/get_style_text_font`).
- [x] The borrowed-font outlives-objects rule documented
      (`borrows`-into-LVGL) on every setter.
- [x] Builds + tests under both postures
      (`core/tests/font_select_test.cpp`, host + `LVGLPP_EMBEDDED_POSTURE`);
      `core/STATUS.md` records FONT-00.

## §13 Files cited

- `rlvgl/docs/concepts/FONT-00-CONCEPTS.md` (v0.2.4 @ `343f596`)
- `lvgl/src/font/lv_font.h`; built-in `lv_font_montserrat_*`
- `lvgl/src/core/lv_obj_style_gen.h`, `lvgl/src/misc/lv_style_gen.h`
- `core/include/lvglpp/core/draw.hpp` (LPAR-08 `Font`)
- `core/include/lvglpp/core/style_cascade.hpp`, `object.hpp` (LPAR-07)

## §14 Unblocks

- FONT-05 (registry feeds these selection setters).
- Every widget phase (per-widget `set_font` calls the `Object` primitive).

## §15 Change log

- **2026-06-15** — FONT-00 drafted: extend the `Font` handle (built-ins,
  glyph metrics, AA query), add cascade font selection
  (`set_text_font`/`set_local_text_font`/`text_font`). Reframe vs. rlvgl
  FONT-00 (LVGL owns rasterization/AA natively) recorded in the README.
- **2026-06-15** — ratified by owner ("ratified - proceed"); execution
  unblocked (gated on LPAR-07/LPAR-08, both landed).
