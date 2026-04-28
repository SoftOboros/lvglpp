# 00 — Checkbox + Switch

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **WID-03**.

## §0 Authority

- `Checkbox` field set, draw sequence, toggle semantics:
  `rlvgl/widgets/src/checkbox.rs` (v0.2.0 @ b178cbc). Canonical.
- `Switch` field set, draw sequence, toggle semantics:
  `rlvgl/widgets/src/switch.rs` (v0.2.0 @ b178cbc). Canonical.
- Underlying widget tree: CORE-03 + CORE-03a.
- Underlying draw helpers: CORE-04a (with the `fill_rounded_rect`
  shim — radius is currently ignored, CORE-04b deferred).

## §1 Purpose

The first **stateful** widgets in lvglpp. Both consume the same
debounced `PressRelease` Button consumes, but maintain a boolean
state and toggle on each tap. Together they exercise the
recogniser pipeline (PLAYIT-04a) against multiple consumers in a
single tree.

## §3 Canonical glossary

- **`Checkbox`** — Owned by this chapter. Mirrors
  `rlvgl/widgets/src/checkbox.rs:9`. Six fields (§5.1) plus a
  10×10 rendered indicator box.
- **`Switch`** — Owned by this chapter. Mirrors
  `rlvgl/widgets/src/switch.rs:10`. Four fields (§5.2) plus a
  half-width sliding knob.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Checkbox` field set + draw + toggle | `rlvgl/widgets/src/checkbox.rs` (canonical) | `lvglpp::widgets::Checkbox`. |
| `Switch` field set + draw + toggle | `rlvgl/widgets/src/switch.rs` (canonical) | `lvglpp::widgets::Switch`. |
| Toggle semantics (`PressRelease` inside bounds → flip state, return `true`) | rlvgl (canonical) | both widgets, byte-for-byte. |

## §5 Frozen decisions

### §5.1 `Checkbox` — **Specification Required**

Field set:

| Field | Type | Notes |
| --- | --- | --- |
| `bounds` | `Rect` | Set by ctor. |
| `text` | `std::string` (private) | Label, accessed via `text()` / `set_text()`. |
| `style` | `Style` | Public. |
| `text_color` | `Color` | Public. Default `(0,0,0,255)`. |
| `check_color` | `Color` | Public. Default `(0,0,0,255)`. |
| `checked` | `bool` (private) | Accessed via `is_checked()` / `set_checked(bool)`. |

`draw()` sequence (mirrors `checkbox.rs:51`):

1. `draw_widget_bg(renderer, bounds, style)`.
2. `fill_rounded_rect(renderer, box_rect, border_color.with_alpha(alpha), radius)`
   — 10×10 box anchored at `(bounds.x, bounds.y)`.
3. **If checked:**
   `fill_rounded_rect(renderer, inner_rect, check_color.with_alpha(alpha), radius)`
   — 6×6 inner mark inset 2px on every side.
4. `Renderer::draw_text((bounds.x + 14, bounds.y + bounds.height), text, text_color.with_alpha(alpha))`
   — text right of the box, baseline at bottom-left.

`handle_event` (mirrors `checkbox.rs:86`):
- `PressRelease{x,y}` inside bounds → `checked = !checked`,
  return `true`.
- All other events → return `false`.

### §5.2 `Switch` — **Specification Required**

Field set:

| Field | Type | Notes |
| --- | --- | --- |
| `bounds` | `Rect` | Set by ctor. |
| `style` | `Style` | Public. |
| `knob_color` | `Color` | Public. Default `(0,0,0,255)`. |
| `on` | `bool` (private) | `is_on()` / `set_on(bool)`. |

`draw()` sequence (mirrors `switch.rs:46`):

1. `draw_widget_bg(renderer, bounds, style)` — track background.
2. `fill_rounded_rect(renderer, knob_rect, knob_color.with_alpha(alpha), radius)`
   — knob occupies `width / 2` of the track. When `on`, anchored
   on the right; when off, anchored on the left.

`handle_event` (mirrors `switch.rs:72`):
- `PressRelease{x,y}` inside bounds → `on = !on`, return `true`.
- All other events → return `false`.

### §5.3 Half-open bounds — **Standards Action**

Both widgets use the half-open convention from WID-02 §5.3
(`x ∈ [bounds.x, bounds.x + bounds.width)`). Identical edge
behaviour to Button.

## §10 Reconciliation vs. adjacent primitives

- **WID-02 Button.** Same PressRelease-inside-bounds shape.
  Toggles do not expose an `on_click` callback; state-change
  observation goes through `is_checked()` / `is_on()` polled by
  the application after the next dispatch. Adding a callback is a
  follow-up sub-phase if a real consumer needs it.
- **CORE-04a `fill_rounded_rect`.** The shim ignores radius
  today; until CORE-04b lands, both widgets render with sharp
  corners regardless of `style.radius`.
- **rlvgl `Radio`** (separate file). Out of scope for WID-03.

## §11 Non-goals

- `on_change` callback. Application polls state after dispatch.
- Tri-state checkboxes. Boolean only.
- Animated knob slide. Switch knob jumps; animation belongs in a
  later sub-phase (CORE-05a).
- Keyboard activation (Space / Enter). Pointer-only for now.

## §12 Acceptance checklist

- [ ] `lvglpp::widgets::Checkbox` per §5.1; `Switch` per §5.2.
- [ ] `handle_event` semantics for both widgets per §5.1 / §5.2 +
      half-open bounds (§5.3).
- [ ] Test target `lvglpp_widgets_checkbox` covers: ctor +
      defaults, PressRelease-inside flips state, PressRelease-outside
      ignored, all other events ignored, draw call sequence
      (bg + box + check-when-set + text).
- [ ] Test target `lvglpp_widgets_switch` covers: ctor + defaults,
      toggle, draw call sequence (track + knob position based on
      state).
- [ ] PARITY/LVGL/DELTA cite block on each public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.

## §13 Files cited

- `rlvgl/widgets/src/checkbox.rs`, `switch.rs` (v0.2.0 @ b178cbc).
- `lvglpp/docs/widgets-button/00-button.md` (the parent chapter
  that established the toggle-by-PressRelease idiom).
- `lvglpp/docs/core-renderer/00-renderer-trait.md`
  + `lvglpp/docs/core-widget/00-widget-tree.md`.

## §14 Unblocks

- The SDL demo gains stateful widgets that recognisable from
  the playit-driven (`T@<tag>:…`) and SDL-driven (mouse-click)
  paths interchangeably.
- WID-07 `Radio` will follow the same shape with mutual exclusion
  added.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Field sets (§5.1,
  §5.2), draw sequences, toggle semantics, half-open bounds
  (§5.3) all frozen.
- 2026-04-27 — WID-03 execution landed.
  `widgets/include/lvglpp/widgets/{checkbox,switch}.hpp` +
  `widgets/src/{checkbox,switch}.cpp`. CORE-04a gained the
  `fill_rounded_rect` shim (radius accepted but ignored until
  CORE-04b). Test targets `lvglpp_widgets_checkbox` (7 fixtures)
  + `lvglpp_widgets_switch` (6 fixtures) green; embedded posture
  clean. SDL demo wires `dark_mode` Checkbox + `mute` Switch
  alongside the existing Button; piped `T@dark_mode:…` and
  `T@mute:…` toggle them identically to mouse clicks.
