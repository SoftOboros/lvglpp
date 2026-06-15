<!--
STATUS.md — Co-located status block for lvglpp::core.

Canonical shape: see CLAUDE.md § "Doc Co-Location Policy". Sections are
fixed: Roadmap intent / As-built / Blockers / Definitions. Edit by
appending to the change log at the bottom; do not silently rewrite
history.
-->

# lvglpp::core — STATUS

Tracks `rlvgl/core` @ `v0.2.4` (commit `343f596`). Last reconciled:
2026-06-15.

Migration in progress: the hand-rolled CORE-* surface is being wrapped
onto upstream LVGL (`lv_*`) under LVGLPP-WRAP (`docs/wrap/`) + LPAR
(`docs/lpar/`). WRAP-00 (RAII `Object`/`Screen`) landed 2026-06-15; the
earlier CORE-01..07n surface still mirrors the v0.2.0 hand-rolled shape
until its migration sub-phases run.

## Roadmap intent

`lvglpp::core` is the foundation library. The intent is a 1:1
triangulation of [`rlvgl-core`](../rlvgl/core/) onto upstream LVGL,
expressed in C++20 with the rlvgl ownership/discipline carried over.

Phase plan (informal until a concepts doc lands under `docs/`):

1. **CORE-01:** Runtime + ObjectView seam (this is the "as-built" line
   today).
2. **CORE-02:** Event surface — `Event`, `EventKind`, supporting
   types (`TouchState`, `TouchPoint`, `Key`, `MAX_TOUCH_POINTS`).
   Parity with `rlvgl/core/src/event.rs`. **Concepts doc ratified
   2026-04-27** at [`docs/core-event/00-event-surface.md`](../docs/core-event/00-event-surface.md);
   execution PR is unblocked, acceptance checklist in §12 of that doc.
3. **CORE-03:** Widget node + WidgetNode borrowing rules — parity with
   `rlvgl/core/src/widget.rs`. This is the load-bearing piece for
   widgets/ and ui/.
4. **CORE-04:** Renderer trait + draw seam — parity with
   `rlvgl/core/src/{renderer,draw}.rs`.
5. **CORE-05:** Style + theme + animation — parity with
   `rlvgl/core/src/{style,theme,animation}.rs`.
6. **CORE-06:** Font helpers (bitmap + packed) — parity with
   `rlvgl/core/src/{bitmap_font,packed_font}.rs`.
7. **CORE-07:** Plugin surface for image / QR / lottie decoders —
   mirrored from `rlvgl/core/src/plugins/`. Each plugin is a separate
   compilation unit gated by `LVGLPP_CORE_<NAME>` (see OPTIONS.md).

## As-built

Implemented:

- **CORE-01:** `lvglpp::Runtime` — RAII guard around `lv_init()` with
  single-instance enforcement. Throws on host posture, calls
  `std::abort()` on embedded posture, exposes `try_make()` returning
  `lvglpp::expected<Runtime, RuntimeError>` regardless of posture.
- **CORE-01:** `lvglpp::ObjectView` — non-owning view over `lv_obj_t*`,
  `external` lifetime tag enforced by comment.
- **CORE-02:** `lvglpp::core::Event` (10-variant `std::variant`),
  `lvglpp::core::TouchState`, `lvglpp::core::TouchPoint`,
  `lvglpp::core::Key` (10-variant `std::variant` with named-key empty
  payloads + `Function`/`Character`/`Other` parametric payloads),
  `lvglpp::core::MAX_TOUCH_POINTS = 5`. All in
  `core/include/lvglpp/core/event.hpp` per the §12 acceptance
  checklist. Header-only.
- **CORE-02 conversions:** `lvglpp::playit::to_event` /
  `to_key` / `to_core` in
  `playit/include/lvglpp/playit/conversion.hpp` bridge wire-format
  specs into core types. Round-trip test
  `lvglpp_playit_conversion` exercises every variant.
- **CORE-03:** `lvglpp::core::Rect`, `lvglpp::core::Color` (with
  `to_argb8888()` and `with_alpha()` helpers), `lvglpp::core::Widget`
  abstract base in `core/include/lvglpp/core/widget.hpp`.
  Test target: `lvglpp_core_widget` (Rect equality, Color helpers,
  Widget consume-flag dispatch).
