# Bug report for rlvgl: LTDC layer-window origin and CFBLL off-by-ones

**For:** rlvgl `v0.2.0` (pin `79f730d`, `tags/v0.1.9-193-g79f730d`)
**From:** lvglpp PLAT-02d display bring-up (STM32H747I-DISCO + NT35510,
adapted command mode), bench session 2026-06-09.
**Status here:** fixed in lvglpp `examples/stm32h747i-disco/disco/display.cpp`
(`setup_ltdc_layer`), deviation documented with a PARITY comment pending
this upstream fix.

## Summary

`setup_ltdc_layer` carries two off-by-one register values, replicated at
three sites. Both deviate from the RM0399 LTDC register contract and the
ST HAL reference (`HAL_LTDC_ConfigLayer`):

1. **Layer window origin is one pixel late in both axes.**
   `x0 = HSW + HBP + 1` / `y0 = VSW + VBP + 1`. RM0399 (`LTDC_LxWHPCR`,
   WHSTPOS description) requires the first visible column to be
   `AHBP + 1`. With `BPCR.AHBP = HSW + HBP − 1`, that is
   `x0 = HSW + HBP` (36 for the 480×800 NT35510 timing), not 37.
   HAL corroboration: `WHSTPOS = WindowX0 + AHBP + 1` with `WindowX0 = 0`.
   Same off-by-one vertically (`y0 = VSW + VBP` = 270, not 271).

   Consequence A: the window stop exceeds the active area —
   `x1 = 37 + 480 − 1 = 516 > AAW = 515`, `y1 = 271 + 800 − 1 = 1070 >
   AAH = 1069` — violating the constraint that the layer window lie
   within the active area.

   Consequence B (bench-visible): the layer is shifted +1 px right/down;
   the framebuffer's **last column and last row are never displayed**
   (clipped outside the active area), and a 1 px background-colour
   (BCCR) hairline appears at the active area's first column/row.

2. **`CFBLR.CFBLL` is `pitch + 7`; RM0399 specifies line length `+ 3`.**
   RM0399 (`LTDC_LxCFBLR`): "these bits define the length of one line of
   pixels in bytes **+ 3**". HAL corroboration:
   `CFBLL = (WindowX1 − WindowX0) * BPP + 3`. Programming `pitch + 7`
   makes the LTDC fetch one extra ARGB8888 pixel per line — the **first
   pixel of the next framebuffer row** — which bleeds at the row seam.
   On the final row the fetch reads 4 bytes past the end of the
   framebuffer allocation (benign on SDRAM here, but an out-of-buffer
   DMA read).

## Affected sites (pin `79f730d`)

| Site | Lines | Layer |
| --- | --- | --- |
| `platform/src/stm32h747i_disco.rs::setup_ltdc_layer` | 1096–1099 (origin), 1112 (CFBLL) | L1, bare-metal |
| `platform/src/display_init.rs::setup_ltdc_layer` | 523–526 (origin), 535 (CFBLL) | L1, Zephyr path |
| `examples/stm32h747i-disco/src/freertos_entry.rs` | 477 (origin), 499 (CFBLL) | **L2**, FreeRTOS overlay |

## Why rlvgl never noticed

The desktop/splash content has no single-pixel fiducial at the
framebuffer edge. A 1 px shift, a clipped final row/column, and a 1 px
seam bleed are imperceptible in that content. lvglpp's first-light
pattern (four colour quadrants + **1 px white border on all four
edges**) made it visible immediately: with the rlvgl values, the border
rendered on only two edges (framebuffer top/left) and a 1 px colour
bleed appeared at the seam; the missing edges were the clipped last
column/row.

## Bench evidence (lvglpp, STM32H747I-DISCO on bench)

- **With rlvgl's values** (`x0 = HSW+HBP+1`, `CFBLL = pitch+7`):
  owner-observed — white border visible only on the framebuffer's
  top/left edges, missing on right/bottom, plus a 1 px colour bleed.
  (Viewing orientation: board-landscape, the joystick/button-natural
  orientation rlvgl's UI uses; the panel scan is native portrait.)
- **With the corrected values** (`x0 = HSW+HBP`, `y0 = VSW+VBP`,
  `CFBLL = pitch+3`), same firmware otherwise: owner-confirmed full
  1 px border on all four edges, no bleed, quadrants unchanged.
- Scan pipeline health identical in both runs (`DSI_WISR = 0x7307`,
  TEIF+ERIF+BUSY, auto-refresh cycling) — the deltas are purely
  geometric, not timing.

Full session log: lvglpp `docs/platform-disco/04-ltdc-dsi-and-panel.md`
§15.11.

## Suggested fix (identical shape at all three sites)

```rust
-        let x0 = hsw + hbp + 1;
+        let x0 = hsw + hbp; // = BPCR.AHBP + 1 (RM0399: WHSTPOS >= AHBP+1)
         let x1 = x0 + (width as u32) - 1;
-        let y0 = vsw + vbp + 1;
+        let y0 = vsw + vbp; // = BPCR.AVBP + 1
         let y1 = y0 + (height as u32) - 1;
```

```rust
-        // CFBLR: bits[28:16]=CFBP (pitch), bits[12:0]=CFBLL (line_len + 7)
-        ... write_volatile((pitch << 16) | (pitch + 7));
+        // CFBLR: bits[28:16]=CFBP (pitch), bits[12:0]=CFBLL (line length + 3, RM0399)
+        ... write_volatile((pitch << 16) | (pitch + 3));
```

After the fix, `x1 == AAW` and `y1 == AAH` exactly (full-screen layer),
and the per-line fetch length equals the visible line.

## Discipline notes

- These values are **not** frozen decisions in
  `docs/disco-platform-guide/05-ltdc-dsi-and-axi-holdoff.md` (checked at
  pin `79f730d`) — no change-log amendment gate; a PR-level fix
  (`DISCO-` prefix at the owner's discretion) suffices.
- Cross-language ordering (lvglpp CLAUDE.md): the lvglpp mirror is
  already corrected with a PARITY-deviation comment in
  `examples/stm32h747i-disco/disco/display.cpp::setup_ltdc_layer`. Once
  the rlvgl fix lands, the lvglpp comment should be downgraded to a
  plain PARITY cite and the `rlvgl/` submodule pin bumped in that same
  PR.
- Verification suggestion for the rlvgl side: temporarily render the
  four-quadrant + 1 px-border fiducial (lvglpp
  `examples/stm32h747i-disco/main_display.cpp::fill_pattern`) — it makes
  this entire bug class visible at a glance, including on the L2
  FreeRTOS overlay.
