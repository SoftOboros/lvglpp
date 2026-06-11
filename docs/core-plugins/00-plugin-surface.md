# 00 — Plugin surface

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-07**.

The key words **MUST**, **SHOULD**, **MAY** in this chapter are
interpreted per RFC 2119 and RFC 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| Per-plugin gating mechanism in rlvgl | `rlvgl/core/src/plugins/mod.rs` (v0.2.0 @ 79f730d) — `#[cfg(feature = "<name>")]` blocks | Canonical for the *intent* of optional, individually gated plugins. |
| C++ gating mechanism (CMake options + preprocessor guards) | this chapter | Normative for lvglpp. |
| Per-plugin decoder semantics | per-plugin sub-phase concepts docs (CORE-07a, CORE-07b, …) | Out of scope for this chapter. |

## §1 Purpose

Define **how** lvglpp adds optional decoders / generators (PNG,
JPEG, GIF, QR, Lottie, Canvas, FATFS, fontdue, pinyin, NES, APNG)
to the core surface without:

1. Forcing every consumer to compile every dependency.
2. Polluting the `lvglpp::core` ABI with conditional types.
3. Drifting from rlvgl's per-feature gating model.

This chapter does **not** define what any specific decoder does;
that's per-plugin sub-phase territory.

## §2 Problem statement

rlvgl plugin gating uses Cargo features (`#[cfg(feature = "png")]`).
lvglpp uses CMake options + preprocessor guards. The mechanism MUST
ensure:

- A consumer that does NOT enable a plugin pays zero code-size /
  compile-time cost for it.
- A consumer that DOES enable a plugin gets a typed surface that
  compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON` (most plugins
  are host-only today; the embedded-friendly ones must be marked
  as such).
- New plugins land additively without touching `lvglpp::core`'s
  unconditional surface.

## §3 Canonical glossary

- **Plugin** — A self-contained translation unit under
  `core/src/plugins/<name>.cpp` and a public header at
  `core/include/lvglpp/core/plugins/<name>.hpp`. Selected by
  `LVGLPP_CORE_<NAME>` CMake option (per
  `core/OPTIONS.md`). Mirrors rlvgl's `plugins/<name>.rs` +
  `feature = "<name>"`.
- **Plugin registry** — A small static lookup keyed by extension /
  MIME / format identifier that returns a typed plugin handle when
  available. Optional; not every plugin needs to register itself
  there (some are direct-call APIs).
- **Host-only plugin** — A plugin whose dependency tree is
  unsuitable for embedded posture. MUST be guarded by
  `#if !defined(LVGLPP_EMBEDDED_POSTURE)` so cross-builds skip it
  cleanly.
- **Embedded-friendly plugin** — A plugin that compiles and runs
  under `LVGLPP_EMBEDDED_POSTURE=ON`. MUST NOT pull in `<iostream>`,
  `<fstream>`, `<thread>`, etc. (per `docs/std-mapping.md`
  § "Freestanding subset").

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Plugin slot identifiers (PNG, JPEG, GIF, QR, Lottie, Canvas, FATFS, fontdue, pinyin, NES, APNG) | `rlvgl/core/src/plugins/mod.rs` (canonical) | `LVGLPP_CORE_<NAME>` CMake options. |
| New plugin slot | this chapter — **Standards Action** | rlvgl + lvglpp PR pair: rlvgl adds `feature = "<name>"`, lvglpp adds `LVGLPP_CORE_<NAME>` and a `core/plugins/<name>.{hpp,cpp}` skeleton. |
| Per-plugin decoder semantics | per-plugin sub-phase concepts docs | Each sub-phase amends its own change log; this chapter does NOT enumerate decoder behaviour. |

## §5 Frozen decisions

### §5.1 Plugin slot table — registration policy: **Standards Action**

The set of plugin slots is frozen at the rlvgl-side parity baseline
(v0.2.0). Adding a slot requires an amendment to this chapter and a
matching change to `rlvgl/core/src/plugins/mod.rs` per CLAUDE.md
§ "Cross-language change ordering".

| Slot | Rust feature | CMake option | Embedded? | Sub-phase |
| --- | --- | --- | --- | --- |
| PNG | `png` | `LVGLPP_CORE_PNG` | No (host-only in rlvgl today) | CORE-07a |
| JPEG | `jpeg` | `LVGLPP_CORE_JPEG` | No (host-only) | CORE-07b |
| GIF | `gif` | `LVGLPP_CORE_GIF` | No (rlvgl pulls `std`) | CORE-07c |
| APNG | `apng` | `LVGLPP_CORE_APNG` | No | CORE-07d |
| QR generator | `qrcode` | `LVGLPP_CORE_QRCODE` | No (host-only in manifest) | CORE-07e |
| Canvas / embedded-graphics | `canvas` | `LVGLPP_CORE_CANVAS` | Yes (`no_std`-friendly) | CORE-07f |
| FATFS | `fatfs` | `LVGLPP_CORE_FATFS` | No (rlvgl pulls `std`) | CORE-07g |
| fontdue | `fontdue` | `LVGLPP_CORE_FONTDUE` | No (host-only) | CORE-07h |
| Lottie API surface | `lottie` | `LVGLPP_CORE_LOTTIE` | API-only (yes) | CORE-07i |
| Lottie backend (rlottie) | `lottie_backend` | `LVGLPP_CORE_LOTTIE_BACKEND` | No | CORE-07j |
| pinyin | `pinyin` | `LVGLPP_CORE_PINYIN` | No (rlvgl pulls `std`) | CORE-07k |
| NES emulator hook | `nes` | `LVGLPP_CORE_NES` | No | CORE-07l |
| dash-lottie (lite) | (no rlvgl feature; module-level) | `LVGLPP_CORE_DASH_LOTTIE` | TBD per sub-phase | CORE-07m |

