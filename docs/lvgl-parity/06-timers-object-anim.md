<!--
06-timers-object-anim.md - lvglpp LPAR-06 mirror timer and animation plan.
-->

# LPAR-CPP-06 - LVGL Timers and Object Animation

Status: **RATIFIED** (2026-06-29). Normative for the LPAR-CPP-06
LVGL-backed timer and animation wrapper implementation.

Parent initiative: [`00-concepts.md`](00-concepts.md). Baseline:
[`01-baseline.md`](01-baseline.md). Object substrate:
[`02-object-substrate.md`](02-object-substrate.md). Event/focus/input:
[`04-event-focus-input.md`](04-event-focus-input.md). Scroll runtime:
[`05-scroll-runtime.md`](05-scroll-runtime.md).

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-06 relationship |
| --- | --- | --- |
| LVGL timers | `lvgl/src/misc/lv_timer.h`, `lvgl/src/misc/lv_timer.c` | Canonical implementation. lvglpp wraps `lv_timer_t` creation, deletion, pause/resume, period, repeat count, auto-delete, ready/reset, and user-data behavior; it MUST NOT add a competing C++ scheduler for LVGL-backed objects. |
| LVGL animations | `lvgl/src/misc/lv_anim.h`, `lvgl/src/misc/lv_anim.c` | Canonical implementation. lvglpp wraps `lv_anim_t` configuration, start, lookup, pause/resume, deletion, repeat/reverse/delay, callbacks, path callbacks, and user data; it MUST NOT add a competing animation engine for LVGL-backed objects. |
| LVGL tick source | `lvgl/src/tick/lv_tick.h`, LVGL timer handler APIs | Canonical elapsed-time source for timers and animations. Tests drive it explicitly; wrappers do not sleep or spawn polling loops. |
| rlvgl timer/object animation phase | `rlvgl/docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by delegating timer and animation runtime ownership to LVGL instead of porting rlvgl's Rust tick registry or object-animation slot. |
| Current lvglpp handles | `core/include/lvglpp/core/object.hpp`, `core/include/lvglpp/core/display.hpp`, `core/include/lvglpp/core/input.hpp`, `core/include/lvglpp/core/scroll.hpp` | `LvObject`, `ObjectView`, display invalidation, input routing, event observation, and explicit scroll animation mode are the adjacent handles this phase uses. |
| Existing compatibility style vocabulary | `core/include/lvglpp/core/style.hpp` | Compatibility-only unless explicitly wired to LVGL animation APIs by this phase. Existing pure value types are not treated as running LVGL animations. |
| Ownership discipline | top-level `AGENTS.md` | Every raw LVGL timer, animation, target variable, callback, and user-data pointer touched by this phase MUST carry explicit ownership/lifetime comments. |

If this chapter changes public timer/animation ownership, callback
lifetime, tick-driving, or object animation semantics, §15 MUST be
amended first. The default strategy is additive.

## 1. Purpose

Define the LVGL-backed C++ timer and animation surface needed by later
wrappers and board demos. This phase gives lvglpp code a way to:

- create and own `lv_timer_t` handles through RAII;
- borrow/observe existing LVGL timer handles without taking ownership;
- configure timer callback, period, ready/reset, repeat count,
  auto-delete, pause/resume, and user data;
- configure `lv_anim_t` values before start with explicit target,
  execution callback, duration, delay, path, repeat, reverse, early
  apply, lifecycle callbacks, and user data;
- observe or cancel running LVGL animations through LVGL's supported
  lookup/delete keys;
- animate real LVGL object properties without storing a second
  object-local animation state;
- test timer and animation behavior deterministically by driving
  `lv_tick_inc` and `lv_timer_handler`.

This phase intentionally does not port rlvgl's Rust `Timers` registry,
`ObjectAnims` slot, custom tween runner, or tick-only public duration
model. LVGL already owns these mechanics for real `lv_obj_t` trees.

## 2. Problem Statement

LPAR-CPP-02 through LPAR-CPP-05 establish object, display, input, event,
and scroll wrappers over real LVGL state. They do not expose a C++
surface for LVGL timers or running animations. Later widgets need
stable wrappers for delayed work, progress/spinner animation, scroll
animation inspection, transition-like behavior, and object property
tweens.

rlvgl LPAR-06 defines custom tick-domain timers and object-bound
animations because rlvgl owns a custom retained object tree and renderer.
lvglpp's route is different: LVGL creates timers with `lv_timer_create`,
runs them from `lv_timer_handler`, starts animations with
`lv_anim_start`, and finds/deletes running animations by target variable
and execution callback. A C++ port of rlvgl's scheduler would duplicate
LVGL state and create divergent behavior.

The missing work is a typed wrapper surface with clear ownership and
lifetime boundaries, plus tests that prove lvglpp can drive LVGL timers
and animations without depending on host wall-clock sleeps.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **LVGL timer** | As defined in `lvgl/src/misc/lv_timer.h` `lv_timer_t`; wrapped by lvglpp as a callback-bearing LVGL runtime handle. |
| **`LvTimer`** | Owned by LPAR-CPP-06; move-only RAII owner for a nullable `lv_timer_t*` created by LVGL and deleted with `lv_timer_delete` unless ownership is released. |
| **`TimerView`** | Owned by LPAR-CPP-06; non-owning observation/borrow wrapper around `lv_timer_t*`. It never deletes the timer. |
| **Timer callback** | As defined in `lvgl/src/misc/lv_timer.h` `lv_timer_cb_t`; lvglpp passes it through with explicit callback/user-data lifetime comments. |
| **Timer user data** | External pointer stored by LVGL through `lv_timer_set_user_data`; lvglpp does not own it unless a future API explicitly transfers ownership. |
| **LVGL animation template** | As defined in `lvgl/src/misc/lv_anim.h` `lv_anim_t` before `lv_anim_start`; wrapped by lvglpp as a value/configuration object. |
| **Running LVGL animation** | As defined in `lvgl/src/misc/lv_anim.h`; created by `lv_anim_start` and owned by LVGL's animation list. lvglpp observes/cancels it through LVGL lookup/delete APIs, not by deleting the returned pointer directly. |
| **Animation target variable** | External `void*` stored as `lv_anim_t::var`; for object animations this is usually the target `lv_obj_t*`. The target MUST outlive the running animation or the animation MUST be cancelled first. |
| **Animation execution callback** | As defined in `lvgl/src/misc/lv_anim.h` `lv_anim_exec_xcb_t` or `lv_anim_custom_exec_cb_t`; it mutates the external target variable with the current animation value. |
| **Animation path callback** | As defined in `lvgl/src/misc/lv_anim.h` `lv_anim_path_cb_t`; LVGL owns interpolation timing and calls the path callback to map progress to value. |
| **Deterministic tick driving** | Test and firmware pattern where callers advance LVGL elapsed time with `lv_tick_inc` and then call `lv_timer_handler`; wrappers do not read wall-clock time. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| Timer creation/deletion | `lvgl/src/misc/lv_timer.h` `lv_timer_create_basic`, `lv_timer_create`, `lv_timer_delete` |
| Timer control | `lv_timer_pause`, `lv_timer_resume`, `lv_timer_set_cb`, `lv_timer_set_period`, `lv_timer_ready`, `lv_timer_reset` |
| Timer repeat/auto-delete | `lv_timer_set_repeat_count`, `lv_timer_set_auto_delete` |
| Timer user data/inspection | `lv_timer_set_user_data`, `lv_timer_get_user_data`, `lv_timer_get_paused`, `lv_timer_get_next` |
| Timer driving | `lv_timer_handler`, `lv_timer_get_time_until_next`, LVGL tick APIs |
| Animation configuration | `lv_anim_init`, `lv_anim_set_var`, `lv_anim_set_exec_cb`, `lv_anim_set_custom_exec_cb`, `lv_anim_set_values`, `lv_anim_set_duration`, `lv_anim_set_delay` |
| Animation callbacks | `lv_anim_set_start_cb`, `lv_anim_set_completed_cb`, `lv_anim_set_deleted_cb`, `lv_anim_set_get_value_cb`, `lv_anim_set_user_data` |
| Animation path/repeat/reverse | `lv_anim_set_path_cb`, `lv_anim_set_reverse_duration`, `lv_anim_set_reverse_delay`, `lv_anim_set_repeat_count`, `lv_anim_set_repeat_delay`, `lv_anim_set_early_apply`, `lv_anim_set_bezier3_param` |
| Running animation lifecycle | `lv_anim_start`, `lv_anim_get`, `lv_anim_delete`, `lv_anim_pause`, `lv_anim_resume`, `lv_anim_pause_for`, `lv_anim_custom_get`, `lv_anim_custom_delete`, `lv_anim_count_running` |
| Object handles | `core/include/lvglpp/core/object.hpp` `LvObject`, `ObjectView` |
| Scroll animation mode | `core/include/lvglpp/core/scroll.hpp` `AnimationMode` |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md` |

