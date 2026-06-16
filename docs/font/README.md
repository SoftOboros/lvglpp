<!--
README.md — Initiative README for the FONT (font-selection + anti-aliased
text) family in lvglpp.
-->

# font — initiative README

FONT is the lvglpp **Wave 2** initiative (LPAR-00 §6/§7): it brings the C++
wrapper up to the **rlvgl v0.2.4** font-selection surface. It mirrors the
rlvgl `FONT-00` and `FONT-05` concepts families
(`rlvgl/docs/concepts/FONT-*.md` @ `343f596`) onto upstream LVGL via **RAII /
handle wrappers that call `lv_*` directly**.

This README is **informative**. The normative artifacts are the per-chapter
concepts docs in this directory (`NN-*.md`).

## The reframe (why lvglpp's FONT is much smaller than rlvgl's)

rlvgl's FONT-00 closes **five** gaps because rlvgl **re-implements** glyph
rasterization: (1) per-widget font selection, (2) anti-aliased coverage
(`PackedFont` 8-bit), (3) `ArcLabel` glyph-path migration, (4) rotated
hardware-renderer glyph throughput, and (5) an AA conformance fixture.

LVGL ships all of the rasterization machinery natively:

| rlvgl FONT gap | LVGL already provides | lvglpp does |
| --- | --- | --- |
| Font selection (FONT-00 §5) | `lv_obj_set_style_text_font` + the style cascade | wraps it: `Object::set_local_text_font`, `Style::set_text_font`, `Object::text_font()` (cascade resolution) |
| Anti-aliased text (FONT-00 §6) | built-in 4-bit AA `lv_font_montserrat_*`, A1/A2/A4/A8 glyph formats | exposes the AA built-ins + a glyph-format / AA query on the `Font` handle |
| `FontId` registry + cascade→widget bridge (FONT-05) | the cascade **is** the bridge — a style stores the `lv_font_t*` and LVGL resolves it per object | provides a thin `FontRegistry` (`FontId → const lv_font_t*`) for the creator-asset path; the bridge itself is native |
| ArcLabel migration (FONT-00 §7) | — (LVGL has no `ArcLabel`) | out of scope: folds into the widget waves / `LVGLPP-WRAP` |
| Rotated-renderer throughput (FONT-00 §8) | DMA2D/LTDC composite path | out of scope: platform, verification-blocked (disco) |
| AA conformance fixture (FONT-00 §9) | — | owned by LPAR-16 conformance, not here |

So the lvglpp FONT wave is two thin chapters: **font selection over the
LVGL cascade + AA built-ins** (FONT-00) and **a `FontId` registry** (FONT-05).

## Chapters

| Chapter | Phase | Mirrors | Status |
| --- | --- | --- | --- |
| [00-concepts.md](./00-concepts.md) | FONT-00 | `rlvgl/docs/concepts/FONT-00-CONCEPTS.md` | ✅ **Landed 2026-06-15** |
| [05-font-registry.md](./05-font-registry.md) | FONT-05 | `rlvgl/docs/concepts/FONT-05-FONT-REGISTRY.md` | ✅ **Landed 2026-06-15** |

FONT-01..04 (ArcLabel migration, rotated-renderer throughput, the AA
fixture) have **no lvglpp execution phase** — see the reframe table; they are
either native, owned by another wave, or platform/verification-blocked. The
lvglpp FONT wave is FONT-00 (selection) + FONT-05 (registry).

## Dependency

Gated on **LPAR-08** (the `Font` handle over `lv_font_t`, landed
`docs/core-draw/00-text-draw-image-mask.md`) and **LPAR-07** (the style
cascade `Style`/`Object::set_local_*`, landed
`docs/core-style/01-style-cascade-theme.md`). Both are complete (Wave 1).

## Cross-language pair

Per CLAUDE.md § "Cross-language change ordering", `FontId` mirrors rlvgl's
`FontId(u16)` / `FontId::DEFAULT` (`rlvgl/core/src/font.rs:15`) and is a
**Standards Action** enum: any divergence amends the rlvgl concept first.
No rlvgl change is required to land these lvglpp mirrors.
