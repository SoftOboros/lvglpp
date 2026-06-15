# 01 — Parity baseline & matrix

Chapter status: **ratified 2026-06-15**.
Phase code: **LPAR-01**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact for the LPAR baseline: the pinned
LVGL target, the Rust-vs-C++ naming policy, the current/partial/missing
parity matrix, and the conformance levels. The initiative
[`README.md`](./README.md) is informative; [`00-concepts.md`](./00-concepts.md)
owns the initiative shape.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| Wrapped LVGL primitive surface + version | `lvgl/` submodule | **lvglpp-owned pin** (see §5.1). lvglpp wraps the `lv_*` of the commit it vendors. |
| Per-widget / per-substrate semantic contract | rlvgl `v0.2.4` `LPAR-*` / `FONT-*` concepts (`343f596`) | Canonical for behavior; mirrored per phase. |
| Naming policy, conformance levels, matrix | this chapter | Normative for lvglpp. |

## §1 Purpose

Pin the exact LVGL target lvglpp wraps, freeze the naming policy that
resolves Rust-vs-C++ and wrapper-collision questions, record the
current/partial/missing parity matrix, and define the conformance levels
a v0.2.4-parity claim is measured against. Until this chapter is
ratified, no implementation phase may claim parity (LPAR-00 §5.3).

## §2 Problem statement

rlvgl's `LPAR-01-BASELINE.md` pins rlvgl to an **LVGL 9.4.0-dev** matrix,
but rlvgl only consults LVGL as a *reference* — it re-implements the
behavior in Rust, so its pin is documentary. lvglpp **links and calls**
LVGL, so its pin is operational: the exact `lvgl/` commit determines
which `lv_*` symbols and `LV_USE_*` flags exist. lvglpp currently vendors
LVGL **9.6.0-dev**, newer than rlvgl's reference. The baseline must record
this DELTA so a "behavior matches rlvgl" claim is not silently a "behavior
matches LVGL 9.6" claim.

## §3 Canonical glossary

- **Parity baseline** — As defined in
  `rlvgl/docs/concepts/LPAR-01-BASELINE.md` (v0.2.4 @ `343f596`);
  **adapted**: lvglpp's baseline pins the *operational* LVGL commit it
  links, not a documentary reference. DELTA: LVGL **9.6.0-dev** vs
  rlvgl's documented **9.4.0-dev**.
- **Conformance level** — Owned by this chapter. The surface set a
  deployment claims (Host / Linux-fbdev / Sim / Disco), each with its own
  acceptance gate. See §6.
- **Naming policy** — Owned by this chapter. The rules mapping LVGL/rlvgl
  names to lvglpp C++ surface names and resolving wrapper collisions.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| LVGL commit + version pin | this chapter §5.1 (lvglpp-owned) |
| `LV_USE_*` / `LV_FONT_*` footprint per phase | the owning phase chapter's `OPTIONS.md` rollup |
| Naming policy | this chapter §5.2 |
| Current/partial/missing matrix | this chapter §7 (reconciled per landed phase) |
| Conformance levels | this chapter §6 |

## §5 Frozen decisions

### §5.1 LVGL pin — registration policy: **Standards Action**

- **LVGL**: `9.6.0-dev`, submodule commit
  `ee436e8520b9c44752e22142448b1dda5bf452a9` (gitlink in `lvgl/`).
- **rlvgl reference**: `v0.2.4` branch, submodule commit
  `343f596336dd01cb1e86bd68494766aeeab4238f` (gitlink in `rlvgl/`).
- **`lv_conf.h`**: `include/lvglpp/lv_conf.h`
  (`CMakeLists.txt:105` sets `LV_BUILD_CONF_PATH`).
- DELTA vs rlvgl LPAR-01: lvglpp wraps LVGL 9.6.0-dev (rlvgl documents
  9.4.0-dev). Any behavior difference between 9.4 and 9.6 in a wrapped
  widget is resolved in favor of **rlvgl observed behavior** (the
  cross-language contract) and documented as a per-widget DELTA. Bumping
  either pin is Standards Action and rides with the LPAR-16 conformance
  re-run.

### §5.2 Naming policy — registration policy: **Specification Required**

1. Public C++ names follow lvglpp idioms (`make_*`, `borrow_*`,
   `view_*`, `attach_*`, `detach_*`), not the Rust or C forms; the
   **ownership story** matches rlvgl (CLAUDE.md § "Cross-Project Parity").
