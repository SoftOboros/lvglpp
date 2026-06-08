<!-- README.md — disco-demo app-shell port initiative (informative). -->

# disco-demo — lvglpp app-shell port

**Informative.** Per-chapter concepts docs are the normative artifacts;
this README does not re-derive normative rules — it points at the
chapter and section that own each rule (see CLAUDE.md § "Normative vs.
informative sections").

## What this initiative is

A faithful C++ mirror of the rlvgl shared demo controller
`rlvgl-app-disco-demo`
(`rlvgl/examples/apps/disco-demo/`) onto the lvglpp
`core` + `widgets` + host-SDL stack. The rlvgl crate is a `no_std`
(+`alloc`) library with no binary of its own, consumed by the simulator,
UEFI, and Cortex-M7 firmware adapters; lvglpp mirrors it as an
equivalently platform-independent app shell consumed by the host-SDL
target first and the STM32H747I-DISCO target once its display stack
(PLAT-02d–f) lands.

## Why it exists

The disco demo is the first end-to-end exercise of the lvglpp widget
tree, event surface, and playit protocol against a non-trivial UI. It is
a **cross-language contract**: the same navigation FSM, command set, and
capability gating must agree with rlvgl so the shared rlvgl-side fixtures
and playit probes exercise lvglpp unchanged (CLAUDE.md § "Cross-Project
Parity With rlvgl").

## The one hard problem

The concepts (widgets, events, FSM, commands) fall directly out of
lvgl/rlvgl. The lvglpp-specific work is **ownership**: rlvgl threads the
widget tree and controller state through `Rc<RefCell<…>>` shared-mutable
aliasing. lvglpp forbids that (CLAUDE.md § "Strict and Explicit
Ownership"). The port must reproduce rlvgl's *behavior* under lvglpp's
single-owner RAII discipline, and must keep the LVGL C handles
encapsulated behind destructor-bearing types. That decision is frozen in
chapter 00 §5 and is the spine of every later chapter.

## Chapters

| Chapter | Code | Status | Owns |
| --- | --- | --- | --- |
| [00 — App-shell contract & ownership model](./00-app-shell-contract.md) | DEMO-00 | **ratified** 2026-06-07 | Vocabulary, source-of-truth map, ownership model, FSM, command/capability enums, prerequisite gaps |
| [Container widget](./01-container-widget.md) | DEMO-01 | ratified | `widgets/container` (mirror `rlvgl/widgets/src/container.rs`) |
| [UI draw helpers](./02-ui-draw-helpers.md) | DEMO-02 | ratified | `panel header` / `close-hit` into `ui/` |
| [EventWindow](./03-event-window.md) | DEMO-03 | ratified | `ui/event_window` (mirror `rlvgl/ui/src/event_window.rs`) |
| [RLE icons](./04-rle-icons.md) | DEMO-04 | ratified | RLE decoder (consume-only) + `assets/icons/*.rle` |
| [Screen descriptor](./0S-screen-descriptor.md) | DEMO-0S | ratified | `platform/screen` (mirror `rlvgl/platform/src/screen.rs`) |
| [Composite widgets](./05-composite-widgets.md) | DEMO-05 | ratified | `IconStrip`/`Wing`/`DashboardPanel`/`ActionHotspot` + app module |
| [Controller + host target](./06-controller-and-host-target.md) | DEMO-06 | ratified | `DiscoController`/`ControllerState` + host-SDL + parity tests |

Dependency waves (chapter 00 §14): **A** = 01/02/04/0S (parallel) →
**B** = 03, 05 → **C** = 06. Each chapter needs its own ratified
concepts doc before code; chapter 00 §5/§6 already freeze the contracts,
so those docs are thin. No execution PR precedes a ratified chapter
(CLAUDE.md § "Execution discipline").

## Authority

rlvgl `v0.2.0` is canonical for app-shell behavior and vocabulary; LVGL
upstream is canonical for the underlying widget primitives; lvglpp owns
only the C++ ownership/encapsulation deltas. Full policy: chapter 00 §0.

## License

MIT, matching the repository.
