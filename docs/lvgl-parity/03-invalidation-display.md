<!--
03-invalidation-display.md - lvglpp LPAR-03 mirror invalidation/display plan.
-->

# LPAR-CPP-03 - LVGL Invalidation and Display Runtime

Status: **RATIFIED** (2026-06-29). Normative for the LPAR-CPP-03
invalidation and display-wrapper implementation.

Parent initiative: [`00-concepts.md`](00-concepts.md). Baseline:
[`01-baseline.md`](01-baseline.md). Object substrate:
[`02-object-substrate.md`](02-object-substrate.md).

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-03 relationship |
| --- | --- | --- |
| LVGL invalidation and refresh | `lvgl/src/core/lv_obj.h`, `lvgl/src/display/lv_display.h`, `lvgl/src/display/lv_refr*.c` | Canonical implementation. lvglpp wraps and observes LVGL invalidation/flush behavior; it MUST NOT add a competing dirty planner for LVGL-backed widgets. |
| rlvgl invalidation/display phase | `rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by delegating dirty collection, refresh, clipping, and flush scheduling to LVGL. |
| Current lvglpp object substrate | `core/include/lvglpp/core/object.hpp`, `core/src/object.cpp` | `LvObject` / `ObjectView` are the object handles this phase invalidates and tests. |
| Current compatibility renderer/display surfaces | `core/include/lvglpp/core/renderer.hpp`, `platform/include/lvglpp/platform/screen.hpp`, host/disco examples | Compatibility-only for this phase. They remain source-compatible and are not promoted to LVGL display drivers here unless explicitly wrapped. |
| Ownership discipline | top-level `AGENTS.md` | Every display, draw buffer, flush callback userdata, and framebuffer pointer added by this phase MUST carry ownership comments. |

If this chapter changes `LvObject`, `ObjectView`, `Runtime`, existing
`Renderer`, or platform display APIs in a source-breaking way, §15 MUST
be amended first. The default strategy is additive.

## 1. Purpose

Define the LVGL-backed C++ display and invalidation surface needed by
later parity wrappers. This phase gives lvglpp code a way to:

- create and own an `lv_display_t`;
- bind LVGL draw buffers and flush callbacks with explicit lifetimes;
- invalidate an `LvObject` / `ObjectView`;
- observe invalidated or flushed logical areas in tests;
- drive LVGL refresh deterministically enough for host unit tests.

It intentionally does not port rlvgl's retained dirty planner. LVGL
already owns invalidation, clipping, redraw, and display flush cadence
for real `lv_obj_t` trees.

## 2. Problem Statement

LPAR-CPP-02 can create real LVGL objects, but tests currently need local
ad-hoc display setup before `lv_obj_create(nullptr)` works. Later widget
wrappers need a stable display owner and invalidation surface so they can
verify "this state change caused redraw/flush" without relying on the
compatibility `Renderer` tree.

rlvgl LPAR-03 introduced a shared dirty planner because rlvgl owns a
custom retained rendering runtime. lvglpp's parity route is different:
LVGL's `lv_obj_invalidate`, `LV_EVENT_INVALIDATE_AREA`, refresh timer,
display buffers, and flush callback are the runtime. Re-implementing
rlvgl's planner in C++ would create the second runtime that LPAR-CPP-00
forbids.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **`LvDisplay`** | Planned move-only RAII owner for an `lv_display_t*`. Owns deletion through `lv_display_delete` when armed. |
| **`DisplayView`** | Planned non-owning view over `lv_display_t*`. It observes an LVGL display and never deletes. |
| **Draw buffer** | Memory passed to `lv_display_set_buffers`. Ownership is external or borrowed by LVGL for flush/render use; mutation authority must be documented. |
| **Flush callback** | Function installed with `lv_display_set_flush_cb`; receives an area and color buffer from LVGL and MUST call `lv_display_flush_ready` unless a wait callback owns completion. |
| **Invalidation** | Declaration that an LVGL object or area needs redraw; implemented with LVGL `lv_obj_invalidate` / `lv_obj_invalidate_area`. |
| **Invalidated area** | Logical `lv_area_t` reported through `LV_EVENT_INVALIDATE_AREA` or observed by refresh/flush tests. |
| **Flush area** | Logical `lv_area_t` passed to the display flush callback. Platform-specific physical rotation/mapping is not owned by this core phase. |
| **Compatibility renderer** | Current `core::Renderer` / `WidgetNode` draw path. It remains a test/app surface but is not the LVGL-backed parity display runtime. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| Display create/delete | `lvgl/src/display/lv_display.h` `lv_display_create`, `lv_display_delete` |
| Display default | `lvgl/src/display/lv_display.h` `lv_display_set_default`, `lv_display_get_default` |
| Draw buffers | `lvgl/src/display/lv_display.h` `lv_display_set_buffers`, `lv_display_set_buffers_with_stride` |
| Flush callbacks | `lvgl/src/display/lv_display.h` `lv_display_set_flush_cb`, `lv_display_flush_ready` |
| Invalidate object/area | `lvgl/src/core/lv_obj.h` `lv_obj_invalidate`, `lv_obj_invalidate_area` |
| Invalidate-area event | LVGL `LV_EVENT_INVALIDATE_AREA` |
| Refresh/tick driving | `lv_timer_handler`, `lv_tick_inc`, LVGL refresh timer behavior |
| Object handles | `core/include/lvglpp/core/object.hpp` |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md` |

