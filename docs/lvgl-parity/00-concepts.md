<!--
00-concepts.md - lvglpp mirror plan for rlvgl LPAR/SCTD parity.
-->

# LPAR-CPP-00 - lvglpp LVGL Parity and SCTD Demo Plan

Status: **RATIFIED** (2026-06-29). Normative for the lvglpp mirror of
rlvgl `v0.2.5` LVGL parity and SCTD state-chart demo work.

This chapter adapts the rlvgl parity initiative to lvglpp. rlvgl is the
canonical sibling for behavior and naming intent; upstream LVGL is the C
library lvglpp wraps. The core rule for this plan is: **lvglpp reaches
parity by wrapping LVGL underneath, not by re-implementing a second LVGL
runtime in C++.**

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** are interpreted per RFC 2119 and
RFC 8174 when capitalized.

## 0. Authority Policy

| Concern | Owner | lvglpp relationship |
| --- | --- | --- |
| LVGL reference behavior and object/widget runtime | `lvgl/` submodule | Canonical C implementation. lvglpp parity wrappers MUST delegate lifecycle, events, styles, layouts, invalidation, draw, timers, and widgets to LVGL where LVGL already owns that behavior. |
| rlvgl parity plan | `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md` through `LPAR-16-CONFORMANCE-EXAMPLES-DOCS-RELEASE.md` (`v0.2.5 @ f999f75`) | Canonical cross-language phase order, dependency analysis, and parity surface. lvglpp mirrors the same phase identifiers when implementing the C++ side. |
| rlvgl SCTD plan | `rlvgl/docs/concepts/SCTD-00-CONCEPTS.md`, `SCTD-02-FIREBEETLE-P4-INTERACTIVE.md`, `SCTD-03-SETUP-AND-AUTO-MODE.md` | Canonical demo behavior, target surface, and generated-machine contracts. lvglpp mirrors with C++ state-machine artifacts and LVGL-backed widgets. |
| rlvgl Qt reactive pipeline | `rlvgl/docs/qt-support/00-concepts.md` and QT-05g..05k (`v0.2.5 @ f999f75`) | Canonical IR and binding semantics. lvglpp consumes the same QML/scjson concepts through `scripts/lvglpp_qt.py` and future C++ emitters. |
| iState generation | SoftOboros iState MCP tools | Generation authority for SCTD machines. lvglpp requests C++ output when supported; if C++ output is unavailable, execution blocks rather than hand-writing generated machines. |
| C++ ownership and LVGL handle wrappers | This document and per-phase lvglpp chapters | lvglpp-originated policy. Every `lv_obj_t*`, callback userdata pointer, generated machine pointer, buffer, and ESP-IDF handle MUST carry explicit ownership comments per top-level `AGENTS.md`. |

If this document disagrees with an rlvgl `v0.2.5` LPAR/SCTD normative
decision about behavior, rlvgl wins and this document must be amended.
If it disagrees with LVGL about C object lifecycle, LVGL wins.

## 1. Purpose

Plan the work needed to bring lvglpp to rlvgl `v0.2.5` parity while
preserving lvglpp's purpose as a modern C++ wrapper around upstream
LVGL. The plan covers:

- reconciling current lvglpp modules and docs to the `rlvgl` `v0.2.5`
  pin;
- adding an LVGL-backed C++ wrapper layer for LPAR runtime/widget parity;
- porting the Qt/QML helper workflow to `scripts/lvglpp_qt.py`;
- generating SCTD state machines through SoftOboros iState as C++;
- adding the FireBeetle 2 ESP32-P4 / DFR0550-V2 SCTD demo path using
  ESP-IDF C for hardware and C++ for the lvglpp app payload.

## 2. Problem Statement

The repository currently contains useful C++ mirrors of earlier rlvgl
surfaces: core events, widget-node tests, renderer helpers, basic widgets,
playit command parsing, SDL host smoke paths, and a disco-demo app port.
Most of those docs and cite blocks still point at `rlvgl v0.2.0`.

`rlvgl v0.2.5` is much broader. It includes the LPAR parity wave, QT-05g
through QT-05k reactive state-machine bindings, SCTD demo machines, and a
FireBeetle 2 ESP32-P4 ESP-IDF host. A literal port of rlvgl internals
would be wrong for lvglpp: C++ can and should use real LVGL objects,
styles, layouts, invalidation, timers, and event delivery underneath.

The missing planning decisions are:

