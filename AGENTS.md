# Agent Runbook — lvglpp

This file is the source of truth for Codex / Codex-style agents working in
the lvglpp repository. README snippets are human-facing and may lag behind
the currently buildable artifact set.

lvglpp is the **C++ sibling of [softoboros/rlvgl](https://github.com/SoftOboros/rlvgl)**.
The two projects target the same boards and the same widget surface; rlvgl
is the Rust implementation, lvglpp is the modern-C++ wrapper around upstream
LVGL. They live as parallel submodules in this tree:

```
lvglpp/
├── lvgl/    # upstream LVGL C source — the library we wrap
└── rlvgl/   # SoftOboros rlvgl — Rust reference, NOT recursively initialized
            #   (its own lvgl/ submodule is intentionally empty here)
```

## Strict and Explicit Ownership — Project-Wide Rule

Every pointer, reference, handle, buffer, and resource in lvglpp must have
an explicit ownership role in the design, even when C++ does not encode it
directly.

Use these categories in comments adjacent to declarations:

```cpp
// owns:     responsible for destruction/release.
// borrows:  temporary non-owning access; must not outlive owner.
// observes: nullable/passive non-owning access; no mutation ownership.
// shares:   reference-counted/shared lifecycle.
// external: lifecycle controlled outside this module.
// mmio:     memory-mapped hardware; never freed.
// dma:      buffer may be mutated by hardware/driver.
```

### Required discipline

1. Prefer value types and RAII.
2. Use `std::unique_ptr<T>` for exclusive ownership.
3. Use `std::shared_ptr<T>` only when shared lifetime is intrinsic.
4. Use references for required borrows.
5. Use raw pointers only for nullable observation, C APIs (LVGL),
   MMIO, DMA, or explicitly documented escape hatches.
6. Every raw pointer member must have a comment stating ownership and
   lifetime.
7. Every ownership transfer must be visible at the call site via
   `std::move`, factory return, or a named release/attach function.
8. Do not hide ownership transfer inside ambiguous setters.
9. Callback / userdata lifetimes must be documented and mechanically safe.
10. Hardware-owned or driver-owned buffers (LVGL draw buffers, DMA2D
    targets, framebuffers) must be marked as such and protected against
    CPU mutation during active use.

### Function contract comments

```cpp
// Args:
//   owner:   owns Foo; ownership transferred into this object.
//   view:    borrows Bar for the duration of the call only.
//   dma_buf: dma/external; caller guarantees inactive DMA during mutation.
// Returns:
//   owns Baz via RAII.
// Throws:
//   std::runtime_error on initialization failure.
```

### Naming conventions

```text
make_*      creates and returns ownership
take_*      consumes/transfers ownership in
borrow_*    non-owning temporary access
view_*      non-owning observation
release_*   gives up ownership
attach_*    transfers ownership into parent/container
detach_*    removes ownership from parent/container
```

Avoid ambiguous names: `set_ptr`, `set_buffer`, `register_callback`. Prefer
`borrow_buffer`, `take_buffer`, `observe_callback_target`,
`register_callback_with_lifetime`.

### Code review checklist

Reject or revise code when:

```text
[ ] A raw pointer member lacks ownership/lifetime documentation.
[ ] A resource has no RAII owner.
[ ] Ownership transfer is implicit.
[ ] A callback captures or stores a borrowed object without lifetime proof.
[ ] A buffer shared with DMA/MMIO is mutated without synchronization.
[ ] `shared_ptr` is used to avoid deciding ownership.
[ ] `reinterpret_cast` or pointer/integer casts lack a provenance comment.
[ ] `new` / `delete` appear outside a narrow RAII wrapper.
[ ] C API handles are not wrapped in a destructor-bearing type.
```

### Escape hatch rule

Unsafe ownership/provenance breaks are allowed only when isolated and
labeled:

```cpp
// SAFETY:
//   raw_handle is external; owned by LVGL object tree.
//   Valid until lv_obj_del(parent) or explicit detach.
//   No ownership transfer occurs here.
```

The goal is not Rust semantics. The goal is C++ code where ownership,
lifetime, provenance, and mutation authority are explicit enough for
humans, static analyzers, and future compiler diagnostics to reason about.

## Spec-Before-Code Planning Discipline

lvglpp adopts the same **spec-before-code** discipline as rlvgl `v0.2.0`
(see `rlvgl/AGENTS.md` § "Spec-Before-Code Planning Discipline" — the
authoritative source). The rules below are the lvglpp restatement; when
they disagree with rlvgl `v0.2.0`, **rlvgl wins** and this file is the
bug.

Vocabulary drift and invariant erosion are the dominant failure modes
once a plan crosses ~3 phases. The cycle exists to prevent silent forks
between rlvgl (Rust) and lvglpp (C++) — same board, same widget surface,
same protocol — not as ceremony.

### When this discipline applies

- Any multi-chapter doc family in lvglpp (`docs/<initiative>/...`) with
  ≥3 phases, or any single-doc family that gains a sibling chapter.
- Any cross-language contract with rlvgl (widget API parity, playit wire
  protocol, BSP consumption surface). Even a single doc here triggers
  the discipline because the cross-language pair is itself a multi-phase
  initiative.
- Any frozen enum / register-bit-position / pixel-format / wire-protocol
  command that must agree with rlvgl.

Single-file C++ refactors, bug fixes, and one-off experiments MAY use
informal form. The moment a change touches a contract shared with rlvgl,
the full discipline applies.

### Normative keywords (RFC 2119 / 8174)

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in lvglpp initiative-family guide
docs and per-chapter concepts docs are interpreted per RFC 2119 and
RFC 8174. Capitalize when invoking; lowercase for ordinary English.
Plain narrative without capitalized keywords is advisory.

### Normative vs. informative sections

- Sections referenced by a chapter's **Acceptance** checklist (or by a
  release roadmap checkbox citing the chapter) are **normative**.
