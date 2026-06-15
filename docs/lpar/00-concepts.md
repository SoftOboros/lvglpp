# 00 — LVGL parity initiative concepts

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-00**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact for the LPAR initiative shape
(waves, phase order, dependency gates, conflict policy, and the
wrap-not-reimplement rule). The initiative [`README.md`](./README.md) is
informative.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| LVGL widget/runtime **primitive behavior** | `lvgl/` submodule (9.6.0-dev @ `ee436e8`) | Canonical for what `lv_*` actually does. LPAR-01 pins the exact commit + version before any phase claims parity. |
| Cross-language **semantic contract** (widget surface, event ordering, tick behavior, naming intent) | rlvgl `v0.2.4` concepts docs (`rlvgl/docs/concepts/LPAR-*.md`, `FONT-*.md` @ `343f596`) | Canonical for the rlvgl/lvglpp pair. lvglpp mirrors the *observed behavior*, not the Rust code shape. |
| C++ **surface naming, ownership tags, RAII, freestanding-subset rules** | this chapter + `docs/std-mapping.md` + CLAUDE.md | Normative for lvglpp. |
| Existing lvglpp surfaces (`CORE-*`, `WID-01..06`, `UI-*`, `PLAYIT-*`, `I18N-*`, `PLAT-*`) | this repo | Repo is canonical for current API. LPAR/`LVGLPP-WRAP` phases extend or migrate them only through explicit phase gates (§9). |

If an LPAR implementation phase changes a frozen decision in this
document, the §15 change log MUST be amended first in a separate docs
change. Unresolvable phase conflicts get a sub-letter doc
`docs/lpar/00-X.md` folded back into this chapter before code lands.

## §1 Purpose

Bring lvglpp from its current **rlvgl v0.2.0** mirror up to **rlvgl
v0.2.4** behavioral parity — the LPAR (LVGL parity) substrate + ~30
widgets and the FONT (font-selection + anti-aliased text) family —
**by wrapping upstream LVGL `lv_*` calls under RAII**, and unify the
existing hand-rolled lvglpp object model onto the same `lv_obj_t*`
foundation.

The goal is not to clone rlvgl's Rust internals. It is to make lvglpp's
C++ API cover the same user-visible widget and runtime behavior, with the
ownership story carried over, on top of the real LVGL library the project
already vendors and links.

## §2 Problem statement

Evidence in the current tree:

- lvglpp's own library code calls **zero** `lv_*` functions
  (`git grep 'lv_[a-z_]*(' core widgets ui platform playit i18n include`
  returns nothing). The CMake at `CMakeLists.txt:108` does
  `add_subdirectory(lvgl …)`, so LVGL compiles and links, but the core
  (`core/include/lvglpp/core/{widget,renderer,draw_helpers}.hpp`) and the
  eight widgets (`widgets/include/lvglpp/widgets/*.hpp`) are hand-rolled
  C++ that re-implement rlvgl's Rust architecture. "Wrapper around
  upstream LVGL" is, today, aspirational.
- lvglpp tracks rlvgl **v0.2.0**: `CORE-01..07n`, `WID-01..06` (Label,
  Button, Checkbox, Switch, Slider, Container, List, Image), `UI-01/02`,
  full `PLAYIT-*`, `I18N-01/02`, `PLAT-01/02/LNX`.
- rlvgl **v0.2.4** (pinned at `rlvgl/` `343f596`) adds two large
  families absent from lvglpp: **LPAR** (16 phases: object/invalidation/
  event/focus/input/scroll/timers/style/text/draw/image/mask/asset/
  layout substrate, then Arc/Bar/LED/Line/Scale/Spinner,
  ButtonMatrix/ImageButton/Spinbox, Dropdown/Keyboard/Menu/Roller/
  Tabview/Tileview/Window, Calendar/Chart/MessageBox/Span/Table/Textarea,
  Canvas/AnimImage/ArcLabel/property/observer, conformance) and **FONT**
  (6 phases: `WidgetFont` selection, AA text, ArcLabel migration,
  rotated-glyph throughput, font registry + cascade→widget bridge), plus
  single-phase `ANIM-00`/`REND-00`/`INPUT-00`/`WID-00`.
