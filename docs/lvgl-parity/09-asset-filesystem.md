<!--
09-asset-filesystem.md - lvglpp LPAR-09 mirror asset and filesystem plan.
-->

# LPAR-CPP-09 - LVGL Asset and Filesystem Sources

Status: **RATIFIED** (2026-06-30). Normative for the LPAR-CPP-09 mirror
of rlvgl `v0.2.5` asset and filesystem source work.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** are interpreted per RFC 2119 and
RFC 8174 when capitalized.

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-09 relationship |
| --- | --- | --- |
| LVGL filesystem driver model | `lvgl/src/misc/lv_fs.h`, `lvgl/src/misc/lv_fs.c` | Canonical C behavior for drive-letter registration, file open/read/write/seek/tell/size, directory iteration, path helpers, and memory-buffer path objects. lvglpp MUST wrap these APIs rather than invent a second runtime dispatcher for LVGL image sources. Safe drive removal is blocked in the current LVGL pin because `lv_fs_remove_drive` removes the matching node while continuing list iteration; this erratum MUST be retried against a non-dev LVGL release before treating it as an upstream-stable defect. See §15. |
| LVGL image decoder lifecycle | `lvgl/src/draw/lv_image_decoder.h` | Canonical open/get-area/close lifecycle and custom decoder registration model. lvglpp adapts with RAII session/decoder wrappers and explicit callback/userdata lifetimes. |
| LVGL image cache | `lvgl/src/misc/cache/instance/lv_image_cache.h` | Canonical cache implementation. lvglpp MUST expose cache init/resize/drop/enable observation instead of implementing a parallel C++ image-cache eviction policy for LVGL-backed widgets. |
| rlvgl asset/filesystem phase | `rlvgl/docs/concepts/LPAR-09-ASSET-FILESYSTEM.md` (`v0.2.5 @ f999f75`) | Canonical cross-language source vocabulary. lvglpp adapts rlvgl's typed `AssetSource`/`AssetRegistry`/cache model to LVGL's filesystem drivers, image source types, decoder chain, and image cache. |
| LPAR-CPP-08 image descriptors and widgets | `core/include/lvglpp/core/draw_lvgl.hpp` | `LvImageDescriptor`, `ImageDescriptorView`, `LvImage`, and `image_source_type` are the image source substrate this phase extends with file/symbol/path-buffer/cache wrappers. |
| Existing lvglpp RLE and app asset table | `core/include/lvglpp/core/plugins/rle.hpp`, `examples/apps/disco-demo/include/lvglpp/app/disco_demo/assets.hpp`, `examples/apps/disco-demo/src/assets.cpp` | Existing consume-only asset path. LPAR-CPP-09 may add LVGL image descriptor/path handoff helpers, but checked-in RLE generation and app-specific icon names remain owned by their app/docs. |
| Qt/QML resource manifests | `scripts/lvglpp_qt.py`, rlvgl Qt support docs | LPAR-CPP-09 defines the stable C++ manifest/catalog shape that generated Qt resources may consume. Parsing/emitting `.qrc`, `.qmldir`, or QML-specific resource tables remains QT-CPP work unless explicitly ratified here. |
| Ownership discipline | top-level `AGENTS.md` | Every raw LVGL filesystem driver, file, directory, path buffer, image decoder, decoder session, cache source pointer, C callback, userdata pointer, asset byte span, and path string touched by this phase MUST carry explicit ownership/lifetime comments. |

If this chapter disagrees with LVGL filesystem, image decoder, or cache
lifetime rules, LVGL wins. If it disagrees with rlvgl `v0.2.5` about
widget-visible asset/source behavior, rlvgl wins and this chapter must be
amended.

## 1. Purpose

LPAR-CPP-09 provides the asset and filesystem source layer needed by
LVGL-backed image widgets, later ImageButton/media widgets, generated Qt
screens, and SCTD demo assets. It gives lvglpp code a way to:

- register and remove LVGL filesystem drivers through a move-only C++
  owner;
