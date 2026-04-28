# 00 — Appearance

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-05**.

The key words **MUST**, **MUST NOT**, **SHOULD**, **MAY** in this
chapter are interpreted per RFC 2119 and RFC 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `Style` field set, defaults, `StyleBuilder` chain | `rlvgl/core/src/style.rs` (v0.2.0 @ 79f730d) | Canonical. |
| `Theme` virtual surface + `LightTheme` / `DarkTheme` | `rlvgl/core/src/theme.rs` (v0.2.0 @ 79f730d) | Canonical. |
| `Easing` curves + `LoopMode` shape + `loop_progress` semantics | `rlvgl/core/src/animation.rs:19, :102, :114` (v0.2.0 @ 79f730d) | Canonical. |
| Richer animation types (`Fade`, `Slide`, `Motion`, `FadeTransition`, `KeyFade`, `Timeline`) | `rlvgl/core/src/animation.rs:165+` | **Out of scope** for this chapter. Each lands in its own CORE-05a / CORE-05b / … sub-phase when the first call site needs it. |

## §1 Purpose

Define the value-type surface that styles widget appearance, the
trait that bulk-applies styles, and the math primitives that drive
animation timelines. These are the smallest units that the widgets
crate, the UI crate, and any future animation code compile against.

## §2 Problem statement

`Style` is consumed by every widget's `draw()`. `Theme` is consumed
by `lvglpp::ui` themed-app bring-up. `Easing` and `LoopMode` are the
math primitives every richer animation type composes; pinning them
here lets later sub-phases land without re-litigating the curve set.

## §3 Canonical glossary

- **`Style`** — As defined in `rlvgl/core/src/style.rs:5`. Mirrored
  as `lvglpp::core::Style`. Five fields (§5.1).
- **`StyleBuilder`** — As defined in `rlvgl/core/src/style.rs:33`.
  Fluent builder; mirrored as `lvglpp::core::StyleBuilder` with
  `bg_color() / border_color() / border_width() / alpha() /
  radius() / build()` chain.
- **`Theme`** — As defined in `rlvgl/core/src/theme.rs:11`. Abstract
  base; mirrored as `lvglpp::core::Theme` with one virtual method
  `apply(Style&)`.
- **`LightTheme` / `DarkTheme`** — As defined in
  `rlvgl/core/src/theme.rs:17, :27`. Concrete defaults swapping
  `bg_color` / `border_color`; mirrored without modification.
- **`Easing`** — As defined in `rlvgl/core/src/animation.rs:19`.
  Nine variants (§5.3).
