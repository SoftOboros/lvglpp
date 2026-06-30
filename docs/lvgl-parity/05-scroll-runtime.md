<!--
05-scroll-runtime.md - lvglpp LPAR-05 mirror scroll runtime plan.
-->

# LPAR-CPP-05 - LVGL Scroll Runtime

Status: **RATIFIED** (2026-06-29). Normative for the LPAR-CPP-05
LVGL-backed scroll wrapper implementation.

Parent initiative: [`00-concepts.md`](00-concepts.md). Baseline:
[`01-baseline.md`](01-baseline.md). Object substrate:
[`02-object-substrate.md`](02-object-substrate.md). Display and
invalidation: [`03-invalidation-display.md`](03-invalidation-display.md).
Event/focus/input: [`04-event-focus-input.md`](04-event-focus-input.md).

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-05 relationship |
| --- | --- | --- |
| LVGL scroll runtime | `lvgl/src/core/lv_obj_scroll.h`, `lvgl/src/core/lv_obj_scroll.c`, `lvgl/src/core/lv_obj.h`, `lvgl/src/indev/lv_indev_scroll*` | Canonical implementation. lvglpp wraps LVGL scroll flags, directions, offsets, scrollbar modes, snap modes, scroll APIs, scroll animations, chaining, and scroll events; it MUST NOT add a competing C++ scroll controller for LVGL-backed objects. |
| LVGL scroll events | `lvgl/src/misc/lv_event.h`, `lvgl/src/core/lv_obj_event.h` | Canonical event codes and payload/accessor behavior. LPAR-CPP-05 extends `EventCode` mapping for scroll codes. |
| rlvgl scroll phase | `rlvgl/docs/concepts/LPAR-05-SCROLL-RUNTIME.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by delegating scroll offset management, throw/momentum, snap, and chaining to LVGL. |
| Current lvglpp object/display/input handles | `core/include/lvglpp/core/object.hpp`, `core/include/lvglpp/core/display.hpp`, `core/include/lvglpp/core/input.hpp` | `LvObject`, `ObjectView`, `LvDisplay`, `EventView`, and input-device wrappers are the handles this phase scrolls and observes. |
| Existing compatibility widgets | `widgets/include/lvglpp/widgets/list.hpp`, current `core::WidgetNode` / `Renderer` path | Compatibility-only for this phase. They are not migrated to LVGL scroll semantics here. |
| Ownership discipline | top-level `AGENTS.md` | Every raw LVGL handle, event callback, scrollbar-area pointer, and animation/read callback touched by this phase MUST carry explicit ownership comments. |

If this chapter changes `ObjectFlag`, `EventCode`, `ObjectView`,
`LvObject`, `EventView`, playit wire grammar, or display/input lifetimes
in a source-breaking way, §15 MUST be amended first. The default
strategy is additive.

## 1. Purpose

Define the LVGL-backed C++ scroll surface needed by later wrapper
widgets. This phase gives lvglpp code a way to:

- expose LVGL scroll-related object flags through `ObjectFlag`;
- configure scroll direction, scrollbar mode, and snap alignment;
- read current scroll offsets and remaining scroll extents;
- scroll by deltas, scroll to positions, and scroll children into view;
- observe scroll begin/scroll/end/throw events through `EventView`;
- test scroll behavior using real LVGL objects and input devices.

This phase intentionally does not port rlvgl's Rust `ScrollController`,
velocity window, tick-only throw planner, or `ScrollState` object slot.
LVGL already owns scroll state and kinetic behavior for real `lv_obj_t`
trees.

## 2. Problem Statement

LPAR-CPP-02 exposes `ObjectFlag::Scrollable` but not the rest of LVGL's
scroll flags. LPAR-CPP-04 exposes generic LVGL event wrappers but does
not name scroll event codes. Later list, dropdown, roller, tabview,
tileview, menu, window, textarea, and table wrappers need a stable C++
surface for scroll configuration and observation.

rlvgl LPAR-05 defines a custom scroll runtime because rlvgl owns a
custom retained object tree and renderer. lvglpp's route is different:
scroll offsets are `lv_obj_get_scroll_x/y`, movement is
`lv_obj_scroll_by` / `lv_obj_scroll_to`, snap is
`lv_obj_set_scroll_snap_x/y`, scrollbars are LVGL-drawn, and
input-driven throw/chaining are handled by LVGL's indev scroll logic.

The missing work is not a planner. The missing work is a typed C++
wrapper surface with ownership/lifetime discipline and tests that prove
lvglpp can configure and observe LVGL scroll behavior.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **Scroll container** | An LVGL object with `LV_OBJ_FLAG_SCROLLABLE`; exposed in lvglpp through `ObjectFlag::Scrollable`. |
| **Scroll offset** | Current LVGL logical scroll position returned by `lv_obj_get_scroll_x` / `lv_obj_get_scroll_y`. |
| **Scroll remaining extent** | Pixels still scrollable toward each edge: top, bottom, left, right from LVGL's `lv_obj_get_scroll_*` APIs. |
| **Scroll direction** | Allowed LVGL scroll direction bitset (`lv_dir_t`) configured with `lv_obj_set_scroll_dir`. |
| **Scrollbar mode** | LVGL `lv_scrollbar_mode_t`: Off, On, Active, Auto. |
| **Scroll snap** | LVGL `lv_scroll_snap_t`: None, Start, End, Center. Snap points are LVGL child/snappable-object behavior. |
| **Scroll event** | LVGL event code `LV_EVENT_SCROLL_BEGIN`, `LV_EVENT_SCROLL`, `LV_EVENT_SCROLL_END`, or `LV_EVENT_SCROLL_THROW_BEGIN` observed through `EventView`. |
| **Scrollbar area** | Horizontal and vertical `lv_area_t` values returned by `lv_obj_get_scrollbar_area`; exposed as `LvArea` values. |
| **Compatibility scroll** | Any current non-LVGL widget-local scroll or list behavior. It remains outside LVGL-backed parity claims in this phase. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| Scroll object flags | `lvgl/src/core/lv_obj.h` `LV_OBJ_FLAG_SCROLLABLE`, `SCROLL_ELASTIC`, `SCROLL_MOMENTUM`, `SCROLL_ONE`, `SCROLL_CHAIN_*`, `SCROLL_ON_FOCUS`, `SCROLL_WITH_ARROW`, `SNAPPABLE`, `FLOATING` |
| Scrollbar mode | `lvgl/src/core/lv_obj_scroll.h` `lv_scrollbar_mode_t` |
| Scroll snap | `lvgl/src/core/lv_obj_scroll.h` `lv_scroll_snap_t` |
| Scroll directions | LVGL `lv_dir_t` |
| Scroll offsets/extents | `lv_obj_get_scroll_x/y/top/bottom/left/right`, `lv_obj_get_scroll_end` |
| Scroll movement | `lv_obj_scroll_by`, `lv_obj_scroll_by_bounded`, `lv_obj_scroll_to*`, `lv_obj_scroll_to_view*` |
| Scroll lifecycle | `lv_obj_is_scrolling`, `lv_obj_stop_scroll_anim`, `lv_obj_update_snap`, `lv_obj_readjust_scroll` |
| Scrollbar geometry/invalidation | `lv_obj_get_scrollbar_area`, `lv_obj_scrollbar_invalidate` |
| Event observation | `core/include/lvglpp/core/input.hpp` |
| Area wrapper | `core/include/lvglpp/core/display.hpp` `LvArea` |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-05-SCROLL-RUNTIME.md` |

