# 00 — Widget tree

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-03**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `Widget` trait shape, `Rect` field set, `Color` channel order, `clear_region` semantics | `rlvgl/core/src/widget.rs` (v0.2.0 @ b178cbc) | Canonical. |
| C++ surface naming (`bounds`, `draw`, `handle_event`, `clear_region`), virtual-call dispatch | this chapter | Normative for lvglpp. |
| Underlying widget substrate | `lvgl/src/core/lv_obj.h` | Informative. `lvglpp::core::Widget` is a higher-level abstraction that the renderer (CORE-04) and platform backends pump events into; LVGL's `lv_obj_t` lives below the renderer seam and is wrapped by `ObjectView` (CORE-01). |

## §1 Purpose

Define the abstract base every widget inherits from, the `Rect` type
used for layout, and the `Color` type used for rendering. These three
together are what `lvglpp::widgets::*`, `lvglpp::ui::*`, and every
`lvglpp::platform::*` backend will compile against.

## §2 Problem statement

The widgets crate cannot land before the surface they implement is
stable. rlvgl's `Widget` trait at `rlvgl/core/src/widget.rs:66` exposes
exactly four methods (`bounds`, `draw`, `handle_event`,
`clear_region`); `Rect` and `Color` are foundational value types used
by the renderer (CORE-04), style (CORE-05), and font (CORE-06)
chapters. Drift on any of these three forks the whole
core-and-widgets layer.

## §3 Canonical glossary

- **`Widget`** — Owned by this chapter. Will be mirrored as
  `lvglpp::core::Widget` (planned at
  `core/include/lvglpp/core/widget.hpp`). Shape: abstract base class
  with four virtual methods (§5.1). Mirrors
  `rlvgl/core/src/widget.rs:66`.
- **`Rect`** — As defined in `rlvgl/core/src/widget.rs:14`; mirrored
  as `lvglpp::core::Rect`. Four `int32_t` fields: `x`, `y`, `width`,
  `height`. Coordinates relative to the parent widget; signed to
  simplify layout math.
- **`Color`** — As defined in `rlvgl/core/src/widget.rs:27`; mirrored
  as `lvglpp::core::Color`. Four `uint8_t` channels in **RGBA**
  order. Helpers: `to_argb8888()` and `with_alpha(opacity)` per
  `rlvgl/core/src/widget.rs:45` and `:52`.
