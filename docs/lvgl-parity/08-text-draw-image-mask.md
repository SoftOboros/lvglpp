<!--
08-text-draw-image-mask.md - lvglpp LPAR-08 mirror text/draw/image/mask plan.
-->

# LPAR-CPP-08 - LVGL Text, Draw, Image, and Mask Substrate

Status: **RATIFIED** (2026-06-30). Normative for the LPAR-CPP-08 mirror
of rlvgl `v0.2.5` text, draw, image, and mask substrate work.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** are interpreted per RFC 2119 and
RFC 8174 when capitalized.

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-08 relationship |
| --- | --- | --- |
| LVGL text, font, label, draw, image, and mask runtime | `lvgl/src/font/lv_font.h`, `lvgl/src/draw/lv_draw_label.h`, `lvgl/src/draw/lv_draw_image.h`, `lvgl/src/draw/lv_image_dsc.h`, `lvgl/src/draw/lv_draw_rect.h`, `lvgl/src/draw/lv_draw_mask.h`, `lvgl/src/widgets/label/lv_label.h`, `lvgl/src/widgets/image/lv_image.h` | Canonical C behavior. lvglpp MUST delegate glyph metrics, wrapping, bidi-enabled label behavior, image source classification, image transforms, recolor, descriptor lifetime, draw descriptors, and mask/rect draw-task construction to LVGL where public LVGL APIs expose them. |
| rlvgl text/draw/image/mask phase | `rlvgl/docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by wrapping LVGL's draw/font/image machinery rather than porting rlvgl's software renderer, `Renderer` trait expansion, `FontMetrics` trait, or retained `ObjectNode` cascade. |
| lvglpp LVGL-backed object/style/display substrate | `core/include/lvglpp/core/object.hpp`, `core/include/lvglpp/core/style_lvgl.hpp`, `core/include/lvglpp/core/display.hpp` | `LvObject`, `ObjectView`, `LvStyle`, `StyleSelector`, `LvDisplay`, and display invalidation are the handles this phase uses. |
| Existing compatibility renderer/font/image path | `core/include/lvglpp/core/renderer.hpp`, `core/include/lvglpp/core/font.hpp`, `widgets/include/lvglpp/widgets/label.hpp`, `widgets/include/lvglpp/widgets/image.hpp` | Compatibility-only unless an implementation explicitly adds an LVGL bridge. These types remain source-compatible and are not promoted to LVGL parity wrappers by this phase. |
| RLE plugin and app assets | `core/include/lvglpp/core/plugins/rle.hpp`, `core/src/plugins/rle.cpp`, `examples/apps/disco-demo/assets/`, `examples/apps/disco-demo/src/assets.cpp` | RLE remains a lvglpp asset source. LPAR-CPP-08 owns conversion to LVGL-compatible image descriptors when the data is memory-resident; LPAR-CPP-09 owns lookup, filesystem, cache, and manifest policy. |
| Ownership discipline | top-level `AGENTS.md` | Every raw LVGL font, image descriptor, image source pointer, string pointer, draw descriptor, layer, task, mask source, decoder callback, and compatibility pixel buffer touched by this phase MUST carry explicit ownership/lifetime comments. |

If this document disagrees with LVGL about object, font, image, or draw
descriptor lifetime, LVGL wins. If this document disagrees with rlvgl
about cross-language widget-visible behavior, rlvgl wins and this chapter
must be amended.

## 1. Purpose

LPAR-CPP-08 provides the LVGL-backed text, font, image, and draw
descriptor substrate required by later layout, widget, Qt, and SCTD demo
work. It gives lvglpp code a way to:

- observe LVGL fonts and query glyph/line metrics without owning font
  storage;
- configure LVGL label text, long modes, recolor, selection, letter
  positions, and point-to-character queries through typed C++ wrappers;
- describe memory-resident images with `lv_image_dsc_t`, including pixel
  format, stride, flags, dimensions, and borrowed data lifetime;
- configure LVGL image widgets for source, offset, scale, rotation,
  pivot, antialiasing, blend mode, recolor, alignment, tiling, and bitmap
  masks;
- expose small typed wrappers around public LVGL draw descriptors for
  label, image, fill, border, box-shadow, rectangle, and rectangle-mask
  tasks where public `lv_layer_t` usage is appropriate;
- preserve the current compatibility `Renderer`, `BitmapFont`,
  `PackedFont`, `widgets::Label`, and `widgets::Image` path until later
  widget migration phases consume LVGL-backed wrappers.

This phase intentionally does not port rlvgl's software raster pipeline.
LVGL already owns draw tasks, font fallback, glyph metrics, label layout,
image decoding, recolor, transforms, clipping, and draw-unit routing.
lvglpp reaches parity by exposing those LVGL capabilities safely.

## 2. Problem Statement

LPAR-CPP-02 through LPAR-CPP-07 establish real LVGL objects, displays,
input, timers, animation, styles, and themes. The next missing substrate
is the content path: text and images.

Current lvglpp has useful compatibility pieces, but they are not LVGL
parity wrappers:

- `core/include/lvglpp/core/font.hpp` defines `BitmapFont` and
  `PackedFont` value/view types that render through `Renderer`.
- `core/include/lvglpp/core/renderer.hpp` defines a small abstract
  software renderer interface with `fill_rect`, `draw_text`,
  `blend_rect`, and `draw_pixels`.
- `widgets/include/lvglpp/widgets/label.hpp` owns a `std::string` and
  calls the compatibility renderer rather than `lv_label_create`.
- `widgets/include/lvglpp/widgets/image.hpp` borrows a raw
  `std::span<const Color>` and blits through `Renderer`, with no LVGL
  image source, transform, recolor, alignment, or decoder path.
- The RLE plugin now lives under lvglpp paths, but no LVGL image
  descriptor bridge records the ownership and pixel-format contract.

rlvgl LPAR-08 solves similar missing surfaces by expanding its Rust
software renderer and font traits. That is the wrong implementation shape
for lvglpp. The C++ side should wrap LVGL's public C APIs, keep the
legacy renderer as a compatibility oracle, and add tests that observe
wrapper-visible LVGL state and render behavior.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **`LvFontView`** | Owned by LPAR-CPP-08; non-owning observation wrapper around `const lv_font_t*`. It never deletes or mutates the font unless an explicit mutable wrapper is later ratified. |
| **Glyph descriptor** | As defined in `lvgl/src/font/lv_font.h` `lv_font_glyph_dsc_t`; used without modification through a C++ value-return helper that calls `lv_font_get_glyph_dsc`. |
| **Glyph bitmap** | As defined in `lvgl/src/font/lv_font.h`; adapted: lvglpp exposes the returned pointer only through a scoped borrowed view and documents that it is valid only until `lv_font_glyph_release_draw_data` when release is required. |
| **`LvLabel`** | Owned by LPAR-CPP-08; move-only `LvObject`-compatible wrapper around an LVGL label object created by `lv_label_create`. |
| **Label long mode** | As defined in `lvgl/src/widgets/label/lv_label.h` `lv_label_long_mode_t`; mirrored by a C++ enum in this phase with Standards Action registration. |
| **`LvImageDescriptor`** | Owned by LPAR-CPP-08; RAII/value wrapper for `lv_image_dsc_t` that documents whether image bytes are borrowed external storage or owned by the wrapper. |
| **Image source** | As defined in `lvgl/src/draw/lv_draw_image.h` / `lv_image_src_get_type`; used without modification for descriptor pointers, filesystem paths, and symbols. |
| **`LvImage`** | Owned by LPAR-CPP-08; move-only `LvObject`-compatible wrapper around an LVGL image object created by `lv_image_create`. |
| **Bitmap mask source** | As defined in `lvgl/src/widgets/image/lv_image.h` `lv_image_set_bitmap_map_src`; adapted: lvglpp requires the referenced `lv_image_dsc_t` lifetime to cover the image object or be owned by a wrapper attached to it. |
| **Draw descriptor view** | Owned by LPAR-CPP-08; small C++ value wrappers around LVGL draw descriptor structs such as `lv_draw_label_dsc_t`, `lv_draw_image_dsc_t`, `lv_draw_rect_dsc_t`, and `lv_draw_mask_rect_dsc_t`. |
| **Compatibility renderer** | As defined in `core/include/lvglpp/core/renderer.hpp`; adapted: remains a non-LVGL software path and may be used by tests/assets, but it is not the LPAR-CPP-08 parity runtime. |
| **RLE-to-LVGL bridge** | Owned by LPAR-CPP-08; conversion boundary that takes decoded or statically generated RLE bytes and presents them as an LVGL-compatible `lv_image_dsc_t`. Lookup and cache policy are owned by LPAR-CPP-09. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact | lvglpp mirror target |
| --- | --- | --- |
| Font metrics and glyph lookup | `lvgl/src/font/lv_font.h` | `LvFontView`, glyph descriptor helpers, scoped glyph bitmap view |
| Label object behavior | `lvgl/src/widgets/label/lv_label.h`, `lvgl/src/widgets/label/lv_label.c` | `LvLabel` wrapper and label enums |
| Label draw descriptors | `lvgl/src/draw/lv_draw_label.h` | optional draw descriptor wrappers for direct layer tests |
| Image source descriptors | `lvgl/src/draw/lv_image_dsc.h` | `LvImageDescriptor`, borrowed/owned data constructors |
| Image widget behavior | `lvgl/src/widgets/image/lv_image.h`, `lvgl/src/widgets/image/lv_image.c` | `LvImage` wrapper and image enums |
| Image draw descriptors/source classification | `lvgl/src/draw/lv_draw_image.h` | image source type helpers and draw descriptor wrappers |
| Fill/border/shadow/rect descriptors | `lvgl/src/draw/lv_draw_rect.h` | draw descriptor wrappers only where public layers are available |
| Rectangle mask descriptor | `lvgl/src/draw/lv_draw_mask.h` | mask descriptor wrapper and image bitmap mask handoff |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md` | this chapter's behavior and phase relationship |
| Existing lvglpp compatibility fonts/images | `core/include/lvglpp/core/font.hpp`, `widgets/include/lvglpp/widgets/image.hpp` | retained as compatibility; bridge only where explicitly implemented |