- Problem statement, narrative, lessons-learned, non-goals, and change
  log are **informative**.
- The initiative `README.md` is **informative**; per-chapter docs are
  the normative artifacts. Do not re-derive normative rules in the
  README narrative — cite the chapter and section heading.

### Definitions — reference vs. restatement

For every term that also exists in code (C++ here, Rust in rlvgl, or C
in lvgl), the glossary entry MUST cite the authoritative source and
mark the relationship:

- **"As defined in [path/to/file.hpp:line]; used without modification."**
  — repo is canonical; spec references it.
- **"As defined in [path/to/file.hpp:line]; adapted: [delta]."** — repo
  is canonical; spec extends/narrows it with a named delta.
- **"As defined in `rlvgl/<path>:line`; mirrored here as
  [path/to/file.hpp:line]."** — rlvgl is canonical, lvglpp tracks it.
  Use whenever the contract crosses the language boundary.
- **"Owned by &lt;CHAPTER&gt;; does not exist in repo yet."** — spec is
  canonical; repo will mirror once the chapter lands.

Silent restatement of an existing rlvgl or lvgl definition is how the
two implementations fork. Don't do it.

### Frozen enumerations — registration policy

Every frozen enum in lvglpp (e.g. PixelFmt mirrors, Rotation,
playit-protocol command set, BSP-vendor set) declares its registration
policy in the concepts doc:

- **Standards Action** — adding a value requires an amendment to the
  initiative's canonical concepts doc and an explicit go-ahead from the
  owner. Use for cross-language contracts (anything that must match
  rlvgl).
- **Specification Required** — adding a value requires a per-chapter
  walkthrough update. Use for enums local to one lvglpp chapter.
- **Expert Review** — chapter owner MAY add with a PR-level note. Use
  for internal enums with no cross-language coupling.

Default to **Standards Action** when in doubt; demote later if churn
justifies. Any enum value that exists in rlvgl is **Standards Action**
in lvglpp by definition.

### Phase document shape

A per-chapter concepts doc follows: §0 authority policy (which external
doc owns which vocabulary — LVGL upstream for widgets, rlvgl `v0.2.0`
concepts docs for cross-language contracts, vendor RM/TRM for board
specifics), §1 purpose, §2 problem statement (evidence pinned to code
paths, e.g. `src/foo.cpp:NN`), §3 canonical glossary, §4 source-of-truth
map (one owner per concept across lvgl / rlvgl / lvglpp), §5–§9 frozen
decisions, §10 reconciliation vs. adjacent rlvgl primitives, §11
non-goals, §12 acceptance checklist, §13 files cited, §14 unblocks, §15
change log. §0, §3/§4, §10, §12, §15 are load-bearing.

