<!--
07-style-theme.md - lvglpp LPAR-07 mirror style and theme plan.
-->

# LPAR-CPP-07 - LVGL Style and Theme Substrate

Status: **RATIFIED** (2026-06-30). Normative for the LPAR-CPP-07
LVGL-backed style and theme wrapper implementation.

Parent initiative: [`00-concepts.md`](00-concepts.md). Baseline:
[`01-baseline.md`](01-baseline.md). Object substrate:
[`02-object-substrate.md`](02-object-substrate.md). Display and
invalidation: [`03-invalidation-display.md`](03-invalidation-display.md).
Event/focus/input: [`04-event-focus-input.md`](04-event-focus-input.md).
Timers/animations: [`06-timers-object-anim.md`](06-timers-object-anim.md).

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-07 relationship |
| --- | --- | --- |
| LVGL style values and properties | `lvgl/src/misc/lv_style.h`, generated `lv_style_gen.h` | Canonical implementation. lvglpp wraps `lv_style_t`, `lv_style_prop_t`, `lv_style_value_t`, property set/get/remove, transition descriptors, property defaults, and property flags. |
| LVGL object style stack | `lvgl/src/core/lv_obj_style.h`, generated `lv_obj_style_gen.h` | Canonical cascade, selector, inheritance, local-style, added-style, disabled-style, refresh, and resolved-property behavior. lvglpp MUST delegate to LVGL instead of adding a C++ cascade registry. |
| LVGL states and parts | `lvgl/src/core/lv_obj_style.h` `lv_state_t`, `lv_part_t`, `lv_style_selector_t` | Canonical bit positions and selector encoding. lvglpp wrappers mirror these constants exactly. |
| LVGL themes | `lvgl/src/themes/lv_theme.h`, `lvgl/src/themes/default`, `simple`, `mono` | Canonical theme chain and apply behavior. lvglpp wraps `lv_theme_t` owner/view and built-in theme init/get/apply APIs where enabled by `lv_conf.h`. |
| rlvgl style/theme phase | `rlvgl/docs/concepts/LPAR-07-STYLE-THEME.md` (`v0.2.5 @ f999f75`) | Canonical cross-language behavior vocabulary. lvglpp adapts by using LVGL's style stack and theme chain instead of porting rlvgl's `ObjectNode` style slot or top-down cascade. |
| Existing compatibility style surface | `core/include/lvglpp/core/style.hpp` | `lvglpp::core::Style`, `StyleBuilder`, `Theme`, `LightTheme`, `DarkTheme`, `Easing`, and `LoopMode` remain compatibility value types for the retained renderer/widget path. They are not the LVGL-backed parity style runtime. |
| Current lvglpp LVGL handles | `core/include/lvglpp/core/object.hpp`, `display.hpp`, `timer.hpp` | `ObjectView`, `LvObject`, `LvDisplay`, `AnimationTemplate`, and deterministic LVGL tick driving are the adjacent handles used by this phase. |
| Ownership discipline | top-level `AGENTS.md` | Every raw LVGL style, theme, selector, transition property array, font pointer, callback, and user-data pointer touched by this phase MUST carry explicit ownership/lifetime comments. |

If this chapter changes public selector encoding, style ownership,
theme ownership, or compatibility-style behavior, §15 MUST be amended
first. The default strategy is additive.

## 1. Purpose

Define the LVGL-backed C++ style and theme surface needed by later
wrappers and generated demos. This phase gives lvglpp code a way to:

- create and own `lv_style_t` values through RAII;
- borrow/observe style and theme handles without taking ownership;
- encode LVGL part/state selectors in typed C++ form;
- add, replace, remove, disable, and refresh styles on real `lv_obj_t`
  objects;
- set/get/remove style properties through generic and focused typed APIs;
- configure LVGL style transitions through `lv_style_transition_dsc_t`;
- read resolved style properties from LVGL's cascade;
- create or observe LVGL themes and apply the active display theme.