2. Each parity widget is one module/header named for its LVGL family
   (`widgets/include/lvglpp/widgets/<family>.hpp`, e.g. `arc.hpp`,
   `dropdown.hpp`, `button_matrix.hpp`).
3. Wrapper collisions resolve here before the widget's code phase:
   - `Bar` (LPAR-11) is its own module; any future `Progress` is an
     alias or app-level wrapper, not a rename of `Bar`.
   - `MessageBox` (LPAR-14) is its own module; app-level `Modal`/`Alert`
     wrap it, not vice-versa.
   - rlvgl `WID-00` editable-input semantics live in the **LPAR-14
     Textarea** chapter; the lvglpp `WID-01..06` codes are NOT reused
     (LPAR-00 §9).
4. Every wrapped widget's header carries the `// PARITY: / // LVGL: /
   // DELTA:` cite block (CLAUDE.md § "Cite-block convention").

### §5.3 Phase footprint declaration

Each implementation phase MUST declare, in its `OPTIONS.md`, the
`LV_USE_*` / `LV_FONT_*` symbols it turns on. Indicative rollup:

| Wave | Indicative `lv_conf.h` symbols |
| --- | --- |
| 1 substrate | `LV_USE_FLEX`, `LV_USE_GRID`, `LV_USE_OBJ_*`, draw/font core (always on) |
| 2 font | `LV_FONT_*`, optionally `LV_USE_FREETYPE` / `LV_USE_TINY_TTF` (host) |
| 4 primitive | `LV_USE_ARC`, `LV_USE_BAR`, `LV_USE_LED`, `LV_USE_LINE`, `LV_USE_SCALE`, `LV_USE_SPINNER` |
| 5 control/nav/data | `LV_USE_BUTTONMATRIX`, `LV_USE_IMAGEBUTTON`, `LV_USE_SPINBOX`, `LV_USE_DROPDOWN`, `LV_USE_KEYBOARD`, `LV_USE_MENU`, `LV_USE_ROLLER`, `LV_USE_TABVIEW`, `LV_USE_TILEVIEW`, `LV_USE_WIN`, `LV_USE_CALENDAR`, `LV_USE_CHART`, `LV_USE_MSGBOX`, `LV_USE_SPAN`, `LV_USE_TABLE`, `LV_USE_TEXTAREA` |
| 6 canvas/media | `LV_USE_CANVAS`, `LV_USE_ANIMIMG`, `LV_USE_OBSERVER` |

## §6 Conformance levels

A v0.2.4-parity deployment declares the levels it satisfies:

- **L0 — Host (MUST)**: builds + passes `ctest` and the shared `playit`
  fixtures on the host (SDL backend), under both default and
  `LVGLPP_EMBEDDED_POSTURE=ON`.
- **L1 — Linux fbdev (SHOULD)**: same fixtures over the PLAT-LNX
  `lv_display`/`lv_indev` backend.
- **L2 — Disco-sim (SHOULD)**: same fixtures over the disco host
  simulator.
- **L3 — Disco hardware (MAY)**: on STM32H747I-DISCO. Currently
  verification-blocked at the LTDC display path; verified blind via
  probe-rs relays + `playit` `D` framebuffer dumps. A phase is "L3
  pending" until the panel path is unblocked.

A phase is **done** at the levels its acceptance checklist names; most
widget phases target L0+L1+L2, with L3 tracked separately.

## §7 Parity matrix (reconciled per landed phase)

Status legend: ✅ present (v0.2.0 mirror, pre-LPAR) · 🔁 to-wrap (LPAR
target) · — n/a.

