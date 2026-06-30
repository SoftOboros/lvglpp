<!--
01-baseline.md - lvglpp LPAR-01 mirror baseline matrix.
-->

# LPAR-CPP-01 - Baseline Matrix

Status: **RATIFIED** (2026-06-29). Normative for lvglpp Wave 0 baseline
reconciliation.

Parent initiative: [`00-concepts.md`](00-concepts.md), ratified
2026-06-29.

## 0. Authority Policy

| Concern | Owner | LPAR-CPP-01 relationship |
| --- | --- | --- |
| LVGL source baseline | `lvgl/` submodule | Pins the exact C source and version macros for lvglpp wrapper parity claims. |
| rlvgl parity baseline | `rlvgl/docs/concepts/LPAR-01-BASELINE.md` at `v0.2.5 @ f999f75` | Canonical phase/status vocabulary. lvglpp mirrors the matrix while adapting implementation to LVGL-backed C++ wrappers. |
| Current lvglpp C++ surface | `core/`, `widgets/`, `ui/`, `platform/`, `playit/`, `examples/apps/disco-demo/` | Repo is canonical for current coverage and compatibility surfaces. |
| LVGL config baseline | `lvgl/lv_conf_template.h`, `lvgl/src/lv_conf_internal.h` | Used as source inventory until a project `lv_conf.h` lands. |

If the `lvgl/` or `rlvgl/` submodule pin advances, or if a project
`lv_conf.h` lands, this document MUST be amended before later phases
claim parity against the new target.

## 1. Purpose

Define the fixed baseline for the first lvglpp parity cycle: LVGL source
pin, rlvgl reference pin, config assumptions, current C++ coverage,
missing LVGL-backed wrapper surface, naming policy, and phase ownership.

## 2. Problem Statement

lvglpp has useful early mirrors, but most are compatibility-tree
implementations. Examples:

- `lvglpp::core::Runtime` calls `lv_init()` and `ObjectView` observes
  `lv_obj_t*`, but there is no owning RAII `LvObject`.
- `Label`, `Button`, `Checkbox`, `Switch`, `Slider`, `Container`,
  `List`, and `Image` exist, but their headers cite LVGL as informative
  and do not wrap `lv_label`, `lv_button`, `lv_slider`, or other real
  LVGL widgets.
- `playit` has strong wire-format coverage, but dispatch targets
  `WidgetNode`, not LVGL object tags/events.
- `platform` has SDL/fbdev/disco work, but no LVGL display/input wrapper
  baseline for parity widgets.

LPAR-CPP-01 records that as the starting point so later phases can
separate "currently available compatibility API" from "LVGL parity
wrapper."

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **LVGL source baseline** | `lvgl/` at commit `ee436e8520b9c44752e22142448b1dda5bf452a9`; owned by this chapter. |
| **rlvgl reference baseline** | `rlvgl/` at branch `v0.2.5`, commit `f999f75ace7f61d3a4766b46f461498ff885aec8`; owned by `.gitmodules` and the gitlink. |
| **Compatibility surface** | Existing C++ API that mirrors earlier rlvgl concepts but does not delegate to real LVGL widgets/subsystems. |
| **Parity wrapper** | LVGL-backed C++ wrapper introduced by LPAR-CPP phases. |
| **Baseline status** | One of Current, Partial, Missing, Optional, or Adjacent, matching rlvgl LPAR-01 vocabulary. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact |
| --- | --- |
| LVGL version macros | `lvgl/lv_version.h` |
| LVGL widget inventory | `lvgl/src/widgets/` |
| Current lvglpp core surface | `core/include/lvglpp/core/` |
| Current lvglpp widget surface | `widgets/include/lvglpp/widgets/` |
| Current lvglpp UI/platform/playit surfaces | `ui/include/`, `platform/include/`, `playit/include/` |
| rlvgl LPAR baseline vocabulary | `rlvgl/docs/concepts/LPAR-01-BASELINE.md` |
| lvglpp phase order | `docs/lvgl-parity/00-concepts.md` |

## 5. Frozen Decisions - Baseline Pin

| Field | Value |
| --- | --- |
| LVGL source path | `lvgl/` |
| LVGL source commit | `ee436e8520b9c44752e22142448b1dda5bf452a9` |
| LVGL version macros | `LVGL_VERSION_MAJOR=9`, `LVGL_VERSION_MINOR=6`, `LVGL_VERSION_PATCH=0`, `LVGL_VERSION_INFO="dev"` |
| Effective target label | `LVGL 9.6.0-dev @ ee436e8` |
| rlvgl source path | `rlvgl/` |
| rlvgl source commit | `f999f75ace7f61d3a4766b46f461498ff885aec8` |
| rlvgl branch | `v0.2.5` |
| Config source | `lvgl/lv_conf_template.h` plus `lvgl/src/lv_conf_internal.h` defaults |
| Project `lv_conf.h` | Not present in this checkout |

