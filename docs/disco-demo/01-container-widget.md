<!-- 01-container-widget.md — DEMO-01 concepts doc (normative, thin). -->

# DEMO-01 — `Container` widget

Status: **ratified** (2026-06-07). Thin chapter — the contract is
inherited from DEMO-00; this doc restates it, names files, and lists
acceptance. RFC 2119 keywords per DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical: `rlvgl/widgets/src/container.rs`
(rlvgl `v0.2.0`). LVGL informative: `lvgl/src/core/lv_obj.h`
(`Container` does not shadow `lv_obj`). Ownership/encapsulation deltas
owned by DEMO-00 §5.

## §1 Purpose

Provide the passive grouping/background widget the disco-demo root and
panels need. Wave-A, independent (DEMO-00 §14).

## §2 Frozen contract (mirror)

As defined in `rlvgl/widgets/src/container.rs:9`:

```rust
pub struct Container { bounds: Rect, pub style: Style }
impl Container { pub fn new(bounds: Rect) -> Self }   // style = Style::default()
impl Widget for Container {
    fn bounds(&self) -> Rect;                          // returns self.bounds
    fn draw(&self, r) { draw_widget_bg(r, bounds, &style) }
    fn handle_event(&mut self, _e) -> bool { false }   // passive
}
```

C++ mirror (idiomatic; ownership per DEMO-00 §5 O-1):

```cpp
// widgets/include/lvglpp/widgets/container.hpp
class Container final : public ::lvglpp::core::Widget {
 public:
  explicit Container(::lvglpp::core::Rect bounds) noexcept;
  ::lvglpp::core::Style style{};                       // public, like Label/Checkbox
  [[nodiscard]] ::lvglpp::core::Rect bounds() const override;
  void draw(::lvglpp::core::Renderer& r) const override;   // core::draw_widget_bg
  [[nodiscard]] bool handle_event(const ::lvglpp::core::Event&) override { return false; }
};
```

`draw()` MUST call `::lvglpp::core::draw_widget_bg`
(`core/include/lvglpp/core/draw_helpers.hpp:99`) — already present, used
without modification. No new core surface.

DELTA vs rlvgl: none beyond the language idiom. `style` is a public
member matching the lvglpp `Label`/`Checkbox` convention rather than a
Rust field.

## §3 Files

- `widgets/include/lvglpp/widgets/container.hpp` (new)
- `widgets/src/container.cpp` (new)
- `widgets/tests/container_test.cpp` (new) + CMake wiring
- `widgets/include/lvglpp/widgets/widgets.hpp` — add the include
- `widgets/STATUS.md` — append change-log line

Each new file carries the triangulation cite block
(`// PARITY: rlvgl/widgets/src/container.rs` / `// LVGL: lvgl/src/core/lv_obj.h`
/ `// DELTA: none`).

## §4 Ownership

Container is owned by its `WidgetNode` (`unique_ptr<Widget>`), like every
widget (DEMO-00 §5 O-1). It holds no raw pointers and no LVGL handle —
nothing to encapsulate (O-7 trivially satisfied).

## §5 Acceptance

- [ ] Header + impl compile under embedded-ON and embedded-OFF posture.
- [ ] `draw()` delegates to `core::draw_widget_bg`; no duplicated
      border/fill logic.
- [ ] `handle_event` returns `false` for all variants (parity test).
- [ ] Test mirrors `container.rs` behavior: `bounds()` echo; bg+border
      drawn per `Style`; passive events.
- [ ] `widgets.hpp` re-exports it; Pre-Publish phases 0–3 green.

## §6 Change log

- _drafted_ — DEMO-01 contract restated from DEMO-00 §6/§14.
- **2026-06-07 — ratified.** Owner directed Wave-A ratification; contract
  is a faithful restatement of DEMO-00 §6 (already ratified). Execution
  may proceed.
