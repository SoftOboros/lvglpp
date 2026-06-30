<!--
04-event-focus-input.md - lvglpp LPAR-04 mirror event/focus/input plan.
-->

# LPAR-CPP-04 - LVGL Event, Focus, and Input Runtime

Status: **RATIFIED** (2026-06-29). Normative for the LPAR-CPP-04
event, focus-group, input-device, and playit injection implementation.

Parent initiative: [`00-concepts.md`](00-concepts.md). Baseline:
[`01-baseline.md`](01-baseline.md). Object substrate:
[`02-object-substrate.md`](02-object-substrate.md). Display and
invalidation: [`03-invalidation-display.md`](03-invalidation-display.md).

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-04 relationship |
| --- | --- | --- |
| LVGL object event runtime | `lvgl/src/misc/lv_event.h`, `lvgl/src/core/lv_obj_event.h`, `lvgl/src/core/lv_obj.h` | Canonical implementation. lvglpp wraps `lv_event_t`, event descriptors, event codes, send semantics, and LVGL bubbling/trickling flags; it MUST NOT add a competing object-event dispatcher for LVGL-backed objects. |
| LVGL focus groups | `lvgl/src/core/lv_group.h` | Canonical implementation for focus membership, next/previous traversal, wrap, editing mode, and focused-object storage. |
| LVGL input devices | `lvgl/src/indev/lv_indev.h` | Canonical implementation for pointer, keypad, encoder, button devices, read callbacks, input-device userdata, and group binding. |
| rlvgl event/focus/input phase | `rlvgl/docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by delegating routing, focus, and device processing to LVGL underneath. |
| Current lvglpp object and display handles | `core/include/lvglpp/core/object.hpp`, `core/include/lvglpp/core/display.hpp` | `LvObject`, `ObjectView`, `LvDisplay`, and `DisplayView` are the handles this phase observes, targets, and tests. |
| Current compatibility event stream | `core/include/lvglpp/core/event.hpp`, `playit/include/lvglpp/playit/conversion.hpp` | Compatibility stream only. It remains the playit/raw-device value surface and does not become LVGL's object-event vocabulary. |
| Ownership discipline | top-level `AGENTS.md` | Every callback userdata pointer, event descriptor, focus group, input device, read callback, and raw LVGL handle added by this phase MUST carry ownership comments. |

If this chapter changes the compatibility `core::Event` variant set,
playit wire grammar, object ownership semantics, or display lifetime
rules, §15 MUST be amended first. The default strategy is additive.

## 1. Purpose

Define the LVGL-backed C++ event, focus, and input-device surface needed
by parity widget wrappers. This phase gives lvglpp code a way to:

- observe LVGL `lv_event_t` values through typed, non-owning views;
- register and remove object event callbacks with documented userdata
  lifetime;
- create and own LVGL focus groups;
- create and own LVGL input devices with explicit read-callback
  lifetimes;
- bind keypad/encoder devices to focus groups;
- bridge existing playit pointer/key commands into an LVGL-backed
  synthetic input path for tests.

This phase intentionally does not port rlvgl's custom `ObjectNode`
propagation runtime. LVGL already owns object-event routing for
`lv_obj_t` trees.

## 2. Problem Statement

LPAR-CPP-02 can create real LVGL objects and LPAR-CPP-03 can create a
real LVGL display, but lvglpp still lacks typed wrappers for LVGL object
events, focus groups, and input devices. Later button, slider, text,
image, and scroll wrappers need to receive press/click/focus/key/encoder
events without each wrapper directly exposing raw `lv_event_t*`,
`lv_group_t*`, and `lv_indev_t*`.

rlvgl LPAR-04 defines a Rust-side event runtime because rlvgl owns a
custom retained object tree. lvglpp's route is different: object events
come from LVGL's `lv_obj_add_event_cb`, `lv_event_get_code`,
`lv_obj_send_event`, focus groups come from `lv_group_t`, and input
polling comes from `lv_indev_t`. Re-implementing rlvgl's propagation
planner in C++ would create the second runtime that LPAR-CPP-00 forbids.

The existing lvglpp `core::Event` and playit conversions remain useful:
they are the raw command/compatibility stream. They are not sufficient
for LVGL object callbacks because LVGL supplies target/current-target,
event descriptors, callback userdata, group focus, input-device state,
and lifecycle events through its own C API.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **`EventView`** | Non-owning view over `lv_event_t*`. Borrows the event for callback duration only; never stores beyond the callback. |
| **`EventCode`** | C++ classifier for the LVGL event codes exposed by LPAR-CPP-04 v1. Mirrors LVGL `lv_event_code_t` values, not `core::Event` variants. |
| **Event descriptor** | LVGL-owned `lv_event_dsc_t*` returned by `lv_obj_add_event_cb`; removable with `lv_obj_remove_event_dsc`. |
| **`EventSubscription`** | Move-only RAII registration token. Observes the LVGL object and descriptor, removes the descriptor when armed, and never owns the callback target through a raw pointer. |
| **Event callback userdata** | User pointer passed through LVGL. Ownership is external/observed unless a wrapper stores a typed RAII object elsewhere and documents that lifetime. |
| **Target** | LVGL event target returned by `lv_event_get_target`. As defined by LVGL; lvglpp exposes it as `ObjectView` when non-null. |
| **Current target** | LVGL current target returned by `lv_event_get_current_target`; may differ from target during bubbling/trickling. |
| **`LvGroup`** | Move-only RAII owner for an `lv_group_t*`; owns deletion through `lv_group_delete` when armed. |
| **`GroupView`** | Non-owning view over `lv_group_t*`; observes an LVGL focus group and never deletes. |
| **`LvInputDevice`** | Move-only RAII owner for an `lv_indev_t*`; owns deletion through `lv_indev_delete` when armed. |
| **`InputDeviceView`** | Non-owning view over `lv_indev_t*`; observes an LVGL input device and never deletes. |
| **Read callback** | Function installed with `lv_indev_set_read_cb`; borrows callback state through LVGL userdata and fills `lv_indev_data_t` for the current poll. |
| **Compatibility event stream** | Existing `core::Event` / playit value stream. It is used to feed synthetic input but is not the object-event dispatch model. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| Event codes and event accessors | `lvgl/src/misc/lv_event.h` |
| Object event registration/removal | `lvgl/src/core/lv_obj_event.h` |
| Event bubbling/trickling flags | `lvgl/src/core/lv_obj.h` |
| Focus group create/delete/traversal | `lvgl/src/core/lv_group.h` |
| Input device create/delete/types/read callbacks | `lvgl/src/indev/lv_indev.h` |
| Object handles | `core/include/lvglpp/core/object.hpp` |
| Display/runtime preconditions | `core/include/lvglpp/core/display.hpp`, `core/include/lvglpp/core/runtime.hpp` |
| Compatibility event values | `core/include/lvglpp/core/event.hpp` |
| playit command conversion | `playit/include/lvglpp/playit/conversion.hpp` |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md` |