- **`clear_region`** — Per-frame compositor hook returning
  `std::optional<Rect>`. When a widget has just become hidden, it
  returns its bounds so the compositor can restore the pristine
  background; otherwise `std::nullopt`. Mirrors
  `rlvgl/core/src/widget.rs:83`.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Widget` method set | `rlvgl/core/src/widget.rs:66` (canonical) | `lvglpp::core::Widget` (CORE-03 exec). |
| Method-set extension | this chapter — **Standards Action** | rlvgl + lvglpp PR pair, change log first. |
| `Rect` field shape | `rlvgl/core/src/widget.rs:14` | `lvglpp::core::Rect`; layout is logical, not byte-stable. |
| `Color` RGBA channel order | `rlvgl/core/src/widget.rs:27` | `lvglpp::core::Color`. The wire-level ARGB8888 layout used by display backends comes from `to_argb8888()`, not from `Color`'s field order. |
| Coordinate space convention | this chapter §5.4 | All widget coordinates relative to parent; renderer-facing draw uses landscape pixel coordinates after layout walks the tree. |

## §5 Frozen decisions

### §5.1 `Widget` virtual method set — **Standards Action**

Exactly four methods. Adding a method requires a chapter amendment
and a matching change in `rlvgl/core/src/widget.rs` per CLAUDE.md
§ "Cross-language change ordering".

| Method | Signature (C++ idiom) | Notes |
| --- | --- | --- |
| `bounds` | `Rect bounds() const` | Pure virtual. Returns area relative to parent. |
| `draw` | `void draw(Renderer& r) const` | Pure virtual. `Renderer` is from CORE-04. |
| `handle_event` | `bool handle_event(const Event& e)` | Pure virtual. Returns `true` if consumed. |
| `clear_region` | `std::optional<Rect> clear_region()` | **Default returns `std::nullopt`.** Override only for overlay widgets needing background restoration. |

Default implementations: only `clear_region` has one. The other three
are pure virtual; concrete widgets MUST implement them.

### §5.2 `Rect` field set — **Standards Action**

`int32_t x, y, width, height`, in this order. Width/height are
signed; negative values are not normalised by `Rect` itself but MAY
be rejected by consumers (e.g. the renderer skips `width <= 0`).

### §5.3 `Color` field set — **Standards Action**

`uint8_t r, g, b, a`. Channel order is **RGBA**. The `a` channel is
255 = opaque, 0 = transparent. `Color` MUST be constructible from
the four channels in RGBA order; aggregate initialisation is the
canonical form.

### §5.4 Coordinate space

- `Widget::bounds()` returns coordinates **relative to the parent
  widget**.
- `Widget::draw(Renderer&)` is called with the renderer in
  **landscape pixel coordinates** after the tree walk has applied
  parent translation. Widgets MUST NOT re-apply parent offsets in
  `draw`.
- `Widget::handle_event` receives events whose coordinates are in
  **landscape pixel coordinates** (per CORE-02 §5.5). Per-widget
  hit-testing (subtracting parent offsets) happens in the dispatch
  layer, not in `handle_event`.

### §5.5 `clear_region` semantics

- Called **once per frame, before `draw`**.
- A widget that has just become hidden SHOULD return its previous
  bounds. The compositor (CORE-04 territory) restores the background
  from a pristine copy.
- A visible widget MUST return `std::nullopt`.
- The C++ form uses `std::optional<Rect>`; rlvgl's `Option<Rect>`
  is identical in semantics (per `docs/std-mapping.md` § "Sum /
  option types").

## §10 Reconciliation vs. adjacent primitives

- **`lv_obj_t` (LVGL C).** LVGL's object tree is the substrate the
  renderer (CORE-04) ultimately writes into. `lvglpp::core::Widget`
  is a value-semantics abstraction layered above; concrete widgets
  MAY hold an `ObjectView` (CORE-01) into the LVGL tree, but the
  `Widget` interface itself does NOT expose `lv_obj_t`. Widget-author
  code SHOULD use `Widget` and reach for `ObjectView` only inside
  per-widget translation units.
- **`Color` vs. `lv_color_t`.** LVGL's `lv_color_t` is depth-dependent
  (16/24/32-bit). `lvglpp::core::Color` is always 32-bit RGBA;
  conversion to `lv_color_t` happens at the renderer seam.
- **`Rect` vs. `lv_area_t`.** Same logical shape, different field
  order/types. `lv_area_t` uses `int32_t x1, y1, x2, y2`;
  `lvglpp::core::Rect` uses `x, y, width, height`. Conversion is a
  small helper in the platform translation unit.

## §11 Non-goals

- **Layout engine.** Flex / grid / absolute positioning belong to
  CORE-05 (style) or to UI-04 (layout helpers). This chapter only
  defines the geometry value type.
- **Event propagation rules.** `handle_event` returning `true`
  consumes the event for that widget; how that interacts with
  parent / sibling propagation is defined by the dispatch layer
  (PLAYIT-02 / CORE-04 territory), not here.
- **Animated transforms.** `Rect` is a static rectangle; animated
  motion belongs to CORE-05.
- **Gesture recognition.** `DoubleTap` / `PressDown` / `PressRelease`
  are produced by recognisers upstream of the widget tree; they
  arrive as plain `Event` values at `handle_event`.

## §12 Acceptance checklist

A conforming CORE-03 execution PR MUST satisfy:

- [ ] `lvglpp::core::Widget` is an abstract base class with the four
      methods in §5.1.
- [ ] `lvglpp::core::Rect` has the four `int32_t` fields in §5.2,
      defaulted `==`.
- [ ] `lvglpp::core::Color` has the four `uint8_t` fields in §5.3,
      defaulted `==`, `to_argb8888()` returning the same value as
      `rlvgl/core/src/widget.rs:45` for any input, `with_alpha(u8)`
      computing alpha exactly per `rlvgl/core/src/widget.rs:52`.
- [ ] PARITY/LVGL/DELTA cite block on each public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] `core/STATUS.md` change log records ratification of CORE-03
      execution.
- [ ] `Widget` may use `virtual` + `= 0` (classical C++); a future
      sub-phase MAY introduce a CRTP-based static polymorphism
      alternative without breaking the value-type contract.

A conforming PR MAY:

- Inline `Rect`, `Color`, `Widget` into a single `widget.hpp` or
  split per concept.
- Provide non-virtual helpers (e.g. `Rect::contains(int32_t x,
  int32_t y)`) without amending this chapter.

## §13 Files cited

- `rlvgl/core/src/widget.rs` (v0.2.0 @ b178cbc)
- `rlvgl/core/src/event.rs`, `rlvgl/core/src/renderer.rs`
- `lvgl/src/core/lv_obj.h` (informative; v9.x)
- `lvglpp/CLAUDE.md`, `lvglpp/docs/std-mapping.md`,
  `lvglpp/docs/core-event/00-event-surface.md`

## §14 Unblocks

- **WID-01** onwards — every widget inherits `Widget`.
- **CORE-04** — `Renderer::fill_rect(Rect, Color)` consumes the
  types frozen here.
- **CORE-05** — `Style::bg_color`, `Style::border_color` use
  `Color`.
- **CORE-06** — `BitmapFont::draw_char(...)` and `PackedFont::...`
  consume `Color` and `Rect`.
- **PLAYIT-02 / PLAYIT-04** — tagged-widget queries (`QB:`, `QE:`,
  `QC:`) require a `Widget` tree to traverse.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. `Widget` method set
  (§5.1), `Rect` field set (§5.2), `Color` channel order (§5.3) all
  frozen with **Standards Action** registration. Execution unblocked.
- 2026-04-27 — CORE-03 execution landed in
  `core/include/lvglpp/core/widget.hpp`. All §12 acceptance bullets
  satisfied: `Widget` is an abstract base with the four §5.1
  methods (default `clear_region` returning `nullopt`); `Rect` and
  `Color` have the §5.2 / §5.3 field sets with defaulted `==`;
  `Color::to_argb8888()` and `Color::with_alpha()` mirror rlvgl
  computation exactly. Test target `lvglpp_core_widget` (6
  assertions). Compiles clean under `LVGLPP_EMBEDDED_POSTURE=ON`.