Slots are listed for completeness; **none of these have execution PRs
in lvglpp today**. Each sub-phase ratifies its own concepts doc and
acceptance.

### §5.2 Per-slot file layout — **Standards Action**

When sub-phase CORE-07x lands a plugin, the file shape MUST be:

```
core/include/lvglpp/core/plugins/<name>.hpp   # public surface
core/src/plugins/<name>.cpp                   # implementation
docs/core-plugins/<NN>-<name>.md              # concepts doc
```

The `<name>.hpp` header MUST be entirely behind:

```cpp
#if !defined(LVGLPP_CORE_<NAME>) || LVGLPP_CORE_<NAME> == 0
#error "<name>.hpp included without LVGLPP_CORE_<NAME> enabled"
#endif
```

so a misconfigured consumer fails at preprocessor time, not link
time. The `<name>.cpp` is added to `lvglpp_core` only when the option
is on (CMake `if(LVGLPP_CORE_<NAME>)` block).

### §5.3 Default state

Every `LVGLPP_CORE_<NAME>` defaults to `OFF`. A consumer opts in
explicitly. The lvglpp default build (`cmake -S . -B build`) MUST
NOT pull any plugin dependency.

### §5.4 No global registry today

This chapter does **not** mandate a runtime plugin registry (a
`std::map` from extension to decoder). Plugins are direct-call APIs
discoverable by `#include`. A registry MAY land later if the call
sites multiply enough to warrant one; that lands as a separate
sub-phase.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl-creator (Rust).** Generated assets land as static C++
  arrays in a TU under the consumer application; plugins decode
  *runtime* assets (PNG image loaded at app boot, etc.). The two
  paths are orthogonal — plugins do not depend on creator, and
  creator does not depend on plugins.
- **`lvglpp::widgets::Image`** (WID-06, deferred). Image rendering
  consults the plugin slots — `Image` MUST be able to load a PNG
  if `LVGLPP_CORE_PNG` is on, and MUST compile (with a clear
  unsupported-format diagnostic) if it is off. The widget itself
  does not pin a specific decoder.

## §11 Non-goals

- **Defining decoder semantics.** Per-plugin sub-phase territory.
- **Cross-language ABI for plugin handles.** Plugins are
  language-internal; rlvgl's plugins do not interoperate with
  lvglpp's plugins at runtime.
- **Plugin discovery via filesystem.** No dynamic loading; plugins
  are statically compiled in.

## §12 Acceptance checklist

A conforming CORE-07 execution PR MUST satisfy:

- [ ] `core/CMakeLists.txt` is augmented with the slot table from
      §5.1 — every `LVGLPP_CORE_<NAME>` option is declared (default
      `OFF`).
- [ ] `core/OPTIONS.md` lists every slot with its default and a
      one-line description (the section already exists; this PR
      reconciles it with §5.1).
- [ ] A skeleton `core/src/plugins/.gitkeep` (or equivalent) marks
      the directory as the canonical landing zone.
- [ ] No plugin source is added in this PR — that's per-sub-phase.
- [ ] `core/STATUS.md` change log records ratification of CORE-07
      execution.

## §13 Files cited

- `rlvgl/core/src/plugins/mod.rs` (v0.2.0 @ 79f730d)
- `rlvgl/Cargo.toml` (informative — feature set)
- `lvglpp/core/OPTIONS.md` (planned options table)
- `lvglpp/CLAUDE.md` § "Doc Co-Location Policy",
  `lvglpp/docs/std-mapping.md` § "Freestanding subset"

## §14 Unblocks

- **CORE-07a / -07b / …** — per-plugin sub-phases (PNG, JPEG, GIF,
  …) can land independently.
- **WID-06** (`Image`) — gains a defined seam to consult.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Plugin slot table
  (§5.1), per-slot file layout (§5.2), default-OFF policy (§5.3)
  frozen with **Standards Action** registration. Per-plugin
  semantics deferred to sub-phase chapters CORE-07a through CORE-07m.
  Execution unblocked.
- 2026-04-27 — CORE-07 mechanism landed. `core/CMakeLists.txt`
  declares all 13 slot options (§5.1) with default `OFF`.
  `core/src/plugins/.gitkeep` marks the canonical plugin landing
  zone. No plugin source — that's per-sub-phase work.

- 2026-06-10 — §5.1 AMENDED (CORE-07n): slot **RLE** added —
  CMake `LVGLPP_CORE_RLE`, embedded-friendly, mirrors the
  `rlvgl-decomp` crate boundary (no Cargo feature exists; the
  crate is the gate, so no rlvgl-side change is required). First
  executed plugin sub-phase: docs/core-plugins/01-rle.md.