- what parts of rlvgl LPAR become thin typed LVGL wrappers versus local
  compatibility adapters;
- how existing non-LVGL test/runtime surfaces coexist during migration;
- which generated artifacts are authoritative and never hand-edited;
- how `scripts/lvglpp_qt.py` feeds C++ output without forking the rlvgl
  QT semantics;
- how the ESP-IDF C host hands framebuffer/input/tick ownership to a
  C++ lvglpp SCTD payload.

## 3. Canonical Glossary

| Term | Definition |
| --- | --- |
| **LPAR** | As defined in `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md`; used without modification for phase scope and ordering. |
| **LPAR mirror** | The lvglpp implementation of an rlvgl LPAR phase. It reuses the `LPAR-NN` identifier and records C++/LVGL adaptations in lvglpp docs. |
| **LVGL-backed wrapper** | A C++ RAII or view type whose behavior is provided by a real LVGL C object or subsystem. Owned by this document; does not exist as a complete layer yet. |
| **`LvObject`** | Planned RAII owner for `lv_obj_t*`. Owns deletion via `lv_obj_delete` or the matching LVGL destroy path. Owned by LPAR-02 mirror. |
| **`ObjectView`** | As defined in `core/include/lvglpp/core/runtime.hpp`; adapted from current external observation into the non-owning view type for LVGL-backed wrappers. |
| **compatibility tree** | Current `lvglpp::core::WidgetNode` + `Widget` retained tree. Used for existing tests and transitional examples; not the long-term LVGL parity runtime. |
| **parity wrapper** | A C++ class such as `Label`, `Button`, `Slider`, or `Table` whose public surface follows C++ idioms while delegating behavior to an LVGL widget. |
| **QT IR** | As defined in `rlvgl/docs/qt-support/02-ir-schema.md`; used without modification as the structural QML interchange shape. |
| **iState machine** | A generated state-machine package emitted by SoftOboros iState. For lvglpp SCTD, the required target language is C++ unless amended. |
| **SCTD demo** | As defined in `rlvgl/docs/concepts/SCTD-00-CONCEPTS.md`; mirrored here as a lvglpp app using C++ generated machines and LVGL-backed UI. |
| **FireBeetle 2 P4 IDF host** | The ESP-IDF C firmware pattern from `rlvgl/examples/beetle-esp32p4-idf/`; lvglpp consumes the same hardware authority and supplies a C++ app payload. |

## 4. Source-of-Truth Map

| Concept | Canonical artifact | lvglpp mirror target |
| --- | --- | --- |
| LPAR phase order and dependencies | `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md` | This document §7 |
| LVGL baseline matrix | `rlvgl/docs/concepts/LPAR-01-BASELINE.md` plus current `lvgl/` pin | `docs/lvgl-parity/01-baseline.md` |
| Object lifecycle | `lvgl/src/core/lv_obj*.c/h`; rlvgl LPAR-02 | `LvObject`, `ObjectView`, callback userdata rules |
| Events/focus/input | LVGL event/input APIs; rlvgl LPAR-04 | typed C++ event adapters over `lv_event_t` / `lv_indev_t` |
| Styles/themes | LVGL style/theme APIs; rlvgl LPAR-07 | C++ style builders/views over `lv_style_t` |
| Draw/text/image/masks | LVGL draw/image/font APIs; rlvgl LPAR-08/09 | LVGL image/font/source wrappers plus existing RLE bridge where needed |
| Layout | LVGL flex/grid APIs; rlvgl LPAR-10 | C++ layout setters, not a custom layout engine |
| Widgets | `lvgl/src/widgets/*`; rlvgl LPAR-11..15 | `lvglpp::widgets` parity wrappers |
| Conformance | rlvgl LPAR-16 fixture ledger | LVGL C smoke vectors + lvglpp wrapper tests |
| QT parser/helper | `scripts/lvglpp_qt.py`; rlvgl QT docs | C++ emitter phases under this family |
| SCTD generated machines | iState MCP output | `examples/apps/sctd-demo/machines/<name>/` C++ artifacts |
| FireBeetle IDF host | rlvgl `examples/beetle-esp32p4-idf` | `examples/firebeetle-esp32p4-idf/` or equivalent lvglpp example |

## 5. Frozen Decisions - Initiative Shape