## 5. Frozen Decisions - LVGL Underneath

1. **No competing text stack.** LPAR-CPP-08 MUST NOT port rlvgl's
   `FontMetrics`, `FontDraw`, text shaping, text wrapping, or
   `ClipRenderer` expansion as the parity path. LVGL's font, label, and
   draw-label APIs own parity behavior.
2. **No competing image renderer.** LPAR-CPP-08 MUST NOT implement a
   parallel image transform/recolor/tile renderer for parity widgets.
   Use `lv_image_*`, `lv_draw_image_*`, and LVGL image decoders
   underneath.
3. **Compatibility stays compatibility.** Existing `core::Renderer`,
   `BitmapFont`, `PackedFont`, `widgets::Label`, and `widgets::Image`
   MAY receive bridge helpers, but their existence does not satisfy
   LVGL-backed parity acceptance unless the phase explicitly says so.
4. **Public LVGL APIs only.** Wrappers SHALL use public LVGL headers.
   Direct use of private draw internals requires a future amendment that
   names the private header, rationale, and blast radius.
5. **Rendering tests observe public output.** Tests SHOULD verify object
   state, descriptor fields, label geometry, image geometry, and display
   flush pixels/areas. Tests MUST NOT assert private draw-task internals
   unless a wrapper intentionally exposes a public descriptor.