## 5. Frozen Decisions - LVGL Owns Event Routing

1. **No competing dispatcher.** LPAR-CPP-04 MUST NOT port rlvgl's
   `ObjectEvent` propagation runtime, target planner, or tree-walk
   dispatcher for LVGL-backed objects.
2. **Use LVGL callbacks.** Object event handlers are registered through
   `lv_obj_add_event_cb` or an equivalent LVGL API and observe
   `lv_event_t` during callback execution.
3. **Use LVGL propagation controls.** Bubbling and trickling behavior is
   controlled by LVGL object flags and LVGL send semantics. lvglpp may
   expose helpers for those flags, but it does not invent a parallel
   propagation flag set.
4. **Target/current-target come from LVGL.** `EventView::target()` and
   `EventView::current_target()` expose LVGL's target handles as
   `ObjectView` values when present.
5. **Stopping propagation is LVGL-backed.** Any C++ helper that stops
   bubbling, trickling, or processing MUST call the corresponding LVGL
   event API; it MUST NOT only mark C++ wrapper state.
6. **Lifecycle events are LVGL lifecycle events.** Delete, child
   changed, create, ready, cancel, and value-changed meanings follow
   LVGL. rlvgl's detach-specific lifecycle vocabulary is informative
   only for lvglpp unless LVGL emits the corresponding event.