## 5. Frozen Decisions - LVGL Owns Timer and Animation Runtime

1. **No competing scheduler.** LPAR-CPP-06 MUST NOT port rlvgl's
   `Timers` registry, object-local timer wheel, or tick registry as the
   runtime path for LVGL-backed objects.
2. **No competing animation runner.** LPAR-CPP-06 MUST NOT port
   rlvgl's `ObjectAnims` slot or custom tween walker as the runtime path
   for LVGL-backed objects.
3. **Use LVGL handles.** Timers are represented by `lv_timer_t*`.
   Animations are configured with `lv_anim_t` and started/managed through
   LVGL animation APIs.
4. **Milliseconds mirror LVGL.** Public C++ timer/animation durations
   MAY use milliseconds because LVGL's timer and animation APIs are
   millisecond-based. Determinism comes from explicit LVGL tick driving,
   not from a separate tick-only duration type.
5. **No hidden wall-clock.** lvglpp timer/animation wrappers MUST NOT
   call sleep/delay APIs, poll host clocks, spawn handler threads, or run
   `lv_timer_handler` implicitly.
6. **Use LVGL deletion semantics.** Running animations are cancelled
   through LVGL's variable/callback lookup and delete APIs. lvglpp MUST
   NOT `delete` or otherwise free a running `lv_anim_t*` returned by
   LVGL.