## 5. Frozen Decisions - LVGL Owns Scroll Runtime

1. **No competing scroll controller.** LPAR-CPP-05 MUST NOT port
   rlvgl's `ScrollController`, `ScrollState`, velocity estimator, snap
   planner, or tick-driven throw tween as the runtime path for
   LVGL-backed objects.
2. **Use LVGL scroll state.** Current offset and remaining scroll extent
   are read from LVGL. lvglpp does not mirror them in C++ storage.
3. **Use LVGL movement APIs.** Programmatic movement uses LVGL
   `lv_obj_scroll_*` functions. Bound/clamp behavior remains LVGL's.
4. **Use LVGL indev scroll behavior.** Pointer drag, throw/momentum,
   chaining, click suppression during scroll, and scroll-on-focus/key
   behavior remain owned by LVGL input processing.
5. **Use LVGL scroll events.** Scroll begin, scroll, scroll end, and
   throw-begin are observed through LVGL event callbacks. lvglpp does
   not synthesize these events for LVGL-backed objects except in tests
   that explicitly call LVGL send APIs.
6. **Use LVGL invalidation.** Scroll offset changes and scrollbar
   invalidation are LVGL dirty sources. lvglpp may expose
   `scrollbar_invalidate`, but it does not push a second dirty planner.

