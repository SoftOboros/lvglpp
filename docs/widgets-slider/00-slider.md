# 00 — Slider

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **WID-04**.

## §0 Authority

- `Slider` field set, position math, draw sequence, value-mapping
  semantics: `rlvgl/widgets/src/slider.rs` (v0.2.0 @ b178cbc).
  Canonical.
- Underlying widget tree: CORE-03 + CORE-03a.
- Underlying draw helpers: CORE-04a (`fill_rounded_rect` shim).

## §1 Purpose

The first **value-bound** widget. `Slider` exposes a
`min`/`max`/`value` triple with continuous-domain semantics —
unlike Button (event-only) or Checkbox/Switch (boolean state).
A `PressRelease` inside bounds maps the x-coordinate to a value
in the range; the recogniser pipeline (PLAYIT-04a) feeds either
SDL clicks or piped `T@<tag>:<x>,<y>` taps into the same handler.

## §3 Canonical glossary

- **`Slider`** — Owned by this chapter. Mirrors
  `rlvgl/widgets/src/slider.rs:9`. Six fields (§5.1) plus a
  4-pixel-tall track and a 10-pixel-square knob.
- **`position_from_value()`** — Private helper computing the knob's
  screen x-coordinate from the current value. Mirrors `slider.rs:44`
  exactly: `bounds.x + ratio * bounds.width` where `ratio = (value
  - min) / (max - min)`. Range==0 returns `bounds.x` directly to
  avoid divide-by-zero.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Field set + ctor signature | `slider.rs` (canonical) | `lvglpp::widgets::Slider`. |
| `set_value(int32_t)` clamping | `slider.rs:39` (canonical) | `lvglpp::widgets::Slider::set_value`. |
| Value → position math (ratio-based) | `slider.rs:44` (canonical) | `position_from_value`. |
| Position → value math (ratio-based) | `slider.rs:107` (canonical) | `handle_event`. |
| Track + knob draw shape | `slider.rs:62` (canonical) | `Slider::draw`. |

## §5 Frozen decisions

### §5.1 Field set — **Specification Required**

| Field | Type | Notes |
| --- | --- | --- |
| `bounds` | `Rect` | Set by ctor. |
| `style` | `Style` | Public. |
| `knob_color` | `Color` | Public. Default `(0,0,0,255)`. |
| `min` | `int32_t` (private) | Set by ctor. |
| `max` | `int32_t` (private) | Set by ctor. |
| `value` | `int32_t` (private) | `value()` getter, `set_value(int32_t)` clamps to `[min, max]`. Initialised to `min`. |

Constructor signature: `Slider(Rect bounds, int32_t min, int32_t max)`.
Mirrors rlvgl exactly.

### §5.2 `set_value` clamping — **Standards Action**

```
void set_value(int32_t v) {
    value_ = std::clamp(v, min_, max_);
}
```

Out-of-range inputs are silently clamped. Same as rlvgl's
`val.clamp(self.min, self.max)`.

### §5.3 Position math — **Standards Action**

```
int32_t position_from_value() const {
    int32_t range = max_ - min_;
    if (range == 0) return bounds_.x;
    float ratio = static_cast<float>(value_ - min_) / static_cast<float>(range);
    return bounds_.x + static_cast<int32_t>(ratio * static_cast<float>(bounds_.width));
}
```

Floating-point math matches rlvgl's `f32`-based computation
(`slider.rs:44`). The cast to `int32_t` truncates toward zero —
identical to Rust's `as i32` from `f32`.

### §5.4 `draw` sequence — **Standards Action**

Mirrors `slider.rs:59`:

1. `draw_widget_bg(renderer, bounds, style)` — full background.
2. **Track** at vertical centre, 4 pixels tall:
   ```
   track_y     = bounds.y + (bounds.height - 4) / 2;
   track_rect  = {bounds.x, track_y, bounds.width, 4};
   track_radius = (style.radius > 0) ? 2 : 0;  // pill when radius set
   fill_rounded_rect(renderer, track_rect,
                     style.border_color.with_alpha(alpha), track_radius);
   ```
3. **Knob** 10×10 centred on `position_from_value()`:
   ```
   knob_x       = position_from_value();
   knob_rect    = {knob_x - 5, bounds.y + (bounds.height - 10) / 2, 10, 10};
   knob_radius  = (style.radius > 0) ? 5 : 0;  // round when radius set
   fill_rounded_rect(renderer, knob_rect,
                     knob_color.with_alpha(alpha), knob_radius);
   ```