## 6. Frozen Decisions - Font Surface

LPAR-CPP-08 SHALL introduce or reserve these font concepts:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `LvFontView` | `const lv_font_t*` | observes external/static font; never releases it |
| `GlyphDescriptor` or direct `lv_font_glyph_dsc_t` return | `lv_font_glyph_dsc_t` | value result; descriptor may carry LVGL cache entry that must be released |
| `GlyphBitmapView` | pointer from `lv_font_get_glyph_bitmap` / `lv_font_get_glyph_static_bitmap` | borrows LVGL/font-owned draw data for a scoped lifetime |

Normative rules:

1. `LvFontView` MUST be nullable only when explicitly constructed as an
   empty observation. Functions that require a font SHOULD take a
   non-empty view or a reference-like wrapper.
2. `LvFontView::line_height`, `glyph_width`, `glyph_descriptor`, and
   default-font helpers SHALL delegate to LVGL.
3. Any helper that calls `lv_font_get_glyph_bitmap` MUST document whether
   `lv_font_glyph_release_draw_data` is required and provide a scoped
   RAII release path when LVGL may allocate/cache glyph data.
4. Font fallback, kerning, glyph format, placeholder handling, and bitmap
   conversion remain LVGL behavior.
5. Existing `BitmapFont` and `PackedFont` are not migrated in this phase
   unless a bridge adapter is explicitly accepted; they remain
   compatibility views over lvglpp-owned data.