- **`LoopMode`** — As defined in `rlvgl/core/src/animation.rs:102`.
  Three variants (§5.4); `0` means infinite for the parametric
  forms.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Style` field set + defaults | `rlvgl/core/src/style.rs:5, :22` | `lvglpp::core::Style`. |
| `StyleBuilder` chain | `rlvgl/core/src/style.rs:33` | `lvglpp::core::StyleBuilder`. |
| Field-set extension | this chapter — **Standards Action** | rlvgl + lvglpp PR pair. |
| `Theme::apply` shape | `rlvgl/core/src/theme.rs:11` | `lvglpp::core::Theme`. |
| `Easing` variant set | `rlvgl/core/src/animation.rs:19` | `lvglpp::core::Easing`; **Standards Action** to add a curve. |
| `LoopMode` variant set | `rlvgl/core/src/animation.rs:102` | `lvglpp::core::LoopMode`; **Standards Action**. |
| Easing math (polynomial coefficients) | `rlvgl/core/src/animation.rs:45` | Bit-exact float parity required (§5.5). |

## §5 Frozen decisions

### §5.1 `Style` field set — **Standards Action**

| Field | Type | Default | Notes |
| --- | --- | --- | --- |
| `bg_color` | `Color` | `(255, 255, 255, 255)` (white opaque) | |
| `border_color` | `Color` | `(0, 0, 0, 255)` (black opaque) | |
| `border_width` | `uint8_t` | `0` | Pixels. |
| `alpha` | `uint8_t` | `255` | Widget-level multiplier. |
| `radius` | `uint8_t` | `0` | Corner radius pixels. |

`Style` MUST be aggregate-constructible and have a defaulted `==`.

### §5.2 `StyleBuilder` chain — **Standards Action**

`bg_color(Color)`, `border_color(Color)`, `border_width(uint8_t)`,
`alpha(uint8_t)`, `radius(uint8_t)`, `build() -> Style`. Every setter
returns `StyleBuilder&` (rlvgl uses `mut self`); the C++ form is a
reference for chained-call ergonomics.

### §5.3 `Easing` variants — **Standards Action**

Nine variants in this exact order (matters for serialization /
recording):

`Linear` (default), `EaseIn`, `EaseOut`, `EaseInOut`, `EaseInCubic`,
`EaseOutCubic`, `EaseInOutCubic`, `Bounce`, `Step(uint8_t n)`.

Adding a curve requires a chapter amendment plus matching
`rlvgl/core/src/animation.rs` PR.

### §5.4 `LoopMode` variants — **Standards Action**

Three variants: `Once` (default), `Repeat(uint16_t n)`,
`PingPong(uint16_t n)`. `n == 0` means **infinite** for both
parametric forms.

### §5.5 Easing math — bit-exact parity

The C++ `Easing::apply(float t)` MUST produce the same float result
as `rlvgl/core/src/animation.rs:45` for every input in `[0, 1]` to
within IEEE-754 single-precision rounding (i.e. the same expression
tree compiled by the same target's FPU). The rlvgl source uses no
`libm` / no transcendentals; the C++ port MUST do the same so
embedded targets without `<cmath>` still compile.

The `Step(0)` case returns `t` unchanged (rlvgl's behaviour at
`animation.rs:85`).

### §5.6 `Theme` virtual surface — **Standards Action**

One method: `void apply(Style& style) const`. Concrete themes
mutate the passed-in style in place. `LightTheme` and `DarkTheme`
match `rlvgl/core/src/theme.rs:17, :27` exactly.

## §10 Reconciliation vs. adjacent primitives

- **LVGL `lv_style_t`.** LVGL's style system is property-bag based
  with cascade rules. `lvglpp::core::Style` is a small fixed-field
  value type — strictly less expressive on purpose. Per-widget
  translation units that need richer LVGL styling (e.g.
  `lv_style_set_text_font`) reach into LVGL directly inside the
  widget's `draw(Renderer&)`; the lvglpp `Style` type is the
  *external* contract widgets accept.
- **CORE-06 fonts.** `Style` does NOT carry a font. Font selection
  is per-widget and per-call-site (CORE-06).
- **Future animation sub-phases.** `Fade`, `Slide`, `Motion`,
  `Timeline` from `rlvgl/core/src/animation.rs:165+` will each need
  a sub-phase concepts amendment because they hold raw mutable
  pointers (`*mut Style`) — the lvglpp port must restate the
  ownership story (likely `borrows mut Style`) explicitly.

## §11 Non-goals

- **Cascade / inheritance.** `Style` is a flat value; merging /
  overriding is the consumer's job.
- **Animated style transitions.** `FadeTransition`, `KeyFade`,
  `Timeline` — out of scope; sub-phases.
- **Property-pack persistence.** No JSON / binary serialization.
  rlvgl-creator owns asset emission; lvglpp consumes the typed
  values it produces.
- **Theme registry / dynamic theme loading.** Themes are concrete
  classes constructed at the application level.

## §12 Acceptance checklist

A conforming CORE-05 execution PR MUST satisfy:

- [ ] `lvglpp::core::Style` exposes the five fields in §5.1 with
      defaults matching rlvgl, defaulted `==`.
- [ ] `lvglpp::core::StyleBuilder` exposes the chain in §5.2 and
      `build()` returns a `Style` by value.
- [ ] `lvglpp::core::Theme` is an abstract base with
      `apply(Style&)`.
- [ ] `lvglpp::core::LightTheme` and `DarkTheme` produce
      byte-identical `Style` outputs to rlvgl's themes for the same
      input.
- [ ] `lvglpp::core::Easing` exposes the nine variants in §5.3,
      with `apply(float t)` producing IEEE-754-equivalent output to
      `rlvgl/core/src/animation.rs:45` across a sampled grid.
- [ ] `lvglpp::core::LoopMode` exposes the three variants in §5.4.
- [ ] PARITY/LVGL/DELTA cite block on each public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON` — note
      `Easing::apply` returns `float`; embedded posture MUST permit
      `<cstdint>` / float arithmetic without `<cmath>`.
- [ ] Test fixture sampling `Easing::apply(t)` at
      `t ∈ {0, 0.25, 0.5, 0.75, 1.0}` for every variant matches
      hand-computed values from the rlvgl source.
- [ ] `core/STATUS.md` change log records ratification of CORE-05
      execution.

A conforming PR MAY:

- Implement `Easing` as `std::variant` (for `Step(n)`) or as an
  `enum class Kind + uint8_t step_n` tagged form. Both satisfy the
  variant set in §5.3.

## §13 Files cited

- `rlvgl/core/src/style.rs` (v0.2.0 @ 79f730d)
- `rlvgl/core/src/theme.rs` (v0.2.0 @ 79f730d)
- `rlvgl/core/src/animation.rs:19-160` (v0.2.0 @ 79f730d) —
  `Easing`, `LoopMode`, `loop_progress`. Lines beyond `:160` are
  out of scope for this chapter.
- `lvglpp/docs/core-widget/00-widget-tree.md` (`Color` definition)

## §14 Unblocks

- **WID-01** onwards — every widget uses `Style`.
- **UI-03** — themed-app bring-up consumes `Theme` and
  `LightTheme` / `DarkTheme`.
- **CORE-05a / CORE-05b / …** — richer animation sub-phases (Fade,
  Slide, Motion, Timeline) build on `Easing` and `LoopMode`.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. `Style` (§5.1),
  `StyleBuilder` (§5.2), `Easing` (§5.3), `LoopMode` (§5.4),
  Easing math (§5.5), `Theme` (§5.6) all frozen with **Standards
  Action** registration. Richer animation surface deferred to
  sub-phases. Execution unblocked.
- 2026-04-27 — CORE-05 execution landed in
  `core/include/lvglpp/core/style.hpp`. All §12 acceptance bullets
  satisfied. `Style` defaults match rlvgl; `StyleBuilder` chain
  builds aggregate `Style`; `Theme` abstract base + `LightTheme`
  / `DarkTheme` produce identical bg/border swaps;
  `Easing::apply` covers all 9 curves with no `<cmath>`
  dependency; `LoopMode` exposes Once/Repeat/PingPong with `count
  == 0` infinite semantics. Test target `lvglpp_core_style` (13
  test functions). Compiles clean under
  `LVGLPP_EMBEDDED_POSTURE=ON`.
