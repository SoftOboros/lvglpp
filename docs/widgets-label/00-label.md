# 00 — Label

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **WID-01**.

The key words **MUST**, **SHOULD**, **MAY** are interpreted per RFC
2119 / 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `Label` field set, method set, draw sequence | `rlvgl/widgets/src/label.rs` (v0.2.0 @ 79f730d) | Canonical. |
| `draw_widget_bg` body | `rlvgl/core/src/draw.rs:480` | Canonical. lvglpp ports a minimal version under CORE-04a (no rounded corners — deferred). |
| `Renderer::draw_text` for the text glyph rendering | `rlvgl/core/src/renderer.rs:17` | rlvgl Label delegates text rendering to the renderer; lvglpp does the same. |

## §1 Purpose

Land the first concrete widget on the lvglpp surface so the end-to-end
stack (Event → Widget → Renderer → Style + draw helper → text) gets
exercised by a real type. The Label chapter is also the precedent for
every subsequent widget chapter: once this is ratified, WID-02..WID-08
can mirror its structure.

## §2 Problem statement

The widget surface (CORE-03) and the renderer surface (CORE-04) and
the style surface (CORE-05) are all defined but have no concrete
consumer in lvglpp. Without a worked example, it is easy for a
follow-up widget to silently diverge from rlvgl in some axis (call
order, alpha handling, text origin, …). Label pins these down.

## §3 Canonical glossary

- **`Label`** — Owned by this chapter. Mirrored as
  `lvglpp::widgets::Label` at
  `widgets/include/lvglpp/widgets/label.hpp`. Five fields (§5.1) and
  the four `Widget` overrides (§5.2).
- **`draw_widget_bg`** — As defined (minimal port) in
  `core/include/lvglpp/core/draw_helpers.hpp`. Mirrors
  `rlvgl/core/src/draw.rs:480` with **DELTA**: rounded-corner +
  rounded-border path is **deferred to CORE-04b**; the lvglpp port
  handles only the radius==0 case. A widget that needs rounded
  corners today MUST set `radius = 0` until CORE-04b lands.