1. **Reuse rlvgl phase identifiers.** lvglpp implementation commits and
   PRs that mirror LPAR or SCTD behavior SHOULD cite the same phase code
   (`LPAR-02`, `QT-05j`, `SCTD-03`) with a C++ scope note. This keeps the
   cross-language pair searchable.
2. **LVGL underneath.** lvglpp MUST prefer real LVGL APIs for object tree,
   styles, layout, invalidation, timers, input devices, and widgets. A
   local C++ reimplementation is allowed only for host tests,
   compatibility adapters, or behavior LVGL does not provide.
3. **Compatibility surfaces are transitional.** Existing `WidgetNode`,
   `Renderer`, and simple widgets MAY stay for current examples, but new
   LVGL parity claims target LVGL-backed wrappers unless a phase document
   explicitly says otherwise.
4. **Generated artifacts are not hand-authored.** iState-generated
   machines, Qt-generated UI modules, and asset manifests MUST carry
   provenance and regeneration instructions. Local edits belong in
   adapters or externals files, not generated files.
5. **C++ ownership is load-bearing.** Every wrapper storing a raw LVGL or
   generated-machine pointer MUST document `owns`, `borrows`,
   `observes`, `external`, `dma`, or `mmio` adjacent to the declaration.
6. **ESP-IDF C owns hardware.** For FireBeetle 2 P4, ESP-IDF C owns DSI,
   DPI, PSRAM/cache, touch, FreeRTOS tasks, and framebuffer flips. lvglpp
   owns only the app payload and C ABI boundary unless a later board
   chapter changes that.
7. **No behavior PR without its chapter.** Any phase that touches a
   cross-language contract, generated machine API, wire protocol, frozen
   enum, or board ABI needs a ratified per-phase lvglpp chapter first.

## 6. Frozen Decisions - Wave Mapping

| Wave | rlvgl phases | lvglpp goal |
| --- | --- | --- |
| **Wave 0 - Reconciliation** | LPAR-01, status docs | Pin `rlvgl v0.2.5`, update cites/status, record the LVGL baseline, and decide wrapper names. |
| **Wave 1 - LVGL wrapper substrate** | LPAR-02..06 | Add RAII/view wrappers for `lv_obj_t`, events, invalidation/display hooks, input/focus, scroll, timers, and object animations by delegating to LVGL. |
| **Wave 2 - Style/draw/layout wrappers** | LPAR-07..10 | Wrap `lv_style_t`, themes, fonts/images/assets, and LVGL flex/grid APIs. |
| **Wave 3 - Widget parity wrappers** | LPAR-11..15 | Add typed C++ wrappers for the LVGL widget matrix in rlvgl LPAR-01. |
| **Wave 4 - Qt reactive C++ path** | QT-03..05k, QT-06..08c | Teach `scripts/lvglpp_qt.py` and follow-up emitters to output C++ LVGL-backed screen modules and binding tables. |
| **Wave 5 - SCTD C++ demo** | SCTD-00/02/03 | Generate C++ iState machines, port SCTD app adapters, assets, setup screen, and FireBeetle ESP-IDF host payload. |
| **Wave 6 - Conformance/release** | LPAR-16, SCTD tests | Add deterministic wrapper tests, LVGL C comparison smoke tests, generated-machine vectors, simulator/board build gates, and docs. |

## 7. Frozen Decisions - Phase Plan