- Because rlvgl re-implements LVGL while lvglpp wraps it, a 1:1 port of
  rlvgl's substrate phases would re-create runtime LVGL already ships.
  The parity contract is **behavior**, the implementation is **`lv_*`
  wrappers**, and the two object models (hand-rolled vs `lv_obj`) cannot
  be allowed to fork — they must unify.

## §3 Canonical glossary

For each term, the form follows CLAUDE.md
§ "Definitions — reference vs. restatement".

- **LPAR** — The LVGL parity initiative family. As defined in
  `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md` (v0.2.4 @ `343f596`);
  mirrored here as this chapter. lvglpp adopts the same phase/wave
  codes (`LPAR-01..16`) for the cross-language pair.
- **FONT** — The font-selection + anti-aliased-text family. As defined
  in `rlvgl/docs/concepts/FONT-00-CONCEPTS.md` (v0.2.4 @ `343f596`);
  mirrored under `docs/font/` as it lands. Phase codes `FONT-00..05`.
- **`LVGLPP-WRAP`** — Owned by this chapter; lvglpp-internal, does not
  exist in rlvgl. The migration initiative that (a) stands up the RAII
  `lv_obj` core and (b) ports the existing hand-rolled `WID-01..06`
  widgets + platform renderers onto it. Lives at `docs/wrap/`. Uses the
  `LVGLPP-WRAP-NN[a-z]:` commit prefix per CLAUDE.md § "Execution
  discipline".
- **Wrap-not-reimplement** — Owned by this chapter. The rule that LPAR
  substrate phases expose RAII C++ over existing `lv_*` primitives rather
  than re-deriving runtime behavior in C++. See §5.
- **Object substrate** — Object-tree behavior equivalent to LVGL
  `lv_obj`: parent/child ownership, flags, hit-testing, invalidation,
  focus, events, lifecycle. As defined in
  `rlvgl/docs/concepts/LPAR-02-OBJECT-SUBSTRATE.md`; mirrored in lvglpp
  as RAII wrappers over `lv_obj_*` (chapter `docs/core-object/`,
  LPAR-02/03).
- **Parity widget** — A widget whose documented behavior maps to an LVGL
  widget family. In lvglpp it is a RAII handle owning an `lv_obj_t*` from
  `lv_<widget>_create`. As defined per-widget in
  `rlvgl/docs/concepts/LPAR-1{1..5}-*.md`.
- **`WidgetFont`** — Per-widget font selection type. As defined in
  `rlvgl/docs/concepts/FONT-00-CONCEPTS.md`; mirrored in lvglpp wrapping
  `lv_obj_set_style_text_font` + a `FontRegistry` indirection (FONT
  family).
- **Wrapper collision** — A conflict between an existing lvglpp
  name/API and an LVGL parity name/API. As defined in
  `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md:79`. lvglpp's load-bearing
  instance is the **WID namespace collision** (below).
- **WID namespace reconciliation** — Owned by this chapter. lvglpp
  already uses `WID-01..06` for its own widgets; rlvgl uses `WID-00` for
  its editable-input initiative. The two are distinct. lvglpp does NOT
  reuse `WID-00`; rlvgl's WID-00 editable-text semantics are folded into
  the lvglpp **LPAR-14 Textarea** chapter. See §9.
- **playit↔lv_obj bridge** — Owned by `LVGLPP-WRAP`; does not exist yet.
  The adapter that re-points the `playit` `Dispatcher` (currently walking
  the hand-rolled `WidgetNode` tree for `QB`/`QE`/`QC`/`T@<tag>`) onto
  the `lv_obj` tree, preserving byte-identical wire responses.
- **Conformance fixture** — A deterministic test / golden image /
  geometry assertion proving a parity claim. As defined in
  `rlvgl/docs/concepts/LPAR-16-CONFORMANCE-EXAMPLES-DOCS-RELEASE.md`;
  mirrored under `docs/lpar-conformance/` (LPAR-16) and driven through
  the shared `playit` harness.

## §4 Source-of-truth map