## 6. Frozen Decisions - Event Vocabulary and Callback Ownership

LPAR-CPP-04 v1 SHALL expose a typed mapping for this initial LVGL event
set:

| `EventCode` | LVGL analogue |
| --- | --- |
| `Pressed` | `LV_EVENT_PRESSED` |
| `Pressing` | `LV_EVENT_PRESSING` |
| `Released` | `LV_EVENT_RELEASED` |
| `Clicked` | `LV_EVENT_CLICKED` |
| `DoubleClicked` | `LV_EVENT_DOUBLE_CLICKED` |
| `LongPressed` | `LV_EVENT_LONG_PRESSED` |
| `LongPressedRepeat` | `LV_EVENT_LONG_PRESSED_REPEAT` |
| `Focused` | `LV_EVENT_FOCUSED` |
| `Defocused` | `LV_EVENT_DEFOCUSED` |
| `Key` | `LV_EVENT_KEY` |
| `Rotary` | `LV_EVENT_ROTARY` |
| `Gesture` | `LV_EVENT_GESTURE` |
| `ChildChanged` | `LV_EVENT_CHILD_CHANGED` |
| `Delete` | `LV_EVENT_DELETE` |
| `ValueChanged` | `LV_EVENT_VALUE_CHANGED` |
| `Ready` | `LV_EVENT_READY` |
| `Cancel` | `LV_EVENT_CANCEL` |
| `InvalidateArea` | `LV_EVENT_INVALIDATE_AREA` |
| `Other` | Any LVGL event code not yet named by lvglpp. |

Growth policy:

1. `EventCode` is **Standards Action** because every exposed value must
   continue to map to the LVGL / rlvgl parity vocabulary.
2. New LVGL codes MAY be observable as `Other(raw)` before they receive
   a named C++ spelling.
3. `core::Event` MUST NOT gain object-semantic LVGL event codes in this
   phase. It may only remain the compatibility/playit stream.
4. If a future phase adds widget-owned codes such as draw-part or scroll
   events, that phase owns the amendment and cites this table.

Event callback ownership:

1. **`EventView` is callback-duration only.** It borrows `lv_event_t*`
   for the current LVGL callback and MUST NOT be stored.
2. **`EventSubscription` is move-only.** Copy construction and copy
   assignment are deleted. Move transfers removal responsibility and
   nulls the source.
3. **LVGL owns event descriptors.** The subscription token observes
   `lv_event_dsc_t*`; it does not delete the descriptor directly. It
   removes the descriptor through LVGL while the observed object is live.
4. **Object lifetime must dominate subscription removal.** A
   subscription that observes an object MUST be destroyed or disarmed
   before that object is deleted, unless LVGL's delete path has already
   removed the descriptor and the wrapper has documented that condition.
5. **Callback userdata is not raw ownership.** Raw userdata pointers are
   `observes` or `external`; owning callback state must live in a RAII
   object whose lifetime is documented adjacent to registration.
6. **No `shared_ptr` event ownership shortcut.** `std::shared_ptr` MUST
   NOT own LVGL event descriptors, event userdata mutation authority, or
   `lv_event_t`. If shared observation is introduced later, it MUST
   preserve a single owner/writer and document the lifetime proof.
7. **Embedded posture applies.** Callback thunks MUST be noexcept or
   catch/abort-compatible under `LVGLPP_EMBEDDED_POSTURE`.

## 7. Frozen Decisions - Focus Groups

1. **`LvGroup` owns `lv_group_t`.** The wrapper is move-only and deletes
   the group through `lv_group_delete` when armed.
