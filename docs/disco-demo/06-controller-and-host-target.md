<!-- 06-controller-and-host-target.md — DEMO-06 concepts doc (normative). -->

# DEMO-06 — `DiscoController` + host-SDL target + parity tests

Status: **ratified** (2026-06-07). The controller's ownership model
(O-1…O-8), event mapping, FSM, and public API are frozen in DEMO-00
§5/§7/§9; this chapter restates them, pins the construction/observer-
capture sequence, the host target, and the parity-test plan. RFC 2119
keywords per DEMO-00.

## §0 Authority

Inherits DEMO-00 §0. Canonical:
`rlvgl/examples/apps/disco-demo/src/lib.rs` (rlvgl `v0.2.0`):
`DiscoController` (:820, `new` :831, `root` :1240, `dispatch_event`
:1245, `handle_event` :1252, `tick` :1284, `drain_commands` :1290,
`publish_status` :1295), `ControllerState` (:285), the FSM enums
(:164–:243), `DiscoCapabilities` (:38 + presets :61), `DiscoCommand`
(:142), `DiscoEffect` (:133). The capstone lands in the existing app
module `lvglpp::app::disco_demo`; the runnable simulator lands in a new
`examples/disco-sim/` host target (mirrors rlvgl `examples/disco-sim/`).

## §1 Purpose

Wire the Wave-A/B primitives (Container, EventWindow, the four
composites, the FSM types) into the running disco demo: the controller
that owns the tree and routes input, a host-SDL binary that renders it,
and a parity-test suite proving the FSM/commands match rlvgl. Final
chapter of the initiative.

## §2 Types (FROZEN — mirror; Standards Action per DEMO-00 §0)

In `include/lvglpp/app/disco_demo/`:

- `capabilities.hpp` — `struct DiscoCapabilities { bool audio, storage,
  diagnostics, effects, pointer; std::string_view platform; }` with
  `static constexpr` presets `simulator()`, `stm32h747i_disco()`,
  `uefi()`, `zephyr()`, `beaglebone_black()` (mirror `lib.rs:38–122`).
- `command.hpp` — `enum class DiscoEffect { AudioScope, StarCrawl };`
  and `using DiscoCommand = std::variant<` `cmd::SetBacklight{u8}`,
  `cmd::LoadStorageSummary`, `cmd::StartEffect{DiscoEffect}`,
  `cmd::StopEffect{DiscoEffect}`, `cmd::ShowStatus{std::string}`,
  `cmd::NoOp` `>` (mirror `lib.rs:133,142`). Variant set FROZEN.
- The FSM enums (`FocusState`, `WingKind`, `MainSlot`, `SettingsSlot`,
  `InfoSlot`) are controller-internal; mirror `lib.rs:164–243` exactly
  (values + ordinals FROZEN).

## §3 Ownership & construction sequence (load-bearing — DEMO-00 §5)

`DiscoController` is the outer owner (O-2): it owns the root
`core::WidgetNode` by value and `ControllerState` by
`std::unique_ptr` (stable address). `make(Screen, DiscoCapabilities)`
MUST follow this order so observers are valid (O-3/O-4):

1. Build every widget as `std::make_unique<W>(…)` and move each into the
   tree (`root.add_child(WidgetNode{std::move(w), "tag"})`), mirroring
   the `lib.rs:831` tree: root `Container` → title/subtitle `Label`,
   `DashboardPanel`, footer `Label`, `EventWindow`, settings `Wing`,
   info `Wing`, `IconStrip`, then the 8+ `ActionHotspot`s.
2. **After** the tree is fully assembled, capture raw observing pointers
   to the widgets the controller mutates (`DashboardPanel*`,
   `Label* subtitle/footer`, `EventWindow*`, `IconStrip*`,
   `Wing* settings/info`). These are `Widget*`-family, **never**
   `WidgetNode*` (O-4). Capture via the `std::make_unique` raw pointer
   taken before the move, or via `find_by_tag` + `static_cast` after —
   the implementer picks one and documents it.
3. Construct `ControllerState` holding those observers + `capabilities`
   + `FocusState::Main(0)` + an empty `std::vector<DiscoCommand>`.
4. Bind tap/visibility callbacks: `IconSlot`/`WingSlot` `on_tap` =
   `std::function` capturing `ControllerState*` (O-5); `ActionHotspot`
   `is_visible` predicates capturing the relevant `const Wing*`.
5. Apply initial focus highlight + `show_home()` on the dashboard.

**Audit invariants (O-1…O-8):** no `std::shared_ptr` widget; no
`WidgetNode*` stored in `ControllerState`; every observer + callback
documents its lifetime (controller outlives tree outlives widgets); no
`lv_obj_t*` escapes (the composites are pure C++). `DiscoController` is
move-only; moving preserves observers (widgets heap-stable, state behind
`unique_ptr`).

## §4 Public API (FROZEN — DEMO-00 §9)

```cpp
class DiscoController {
 public:
  static DiscoController make(platform::Screen, DiscoCapabilities);
  DiscoController(DiscoController&&) noexcept;            // move-only
  [[nodiscard]] bool dispatch_event(const core::Event&); // tree then handle_event
  void handle_event(const core::Event&);
  void tick();
  [[nodiscard]] std::vector<DiscoCommand> drain_commands();
  void publish_status(std::string);
  [[nodiscard]] core::WidgetNode& root() noexcept;       // borrow, for draw/dispatch
};
```

`dispatch_event` runs the tree (`root.dispatch_event`) then
`handle_event` (mirror `lib.rs:1245`). `handle_event` consumes only
`Tick` (tick_count++, footer every 600, re-render active info page),
`KeyDown` (FSM via `handle_key`), and `PressRelease` (gated by
`capabilities.pointer`) per DEMO-00 §7. `tick()` = dispatch `Tick` to the
tree + `handle_event(Tick)`. `drain_commands` moves out the queue.

