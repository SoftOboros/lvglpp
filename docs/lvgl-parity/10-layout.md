<!--
10-layout.md - lvglpp LPAR-10 mirror layout and geometry plan.
-->

# LPAR-CPP-10 - LVGL Layout and Geometry Substrate

Status: **RATIFIED** (2026-06-30). Normative for the LPAR-CPP-10 mirror
of rlvgl `v0.2.5` layout substrate work.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** are interpreted per RFC 2119 and
RFC 8174 when capitalized.

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-10 relationship |
| --- | --- | --- |
| LVGL object geometry | `lvgl/src/core/lv_obj_pos.h`, `lvgl/src/core/lv_obj_pos.c` | Canonical C behavior for object position, size, content size, alignment, coordinates, layout dirty marking, and explicit layout update. lvglpp MUST wrap these APIs rather than re-implement rlvgl's retained layout pass for LVGL-backed objects. |
| LVGL layout registry | `lvgl/src/layouts/lv_layout.h`, `lvgl/src/layouts/lv_layout.c` | Canonical layout id model. LPAR-CPP-10 exposes built-in layout selection and MAY reserve custom layout registration only with explicit callback/userdata ownership. |
| LVGL flex layout | `lvgl/src/layouts/flex/lv_flex.h`, `lvgl/src/layouts/flex/lv_flex.c` | Canonical flex flow, placement, and grow behavior. lvglpp mirrors enum values and delegates to `lv_obj_set_flex_flow`, `lv_obj_set_flex_align`, and `lv_obj_set_flex_grow` when `LV_USE_FLEX` is enabled. |
| LVGL grid layout | `lvgl/src/layouts/grid/lv_grid.h`, `lvgl/src/layouts/grid/lv_grid.c` | Canonical grid track, alignment, descriptor-array, and cell placement behavior. lvglpp mirrors enum values and delegates to `lv_obj_set_grid_dsc_array`, `lv_obj_set_grid_align`, and `lv_obj_set_grid_cell` when `LV_USE_GRID` is enabled. |
| LVGL style layout properties | `lvgl/src/misc/lv_style.h`, `lvgl/src/misc/lv_style_gen.h` | Canonical style property ids for width, height, align, padding, margin, row/column gaps, layout, flex, and grid properties. LPAR-CPP-10 extends typed helpers from LPAR-CPP-07 instead of adding parallel layout state. |
| rlvgl layout phase | `rlvgl/docs/concepts/LPAR-10-LAYOUT.md` (`v0.2.5 @ f999f75`) | Canonical cross-language vocabulary for percent/content sizing, flex/grid concepts, padding, margin, gaps, and dirty layout. lvglpp adapts the vocabulary to LVGL object/style APIs and does not port the Rust `ObjectNode` slot or software layout engine. |
| Existing lvglpp LVGL object/style substrate | `core/include/lvglpp/core/object.hpp`, `core/include/lvglpp/core/style_lvgl.hpp` | `LvObject`, `ObjectView`, `LvStyle`, `StyleSelector`, and local style property helpers are the handles this phase extends. |
| Existing rect-based compatibility helpers | `core/include/lvglpp/core/widget.hpp`, `widgets/include/lvglpp/widgets/*.hpp`, `ui/include/lvglpp/ui/*.hpp` | Compatibility-only static rectangle surfaces. LPAR-CPP-10 MUST NOT treat these helpers as LVGL parity and MUST NOT break their public shape while adding LVGL-backed layout wrappers. |
| Ownership discipline | top-level `AGENTS.md` | Every raw `lv_obj_t*`, layout callback, userdata pointer, grid descriptor array pointer, and style pointer touched by this phase MUST carry explicit ownership/lifetime comments. |

If this chapter disagrees with LVGL about geometry, layout, flex, grid,
or style property lifetime, LVGL wins. If it disagrees with rlvgl
`v0.2.5` about cross-language widget-visible layout intent, rlvgl wins
and this chapter must be amended.

## 1. Purpose

LPAR-CPP-10 provides the LVGL-backed geometry and layout substrate needed
by later widget wrappers, generated Qt screens, and SCTD demo panels. It
gives lvglpp code a way to:

- set and query object position, size, content size, and coordinates;
- express pixel, percent, and content-size values using LVGL's
  `lv_pct` and `LV_SIZE_CONTENT` encodings;