| Phase | Scope | Depends on | Acceptance evidence |
| --- | --- | --- | --- |
| **LPAR-01 mirror - Baseline and inventory** | Record LVGL submodule commit/version/config, rlvgl `v0.2.5 @ f999f75`, current lvglpp coverage, missing wrappers, and naming policy. | current submodule pin | `docs/lvgl-parity/01-baseline.md`; updated module `STATUS.md` pins. |
| **LPAR-02 mirror - Object substrate** | `LvObject` owner, `ObjectView` observer, parent/child attach/detach, hidden/disabled/clickable flags, userdata lifetime rules. | LPAR-01 | unit tests using real `lv_obj_t`; ownership comments on every raw pointer member. |
| **LPAR-03 mirror - Invalidation/display** | C++ wrapper hooks for `lv_obj_invalidate`, display flush callbacks, dirty-area observation, and current compatibility renderer bridge. | LPAR-02 | host LVGL display test records expected flush rectangles. |
| **LPAR-04 mirror - Events/focus/input** | Typed event adapters over `lv_event_t`; focus group wrappers; pointer/key/encoder input device wrappers; playit injection bridge. | LPAR-02 | event ordering tests against LVGL callbacks and playit command injection. |
| **LPAR-05 mirror - Scroll runtime** | Scroll flags, scrollbars, snap/chaining wrappers, scroll event adapters. | LPAR-03, LPAR-04 | LVGL scroll object tests plus parity notes against rlvgl `ScrollView`. |
| **LPAR-06 mirror - Timers/animations** | RAII wrappers for `lv_timer_t` and `lv_anim_t`; deterministic tick-driving hooks for tests. | LPAR-02, LPAR-04 | synthetic tick tests; no hidden wall-clock dependency in embedded posture. |
| **LPAR-07 mirror - Styles/themes** | `Style` owner/view over `lv_style_t`, selector/state/part mapping, transitions, default theme hooks. | LPAR-02, LPAR-06 | part/state style tests; compatibility notes for existing `core::Style`. |
| **LPAR-08 mirror - Text/draw/image/mask** | Font/image descriptors, text wrapping/metrics exposure, image recolor/transform, masks, draw-task hooks where public LVGL APIs allow. | LPAR-03, LPAR-07 | render buffer tests for label/image primitives and RLE asset handoff. |
| **LPAR-09 mirror - Assets/filesystem** | memory/file/symbol image sources, cache policy, app asset lookup, qrc/qmldir manifest consumption. | LPAR-08 | asset hit/miss tests; deterministic RLE and PNG/JPEG feature gates. |
| **LPAR-10 mirror - Layout** | C++ wrappers for LVGL flex/grid, sizing, align, min/max, content/percent size. | LPAR-02, LPAR-07 | geometry tests against LVGL-computed coordinates. |
| **LPAR-11 mirror - Primitive widgets** | Arc, Bar, LED, Line, Spinner, Scale. | LPAR-07, LPAR-08, LPAR-10 | one wrapper test per widget and docs with LVGL/rlvgl citations. |
| **LPAR-12 mirror - Control widgets** | ButtonMatrix, ImageButton, Spinbox, and reconcile existing Button/Checkbox/Switch/Slider wrappers. | LPAR-04, LPAR-07, LPAR-08 | event + visual smoke tests; migration notes for current non-LVGL wrappers. |
| **LPAR-13 mirror - Selection/navigation widgets** | Dropdown, Keyboard, Menu, Roller, Tabview, Tileview, Window, List reconciliation. | LPAR-04, LPAR-05, LPAR-10 | focus/key/scroll tests; LVGL layout-backed examples. |
| **LPAR-14 mirror - Data/rich widgets** | Calendar, Chart, Span, Table, Textarea v2, MessageBox. | LPAR-04, LPAR-07, LPAR-08, LPAR-10 | text/layout behavior tests and rlvgl API-delta docs. |
| **LPAR-15 mirror - Canvas/media/property/observer** | Canvas, AnimImage, Lottie/3DTexture gates, property/observer wrapper surface needed by generated UI. | LPAR-08, LPAR-09 | feature-gated builds; property lookup tests; generated UI consumer test. |
| **LPAR-16 mirror - Conformance/docs/release** | C-reference smoke vectors, generated examples, no-exception embedded checks, docs/status updates. | all phases feed it | `ctest`, host LVGL fixtures, embedded compile gates, release notes. |
| **QT-CPP-01 - Python helper parity** | Keep `scripts/lvglpp_qt.py` behavior aligned with rlvgl QT IR/schema and add lvglpp-specific C++ output commands only under spec. | LPAR-01 | parser/schema tests and byte-stable manifests. |
| **QT-CPP-02 - C++ widget emitter** | Emit LVGL-backed C++ screen modules from QML structural IR. | LPAR-02, LPAR-07, LPAR-10, QT-CPP-01 | generated C++ compiles and renders a fixed QML fixture. |
| **QT-CPP-03 - Reactive bindings** | Mirror QT-05g..05k in C++: state predicate images, visibility, chained predicates, button event table, external text resolver. | QT-CPP-02, iState C++ linkage | generated binding tables compile and pass media-player interaction tests. |
| **SCTD-CPP-01 - Machine generation** | Generate C++ iState artifacts for media player and interactive Dining Philosophers from the rlvgl SCTD sources/normalizations. | iState C++ MCP support | generated sources, self-manifests, and vector tests are vendored. |
| **SCTD-CPP-02 - App shell** | C++ SCTD app controller, selector `[Setup, DP, MP]`, setup screen, auto model, machine panels, asset provenance. | SCTD-CPP-01, QT-CPP-03, LPAR wrappers | host simulator test selects machines and dispatches events. |
| **SCTD-CPP-03 - FireBeetle IDF host** | ESP-IDF C host plus lvglpp C++ payload for FireBeetle 2 P4 / DFR0550-V2, using C hardware ownership and C++ app logic. | SCTD-CPP-02 | IDF configure/build gate; ABI test for framebuffer/touch/tick handoff. |

