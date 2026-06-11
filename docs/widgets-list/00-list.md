# 00 — List

Chapter status: **draft, ratified 2026-06-10**.
Phase code: **WID-05** (List half; `Container` already landed via
DEMO-01 — see §10).

## §0 Authority

- Widget semantics, field set, layout constants, and event handling
  are owned by `rlvgl/widgets/src/list.rs` (v0.2.0 @ 79f730d) —
  parity by design; lvglpp adapts only C++ idiom (per CLAUDE.md
  § "Cross-Project Parity With rlvgl").
- Draw-helper vocabulary (`draw_widget_bg`) is owned by CORE-04a.
- No application logic and no processor specifics belong in this
  chapter or its execution (ticket contract).

## §3 Canonical glossary

- **`List`** — As defined in `rlvgl/widgets/src/list.rs:10`;
  mirrored here as `widgets/include/lvglpp/widgets/list.hpp`.
- **`ROW_HEIGHT`** — 16 px, hardcoded in list.rs:49/:74. Frozen.
- **`index_at`** — As defined in list.rs:48 (`(y − bounds.y) / 16`
  with bounds checks); mirrored as a private helper.

## §5 Frozen decisions

### §5.1 Surface — **Standards Action** (mirrors list.rs)

- Fields: private `bounds_`, `items_` (`std::vector<std::string>`),
  `selected_` (`std::optional<std::size_t>`); public `style`,
  `text_color` (member-public like Label/Container — rlvgl `pub`).
- `explicit List(core::Rect bounds)`.
- `add_item(std::string)` (consumes), `items()` →
  `std::span<const std::string>` (borrows),
  `selected()` → `std::optional<std::size_t>`.

### §5.2 Draw — **Standards Action** (list.rs:71–85)

1. `draw_widget_bg(renderer, bounds_, style)`.
2. Per item i: `draw_text(bounds.x + 2, bounds.y + i*16 + 16, …)` —
   baseline anchor, same +16 quirk as rlvgl.
3. Selected item text colour = `style.border_color`, others =
   `text_color`; both via `.with_alpha(style.alpha)`.
4. No clipping, no scroll state — every item draws (rlvgl parity;
   scrolling is a future rlvgl-first amendment).

### §5.3 Events — **Standards Action** (list.rs:88–103)

Only `PressRelease` is consumed: inside x-range, `index_at(y)`
valid → `selected_ = idx`, return true; everything else false.
No per-item callbacks (selection state only — observers read
`selected()`).

## §10 Reconciliation

- WID-05 in `widgets/STATUS.md` bundles `Container` + `List`;
  `Container` landed 2026-06-08 under DEMO-01
  (`docs/disco-demo/01-container-widget.md`). This chapter covers
  the remaining List half; STATUS.md change log records the split.
- Conformance (ticket): List is usable on the host sim and verified
  via playit headless `D` dumps (DEMO-07 harness) — see the
  widget-gallery conformance example, §12.

## §12 Acceptance checklist

- [x] `widgets/{include,src}` List per §5.1–§5.3 with ownership
      comments + PARITY cites.
- [x] Host unit tests mirroring rlvgl's `list_event.rs` (boundary
      y=16 → item 1), `list_draw_selected.rs` (selected item uses
      `style.border_color`), `golden_list.rs` (bg fill present).
- [x] Gallery sim composition renders a List; headless `D` dumps
      return expected content (conformance bar, shared with WID-06).
- [x] `-Werror` clean, embedded-posture compile clean
      (`std::string` host-friendliness caveat shared with WID-01).

## §15 Change log

- 2026-06-10 — Chapter ratified at draft level; §5.1–§5.3 frozen
  from list.rs at the v0.2.0 pin. Scroll support explicitly out of
  scope until rlvgl grows it (rlvgl-first ordering).
