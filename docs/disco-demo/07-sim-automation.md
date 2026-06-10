# 07 — Simulator Automation Surface (playit TCP + headless capture)

Chapter status: **draft, ratified 2026-06-10**.
Phase code: **DEMO-07**.

## §0 Authority

- The automation surface (flag names, semantics, `PLAYIT_READY`
  handshake, ASCII luminance mapping) is owned by
  `rlvgl/examples/disco-sim/src/main.rs` (v0.2.0 @ 79f730d) — the
  whole point is that one driver script runs against both sims, so
  lvglpp mirrors it byte-for-byte where implemented and declares
  every gap a named DELTA, never a silent variation.
- The TCP transport contract is owned by `rlvgl/playit/src/tcp.rs`
  (`TcpServerTransport`): single client, loopback only,
  non-blocking accept/read, buffered best-effort write, drop the
  stream (and its buffers) on error/EOF and re-listen.
- The wire protocol carried over the socket is PLAYIT-04b
  (frozen); this chapter adds **no** wire vocabulary.

## §1 Purpose

Make `lvglpp_example_disco_sim` drivable by the same headless
scripts that drive `rlvgl-disco-sim`, so the two implementations of
the same demo can be diffed without eyes or a window: bind a playit
TCP socket, run without SDL, and capture frames as ASCII.

## §5 Frozen decisions

### §5.1 Flag set — **Standards Action** (mirrors rlvgl)

| Flag | Behaviour |
| --- | --- |
| `--screen=WxH` | Override the 800×480 default. |
| `--headless[=path]` / `--headless path` | Render one frame to ASCII at `path` (default `disco-headless.txt`), exit. |
| `--automation-headless` | Run the main loop with no window. Mutually exclusive with `--headless`/PNG (same error message as rlvgl). |
| `--playit-port[=N]` / `--playit-port N` | Bind playit TCP on `127.0.0.1:N` (0 = ephemeral); print `PLAYIT_READY tcp://127.0.0.1:<port>` to stdout, flushed, before the loop starts. |

DELTAs (declared, not silent):

- **PNG capture (positional `file.png`) — not implemented**; exits
  with `PNG capture not implemented in lvglpp_example_disco_sim
  (DEMO-07 §5.1 DELTA)`. Lands when a host PNG writer is worth its
  weight; ASCII is the diff channel meanwhile.
- **`--color` — not implemented** (lvglpp sim renders ARGB8888
  only); same explicit-error treatment.
- **Default transport**: rlvgl uses a null transport unless
  `--playit-port` is given; lvglpp keeps its DEMO-06 stdin
  transport as the windowed default (existing behavior). With
  `--playit-port`, TCP **replaces** stdin.
- **`D` dumps over the socket answer `ERR`** — the lvglpp Executor
  has no framebuffer-reader seam yet (dispatcher returns
  not-implemented). Full-frame ASCII capture covers the visual
  diff; the reader seam is a future PLAYIT phase.

### §5.2 ASCII luminance mapping — **Standards Action**

Byte-for-byte rlvgl's `dump_ascii_frame`: per pixel
`val = (r+g+b)/3`, then `0→' '`, `1..63→'.'`, `64..127→':'`,
`128..191→'*'`, `192..223→'#'`, `224..255→'@'`; rows newline-
terminated. A diff between the two sims' captures must be a
*content* diff, never a mapping diff.

### §5.3 MemoryRenderer — **Specification Required**

Headless modes render through a `MemoryRenderer` (ARGB8888 host
buffer, clipped `fill_rect`, `draw_text` via core `FONT_6X10`) —
the same draw calls the SDL renderer receives, minus SDL. Headless
frames start from the same clear color the windowed loop uses
(`Color{8,10,16}`). Lives in `examples/disco-sim/` until a second
consumer wants it.

### §5.4 TcpServerTransport — **Standards Action** (mirrors tcp.rs)

`lvglpp::playit::TcpServerTransport` in the playit module
(host-only, same CMake gate as StdioTransport): POSIX loopback
listener, non-blocking accept/read, FIFO write buffer flushed
best-effort each write, stream + buffers dropped on EOF/error and
the listener keeps accepting the next client.

## §10 Reconciliation

- DEMO-06 §6 owns the sim's controller/loop shape — unchanged; this
  chapter only adds modes around it. Windowed behavior with no
  flags is bit-identical to DEMO-06.
- The headless step order matches the windowed loop (poll → tick →
  drain → render) so captures and socket state can't skew.

## §12 Acceptance checklist

- [x] `TcpServerTransport` lands in playit (host gate) with PARITY
      cite to tcp.rs.
- [x] All §5.1 flags parse; unknown-combination errors match rlvgl
      text.
- [x] `--playit-port=0 --automation-headless`: `PLAYIT_READY` line,
      then a TCP client ran `?` / `QE:` (hit+miss) / `QB:` /
      `T@disco.main.settings:760,40` with correct responses; STAT
      tick/present advanced between queries. 2026-06-10.
- [x] `--headless` ASCII capture renders the demo tree — 480 rows,
      icon strip visible at the top-right, title text legible in
      the dump (§5.2 mapping). 2026-06-10.
- [x] Same script against `rlvgl-disco-sim` and the lvglpp sim:
      **PARITY CLEAN** (2026-06-10, rlvgl built from the local
      checkout @ c26d3ba, v0.2.1-era — slightly ahead of the
      79f730d pin). 9-step fixture (`?`, QE hit/miss, QB ×2,
      `T@disco.main.settings`, post-tap QE, `?`): every response
      byte-identical including `BOUNDS:730,0,60,82` /
      `BOUNDS:730,82,60,70`; STAT differs only in free-running
      counters (normalized by design).
- [x] Host build + tests stay green (29/29).

## §15 Change log

- 2026-06-10 — Chapter ratified at draft level; §5.1–§5.4 frozen.
  Motivated by the blind-bench posture (owner eyes unavailable):
  the rust sim is the behavioral oracle, and headless parity
  driving needs this surface on the lvglpp side.
- 2026-06-10 — PLAYIT-07a consumed: the §5.1 "D dumps answer ERR"
  DELTA is CLOSED for automation-headless mode (the Executor's new
  FramebufferReader seam + a MemoryRenderer adapter). Windowed mode
  still has no CPU frame (SDL renders directly) — D stays ERR there.
- 2026-06-10 — Cross-sim pixel parity attempted; BLOCKED by an
  rlvgl-side gap, recorded in rlvgl-sim-headless-frame-gap.md:
  rlvgl-disco-sim's FrameMirror is unpopulated in
  --automation-headless (D dumps all 00000000) and its --headless
  ASCII contains icons but no background fill and no text. lvglpp
  renders complete frames in all modes. On the shared content the
  ASCII diff is clean: sidebar icons pixel-identical in position,
  98.1% cell match after normalizing a one-bucket background
  luminance offset, zero 20×20 tiles off by >1.0 mean rank.
