# 00 — Button

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **WID-02**.

## §0 Authority

- `Button` field set, method set, click semantics: `rlvgl/widgets/src/button.rs`
  (v0.2.0 @ 79f730d). Canonical.
- Underlying widget tree: CORE-03 + CORE-03a.
- Underlying text rendering: WID-01 (Label is the inner draw target).

## §1 Purpose

The first **interactive** widget. Sets the precedent for
event-consuming widgets in lvglpp: how a widget detects that an
input event lands inside its bounds and how it fires a callback.

## §3 Canonical glossary

- **`Button`** — Owned by this chapter. Mirrors
  `rlvgl/widgets/src/button.rs:11`. Composes a `Label` for visual
  rendering and adds a click-callback slot.
- **`ClickHandler`** — Owned by this chapter. C++ form is
  `std::function<void(Button&)>`; rlvgl uses
  `Box<dyn FnMut(&mut Button)>`. Both express "a heap-owned mutable
  closure receiving the button by mutable reference."
- **`inside_bounds`** — Private helper testing
  `bounds.x ≤ x < bounds.x + bounds.width &&
  bounds.y ≤ y < bounds.y + bounds.height`. Mirrors
  `rlvgl/widgets/src/button.rs:60`.

## §5 Frozen decisions

### §5.1 Field set — **Specification Required**

| Field | Type | Notes |
| --- | --- | --- |
| `bounds` | `lvglpp::core::Rect` | Set by ctor; defines the click region. |
| `label` | `lvglpp::widgets::Label` (private) | Owns text + style + draw path. |
| `on_click` | `std::function<void(Button&)>` | Defaults to empty (`!has_value()`). When called, the handler receives a mutable reference to the button itself, allowing in-handler state mutation. |

Public surface:

- `Button(std::string text, Rect bounds)` — primary ctor. Internally
  constructs a Label with the same bounds.
- `void set_text(std::string)` — replace the label text.
- `std::string_view text() const noexcept` — read-only view.
- `Style& style() noexcept`, `const Style& style() const noexcept` —
  expose the inner Label's `style` field for tweaking.
- `Color& text_color() noexcept`, `const Color& text_color() const noexcept` —
  expose the inner Label's `text_color`.
- `void set_on_click(std::function<void(Button&)>)` — register the
  callback.

### §5.2 `Widget` overrides — **Specification Required**

| Method | Body |
| --- | --- |
| `bounds() const` | returns `bounds` field. |
| `draw(Renderer& r) const` | delegates to the inner Label's `draw`. |
| `handle_event(const Event&)` | **§5.3 below.** |
| `clear_region()` | inherits `Widget` default (`std::nullopt`). |

### §5.3 `handle_event` semantics — **Standards Action**

```
bool handle_event(const Event& event) {
    if (event matches PressRelease{x, y} && inside_bounds(x, y)) {
        if (on_click) on_click(*this);
        return true;   // consumed
    }
    return false;       // ignored
}
```

Notes:

- Only **`PressRelease`** triggers the callback — not `PointerUp`,
  not `PressDown`. This matches `rlvgl/widgets/src/button.rs:78` and
  WID-01 Label's documented baseline (Label ignores all events; Button
  consumes only `PressRelease` inside-bounds).
- Out-of-bounds `PressRelease` returns `false` (event passes through
  to siblings via `WidgetNode::dispatch_event`).
- All other event variants return `false`. A `Button` MUST NOT consume
  pointer/key events that aren't `PressRelease`.

This three-line behaviour is **Standards Action**: changing what
events Button consumes affects every cross-language playit fixture
that drives a button.

## §10 Reconciliation vs. adjacent primitives

- **WID-01 Label.** Button composes Label rather than inheriting from
  it — rlvgl does the same (`rlvgl/widgets/src/button.rs:14`). The
  button-as-styled-label idiom is preserved.
- **`Renderer::draw_text` baseline.** Button inherits Label's
  baseline-anchored text via composition. No new text-origin
  conventions.

## §11 Non-goals

- **Hover / press visual feedback.** No state-driven style swap.
  When CORE-03b lands a richer dispatch model, a follow-up sub-phase
  may add a `pressed_style` field; today the visual state is static.
- **Repeat-fire on hold.** Single-shot `PressRelease` only.
- **Disabled state.** Out of scope.

## §12 Acceptance checklist

- [ ] `lvglpp::widgets::Button` exists with the §5.1 fields and
      accessor surface.
- [ ] All four `Widget` overrides per §5.2.
- [ ] `handle_event` matches §5.3 — verified by a test fixture that
      injects `PressRelease` inside / outside bounds and asserts
      callback firing + return value, plus negative tests for `Tick`,
      `PointerDown`, `PointerUp`, `KeyDown`.
- [ ] `draw` delegates to the inner Label — verified by a
      `RecordingRenderer` test that compares Button's calls to a
      bare Label's calls for the same bounds + text.
- [ ] PARITY/LVGL/DELTA cite block.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`
      (`std::function` caveat: heap allocation via type erasure;
      first board target may need a fixed-storage callback variant
      in a follow-up sub-phase).
- [ ] `widgets/STATUS.md` change log records the WID-02 landing.

## §13 Files cited

- `rlvgl/widgets/src/button.rs` (v0.2.0 @ 79f730d)
- `lvglpp/docs/widgets-label/00-label.md`
- `lvglpp/docs/core-event/00-event-surface.md`
- `lvglpp/docs/core-widget/00-widget-tree.md`,
  `lvglpp/docs/core-widget/01-widget-node.md`

## §14 Unblocks

- The first interactive cross-language playit fixture: a
  `T@<tag>:<x>,<y>` from rlvgl's playit driver, parsed by lvglpp's
  PLAYIT-01 parser, dispatched by PLAYIT-04 into a tagged Button,
  which fires its `on_click` and (for example) updates a Label.
- **WID-03** (Checkbox / Switch) — toggle widgets reuse Button's
  PressRelease-inside-bounds idiom with two-state semantics.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Field set (§5.1),
  Widget overrides (§5.2), handle_event semantics (§5.3) frozen.
  Execution unblocked.
- 2026-04-27 — WID-02 execution landed.
  `widgets/include/lvglpp/widgets/button.hpp` +
  `widgets/src/button.cpp`. Test target `lvglpp_widgets_button`
  (7 fixtures) green; embedded posture clean. Wired into the SDL
  example as a tagged Button driven by a startup
  `parse_command("T@ok_button:320,160")`.
