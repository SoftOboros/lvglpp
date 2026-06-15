# 00 — Text, draw, image & mask

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-08**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. [`../lpar/README.md`](../lpar/README.md)
is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Text/draw/image/mask **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md` (@ `343f596`) |
| The **primitive** | `lvgl/src/draw/lv_draw.h` + `lv_draw_rect/label/image/line/arc`, `lvgl/src/font/lv_font.h`, `lvgl/src/draw/lv_image_dsc.h`, `lv_image_decoder.h` |
| Existing font/renderer/draw-helpers | CORE-04/04a/06 (superseded) |

## §1 Purpose

Wrap LVGL's draw layer: glyph metrics + text wrapping (`lv_font_*`), draw
primitives (`lv_draw_rect`/`label`/`image`/`line`/`arc`), image
descriptors + decoders (`lv_image_dsc_t`, `lv_image_decoder_t`), and
mask/layer compositing. This is the substrate the text-bearing and visual
widgets draw through.

## §2 Problem statement

rlvgl re-implements glyph metrics, shaped text, draw primitives, masks,
gradients, and an image descriptor/cache (`core::font`, `core::renderer`,
`core::image`, `core::mask`, `core::draw`) because it has no LVGL draw
engine. lvglpp has one. It wraps the `lv_draw_*`/`lv_font_*`/`lv_image_*`
surface and retires the hand-rolled CORE-04 `Renderer`, CORE-04a
`draw_widget_bg`/`draw_border_straight`, and the CORE-06 `BitmapFont`/
`PackedFont`/`FONT_6X10` bring-up path (kept only as an optional embedded
`lv_font_t`).

## §3 Canonical glossary

- **`Font`** — non-owning view/handle over an `lv_font_t*` (LVGL fonts are
  typically static/`external`). Glyph metrics via `lv_font_get_glyph_dsc`.
  FONT-00..05 (`docs/font/`) build the selection + registry layer on this.
- **Draw helpers** — thin wrappers over `lv_draw_rect`/`label`/`image`/
  `line`/`arc` taking an `lv_layer_t*` (draw context); used by custom-draw
  widgets (Canvas, Scale, Chart).
- **`ImageDescriptor`** — value/handle over `lv_image_dsc_t`; image
  sources resolved via `lv_image_decoder_t` (LPAR-09 supplies the source).
- **Mask / layer** — wraps LVGL's layer + mask compositing for clipped /
  alpha draws.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| Glyph metrics, draw primitive behavior | `lvgl` draw engine + rlvgl `LPAR-08` semantics |
| Image pixel/color formats | `lvgl` `lv_color_format_t` ↔ lvglpp `ColorFormat` (DEMO-0S) — **Standards Action** |
| Font selection (`WidgetFont`/registry) | `docs/font/` (FONT-00..05) |

## §5 Frozen decisions

1. lvglpp does NOT re-implement rasterization, glyph metrics, masks, or
   gradients — it wraps `lv_draw_*`/`lv_font_*`.
2. `lv_font_t` is `external`/static; the `Font` wrapper observes, it does
   not own (FreeType/TTF dynamic fonts, FONT family, are the exception
   and own their `lv_font_t`).
3. Draw helpers take an `lv_layer_t*` draw context and are used only by
   widgets that custom-draw; ordinary widgets draw via their `lv_*`
   widget type.
4. `ColorFormat` (DEMO-0S) reconciles 1:1 with `lv_color_format_t`
   (**Standards Action**).

## §10 Reconciliation vs. adjacent primitives

- **CORE-04 `Renderer` / CORE-04a draw helpers** — superseded by the
  LVGL draw pipeline.
- **CORE-06 `BitmapFont`/`PackedFont`/`FONT_6X10`** — superseded by
  `lv_font_t`; the bring-up font may be re-expressed as a static
  `lv_font_t` if a host test still needs a fixed font.
- **CORE-07n RLE decoder** — reconciles with `lv_image_decoder_t`
  registration (LPAR-09).

## §11 Non-goals

- Font selection model (`WidgetFont`, FONT family); per-widget draw
  (widget phases); asset source resolution (LPAR-09).

## §12 Acceptance checklist

- [ ] `Font` handle over `lv_font_t` with glyph metrics via
      `lv_font_get_glyph_dsc`.
- [ ] Draw-helper wrappers over `lv_draw_rect/label/image/line/arc`
      taking an `lv_layer_t*`.
- [ ] `ImageDescriptor` over `lv_image_dsc_t`; `ColorFormat` ↔
      `lv_color_format_t` mapping.
- [ ] CORE-04/04a/06 supersession recorded as DELTAs.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-08.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md` (v0.2.4 @ `343f596`)
- `lvgl/src/draw/lv_draw.h`, `lvgl/src/font/lv_font.h`, `lvgl/src/draw/lv_image_dsc.h`, `lv_image_decoder.h`
- `core/include/lvglpp/core/{renderer,draw_helpers,font}.hpp` (CORE-04/04a/06, superseded)

## §14 Unblocks

- FONT-00..05, LPAR-11 (Arc/Scale/Spinner custom draw), LPAR-14
  (Chart/Table/Span text), LPAR-15 (Canvas).

## §15 Change log

- **2026-06-15** — LPAR-08 drafted: wrap `lv_draw_*`/`lv_font_*`/
  `lv_image_*`; supersede CORE-04/04a/06; reconcile CORE-07n with
  `lv_image_decoder_t`. **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