| Concept | Owner | Mirror / consumer sites |
| --- | --- | --- |
| Initiative phase order, wave gates, conflict policy | this chapter | `docs/wrap/`, per-phase chapters |
| LVGL primitive behavior + version pin | `lvgl/` submodule; LPAR-01 records the pin | every `lv_*` wrapper |
| Per-widget / per-substrate semantic contract | rlvgl `v0.2.4` `LPAR-*` / `FONT-*` concepts docs | lvglpp per-phase chapters; `// PARITY:` cite blocks |
| C++ ownership / RAII / naming | this chapter + CLAUDE.md § "Strict and Explicit Ownership" | every `.hpp`/`.cpp` |
| Object-model unification | `LVGLPP-WRAP` (`docs/wrap/`) | existing `CORE-*`/`WID-*`, `playit`, `platform` |
| Shared-contract extension | this chapter / the owning phase chapter — **Standards Action** | rlvgl + lvglpp PR pair, change log amended first |
| Phase acceptance evidence | per-phase implementation PRs + LPAR-16 fixtures | `ctest`, shared `playit` fixtures |

## §5 Frozen decisions — initiative shape

1. **Wrap, do not re-implement.** LPAR substrate and widget phases MUST
   implement parity by calling upstream `lv_*` under RAII. Re-deriving
   LVGL runtime behavior in C++ is OUT OF SCOPE except where a documented
   DELTA in the owning phase chapter justifies it (e.g. a behavior LVGL
   does not provide).
2. **One object model.** There MUST be a single `lv_obj_t*`-backed object
   model. The existing hand-rolled `Widget`/`WidgetNode`/`Renderer`/
   `draw_*` layer is migrated onto it under `LVGLPP-WRAP` and retired;
   no new phase may add a second parallel tree.
3. **Baseline before behavior.** LPAR-01 MUST land (pin LVGL commit +
   version, naming policy, conformance levels, current/partial/missing
   matrix) before any implementation phase claims parity.
4. **Substrate before broad widgets.** Widget phases that need object,
   style, layout, text, draw, focus, or scroll behavior MUST wait for the
   owning substrate wrapper phase or document a narrower v1 contract.
5. **`lv_conf.h` is load-bearing.** Each wrapped feature MUST enable its
   `LV_USE_*` / `LV_FONT_*` symbol in `include/lvglpp/lv_conf.h`, tracked
   in the owning module's `OPTIONS.md`.
6. **Ownership is explicit and LVGL-aware.** A wrapper that *owns* an
   `lv_obj_t` deletes it via `lv_obj_delete` in its destructor; once an
   object is *attached* to a parent, the parent owns it (LVGL deletes
   children with parents). Every wrapper MUST document which case it is
   (CLAUDE.md ownership tags). Double-free and use-after-`lv_obj_delete`
   are the dominant failure modes; `LVGLPP-WRAP` freezes the rule.
7. **Embedded posture preserved.** Every wrapper MUST compile under
   `LVGLPP_EMBEDDED_POSTURE=ON` (`-fno-exceptions -fno-rtti`); throwing
   construction paths `std::abort()`.
8. **playit parity preserved.** After the `playit`↔`lv_obj` bridge, the
   wire responses for `?`, `T`, `QB`/`QE`/`QC`, `T@<tag>`, `D`, and the
   recorder MUST stay byte-identical to rlvgl. The bridge is a gate, not
   a rewrite of the protocol.
9. **Existing ratified initiatives inherited.** `ANIM-00`, `REND-00`,
   `INPUT-00`, `WID-00` (rlvgl) are mirrored where they add behavior LVGL
   does not already provide; where LVGL provides the behavior
   (`lv_anim`, clip layers, scroll), the lvglpp mirror is the `lv_*`
   wrapper, and the rlvgl single-phase initiative folds into the relevant
   substrate chapter.
10. **Phase-scoped parity claims.** No PR may claim "LVGL parity"
    generically; each MUST cite a specific phase code and the subset of
    the LPAR-01 matrix it closes.

## §6 Frozen decisions — waves