## 6. Frozen Decisions - Timer API Surface v1

LPAR-CPP-06 SHALL introduce or reserve these timer concepts:

| Surface | Required shape |
| --- | --- |
| `LvTimer` | Move-only RAII owner with `// owns:` raw `lv_timer_t*`; destructor calls `lv_timer_delete` when non-null. |
| `TimerView` | Copyable non-owning view with `// observes:` raw `lv_timer_t*`; no destructor side effects. |
| `make_timer(callback, period_ms, user_data)` | Calls `lv_timer_create`. Callback and user data are external unless a future overload explicitly transfers ownership. |
| `make_basic_timer()` | Calls `lv_timer_create_basic` and returns an owned timer. |
| `borrow_timer()` / `view_timer()` | Returns `TimerView` from `LvTimer` or a raw LVGL handle without ownership transfer. |
| `release_timer()` | Gives up deletion responsibility and returns the raw `lv_timer_t*`; call site makes ownership transfer visible. |
| Timer mutators | `set_callback`, `set_period`, `ready`, `reset`, `pause`, `resume`, `set_repeat_count`, `set_auto_delete`, `set_user_data`. |
| Timer observers | `is_paused`, `user_data`, and raw-handle access for C API interop. |
| Timer handler helpers | Optional thin wrappers for `lv_timer_handler`, `lv_timer_get_time_until_next`, and timer iteration; they MUST be explicit calls. |

Timer growth policy:

1. `LvTimer` ownership and `TimerView` non-ownership are
   **Specification Required** because they are local C++ wrappers over
   LVGL handles.
2. Timer callback/user-data ownership is **Standards Action** when it
   crosses into shared app or board-demo contracts. Raw pointer user data
   MUST be documented as external/observed unless an owning callback
   capsule is explicitly designed and ratified.
3. `std::shared_ptr` MUST NOT be used as a callback lifetime shortcut.
   If shared ownership is ever introduced for observation, the design
   MUST preserve a single owner/writer authority so it matches the
   rlvgl mutable-borrow/observer lifetime model.
4. LVGL auto-delete is allowed only when the C++ wrapper has released
   ownership or otherwise marks itself as non-owning before LVGL can
   delete the timer. A wrapper that still believes it owns a timer MUST
   NOT enable an LVGL path that frees the same handle behind its back.

## 7. Frozen Decisions - Animation API Surface v1

LPAR-CPP-06 SHALL introduce or reserve these animation concepts:

| Surface | Required shape |
| --- | --- |
| `AnimationTemplate` or equivalent | Value/configuration wrapper with an embedded `lv_anim_t`; initialized with `lv_anim_init` and safe to pass to `lv_anim_start`. |
| `AnimationView` or equivalent | Non-owning observation wrapper over a running `lv_anim_t*`; it never frees the pointed-to animation. |
| `AnimationKey` or equivalent | Explicit cancellation/lookup key containing the target `var` plus the execution callback family required by LVGL's `lv_anim_get` / `lv_anim_delete` APIs. |
| Target binding | `set_var`/`set_target` stores an external target pointer. Object-specific helpers may accept `ObjectView` and pass the underlying `lv_obj_t*`. |
| Execution callbacks | APIs expose both LVGL `exec_cb` and `custom_exec_cb` routes, or document why v1 only supports one route. |
| Timing and values | APIs expose values, duration, delay, reverse duration/delay, repeat count/delay, and early apply. |
| Path/callbacks | APIs expose path callback, start callback, completed callback, deleted callback, get-current-value callback, bezier parameters, and user data where LVGL supports them. |
| Start | `start()` calls `lv_anim_start`, returns an observation/cancellation handle, and leaves runtime ownership with LVGL. |
| Cancel/lookup | Calls `lv_anim_delete`/`lv_anim_get` or `lv_anim_custom_delete`/`lv_anim_custom_get` according to the configured callback family. |
| Pause/resume | Calls `lv_anim_pause`, `lv_anim_pause_for`, and `lv_anim_resume` on a running animation view when LVGL returns a handle. |

