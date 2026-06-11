# 00 — Image

Chapter status: **draft, ratified 2026-06-10**.
Phase code: **WID-06**.

## §0 Authority

- Widget semantics owned by `rlvgl/widgets/src/image.rs` (v0.2.0 @
  79f730d). Parity by design; C++ idiom adaptations only.
- The asset-decode path is owned by CORE-07n (RLE plugin) — the
  widget itself performs NO decoding (image.rs parity: pixels are
  caller-provided).
- Generic framework only: no application logic, no processor
  specifics (ticket contract).

## §3 Canonical glossary

- **`Image`** — As defined in `rlvgl/widgets/src/image.rs:9`;
  mirrored as `widgets/include/lvglpp/widgets/image.hpp`.
- **Asset-loader seam** — the `widgets/STATUS.md` WID-06 blocker:
  satisfied by Image borrowing a decoded `core::Color` buffer that
  a CORE-07 plugin (first: RLE, CORE-07n) produced. No loader
  indirection inside the widget (image.rs parity — rlvgl decodes
  upstream and passes a slice).

## §5 Frozen decisions

### §5.1 Surface — **Standards Action** (mirrors image.rs:9–28)

- Fields: private `bounds_`, `width_`, `height_`, `pixels_`
  (`std::span<const core::Color>` — **borrows**; caller guarantees
  the buffer outlives the widget, mirroring rlvgl's `&'a [Color]`);
  public `style`.
- `Image(core::Rect bounds, std::int32_t width, std::int32_t
  height, std::span<const core::Color> pixels)`.
- Accessors: `width()`, `height()`, `pixels()` (borrows).

### §5.2 Draw — **Standards Action** (image.rs:36–44)

1. `draw_widget_bg(renderer, bounds_, style)`.
2. `renderer.draw_pixels(bounds.x, bounds.y, pixels_, width,
   height)` — the Renderer default per-pixel path or a backend
   override (DMA2D etc.) decides the emission strategy.

### §5.3 Events — **Standards Action**

`handle_event` always returns false (image.rs:46–49 —
non-interactive).

### §5.4 Golden test — **Specification Required**

Mirror `rlvgl/widgets/tests/golden_image.rs`: 2×2 buffer
[red, green, blue, white] drawn through a memory renderer
reproduces the buffer exactly at the widget origin.

## §10 Reconciliation

- **CORE-07n** supplies the first decoded-pixel producer (RLE);
  the conformance composition decodes an RLEC blob via the plugin
  and hands the buffer to Image — closing the `widgets/STATUS.md`
  "Asset-loader seam (WID-06)" blocker.
- Lifetime discipline: the borrowed span gets the same treatment as
  rlvgl's examples (static or otherwise outliving storage owned by
  the composition, never by the widget).

## §12 Acceptance checklist

- [x] Image per §5.1–§5.3 with ownership comments + PARITY cites.
- [x] Golden 2×2 host test per §5.4.
- [x] Gallery sim composition shows an RLE-decoded Image; headless
      `D` dumps return expected pixels (conformance bar, shared
      with WID-05).
- [x] `-Werror` + embedded-posture compile clean.

## §15 Change log

- 2026-06-10 — Chapter ratified at draft level; §5.1–§5.4 frozen
  from image.rs at the v0.2.0 pin.