This phase intentionally does not port rlvgl's retained `ObjectNode`
style slot, custom cascade walk, inherited-context propagation, or
tick-only transition clock. LVGL already owns style storage, selector
matching, inheritance, transitions, invalidation, and theme application
for real `lv_obj_t` trees.

## 2. Problem Statement

LPAR-CPP-02 through LPAR-CPP-06 establish LVGL-backed object, display,
event/input, scroll, timer, and animation wrappers. They still leave
styles on the compatibility side: `lvglpp::core::Style` is a small value
type used by the retained renderer/widgets, not a wrapper around
`lv_style_t`. Later LVGL-backed widget phases need actual LVGL style
handles, selectors, local style properties, transitions, and themes.

rlvgl LPAR-07 defines a Rust cascade because rlvgl owns its object tree.
lvglpp's route is different: LVGL's object style stack stores added
styles, local style properties, selector states/parts, disabled style
entries, resolved properties, inherited properties, and refresh/invalidate
effects. A C++ reimplementation would duplicate LVGL state and risk
divergence from upstream widget behavior.

The missing work is a typed C++ wrapper surface with explicit lifetimes
and tests that prove lvglpp can configure and observe LVGL styling
without breaking the existing compatibility `core::Style` value type.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **`LvStyle`** | Owned by LPAR-CPP-07; move-only RAII owner for `lv_style_t`. Initializes with `lv_style_init`, clears with `lv_style_reset`, and may copy/merge through explicit APIs. |
| **`StyleView`** | Owned by LPAR-CPP-07; non-owning observation wrapper around `lv_style_t*` / `const lv_style_t*`. It never resets or deletes the style. |
| **Style property** | As defined in `lvgl/src/misc/lv_style.h` `lv_style_prop_t`; a frozen LVGL property identifier. |
| **Style value** | As defined in `lvgl/src/misc/lv_style.h` `lv_style_value_t`; numeric, color, or pointer payload selected by the property type. |
| **Part** | As defined in `lvgl/src/core/lv_obj_style.h` `lv_part_t`; mirrored by a C++ `StylePart` wrapper with LVGL bit positions. |
| **State mask** | As defined in `lvgl/src/core/lv_obj_style.h` `lv_state_t`; mirrored by a C++ `StyleState` wrapper and compatible with existing `ObjectState` values where they share LVGL state constants. |
| **Style selector** | As defined in `lvgl/src/core/lv_obj_style.h` `lv_style_selector_t`; C++ wrapper composed from one part and zero or more LVGL state bits. |
| **Added style** | A style pointer stored on an LVGL object through `lv_obj_add_style`; LVGL observes it and the caller must keep the style alive while attached. |
| **Local style property** | An LVGL object-local property set through `lv_obj_set_local_style_prop`; LVGL owns storage internally on the object. |
| **Resolved property** | The LVGL cascade output returned by `lv_obj_get_style_prop`, including local styles, added styles, theme styles, inheritance, and property defaults. |
| **Style transition descriptor** | As defined in `lvgl/src/misc/lv_style.h` `lv_style_transition_dsc_t`; describes LVGL-managed transition properties, path callback, duration, delay, and user data. |
| **`LvTheme`** | Owned by LPAR-CPP-07; move-only RAII owner for heap-created `lv_theme_t*` from `lv_theme_create`, deleted with `lv_theme_delete`. |
| **`ThemeView`** | Owned by LPAR-CPP-07; non-owning wrapper for LVGL themes returned by display/built-in theme APIs or borrowed from `LvTheme`. |
| **Compatibility style** | Existing `lvglpp::core::Style` in `core/include/lvglpp/core/style.hpp`; retained for the current renderer/widget path and not used as the LVGL style cascade. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| Style owner storage | `lvgl/src/misc/lv_style.h` `lv_style_t`, `lv_style_init`, `lv_style_reset`, `lv_style_copy`, `lv_style_merge` |
| Generic property API | `lv_style_set_prop`, `lv_style_get_prop`, `lv_style_remove_prop`, `lv_style_prop_get_default`, `lv_style_prop_lookup_flags` |
| Generated typed property API | `lvgl/src/misc/lv_style_gen.h`, `lvgl/src/core/lv_obj_style_gen.h` |
| Transition descriptor | `lv_style_transition_dsc_t`, `lv_style_transition_dsc_init` |
| Parts/states/selectors | `lv_part_t`, `lv_state_t`, `lv_style_selector_t`, `lv_obj_style_get_selector_part`, `lv_obj_style_get_selector_state` |
| Object style stack | `lv_obj_add_style`, `lv_obj_replace_style`, `lv_obj_remove_style`, `lv_obj_remove_style_all`, `lv_obj_remove_theme` |
| Object local style props | `lv_obj_set_local_style_prop`, `lv_obj_get_local_style_prop`, `lv_obj_remove_local_style_prop` |
| Object resolved props | `lv_obj_get_style_prop`, generated `lv_obj_get_style_*`, `lv_obj_has_style_prop` |
| Style refresh/invalidation | `lv_obj_report_style_change`, `lv_obj_refresh_style`, `lv_obj_enable_style_refresh` |
| Themes | `lv_theme_create`, `lv_theme_delete`, `lv_theme_set_parent`, `lv_theme_set_apply_cb`, `lv_theme_apply`, built-in theme init/get/deinit APIs |
| Compatibility style | `core/include/lvglpp/core/style.hpp` |
| rlvgl conceptual vocabulary | `rlvgl/docs/concepts/LPAR-07-STYLE-THEME.md` |