## 8. Frozen Decisions - rlvgl LPAR Adaptation Rules

| rlvgl LPAR concern | lvglpp adaptation |
| --- | --- |
| Object substrate | Use `lv_obj_t` as the actual object. C++ wrappers own or view it explicitly. |
| Invalidation/display | Use LVGL invalidation and display drivers; add observers/tests rather than a competing dirty planner. |
| Event propagation | Use LVGL event delivery; C++ adapters translate to typed callbacks and playit fixtures. |
| Focus/input | Use LVGL groups and input devices; preserve playit command parity at the injection boundary. |
| Scroll | Use LVGL scroll flags/events. Existing `ScrollView` compatibility tests remain until migrated. |
| Timers/animations | Use `lv_timer_t` / `lv_anim_t` with test-controlled ticks. |
| Style/theme | Use `lv_style_t` and LVGL states/parts. Existing `core::Style` is either adapted or documented as legacy. |
| Text/draw/image | Use LVGL draw/font/image primitives; keep RLE as an asset source feeding LVGL-compatible image data. |
| Layout | Use LVGL flex/grid, not a separate C++ solver for parity widgets. |
| Widgets | Wrap LVGL widgets directly. Existing hand-drawn widgets are compatibility or app-specific unless migrated. |
| Conformance | Compare wrapper-visible behavior to LVGL and rlvgl contracts, not to private LVGL internals. |

## 9. Frozen Decisions - SCTD / iState Boundary

1. **C++ target required.** SCTD-CPP machine artifacts MUST come from
   SoftOboros iState with a C++ target. If the MCP surface does not
   expose C++ generation, SCTD-CPP-01 is blocked and the blocker is
   recorded; handwritten replacement machines are not acceptable. If the
   C++ target generates but the output fails to compile, fails vectors, or
   does not function correctly under the SCTD adapter contract, lvglpp
   records a local iState errata note for handoff, but that errata note
   MUST NOT be committed to this repository. SoftOboros owns resolving
   the iState generator errata before SCTD-CPP-01 can proceed.
2. **Same source machines.** The initial machine set mirrors rlvgl
   `v0.2.5`: interactive Dining Philosophers and normalized Media Player.
   Their source/provenance files are the rlvgl SCTD machine sources unless
   a later cross-language amendment changes them.
3. **Linkage surface is frozen before adapters.** The generated C++ API
   names for `start`, `step`, `is_active`, `get_var`, active-state
   listing, and value representation MUST be documented in SCTD-CPP-01
   before app code calls them.
4. **Adapters own app vocabulary.** MediaFunc-to-machine-event mapping,
   external text resolvers, DP auto cadence, and setup defaults live in
   hand-authored adapters, not generated files.
5. **Vectors required.** Each generated machine MUST carry deterministic
   vector tests equivalent to rlvgl's machine vectors before the app
   selector exposes it.

## 10. Reconciliation vs Current lvglpp Primitives

| Current primitive | Plan |
| --- | --- |
| `lvglpp::Runtime` | Keep as the process-wide `lv_init` RAII guard; extend to initialize display/input wrappers as needed. |
| `ObjectView` | Retain as the non-owning LVGL handle view; tighten ownership comments and lifetime rules in LPAR-02. |
| `WidgetNode` / `Widget` / `Renderer` | Keep for existing examples and tests. Do not claim new LVGL widget parity through this path unless a phase chapter explicitly accepts a compatibility bridge. |
| Existing `widgets::Label/Button/...` | Reconcile in LPAR-12/13. Either migrate to LVGL-backed wrappers or label as legacy compatibility classes with new parity wrappers beside them. |
| `playit` parser/dispatcher | Keep wire protocol parity. Add an LVGL-backed dispatcher path once object tags and event injection wrappers exist. |
| `examples/apps/disco-demo` | Treat as existing app parity work. Update cites to `v0.2.5` where behavior still matches; do not block LVGL wrapper substrate on disco-specific code. |
| `scripts/lvglpp_qt.py` | Source for Python-side Qt/QML structural tooling. It must remain app-agnostic; generated SCTD-specific meanings belong in adapters. |