Animation growth policy:

1. Animation wrapper enums or value types that mirror LVGL constants are
   **Standards Action**.
2. Object-bound animation helpers are **Specification Required** for
   this phase unless their names become cross-language public API.
3. `AnimationMode` from LPAR-CPP-05 remains the wrapper for
   `lv_anim_enable_t` used by scroll helpers. LPAR-CPP-06 MAY re-export
   it but MUST NOT redefine incompatible values.
4. Animation target pointers, callback pointers, and user data are
   external/observed by default. Their lifetimes MUST outlive the running
   animation or the animation MUST be cancelled before teardown.
5. Object animations use the LVGL object pointer as the target variable
   and LVGL setters as execution callbacks. lvglpp does not add a
   second object-local animation list.
6. At the current LVGL submodule pin, custom animation lookup/delete is
   var-wide in practice because `lv_anim_custom_get` /
   `lv_anim_custom_delete` route through the normal `exec_cb` lookup
   slot. lvglpp MAY expose custom callbacks, but custom cancellation
   MUST document that it cancels the target variable's matching custom
   animation through LVGL's supported var-wide delete behavior.

## 8. Frozen Decisions - Deterministic Driving and Tests

1. Host tests MUST drive time with explicit LVGL calls: increment the
   LVGL tick source, then call `lv_timer_handler`.
2. Tests MUST NOT depend on `sleep`, `usleep`, OS timers, host
   scheduling jitter, or wall-clock timestamps.
3. Wrappers MUST NOT hide `lv_timer_handler` in constructors,
   destructors, callbacks, background threads, or `Runtime` internals.
4. Timer tests MUST use real `lv_timer_t` callbacks and verify period,
   ready/reset, pause/resume, repeat count, auto-delete-safe behavior,
   and user-data round trips.
5. Animation tests MUST use real `lv_anim_t` instances and verify value
   progression, delay, completion/deletion callbacks, pause/resume or
   pause-for, repeat/reverse behavior, and cancellation through LVGL's
   supported lookup/delete APIs.
6. Object animation tests MUST animate at least one real LVGL object
   property and observe the resulting object state, invalidation, or
   flush effect through existing lvglpp wrappers.

## 9. Frozen Decisions - Callback and Lifetime Rules

1. Callback storage MUST be mechanically safe. A callback that captures
   C++ state through user data MUST document whether that state is
   external, observes, borrows, owns, or shares per top-level
   `AGENTS.md`.
2. Raw target and user-data pointers MUST be nullable only when LVGL
   accepts null for that API and the wrapper documents the behavior.
3. Ownership transfer into a callback capsule, if introduced, MUST be
   visible at the call site through `std::move`, a `make_*` factory, or
   a named `take_*` API.
4. Borrowed C++ objects MUST NOT be stored into LVGL user data unless
   the wrapper proves the borrow cannot outlive the owner.
5. `shared_ptr` MAY only appear when shared lifetime is intrinsic. It
   MUST preserve a single writer/owner authority for mutable state; using
   it to avoid deciding ownership is forbidden.
6. Interrupt or driver-owned contexts that invoke timer control APIs
   MUST follow LVGL's documented thread/interrupt safety boundaries and
   must not mutate DMA/MMIO buffers without the synchronization required
   by the relevant board phase.

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `LvObject` / `ObjectView` | Object animation helpers operate on these handles and never own objects. |
| `LvDisplay` / invalidation helpers | LVGL owns invalidation caused by animated object property changes. Tests may observe invalidated/flushed areas; no second dirty planner is introduced. |
| `LvInputDevice` / `LvglInputBridge` | No new input behavior is added. Input-driven scroll animations remain LVGL behavior. |
| `EventView` / `EventCode` | Timer and animation lifecycle callbacks are LVGL callbacks, not object events, in v1. No new `EventCode` entries are required by this phase. |
| `AnimationMode` | Existing explicit scroll animation enable/disable wrapper remains valid and may be used by timer/animation tests. |
| compatibility `Easing` / `LoopMode` | Existing pure value vocabulary remains compatibility code unless explicitly wired to LVGL path callbacks by this phase. No deprecation is implied by this draft. |
| `WidgetNode` / `Renderer` | Compatibility-only. No LVGL timer or animation behavior is added to the retained compatibility tree. |
| playit wire protocol | No new commands are added. Later demos may use existing tick/present/query commands to observe animation effects. |

