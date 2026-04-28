# 00 — Renderer trait

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-04**.

The key words **MUST**, **MUST NOT**, **SHOULD**, **MAY** in this
chapter are interpreted per RFC 2119 and RFC 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `Renderer` method set, default implementations, blend/blit semantics | `rlvgl/core/src/renderer.rs` (v0.2.0 @ b178cbc) | Canonical. |
| C++ surface (virtual dispatch, signature shape) | this chapter | Normative for lvglpp. |
| Underlying draw substrate | `lvgl/src/draw/lv_draw.h` and the LVGL display driver API | Informative. lvglpp's `Renderer` is a thin abstract base; backends translate to LVGL primitives or raw framebuffers as fits the target. |

## §1 Purpose

Define the abstract base every display / off-screen / simulator
backend implements and every widget's `draw(Renderer&)` accepts.

## §2 Problem statement

Backends (`lvglpp::platform::*`) cannot land before the renderer
contract is stable. rlvgl's `Renderer` trait at
`rlvgl/core/src/renderer.rs:12` exposes four methods —
`fill_rect`, `draw_text`, `blend_rect`, `draw_pixels` — with the
latter two providing default implementations expressed in terms of
the former. Drift on this contract forks every backend.

## §3 Canonical glossary

- **`Renderer`** — Owned by this chapter; mirrored as
  `lvglpp::core::Renderer` (planned at
  `core/include/lvglpp/core/renderer.hpp`). Abstract base class with
  four virtual methods (§5.1).
- **`fill_rect`** — Solid-colour rectangle fill, the most primitive
  drawing operation. Every backend MUST implement.
- **`draw_text`** — UTF-8 baseline-anchored text. Default text
  rendering is the backend's choice; widgets that need a specific
  font use the font helpers (CORE-06).
- **`blend_rect`** — Alpha-blended rectangle. **Default falls back
  to `fill_rect`** (alpha-ignoring). Backends with hardware blending
  override.