## 11. Non-Goals

- No C ABI clone of LVGL in C++.
- No broad rewrite of LVGL internals.
- No replacement of rlvgl; lvglpp and rlvgl remain sibling choices.
- No handwritten stand-in for iState-generated SCTD machines.
- No FireBeetle raw-PAC bring-up in this plan; the requested board path is
  ESP-IDF C plus lvglpp C++ app payload.
- No promise that existing legacy hand-drawn widgets disappear in the
  first parity wave.

## 12. Acceptance Checklist

This plan is accepted when:

- [x] `rlvgl` is pinned to `v0.2.5` and `.gitmodules` tracks that branch.
- [x] LPAR-01 through LPAR-16 mirrors are listed with lvglpp-specific
      adaptation rules.
- [x] The LVGL-underneath policy is frozen.
- [x] QT and SCTD follow-up phases are listed before code generation.
- [x] The iState C++ generation blocker/requirement is explicit.
- [x] FireBeetle 2 P4 is scoped to ESP-IDF C hardware ownership plus
      C++ app payload ownership.
- [x] Module `STATUS.md` files are updated to reference the new
      `rlvgl v0.2.5` reconciliation plan.

Implementation phases are accepted only when their own chapters define:

- [ ] exact LVGL APIs wrapped and their ownership roles;
- [ ] rlvgl source paths and line citations for mirrored behavior;
- [ ] tests, fixture kind, and embedded-posture expectations;
- [ ] generated-file provenance and regeneration commands where relevant.

## 13. Files Cited

- `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md`
- `rlvgl/docs/concepts/LPAR-01-BASELINE.md`
- `rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md`
- `rlvgl/docs/concepts/LPAR-16-CONFORMANCE-EXAMPLES-DOCS-RELEASE.md`
- `rlvgl/docs/concepts/SCTD-00-CONCEPTS.md`
- `rlvgl/docs/concepts/SCTD-02-FIREBEETLE-P4-INTERACTIVE.md`
- `rlvgl/docs/concepts/SCTD-03-SETUP-AND-AUTO-MODE.md`
- `rlvgl/docs/qt-support/05g-state-predicate-bindings.md`
- `rlvgl/docs/qt-support/05h-visibility-bindings.md`
- `rlvgl/docs/qt-support/05i-chained-predicate-bindings.md`
- `rlvgl/docs/qt-support/05j-button-event-bindings.md`
- `rlvgl/docs/qt-support/05k-external-text-bindings.md`
- `rlvgl/examples/apps/sctd-demo/`
- `rlvgl/examples/beetle-esp32p4-idf/`
- `scripts/lvglpp_qt.py`
- `lvgl/`

## 14. Unblocks

- `LPAR-01 mirror`: baseline matrix and status-file reconciliation.
- `QT-CPP-01`: tests around `scripts/lvglpp_qt.py` before adding C++
  emit modes.
- iState MCP discovery for C++ code generation.
- SCTD-CPP-01 once the iState C++ target is confirmed.

Current SCTD-CPP blocker: this Codex session does not expose
SoftOboros/iState MCP tools (`istate_upload_xml`,
`istate_codegen_create`, `istate_codegen_status`,
`istate_codegen_download`) and no installable SoftOboros/iState plugin is
listed. SCTD-CPP-01 remains blocked until those tools are available and a
C++ target is confirmed.

## 15. Change Log

| Date | Status | Note |
| --- | --- | --- |
| 2026-06-29 | DRAFT | Initial lvglpp mirror plan for rlvgl `v0.2.5` LPAR/SCTD parity. Freezes the LVGL-underneath rule, phase mapping, iState C++ generation requirement, and ESP-IDF C + C++ payload boundary for FireBeetle 2 P4. |
| 2026-06-29 | RATIFIED | Owner accepted the plan and directed execution to proceed. Added iState C++ errata handling: failed/incorrect C++ generation produces a local, uncommitted handoff note; SoftOboros owns generator resolution before SCTD-CPP-01 proceeds. |
| 2026-06-29 | BLOCKER RECORDED | Tool discovery in this Codex session did not expose `istate_*` MCP tools or an installable SoftOboros/iState plugin. SCTD-CPP-01 remains blocked until C++ iState generation is available. |
