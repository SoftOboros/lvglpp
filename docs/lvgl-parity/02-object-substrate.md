<!--
02-object-substrate.md - lvglpp LPAR-02 mirror object substrate plan.
-->

# LPAR-CPP-02 - LVGL-Backed Object Substrate

Status: **RATIFIED** (2026-06-29). Normative for the LPAR-CPP-02
object substrate implementation.

Parent initiative: [`00-concepts.md`](00-concepts.md). Baseline:
[`01-baseline.md`](01-baseline.md).

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-02 relationship |
| --- | --- | --- |
| LVGL object lifecycle and tree semantics | `lvgl/src/core/lv_obj.h`, `lvgl/src/core/lv_obj_tree.h`, `lvgl/src/core/lv_obj_event.h` | Canonical implementation. lvglpp wraps these APIs and MUST NOT reimplement object ownership, child storage, deletion, flags, or state bits. |
| rlvgl object-substrate phase | `rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by delegating to LVGL instead of adding a second retained tree. |
| Current lvglpp runtime surface | `core/include/lvglpp/core/runtime.hpp` | `Runtime` and `ObjectView` are repo-canonical existing types; LPAR-CPP-02 extends around them additively. |
| Current compatibility tree | `core/include/lvglpp/core/widget.hpp`, `core/include/lvglpp/core/widget_node.hpp` | Compatibility surface. Must keep compiling; not the parity object substrate. |
| Ownership discipline | top-level `AGENTS.md` | Every raw pointer / userdata / callback handle added by this phase MUST carry ownership comments. |

If this chapter changes `Runtime`, `ObjectView`, `Widget`, or
`WidgetNode` in a source-breaking way, §15 MUST be amended first with a
migration plan. The default strategy is additive.

## 1. Purpose

Define the LVGL-backed C++ object substrate that later parity wrappers
can use for object creation, ownership, parent/child relationships,
flags, states, deletion, user data, and tree traversal.

This phase closes the gap between the current non-owning `ObjectView`
and the needed parity-owner type. It does **not** port widgets; it gives
widget phases a safe `lv_obj_t` foundation.

## 2. Problem Statement

Current lvglpp has:

- `Runtime`, which owns the global LVGL initialized lifetime;
- `ObjectView`, which observes a raw `lv_obj_t*`;
- a compatibility `WidgetNode` tree that owns C++ `Widget` instances and
  draws through `Renderer`.

That is not enough for LVGL parity. LVGL widgets are real `lv_obj_t`
nodes with parent-owned lifecycles, flags, state bits, event callbacks,
user data, and deletion events. Existing lvglpp widgets deliberately say
their LVGL cites are informative and do not wrap `lv_*` widgets. Later
LPAR-CPP phases need an owning wrapper that makes LVGL object ownership
explicit without breaking compatibility examples.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **`LvObject`** | Planned move-only RAII owner for an `lv_obj_t*`. Owns deletion through `lv_obj_delete` unless ownership has been released or transferred to a parent/container wrapper under a named API. |
| **`ObjectView`** | Existing non-owning view over `lv_obj_t*`; used without ownership transfer. |
| **Screen object** | An `lv_obj_t*` created with `lv_obj_create(nullptr)` or returned by LVGL screen APIs. It may be owned by `LvObject` only when lvglpp is responsible for deleting it. |
| **Child object** | An `lv_obj_t*` created with a parent. LVGL stores it in the parent's child list; lvglpp may still own a wrapper handle whose destructor deletes the child unless ownership is explicitly released. |
| **Owned handle** | A wrapper state that will call `lv_obj_delete(raw)` at destruction. |
| **Borrowed view** | A wrapper state that never deletes; represented by `ObjectView` or `LvObject::borrow()` return values. |
| **Callback userdata** | A pointer passed to LVGL event callbacks. In this phase it MUST observe or borrow a C++ owner whose lifetime is mechanically tied to the LVGL object. |
| **External data** | Data installed with `lv_obj_set_external_data`; may be owned by LVGL when paired with a free callback. Use only when callback/userdata lifetime cannot be expressed more simply. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| Create base object | `lvgl/src/core/lv_obj.h` `lv_obj_create` |
| Delete object/subtree | `lvgl/src/core/lv_obj_tree.h` `lv_obj_delete`, `lv_obj_clean`, `lv_obj_delete_async` |
| Parent/child operations | `lvgl/src/core/lv_obj_tree.h` `lv_obj_set_parent`, `lv_obj_get_parent`, `lv_obj_get_child`, `lv_obj_get_child_count`, `lv_obj_move_to_index`, `lv_obj_swap` |
| Flags/states | `lvgl/src/core/lv_obj.h` `LV_OBJ_FLAG_*`, `lv_obj_add/remove/set_flag`, `LV_STATE_*`, `lv_obj_add/remove/set_state` |
| Validity and delete nulling | `lvgl/src/core/lv_obj.h` `lv_obj_is_valid`, `lv_obj_null_on_delete` |
| User data | `lvgl/src/core/lv_obj.h` `lv_obj_set_user_data`, `lv_obj_get_user_data` |
| Event callback userdata | `lvgl/src/core/lv_obj_event.h` `lv_obj_add_event_cb`, `lv_obj_remove_event_cb_with_user_data` |
| Current lvglpp view | `core/include/lvglpp/core/runtime.hpp` `ObjectView` |
| Compatibility tree | `core/include/lvglpp/core/widget_node.hpp` |

## 5. Frozen Decisions - Ownership Model

1. **`LvObject` is move-only.** Copy construction and copy assignment are
   deleted. Move construction transfers deletion responsibility and
   nulls the source.
2. **Creation is explicit.** Factory names MUST show ownership:
   `make_object(parent)` or `make_screen()` returns an owning
   `LvObject`. Constructors taking a raw pointer are private or tagged.
3. **Borrowing is explicit.** `borrow()` returns `ObjectView`. It never
   transfers deletion responsibility.
4. **Release is explicit.** A named `release()` may return the raw
   `lv_obj_t*` and permanently disarm deletion. It MUST be rare and
   documented at the call site.
5. **Parenting does not hide transfer.** APIs that move an object under a
   parent MUST be named `attach_*` / `take_*` when they consume an owner,
   or `set_parent` when they only move an already-owned LVGL object.
6. **Destructor deletes synchronously.** An armed `LvObject` destructor
   calls `lv_obj_delete(raw_)`. Async delete is not the default and
   requires a named function.
7. **No `shared_ptr` object ownership.** Shared ownership of `lv_obj_t`
   wrappers is forbidden in this phase. Shared observation is via
   `ObjectView` or documented raw observation.
8. **Shared observation preserves single-writer authority.** If a later
   phase uses `std::shared_ptr` for observer or adapter lifetime, the
   observed object MUST still have exactly one mutation owner/writer.
   Shared handles MAY observe or schedule work, but MUST NOT create
   competing mutable authority. This mirrors Rust mutable-borrow plus
   observer semantics and requires documented lifetime proof.

## 6. Frozen Decisions - API Surface v1

LPAR-CPP-02 v1 SHALL introduce or reserve these concepts:

| Surface | Required shape |
| --- | --- |
| `class LvObject` | Owns nullable `lv_obj_t* raw_`; destructor deletes when armed. |
| `LvObject::make_screen()` | Creates an owned screen object using `lv_obj_create(nullptr)`. |
| `LvObject::make_child(ObjectView parent)` | Creates an owned child object using `lv_obj_create(parent.borrow_raw())`. Parent view must be non-null. |
| `LvObject::borrow()` | Returns `ObjectView`; borrows for as long as the object remains alive. |
| `LvObject::release()` | Returns `lv_obj_t*`; caller becomes responsible for explaining lifecycle. |
| `LvObject::valid()` | Uses null check and MAY call `lv_obj_is_valid` in host/test builds. |
| `ObjectView` helpers | May gain flag/state/tree helpers if non-owning wrappers are more ergonomic, but helpers MUST NOT delete. |

The implementation MAY place `LvObject` in `runtime.hpp` or a new
`object.hpp`; if new, `core.hpp` MUST re-export it.

## 7. Frozen Decisions - Flags and States

LPAR-CPP-02 defines the C++ enum vocabulary that maps directly to LVGL
flags and states. Registration policy: **Standards Action** for values
that mirror LVGL.

Initial flags:

| C++ value | LVGL value | Notes |
| --- | --- | --- |
| `ObjectFlag::Hidden` | `LV_OBJ_FLAG_HIDDEN` | skip draw/targeting |
| `ObjectFlag::Clickable` | `LV_OBJ_FLAG_CLICKABLE` | pointer targeting |
| `ObjectFlag::ClickFocusable` | `LV_OBJ_FLAG_CLICK_FOCUSABLE` | LVGL click focus behavior |
| `ObjectFlag::Checkable` | `LV_OBJ_FLAG_CHECKABLE` | toggles checked state |
| `ObjectFlag::Scrollable` | `LV_OBJ_FLAG_SCROLLABLE` | detailed scroll semantics owned by LPAR-CPP-05 |
| `ObjectFlag::EventBubble` | `LV_OBJ_FLAG_EVENT_BUBBLE` | event propagation owned by LPAR-CPP-04 |
| `ObjectFlag::EventTrickle` | `LV_OBJ_FLAG_EVENT_TRICKLE` | event propagation owned by LPAR-CPP-04 |

Initial states:

| C++ value | LVGL value | Notes |
| --- | --- | --- |
| `ObjectState::Default` | `0` / `LV_STATE_DEFAULT` | no state bits |
| `ObjectState::Checked` | `LV_STATE_CHECKED` | toggle state |
| `ObjectState::Focused` | `LV_STATE_FOCUSED` | focus state |
| `ObjectState::FocusKey` | `LV_STATE_FOCUS_KEY` | keypad/encoder focus |
| `ObjectState::Edited` | `LV_STATE_EDITED` | text/control edit mode |
| `ObjectState::Hovered` | `LV_STATE_HOVERED` | pointer hover |
| `ObjectState::Pressed` | `LV_STATE_PRESSED` | pointer/key press |
| `ObjectState::Disabled` | `LV_STATE_DISABLED` | disabled style/interaction |
| `ObjectState::Scrolled` | `LV_STATE_SCROLLED` | scroll transient |

Adding flags/states outside this table requires amending this chapter or
the owning later phase before code lands.

## 8. Frozen Decisions - Tree Operations

1. **Parent query wraps LVGL.** `parent()` returns `ObjectView` or empty
   if LVGL reports no parent.
2. **Child query wraps LVGL.** `child_count()` and `child(index)` use
   `lv_obj_get_child_count` and `lv_obj_get_child`.
3. **Reparenting wraps LVGL.** `set_parent(ObjectView)` calls
   `lv_obj_set_parent`; it does not change C++ ownership state by
   itself.
4. **Ordering wraps LVGL.** `move_to_index`, `raise_to_front`, and
   `lower_to_back` are wrappers over `lv_obj_move_to_index`.
5. **Clean is explicit.** `clean_children()` calls `lv_obj_clean` and
   invalidates any borrowed child views. Docs/tests MUST say this.
6. **Compatibility tree is not adopted in v1.** LPAR-CPP-02 does not
   convert `WidgetNode` into LVGL objects. A later migration bridge may
   build LVGL wrapper trees from generated UI.

## 9. Frozen Decisions - Userdata and Callback Lifetimes

1. **Callback userdata is observation by default.** Event callback
   userdata pointers passed through LVGL MUST be marked `observes` or
   `borrows`; no callback may own a C++ object through a raw pointer.
2. **Owner outlives callback.** The C++ object referenced by callback
   userdata MUST outlive the LVGL event descriptor or remove the event
   callback before destruction.
3. **Deletion callbacks disarm observation.** When a C++ wrapper keeps
   nullable observations to an LVGL object, it SHOULD use
   `lv_obj_null_on_delete` or an `LV_EVENT_DELETE` callback to null the
   observation.
4. **External data is opt-in.** `lv_obj_set_external_data` MAY be used
   only when the free callback and ownership transfer are documented.
5. **No capturing borrowed stack state.** Lambdas or callback structs
   registered with LVGL MUST NOT observe stack objects unless the
   callback is removed before the stack frame exits.

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `Runtime` | Must be alive before `LvObject` creation. LPAR-CPP-02 does not change singleton runtime semantics. |
| `ObjectView` | Becomes the standard borrow/view return type for owned objects and widget wrappers. |
| `WidgetNode` | Remains compatibility-only. Its ownership and traversal are not changed by this phase. |
| `Renderer` | Not used by LVGL-backed wrappers in this phase. Display flush wrappers are LPAR-CPP-03. |
| `playit` tags | Object IDs/names/tags are not standardized in v1. Playit LVGL dispatch waits for LPAR-CPP-04. |

## 11. Non-Goals

- No widget wrapper implementation.
- No style/theme wrapper implementation.
- No LVGL display driver wrapper.
- No focus group or input-device wrapper.
- No migration of existing compatibility widgets to LVGL-backed widgets.
- No custom C++ child storage parallel to LVGL's object tree.

## 12. Acceptance Checklist

LPAR-CPP-02 implementation is complete only when:

- [x] `LvObject` or equivalent move-only owner exists with explicit
      ownership comments.
- [x] `make_screen` and `make_child` create real LVGL objects.
- [x] Destructor and `release` semantics are tested.
- [x] `ObjectView` remains non-owning and cannot delete.
- [x] Flag/state wrappers cover the §7 initial values.
- [x] Parent/child/order wrappers cover §8.
- [x] Userdata/callback lifetime rules are documented next to any raw
      pointer members.
- [x] Existing `WidgetNode`/compatibility widget tests still compile.
- [x] Host unit tests exercise creation, child count, reparent/order,
      flags, states, release, and delete/nulling behavior.
- [ ] Embedded posture compile check passes for the new headers.

## 13. Files Cited

- `lvgl/src/core/lv_obj.h`
- `lvgl/src/core/lv_obj_tree.h`
- `lvgl/src/core/lv_obj_event.h`
- `lvgl/src/core/lv_obj_class.h`
- `core/include/lvglpp/core/runtime.hpp`
- `core/include/lvglpp/core/widget.hpp`
- `core/include/lvglpp/core/widget_node.hpp`
- `rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md`
- `docs/lvgl-parity/00-concepts.md`
- `docs/lvgl-parity/01-baseline.md`

## 14. Unblocks

- LPAR-CPP-03 invalidation/display wrappers.
- LPAR-CPP-04 event/focus/input wrappers.
- LPAR-CPP-07 style wrappers and all LVGL-backed widget phases.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial object-substrate draft. Adapts rlvgl LPAR-02 to lvglpp by wrapping LVGL object APIs directly, adding a move-only owning `LvObject` beside existing non-owning `ObjectView`, and preserving `WidgetNode` as compatibility-only. |
| 2026-06-29 | RATIFIED | Ratified with the invariant that any later `std::shared_ptr` observation/adaptation MUST preserve one owner/writer and Rust-like mutable-borrow/observer lifetime discipline. |
| 2026-06-29 | IMPLEMENTED | Added `LvObject`, `ObjectFlag`, `ObjectState`, LVGL tree/flag/state wrappers, and `lvglpp_core_object` host tests. Embedded posture compile remains unchecked in this turn. |
