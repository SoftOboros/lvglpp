<!-- 02-ui-draw-helpers.md — DEMO-02 concepts doc (normative, thin). -->

# DEMO-02 — UI draw helpers (`draw_panel_header`, `panel_close_hit`)

Status: **ratified** (2026-06-07). Thin chapter; contract inherited from
DEMO-00. RFC 2119 keywords per DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical: `rlvgl/ui/src/draw_helpers.rs`
(rlvgl `v0.2.0`). These are the two helpers `DashboardPanel` (DEMO-05)
and `EventWindow` (DEMO-03) consume; they live in lvglpp `ui/`, moving it
off stub alongside DEMO-03.

## §1 Purpose

Mirror the panel-header renderer and its close-button hit test so panels
draw identically to rlvgl. Wave-A, independent; unblocks DEMO-03/05.

## §2 Frozen contract (mirror)

As defined in `rlvgl/ui/src/draw_helpers.rs:41,98`:

```rust
pub fn draw_panel_header(
    renderer, bounds: Rect, accent: Color, title: &str,
    font: &BitmapFont, title_color, close_color, divider_color: Color,
) -> i32                  // returns body-content start y

pub fn panel_close_hit(bounds: Rect, x: i32, y: i32) -> bool
```

Behavior (FROZEN — mirror byte-for-byte, `draw_helpers.rs:41`–`106`):
accent bar `72×8 @ +PANEL_PADDING` (radius 4), title at
`+PANEL_PADDING, +PANEL_PADDING+20`, close `"X"` near the right edge,
divider at `title_y + font.scaled_height() + 12`, returns `div_y + 12`.
Hit test: a `CLOSE_SIZE` square anchored at the panel's top-right inside
padding. Constants `PANEL_PADDING`, `CLOSE_SIZE` mirror the rlvgl values
(FROZEN; read from `draw_helpers.rs` head).

C++ mirror (free functions, `ui/`):

```cpp
// ui/include/lvglpp/ui/draw_helpers.hpp
namespace lvglpp::ui {
std::int32_t draw_panel_header(
    core::Renderer& r, core::Rect bounds, core::Color accent,
    std::string_view title, const core::BitmapFont& font,
    core::Color title_color, core::Color close_color,
    core::Color divider_color);
[[nodiscard]] bool panel_close_hit(core::Rect bounds,
                                   std::int32_t x, std::int32_t y) noexcept;
}
```

Dependencies, used without modification:
`core::fill_rounded_rect` + `core::Renderer::fill_rect`
(`core/draw_helpers.hpp:74`, `renderer.hpp`) and
`core::BitmapFont::draw_str` (`core/font.hpp`).

DELTA vs rlvgl: free functions in `lvglpp::ui` instead of `rlvgl_ui`;
`&str` → `std::string_view`. No behavioral delta.

## §3 Files

- `ui/include/lvglpp/ui/draw_helpers.hpp` (new)
- `ui/src/draw_helpers.cpp` (new) + `ui/` CMake gains a compiled lib
  (currently INTERFACE-only)
- `ui/tests/draw_helpers_test.cpp` (new) — mirror the rlvgl
  `CountRenderer` test (`draw_helpers.rs` `#[cfg(test)]`) and the
  close-hit boundary cases
- `ui/include/lvglpp/ui/ui.hpp` — add the include
- `ui/STATUS.md` — note `ui/` moves from INTERFACE stub to compiled

Triangulation cite block on each new file
(`// PARITY: rlvgl/ui/src/draw_helpers.rs` / `// LVGL: N/A (composite UI
helper)` / `// DELTA: free fn, string_view`).

## §4 Ownership

Pure functions over a borrowed `Renderer&` and a borrowed
`const BitmapFont&` (DEMO-00 §5: renderer/font borrowed for the call
only; never retained). No allocation, no handles.

## §5 Acceptance

- [ ] `ui/` builds as a compiled library; `ui.hpp` re-exports the
      helpers.
- [ ] `draw_panel_header` returns the same body-start `y` as rlvgl for a
      fixed `bounds`/font (golden value in test).
- [ ] `panel_close_hit` matches rlvgl on the corner, just-inside, and
      just-outside cases.
- [ ] Constants equal the rlvgl `PANEL_PADDING`/`CLOSE_SIZE`.
- [ ] Pre-Publish phases 0–3 green.

## §6 Change log

- _drafted_ — DEMO-02 contract restated from DEMO-00 §6/§14.
- **2026-06-07 — ratified.** Owner directed Wave-A ratification; faithful
  restatement of DEMO-00 §6. Execution may proceed.