2. **Membership is LVGL membership.** Adding/removing objects uses
   `lv_group_add_obj`, `lv_group_remove_obj`, or their LVGL equivalents.
   lvglpp does not mirror group membership in a separate C++ container.
3. **Focus location is LVGL focus location.** `focused()` observes
   `lv_group_get_focused` and returns `ObjectView` when present.
4. **Traversal delegates to LVGL.** `focus_next()`, `focus_prev()`,
   `focus_object(ObjectView)`, wrap configuration, and editing mode
   call LVGL group APIs.
5. **Group binding is explicit.** Keypad/encoder input devices are bound
   to a focus group with a named function that makes the borrow visible.
6. **No hidden object ownership.** Adding an object to a group does not
   transfer object ownership; the group observes LVGL objects owned by
   their normal object tree.

## 8. Frozen Decisions - Input Devices

1. **`LvInputDevice` owns `lv_indev_t`.** The wrapper is move-only and
   deletes the device through `lv_indev_delete` when armed.
2. **Device type is LVGL type.** Pointer, keypad, encoder, and button
   devices map to LVGL `lv_indev_type_t` values.
3. **Read callbacks are externally synchronized.** `lv_indev_set_read_cb`
   installs a callback that borrows driver or test state through LVGL
   userdata. The owner of that state must outlive the input device or
   explicitly detach before destruction.
4. **Read data is per-poll.** The callback fills `lv_indev_data_t` for
   the current poll and MUST NOT store pointers into that stack data.
5. **Keypad/encoder focus is LVGL focus.** Keypad and encoder devices
   route through `lv_indev_set_group`; lvglpp does not route key focus
   through the compatibility `WidgetNode` tree.
6. **Pointer coordinates are LVGL logical coordinates.** Synthetic
   pointer input uses the same logical coordinate system as
   `LvDisplay`/`ObjectView` tests. Platform rotation remains a platform
   concern.
7. **Long-press/repeat timing belongs to LVGL unless test seams require
   synthesis.** lvglpp does not hand-port rlvgl recognizers while LVGL's
   input device processing can produce the equivalent object events.

## 9. Frozen Decisions - playit Bridge

1. **No playit wire change.** LPAR-CPP-04 consumes existing playit
   pointer/key commands; it does not add command grammar.
2. **Compatibility conversion remains value-based.** Existing
   `playit::to_event` continues to produce `core::Event` values for
   compatibility tests and legacy examples.
3. **LVGL injection is a separate bridge.** This phase adds or reserves
   a bridge that feeds playit pointer/key events into synthetic LVGL
   input-device read state or direct LVGL test injection.
4. **Deterministic tests drive LVGL explicitly.** Tests advance LVGL
   ticks/timers/input reads directly; they MUST NOT depend on wall-clock
   scheduling.
5. **Framebuffer dump behavior is unchanged.** `D`, `RS`, `RE`, and
   `RD` playit behavior remains owned by the display/recorder surfaces
   until an LVGL-backed app target consumes them.

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `Runtime` | Must be alive before event/focus/input wrappers create LVGL handles. |
| `LvDisplay` | A display must exist for object and input tests that need a default screen. This phase does not change display ownership. |
| `LvObject` / `ObjectView` | Event callbacks target these handles; focus groups and input devices observe them but do not own them. |
| `core::Event` | Compatibility/playit stream only. No LVGL object-event semantics are added here. |
| `WidgetNode` / `Renderer` | Compatibility-only. No bubbling, focus, or LVGL input routing is added to this tree. |
| `playit` | Supplies existing command values and future synthetic LVGL injection tests. Wire grammar remains stable. |
| platform board drivers | Existing board input drivers are not migrated in this phase. Future platform chapters may bind physical devices to `LvInputDevice`. |

## 11. Non-Goals

