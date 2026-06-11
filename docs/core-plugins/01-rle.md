# 01 — RLE decoder plugin

Chapter status: **draft, ratified 2026-06-10**.
Phase code: **CORE-07n** — the first executed CORE-07 plugin
sub-phase.

## §0 Authority

- Decoder semantics, frozen constants, and error set are owned by
  `rlvgl/rlvgl-decomp/src/lib.rs` (v0.2.0 @ 79f730d), already
  mirrored by DEMO-04 (`docs/disco-demo/04-rle-icons.md`). This
  sub-phase moves that surface behind the CORE-07 gating mechanism;
  it changes NO decode behaviour.
- Gating mechanism owned by CORE-07 (`00-plugin-surface.md` §5.2).

## §5 Frozen decisions

### §5.1 Slot — **Standards Action** (amends CORE-07 §5.1)

| Slot | rlvgl gate | CMake option | Embedded? | Sub-phase |
| --- | --- | --- | --- | --- |
| RLE | the `rlvgl-decomp` **crate boundary** (no Cargo feature — presence of the dep is the gate) | `LVGLPP_CORE_RLE` | **Yes** (allocation-free, freestanding-clean; proven on the CM7 bench) | CORE-07n |

Adapted authority note: rlvgl gates plugins by Cargo feature;
rlvgl-decomp predates `plugins/mod.rs` and is gated by being a
separate crate. The lvglpp slot maps to that crate boundary — no
rlvgl-side change is required, so cross-language ordering is
satisfied trivially (rlvgl artifact already exists and is frozen).

### §5.2 File move — **Specification Required**

`core/include/lvglpp/core/rle.hpp` →
`core/include/lvglpp/core/plugins/rle.hpp` +
`core/src/rle.cpp` → `core/src/plugins/rle.cpp`, per CORE-07 §5.2,
including the `#error`-on-unconfigured guard. The old header
remains as a **deprecated forwarder** (one release) so DEMO-04
consumers keep compiling; it `#include`s the plugin header and
carries a deprecation comment pointing here.

### §5.3 Default + in-repo consumers — **Specification Required**

`LVGLPP_CORE_RLE` defaults OFF (CORE-07 §5.3). The in-repo
consumers (disco-demo app + the conformance gallery) force it ON
from `examples/CMakeLists.txt` when examples are enabled — this
honours §5.3's intent (the *bare* `cmake -S . -B build` library
build pulls nothing) while keeping example DX one-step. RLE has no
external dependency, so the cost of the forced-on path is one TU.

## §12 Acceptance checklist

- [x] Files moved per §5.2; old header forwards with deprecation
      note; `#error` guard verified by a negative compile check
      (manual, recorded here): including the plugin header without
      the option errors at preprocessor time.
- [x] CORE-07 §5.1 table amended (this doc + change-log entry
      there).
- [x] Host tests + disco cross-build stay green (decode behaviour
      unchanged — DEMO-04's tests are the regression net).
- [x] `widgets/STATUS.md` WID-06 blocker line moved to its change
      log (CORE-07 first decoder landed).

## §15 Change log

- 2026-06-10 — Sub-phase ratified; slot added by CORE-07 §5.1
  amendment; file move + forwarder + default policy frozen.