| Wave | Phases | Goal | Parallelism rule |
| --- | --- | --- | --- |
| **0 — Pivot & baseline** | `LVGLPP-WRAP-00`, LPAR-01 | RAII `lv_obj` core (`Handle<T>`/`Object`/`Screen`), `lv_conf.h` baseline, pin + naming + conformance matrix. Additive — removes nothing. | Serial; gates everything. |
| **1 — Substrate wrappers** | LPAR-02..10 | RAII over `lv_obj`/`event`/`group`/`indev`/`scroll`/`timer`/`anim`/`style`/`theme`/`draw`/`font`/`fs`/`flex`/`grid`. | LPAR-02 first; LPAR-03/04/05/07/08/10 parallel once `Object` is fixed. |
| **2 — Font** | FONT-00..05 | `WidgetFont` selection, `FontRegistry`, AA text, cascade→widget bridge. | After LPAR-08 font wrapper. |
| **3 — Migrate** | `LVGLPP-WRAP-01..0N` | Port `WID-01..06` onto `Object`; `playit`↔`lv_obj` bridge; SDL/fbdev/disco → `lv_display`+`lv_indev`. | Widget-by-widget; `ctest`+`playit` green at each step; old layer retired last. |
| **4 — Primitive widgets** | LPAR-11 | Arc, Bar, LED, Line, Scale, Spinner. | Parallel by widget (separate files, settled substrate). |
| **5 — Control / Nav / Data** | LPAR-12, LPAR-13, LPAR-14 | ButtonMatrix/ImageButton/Spinbox → Dropdown/Keyboard/Menu/Roller/Tabview/Tileview/Window → Calendar/Chart/MessageBox/Span/Table/Textarea(+WID-00). | Parallel after each widget's declared deps land. |
| **6 — Canvas/media/observer** | LPAR-15, ANIM-00, REND-00, INPUT-00 | Canvas, AnimImage, ArcLabel, property/observer; fold tick-anim, clip/scrollview, drag-recognizer into substrate chapters. | After waves 1–5. |
| **7 — Conformance** | LPAR-16 | Determinism / geometry / pixel goldens via the shared `playit` fixtures. | Continuous; each phase incomplete until its LPAR-16 evidence lands. |

## §7 Frozen decisions — phase plan

Phase codes reuse the rlvgl prefixes (cross-language pair). `lv_*`
primitive is the upstream surface each lvglpp phase wraps.