- No rlvgl `ObjectEvent` dispatcher port.
- No compatibility `WidgetNode` event rewrite.
- No new playit command grammar.
- No widget wrapper implementation.
- No scroll-specific events beyond naming existing LVGL codes; LPAR-CPP-05
  owns scroll wrappers.
- No draw-phase event wrappers; LPAR-CPP-08 owns draw/text/image detail.
- No board-specific touch/key/encoder driver migration.
- No handwritten replacement state machines for the later state-chart
  demo.

## 12. Acceptance Checklist

LPAR-CPP-04 implementation is complete only when:

- [x] `EventView` or equivalent non-owning LVGL event view exists with
      explicit lifetime comments.
- [x] `EventCode` maps the v1 LVGL event set and preserves unknown raw
      event codes.
- [x] Event callback registration/removal is wrapped with documented
      userdata ownership and callback-duration borrows.
- [x] Event callback tests verify LVGL target/current-target and at
      least one bubbling or trickling path through real LVGL objects.
- [x] `LvGroup` or equivalent move-only focus-group owner exists and is
      tested for create/delete, add object, next/previous focus, wrap,
      and editing mode.
- [x] `LvInputDevice` or equivalent move-only input-device owner exists
      and is tested for pointer, keypad, and encoder synthetic reads.
- [x] Keypad or encoder tests bind an input device to `LvGroup` and
      verify delivery to the focused object through LVGL callbacks.
- [x] playit pointer/key commands can feed the LVGL-backed synthetic
      input seam without changing the playit wire grammar.
- [x] Existing compatibility `core::Event`, playit conversion, object,
      display, widget, and app tests still compile and pass.
- [x] Embedded posture compile check passes for the new public headers.

## 13. Files Cited

- `lvgl/src/misc/lv_event.h`
- `lvgl/src/core/lv_obj_event.h`
- `lvgl/src/core/lv_obj.h`
- `lvgl/src/core/lv_group.h`
- `lvgl/src/indev/lv_indev.h`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/display.hpp`
- `core/include/lvglpp/core/input.hpp`
- `core/include/lvglpp/core/runtime.hpp`
- `core/include/lvglpp/core/event.hpp`
- `playit/include/lvglpp/playit/conversion.hpp`
- `playit/include/lvglpp/playit/lvgl_input_bridge.hpp`
- `rlvgl/docs/concepts/LPAR-04-EVENT-FOCUS-INPUT.md`
- `docs/lvgl-parity/00-concepts.md`
- `docs/lvgl-parity/01-baseline.md`
- `docs/lvgl-parity/02-object-substrate.md`
- `docs/lvgl-parity/03-invalidation-display.md`

## 14. Unblocks

- LPAR-CPP-05 scroll wrappers with LVGL scroll events and input-driven
  scroll behavior.
- LPAR-CPP-06 timers/animations that drive event-visible visual changes.
- LPAR-CPP-12 through LPAR-CPP-14 control widgets that need focus,
  key/encoder input, and click/value-change callbacks.
- LVGL-backed playit smoke tests for host and board demos.
- SCTD state-chart demo event injection once istate-generated C++ state
  machines are available.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial event/focus/input draft. Adapts rlvgl LPAR-04 by delegating object event routing, focus traversal, and input-device processing to LVGL, while preserving `core::Event` and playit as compatibility streams. |
| 2026-06-29 | RATIFIED | Ratified by owner instruction. Implementation unblocked under the LVGL-underneath rule; no rlvgl-style object-event dispatcher is permitted for LVGL-backed objects. |
| 2026-06-29 | IMPLEMENTED | Added `EventCode`, `EventView`, `EventSubscription`, `LvGroup`, `GroupView`, `LvInputDevice`, `InputDeviceView`, and a playit `LvglInputBridge` over LVGL event/group/indev APIs. Test targets `lvglpp_core_input` and `lvglpp_playit_lvgl_input_bridge` validate callbacks, bubbling, focus, synthetic pointer/key/encoder reads, and playit injection. Full default tests and embedded-posture `lvglpp_core`/`lvglpp_playit` compile pass. |