## §5 FSM & behavior (FROZEN — restate DEMO-00 §7)

Mirror `lib.rs` `cycle_main_focus`/`cycle_wing_focus`/`activate_main`/
`activate_settings`/`activate_info`/`close_wings`/`handle_key`/
`render_info_page`/`push_status`/`sync_focus_highlights`. Start
`Main(0)`, wings closed. The full transition table is DEMO-00 §7 E-3 —
implement it edge-for-edge, including capability gating
(`audio`/`effects`) of the wing slots and the `s`/`f`/`i`/`b` hotkeys.
`publish_status` sets footer + `EventWindow::push_event` + queues
`cmd::ShowStatus` (mirror `lib.rs:1295`).

## §6 Host-SDL target (`examples/disco-sim/`)

A runnable simulator mirroring rlvgl `examples/disco-sim/`, gated on
`LVGLPP_PLATFORM_HOST_SDL` (like `examples/host_sdl_label/`). Structure
(mirror `examples/host_sdl_label/main.cpp`):

```
HostSdlBackend backend = try_make("lvglpp disco-demo", 800, 480);
DiscoController ctl = DiscoController::make(
    Screen::landscape(800, 480), DiscoCapabilities::simulator());
playit wiring on stdin (Executor + dispatcher over ctl.root());
loop while !quit:
  while (e = backend.poll_event()) ctl.dispatch_event(*e);   // SDL + gestures
  executor.poll();                                            // playit stdin
  ctl.tick();
  for (cmd : ctl.drain_commands()) execute_host(cmd);         // backlight=noop,
                                                              //   ShowStatus=log, etc.
  backend.clear(bg); ctl.root().draw(backend.renderer()); backend.present_frame();
```

Host command execution is a thin adapter: `SetBacklight`→no-op/log,
`LoadStorageSummary`→`publish_status` with a canned summary,
`Start/StopEffect`→log (effects are out of scope, DEMO-00 §11),
`ShowStatus`→log, `NoOp`→nothing. The playit `T@<tag>` path drives the
FSM through the same `PressRelease` edges as SDL input.

## §7 Parity tests (`examples/apps/disco-demo/tests/`)

Mirror rlvgl `cargo test -p rlvgl-app-disco-demo` (its `#[cfg(test)]`
suite). SDL-free — build a controller, feed `core::Event`s, assert FSM
state and drained commands:

- **navigation** — arrows cycle main 0→1→2→0 (wrap); enter on Main(0)
  opens settings wing → `Wing(Settings,0)`; arrows cycle wing slots with
  wrap; left/escape close wings back to `Main(i)`.
- **focus** — `set_focused_slot` wiring: focusing a main/wing slot
  updates the corresponding widget's `focused_slot` (focus-highlight
  parity).
- **hotkeys** — `s`/`f`/`i` activate Settings/Files/Info; `b` activates
  Backlight (queues `SetBacklight`).
- **command emission** — activating Files queues `LoadStorageSummary`;
  backlight change queues `SetBacklight(level)`; `publish_status` queues
  `ShowStatus` and `drain_commands` returns+clears.
- **capability gating** — `simulator()` vs a caps set with `audio=false`
  /`effects=false`: gated wing slots disabled; `pointer=false` →
  `PressRelease` queues an "ignored" status.

Test target(s) `lvglpp_app_disco_demo_controller` (one or split per
area). Pointer/key events constructed directly from `core::event::*`.

## §8 Files

- `include/lvglpp/app/disco_demo/{capabilities,command,disco_controller}.hpp`
  (new) + `src/disco_controller.cpp` (new; the construction + FSM + state)
- `tests/disco_controller_test.cpp` (new) + register in
  `tests/CMakeLists.txt`
- `examples/disco-sim/{CMakeLists.txt,main.cpp}` (new) + register from
  `examples/CMakeLists.txt` (gated on `LVGLPP_PLATFORM_HOST_SDL`)
- `examples/apps/disco-demo/STATUS.md` — change-log + Definitions
- Cite blocks per file: `// PARITY: rlvgl/examples/apps/disco-demo/src/
  lib.rs (v0.2.0 @ 79f730d).` / `// LVGL: N/A (app controller).` /
  `// DELTA: …`.

## §9 Acceptance

- [ ] Ownership audit (O-1…O-8): no `shared_ptr` widget, no
      `WidgetNode*` in `ControllerState`, observers + callbacks
      lifetime-documented, no `lv_obj_t*` escape, `DiscoController`
      move-only and move-safe.
- [ ] FSM matches DEMO-00 §7 E-3 edge-for-edge (parity tests green).
- [ ] `DiscoCommand`/`DiscoCapabilities`/FSM enums byte-match rlvgl.
- [ ] `drain_commands` returns + clears; `publish_status` updates footer
      + event window + queues `ShowStatus`.
- [ ] Host target builds under `LVGLPP_PLATFORM_HOST_SDL`, renders the
      demo, and is drivable by playit `T@<tag>`.
- [ ] Controller + parity tests build SDL-free on a plain host configure
      and under embedded posture; Pre-Publish 0–3 green.

## §10 Change log

- _drafted_ — DEMO-06 restated from DEMO-00 §5/§7/§9; construction/
  observer-capture sequence, host-sim target, and parity-test plan
  pinned.
- **2026-06-07 — ratified.** Owner directed Wave-C. Capstone chapter:
  `DiscoController`/`ControllerState` (in `lvglpp::app::disco_demo`),
  `examples/disco-sim/` host target, and the rlvgl-parity FSM/command
  test suite. Execution may proceed.