- **CORE-04:** `lvglpp::core::Renderer` abstract base with default
  `blend_rect` and `draw_pixels` implementations matching
  `rlvgl/core/src/renderer.rs:25, :33`. Test target:
  `lvglpp_core_renderer` uses a `RecordingRenderer` to verify the
  default decompositions (blend → fill, draw_pixels → fill loop,
  short-buffer skip).
- **CORE-05:** `lvglpp::core::Style`, `StyleBuilder`, `Theme`,
  `LightTheme`, `DarkTheme`, `Easing` (all 9 curves + math),
  `LoopMode` in `core/include/lvglpp/core/style.hpp`. Test target:
  `lvglpp_core_style` covers Style defaults, builder chain, theme
  parity, easing math at canonical sample points, LoopMode shape.
- **CORE-07:** Plugin slot CMake options (`LVGLPP_CORE_PNG`,
  `LVGLPP_CORE_JPEG`, ... 13 slots, all defaulting OFF) declared in
  `core/CMakeLists.txt`. `core/src/plugins/.gitkeep` marks the
  per-plugin landing zone. No actual plugin source landed — that's
  per-sub-phase work (CORE-07a, ...).
- Module umbrella `core.hpp` re-exports `event.hpp` + `renderer.hpp`
  + `runtime.hpp` + `style.hpp` + `widget.hpp`.
- Seam header `lvglpp/std/expected.hpp`.

- **CORE-03a** (Widget tree): `lvglpp::core::WidgetNode` in
  `core/include/lvglpp/core/widget_node.hpp` with
  `unique_ptr<Widget> widget`, `vector<WidgetNode> children`,
  `optional<string_view> tag`. `dispatch_event` (DFS, first-consume
  short-circuit) and `draw` (parent-then-children DFS) per
  `rlvgl/core/src/lib.rs:138, :154`. `find_by_tag` (const +
  non-const overloads) lifted from rlvgl playit into
  `lvglpp::core` per CORE-03a §5.4. Test target
  `lvglpp_core_widget_node` (7 fixtures: dispatch order, consume
  short-circuit, draw order, find_by_tag root/nested/untagged/mut).
- **CORE-04a** (minimal draw helpers): `draw_widget_bg` +
  `draw_border_straight` in
  `core/include/lvglpp/core/draw_helpers.hpp`. Handles the
  `radius == 0` path identically to `rlvgl/core/src/draw.rs:480, :425`;
  the `radius > 0` rounded-corner path is documented as DELTA and
  deferred to CORE-04b.
- **CORE-06**: `lvglpp::core::BitmapFont`, `PackedFont`,
  `GlyphMetric` types in `core/include/lvglpp/core/font.hpp`.
  Bring-up font `lvglpp::core::fonts::FONT_6X10` declared in
  `core/include/lvglpp/core/fonts/font_6x10.hpp` and defined in
  `core/src/fonts/font_6x10.cpp`, with the 713-byte glyph blob
  copied byte-identically from rlvgl as
  `core/src/fonts/font_6x10.bin` and embedded via CMake-time hex
  conversion (no xxd/objcopy dependency). Test target
  `lvglpp_core_font` covers minimal-glyph dispatch, non-printable
  skip, scaled dimensions, and bring-up font emit-shape.

- **LVGLPP-WRAP-00** (RAII lv_obj core): `lvglpp::core::Object` +
  `Screen` in `core/include/lvglpp/core/object.hpp` /
  `core/src/object.cpp`. Move-only RAII owner of an `lv_obj_t*`; a
  self-registered `LV_EVENT_DELETE` callback nulls the handle on
  LVGL-driven deletion (parent delete / `lv_obj_clean`) so the
  destructor never double-frees; `user_data` holds the C++ back-pointer
  (rebased on move); `try_make` (→ `expected`) + throwing `make`
  (abort under embedded posture). **First lvglpp code to call `lv_*`.**
  Test target `lvglpp_core_object` (delete-on-drop, move transfer,
  parent-delete double-free safety, Screen create+load, host `make`).
  Additive — the hand-rolled `Widget`/`WidgetNode`/`Renderer` layer is
  unchanged, retired later under LVGLPP-WRAP-01..0N. See
  `docs/wrap/00-concepts.md`.