## 11. Non-Goals

- No rlvgl `Timers` registry port.
- No rlvgl `ObjectAnims` or custom tween walker port.
- No C++ replacement for LVGL's animation path functions.
- No wall-clock sleeps, background handler thread, or hidden polling
  loop.
- No style-transition wrapper; LPAR-CPP-07 owns style/theme
  reconciliation.
- No new playit wire grammar.
- No board-specific hardware timer, RTOS, or interrupt integration.
- No handwritten state-chart machines for the ESP P4 demo; that work is
  owned by the SCTD/istate phases.

## 12. Acceptance Checklist

LPAR-CPP-06 implementation is complete only when:

- [x] `LvTimer` and `TimerView` exist with explicit owner/view raw-handle
      comments and move/copy behavior matching §6.
- [x] Timer create/delete, release, pause/resume, ready/reset, period,
      repeat count, auto-delete-safe behavior, callback, and user-data
      APIs call the corresponding LVGL functions.
- [x] Timer tests drive LVGL ticks/handler explicitly and do not sleep.
- [x] An animation configuration wrapper exists with explicit target,
      callback, user-data, and running-animation lifetime rules.
- [x] Animation start, lookup, cancel, pause/resume or pause-for,
      timing/value, repeat/reverse, path, lifecycle-callback, and
      user-data APIs call the corresponding LVGL functions.
- [x] Object animation tests animate a real LVGL object property through
      LVGL callbacks and observe the resulting state or invalidation.
- [x] Existing object, display, input, scroll, playit, widget, and app
      tests still compile and pass.
- [x] Embedded posture compile check passes for the new public headers;
      wrappers do not require exceptions, RTTI, wall-clock facilities, or
      heap ownership beyond LVGL's own runtime allocation.

## 13. Files Cited

- `lvgl/src/misc/lv_timer.h`
- `lvgl/src/misc/lv_timer.c`
- `lvgl/src/misc/lv_anim.h`
- `lvgl/src/misc/lv_anim.c`
- `lvgl/src/tick/lv_tick.h`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/display.hpp`
- `core/include/lvglpp/core/input.hpp`
- `core/include/lvglpp/core/scroll.hpp`
- `core/include/lvglpp/core/style.hpp`
- `rlvgl/docs/concepts/LPAR-06-TIMERS-OBJECT-ANIM.md`
- `docs/lvgl-parity/00-concepts.md`
- `docs/lvgl-parity/02-object-substrate.md`
- `docs/lvgl-parity/04-event-focus-input.md`
- `docs/lvgl-parity/05-scroll-runtime.md`

## 14. Unblocks

- LPAR-CPP-07 style/theme wrappers that need LVGL animation and
  transition timing vocabulary.
- LPAR-CPP-11 through LPAR-CPP-14 widget wrappers that need delayed work
  or animated object properties.
- Host and board demos that need deterministic animation smoke tests.
- SCTD-generated state-chart demos that need timers as LVGL-backed event
  sources.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial timer/object-animation draft. Adapts rlvgl LPAR-06 by delegating timers and running animations to LVGL `lv_timer_t` / `lv_anim_t`, while requiring explicit C++ ownership, callback lifetime documentation, and deterministic `lv_tick_inc` + `lv_timer_handler` tests. |
| 2026-06-29 | RATIFIED | Ratified by owner instruction. Implementation unblocked under the LVGL-underneath rule; no rlvgl-style timer registry, object-animation slot, or hidden wall-clock handler is permitted for LVGL-backed objects. |
| 2026-06-29 | IMPLEMENTED | Added `core/timer.hpp` wrappers for `LvTimer`, `TimerView`, `AnimationTemplate`, `AnimationView`, and `AnimationKey`, backed by LVGL `lv_timer_t` and `lv_anim_t`. Test target `lvglpp_core_timer` validates explicit tick-driven timers, safe LVGL auto-delete release, animation start/find/cancel/pause/resume, lifecycle callbacks, and object-property animation. Full default tests and embedded-posture `lvglpp_core`/`lvglpp_playit` compile pass. |