- align objects to parents or other objects through LVGL alignment APIs;
- select built-in LVGL layouts and force or mark layout updates;
- configure flex containers and flex child grow factors;
- configure grid containers, grid track descriptor arrays, and grid cell
  placement with explicit descriptor-array lifetime;
- configure padding, margin, row gaps, column gaps, and layout style
  properties through typed style helpers;
- preserve existing static `ui` helpers as compatibility utilities while
  the LVGL-backed parity path uses LVGL layout underneath.

This phase intentionally does not copy rlvgl's Rust `LayoutState`,
`LayoutPass`, or custom flex/grid engines. LVGL already owns retained
object geometry, size resolution, flex/grid placement, dirty layout
propagation, and update ordering for LVGL-backed widgets.

## 2. Problem Statement

LPAR-CPP-02 through LPAR-CPP-09 establish real LVGL objects, displays,
input, scroll, timers, animation, styles, themes, labels, images, and
asset sources. The missing substrate is LVGL object geometry and layout:

- `core/include/lvglpp/core/object.hpp` wraps object ownership, tree
  operations, flags, and states, but exposes no typed geometry,
  alignment, layout, flex, or grid helpers.
- `core/include/lvglpp/core/style_lvgl.hpp` exposes generic local style
  property helpers and a few typed style setters, but does not yet expose
  typed helpers for per-side padding, per-side margin, row/column gaps,
  layout, flex, or grid style properties.
- Existing `ui` helpers and compatibility widgets work in static
  rectangles. They are useful as legacy APIs and test references, but
  they are not an LVGL layout runtime and cannot satisfy LVGL-backed
  parity.
- rlvgl LPAR-10 adds a Rust-side layout slot and layout pass because
  rlvgl owns its software object tree. lvglpp must instead use LVGL's
  existing `lv_obj_*`, `lv_flex_*`, and `lv_grid_*` behavior so C++ and
  C observe one geometry source of truth.
- Grid descriptor arrays are borrowed by LVGL as sentinel-terminated
  `int32_t` arrays. A C++ wrapper must make that borrowing lifetime
  visible and hard to misuse.