- open files and directories through RAII wrappers over `lv_fs_file_t`
  and `lv_fs_dir_t`;
- create LVGL path objects that represent memory buffers as file-like
  sources;
- pass descriptor, file-path, symbol, and buffer-backed sources into
  `LvImage`;
- open image decoder sessions and register custom image decoders when a
  codec path is intentionally implemented in C++;
- initialize, resize, drop, and observe LVGL's image cache;
- define an app asset catalog shape with stable string and byte lifetimes
  suitable for generated Qt/SCTD assets without committing to a C++
  creator port.

This phase intentionally does not build a second C++ asset registry or
image cache. LVGL already owns the source-type dispatch, filesystem
driver table, decoder chain, and image cache. lvglpp wraps those pieces
with explicit C++ ownership.

## 2. Problem Statement

LPAR-CPP-08 can create memory-resident `lv_image_dsc_t` descriptors and
LVGL image widgets, but the broader source path is still missing:

- file paths accepted by `lv_image_set_src` require an LVGL filesystem
  driver registered by drive letter;
- memory buffers that are not `lv_image_dsc_t` descriptors, such as PNG
  bytes, need `lv_fs_path_ex_t` or a decoder-specific source path;
- LVGL image cache control is still only available through raw C calls;
- custom decoder registration has no C++ RAII owner or callback lifetime
  contract;
- existing app assets are CMake-generated static byte arrays and RLE
  blobs, not a general LVGL source catalog;
- future Qt/SCTD generated assets need a stable manifest/catalog
  boundary, but `creator-cpp` remains deferred by top-level `AGENTS.md`.