| Phase | Wave | Scope | Wraps (`lv_*`) | Depends on | Conflict gates |
| --- | --- | --- | --- | --- | --- |
| **LVGLPP-WRAP-00** | 0 | RAII `lv_obj` core: `Handle<T>`, `Object`, `Screen`; ownership rule; `lv_conf.h` baseline. | `lv_obj_create`, `lv_obj_delete`, `lv_screen_active` | — | Coexists with hand-rolled core (additive); no removals yet. |
| **LPAR-01** | 0 | Pin LVGL commit/version; naming policy; current/partial/missing matrix; conformance levels. | — | WRAP-00 | `Progress`/`Bar`, `Textarea`, `MessageBox`, WID code collision (§9). |
| **LPAR-02** | 1 | Object substrate: flags, states, hit-test, lifecycle, parent/child. | `lv_obj_*`, `lv_obj_add/clear_flag`, `lv_obj_add/remove_state` | WRAP-00, LPAR-01 | Hand-rolled `WidgetNode` retirement order. |
| **LPAR-03** | 1 | Invalidation & display: dirty rects, flush, refresh. | `lv_obj_invalidate`, `lv_display_*` | LPAR-02 | Platform renderer → `lv_display` migration (WRAP). |
| **LPAR-04** | 1 | Event/focus/input: codes, bubbling, focus groups, indev. | `lv_event_*`, `lv_group_*`, `lv_indev_*` | LPAR-02 | CORE-02 `Event` enum vs `lv_event_code_t`; playit injection path. |
| **LPAR-05** | 1 | Scroll: flags, scrollbars, snap, momentum/throw. | `lv_obj_scroll_*`, `lv_obj_set_scroll_snap_*` | LPAR-03, LPAR-04 | REND-00 ScrollView; INPUT-00 drag suppression. |
| **LPAR-06** | 1 | Timers & object animations. | `lv_timer_*`, `lv_anim_*` | LPAR-02, ANIM-00 | CORE-05 `Easing`/`LoopMode` vs `lv_anim` paths. |
| **LPAR-07** | 1 | Style cascade & theme: parts, states, selectors, inherit. | `lv_style_*`, `lv_obj_set_style_*`, `lv_theme_*` | LPAR-02 | CORE-05 `Style`/`Theme` overlap; ownership/re-export. |
| **LPAR-08** | 1 | Text/draw/image/mask: glyph metrics, wrap, draw prims, image. | `lv_draw_*`, `lv_font_*`, `lv_image_*`, draw layers/masks | LPAR-03, LPAR-07 | CORE-04/04a renderer + draw-helper retirement; CORE-06 font. |
| **LPAR-09** | 1 | Asset & filesystem sources. | `lv_fs_*`, image decoders | LPAR-08 | CORE-07 plugin slots; RLE decoder (CORE-07n). |
| **LPAR-10** | 1 | Layout: sizing, flex, grid. | `lv_obj_set_flex_*`, `lv_obj_set_grid_*`, size helpers | LPAR-02, LPAR-07 | Supersedes stubbed UI-04 layout helpers. |
| **FONT-00..05** | 2 | `WidgetFont` selection, registry, AA, cascade bridge. | `lv_obj_set_style_text_font`, `lv_font_t`, `lv_freetype`/`lv_tiny_ttf` seam | LPAR-08 | CORE-06 bring-up font; embedded vs host font loading. |
| **LVGLPP-WRAP-01..0N** | 3 | Migrate WID-01..06 onto `Object`; playit↔lv_obj bridge; platform → `lv_display`/`lv_indev`. | per migrated widget + `lv_display_*`/`lv_indev_*` | Wave 1 | Touches passing playit fixtures + disco firmware (verification-blocked). |
| **LPAR-11** | 4 | Arc, Bar, LED, Line, Scale, Spinner. | `lv_arc_*`, `lv_bar_*`, `lv_led_*`, `lv_line_*`, `lv_scale_*`, `lv_spinner_*` | Wave 1, Wave 3 | `Progress`/`Bar`; meter overlap. |
| **LPAR-12** | 5 | ButtonMatrix, ImageButton, Spinbox. | `lv_buttonmatrix_*`, `lv_imagebutton_*`, `lv_spinbox_*` | LPAR-04/07/08 | Existing Button/Checkbox/Switch routing. |
| **LPAR-13** | 5 | Dropdown, Keyboard, Menu, Roller, Tabview, Tileview, Window. | `lv_dropdown_*`, `lv_keyboard_*`, `lv_menu_*`, `lv_roller_*`, `lv_tabview_*`, `lv_tileview_*`, `lv_win_*` | LPAR-04/05/07/10 | Focus/input conflicts; overlay/modal patterns. |
| **LPAR-14** | 5 | Calendar, Chart, MessageBox, Span, Table, Textarea (absorbs rlvgl WID-00). | `lv_calendar_*`, `lv_chart_*`, `lv_msgbox_*`, `lv_span_*`, `lv_table_*`, `lv_textarea_*` | LPAR-04/07/08/10 | WID code collision (§9); text metrics. |
| **LPAR-15** | 6 | Canvas, AnimImage, ArcLabel, property/observer. | `lv_canvas_*`, `lv_animimg_*`, `lv_arclabel_*`/draw; observer via `lv_subject_*` | LPAR-08/09 | Plugin boundaries; playit introspection ownership. |
| **LPAR-16** | 7 | Conformance fixtures, examples, docs, release. | — (test harness) | LPAR-01; each phase feeds it | Visual determinism; DMA2D vs software pixels on disco. |

## §8 Dependency analysis

| Dependency | Why it matters | Blocks |
| --- | --- | --- |
| WRAP-00 `Object` before all wrappers | Every `lv_*` wrapper needs the RAII handle + ownership rule. | LPAR-02..16, FONT |
| LPAR-01 before any parity claim | Without a pinned LVGL commit/version, "parity" is unbounded. | all implementation phases |
| LPAR-02 before event/style/layout/widgets | Flags/states/parentage are the shared vocabulary of every widget. | LPAR-03..15 |
| LPAR-08 text/font before text-heavy widgets | Label/Textarea/Span/Table/Calendar/Chart need consistent metrics. | FONT, LPAR-13/14 |
| Wave-3 migration before retiring hand-rolled core | playit + platform must move to `lv_obj`/`lv_display` before the old `Renderer`/`WidgetNode` can be deleted. | hand-rolled core removal |
| LPAR-16 fixtures throughout | Each phase needs evidence before "done". | all phases |

## §9 Conflict analysis