## 5. Frozen Decisions - LVGL Owns Style Cascade

1. **No competing cascade.** LPAR-CPP-07 MUST NOT port rlvgl's
   `StyleState`, `ObjectNode` style slot, local/added style vectors, or
   inherited-context traversal as the runtime path for LVGL-backed
   objects.
2. **Use LVGL object style stack.** Added styles are attached with
   `lv_obj_add_style`; local properties are set with
   `lv_obj_set_local_style_prop`; resolved properties are read from LVGL.
3. **Use LVGL invalidation.** Style mutations and state changes rely on
   LVGL style refresh/invalidation behavior. lvglpp may expose refresh
   helpers, but it does not add a second dirty planner.
4. **Use LVGL transition descriptors.** Style transitions use
   `lv_style_transition_dsc_t` and LVGL's animation engine. Durations
   and delays MAY be milliseconds because LVGL's API is millisecond
   based; deterministic tests drive LVGL ticks/handler explicitly.
5. **Use LVGL inheritance.** Inheritable properties are resolved by
   LVGL. lvglpp does not thread an inherited context through a C++ tree.
6. **Compatibility style remains separate.** Existing
   `lvglpp::core::Style` and `StyleBuilder` continue to compile
   unchanged; LVGL-backed parity claims are made only for `LvStyle` and
   object style helpers.

## 6. Frozen Decisions - Parts, States, and Selectors

LPAR-CPP-07 SHALL introduce or reserve these selector concepts:

| Surface | LVGL analogue |
| --- | --- |
| `StylePart::Main` | `LV_PART_MAIN` |
| `StylePart::Scrollbar` | `LV_PART_SCROLLBAR` |
| `StylePart::Indicator` | `LV_PART_INDICATOR` |
| `StylePart::Knob` | `LV_PART_KNOB` |
| `StylePart::Selected` | `LV_PART_SELECTED` |
| `StylePart::Items` | `LV_PART_ITEMS` |
| `StylePart::Cursor` | `LV_PART_CURSOR` |
| `StylePart::CustomFirst` / custom helper | `LV_PART_CUSTOM_FIRST` and above |
| `StylePart::Any` | `LV_PART_ANY` |
| `StyleState` bit wrapper | `lv_state_t`, including default, checked, focused, focus-key, edited, hovered, pressed, scrolled, disabled, user bits, and any |
| `StyleSelector` | `lv_style_selector_t` composed from a part and state mask |