### Execution discipline

Once a concepts doc is ratified (dated change-log entry), execution PRs:

- Cite the initiative-and-phase code in the commit subject. Suggested
  prefixes: `LVGLPP-NN[a-z]:` for lvglpp-internal initiatives. When a
  change implements an rlvgl-ratified phase on the C++ side, **reuse
  the rlvgl prefix** (`DISCO-NN[a-z]:`, `BBB-NN[a-z]:`,
  `CREATOR-NN[a-z]:`, `CHIPS-<VENDOR>-NN[a-z]:`) so the cross-language
  pair shares one identifier.
- Name in the PR description which invariants (from the concepts doc's
  frozen-decisions sections) the change touches, and how each is
  preserved.
- Touching a frozen enum value or invariant requires a change-log
  amendment **first**, in a separate PR. No behavior PR rides on an
  unamended invariant. If the invariant lives in rlvgl, the amendment
  PR lands in rlvgl first; the lvglpp mirror PR cites that PR's SHA.

Conventional-commit style (`feat:`, `fix:`, `docs:`, `tools:`) remains
the default for non-initiative work.

### Cross-language change ordering

When a change crosses rlvgl ↔ lvglpp (widget API, playit command,
shared enum, BSP-consumption contract):

1. Concepts-doc amendment lands in rlvgl `v0.2.0` first (or in lvglpp
   if the concept originates here — rare).
2. The owning side's implementation lands, citing the amendment.
3. The mirroring side's implementation lands, citing both the amendment
   SHA and the owning-side implementation SHA.
4. The lvglpp `rlvgl/` submodule pin is bumped in the same PR as
   step 3, never in a drive-by PR.

The point is convergence over time: form is cheaper to align than
vocabulary, and a fork between Rust and C++ implementations of the same
widget surface is the most expensive kind to repair.

## Doc Co-Location Policy

lvglpp follows rlvgl's convention of **co-locating module docs with
module code**, not pooling them in a top-level `docs/`. Every module
under the per-library directories (`core/`, `widgets/`, `ui/`,
`platform/`, `playit/`) carries the same fixed doc trio plus an optional
fourth file. The agent and the human both look in the same place for
the same kind of information.

### Per-module file shape

| File | Purpose | When to edit |
| --- | --- | --- |
| `README.md` | Publish-facing overview. Triangulation paragraph (rlvgl + lvgl + lvglpp). Main areas. Where it is used. License. | When the module surface changes shape. |
| `OPTIONS.md` | Build-flag / feature reference. Project-wide CMake options that affect this module. Per-module `LVGLPP_<MODULE>_<FEATURE>` flags. Relevant `LV_USE_*` symbols from `lv_conf.h`. | When a flag is added, removed, or changes default. |
| `STATUS.md` | **Formal block shape — see below.** Roadmap intent / As-built / Blockers / Definitions / Change log. | Append to change log on every status-affecting commit. Update the four sections in place. |
| `AGENTS.md` / `AGENTS.md` (optional) | Module-scoped agent rules. Only present when the rules differ from the top-level AGENTS.md. | Rare. |

`STATUS.md` is the load-bearing file for the agent: it answers "what
should this be?" (intent), "what is it?" (as-built), and "what's in the
way?" (blockers) without reading any code. Keep it accurate.

### `STATUS.md` canonical sections

`STATUS.md` MUST have these sections, in this order, with these names:

```markdown
# <module> — STATUS

Tracks rlvgl/<crate> @ <pin> (commit <sha>). Last reconciled: <YYYY-MM-DD>.

## Roadmap intent
What this module is for. Numbered phase plan with PHASE-NN[a-z] codes
(see AGENTS.md § "Spec-Before-Code Planning Discipline" / "Execution
discipline" for the prefix convention). Each phase names its
dependencies on other modules' phases.

## As-built
What is implemented today, broken into "Implemented" and "Stubbed"
sub-lists. Be honest — stubbed-but-greppable beats accidentally-vapor.

## Blockers
Each blocker has an owner. Phrasing: "Owner: <person | role>". When a
blocker is resolved, move the line to the change log instead of
deleting it silently.

## Definitions
Local glossary. Every term that also exists in rlvgl or lvgl MUST cite
the authoritative source per AGENTS.md § "Definitions — reference vs.
restatement". The four reference forms are mandatory; do not invent
new ones.

## Change log
Append-only. One line per status-affecting change, dated.
```