| Conflict | Risk | Resolution policy |
| --- | --- | --- |
| **Two object models** (hand-rolled `WidgetNode` vs `lv_obj`) | A wrapped `lv_obj` cannot be a child of a hand-rolled `WidgetNode`; the playit Dispatcher and disco `DiscoRenderer` walk the old tree. | `LVGLPP-WRAP` migrates onto `lv_obj` widget-by-widget; the old tree is retired only after the last widget + playit bridge + platform display move. No phase adds a third model. |
| **WID code collision** (lvglpp `WID-01..06` vs rlvgl `WID-00`) | Reusing `WID-00` in lvglpp would alias an existing widget series and fork vocabulary. | lvglpp does NOT use `WID-00`. rlvgl's WID-00 editable-input semantics fold into the **LPAR-14 Textarea** chapter; the lvglpp `WID-NN` series stays its own (migrated under `LVGLPP-WRAP`). |
| **playit fixture parity** | Re-pointing `QB`/`QE`/`QC`/`T@<tag>` to `lv_obj` could drift wire bytes. | The bridge maps tag → `lv_obj` name/user-data and preserves response formatting; LPAR-16 fixtures gate byte-identity vs rlvgl. |
| **Platform display re-architecture** | SDL/fbdev/disco renderers draw a custom tree today; LVGL drives its own `lv_display` flush. Disco LTDC is verification-blocked. | WRAP migrates renderers to `lv_display_t` flush + `lv_indev_t`; disco glue lands but is verified blind (probe-rs + playit `D` dumps); host SDL/fbdev/sim carry correctness sign-off. |
| **CORE-05 `Style`/`Easing` vs `lv_style`/`lv_anim`** | Existing value-type style/easing overlaps the LVGL cascade. | LPAR-06/07 decide whether CORE-05 types become thin views over `lv_style_t`/`lv_anim_t` or are deprecated; named in those chapters before code. |
| **CORE-04/04a/06 renderer+draw+font vs `lv_draw`/`lv_font`** | The hand-rolled `Renderer`/`draw_widget_bg`/`BitmapFont` are superseded by LVGL's pipeline. | LPAR-08 owns the retirement; FONT owns the font-selection path; the bring-up `FONT_6X10` becomes an `lv_font_t` or is retired. |
| **`Progress`/`Bar`, `Modal`/`MessageBox` naming** | Parity widget names may overlap future lvglpp app widgets. | LPAR-01 naming policy resolves alias-vs-wrapper-vs-new-module before LPAR-11/14 code. |
| **`no_std`/embedded footprint vs LVGL feature breadth** | Some widgets (Chart, Canvas, FreeType fonts) pull heavier `lv_conf.h` features. | Each phase declares its `LV_USE_*`/`LV_FONT_*` footprint in `OPTIONS.md`; heavy features stay opt-in. |

## §10 Reconciliation vs. adjacent lvglpp primitives

| Primitive | Relationship |
| --- | --- |
| `CORE-01` `Runtime` (wraps `lv_init`) | Already the one true LVGL bootstrap. `Object`/`Screen` (WRAP-00) build on it; no change to its single-instance contract. |
| `CORE-02` `Event`/`Key`/`TouchState` | Stays the canonical input value type. LPAR-04 maps it to/from `lv_event_t`/`lv_indev` at the driver seam; the enum stays Standards Action. |
| `CORE-03/03a/04/04a/06` hand-rolled core | Superseded by `lv_obj`/`lv_draw`/`lv_font` wrappers; retired under `LVGLPP-WRAP` + LPAR-02/08 once migration completes. |
| `WID-01..06` widgets | Migrated onto `Object` (WRAP); their behavior is preserved and validated against the same playit fixtures. |
| `UI-01/02` draw helpers + EventWindow; stubbed UI-03/04 | UI-04 layout helpers are superseded by LPAR-10 (`lv_flex`/`lv_grid`). UI-01/02 reconcile against `lv_draw`/event wrappers. |
| `PLAYIT-*` harness | Unchanged wire protocol; gains the `lv_obj` query bridge. The cross-language fixtures are the parity gate for the whole initiative. |
| `I18N-01/02` (RLTN + `lv_translation` bridge) | Orthogonal; FONT-family glyph selection must not regress the i18n label-tag re-resolution path. |
| `PLAT-01/02/LNX` backends | Become `lv_display_t`/`lv_indev_t` providers under WRAP. Disco LTDC remains the verification-blocked prong. |