The baseline is a source-feature inventory. A later C-reference fixture
MAY compile LVGL with a project config, but that config must cite this
chapter or amend it first.

## 6. Frozen Decisions - Conformance Levels

| Level | Meaning | Required evidence |
| --- | --- | --- |
| **LPAR-CPP-Core** | LVGL-backed object/runtime/style/draw/layout wrappers needed by default-enabled LVGL widgets. | Phase docs, C++ unit tests over real LVGL APIs, embedded-posture compile checks. |
| **LPAR-CPP-Widget** | A specific LVGL widget family has a typed C++ wrapper with documented rlvgl behavior relationship. | Per-widget docs, behavior tests, and where practical LVGL render/geometry smoke tests. |
| **LPAR-CPP-Optional** | Heavy or integration-dependent LVGL widgets/features such as Lottie, 3DTexture, GIF, or board-specific acceleration. | Feature-gated implementation and dependency/footprint notes. |
| **LPAR-CPP-Adjacent** | Useful lvglpp/rlvgl-compatible API that is not an LVGL parity wrapper. | Documentation labels it adjacent; no parity claim is made. |

No commit or release may claim generic "LVGL parity." It may claim only
named rows and levels from this chapter or a ratified child chapter.

## 7. Frozen Decisions - Naming Policy

1. LVGL-backed wrapper modules SHOULD use LVGL widget family names
   without the `lv_` prefix: `arc`, `bar`, `buttonmatrix`,
   `imagebutton`, `msgbox`, `tabview`, `tileview`.
2. Public C++ types SHOULD use idiomatic UpperCamelCase while keeping the
   LVGL name recognizable: `Arc`, `Bar`, `ButtonMatrix`,
   `ImageButton`, `MessageBox`, `TabView`, `TileView`.
3. Existing compatibility names remain stable until a migration chapter
   says otherwise.
4. If an existing type name collides with a parity wrapper, the parity
   wrapper lands under a clearly documented namespace or suffix rather
   than silently changing the existing class behavior.
5. `objx_templ` is template/reference material and is not a parity widget
   target.

## 8. Frozen Decisions - Runtime and Substrate Matrix

| Area | lvglpp baseline status | Current lvglpp surface | Owning phase |
| --- | --- | --- | --- |
| Runtime init | Partial | `Runtime` RAII around `lv_init()` | LPAR-02 |
| Object tree / `lv_obj_t` ownership | Partial | `ObjectView` observes; no owning `LvObject` | LPAR-02 |
| Screen roots / lifecycle | Missing | compatibility `WidgetNode` roots only | LPAR-02 |
| Invalidation / dirty propagation | Missing for LVGL-backed wrappers | compatibility renderer tests | LPAR-03 |
| Display flush wrappers | Partial | SDL/fbdev/disco surfaces, not LVGL display wrapper | LPAR-03 |
| Event vocabulary and propagation | Partial | `core::Event`, playit conversion; no `lv_event_t` adapter | LPAR-04 |
| Focus groups / input devices | Missing for LVGL-backed wrappers | SDL/evdev/playit compatibility paths | LPAR-04 |
| Scroll runtime | Missing for LVGL-backed wrappers | no common LVGL scroll wrapper | LPAR-05 |
| Timers / object animations | Missing for LVGL-backed wrappers | `Easing`/`LoopMode` value types only | LPAR-06 |
| Style cascade / parts / states | Partial | `core::Style` value type; no `lv_style_t` owner | LPAR-07 |
| Text / font / wrapping | Partial | bitmap/packed font compatibility renderer | LPAR-08 |
| Image descriptors / masks / transforms | Partial | RLE decode + compatibility `Image`; no LVGL image wrapper | LPAR-08/09 |
| Asset source conventions | Partial | RLE plugin and app assets | LPAR-09 |
| Flex / grid layout | Missing for LVGL-backed wrappers | no LVGL layout wrappers | LPAR-10 |
| Property / observer | Missing | none outside app/generated concepts | LPAR-15 |

## 9. Frozen Decisions - Widget Matrix

Status meanings mirror rlvgl LPAR-01:

- **Current:** LVGL-backed C++ wrapper exists and covers the basic family.
- **Partial:** analogous C++ compatibility surface exists but does not
  provide LVGL-backed parity behavior.
- **Missing:** no first-party wrapper exists.
- **Optional:** in LVGL source baseline but not required for
  LPAR-CPP-Core.
- **Adjacent:** useful lvglpp-specific surface, not a parity target.