### Cite-block convention for source files

Every `*.hpp` and `*.cpp` under a module MUST carry a triangulation
cite block at file head, before the include guard. The forms:

```cpp
// PARITY: rlvgl/<path>.rs (v0.2.0 @ <sha or branch tip>).
// LVGL:   lvgl/<path>.h (or "N/A" with reason).
// DELTA:  one-sentence summary of the C++ deviation, or "none".
```

This makes the spec-before-code glossary forms ("As defined in
`rlvgl/<path>:line`; mirrored here as `<hpp>:line`") mechanically
grep-able by both agents and humans.

### `playit` is first-class; `creator-cpp` is deferred

The five first-wave modules — `core`, `widgets`, `ui`, `platform`,
`playit` — all carry the full doc trio and are scaffolded today.
`playit` is **first-class** because the wire protocol is a
cross-language contract: lvglpp re-implements the parser in C++ so the
same rlvgl-side fixtures and probe drivers exercise lvglpp targets
unchanged.

`creator-cpp` (a C++ port of `rlvgl-creator`) is **explicitly deferred**.
The order of operations is:

1. Land `lvglpp` core surface and a board target.
2. Teach `rlvgl-creator` (Rust) to emit lvglpp-consumable assets — i.e.
   the rlvgl → lvglpp asset path becomes well-worn.
3. Only after step 2 stabilizes, consider porting creator to C++ for
   the return path (lvglpp generating its own assets).

This means: `lvglpp::core` plugin / asset-loader headers should
deliberately leave a seam for rlvgl-creator output. Don't design
around assumptions that creator will eventually be C++.

### Polyfill convention

When a C++ std-library type is needed before the toolchain floor
includes it, lvglpp ships a **minimal vendored polyfill** under
`third_party/<feature>/` plus a **seam header** at
`include/lvglpp/std/<feature>.hpp`. The seam selects between
`<feature>` and the polyfill at compile time. Consumers always write
`lvglpp::<name>`; never `std::<name>` or the polyfill namespace
directly. See `third_party/lvglpp_expected/README.md` for the worked
example, and `docs/std-mapping.md` § "Polyfill convention" for the
rule.

### Embedded posture mirrors rlvgl

The CMake option `LVGLPP_EMBEDDED_POSTURE` mirrors rlvgl's `no_std` +
`panic = abort` posture. When ON: `-fno-exceptions -fno-rtti` are
applied to every lvglpp target; throwing constructors call
`std::abort()` instead. Cross-build / firmware targets MUST set this
ON. Host smoke tests leave it OFF for ergonomic exception flow.

The full rule, including the freestanding-subset header allowlist,
lives in `docs/std-mapping.md` § "Embedded posture".

## Submodule Policy

- `lvgl/` is the **only** LVGL source tree consumed by the build.
- `rlvgl/` is pinned to the **`v0.2.0`** branch (tracked via
  `.gitmodules` `branch = v0.2.0`). The Spec-Before-Code discipline
  above lives there, not on `main`; do not switch the pin to `main`
  without first lifting that section into `main`.
- `rlvgl/` is a **non-recursive** submodule. Its own `lvgl/` submodule
  is deliberately left uninitialized: we already have lvgl at the top
  level, and pulling rlvgl's pin would risk a version split.
  `.gitmodules` sets `fetchRecurseSubmodules = false` for rlvgl to
  enforce this.
- To bring up the tree from a fresh clone:
  ```bash
  git clone git@github.com:SoftOboros/lvglpp.git
  cd lvglpp
  git submodule update --init lvgl rlvgl   # NO --recursive
  ```
- To advance the rlvgl pin within `v0.2.0`:
  ```bash
  cd rlvgl && git fetch origin v0.2.0 && git checkout origin/v0.2.0
  cd .. && git add rlvgl && git commit -m "Bump rlvgl pin on v0.2.0"
  ```
- Never run `git submodule update --init --recursive` at the top level.
  If you do, delete `rlvgl/lvgl/` afterwards and re-pin rlvgl.

## Board Target

- Board: `STM32H747I-DISCO`
- Probe-rs chip id: `STM32H747XIHx`
- The Rust reference binary in rlvgl is `rlvgl-stm32h747i-disco`. The C++
  example binary will mirror this name once the disco target lands here.

## Build

lvglpp uses CMake (>= 3.20) and C++20.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Compile-commands are exported (`compile_commands.json` in `build/`) so
clangd / clang-tidy work without extra config. The host build assumes a
desktop toolchain; embedded targets live under `examples/` and will pull
in the appropriate cross-toolchain file.

## Cross-Project Parity With rlvgl

When implementing a feature in lvglpp, treat rlvgl as the canonical
reference for behavior and naming **intent**, not for code shape:

- Widget semantics, event ordering, and tick behavior should match
  rlvgl's observed runtime.
- Public API names should follow C++ idioms (`make_label`,
  `borrow_screen`) rather than the Rust forms; the *ownership story*
  must match.
- When a behavior diverges, document the divergence in the relevant
  header with a `// PARITY:` comment pointing at the rlvgl path.

## Runtime Command Protocol (rlvgl-playit)

The Rust side ships a serial test driver (`rlvgl-playit`) used for
end-to-end UI testing on hardware. lvglpp targets that consume the same
protocol should re-implement the parser in C++ rather than depend on the
Rust crate. Commands are single lines terminated by `\n` or `\r\n`. See
`rlvgl/playit/README.md` (in the rlvgl submodule) for the full wire
protocol reference.

Core commands:

- `?` — tick count, present count, serial queue/drop state
- `T<x>,<y>` — inject `PressRelease` at landscape `(x, y)`
- `PD<x>,<y>` / `PM<x>,<y>` / `PU<x>,<y>` — raw pointer down/move/up
- `MT<n>:<id>,<s>,<x>,<y>;...` — multi-touch frame (s=D/U/C)
- `KD:<key>` / `KU:<key>` — key down/up
- `T@<tag>:<x>,<y>` — inject tap to tagged widget
- `QB:<tag>` / `QE:<tag>` / `QC:<tag>` — query bounds/exists/children
- `D<x>,<y>,<w>,<h>[,<frames>]` — framebuffer pixel dump
- `RS` / `RE` / `RD` — start / stop+dump / dump event recorder

## Pre-Publish Validation

Run these before committing changes that touch the public C++ API or the
build system. All phases must pass.

```bash
# Phase 0: format
cmake --build build --target clang-format-check 2>/dev/null || \
    git ls-files '*.cpp' '*.hpp' '*.h' | xargs clang-format --dry-run -Werror

# Phase 1: warnings = errors
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-Werror"
cmake --build build -j

# Phase 2: tests (host)
ctest --test-dir build --output-on-failure

# Phase 3: clang-tidy (ownership-discipline lint)
git ls-files 'src/*.cpp' 'include/**/*.hpp' | \
    xargs clang-tidy -p build

# Phase 4: cross-build smoke (embedded target — once the disco example
# lands, build it here to catch regressions).
# cmake -S . -B build-disco -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
# cmake --build build-disco -j
```

## Profiling Guidance (carries over from rlvgl)

- Prefer DWT + D3 SRAM telemetry over serial output when measuring timing.
- Use serial for control, coarse summaries, and targeted framebuffer dumps.
- The CM7 main loop is intended to stay responsive while DMA2D and USART1
  IRQs run in the background. If you add new waits, they should be
  stateful and return to the loop instead of spinning.
- Relevant telemetry includes idle cycles, loop count, pipeline
  stage/frame, DMA2D last/max cycles, DMA completion/error counts, and
  serial queue/drop counters.

## Things That Are NOT Goals For lvglpp

- Replacing rlvgl. They coexist; choose by language constraint of the
  consuming project.
- Wrapping every LVGL widget on day one. Prefer correct ownership
  discipline on a small surface over a wide, leaky API.
- Inheriting rlvgl's chipdb / BSP generator. lvglpp consumes generated
  BSPs but does not regenerate them.