## §11 Non-goals

- No C ABI compatibility with LVGL; lvglpp wraps the C API, it does not
  re-export it.
- No re-implementation of LVGL runtime behavior in C++ where `lv_*`
  already provides it (that is rlvgl's job, not lvglpp's).
- No second object model; no parallel widget tree.
- No `creator-cpp` work — asset generation stays the rlvgl-creator path
  (CLAUDE.md § "`creator-cpp` is deferred").
- No promise that every heavy LVGL widget (full Lottie, 3DTexture) ships
  in the embedded feature set; those are optional conformance levels.
- No breaking change to the `playit` wire protocol.

## §12 Acceptance checklist

LPAR-00 is accepted when:

- [ ] The wrap-not-reimplement rule (§5.1) and one-object-model rule
      (§5.2) are stated and frozen.
- [ ] `LVGLPP-WRAP`, the WID namespace reconciliation, and the
      playit↔lv_obj bridge are defined with owners.
- [ ] LPAR-01..16 and FONT-00..05 are listed with `lv_*` mappings,
      dependencies, and conflict gates (§7).
- [ ] Each rlvgl v0.2.4 initiative area (LPAR/FONT/ANIM/REND/INPUT/WID)
      maps to at least one lvglpp phase or a documented fold.
- [ ] [`README.md`](./README.md) lists LPAR as an active draft
      initiative.

Individual implementation phases are accepted only when their phase
chapters define: scope/non-goals, the `lv_*` surface wrapped, ownership
tags, `LV_USE_*` footprint, dependency prerequisites, conflict
resolutions, conformance fixtures (incl. embedded-posture build), and
`STATUS.md` change-log updates.

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-00-CONCEPTS.md` (v0.2.4 @ `343f596`) — rlvgl LPAR initiative shape
- `rlvgl/docs/concepts/LPAR-0{2..10}-*.md`, `LPAR-1{1..6}-*.md` — substrate + widget contracts
- `rlvgl/docs/concepts/FONT-00-CONCEPTS.md`, `FONT-05-FONT-REGISTRY.md` — font family
- `lvgl/lv_version.h` (9.6.0-dev @ `ee436e8`) — wrapped primitive version
- `CMakeLists.txt:108` — `add_subdirectory(lvgl …)` (LVGL linked, not yet called)
- `core/include/lvglpp/core/{widget,renderer,draw_helpers,font}.hpp` — hand-rolled core to migrate
- `widgets/include/lvglpp/widgets/*.hpp` — WID-01..06 to migrate
- `playit/include/lvglpp/playit/{dispatcher,parser,conversion}.hpp` — bridge target
- `CLAUDE.md` § "Strict and Explicit Ownership", § "Spec-Before-Code Planning Discipline", § "Doc Co-Location Policy", § "Cross-language change ordering"
- `docs/lpar/README.md`, `docs/lpar/01-baseline.md`

## §14 Unblocks

- **LPAR-01** — pins the LVGL commit/version, naming policy, and
  conformance matrix this chapter assumes.
- **`LVGLPP-WRAP-00`** — the RAII `lv_obj` core every later phase wraps.
- **All LPAR-02..16 / FONT-00..05 chapters** — each cites this chapter's
  wave/phase plan and conflict gates.

## §15 Change log

- **2026-06-15** — LPAR-00 drafted. Reframes rlvgl's LPAR/FONT v0.2.4
  surface for lvglpp's wrap-`lv_*` architecture; introduces the
  one-object-model rule, the `LVGLPP-WRAP` migration initiative, the WID
  namespace reconciliation, the playit↔lv_obj bridge, and the
  wave/phase plan (LPAR-01..16, FONT-00..05). **Not ratified** — awaiting
  owner go-ahead (the equivalent of rlvgl's "LPAR-00 RATIFIED"
  instruction). No execution PR may cite an LPAR phase until this chapter
  and LPAR-01 are ratified.
- **2026-06-15** — LPAR-00 **ratified** by owner instruction ("01 and
  02 ratified" — the two drafted chapters, LPAR-00 + LPAR-01). Wave 0
  unblocked: `LVGLPP-WRAP-00` and LPAR-01 may proceed.