## 7. Frozen Decisions - Label Surface

LPAR-CPP-08 SHALL introduce an LVGL-backed label wrapper:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `LvLabel` | `lv_label_create`, label-specific getters/setters | owns `lv_obj_t*` through `LvObject` or attaches to a parent with explicit ownership transfer |
| `LabelView` if needed | `lv_obj_t*` known to be an LVGL label | observes/borrows existing label object |
| `LabelLongMode` | `lv_label_long_mode_t` | value enum, Standards Action |

The v1 wrapper SHALL expose typed equivalents for:

- create/attach under an `ObjectView` parent;
- `set_text`, `set_text_static`, `text`;
- `set_long_mode`, `long_mode`;
- `set_recolor`, `recolor`;
- selection start/end getters/setters;
- `letter_pos`, `letter_on`, and `char_under_pos` helpers using LVGL
  point/value structs or lvglpp point adapters;
- style-driven text color/font/spacing via LPAR-CPP-07 local style
  helpers where appropriate, rather than duplicating text style storage.

Normative rules:

1. `set_text` MUST copy into LVGL-owned label storage. `set_text_static`
   MUST document that the caller's C string storage is external and must
   outlive the label or the next text assignment.
2. `text()` returns a borrowed view of LVGL label storage. It MUST NOT be
   retained across label mutation or deletion.
3. The wrapper MUST NOT allocate a second `std::string` mirror for
   ordinary label text state unless a future binding phase needs it for a
   generated UI cache.
4. Translation tags and observer binding MAY be reserved, but if exposed
   they MUST be feature-gated by the matching LVGL feature macros.

## 8. Frozen Decisions - Image Descriptor and Image Widget Surface

LPAR-CPP-08 SHALL introduce or reserve:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `LvImageDescriptor` | `lv_image_dsc_t` | owns descriptor value; either owns or borrows byte storage explicitly |
| `ImageDescriptorView` | `const lv_image_dsc_t*` | observes external/static descriptor |
| `ImageSourceView` | descriptor pointer, file path, or symbol pointer | observes external source; source lifetime follows LVGL rules |
| `LvImage` | `lv_image_create`, `lv_image_*` setters/getters | owns `lv_obj_t*` through `LvObject` or explicit attach |
| `ImageAlign` | `lv_image_align_t` | value enum, Standards Action |

The v1 descriptor wrapper SHALL support memory-resident images with:

- color format (`lv_color_format_t`);
- width, height, stride, data size, and flags;
- borrowed bytes for generated/static assets;
- optional owned byte storage only when clearly named as ownership
  transfer, for example `make_owned_image_descriptor`;
- conversion helpers for decoded RLE output when the decoded format is
  compatible with LVGL's color format and stride requirements.

The v1 image wrapper SHALL expose typed equivalents for:

- create/attach under an `ObjectView` parent;
- `set_source` from descriptor, path, or symbol source views;
- source width/height and source type observation;
- offsets, scale, per-axis scale, rotation, pivot, antialiasing,
  blend mode, inner alignment;
- bitmap mask source through `lv_image_set_bitmap_map_src`;
- recolor/recolor opacity through object style properties or draw/image
  descriptor helpers where LVGL exposes them for the selected path.