Growth policy:

1. `StylePart`, `StyleState`, and `StyleSelector` are **Standards
   Action** because they mirror LVGL bit positions and are a
   cross-language style contract.
2. Existing `ObjectState` values remain the object-state wrapper for
   state mutation. `StyleState` is the selector-state wrapper. Conversion
   helpers MAY exist, but they MUST preserve LVGL bit positions exactly.
3. A default selector has part `Main` and state mask `Default`/zero.
   It follows LVGL semantics: it is a base style and participates in
   normal LVGL matching.
4. `Any` part/state are removal/query wildcard values only where LVGL
   documents support. They MUST NOT be treated as ordinary style apply
   selectors unless LVGL accepts them for that API.

## 7. Frozen Decisions - Style API Surface v1

LPAR-CPP-07 SHALL introduce or reserve these style concepts:

| Surface | Required shape |
| --- | --- |
| `LvStyle` | Move-only RAII owner with `// owns:` embedded or pointer-backed `lv_style_t`; destructor calls `lv_style_reset`. |
| `StyleView` | Non-owning view over mutable or const `lv_style_t`; no reset/delete side effects. |
| `make_style()` | Initializes a new LVGL style and returns `LvStyle`. |
| `copy_from` / `merge_from` | Explicit LVGL copy/merge operations; no hidden sharing. |
| `set_prop` / `get_prop` / `remove_prop` | Generic property access using `lv_style_prop_t` and `lv_style_value_t`. |
| Typed property helpers | Focused helpers for initial parity properties: background color/opacity, border color/width/opacity, radius, text color/opacity/font, padding, margin, width/height, transition descriptor. |
| `add_style(ObjectView, StyleView, StyleSelector)` | Calls `lv_obj_add_style`; style lifetime is external/observed by LVGL. |
| `replace_style` / `remove_style` / `remove_all_styles` / `remove_theme_styles` | Call corresponding LVGL object style APIs. |
| `set_local_style_prop` / `get_local_style_prop` / `remove_local_style_prop` | Call corresponding LVGL local property APIs. |
| `resolved_style_prop` | Calls `lv_obj_get_style_prop` or generated typed getters. |
| `has_style_prop` | Calls `lv_obj_has_style_prop`. |
| `report_style_change` / `refresh_style` / `set_style_refresh_enabled` | Explicit LVGL refresh helpers; no hidden refresh thread. |

Style lifetime rules:

1. `LvStyle` owns the underlying `lv_style_t` storage. Moving transfers
   ownership and leaves the source inert.
2. LVGL observes styles added to objects. An `LvStyle` attached to an
   object MUST outlive the attachment or be removed before destruction.
3. `release` MAY be provided only if it makes lifecycle transfer visible
   at the call site. A released style is no longer reset by `LvStyle`.
4. Constant LVGL styles are observed through `StyleView`; they are never
   reset by lvglpp.
5. Pointer-valued style properties such as font or image source are
   external/observed unless a future phase designs an owning asset
   capsule.

## 8. Frozen Decisions - Transition Descriptors

LPAR-CPP-07 SHALL introduce or reserve these transition concepts:

| Surface | Required shape |
| --- | --- |
| `StyleTransition` or equivalent | Owns or borrows an `lv_style_transition_dsc_t` plus the property-id array lifetime needed by LVGL. |
| property array | External/observed or owned storage; must remain valid while the transition descriptor can be read by LVGL. |
| path callback | LVGL `lv_anim_path_cb_t`; callback/user-data lifetime is external unless explicitly owned by a future callback capsule. |
| duration/delay | Milliseconds matching LVGL API. Tests remain deterministic by driving LVGL ticks. |
| user data | External/observed pointer passed through LVGL transition animation user data. |

Transition rules:

1. lvglpp MUST NOT convert style transitions into its own
   `AnimationTemplate` objects. LVGL owns transition spawning.
2. lvglpp MAY provide convenience builders for common paths, but the
   final descriptor MUST be an LVGL `lv_style_transition_dsc_t`.
3. A transition descriptor stored inside an `LvStyle` MUST not observe
   a temporary property array. The owner/view distinction must be clear.
4. Transition tests MUST drive time by `lv_tick_inc` and
   `lv_timer_handler`, not by sleeping.

## 9. Frozen Decisions - Theme API Surface v1

LPAR-CPP-07 SHALL introduce or reserve these theme concepts:

| Surface | Required shape |
| --- | --- |
| `LvTheme` | Move-only RAII owner for heap-created `lv_theme_t*` from `lv_theme_create`; destructor calls `lv_theme_delete`. |
| `ThemeView` | Non-owning view over `lv_theme_t*` returned by display or built-in theme APIs. |
| `set_parent` | Calls `lv_theme_set_parent`; parent is external/observed and must outlive child theme use. |
| `set_apply_callback` | Calls `lv_theme_set_apply_cb`; callback is external function pointer. |
| `apply_theme(ObjectView)` | Calls `lv_theme_apply`. |
| `theme_from_object(ObjectView)` | Calls `lv_theme_get_from_obj`. |
| built-in theme helpers | Optional wrappers for default/simple/mono theme init/get/deinit under the corresponding `LV_USE_THEME_*` gates. |
| theme fonts/colors | Thin wrappers around LVGL theme font/color getters. |

Theme lifetime rules:

1. `LvTheme` owns only themes created by `lv_theme_create`.
2. Built-in theme init/get functions return `ThemeView` unless LVGL
   documentation for the active pin states the caller owns deletion.
3. Parent theme pointers are external/observed. A parent theme must
   outlive the child theme chain.
4. Theme apply callbacks and any callback state are external/observed
   unless a future owning callback capsule is ratified.

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `LvObject` / `ObjectView` | Style helpers operate on these handles and never own objects. |
| `ObjectState` | Remains the object-state mutation enum. Style selector wrappers mirror LVGL state bits and may provide conversion helpers. |
| `LvDisplay` | Theme init/get/apply flows through the display associated with LVGL objects. |
| `LvTimer` / `AnimationTemplate` | Style transitions use LVGL's style transition machinery, which in turn uses LVGL animation timers. lvglpp timer wrappers are not the public style-transition runtime. |
| `core::Style` / `StyleBuilder` | Compatibility value types for retained widgets. They remain source-compatible and are not silently mapped to `lv_style_t`. |
| `core::Theme`, `LightTheme`, `DarkTheme` | Compatibility theme value mutators. They remain source-compatible; LVGL parity themes use `LvTheme`/`ThemeView`. |
| retained `WidgetNode` / `Renderer` widgets | No migration in this phase. LVGL-backed widget phases decide when to consume `LvStyle`. |
| playit wire protocol | No new commands. Style effects are observed through existing query/dump paths in later demos. |

## 11. Non-Goals

- No rlvgl `StyleState` / custom cascade port.
- No C++ parent-pointer or inherited-context traversal.
- No migration of current retained widgets to LVGL styles.
- No replacement or removal of `lvglpp::core::Style`.
- No exhaustive wrapper for every generated LVGL style setter in v1.
- No custom C++ style transition engine.
- No new playit wire grammar.
- No board-specific theme policy.

## 12. Acceptance Checklist

LPAR-CPP-07 implementation is complete only when:

- [x] `LvStyle` and `StyleView` exist with explicit owner/view raw-handle
      or embedded-storage comments and move/copy behavior matching §7.
- [x] `StylePart`, `StyleState`, and `StyleSelector` map exactly to LVGL
      part/state/selector constants and preserve wildcard semantics.
- [x] Generic style property set/get/remove wrappers call LVGL
      `lv_style_*_prop` APIs and expose property defaults/flags.