Track and knob radii are derived locally — they're independent of
`style.radius`'s exact value, gated only on whether it's non-zero.
Mirrors `slider.rs:73, :90`. Until CORE-04b lands the actual
rounding, the radii are accepted but ignored.

### §5.5 `handle_event` semantics — **Standards Action**

```
bool handle_event(const Event& event) {
    auto* pr = std::get_if<event::PressRelease>(&event);
    if (!pr) return false;
    if (pr->y < bounds.y || pr->y >= bounds.y + bounds.height) return false;
    if (pr->x < bounds.x || pr->x >= bounds.x + bounds.width)  return false;
    int32_t relative = pr->x - bounds.x;
    float   ratio    = static_cast<float>(relative) / static_cast<float>(bounds.width);
    int32_t new_val  = min_ + static_cast<int32_t>(static_cast<float>(max_ - min_) * ratio);
    set_value(new_val);
    return true;
}
```

Out-of-bounds (in either axis) → `false` (event passes through).
Inside-bounds → update value, return `true`.

## §10 Reconciliation vs. adjacent primitives

- **WID-02 Button.** Same `PressRelease`-only consumer pattern.
  Slider does not expose an `on_change` callback — application
  polls `value()` after dispatch. Following sub-phase if needed.
- **CORE-04a `fill_rounded_rect`.** The radius arguments are
  passed but ignored until CORE-04b. Sharp-corner rendering is
  byte-for-byte rlvgl-equivalent for `style.radius == 0`.
- **PointerMove tracking** (continuous drag while held). rlvgl
  responds only to `PressRelease`; lvglpp matches. A drag-aware
  variant is a follow-up sub-phase (WID-04a) once a real consumer
  needs it.

## §11 Non-goals

- Drag-tracking (knob follows finger). PressRelease-only.
- Vertical orientation. Horizontal only.
- Logarithmic / non-linear value mapping. Linear only.
- `on_change` callback. Polled.
- Step quantisation. Continuous integer values.

## §12 Acceptance checklist

- [ ] `lvglpp::widgets::Slider` per §5.1.
- [ ] `set_value` clamps per §5.2.
- [ ] `position_from_value` matches §5.3 byte-for-byte for
      common (min,max,value,bounds) tuples.
- [ ] `draw` sequence per §5.4 — verified by a `RecordingRenderer`
      test asserting (bg fill, track fill_rect, knob fill_rect).
- [ ] `handle_event` per §5.5 — verified by fixtures: tap at
      bounds.x sets value to min; tap at bounds.x + bounds.width - 1
      sets to ~max; tap outside bounds returns false; tap on a
      degenerate range (min == max) clamps to min.
- [ ] PARITY/LVGL/DELTA cite block on the public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] `widgets/STATUS.md` change log records WID-04 landing.

## §13 Files cited

- `rlvgl/widgets/src/slider.rs` (v0.2.0 @ b178cbc).
- `lvglpp/docs/widgets-button/00-button.md` (parent idiom).
- `lvglpp/docs/core-renderer/00-renderer-trait.md`,
  `lvglpp/docs/core-widget/00-widget-tree.md`,
  `lvglpp/docs/core-style/00-appearance.md`.

## §14 Unblocks

- The SDL demo gains a continuous-value control. The recogniser
  pipeline (PLAYIT-04a) now feeds the same fixture into both
  toggle and value widgets.
- WID-04a — drag-tracking variant — has a defined seam if a
  real consumer needs continuous knob updates.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Field set (§5.1),
  clamping (§5.2), position math (§5.3), draw sequence (§5.4),
  handle_event (§5.5) all frozen.
- 2026-04-27 — WID-04 execution landed.
  `widgets/include/lvglpp/widgets/slider.hpp` +
  `widgets/src/slider.cpp`. Test target `lvglpp_widgets_slider`
  (11 fixtures, including degenerate `min == max` and negative
  ranges) green; embedded posture clean. SDL demo wires the
  Slider as tagged `volume`; piped `T@volume:<x>,<y>` taps
  produce values matching the on-screen layout
  (ratio of x within bounds → value in [0, 100]).
