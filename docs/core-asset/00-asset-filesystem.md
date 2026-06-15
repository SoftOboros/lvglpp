# 00 — Asset & filesystem sources

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-09**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. [`../lpar/README.md`](../lpar/README.md)
is informative.

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| Asset source / fs **semantics** | rlvgl `v0.2.4` `docs/concepts/LPAR-09-ASSET-FILESYSTEM.md` (@ `343f596`) |
| The **primitive** | `lvgl/src/misc/lv_fs.h` (`lv_fs_drv_t`, `lv_fs_open`/`read`), `lvgl/src/draw/lv_image_decoder.h` |
| Existing plugin slots / RLE | CORE-07 / CORE-07n |

## §1 Purpose

Wrap LVGL's filesystem-driver + image-decoder registration so widgets can
reference assets by typed source (embedded array, FATFS path, simulator
file, memory blob), and register custom decoders (e.g. the CORE-07n RLE
decoder) through `lv_image_decoder_*`.

## §2 Problem statement

rlvgl re-implements typed asset sources + a cache (`core::asset`,
`core::fs`, `AssetPath`). LVGL ships a drive-letter filesystem
(`lv_fs_drv_t`) and a decoder registry. lvglpp wraps them: an `AssetPath`
maps to an LVGL drive/path or an embedded `lv_image_dsc_t`. The CORE-07
plugin slots become `lv_image_decoder_t` registrations.

## §3 Canonical glossary

- **`AssetPath`** — frozen mirror of rlvgl `AssetPath`
  (`Embedded`/`FATFS`/`Simulator`/`Memory`); maps to an LVGL drive-letter
  path or an embedded descriptor.
- **`FsDriver`** — RAII over a registered `lv_fs_drv_t`
  (`lv_fs_drv_register`); owns its registration for its lifetime.
- **`ImageDecoder`** — RAII over a registered `lv_image_decoder_t`; the
  seam through which CORE-07/07n decoders plug in.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| `AssetPath` variant set | rlvgl `LPAR-09` — **Standards Action** |
| Drive-letter / path convention | `lvgl` `lv_fs` |
| Decoder registration | `lvgl` `lv_image_decoder` + CORE-07 plugin policy |

## §5 Frozen decisions

1. `AssetPath` mirrors rlvgl's variant set (**Standards Action**); it maps
   to LVGL drive paths / embedded descriptors rather than re-implementing
   a VFS.
2. `FsDriver`/`ImageDecoder` are RAII over the LVGL registrations and own
   them for their lifetime (de-register on destruction).
3. CORE-07 plugin slots (PNG/JPEG/…/RLE) become `lv_image_decoder_t`
   registrations behind the same `LVGLPP_CORE_*` CMake options.

## §10 Reconciliation vs. adjacent primitives

- **CORE-07 plugin slots / CORE-07n RLE** — reconciled as
  `lv_image_decoder_t` registrations; the consume-only RLE decoder
  registers through `ImageDecoder`.
- **FATFS plugin (CORE-07 `LVGLPP_CORE_FATFS`)** — backs the
  `AssetPath::FATFS` variant via an `lv_fs_drv_t`.

## §11 Non-goals

- A bespoke asset cache (LVGL's image cache suffices for v1); creator
  asset generation (stays rlvgl-creator, CLAUDE.md).

## §12 Acceptance checklist

- [ ] `AssetPath` mirror enum; `FsDriver`/`ImageDecoder` RAII over the
      LVGL registrations.
- [ ] CORE-07n RLE registers via `ImageDecoder`.
- [ ] Builds + tests under both postures; `core/STATUS.md` records LPAR-09.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-09-ASSET-FILESYSTEM.md` (v0.2.4 @ `343f596`)
- `lvgl/src/misc/lv_fs.h`, `lvgl/src/draw/lv_image_decoder.h`
- `docs/core-plugins/00-plugin-surface.md`, `01-rle.md` (CORE-07/07n)

## §14 Unblocks

- LPAR-08 image draw, WID-06 Image migration, LPAR-12 ImageButton,
  LPAR-15 AnimImage.

## §15 Change log

- **2026-06-15** — LPAR-09 drafted: wrap `lv_fs`/`lv_image_decoder`;
  mirror `AssetPath`; reconcile CORE-07 slots as decoder registrations.
  **Not ratified** — batch pending with Wave 1.
- **2026-06-15** — ratified by owner ("All ratified") with the Wave-1 batch; execution unblocked in dependency order (LPAR-02 first per LPAR-00 §6).