## 5. Frozen Decisions - LVGL Owns Dirty Planning

1. **No competing dirty planner.** LPAR-CPP-03 MUST NOT port rlvgl's
   `InvalidationList`, `PresentPlan`, or `BufferedInvalidation` as the
   runtime path for LVGL-backed widgets.
2. **Use LVGL invalidation APIs.** Object invalidation wrappers call
   `lv_obj_invalidate` or `lv_obj_invalidate_area` on real LVGL objects.
3. **Use LVGL refresh/flush callbacks.** Display observation tests record
   LVGL flush callback areas or invalidate-area events. They do not infer
   dirty regions from compatibility `Renderer` calls.
4. **Logical coordinates are LVGL coordinates.** Areas exposed by this
   core phase are LVGL logical `lv_area_t` values. Platform rotation and
   physical framebuffer mapping remain platform/display-driver concerns.
5. **Full-refresh fallback belongs to LVGL or the display backend.** If a
   backend cannot partial-refresh, it chooses full-frame behavior through
   its LVGL display configuration or flush callback, not through a second
   lvglpp dirty planner.

## 6. Frozen Decisions - Ownership Model

1. **`LvDisplay` is move-only.** Copy construction and copy assignment
   are deleted. Move transfers `lv_display_delete` responsibility and
   nulls the source.
2. **Draw buffers are not implicitly owned.** A display wrapper MAY
   accept caller-owned buffers by span/view. Ownership comments MUST mark
   them `external`, `borrows`, or `dma` as appropriate.
3. **Host test buffers may be owned by a fixture.** A host helper MAY own
   a `std::vector`/`std::array` draw buffer and pass it to LVGL, but the
   stored raw buffer pointer inside the display wrapper is still a borrow
   from the helper's storage.
4. **Flush userdata is observation by default.** Callback userdata MAY
   observe a recording sink or backend object, but MUST NOT own that
   object through a raw pointer.
5. **Completion is explicit.** A flush callback installed by lvglpp MUST
   call `lv_display_flush_ready` before returning unless the wrapper has
   installed a documented wait/completion callback.
6. **No `shared_ptr` display ownership.** `std::shared_ptr` MUST NOT own
   `lv_display_t` or draw-buffer mutation authority. If a later adapter
   uses shared observation, it MUST preserve one writer/owner and
   document lifetime proof, matching LPAR-CPP-02 §5.8.

## 7. Frozen Decisions - API Surface v1

LPAR-CPP-03 v1 SHALL introduce or reserve these concepts:

| Surface | Required shape |
| --- | --- |
| `class LvDisplay` | Owns nullable `lv_display_t* raw_`; destructor deletes when armed. |
| `class DisplayView` | Non-owning `lv_display_t*` view; never deletes. |
| `LvDisplay::make(width, height)` | Creates an owned LVGL display using `lv_display_create`. |
| `LvDisplay::borrow()` | Returns `DisplayView`; borrows for as long as the display remains alive. |
| `LvDisplay::release()` | Returns `lv_display_t*`; caller owns lifecycle explanation. |
| `LvDisplay::set_default()` | Calls `lv_display_set_default`. |
| `LvDisplay::set_buffers(...)` | Wraps `lv_display_set_buffers` with explicit buffer lifetime documentation. |
| `LvDisplay::borrow_raw()` | Explicit LVGL C interop escape hatch. |
| `LvObject::invalidate()` or free helper | Calls `lv_obj_invalidate` for a live object/view. |
| `invalidate_area(ObjectView, Area)` | Calls `lv_obj_invalidate_area` using an LVGL-compatible area wrapper. |
| host test recorder | Records invalidated/flush areas for deterministic tests; not a production display backend. |

The implementation MAY place display wrappers in `core/display.hpp` or a
similarly named core header. If new, `core.hpp` MUST re-export it.

## 8. Frozen Decisions - Area Vocabulary

1. **Area wrapper is additive.** This phase MAY introduce a small
   `Area`/`LvArea` value type that maps exactly to `lv_area_t`. It MUST
   not replace existing `core::Rect` in compatibility widgets.
2. **Conversion is explicit.** If conversions between `core::Rect` and
   `lv_area_t` are added, they MUST document inclusive/exclusive edge
   semantics. LVGL areas are inclusive; `core::Rect` width/height are
   extent values.
3. **No hidden rotation.** Core conversion helpers MUST NOT rotate
   coordinates. Rotation belongs to platform/display wrappers.
4. **Degenerate areas are caller responsibility unless LVGL rejects
   them.** Helpers SHOULD avoid passing obviously empty extents to LVGL,
   but LVGL remains authoritative for final invalidation behavior.

