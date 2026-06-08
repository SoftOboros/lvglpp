<!-- 03-event-window.md — DEMO-03 concepts doc (normative, thin). -->

# DEMO-03 — `EventWindow`

Status: **ratified** (2026-06-07). Thin chapter; contract inherited from
DEMO-00. RFC 2119 keywords per DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical: `rlvgl/ui/src/event_window.rs`
(rlvgl `v0.2.0`). Lands in lvglpp `ui/` (already a compiled library
after DEMO-02). The `DiscoController` (DEMO-06) owns one `EventWindow` as
its floating-notification surface.

## §1 Purpose

Mirror the transient overlay that lists recent input events / status
messages, auto-expiring after a tick budget. Wave-B; unblocks DEMO-06.

## §2 Frozen contract (mirror)

As defined in `rlvgl/ui/src/event_window.rs:34` (struct), `:186`
(`impl Widget`), `:279`/`:293` (`EventWindowBuilder`).

Constants (FROZEN — mirror `event_window.rs:18-24`):
`MAX_LINES = 10`, `CLEAR_FRAMES = 3`, `DEFAULT_EXPIRE_TICKS = 60`.

App-relevant surface to mirror (the part `DiscoController` and the host
loop touch):

```rust
EventWindowBuilder::new(font)            // :295
    .expire_ticks(u32).bg_color(Color).border_color(Color)
    .radius(u8).width(i32).center(screen_w, screen_h)   // :319-355
    .build() -> EventWindow                              // :356

EventWindow:
    push_event(String)                  // :173 — append, cap to MAX_LINES, show
    is_visible/toggle_visible/hide      // :66/:90/:102
    set_enabled/is_enabled/entry_count  // :82/:76/:71
impl Widget:
    bounds() / draw() / handle_event()  // :186+ — Tick ages entries,
                                        //   expires past expire_ticks, hides
                                        //   when empty
    clear_region() -> Option<Rect>      // paint-over for CLEAR_FRAMES after hide
```

C++ mirror (`ui/`, ownership per DEMO-00 §5 — a `core::Widget` owned by
the tree; the controller holds a non-owning `EventWindow*` observer):

```cpp
// ui/include/lvglpp/ui/event_window.hpp
namespace lvglpp::ui {
class EventWindow final : public core::Widget {
 public:
  void push_event(std::string text);
  [[nodiscard]] bool is_visible() const noexcept;
  void toggle_visible() noexcept;
  void hide() noexcept;
  void set_enabled(bool) noexcept;
  [[nodiscard]] bool is_enabled() const noexcept;
  [[nodiscard]] std::size_t entry_count() const noexcept;
  // Widget overrides: bounds(), draw(Renderer&), handle_event(Event&),
  // clear_region().
};
class EventWindowBuilder {
 public:
  explicit EventWindowBuilder(const core::BitmapFont& font) noexcept;
  EventWindowBuilder& expire_ticks(std::uint32_t) noexcept;
  EventWindowBuilder& bg_color(core::Color) noexcept;
  EventWindowBuilder& border_color(core::Color) noexcept;
  EventWindowBuilder& radius(std::uint8_t) noexcept;
  EventWindowBuilder& width(std::int32_t) noexcept;
  EventWindowBuilder& center(std::int32_t screen_w, std::int32_t screen_h) noexcept;
  [[nodiscard]] EventWindow build() const;
};
}
```

Behavior FROZEN: `push_event` appends an entry (text + age 0), trims the
front past `MAX_LINES`, sets `visible`. `handle_event(Tick)` increments
every entry's age and drops those past `expire_ticks`; when the list
empties, hides (starting the `CLEAR_FRAMES` clear countdown).
`handle_event(PressRelease/...)` does not consume. `bounds()` collapses
to zero when hidden; `clear_region()` returns the panel rect while
`clear_countdown > 0`. `draw()` fills a rounded rect bg + border and
draws up to `MAX_LINES` text lines, newest at the bottom.

DELTA vs rlvgl (DEFERRED — informative): the board-render telemetry and
overlay-pipeline hooks are **out of scope** for the host shell and are
NOT mirrored: `dma2d_mode`, `frozen`/`set_frozen`, `diag_state`,
`draw_seq`, `last_draw_lines`, `for_each_visible`, and the
`font()`/`padding()`/`line_height()` accessors. They serve the board
DMA2D overlay pipeline (PLAT-02e); a later chapter restores them when
that lands. Border uses `core::draw_border_straight` (rounded corners
deferred to CORE-04b, consistent with the rest of lvglpp); bg uses
`core::fill_rounded_rect` (radius-ignoring shim today). `String` →
`std::string`; `&'static BitmapFont` → `const core::BitmapFont&`
(borrows; must outlive the window).

## §3 Files

- `ui/include/lvglpp/ui/event_window.hpp` (new) + `ui/src/event_window.cpp` (new)
- `ui/tests/event_window_test.cpp` (new) + register `lvglpp_ui_event_window`
- `ui/CMakeLists.txt` — add `src/event_window.cpp`
- `ui/include/lvglpp/ui/ui.hpp` — add the include
- `ui/STATUS.md` — change-log + Definitions

Cite block: `// PARITY: rlvgl/ui/src/event_window.rs (v0.2.0 @ 79f730d).`
/ `// LVGL: N/A (composite overlay).` / `// DELTA: board-render telemetry
deferred; see DEMO-03 §2.`

## §4 Ownership

`EventWindow` is a `core::Widget` owned by the tree (DEMO-00 §5 O-1). It
owns its `std::vector` of entries (value) and **borrows** the
`BitmapFont` (must outlive it). It holds no LVGL handle. The controller
observes it via a raw `EventWindow*` (O-3) to call `push_event` /
`toggle_visible`.

## §5 Acceptance

- [ ] Builder produces a centered window with the given width / colors /
      expiry.
- [ ] `push_event` appends, caps at `MAX_LINES` (oldest dropped), sets
      visible.
- [ ] N `Tick`s past `expire_ticks` expire entries; emptying hides and
      arms `clear_region` for `CLEAR_FRAMES`.
- [ ] `bounds()` zero when hidden; non-consuming on pointer/key events.
- [ ] Deferred telemetry members are absent (scope boundary held).
- [ ] `ui.hpp` re-exports; Pre-Publish 0–3 green; embedded-posture clean.

## §6 Change log

- _drafted_ — DEMO-03 contract restated from DEMO-00 §6/§14;
  board-render telemetry explicitly deferred.
- **2026-06-07 — ratified.** Owner directed Wave-B; app-relevant
  EventWindow surface frozen, board DMA2D/telemetry hooks deferred to a
  PLAT-02e-era chapter. Execution may proceed.