- **LPAR-02** (object substrate): `Object` gains `add_flag`/`remove_flag`/
  `has_flag` over `ObjectFlag`, `add_state`/`remove_state`/`has_state`/
  `state` over `ObjectState`, `hit_test`, and non-owning tree accessors
  `parent`/`child_count`/`child` (→ `ObjectView`), all wrapping `lv_obj_*`
  and empty-safe. `ObjectFlag`/`ObjectState` mirror `lv_obj_flag_t`/
  `lv_state_t` (Standards Action). Test
  `lvglpp_core_object_substrate`. See
  `docs/core-object/00-object-substrate.md`.
- **LPAR-05** (scroll): `Object` gains `scroll_to`/`scroll_by`/
  `scroll_x`/`scroll_y`/`set_scroll_dir`/`set_scrollbar_mode`/
  `set_scroll_snap` over `lv_obj_scroll_*`, with `ScrollbarMode`/
  `ScrollSnap`/`ScrollDir` mirror enums (Standards Action). Empty-safe.
  Test `lvglpp_core_object_scroll`. See
  `docs/core-scroll/00-scroll-runtime.md`.
- **LPAR-10** (layout): `Object` gains `set_size`, `set_flex_flow`/
  `set_flex_grow`/`set_flex_align`, `set_grid_dsc`/`set_grid_cell`/
  `set_grid_align`, and `size_content`/`pct` sizing helpers over
  `lv_obj_set_flex_*`/`grid_*`. `FlexFlow`/`FlexAlign`/`GridAlign` mirror
  enums. Grid track arrays are caller-owned and MUST outlive the object
  (documented `borrows`-into-LVGL). Empty-safe. Test
  `lvglpp_core_object_layout`. See `docs/core-layout/00-layout.md`.

Stubbed (chapter ratified, no implementation yet):

- CORE-04b — rounded-corner draw helpers (governs `radius > 0`
  path). Concepts amendment + execution deferred until first
  caller needs rounded corners.
- CORE-07a through CORE-07m — per-plugin sub-phases (PNG, JPEG, GIF,
  ...). Each lands when its first call site needs it.
- CORE-05a / CORE-05b / ... — richer animation surface (Fade,
  Slide, Motion, FadeTransition, Timeline). Deferred until first
  call site.

## Blockers

- **Per-plugin sub-phases (CORE-07a..m).** CORE-07 mechanism in
  place; each plugin needs its own concepts doc + execution PR
  before it can be enabled. Owner: per-plugin implementer.
- **CORE-04b rounded draw helpers.** `draw_widget_bg` falls back
  to axis-aligned bg when `style.radius > 0` today. First widget
  needing rounded corners triggers the CORE-04b concepts amendment
  + execution. Owner: that widget's implementer.
- **`std::expected` floor.** AppleClang 14 and most embedded ARM
  toolchains lack `<expected>`. We ship a vendored polyfill under
  `third_party/lvglpp_expected/` that covers the subset we use; if a
  caller needs `and_then` / `or_else` / `transform`, the polyfill must
  grow first or be swapped for `tl::expected`. See
  `third_party/lvglpp_expected/README.md`. Owner: implementer of the
  first call site that needs richer monadic ops.
- **Embedded posture wiring.** `LVGLPP_EMBEDDED_POSTURE` is defined and
  threaded through CMake but has not been exercised by a real cross
  build. The first board target under `examples/` will exercise it.
  Owner: first board-bring-up implementer.
- **Plugin features.** All `LVGLPP_CORE_<NAME>` options listed in
  OPTIONS.md are intentionally not wired today. Each requires its own
  decoder integration decision (host-only vs. embedded-capable). Owner:
  per-plugin implementer.

## Definitions

Local glossary. Forms follow `CLAUDE.md` §
"Definitions — reference vs. restatement".

- **`Runtime`** — As defined in
  `core/include/lvglpp/core/runtime.hpp:line` (this repo);
  used without modification. Mirrors the bootstrap role of
  `Application` in `rlvgl/core/src/application.rs`.
- **`ObjectView`** — As defined in
  `core/include/lvglpp/core/runtime.hpp:line` (this repo);
  the C++ name for the `external`-tagged borrow over an `lv_obj_t*`.
  Owned by chapter CORE-03 once the widget tree concepts doc lands;
  current shape is provisional.