## 6. Frozen Decisions - Flags, Directions, and Modes

LPAR-CPP-05 SHALL extend the public C++ scroll vocabulary as follows:

| Surface | LVGL analogue |
| --- | --- |
| `ObjectFlag::ScrollElastic` | `LV_OBJ_FLAG_SCROLL_ELASTIC` |
| `ObjectFlag::ScrollMomentum` | `LV_OBJ_FLAG_SCROLL_MOMENTUM` |
| `ObjectFlag::ScrollOne` | `LV_OBJ_FLAG_SCROLL_ONE` |
| `ObjectFlag::ScrollChainHorizontal` | `LV_OBJ_FLAG_SCROLL_CHAIN_HOR` |
| `ObjectFlag::ScrollChainVertical` | `LV_OBJ_FLAG_SCROLL_CHAIN_VER` |
| `ObjectFlag::ScrollChain` | `LV_OBJ_FLAG_SCROLL_CHAIN` |
| `ObjectFlag::ScrollOnFocus` | `LV_OBJ_FLAG_SCROLL_ON_FOCUS` |
| `ObjectFlag::ScrollWithArrow` | `LV_OBJ_FLAG_SCROLL_WITH_ARROW` |
| `ObjectFlag::Snappable` | `LV_OBJ_FLAG_SNAPPABLE` |
| `ObjectFlag::Floating` | `LV_OBJ_FLAG_FLOATING` |
| `ScrollDirection` | LVGL `lv_dir_t`, including none, horizontal, vertical, all, and directional bits |
| `ScrollbarMode` | LVGL `lv_scrollbar_mode_t` Off, On, Active, Auto |
| `ScrollSnap` | LVGL `lv_scroll_snap_t` None, Start, End, Center |

Growth policy:

1. Scroll-related `ObjectFlag` entries are **Standards Action** because
   they mirror LVGL bit positions and cross-language scroll vocabulary.
2. `ScrollbarMode`, `ScrollSnap`, and `ScrollDirection` are
   **Standards Action** for the same reason.
3. `ScrollbarMode::Auto` follows LVGL semantics: visible when content is
   large enough to scroll. `ScrollbarMode::Active` follows LVGL's
   "visible while scrolling" behavior. This differs from rlvgl's
   custom overlay vocabulary and is an intentional LVGL-underneath
   adaptation.

## 7. Frozen Decisions - Event Vocabulary

LPAR-CPP-05 SHALL extend `EventCode` with this scroll set:

| `EventCode` | LVGL analogue | Notes |
| --- | --- | --- |
| `ScrollBegin` | `LV_EVENT_SCROLL_BEGIN` | `EventView::param()` may be an LVGL animation pointer per LVGL. |
| `Scroll` | `LV_EVENT_SCROLL` | Offset should be read from the event target with scroll accessors. |
| `ScrollEnd` | `LV_EVENT_SCROLL_END` | Last scroll event for a completed LVGL scroll session. |
| `ScrollThrowBegin` | `LV_EVENT_SCROLL_THROW_BEGIN` | Mirrors rlvgl `ScrollThrow` vocabulary but uses LVGL's event spelling. |

Growth policy:

1. These codes are added under the LPAR-CPP-04 `EventCode` Standards
   Action policy.
2. Scroll events target the LVGL object that owns the scroll offset.
   Bubbling follows LVGL object flags; lvglpp does not mutate event
   bubbling flags to force scroll propagation.
3. Scroll event payloads are not re-shaped into C++ structs in this
   phase. Access to raw LVGL `param()` stays through `EventView` until
   a later widget phase needs typed animation mutation.

## 8. Frozen Decisions - API Surface v1

LPAR-CPP-05 v1 SHALL introduce or reserve these concepts:

| Surface | Required shape |
| --- | --- |
| `ScrollOffset` | Value type with `x`, `y`; maps to LVGL logical scroll position. |
| `ScrollExtents` | Value type with `top`, `bottom`, `left`, `right`; read-only snapshot from LVGL. |
| `ScrollbarAreas` | Value type with horizontal and vertical `LvArea`; read from LVGL. |
| `set_scrollbar_mode(ObjectView, ScrollbarMode)` | Calls `lv_obj_set_scrollbar_mode`. |
| `scrollbar_mode(ObjectView)` | Calls `lv_obj_get_scrollbar_mode`. |
| `set_scroll_direction(ObjectView, ScrollDirection)` | Calls `lv_obj_set_scroll_dir`. |
| `scroll_direction(ObjectView)` | Calls `lv_obj_get_scroll_dir`. |
| `set_scroll_snap_x/y(ObjectView, ScrollSnap)` | Calls `lv_obj_set_scroll_snap_x/y`. |
| `scroll_snap_x/y(ObjectView)` | Calls `lv_obj_get_scroll_snap_x/y`. |
| `scroll_offset(ObjectView)` | Calls `lv_obj_get_scroll_x/y`. |
| `scroll_extents(ObjectView)` | Calls `lv_obj_get_scroll_top/bottom/left/right`. |
| `scroll_end(ObjectView)` | Calls `lv_obj_get_scroll_end`. |
| `scroll_by`, `scroll_by_bounded` | Call LVGL delta-scroll APIs with explicit animation mode. |
| `scroll_to`, `scroll_to_x`, `scroll_to_y` | Call LVGL absolute scroll APIs with explicit animation mode. |
| `scroll_to_view`, `scroll_to_view_recursive` | Call LVGL visibility helpers. |
| `is_scrolling`, `stop_scroll_anim` | Call LVGL scroll lifecycle helpers. |
| `update_snap`, `readjust_scroll` | Call LVGL snap/readjust helpers. |
| `scrollbar_areas`, `scrollbar_invalidate` | Call LVGL scrollbar geometry/invalidation helpers. |

The implementation MAY place these in `core/scroll.hpp` or another
core header with the same cite block shape. If new, `core.hpp` MUST
re-export it.

## 9. Frozen Decisions - Animation and Determinism

1. **Animation mode is explicit.** Programmatic scroll helpers MUST take
   an explicit `AnimationMode`/`AnimEnable` value or equivalent wrapper
   around `lv_anim_enable_t`; defaulting to hidden animation behavior is
   not allowed in the C++ wrapper.
2. **No C++ wall-clock model.** lvglpp does not add wall-clock timing for
   scroll. Animated scrolls are driven by LVGL's timer/animation system.
   Tests drive LVGL ticks/timers explicitly.
3. **No C++ velocity model.** rlvgl's px/tick velocity rules are
   informative only for comparing user-visible behavior. LVGL's indev
   scroll implementation owns actual throw/momentum.
4. **Deterministic tests use synthetic LVGL input.** Host tests may use
   `LvInputDevice` and `LvglInputBridge` to feed deterministic pointer
   events into LVGL. They MUST NOT depend on wall-clock scheduling.

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `LvObject` / `ObjectView` | Scroll helpers operate on these handles and never own objects. |
| `ObjectFlag::Scrollable` | Finalized as the LVGL scrollable flag. Additional scroll flags are additive LVGL mirrors. |
| `EventView` / `EventCode` | LPAR-CPP-05 adds named scroll event codes. Existing callback lifetime rules remain unchanged. |
| `LvInputDevice` / `LvglInputBridge` | Used by tests and later apps to drive LVGL scroll behavior through actual indev processing. |
| `LvDisplay` / invalidation helpers | LVGL owns scroll invalidation. Tests may observe invalidated or flushed areas, but no second dirty planner is introduced. |
| `core::Event` | Compatibility/playit stream only. No scroll object-event semantics are added to `core::Event`. |
| `WidgetNode` / `Renderer` | Compatibility-only. No LVGL scroll behavior is added to this retained tree. |
| existing `widgets::List` | Remains non-scroll in this phase. LPAR-CPP-13 owns List reconciliation with LVGL scroll wrappers. |

## 11. Non-Goals

