# Note for rlvgl: disco-sim headless frame surface is incomplete

**For:** rlvgl (observed on local checkout @ c26d3ba, v0.2.1-era;
sim code matches v0.2.0 @ 79f730d).
**From:** lvglpp DEMO-07/PLAYIT-07a cross-sim parity work,
2026-06-10.

## Symptoms (rlvgl-disco-sim, macOS aarch64, release)

1. `--automation-headless --playit-port=0`: `D` dumps return rows of
   `00000000` — the `FrameMirror` the executor reads is never
   populated in this mode (`run_automation_headless` steps the
   runtime but nothing renders into the mirror).
2. `--headless=<path>`: the ASCII frame contains the right-edge icon
   strip only. No background fill (background cells read luminance
   0, i.e. literal zero bytes) and **no text at all** — headline,
   subtitle, footer are absent. The icon blit path writes the
   mirror; the bg/text path appears to render only via wgpu.

## Why it matters

The automation surface exists so one script drives every target.
Protocol-level parity vs lvglpp's sim is clean (queries, bounds,
taps byte-identical), but pixel-level parity through `D` dumps and
ASCII capture can't be closed while the headless mirror is partial.
On shared content the sims already agree: icon strip pixel-identical
in position; 98.1% ASCII cell match after normalizing a one-bucket
background-luminance offset.

## Suggested fix shape

Route the full frame (bg fill + text + blits) through the
`FrameMirror` whenever it backs `--headless` or
`--automation-headless`, or composite the wgpu output back into the
mirror before `emit_dump_if_ready` / `dump_ascii_frame` read it.

Repro scripts: lvglpp `/tmp/dump_parity.py` + the DEMO-07 §12 notes.