- **`draw_pixels`** — Bulk pixel blit. **Default falls back to
  per-pixel `fill_rect`** (slow; correct). Backends with
  bulk-copy / DMA support override.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Renderer` method set | `rlvgl/core/src/renderer.rs:12` | `lvglpp::core::Renderer` (CORE-04 exec). |
| Method-set extension | this chapter — **Standards Action** | rlvgl + lvglpp PR pair, change log first. |
| Default implementations | this chapter §5.2 | Both languages MUST agree on default behaviour or the parity story breaks. |
| Coordinate space | CORE-03 §5.4 | Renderer receives landscape pixel coordinates. |

## §5 Frozen decisions

### §5.1 `Renderer` virtual method set — **Standards Action**

| Method | Signature (C++ idiom) | Default? |
| --- | --- | --- |
| `fill_rect` | `void fill_rect(Rect rect, Color color)` | Pure virtual. |
| `draw_text` | `void draw_text(int32_t x, int32_t y, std::string_view text, Color color)` | Pure virtual. |
| `blend_rect` | `void blend_rect(Rect rect, Color color)` | **Default:** delegates to `fill_rect(rect, color)`. |
| `draw_pixels` | `void draw_pixels(int32_t x, int32_t y, std::span<const Color> pixels, std::uint32_t width, std::uint32_t height)` | **Default:** per-pixel `fill_rect` loop, mirroring `rlvgl/core/src/renderer.rs:33`. |

`Color` and `Rect` are from CORE-03. The `position` argument in rlvgl
is `(i32, i32)`; lvglpp expresses it as two named `int32_t` parameters
(`x`, `y`) for C++ readability. The `text` argument is `&str` in
rlvgl, `std::string_view` in C++ — `borrows` lifetime tied to the
caller per CLAUDE.md ownership rules.

### §5.2 Default implementations are normative

Both default bodies are part of the contract:

- `blend_rect`'s default ignores alpha. Widgets that rely on alpha
  blending MUST tolerate this fallback (or document a backend
  capability requirement).
- `draw_pixels`'s default is O(width × height) `fill_rect` calls; it
  is correct but slow. Backends targeting any practical resolution
  SHOULD override.

A backend implementer MAY override `blend_rect` and `draw_pixels`,
MUST NOT change the default body of either method (since changing
the default would silently affect every other backend that did not
override).

### §5.3 Pixel buffer ordering

For `draw_pixels`: pixels are stored **row-major**, top-to-bottom,
left-to-right. `pixels[y * width + x]` is the colour at position
`(x, y)` relative to the destination origin. The buffer length MUST
be at least `width * height`; backends MAY tolerate a shorter buffer
by skipping out-of-bounds indices (rlvgl does), but consumers
SHOULD NOT rely on that.

## §10 Reconciliation vs. adjacent primitives

- **LVGL display driver API** (`lv_display_t`, `lv_display_set_flush_cb`,
  …). Each lvglpp backend translation unit is responsible for
  translating between the `Renderer` virtual surface and LVGL's
  display-driver callbacks. The renderer abstraction does NOT expose
  LVGL types in its public signatures.
- **DMA2D / hardware blitters.** A backend overriding `draw_pixels`
  to use DMA2D MUST treat the source `pixels` buffer per the
  CLAUDE.md `dma` ownership tag — caller guarantees the buffer is
  inactive on DMA during the call, or wraps the call in an
  `InFlight<>` token (per rlvgl's STM32 discipline).
- **Off-screen renderers.** The `Renderer` interface is the unit a
  test fixture or simulator implements. The host smoke-test suite
  MAY use a `RecordingRenderer` that captures calls instead of
  drawing.

## §11 Non-goals

- **Specific font rendering.** `draw_text` is the renderer's
  primitive; rich text layout, font fallback, and font caching live
  in CORE-06.
- **Partial-update rectangles / dirty regions.** That's a
  compositor concern (renderer-adjacent but separate); document in a
  later chapter when a real renderer needs it.
- **Sub-pixel anti-aliasing.** Not part of the renderer contract.
- **Scene graph / retained-mode rendering.** Renderer is
  immediate-mode by design; retained modes belong in `lvglpp::ui`.

## §12 Acceptance checklist

A conforming CORE-04 execution PR MUST satisfy:

- [ ] `lvglpp::core::Renderer` is an abstract base class with the
      four methods in §5.1.
- [ ] `fill_rect` and `draw_text` are pure virtual.
- [ ] `blend_rect` has a default body that calls `fill_rect`.
- [ ] `draw_pixels` has a default body that loops `fill_rect`
      identically to `rlvgl/core/src/renderer.rs:33`.
- [ ] PARITY/LVGL/DELTA cite block on the public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] A unit test that constructs a `RecordingRenderer` (a tiny
      test-only subclass that records `fill_rect` calls into a
      vector) and verifies `blend_rect` and `draw_pixels` defaults
      decompose into the expected `fill_rect` sequence.
- [ ] `core/STATUS.md` change log records ratification of CORE-04
      execution.

A conforming PR MAY:

- Provide additional non-virtual helpers (e.g.
  `Renderer::draw_hline(...)`) without amending this chapter.

## §13 Files cited

- `rlvgl/core/src/renderer.rs` (v0.2.0 @ b178cbc)
- `rlvgl/core/src/widget.rs` (`Color`, `Rect`)
- `lvglpp/docs/core-widget/00-widget-tree.md`,
  `lvglpp/docs/core-event/00-event-surface.md`,
  `lvglpp/CLAUDE.md`, `lvglpp/docs/std-mapping.md`

## §14 Unblocks

- **PLAT-NN** — every backend implements `Renderer`.
- **WID-01** onwards — `Widget::draw` accepts `Renderer&`.
- **CORE-06** — `BitmapFont::draw_*` calls into `Renderer`.
- **CORE-07** — image-decoder plugins terminate by calling
  `Renderer::draw_pixels`.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Method set (§5.1),
  default implementations (§5.2), pixel buffer ordering (§5.3) all
  frozen with **Standards Action** registration. Execution unblocked.
- 2026-04-27 — CORE-04 execution landed in
  `core/include/lvglpp/core/renderer.hpp`. `Renderer` abstract base
  exposes the four §5.1 methods; `blend_rect` and `draw_pixels`
  default bodies match `rlvgl/core/src/renderer.rs:25, :33` (alpha
  ignored, per-pixel `fill_rect` loop with short-buffer skip). Test
  target `lvglpp_core_renderer` uses a `RecordingRenderer` to
  verify the default decompositions. Compiles clean under
  `LVGLPP_EMBEDDED_POSTURE=ON`.