| Area | rlvgl v0.2.4 | lvglpp today | LPAR target |
| --- | --- | --- | --- |
| Object/event/focus/scroll/timer substrate | ✅ (LPAR-02..06) | hand-rolled CORE-02/03/03a (Event/Widget/WidgetNode) | 🔁 LPAR-02..06 wrap `lv_obj/event/group/indev/scroll/timer/anim` |
| Style/theme | ✅ (LPAR-07) | CORE-05 `Style`/`Theme` (value types) | 🔁 LPAR-07 wrap `lv_style`/`lv_theme` |
| Text/draw/image/mask | ✅ (LPAR-08) | CORE-04/04a renderer + `draw_widget_bg`; CORE-06 `BitmapFont` | 🔁 LPAR-08 wrap `lv_draw`/`lv_font`/`lv_image` |
| Asset/fs | ✅ (LPAR-09) | CORE-07 plugin slots; CORE-07n RLE | 🔁 LPAR-09 wrap `lv_fs` + decoders |
| Layout (flex/grid) | ✅ (LPAR-10) | UI-04 stubbed | 🔁 LPAR-10 wrap `lv_flex`/`lv_grid` |
| Font selection (`WidgetFont`/registry/AA) | ✅ (FONT-00..05) | absent | 🔁 FONT-00..05 |
| Label/Button/Checkbox/Switch/Slider/Container/List/Image | partial (rlvgl set) | ✅ WID-01..06 (hand-rolled) | 🔁 migrate onto `lv_*` (LVGLPP-WRAP) |
| Arc/Bar/LED/Line/Scale/Spinner | ✅ (LPAR-11) | absent | 🔁 LPAR-11 |
| ButtonMatrix/ImageButton/Spinbox | ✅ (LPAR-12) | absent | 🔁 LPAR-12 |
| Dropdown/Keyboard/Menu/Roller/Tabview/Tileview/Window | ✅ (LPAR-13) | absent | 🔁 LPAR-13 |
| Calendar/Chart/MessageBox/Span/Table/Textarea | ✅ (LPAR-14) | absent | 🔁 LPAR-14 (+ rlvgl WID-00) |
| Canvas/AnimImage/ArcLabel/property/observer | ✅ (LPAR-15) | absent | 🔁 LPAR-15 |
| playit harness | ✅ | ✅ PLAYIT-* (walks hand-rolled tree) | 🔁 lv_obj query bridge (LVGLPP-WRAP) |
| i18n (RLTN + lv_translation) | ✅ | ✅ I18N-01/02 | — (orthogonal; no regression) |
| Conformance fixtures | ✅ (LPAR-16) | per-module ctest | 🔁 LPAR-16 shared goldens |

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `LPAR-01-BASELINE.md`** — lvglpp mirrors its *intent* (pin
  before parity) but owns its *pin* (operational LVGL commit). The two
  baselines may legitimately differ in LVGL minor version; §5.1 records
  the DELTA and the resolution rule (rlvgl observed behavior wins on
  conflict).
- **`include/lvglpp/lv_conf.h`** — the single switchboard for wrapped
  features. Phase `OPTIONS.md` files roll up into it; this chapter does
  not enumerate the full config, only the per-wave footprint (§5.3).

## §11 Non-goals

- Not a full `lv_conf.h` reference (that lives per-module in `OPTIONS.md`).
- Not a commitment to track LVGL `master`; the pin moves by Standards
  Action with a conformance re-run.

## §12 Acceptance checklist

LPAR-01 is accepted when:

- [ ] §5.1 pins the LVGL commit/version and rlvgl commit, and records the
      9.6-vs-9.4 DELTA + resolution rule.
- [ ] §5.2 naming policy resolves the `Bar`/`Progress`,
      `MessageBox`/`Modal`, and WID-code collisions.
- [ ] §6 conformance levels (L0–L3) are defined with gates.
- [ ] §7 matrix maps every rlvgl v0.2.4 area to a lvglpp status + LPAR
      target.
- [ ] `STATUS.md` files referencing the pin are updated to `v0.2.4` as
      their phases land (tracked, not required at ratification).

## §13 Files cited

- `rlvgl/docs/concepts/LPAR-01-BASELINE.md` (v0.2.4 @ `343f596`)
- `lvgl/lv_version.h` (9.6.0-dev @ `ee436e8`)
- `include/lvglpp/lv_conf.h`, `CMakeLists.txt:105`/`:108`
- `docs/lpar/00-concepts.md`
- per-module `STATUS.md` files (pin reconciliation targets)

## §14 Unblocks

- All LPAR-02..16 and FONT-00..05 implementation phases (parity may be
  claimed once this chapter is ratified).
- `LVGLPP-WRAP` migration (consumes the naming policy + conformance
  levels).

## §15 Change log

- **2026-06-15** — LPAR-01 drafted. Pins LVGL 9.6.0-dev @ `ee436e8` and
  rlvgl v0.2.4 @ `343f596`; records the 9.6-vs-9.4 baseline DELTA;
  freezes the naming policy (wrapper-collision + WID-code resolutions);
  defines conformance levels L0–L3; records the current/partial/missing
  matrix. **Not ratified** — awaiting owner go-ahead together with
  LPAR-00.
- **2026-06-15** — LPAR-01 **ratified** by owner instruction ("01 and
  02 ratified"). Parity may be claimed by LPAR-02..16 / FONT-00..05 once
  each phase's own chapter ratifies; `LVGLPP-WRAP` unblocked.