Without this phase, later widgets can create LVGL nodes but cannot
idiomatically configure or verify their geometry, flex/grid layout, or
style-driven spacing through the C++ parity surface.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **Coordinate** | As defined in `lvgl/src/core/lv_obj_pos.h`; adapted: lvglpp passes `std::int32_t` values to LVGL for pixels, `lv_pct`, or special size encodings where LVGL permits them. |
| **Size value** | As defined by `lv_obj_set_size`, `lv_obj_set_width`, and `lv_obj_set_height`; adapted: lvglpp exposes named helpers for pixel, percent, and content-sized values while preserving LVGL's integer representation. |
| **Percent size** | As defined by LVGL `lv_pct(v)` in `lvgl/src/core/lv_obj_pos.h`; used without modification. It resolves against the parent's content area. |
| **Content size** | As defined by LVGL `LV_SIZE_CONTENT`; used without modification. It resolves from object content or children according to LVGL rules. |
| **Object alignment** | As defined by LVGL `lv_align_t`; adapted: lvglpp mirrors alignment through a C++ enum or typed helper and delegates to `lv_obj_set_align`, `lv_obj_align`, `lv_obj_align_to`, and `lv_obj_center`. |
| **Layout kind** | As defined by `lvgl/src/layouts/lv_layout.h` `lv_layout_t`; mirrored by `LayoutKind` in lvglpp for `None`, `Flex`, and `Grid` when the matching LVGL features are enabled. |
| **Layout dirty** | As defined by `lv_obj_mark_layout_as_dirty`; adapted: lvglpp exposes a wrapper that marks LVGL's layout dirty state, not a separate C++ dirty flag. |
| **Layout update** | As defined by `lv_obj_update_layout`; adapted: lvglpp exposes an explicit update wrapper for tests and generated code that need current coordinates before redraw. |
| **Flex flow** | As defined in `lvgl/src/layouts/flex/lv_flex.h` `lv_flex_flow_t`; mirrored by a C++ enum with Standards Action registration. |
| **Flex align** | As defined in `lvgl/src/layouts/flex/lv_flex.h` `lv_flex_align_t`; mirrored by a C++ enum with Standards Action registration. |
| **Flex grow** | As defined by `lv_obj_set_flex_grow`; used without modification as an unsigned child grow factor stored by LVGL style state. |
| **Grid align** | As defined in `lvgl/src/layouts/grid/lv_grid.h` `lv_grid_align_t`; mirrored by a C++ enum with Standards Action registration. |
| **Grid track** | As defined by LVGL pixel track values, `LV_GRID_FR(x)`, `LV_GRID_CONTENT`, and `LV_GRID_TEMPLATE_LAST`; adapted: lvglpp owns a descriptor list wrapper that appends the required sentinel. |
| **Grid descriptor array** | As defined by `lv_obj_set_grid_dsc_array`; adapted: LVGL observes caller storage, so lvglpp MUST document and preserve the descriptor storage lifetime for as long as the grid object can use it. |
| **Grid cell** | As defined by `lv_obj_set_grid_cell`; adapted: lvglpp exposes a value or direct helper for column/row position, span, and per-axis alignment. |
| **Static rectangle helper** | Existing compatibility rectangle-based helpers under `core`, `widgets`, `ui`, and app examples; used without modification. They are not LVGL-backed parity layout. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact | lvglpp mirror target |
| --- | --- | --- |
| Object position, size, content size, coordinates, alignment | `lvgl/src/core/lv_obj_pos.h` | geometry helpers on `LvObject` and/or free `ObjectView` helpers |
| Built-in layout ids and custom layout callbacks | `lvgl/src/layouts/lv_layout.h` | `LayoutKind`, `set_layout`, optional future custom layout owner |
| Flex flow, placement, grow | `lvgl/src/layouts/flex/lv_flex.h` | `FlexFlow`, `FlexAlign`, `set_flex_flow`, `set_flex_align`, `set_flex_grow` |
| Grid tracks, alignment, cells | `lvgl/src/layouts/grid/lv_grid.h` | `GridTrackList`, `GridAlign`, `set_grid_descriptor_array`, `set_grid_align`, `set_grid_cell` |
| Style spacing and layout properties | `lvgl/src/misc/lv_style.h`, `lvgl/src/misc/lv_style_gen.h` | typed `LvStyle` and local-style helpers for width, height, align, padding, margin, gaps, layout, flex, and grid |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-10-LAYOUT.md` | this chapter's terminology and phase relationship |
| Existing LVGL object owner/view | `core/include/lvglpp/core/object.hpp` | extension point for geometry helpers |
| Existing LVGL style owner/view | `core/include/lvglpp/core/style_lvgl.hpp` | extension point for style layout helpers |
| Existing static rectangle helpers | `core/include/lvglpp/core/widget.hpp`, `widgets/include/lvglpp/widgets/*.hpp`, `ui/include/lvglpp/ui/*.hpp` | preserved compatibility surface |

## 5. Frozen Decisions - LVGL Underneath

1. **No competing layout engine.** LPAR-CPP-10 MUST NOT implement a
   C++ clone of rlvgl's retained `LayoutPass`, flex engine, or grid
   engine for LVGL-backed objects. LVGL owns layout computation.
2. **One geometry source of truth.** For LVGL-backed objects, the
   authoritative geometry is the `lv_obj_t` geometry stored and computed
   by LVGL. C++ wrappers may expose values but MUST NOT cache geometry in
   a separate mutable mirror.
3. **Compatibility stays compatibility.** Existing static `ui` helpers
   and compatibility widgets MAY remain useful, but they do not satisfy
   LVGL parity unless they operate on real LVGL objects through this
   phase's wrappers.
4. **No `Widget::set_bounds` parity shim.** rlvgl needs a software
   widget bounds reconciliation hook. lvglpp MUST NOT add a parallel
   compatibility widget resizing contract as the LVGL parity path.
5. **Layout state is LVGL style/object state.** Flex, grid, padding,
   margin, gaps, width, height, alignment, and layout selection are LVGL
   object/style properties. lvglpp should make those properties typed and
   visible, not duplicate them.
6. **Feature gates follow LVGL config.** Flex wrappers compile only when
   `LV_USE_FLEX` is enabled. Grid wrappers compile only when
   `LV_USE_GRID` is enabled. Tests must respect those gates.
7. **Grid track ownership is explicit.** Any C++ descriptor wrapper
   passed to LVGL MUST own or borrow its track array explicitly and state
   how long that storage must outlive the container using it.

## 6. Frozen Decisions - Geometry and Size Surface

LPAR-CPP-10 SHALL introduce or reserve:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `Coord` or direct `std::int32_t` | LVGL coordinate value | value |
| `SizeValue` helpers | pixel values, `lv_pct`, `LV_SIZE_CONTENT` | value |
| `LvArea` reuse | `lv_area_t` | value, already introduced by display wrappers |
| `Align` | `lv_align_t` | value enum, Standards Action |
| geometry helpers | `lv_obj_set_pos`, `lv_obj_set_size`, getters, coordinate helpers | borrow/observe `ObjectView` |

Normative rules:

1. Pixel size helpers SHALL pass raw integer pixel values to LVGL.
2. Percent helpers SHALL call LVGL's `lv_pct` or return an equivalent
   LVGL percent-encoded value. They MUST NOT invent a second percent
   representation for LVGL-backed objects.
3. Content-size helpers SHALL use `LV_SIZE_CONTENT`.
4. Object geometry wrappers SHALL cover `set_pos`, `set_x`, `set_y`,
   `set_size`, `set_width`, `set_height`, `set_content_width`,
   `set_content_height`, `x`, `y`, `x2`, `y2`, `width`, `height`,
   `content_width`, `content_height`, `coords`, and `content_coords`
   where LVGL exposes public APIs.
5. Alignment wrappers SHALL cover `set_align`, `align`, `align_to`, and
   `center`. `align_to` observes the base object and MUST document that
   LVGL does not keep a live relationship after the call.
6. Getters that depend on current layout SHALL document the need to call
   `update_layout` before reading coordinates when redraw has not yet
   recalculated them.

## 7. Frozen Decisions - Layout Dirty and Update Surface

LPAR-CPP-10 SHALL expose:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `LayoutKind` | `lv_layout_t` | value enum, Standards Action |
| `set_layout` | `lv_obj_set_layout` | borrows `ObjectView` |
| `is_layout_positioned` | `lv_obj_is_layout_positioned` | observes `ObjectView` |
| `mark_layout_dirty` | `lv_obj_mark_layout_as_dirty` | borrows `ObjectView` |
| `update_layout` | `lv_obj_update_layout` | observes `ObjectView` |

Normative rules:

1. `LayoutKind::None` MUST map to `LV_LAYOUT_NONE`.
2. `LayoutKind::Flex` and `LayoutKind::Grid` MUST exist only when the
   matching LVGL feature macros expose those layout ids.
3. `mark_layout_dirty` and `update_layout` MUST delegate to LVGL and
   MUST NOT maintain an additional C++ dirty graph.
4. Custom layout registration is deferred unless an amendment names the
   callback ownership model. If added, callbacks MUST document the
   lifetime of `void* user_data` and whether C++ owns or observes it.

## 8. Frozen Decisions - Flex Surface

When `LV_USE_FLEX` is enabled, LPAR-CPP-10 SHALL introduce or reserve:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `FlexFlow` | `lv_flex_flow_t` | value enum, Standards Action |
| `FlexAlign` | `lv_flex_align_t` | value enum, Standards Action |
| `set_flex_flow` | `lv_obj_set_flex_flow` | borrows `ObjectView` |
| `set_flex_align` | `lv_obj_set_flex_align` | borrows `ObjectView` |
| `set_flex_grow` | `lv_obj_set_flex_grow` | borrows `ObjectView` |

Normative rules:

1. `FlexFlow` SHALL mirror all LVGL values: row, column, row wrap, row
   reverse, row wrap reverse, column wrap, column reverse, and column
   wrap reverse.
2. `FlexAlign` SHALL mirror all LVGL values: start, end, center,
   space evenly, space around, and space between.
3. Container helpers SHALL delegate to LVGL's object setters. Child grow
   helpers SHALL delegate to LVGL's grow setter.
4. Style helper coverage SHOULD include direct `LvStyle` setters for
   flex flow, main placement, cross placement, track placement, and grow
   so generated code can choose either object setters or style objects.
5. Tests MUST verify wrapper-to-LVGL enum mapping and at least one real
   flex layout update when LVGL flex is enabled.

## 9. Frozen Decisions - Grid Surface

When `LV_USE_GRID` is enabled, LPAR-CPP-10 SHALL introduce or reserve:

| C++ concept | LVGL backing | Ownership role |
| --- | --- | --- |
| `GridAlign` | `lv_grid_align_t` | value enum, Standards Action |
| `GridTrackList` | `int32_t[]` ending in `LV_GRID_TEMPLATE_LAST` | owns descriptor storage |
| `GridTrackView` if needed | `const int32_t*` | observes external sentinel-terminated descriptor storage |
| `grid_fr` helper | `LV_GRID_FR` or `lv_grid_fr` | value |
| `grid_content` helper | `LV_GRID_CONTENT` | value |
| `set_grid_descriptor_array` | `lv_obj_set_grid_dsc_array` | LVGL observes descriptor arrays |
| `set_grid_align` | `lv_obj_set_grid_align` | borrows `ObjectView` |
| `set_grid_cell` | `lv_obj_set_grid_cell` | borrows `ObjectView` |

Normative rules:

1. `GridAlign` SHALL mirror all LVGL values: start, center, end,
   stretch, space evenly, space around, and space between.
2. `GridTrackList` MUST append `LV_GRID_TEMPLATE_LAST` and keep the
   underlying storage stable for as long as LVGL may read it.
3. Passing raw external descriptor arrays MAY be supported through a view
   type, but the function contract MUST state that the caller owns the
   array and must keep it alive until the container no longer uses the
   grid descriptor.
4. `GridTrackList` SHOULD be move-only or otherwise prevent accidental
   shared mutation after an object observes its storage. If shared
   lifetime is later required, `std::shared_ptr` use MUST preserve a
   single owner/writer model so observers cannot race the LVGL borrow.
5. Style helper coverage SHOULD include direct `LvStyle` setters for
   grid column/row descriptor arrays, column/row alignment, and grid
   cell placement properties.
6. Tests MUST verify wrapper-to-LVGL enum mapping, sentinel appending,
   descriptor lifetime documentation, and at least one real grid layout
   update when LVGL grid is enabled.

### 9.A - Style Spacing and Layout Properties

LPAR-CPP-10 SHALL extend typed style coverage from LPAR-CPP-07:

| Style concept | LVGL property or helper | Required coverage |
| --- | --- | --- |
| width / height | `LV_STYLE_WIDTH`, `LV_STYLE_HEIGHT` | typed `LvStyle` and local-style helpers where useful |
| align | `LV_STYLE_ALIGN` | typed helper using `Align` |
| padding | `LV_STYLE_PAD_TOP/BOTTOM/LEFT/RIGHT/RADIAL` | all-side and per-side setters |
| gaps | `LV_STYLE_PAD_ROW`, `LV_STYLE_PAD_COLUMN` | row and column gap setters |
| margin | `LV_STYLE_MARGIN_TOP/BOTTOM/LEFT/RIGHT` | all-side and per-side setters |
| layout | `LV_STYLE_LAYOUT` | typed helper using `LayoutKind` |
| flex | `LV_STYLE_FLEX_*` | typed helper coverage when `LV_USE_FLEX` |
| grid | `LV_STYLE_GRID_*` | typed helper coverage when `LV_USE_GRID` |

Normative rules:

1. Spacing helpers SHALL write LVGL style properties. They MUST NOT add a
   separate `LayoutStyle` cache for LVGL-backed objects.
2. Per-side padding and margin helpers SHALL coexist with existing
   `set_pad_all` and `set_margin_all`.
3. Row and column gap helpers SHALL use LVGL's `PAD_ROW` and
   `PAD_COLUMN` properties.
4. Object-local style helpers MAY be free functions using
   `set_local_style_prop`; if exposed, they MUST use the same enum and
   size helper vocabulary as `LvStyle`.
5. Resolved-style tests SHOULD verify that typed helpers write the
   expected LVGL property ids and values.

## 10. Reconciliation vs Adjacent lvglpp Primitives

| Existing primitive | LPAR-CPP-10 relationship |
| --- | --- |
| `Runtime` | Must be initialized before LVGL geometry/layout wrappers are used. No semantic change. |
| `LvObject` / `ObjectView` | Primary handles for geometry, layout, flex, and grid helpers. |
| `LvStyle` / local style helpers | Primary handles for style-driven size, spacing, layout, flex, and grid properties. |
| `LvArea` | Reused for coordinate/content-coordinate reads. |
| LPAR-CPP-08 labels/images | Primary content widgets used by layout tests. Their LVGL geometry is controlled by this phase's object wrappers. |
| LPAR-CPP-09 asset sources | Can supply images for layout smoke tests, but asset lookup/cache behavior is unchanged. |
| compatibility `core::Widget`, `widgets`, and `ui` rectangle helpers | Preserved as static helpers. No `Widget::set_bounds` or compatibility layout engine is required for LVGL parity. |
| `scripts/lvglpp_qt.py` | Future generated screens should target LVGL object/style layout wrappers from this phase. Parser/emitter work remains under QT/SCTD phases. |

## 11. Non-Goals

- No C++ clone of rlvgl's `LayoutState`, `LayoutPass`, flex engine, or
  grid engine for LVGL-backed objects.
- No migration or removal of existing static rectangle helpers.
- No custom LVGL layout registration in v1 unless a later amendment
  defines callback/userdata ownership.
- No generated Qt screen emission in this phase.
- No SCTD state-chart generation in this phase.
- No board-specific FireBeetle or STM32 display geometry integration.
- No direct use of private LVGL layout internals.

## 12. Acceptance Checklist

LPAR-CPP-10 implementation is complete only when:

- [x] a ratified change-log entry marks this chapter accepted;
- [x] geometry helpers cover position, size, content size, coordinates,
      alignment, and current-layout read caveats;
- [x] percent and content-size helpers delegate to LVGL encodings;
- [x] layout kind, dirty, update, and layout-positioned wrappers
      delegate to LVGL without a parallel C++ dirty graph;
- [x] flex enum mappings, container setters, child grow setter, style
      helpers, and a real LVGL flex update test land under `LV_USE_FLEX`;
- [x] grid enum mappings, track helpers, descriptor-array wrapper,
      container setters, cell setters, style helpers, and a real LVGL
      grid update test land under `LV_USE_GRID`;
- [x] grid descriptor storage lifetime is mechanically documented and
      protected against accidental mutation or dangling storage;
- [x] typed style helpers cover width, height, align, padding, margin,
      row gap, column gap, layout, and gated flex/grid properties;
- [x] compatibility rectangle helpers remain source-compatible;
- [x] embedded posture builds affected targets with
      `LVGLPP_EMBEDDED_POSTURE=ON`;
- [x] every raw pointer member added by this phase has an adjacent
      ownership/lifetime comment.

## 13. Files Cited

- `rlvgl/docs/concepts/LPAR-10-LAYOUT.md`
- `lvgl/src/core/lv_obj_pos.h`
- `lvgl/src/core/lv_obj_pos.c`
- `lvgl/src/layouts/lv_layout.h`
- `lvgl/src/layouts/lv_layout.c`
- `lvgl/src/layouts/flex/lv_flex.h`
- `lvgl/src/layouts/flex/lv_flex.c`
- `lvgl/src/layouts/grid/lv_grid.h`
- `lvgl/src/layouts/grid/lv_grid.c`
- `lvgl/src/misc/lv_style.h`
- `lvgl/src/misc/lv_style_gen.h`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/style_lvgl.hpp`
- `core/include/lvglpp/core/widget.hpp`
- `widgets/include/lvglpp/widgets/label.hpp`
- `widgets/include/lvglpp/widgets/image.hpp`
- `widgets/include/lvglpp/widgets/container.hpp`
- `ui/include/lvglpp/ui/draw_helpers.hpp`
- `ui/include/lvglpp/ui/event_window.hpp`
- `scripts/lvglpp_qt.py`

## 14. Unblocks

- LPAR-CPP-11 and later widget wrappers that need container-driven size
  and placement.
- LPAR-CPP-12 ImageButton and richer composed widgets.
- QT-CPP generated screen modules that need LVGL object/style layout
  calls instead of static rectangles.
- SCTD-CPP demo panels that need flex/grid screen composition before
  state-chart generated behavior is attached.
- FireBeetle 2 / ESP P4 demo UI composition once the board target phase
  owns the IDF integration.

## 15. Change Log

| Date | State | Notes |
| --- | --- | --- |
| 2026-06-30 | DRAFT | Initial LVGL layout and geometry draft. Adapts rlvgl LPAR-10 by delegating object geometry, percent/content sizing, dirty layout, flex, grid, and layout style properties to LVGL public APIs while preserving existing static `ui` helpers as compatibility-only. |
| 2026-06-30 | RATIFIED | Owner accepted the LPAR-CPP-10 phase and directed execution to proceed. |
| 2026-06-30 | IMPLEMENTED | Added LVGL-backed geometry, size, alignment, dirty/update, flex, grid, grid-track lifetime, and layout style helpers in `core/include/lvglpp/core/layout.hpp` with focused host coverage in `lvglpp_core_layout`. Embedded-posture `lvglpp_core` and `lvglpp_playit` compile gate passed. |