- **`RuntimeError`** — As defined in
  `core/include/lvglpp/core/runtime.hpp:line`. Distinct from
  `lv_result_t` (LVGL's small enum); the lvglpp surface is richer and
  mirrored on the rlvgl side via `Result<_, _>`.
- **`Event`** — As defined in
  `core/include/lvglpp/core/event.hpp` (this repo); mirrors
  `rlvgl/core/src/event.rs:43`. Authoritative chapter:
  `docs/core-event/00-event-surface.md`.
- **`TouchState`, `TouchPoint`, `Key`, `MAX_TOUCH_POINTS`** — As
  defined in `core/include/lvglpp/core/event.hpp`; frozen by
  concepts doc §5.2 / §3 / §5.3 / §5.4.
- **Freestanding subset** — As defined in
  `docs/std-mapping.md` § "Freestanding subset"; that document is the
  authoritative source for which `<header>`s `lvglpp::core` may
  include.
- **`EventKind`, `Widget`, `WidgetNode`, `Renderer`, `Theme`,
  `Style`, `Animation`** — Owned by chapter CORE-03 / CORE-04 /
  CORE-05; do not exist in repo yet.
- **`rle::Error`, `rle::ParsedBlob`, `rle::parse_blob`,
  `rle::decode_into`, `rle::consts::*`** — As defined in
  `rlvgl/rlvgl-decomp/src/lib.rs:42` (`Error`), `:341`
  (`parse_rle_blob`), `:382` (`decode_argb_into`), `:27` (`mod
  consts`); mirrored here as `core/include/lvglpp/core/rle.hpp`.
  Consume-only: the encoder side (`write_rle_blob`, `encode_rgba`,
  lib.rs:201/:318) is deliberately NOT mirrored. DELTA: decode target
  is `std::span<core::Color>` (RGB565 palette converted at decode
  time, a=255) instead of a native-u32 buffer. Frozen constants are
  Standards Action (must agree with rlvgl).

## Change log

- 2026-04-27 — Initial scaffold. Phase CORE-01 (Runtime + ObjectView)
  landed. All later phases are stubs.
- 2026-04-27 — CORE-02 concepts doc ratified at
  `docs/core-event/00-event-surface.md`. Variant sets and
  `MAX_TOUCH_POINTS = 5` frozen with Standards Action registration.
  Execution unblocked.
- 2026-04-27 — CORE-02 execution landed.
  `core/include/lvglpp/core/event.hpp` defines `Event`, `Key`,
  `TouchState`, `TouchPoint`, `MAX_TOUCH_POINTS` per the §12
  acceptance checklist (all 9 bullets satisfied). `lvglpp::playit`
  gained `to_event` / `to_key` / `to_core` conversions in
  `playit/include/lvglpp/playit/conversion.hpp`. Round-trip test
  `lvglpp_playit_conversion` passes; 3/3 ctest entries green.
- 2026-04-27 — CORE-03, CORE-04, CORE-05, CORE-07 concepts docs
  ratified at `docs/core-widget/`, `docs/core-renderer/`,
  `docs/core-style/`, `docs/core-plugins/`. CORE-06 chapter
  ratified at `docs/core-font/`; execution deferred to WID-01.
- 2026-04-27 — CORE-03, CORE-04, CORE-05 minimal execution
  landed. `widget.hpp` (Rect/Color/Widget), `renderer.hpp`
  (Renderer with default blend/blit), `style.hpp` (Style /
  StyleBuilder / Theme / LightTheme / DarkTheme / Easing /
  LoopMode). Per-module `core/tests/` directory wired up with
  three test targets (widget/renderer/style). 7/7 ctest entries
  green; `LVGLPP_EMBEDDED_POSTURE=ON` builds clean.
- 2026-04-27 — CORE-07 plugin-slot CMake options declared
  (13 slots, all default OFF). No plugin source landed; that's
  sub-phase work (CORE-07a..m).
- 2026-04-27 — CORE-04a (minimal draw helpers) landed.
  `core/include/lvglpp/core/draw_helpers.hpp` defines
  `draw_widget_bg` + `draw_border_straight` for the `radius == 0`
  path. Rounded-corner path deferred to CORE-04b (DELTA documented
  in the header).
- 2026-04-27 — CORE-06 execution landed.
  `core/include/lvglpp/core/font.hpp` defines `BitmapFont`,
  `PackedFont`, `GlyphMetric` per
  `docs/core-font/00-fonts.md` §5.1 / §5.2. Bring-up font
  `FONT_6X10` (713 bytes, byte-identical to
  `rlvgl/core/src/bitmap_font_6x10.bin`) embedded via CMake-time
  hex conversion. Test target `lvglpp_core_font` registered;
  bring-up font verified to emit fill_rects for printable glyphs
  and to skip the blank space glyph entirely.
- 2026-06-15 — LVGLPP-WRAP-00 (additive RAII `lv_obj` core) landed.
  `core/include/lvglpp/core/object.hpp` + `core/src/object.cpp` define
  `lvglpp::core::Object` and `Screen` — the first lvglpp surface that
  calls upstream LVGL (`lv_obj_create`/`lv_obj_delete`). Move-only;
  self-registered `LV_EVENT_DELETE` delete-safety; `user_data`
  back-pointer rebased on move; `try_make` (→ `expected`) + throwing
  `make` (abort under `LVGLPP_EMBEDDED_POSTURE`). `lv_conf.h` baseline
  confirmed (`LV_USE_OBJ`). Test `lvglpp_core_object` green (31/31 full
  suite); builds + runs clean under default and embedded posture.
  Removes nothing (WRAP-00 §5.7). rlvgl pin reference bumped to v0.2.4
  @ `343f596`. Ratified chapter: `docs/wrap/00-concepts.md`.
- 2026-06-15 — LPAR-02 (object substrate) landed. `Object`
  (`object.hpp`/`object.cpp`) gains flag/state/hit-test/tree-query
  wrappers over `lv_obj_*`; `ObjectFlag`/`ObjectState` mirror enums
  (Standards Action). Test `lvglpp_core_object_substrate` green; builds +
  runs under default and `LVGLPP_EMBEDDED_POSTURE=ON`. Additive.
  Ratified chapter: `docs/core-object/00-object-substrate.md`.
- 2026-06-15 — LPAR-05 (scroll) landed. `Object` gains scroll
  position/by/to getters+setters, scrollbar-mode, snap, and dir over
  `lv_obj_scroll_*`; `ScrollbarMode`/`ScrollSnap`/`ScrollDir` mirror
  enums. Test `lvglpp_core_object_scroll` green (full suite 33/33);
  builds + runs under both postures. Additive. Ratified chapter:
  `docs/core-scroll/00-scroll-runtime.md`.
- 2026-06-15 — LPAR-10 (layout) landed. `Object` gains flex (flow/grow/
  align), grid (dsc/cell/align), and sizing (`set_size`, `size_content`,
  `pct`) over `lv_obj_set_flex_*`/`grid_*`; `FlexFlow`/`FlexAlign`/
  `GridAlign` mirror enums. Grid track arrays are caller-owned and must
  outlive the object (frozen rule). Test `lvglpp_core_object_layout`
  green (full suite 34/34); both postures. Additive. Ratified chapter:
  `docs/core-layout/00-layout.md`.
- 2026-04-27 — CORE-03a chapter ratified at
  `docs/core-widget/01-widget-node.md` and execution landed.
  `lvglpp::core::WidgetNode` provides the tree composition layer
  above the CORE-03 Widget abstract base. `find_by_tag` lifted
  from rlvgl playit into core. Test target
  `lvglpp_core_widget_node` green; embedded posture clean.
- 2026-06-07 — DEMO-04 (consume-only RLE icon decoder) landed.
  `core/include/lvglpp/core/rle.hpp` + `core/src/rle.cpp` mirror the
  rlvgl-decomp parser (`parse_rle_blob`) and ARGB decode loop
  (`decode_argb_into`) from `rlvgl/rlvgl-decomp/src/lib.rs` (v0.2.0 @
  79f730d), decoding into a `std::span<core::Color>` buffer. The
  encoder (`write_rle_blob` / `encode_rgba`) is explicitly OUT OF
  SCOPE per the chapter's consume-only boundary (CLAUDE.md §
  "`creator-cpp` is deferred"). Zero-alloc, `-fno-exceptions` clean.
  Added an `expected<void, E>` partial specialization to the
  `third_party/lvglpp_expected` polyfill (frozen `decode_into`
  signature is `expected<void, Error>`). `core.hpp` re-exports
  `rle.hpp`. Test target `lvglpp_core_rle` (index / short-repeat /
  inline single+double / long-repeat decode, four frozen error paths,
  plus a real `file.rle` asset decode); 20/20 ctest entries green.