| LVGL source widget | lvglpp status | Current / adjacent surface | Owning phase |
| --- | --- | --- | --- |
| `3dtexture` | Optional | none | LPAR-15 |
| `animimage` | Missing | none | LPAR-15 |
| `arc` | Missing | none | LPAR-11 |
| `arclabel` | Optional | none | LPAR-15 |
| `bar` | Missing | no `Progress` wrapper in lvglpp baseline | LPAR-11 |
| `button` | Partial | `widgets::Button` compatibility widget | LPAR-12 |
| `buttonmatrix` | Missing | none | LPAR-12 |
| `calendar` | Missing | none | LPAR-14 |
| `canvas` | Missing | none | LPAR-15 |
| `chart` | Missing | none | LPAR-14 |
| `checkbox` | Partial | `widgets::Checkbox` compatibility widget | LPAR-12 |
| `dropdown` | Missing | none | LPAR-13 |
| `gif` | Optional | none | LPAR-15 |
| `image` | Partial | `widgets::Image` compatibility widget; RLE decoder | LPAR-08/09 |
| `imagebutton` | Missing | none | LPAR-12 |
| `ime` | Optional | none | LPAR-15 |
| `keyboard` | Missing | none | LPAR-13 |
| `label` | Partial | `widgets::Label` compatibility widget | LPAR-08 |
| `led` | Missing | none | LPAR-11 |
| `line` | Missing | draw helpers only | LPAR-11 |
| `list` | Partial | `widgets::List` compatibility widget | LPAR-13 |
| `lottie` | Optional | none | LPAR-15 |
| `menu` | Missing | none | LPAR-13 |
| `msgbox` | Missing | none | LPAR-14 |
| `property` | Missing | none | LPAR-15 |
| `roller` | Missing | none | LPAR-13 |
| `scale` | Missing | none | LPAR-11 |
| `slider` | Partial | `widgets::Slider` compatibility widget | LPAR-12 |
| `span` | Missing | none | LPAR-14 |
| `spinbox` | Missing | none | LPAR-12 |
| `spinner` | Missing | none | LPAR-11 |
| `switch` | Partial | `widgets::Switch` compatibility widget | LPAR-12 |
| `table` | Missing | none | LPAR-14 |
| `tabview` | Missing | none | LPAR-13 |
| `textarea` | Missing | none | LPAR-14 |
| `tileview` | Missing | none | LPAR-13 |
| `win` | Missing | none | LPAR-13 |
| `objx_templ` | Adjacent | template only | none |

## 10. Reconciliation vs Adjacent Primitives

| Primitive | Relationship |
| --- | --- |
| `WidgetNode` / `Widget` | Compatibility tree. Existing tests remain valid, but new parity wrappers target LVGL objects. |
| `Renderer` | Compatibility renderer and software-test oracle. LVGL-backed display flush is owned by LPAR-03. |
| `core::Style` | Value-type compatibility style. LPAR-07 decides whether to adapt, replace, or keep beside `lv_style_t` wrappers. |
| `playit` | Wire protocol remains first-class. LVGL-backed dispatch is added after object/event wrappers exist. |
| `disco-demo` | App-specific parity surface. It does not make the widget wrappers current. |
| `scripts/lvglpp_qt.py` | Structural helper. C++ emit modes require QT-CPP child chapters. |

## 11. Non-Goals

- No implementation of LPAR-02 through LPAR-16 in this phase.
- No generated C++ state-machine work in this phase.
- No project `lv_conf.h` creation in this phase.
- No claim that compatibility widgets are LVGL-backed parity widgets.

## 12. Acceptance Checklist

- [x] LVGL submodule commit and version macros are pinned.
- [x] rlvgl submodule branch/commit is pinned.
- [x] Config baseline is stated, including absence of project `lv_conf.h`.
- [x] Conformance levels are defined.
- [x] Naming policy is defined.
- [x] Runtime/substrate matrix maps each area to a phase.
- [x] Widget matrix maps each `lvgl/src/widgets` family to a status and
      phase, excluding `objx_templ` as template-only.
- [x] Module `STATUS.md` files cite `rlvgl v0.2.5 @ f999f75` and this
      baseline where applicable.

## 13. Files Cited

- `lvgl/lv_version.h`
- `lvgl/lv_conf_template.h`
- `lvgl/src/lv_conf_internal.h`
- `lvgl/src/widgets/`
- `rlvgl/docs/concepts/LPAR-01-BASELINE.md`
- `core/include/lvglpp/core/`
- `widgets/include/lvglpp/widgets/`
- `ui/include/lvglpp/ui/`
- `platform/include/lvglpp/platform/`
- `playit/include/lvglpp/playit/`
- `examples/apps/disco-demo/`

## 14. Unblocks

- LPAR-CPP-02 object substrate chapter.
- STATUS reconciliation for module pins.
- iState MCP discovery for the C++ target, because the active rlvgl
  baseline is now explicit.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial baseline for lvglpp at LVGL `9.6.0-dev @ ee436e8` and rlvgl `v0.2.5 @ f999f75`. Records current compatibility surfaces and missing LVGL-backed parity wrappers. |
| 2026-06-29 | RATIFIED | Owner accepted the baseline and directed execution to proceed. LPAR-CPP-02 may now draft the LVGL-backed object substrate. |