- **Text origin** — Concepts doc §5.3 — Label passes
  `(bounds.x, bounds.y + bounds.height)` to `Renderer::draw_text`,
  matching `rlvgl/widgets/src/label.rs:48` exactly. The `Renderer`
  treats this as the **text baseline** anchor (per CORE-04 §5.1).

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Label` fields + method set | `rlvgl/widgets/src/label.rs:11, :42` (canonical) | `lvglpp::widgets::Label`. |
| Field-set / method-set extension | this chapter — **Specification Required** | Per-widget chapter amendment; no concepts-doc-level Standards Action because Label-internal additions don't cross language pairs. |
| Text-origin convention (baseline at bottom-left) | `rlvgl/widgets/src/label.rs:48` | All future text-bearing widgets in lvglpp MUST follow this. |
| Bg fill semantics | `rlvgl/core/src/draw.rs:480` | minimal port in CORE-04a; rounded-corner extension is CORE-04b. |

## §5 Frozen decisions

### §5.1 `Label` field set — **Specification Required**

| Field | Type | Default / source | Notes |
| --- | --- | --- | --- |
| `bounds` | `lvglpp::core::Rect` | required ctor arg | Position + size relative to parent. |
| `text` | `std::string` | required ctor arg | UTF-8 by convention. Owned by the Label. |
| `style` | `lvglpp::core::Style` | `Style{}` (CORE-05 default) | Public — application code mutates directly. |
| `text_color` | `lvglpp::core::Color` | `Color{0,0,0,255}` (black opaque) | Public — modulated by `style.alpha` at draw time. |

Construction:

- `Label(std::string text, Rect bounds)` — primary ctor.
- `void set_text(std::string text)` — replace `text`.
- `std::string_view text() const noexcept` — read-only view (rlvgl
  returns `&str`; lvglpp returns `std::string_view` under the
  `borrows` ownership tag with lifetime tied to the Label).

### §5.2 `Widget` overrides — **Specification Required**

| Method | Body |
| --- | --- |
| `bounds() const` | returns `bounds` field. |
| `draw(Renderer& r) const` | **§5.3 sequence below.** |
| `handle_event(const Event&)` | returns `false`. Labels do not consume input. |
| `clear_region()` | inherits the `Widget` default (`std::nullopt`). |

### §5.3 `draw` call sequence — **Standards Action**

The draw method MUST issue exactly these calls in this order (parity
with `rlvgl/widgets/src/label.rs:46`):

1. `lvglpp::core::draw_widget_bg(renderer, bounds, style)`
   — bg fill + (deferred-richness) border per CORE-04a.
2. `renderer.draw_text(bounds.x, bounds.y + bounds.height, text,
   text_color.with_alpha(style.alpha))`
   — text baseline at bottom-left of `bounds`, alpha-modulated.

Adding, removing, or reordering renderer calls in the Label draw path
requires a chapter amendment because downstream tests (and rlvgl
playit fixtures) MAY depend on the exact call sequence.

## §10 Reconciliation vs. adjacent primitives

- **`lv_label` (LVGL C).** LVGL ships its own label widget. lvglpp
  Label does NOT wrap `lv_label` — it sits at the widget-tree layer
  above the renderer and uses `Renderer::draw_text` directly. A
  future variant that delegates to LVGL's text rendering is possible
  but out of scope here.
- **CORE-06 fonts.** Label relies on the renderer's built-in text
  path; the BitmapFont / PackedFont types from CORE-06 are NOT
  consulted by Label directly. A backend implementing
  `Renderer::draw_text` MAY internally use BitmapFont. Future
  text-bearing widgets (e.g. WID-02 Button) MAY accept an explicit
  `Font*` parameter; that decision belongs to those chapters.

## §11 Non-goals

- **Font selection per Label.** No `set_font(...)`. Renderer-level
  default only.
- **Rich text (multi-color, multi-font, alignment).** Out of scope.
- **Word wrap / line breaking.** Single-line draw_text call.
- **Click handling.** Labels return `false` from `handle_event`.
  Make a `Button` (WID-02) for clickable text.

## §12 Acceptance checklist

A conforming WID-01 execution PR MUST satisfy:

- [ ] `lvglpp::widgets::Label` is a class inheriting
      `lvglpp::core::Widget`.
- [ ] Five fields per §5.1 with the documented defaults / accessors.
- [ ] All four `Widget` overrides per §5.2.
- [ ] `draw(Renderer&)` issues the two-call sequence in §5.3, in
      that order.
- [ ] PARITY/LVGL/DELTA cite block at file head of every public
      header and translation unit.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
      *Note:* `std::string` is host-friendly; embedded posture
      consumers MAY substitute a string-view-only Label variant in a
      follow-up sub-phase. For WID-01 the embedded posture build
      need only verify the header *compiles*, not that all targets
      ship `<string>`.
- [ ] Test target `lvglpp_widgets_label` records the renderer call
      sequence via a `RecordingRenderer` and asserts:
      (a) one `fill_rect` for the bg, (b) one `draw_text` with the
      correct baseline anchor and alpha-modulated color.
- [ ] `widgets/STATUS.md` change log records the WID-01 landing.

A conforming PR MAY:

- Inline `Label` into a single header (pure-virtual base + concrete
  class typically warrant a .cpp; both shapes are conformant).

## §13 Files cited

- `rlvgl/widgets/src/label.rs` (v0.2.0 @ 79f730d)
- `rlvgl/core/src/draw.rs:480` (`draw_widget_bg`)
- `rlvgl/core/src/renderer.rs:17` (`Renderer::draw_text`)
- `lvglpp/docs/core-event/00-event-surface.md`,
  `lvglpp/docs/core-widget/00-widget-tree.md`,
  `lvglpp/docs/core-renderer/00-renderer-trait.md`,
  `lvglpp/docs/core-style/00-appearance.md`

## §14 Unblocks

- **WID-02** (`Button`) — gains a precedent for widget shape.
- **PLAT-01** (host SDL backend) — needs at least one widget to
  smoke-test with.
- **CORE-04a** — Label is the first call site exercising
  `draw_widget_bg`, locking its minimal-port shape.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Field set (§5.1),
  Widget-override set (§5.2), draw call sequence (§5.3) frozen.
  Execution unblocked.
- 2026-04-27 — WID-01 execution landed.
  `widgets/include/lvglpp/widgets/label.hpp` +
  `widgets/src/label.cpp`. `lvglpp::widgets` switched from
  INTERFACE to compiled library. Test target
  `lvglpp_widgets_label` with 6 fixtures passing — covers default
  opaque bg, translucent bg → blend_rect, zero-alpha skip-bg,
  border emits 4 fill_rects, text alpha modulation. Compiles
  clean under `LVGLPP_EMBEDDED_POSTURE=ON` (with the documented
  `std::string` host-friendliness caveat from §12).
