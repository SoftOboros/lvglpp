# 00 — Generic Linux backend: fbdev display + evdev input

Chapter status: **draft, ratified 2026-06-10**.
Phase code: **PLAT-LNX** (see §15 for the numbering decision).

## §0 Authority

- Backend shape is owned by `rlvgl/platform/src/linux_fbdev.rs` and
  `rlvgl/platform/src/linux_evdev.rs` (v0.2.0 @ 79f730d) — the
  generic layer, NOT the BeagleBone example (whose devmem/EDMA/
  rotation logic is board-specific and out of scope here).
- ioctl/structure vocabulary is owned by the Linux UAPI headers
  (`<linux/fb.h>`, `<linux/input.h>`).
- Ticket contract: generic framework only — no application logic,
  no processor specifics, no board specifics.

## §1 Purpose

A generic Linux display+input prong: open a framebuffer device,
mmap it, flush dirty rects in the device's pixel format; open an
evdev node, translate `input_event` streams (single-touch and MT
protocol B) into `core::Event`s. First consumer is an **external
repo on fbdev**; PLAT-03 (BBB) consumes this chapter later and adds
only board specifics on top.

## §5 Frozen decisions

### §5.1 Surface — **Standards Action** (mirrors linux_fbdev.rs / linux_evdev.rs)

- `lvglpp::platform::lnx::FbdevDisplay`:
  `open(const char* path)` (default `/dev/fb0`) →
  FBIOGET_VSCREENINFO + FBIOGET_FSCREENINFO + `mmap(MAP_SHARED)`;
  accessors `width()/height()/bits_per_pixel()`; `flush(Rect,
  span<const core::Color>)` writes the rect row-by-row honouring
  `line_length`, converting ARGB→device format.
- Pixel formats: 32bpp BGRA8888, 24bpp BGR888, 16bpp RGB565 —
  exactly the rlvgl set (linux_fbdev.rs:233–262); anything else is
  an open error.
- `lvglpp::platform::lnx::EvdevInput`: `open(const char* path)`
  (default `/dev/input/event0`, O_NONBLOCK); reads 24-byte
  `input_event` records; single-touch ABS_X/Y + BTN_TOUCH and MT
  protocol B (ABS_MT_SLOT / TRACKING_ID / POSITION_X/Y, 10 slots);
  EV_SYN/SYN_REPORT commits accumulated state to
  `PointerDown/PointerMove/PointerUp`; `poll()` →
  `std::optional<core::Event>`.
- Dirty-rect coalescing is the CALLER's job (rlvgl parity — the
  driver flushes what it is given).
- Namespace `lnx` not `linux`: `linux` is a predefined macro under
  `-std=gnu*` dialects and a reserved identifier risk.

### §5.2 Build gating — **Specification Required**

`LVGLPP_PLATFORM_LINUX_FBDEV` (default OFF), valid only when
`CMAKE_SYSTEM_NAME STREQUAL "Linux"` — configuring it elsewhere is
a `FATAL_ERROR`. Sources live at
`platform/src/lnx/{fbdev_display,evdev_input}.cpp`, headers at
`platform/include/lvglpp/platform/lnx/`. Host-SDL and Linux prongs
may coexist in one build (no exclusivity rule needed yet).

### §5.3 Smoke example — **Specification Required**

`examples/linux-fbdev-smoke`: renders the WID-05/WID-06 conformance
composition (List + Image, same tree as the gallery sim) to the
framebuffer once, then polls evdev and exits on first touch or
SIGINT. Env overrides `LVGLPP_FB` / `LVGLPP_INPUT` (mirrors rlvgl's
`RLVGL_FB`/`RLVGL_INPUT`). Renders via an offscreen ARGB buffer +
`flush` of the full frame (dirty-rect demos are a consumer concern).

## §10 Reconciliation

- **PLAT-03 (BBB)**: consumes this chapter's two classes and adds
  board specifics in its own family; nothing here may grow
  BBB-isms. `platform/STATUS.md` roadmap gains a PLAT-LNX line
  between PLAT-02 and PLAT-03 recording the dependency.
- **PLAT-01 host SDL**: sibling prong; the Renderer-side seam is
  identical (a memory frame flushed to a device instead of SDL).
- **Conformance bar (ticket)**: the fbdev smoke renders on a real
  Linux console. No Linux host is attached to this bench —
  execution lands compile-clean (Linux-gated) and the console run
  is the external consumer's leg. Named blocker recorded in
  `platform/STATUS.md`.

## §12 Acceptance checklist

- [x] `FbdevDisplay` + `EvdevInput` per §5.1, ownership comments +
      PARITY cites; fds RAII-owned, mmap unmapped in destructor.
- [x] CMake gate per §5.2; macOS/embedded configs untouched.
- [ ] `examples/linux-fbdev-smoke` builds under a Linux configure
      (compile gate); renders on a Linux console (external leg —
      blocker until a Linux host is available).
- [x] Unit-testable pure parts (pixel-format pack, input_event
      state machine) covered by host tests with synthetic streams
      (no /dev access in tests).

## §15 Change log

- 2026-06-10 — Chapter ratified at draft level. **Numbering/folder
  decision (maintainer call, per ticket):** phase code `PLAT-LNX`,
  folder `docs/platform-linux/`. Rationale: the numbered PLAT-NN
  sequence is per-board; this chapter is a cross-cutting OS layer
  consumed by boards (first: an external fbdev repo; later:
  PLAT-03 BBB), so it gets a lettered code outside the board
  sequence rather than stealing PLAT-03's slot. Note: rlvgl's
  generic layer is fbdev+evdev — `platform/STATUS.md`'s earlier
  "BBB (Linux DRM)" wording was aspirational; DRM, if it ever
  lands, is a new backend class in this family, not a rename.