- No rlvgl `ScrollController` port.
- No custom velocity estimator, throw tween, or snap planner.
- No compatibility `WidgetNode` scroll rewrite.
- No migration of current hand-drawn widgets to LVGL scroll.
- No new playit wire grammar.
- No board-specific touch driver migration.
- No interactive scrollbar thumb API.
- No typed wrapper for mutating LVGL scroll-begin animation payloads in
  v1; raw access remains through `EventView::param()`.

## 12. Acceptance Checklist

LPAR-CPP-05 implementation is complete only when:

- [x] Scroll-related `ObjectFlag` entries listed in §6 exist and map to
      LVGL constants.
- [x] `ScrollDirection`, `ScrollbarMode`, and `ScrollSnap` wrappers map
      to LVGL values and preserve LVGL semantics.
- [x] `EventCode` names `ScrollBegin`, `Scroll`, `ScrollEnd`, and
      `ScrollThrowBegin`; unknown scroll-related codes still preserve raw
      values.
- [x] Scroll offset, extent, end-position, movement, snap, scrollbar,
      and lifecycle helpers call the corresponding LVGL APIs.
- [x] Animation enablement for programmatic scroll is explicit at call
      sites.
- [x] Host tests create real LVGL scroll containers with overflowing
      children and verify scroll offset/extents before and after
      `scroll_by` / `scroll_to`.
- [x] Host tests verify scrollbar mode, snap mode, and scrollbar area
      wrappers over real LVGL objects.
- [x] Host tests verify scroll event callbacks through `EventView` for
      at least `ScrollBegin`, `Scroll`, and `ScrollEnd` where LVGL emits
      them under the chosen test path.
- [x] Synthetic input tests use `LvInputDevice` or `LvglInputBridge`
      to drive at least one LVGL scroll path without changing playit
      wire grammar.
- [x] Existing compatibility `core::Event`, playit conversion, object,
      display, input, widget, and app tests still compile and pass.
- [x] Embedded posture compile check passes for the new public headers.

## 13. Files Cited

- `lvgl/src/core/lv_obj.h`
- `lvgl/src/core/lv_obj_scroll.h`
- `lvgl/src/core/lv_obj_scroll.c`
- `lvgl/src/misc/lv_event.h`
- `lvgl/src/core/lv_obj_event.h`
- `lvgl/src/indev/lv_indev_scroll*`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/display.hpp`
- `core/include/lvglpp/core/input.hpp`
- `core/include/lvglpp/core/scroll.hpp`
- `playit/include/lvglpp/playit/lvgl_input_bridge.hpp`
- `rlvgl/docs/concepts/LPAR-05-SCROLL-RUNTIME.md`
- `docs/lvgl-parity/00-concepts.md`
- `docs/lvgl-parity/02-object-substrate.md`
- `docs/lvgl-parity/03-invalidation-display.md`
- `docs/lvgl-parity/04-event-focus-input.md`

## 14. Unblocks

- LPAR-CPP-06 timer/animation wrappers that need deterministic LVGL
  animated scroll checks.
- LPAR-CPP-13 selection/navigation wrappers: Dropdown, Menu, Roller,
  Tabview, Tileview, Window, and List reconciliation.
- LPAR-CPP-14 data/rich widgets that need text/table scroll surfaces.
- LVGL-backed playit scroll smoke tests for host and board demos.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial scroll-runtime draft. Adapts rlvgl LPAR-05 by delegating scroll state, scrollbar behavior, snap, chaining, and throw/momentum to LVGL while exposing typed C++ wrappers and tests around LVGL scroll APIs. |
| 2026-06-29 | RATIFIED | Ratified by owner instruction. Implementation unblocked under the LVGL-underneath rule; no rlvgl-style custom scroll controller is permitted for LVGL-backed objects. |
| 2026-06-29 | IMPLEMENTED | Added scroll-related `ObjectFlag` entries, scroll `EventCode` mappings, `AnimationMode`, `ScrollDirection`, `ScrollbarMode`, `ScrollSnap`, scroll offset/extents/areas value types, and LVGL-backed scroll helpers in `core/scroll.hpp`. Test target `lvglpp_core_scroll` validates real LVGL scroll containers, programmatic scroll events, scrollbar/snap wrappers, scroll-to-view, and synthetic pointer-driven scroll. Full default tests and embedded-posture `lvglpp_core`/`lvglpp_playit` compile pass. |