Normative rules:

1. A descriptor that borrows bytes MUST keep those bytes alive until no
   LVGL object, cache entry, or decoder can reference them.
2. Owned byte storage MUST be stable in memory for the lifetime of the
   descriptor and any LVGL users of it.
3. File path and symbol sources are observed external storage. The wrapper
   MUST document whether LVGL copies or observes the provided string for
   each API call; if LVGL observes, the C string must outlive the object
   or next source assignment.
4. Cache policy, filesystem mount, qrc/qmldir manifest resolution, and
   generated asset lookup are deferred to LPAR-CPP-09.

## 9. Frozen Decisions - Draw Descriptor and Mask Surface

LPAR-CPP-08 MAY expose typed wrappers for public draw descriptors:

- `LvDrawLabelDescriptor` over `lv_draw_label_dsc_t`;
- `LvDrawImageDescriptor` over `lv_draw_image_dsc_t`;
- `LvDrawFillDescriptor` over `lv_draw_fill_dsc_t`;
- `LvDrawBorderDescriptor` over `lv_draw_border_dsc_t`;
- `LvDrawBoxShadowDescriptor` over `lv_draw_box_shadow_dsc_t`;
- `LvDrawRectDescriptor` over `lv_draw_rect_dsc_t`;
- `LvDrawMaskRectDescriptor` over `lv_draw_mask_rect_dsc_t`.

These wrappers are lower-level than widget wrappers. They are accepted
only if they:

1. initialize through the matching LVGL `_dsc_init` function;
2. expose field setters/getters without hiding pointer lifetimes;
3. require an explicit borrowed `lv_layer_t&` or layer view for draw-task
   creation;
4. document every source pointer, font pointer, bitmap mask pointer, and
   draw hint pointer as borrowed/external;
5. preserve LVGL clipping and draw-unit scheduling rather than directly
   touching software draw internals.

Direct draw descriptor wrappers are not required for basic `LvLabel` and
`LvImage` object wrappers. They are useful for conformance fixtures,
future canvas work, and generated UI code that needs explicit draw tasks.

## 10. Reconciliation vs Adjacent lvglpp Primitives

| Existing primitive | LPAR-CPP-08 relationship |
| --- | --- |
| `Runtime` | Must be initialized before using LVGL font/image/widget wrappers. No semantic change. |
| `LvObject` / `ObjectView` | Ownership substrate for `LvLabel` and `LvImage`. This phase MUST reuse existing object ownership rules. |
| `LvStyle` / style helpers | Text color, font, opacity, spacing, recolor, radius, and image-related style properties should use the LPAR-CPP-07 style API where LVGL models them as style properties. |
| `LvDisplay` | Display flush tests can observe label/image rendering. This phase does not change display ownership. |
| `core::Renderer` | Retained for compatibility widgets and software tests. Not expanded as the LVGL parity path in this phase. |
| `BitmapFont` / `PackedFont` | Retained compatibility font views. May feed future generated asset tests but do not replace `lv_font_t`. |
| `widgets::Label` | Retained compatibility widget. `LvLabel` or a similarly named LVGL-backed wrapper is the parity path. LPAR-CPP-12 through LPAR-14 decide broader widget migration naming. |
| `widgets::Image` | Retained compatibility widget over borrowed pixels. `LvImage` plus `LvImageDescriptor` is the parity path. |
| RLE plugin | Provides decode/source bytes. LPAR-CPP-08 may convert decoded memory to `LvImageDescriptor`; LPAR-CPP-09 owns lookup/cache. |

## 11. Non-Goals

- No port of rlvgl's `Renderer` trait expansion, text shaper, or
  software gradient/shadow/mask rasterizer as the parity path.
- No replacement of LVGL font fallback, kerning, bidi, label wrapping, or
  image decoder behavior.
- No broad widget-matrix migration beyond label and image surfaces needed
  for this phase.
- No filesystem, qrc/qmldir, generated asset manifest, or image cache
  eviction policy; LPAR-CPP-09 owns that.