## 9. Frozen Decisions - Test Display and Refresh Driving

1. **Tests create a real LVGL display.** Host tests use
   `lv_display_create` through `LvDisplay`; ad-hoc local display owners
   in object tests should migrate to the shared helper.
2. **Tests use real LVGL objects.** Invalidation tests create an
   `LvObject` screen/child and call the wrapper invalidation APIs.
3. **Flush recorder owns no LVGL object.** A recorder fixture observes
   flush callback calls and records copied `lv_area_t` values.
4. **Flush callback does not retain color buffer views.** Color buffer
   data passed to flush is borrowed for callback duration only.
5. **Refresh is explicitly driven.** Tests use LVGL tick/timer handler
   functions as needed; they MUST NOT depend on wall-clock timers.
6. **No graphical assertion in this phase.** This phase asserts area and
   callback behavior, not pixel-perfect widget drawing.

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `Runtime` | Must be alive before display creation. This phase may add helper docs but does not change singleton semantics. |
| `LvObject` / `ObjectView` | Invalidated by this phase. Object ownership remains LPAR-CPP-02. |
| `Renderer` / `WidgetNode` | Compatibility-only. No new LVGL invalidation behavior targets this tree. |
| `platform::Screen` | Remains the logical display descriptor for current examples. `LvDisplay` may consume width/height but does not absorb rotation policy. |
| `playit` framebuffer dumps | Still use existing compatibility readers until LVGL-backed screen examples exist. Bridging dumps to LVGL display buffers is later conformance work. |
| board display drivers | Existing STM32H747I-DISCO and Linux fbdev examples are not rewritten in this phase. Future platform chapters may bind them to `lv_display_t`. |

## 11. Non-Goals

- No rlvgl-style bounded dirty planner port.
- No compatibility `Renderer` rewrite.
- No widget wrapper implementation.
- No input device or focus wrapper; LPAR-CPP-04 owns those.
- No scroll runtime; LPAR-CPP-05 owns scroll-specific invalidation.
- No timer/animation wrapper; LPAR-CPP-06 owns `lv_timer_t` /
  `lv_anim_t`.
- No board-specific display driver migration.
- No pixel-perfect rendering assertions.

## 12. Acceptance Checklist

LPAR-CPP-03 implementation is complete only when:

- [x] `LvDisplay` or equivalent move-only owner exists with explicit
      ownership comments.
- [x] `DisplayView` or equivalent non-owning view exists and cannot
      delete.
- [x] Display creation/deletion/default selection are tested with a real
      LVGL display.
- [x] Draw-buffer binding documents buffer ownership and mutation
      authority.
- [x] Flush callback wrapper calls `lv_display_flush_ready` or documents
      an alternate completion path.
- [x] Object invalidation wrappers call `lv_obj_invalidate` /
      `lv_obj_invalidate_area`.
- [x] Host tests record at least one expected invalidate or flush area
      from a real LVGL object.
- [x] Area conversion helpers, if added, document LVGL inclusive bounds
      versus `core::Rect` extent bounds.
- [x] Existing compatibility `WidgetNode` / `Renderer` / app tests still
      compile and pass.
- [x] Embedded posture compile check passes for the new public headers.

## 13. Files Cited

- `lvgl/src/core/lv_obj.h`
- `lvgl/src/display/lv_display.h`
- `lvgl/src/display/lv_display.c`
- `lvgl/src/display/lv_refr*.c`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/runtime.hpp`
- `core/include/lvglpp/core/renderer.hpp`
- `platform/include/lvglpp/platform/screen.hpp`
- `rlvgl/docs/concepts/LPAR-03-INVALIDATION-DISPLAY.md`
- `docs/lvgl-parity/00-concepts.md`
- `docs/lvgl-parity/01-baseline.md`
- `docs/lvgl-parity/02-object-substrate.md`

## 14. Unblocks

- LPAR-CPP-04 event/focus/input wrappers with visual-state
  invalidation.
- LPAR-CPP-05 scroll runtime tests over real LVGL scroll objects.
- LPAR-CPP-06 timer/animation tests that invalidate objects.
- LPAR-CPP-08 text/image draw tests using LVGL display buffers.
- LVGL-backed widget wrapper smoke tests.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial invalidation/display draft. Adapts rlvgl LPAR-03 by delegating dirty planning and refresh to LVGL, adding planned move-only display ownership, explicit draw-buffer/flush lifetimes, and host tests that record LVGL invalidate/flush areas. |
| 2026-06-29 | RATIFIED | Ratified by owner instruction. Implementation unblocked under the LVGL-underneath rule; no rlvgl-style dirty planner is permitted for LVGL-backed widgets. |
| 2026-06-29 | IMPLEMENTED | Added `LvDisplay`, `DisplayView`, `LvArea`, draw-buffer/flush wrappers, invalidation helpers, and `lvglpp_core_display` tests over real LVGL invalidation and flush callbacks. Default host tests and embedded-posture `lvglpp_core` compile pass. |