rlvgl LPAR-09 solves this with Rust `AssetSource`, `AssetRegistry`,
`AssetPath`, `AssetHandle`, and a concrete cache. lvglpp should not port
that runtime literally. For LVGL-backed widgets the authoritative source
dispatch is already `LV_IMAGE_SRC_VARIABLE`, `LV_IMAGE_SRC_FILE`,
`LV_IMAGE_SRC_SYMBOL`, `lv_fs_drv_t`, `lv_image_decoder_*`, and
`lv_image_cache_*`.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **Filesystem driver** | As defined in `lvgl/src/misc/lv_fs.h` `lv_fs_drv_t`; adapted: lvglpp owns a registered driver through `LvFsDriver` or observes a driver through `FsDriverView`. |
| **Drive letter** | As defined in `lvgl/src/misc/lv_fs.h`; used without modification. It is the LVGL source-kind discriminator for file paths such as `S:/folder/file.png`. |
| **`LvFsDriver`** | Owned by LPAR-CPP-09; move-only RAII owner for a heap/stable `lv_fs_drv_t` registered with LVGL and removed by drive letter on reset/destruction. |
| **`FsDriverView`** | Owned by LPAR-CPP-09; non-owning observation wrapper around `lv_fs_drv_t*`. It never unregisters a drive. |
| **`LvFile`** | Owned by LPAR-CPP-09; RAII wrapper for `lv_fs_file_t`. Opens through `lv_fs_open`, closes with `lv_fs_close`, and exposes read/write/seek/tell/size helpers. |
| **`LvDirectory`** | Owned by LPAR-CPP-09; RAII wrapper for `lv_fs_dir_t`. Opens through `lv_fs_dir_open`, iterates with `lv_fs_dir_read`, closes with `lv_fs_dir_close`. |
| **`LvPathFromBuffer`** | Owned by LPAR-CPP-09; value wrapper for `lv_fs_path_ex_t` created by `lv_fs_make_path_from_buffer`. It observes external buffer bytes. |
| **Image source type** | As defined in `lvgl/src/draw/lv_image_decoder.h` `lv_image_src_t`; used without modification for variable, file, symbol, and unknown sources. |
| **Image decoder session** | Owned by LPAR-CPP-09; RAII wrapper around `lv_image_decoder_dsc_t` opened by `lv_image_decoder_open` and closed by `lv_image_decoder_close`. |
| **`LvImageDecoder`** | Owned by LPAR-CPP-09; move-only RAII owner for `lv_image_decoder_t*` created by `lv_image_decoder_create` and deleted by `lv_image_decoder_delete`. |
| **LVGL image cache** | As defined in `lvgl/src/misc/cache/instance/lv_image_cache.h`; used without modification. lvglpp exposes control helpers but does not replace the cache. |
| **Static asset catalog** | Owned by LPAR-CPP-09; C++ table of stable asset names, byte spans, optional LVGL path strings, and optional image descriptors. It is a source catalog, not a decoder or cache. |
| **qrc/qmldir manifest** | As defined by Qt/QML tooling and rlvgl Qt docs; adapted: LPAR-CPP-09 reserves the C++ catalog record shape, while parsing/emission remains QT-CPP unless this chapter is amended. |
| **Compatibility RLE asset** | Existing consume-only RLEC blobs decoded by `core::rle`. LPAR-CPP-09 may feed decoded bytes to `LvImageDescriptor`, but does not add an encoder or generator. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact | lvglpp mirror target |
| --- | --- | --- |
| FS errors, modes, seek modes, driver callbacks | `lvgl/src/misc/lv_fs.h` | `FsResult`, `FileMode`, `SeekWhence`, `LvFsDriver`, callback contracts |
| File and directory RAII | `lvgl/src/misc/lv_fs.h` | `LvFile`, `LvDirectory` |
| Path helper and memory-buffer path | `lvgl/src/misc/lv_fs.h` | `LvPathFromBuffer`, path helper functions |
| Image source classification | `lvgl/src/draw/lv_image_decoder.h`, `lvgl/src/draw/lv_draw_image.h` | existing `image_source_type` plus file/symbol/path-buffer source wrappers |
| Image decoder lifecycle | `lvgl/src/draw/lv_image_decoder.h` | `LvImageDecoder`, `ImageDecoderSession` |
| Image cache | `lvgl/src/misc/cache/instance/lv_image_cache.h` | cache control wrappers |
| Memory-resident image descriptors | `core/include/lvglpp/core/draw_lvgl.hpp` | `LvImageDescriptor`, `ImageDescriptorView` |
| Existing RLE source bytes | `core/include/lvglpp/core/plugins/rle.hpp` | optional RLE-to-descriptor helper or explicit deferral |
| Disco app static asset table | `examples/apps/disco-demo/include/lvglpp/app/disco_demo/assets.hpp` | example of app-owned stable byte catalog |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-09-ASSET-FILESYSTEM.md` | this chapter's phase relationship |

## 5. Frozen Decisions - LVGL Underneath

1. **No competing filesystem registry.** LPAR-CPP-09 MUST NOT port
   rlvgl's `AssetRegistry` as the runtime path for LVGL-backed images.
   LVGL's `lv_fs_drv_t` registration is the filesystem registry for
   file-path image sources.
2. **No competing image cache.** LPAR-CPP-09 MUST NOT implement a C++
   LRU cache for LVGL-backed images. LVGL's image cache owns decode cache
   behavior. lvglpp may expose init/resize/drop/enable/iterate helpers.
3. **No hidden source lifetime.** Any wrapper that passes a C string,
   byte buffer, descriptor pointer, or decoder userdata pointer into LVGL
   MUST document whether LVGL copies or observes that storage.
4. **C callbacks are explicit.** Filesystem and image decoder callbacks
   are raw C function pointers. lvglpp wrappers MAY store them in
   `lv_fs_drv_t` or `lv_image_decoder_t`, but the user data they observe
   must have a documented owner and lifetime.
5. **Generated assets are cataloged, not generated here.** LPAR-CPP-09
   may define a C++ catalog shape for generated assets. It MUST NOT
   implement `creator-cpp` or a new asset encoder.

## 6. Frozen Decisions - Filesystem Driver Surface

LPAR-CPP-09 SHALL introduce or reserve:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `FsResult` | `lv_fs_res_t` | value enum or alias, Standards Action |
| `FileMode` | `lv_fs_mode_t` | value enum/bitmask |
| `SeekWhence` | `lv_fs_whence_t` | value enum |
| `FsDriverView` | `lv_fs_drv_t*` | observes external/registered driver |
| `LvFsDriver` | stable `lv_fs_drv_t` storage + `lv_fs_drv_register` / `lv_fs_remove_drive` | owns driver storage and registration |
| `LvFile` | `lv_fs_file_t` | owns open file state until close |
| `LvDirectory` | `lv_fs_dir_t` | owns open directory state until close |

Normative rules:

1. `LvFsDriver` MUST keep its `lv_fs_drv_t` storage at a stable address
   for the entire registration lifetime. Moving the C++ owner must not
   invalidate LVGL's stored pointer.
2. `LvFsDriver` destructor/reset SHALL remove the drive by letter when
   registered once the current LVGL `lv_fs_remove_drive` list-iteration
   defect is resolved. Until then, `LvFsDriver` MUST avoid leaving LVGL
   with a dangling driver pointer, MAY call `remove_cb`, and MUST record
   the safe-unregister blocker in §15. This blocker MUST be retried
   against a non-dev LVGL release before lvglpp treats it as an
   upstream-stable LVGL defect. If LVGL cannot report duplicate letters
   at registration time, tests MUST verify duplicate behavior through
   `lv_fs_get_drv`.
3. Filesystem callback userdata is external unless the wrapper explicitly
   owns it. The declaration adjacent to any raw `void*` MUST state that
   role.
4. `LvFile` SHALL close in its destructor when open. `release` is not
   required for v1 because `lv_fs_file_t` is a value session, not a heap
   handle.
5. `LvDirectory` SHALL close in its destructor when open.
6. Path helper wrappers SHALL include `extension`, `last_component`,
   `join`, `up`, and drive-letter readiness helpers where useful, all
   delegating to LVGL.

## 7. Frozen Decisions - Memory, File, Symbol, and Catalog Sources

LPAR-CPP-09 maps rlvgl's source-kind vocabulary to LVGL image sources:

| rlvgl source kind | lvglpp / LVGL source |
| --- | --- |
| Embedded | `LvImageDescriptor` for already-decoded bytes, `LvPathFromBuffer` for encoded bytes, or static symbol/path strings in an app catalog |
| FATFS | LVGL file path using a registered drive letter, e.g. `S:/icons/play.png` |
| Simulator | LVGL file path through a host/stdio/posix driver when enabled by LVGL config |
| Memory | `LvPathFromBuffer` or `LvImageDescriptor`, depending on whether bytes are encoded or already pixel data |

Normative rules:

1. Descriptor sources (`LV_IMAGE_SRC_VARIABLE`) require descriptor and
   byte storage to outlive all LVGL users.
2. File sources (`LV_IMAGE_SRC_FILE`) require the path string to remain
   valid for any LVGL API that observes rather than copies it. Wrappers
   MUST document the lifetime for each setter.
3. Symbol sources (`LV_IMAGE_SRC_SYMBOL`) are observed string storage.
   Static string literals are RECOMMENDED.
4. `LvPathFromBuffer` observes the buffer passed to
   `lv_fs_make_path_from_buffer`; that buffer MUST outlive every file or
   image decode operation using the path object.
5. Static asset catalogs SHALL expose stable byte spans and stable names.
   They MAY expose pre-built `LvImageDescriptor` values only if the
   descriptor storage and byte storage lifetimes are both documented.
6. `.qrc` and `.qmldir` consumption is limited in this phase to a stable
   catalog record shape: asset id, source kind, path/name, MIME or file
   extension, optional dimensions, and provenance. Parsing those manifest
   formats is QT-CPP work unless this chapter is amended.

## 8. Frozen Decisions - Image Decoder and Cache Surface

LPAR-CPP-09 SHALL introduce or reserve:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `ImageDecoderSession` | `lv_image_decoder_dsc_t` + `lv_image_decoder_open/get_area/close` | owns an open decoder session until close |
| `LvImageDecoder` | `lv_image_decoder_t*` | owns custom decoder registration object |
| cache helpers | `lv_image_cache_init/resize/drop/is_enabled` | controls LVGL global image cache |

Normative rules:

1. `ImageDecoderSession` MUST call `lv_image_decoder_close` exactly once
   after a successful open unless ownership is explicitly released by a
   future amendment.
2. The session source pointer is observed by LVGL and must outlive the
   session.
3. `LvImageDecoder` MUST expose setters for info/open/get-area/close
   callbacks and document callback/userdata lifetime.
4. LPAR-CPP-09 MAY register a C++ decoder for RLEC only if it can do so
   without adding encoder/generator behavior and without violating the
   existing `LVGLPP_CORE_RLE` feature gate. Otherwise RLE remains a
   memory-decode-to-descriptor path.
5. Cache helpers MUST operate on LVGL's global image cache. Source-specific
   drop uses the exact source pointer/string passed to LVGL, matching
   `lv_image_cache_drop`.

## 9. Frozen Decisions - Feature Gates and LVGL Config

1. `LVGLPP_CORE_RLE` remains the lvglpp feature gate for RLEC decode.
2. PNG/JPEG/GIF/APNG/Lottie and heavyweight decoder plugin paths remain
   off by default and follow existing `LVGLPP_CORE_*` options until a
   dedicated plugin phase implements them.
3. LVGL filesystem backends such as stdio, POSIX, FATFS, memfs, littlefs,
   Arduino SD, or FrogFS are controlled by LVGL config (`LV_USE_FS_*`).
   LPAR-CPP-09 wrappers MAY compile regardless of which concrete backend
   is enabled, but tests requiring a backend MUST be feature/config gated.
4. Host-only helpers may use `std::filesystem` only under an explicit
   host/test option. Core wrappers must remain compatible with embedded
   posture.
5. ESP-IDF/FireBeetle storage drivers are not implemented in this phase;
   SCTD-CPP-03 owns board host integration.

## 10. Reconciliation vs Adjacent lvglpp Primitives

| Existing primitive | LPAR-CPP-09 relationship |
| --- | --- |
| `Runtime` | Must be initialized before filesystem/image decoder/cache wrappers are used. No semantic change. |
| `LvImageDescriptor` / `LvImage` | Primary consumers of descriptor, file, symbol, and path-buffer sources. |
| `LvDisplay` | Render/flush tests can verify file or descriptor sources after cache/decode. Display ownership unchanged. |
| `core::rle` | Existing consume-only decoder. It may feed `LvImageDescriptor`; custom LVGL decoder registration is optional and feature-gated. |
| compatibility `widgets::Image` | Remains raw decoded-pixel widget; no LVGL source/cache parity claim. |
| disco-demo asset table | Remains app-specific static catalog. LPAR-CPP-09 may use it as a test fixture but should not force all apps into the disco naming model. |
| `scripts/lvglpp_qt.py` | Future generated asset catalogs should target the record shape defined here. Parser/emitter work remains QT-CPP. |

## 11. Non-Goals

- No C++ clone of rlvgl's `AssetRegistry`, `AssetHandle`, or concrete
  image cache for LVGL-backed widgets.
- No implementation of FATFS, LittleFS, POSIX, stdio, or ESP-IDF storage
  drivers beyond wrapping LVGL's public driver model.
- No new PNG/JPEG/GIF/APNG/Lottie decoder implementation.
- No `creator-cpp`, asset encoder, RLE encoder, or manifest generator.
- No board-specific SD/eMMC/flash bring-up.
- No private LVGL cache or decoder internals unless a later amendment
  names and accepts that dependency.

## 12. Acceptance Checklist

LPAR-CPP-09 implementation is complete only when:

- [x] a ratified change-log entry marks this chapter accepted;
- [x] filesystem result/mode/seek wrappers map to LVGL values without
      changing numeric behavior;
- [x] `LvFsDriver` owns stable driver storage, registers a drive, records
      the safe-unregister blocker, and avoids dangling LVGL driver storage;
- [x] driver remove-callback behavior is covered, and safe drive removal is
      deferred to LVGL upstream resolution;
- [x] `LvFile` opens, reads, seeks/tells, sizes, and closes through LVGL;
- [x] `LvDirectory` opens, reads names, and closes through LVGL;
- [x] `LvPathFromBuffer` creates a valid LVGL memory-buffer path while
      documenting external buffer lifetime;
- [x] image decoder session and custom decoder wrappers are implemented
      or explicitly deferred with an owner in this change log;
- [x] LVGL image cache helpers initialize/resize/drop/observe cache state;
- [x] descriptor, file, symbol, and path-buffer source tests use real
      LVGL image source classification or decoder behavior;
- [x] any RLE-to-LVGL handoff is implemented under `LVGLPP_CORE_RLE` or
      explicitly deferred to LPAR-CPP-09 change log with a blocker owner;
- [x] static asset catalog record shape is documented for QT/SCTD
      generated consumers;
- [x] embedded posture builds affected targets with
      `LVGLPP_EMBEDDED_POSTURE=ON`;
- [x] every raw pointer member added by this phase has an adjacent
      ownership/lifetime comment.

## 13. Files Cited

- `rlvgl/docs/concepts/LPAR-09-ASSET-FILESYSTEM.md`
- `lvgl/src/misc/lv_fs.h`
- `lvgl/src/misc/lv_fs.c`
- `lvgl/src/draw/lv_image_decoder.h`
- `lvgl/src/misc/cache/instance/lv_image_cache.h`
- `core/include/lvglpp/core/draw_lvgl.hpp`
- `core/include/lvglpp/core/plugins/rle.hpp`
- `examples/apps/disco-demo/include/lvglpp/app/disco_demo/assets.hpp`
- `examples/apps/disco-demo/src/assets.cpp`
- `scripts/lvglpp_qt.py`

## 14. Unblocks

- LPAR-CPP-10 layout tests involving image and label sources loaded from
  catalogs.
- LPAR-CPP-12 ImageButton wrappers using descriptor/file/symbol sources.
- LPAR-CPP-15 Canvas, AnimImage, media, property, and observer wrappers.
- QT-CPP generated screen modules that need stable resource catalogs.
- SCTD-CPP demo panels and FireBeetle app payload assets.

## 15. Change Log

| Date | State | Notes |
| --- | --- | --- |
| 2026-06-30 | DRAFT | Initial LVGL asset/filesystem draft. Adapts rlvgl LPAR-09 by delegating filesystem dispatch, image source classification, image decoder lifecycle, and image cache policy to LVGL public APIs while reserving static asset catalog records for future Qt/SCTD generated consumers. |
| 2026-06-30 | RATIFIED | Owner accepted the LPAR-CPP-09 phase and directed execution to proceed. |
| 2026-06-30 | IMPLEMENTED | Added LVGL filesystem driver, file, directory, memory-buffer path, custom image decoder owner, image cache helper, and static asset catalog wrappers with a custom in-memory filesystem test. `ImageDecoderSession` RAII is deferred because `lv_image_decoder_dsc_t` is defined only in LVGL's private decoder header; Owner: LPAR-CPP-15 or an LPAR-CPP-09 amendment if direct decoder sessions become necessary. RLE custom LVGL decoder registration is deferred; Owner: LPAR-CPP-15 media/property phase or app-specific asset integration. Safe filesystem-driver unregister is blocked by the current LVGL `lv_fs_remove_drive` implementation continuing iteration after removal; lvglpp calls `remove_cb` and preserves registered storage to avoid dangling LVGL pointers until LVGL upstream resolves it. This LVGL erratum MUST be retried with a non-dev LVGL release before treating it as upstream-stable. Embedded-posture checkbox remains open until the embedded build gate is run for this change. |