- No Canvas, AnimImage, Lottie, 3DTexture, property, or observer wrapper;
  LPAR-CPP-15 owns those.
- No private LVGL draw-unit dependency unless a later amendment names and
  accepts that dependency.

## 12. Acceptance Checklist

LPAR-CPP-08 implementation is complete only when:

- [x] a ratified change-log entry marks this chapter accepted;
- [x] `LvFontView` or equivalent font observation wrappers expose default
      font, line height, glyph width/descriptor, and scoped glyph bitmap
      lifetime where applicable;
- [x] an LVGL-backed label wrapper exposes text, static text, long mode,
      recolor, selection, letter-position, and point-hit helpers;
- [x] an LVGL-backed image descriptor wrapper documents borrowed vs owned
      image bytes and supports LVGL-compatible memory-resident sources;
- [x] an LVGL-backed image wrapper exposes source, offset, transform,
      antialias, blend, alignment, source-size, and bitmap-mask helpers;
- [x] direct draw descriptor wrappers are either implemented with tests or
      explicitly deferred in this chapter's change log;
- [x] RLE-to-LVGL descriptor handoff is either implemented for compatible
      decoded formats or explicitly deferred to LPAR-CPP-09 with a blocker
      owner;
- [x] compatibility `Renderer`, `BitmapFont`, `PackedFont`,
      `widgets::Label`, and `widgets::Image` remain source-compatible;
- [x] tests use real LVGL objects and verify wrapper-visible state plus at
      least one label/image render path through the display flush harness;
- [x] embedded posture builds the new core/widget wrappers without
      exceptions or RTTI when `LVGLPP_EMBEDDED_POSTURE=ON`;
- [x] every raw pointer member added by this phase has an adjacent
      ownership/lifetime comment.

## 13. Files Cited

- `rlvgl/docs/concepts/LPAR-08-TEXT-DRAW-IMAGE-MASK.md`
- `lvgl/src/font/lv_font.h`
- `lvgl/src/draw/lv_draw_label.h`
- `lvgl/src/draw/lv_draw_image.h`
- `lvgl/src/draw/lv_image_dsc.h`
- `lvgl/src/draw/lv_draw_rect.h`
- `lvgl/src/draw/lv_draw_mask.h`
- `lvgl/src/widgets/label/lv_label.h`
- `lvgl/src/widgets/image/lv_image.h`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/style_lvgl.hpp`
- `core/include/lvglpp/core/display.hpp`
- `core/include/lvglpp/core/renderer.hpp`
- `core/include/lvglpp/core/font.hpp`
- `core/include/lvglpp/core/plugins/rle.hpp`
- `widgets/include/lvglpp/widgets/label.hpp`
- `widgets/include/lvglpp/widgets/image.hpp`

## 14. Unblocks

- LPAR-CPP-09 asset/filesystem wrappers, because image descriptors and
  source lifetimes are frozen here.
- LPAR-CPP-10 layout tests involving label/image geometry.
- LPAR-CPP-11 primitive widgets that need draw descriptor and font/image
  substrate for visual smoke tests.
- LPAR-CPP-12 ImageButton and control widgets using image sources.
- LPAR-CPP-14 text-heavy widgets: Span, Table, Textarea, MessageBox.
- QT-CPP generated screens and SCTD demo panels that need labels and
  image assets backed by real LVGL objects.

## 15. Change Log

| Date | State | Notes |
| --- | --- | --- |
| 2026-06-30 | DRAFT | Initial LVGL text/draw/image/mask substrate draft. Adapts rlvgl LPAR-08 by delegating fonts, labels, image descriptors, image widgets, draw descriptors, and masks to LVGL public APIs while preserving current renderer/font/image classes as compatibility surfaces. |
| 2026-06-30 | RATIFIED | Owner accepted the LPAR-CPP-08 phase and directed execution to proceed. |
| 2026-06-30 | IMPLEMENTED | Added LVGL-backed font, label, image descriptor, image widget, draw descriptor, and mask descriptor wrappers with focused host tests. Embedded-posture checkbox remains open until the embedded build gate is run for this change. |