- [x] Focused typed helpers cover background color/opacity, border
      color/width/opacity, radius, text color/opacity/font, padding,
      margin, size, and transition descriptor properties where LVGL
      supports them.
- [x] Object style add/replace/remove/remove-all/remove-theme helpers
      call corresponding LVGL APIs and document style lifetime.
- [x] Local style property set/get/remove and resolved property getters
      call corresponding LVGL APIs.
- [x] Style refresh/report helpers call LVGL APIs explicitly and do not
      add a second invalidation path.
- [x] Transition descriptor wrapper preserves property-array,
      path-callback, and user-data lifetimes; transition tests drive LVGL
      ticks/handler explicitly.
- [x] `LvTheme` and `ThemeView` exist with explicit ownership rules, and
      theme parent/apply/built-in getter helpers use LVGL APIs.
- [x] Host tests create real LVGL objects, attach styles with selectors,
      mutate states, and verify resolved properties through LVGL.
- [x] Existing compatibility `core::Style`, widget, playit, timer,
      object, display, input, and scroll tests still compile and pass.
- [x] Embedded posture compile check passes for the new public headers.

## 13. Files Cited

- `lvgl/src/misc/lv_style.h`
- `lvgl/src/misc/lv_style_gen.h`
- `lvgl/src/core/lv_obj_style.h`
- `lvgl/src/core/lv_obj_style_gen.h`
- `lvgl/src/themes/lv_theme.h`
- `lvgl/src/themes/default/lv_theme_default.h`
- `lvgl/src/themes/simple/lv_theme_simple.h`
- `lvgl/src/themes/mono/lv_theme_mono.h`
- `core/include/lvglpp/core/style.hpp`
- `core/include/lvglpp/core/object.hpp`
- `core/include/lvglpp/core/display.hpp`
- `core/include/lvglpp/core/timer.hpp`
- `rlvgl/docs/concepts/LPAR-07-STYLE-THEME.md`
- `docs/lvgl-parity/00-concepts.md`
- `docs/lvgl-parity/02-object-substrate.md`
- `docs/lvgl-parity/03-invalidation-display.md`
- `docs/lvgl-parity/06-timers-object-anim.md`

## 14. Unblocks

- LPAR-CPP-08 text/draw/image/mask wrappers that need text style
  properties and image style properties.
- LPAR-CPP-10 layout wrappers that need padding, margin, size, and
  alignment style properties.
- LPAR-CPP-11 through LPAR-CPP-14 LVGL-backed widget wrappers that need
  part/state style selectors and default theme application.
- QT-CPP-02 generated C++ widget modules that need stable style builder
  and selector vocabulary.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-30 | DRAFT | Initial LVGL style/theme substrate draft. Adapts rlvgl LPAR-07 by delegating cascade, inheritance, transitions, invalidation, and themes to LVGL `lv_style_t`, object style stack, transition descriptors, and `lv_theme_t`, while preserving existing `lvglpp::core::Style` as a compatibility value type. |
| 2026-06-30 | RATIFIED | Ratified by owner instruction. Implementation unblocked under the LVGL-underneath rule; no rlvgl-style `StyleState`, object-node style slot, custom cascade, inherited-context traversal, or custom transition engine is permitted for LVGL-backed objects. |
| 2026-06-30 | IMPLEMENTED | Added `core/style_lvgl.hpp` wrappers for `LvStyle`, `StyleView`, `StylePart`, `StyleState`, `StyleSelector`, `StyleTransition`, `LvTheme`, and `ThemeView`, backed by LVGL `lv_style_t`, object style APIs, transition descriptors, and `lv_theme_t`. Test target `lvglpp_core_style_lvgl` validates selector mapping, generic and typed style properties, object style stack resolution, local properties, transition descriptors, explicit style refresh, custom theme apply, and compatibility `core::Style`. Full default tests and embedded-posture `lvglpp_core`/`lvglpp_playit` compile pass. |
