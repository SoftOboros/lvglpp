<!-- 00-app-shell-contract.md — DEMO-00 concepts doc (normative). -->

# DEMO-00 — Disco-demo app-shell contract & ownership model

Status: **ratified** (owner signed off D1–D4 on 2026-06-07; see §15).
Execution PRs for DEMO-00's prerequisite chapters MAY now proceed once
each prerequisite chapter is itself ratified (CLAUDE.md § "Execution
discipline").

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, **RECOMMENDED** are interpreted per RFC 2119 / RFC 8174.
Sections cited by the §12 Acceptance checklist are **normative**; problem
statement, narrative, reconciliation, non-goals, and change log are
**informative**.

---

## §0 Authority policy

Which external doc owns which vocabulary:

- **App-shell behavior, navigation FSM, command set, capability gating,
  layout constants** — owned by rlvgl `v0.2.0`, crate
  `rlvgl-app-disco-demo` at `rlvgl/examples/apps/disco-demo/`
  (README: "Shared STM32H747I-DISCO-style demo controller used by the
  simulator, UEFI runtime, and board-specific adapters"). rlvgl is
  **canonical**; lvglpp mirrors.
- **Event / Key surface, Widget / WidgetNode / Renderer / Style /
  Color / Rect / BitmapFont** — owned by `rlvgl-core`
  (`rlvgl/core/src/`), already mirrored into lvglpp `core/` (see the
  ratified `docs/core-*` chapters). This chapter consumes those mirrors
  without modification.
- **UI helpers (EventWindow, panel header / close-hit drawing)** — owned
  by `rlvgl-ui` (`rlvgl/ui/src/`); lvglpp `ui/` is presently a stub and
  MUST gain mirrors before the controller can be built (§14).
- **Underlying widget primitives** — LVGL upstream (`lvgl/`) is
  canonical; lvglpp wraps, and MUST NOT shadow, `lv_obj_t` /
  `lv_event_t` (CLAUDE.md § "Cross-Project Parity").
- **C++ ownership, lifetime, provenance, and LVGL-handle encapsulation**
  — owned **here**. This is the only vocabulary this initiative
  originates; everything else is reference or restatement.

Initiative code: **DEMO-NN[a-z]** (lvglpp-internal port initiative per
CLAUDE.md § "Execution discipline"; the rlvgl `DISCO-NN` board-bringup
codes are a different axis and are not reused here).

Registration policy for this chapter's frozen enumerations: **Standards
Action** — every enum below mirrors an rlvgl type across the language
boundary, so adding/altering a value requires an amendment in rlvgl
`v0.2.0` first, then a mirroring amendment here (CLAUDE.md § "Frozen
enumerations" + § "Cross-language change ordering").

---

## §1 Purpose

Define the contract for porting the rlvgl disco-demo app shell to C++:
the vocabulary mapping, the **ownership model** that replaces rlvgl's
`Rc<RefCell<…>>` aliasing with lvglpp single-owner RAII, the
event/FSM/command mapping, and the prerequisite gaps that MUST close
before controller code lands. This chapter is the architectural spine;
later chapters implement against it.

---

## §2 Problem statement (informative)

1. **rlvgl leans on shared mutable aliasing.** `DiscoController` holds
   `root: Rc<RefCell<WidgetNode>>` and `state: Rc<RefCell<ControllerState>>`
   (`rlvgl/examples/apps/disco-demo/src/lib.rs:820`). Each mutable widget
   (`dashboard`, `subtitle`, `footer`, `event_window`, `icon_strip`,
   `settings_wing`, `info_wing`) is an `Rc<RefCell<…>>` aliased **both**
   by the widget tree (for draw/dispatch) **and** by `ControllerState`
   (for post-construction mutation), with tap callbacks
   (`Box<dyn FnMut>`) capturing `Rc<RefCell<ControllerState>>`
   (`lib.rs:285`, `icon_strip.rs`, `wing.rs`, `hotspot.rs`). This is
   exactly the pattern CLAUDE.md § "Strict and Explicit Ownership"
   forbids (`shared_ptr`/interior mutability used to avoid deciding
   ownership).

2. **lvglpp core is single-owner and move-only.**
   `core::WidgetNode` owns its widget via
   `std::unique_ptr<Widget>` and its subtree via
   `std::vector<WidgetNode>`, with copy deleted
   (`core/include/lvglpp/core/widget_node.hpp:43`). There is **no**
   shared-ownership primitive in core, by design.

3. **The composite widgets do not exist in lvglpp.** `IconStrip`,
   `Wing`, `DashboardPanel`, `ActionHotspot` live in the rlvgl app crate,
   not in `rlvgl-core`/`rlvgl-widgets`. lvglpp `widgets/` ships only
   `Label`, `Button`, `Checkbox`, `Switch`, `Slider`
   (`widgets/include/lvglpp/widgets/`). A `Container` widget, the
   `EventWindow`, and two draw helpers
   (`draw_panel_header`, `panel_close_hit`,
   `rlvgl/ui/src/draw_helpers.rs:41,98`) are also absent
   (`core/include/lvglpp/core/draw_helpers.hpp` has only
   `draw_border_straight`, `fill_rounded_rect`, `draw_widget_bg`).

4. **Icons are RLE blobs.** `IconStrip`/`Wing` render RLE-decoded icons
   from `examples/apps/disco-demo/assets/icons/*.rle`
   (`assets.rs:1`). lvglpp has no RLE decoder and the creator path is
   deferred (CLAUDE.md § "`creator-cpp` is deferred").

The port therefore is **not** a transliteration; it is a re-expression
of rlvgl's observable behavior under lvglpp ownership discipline, plus a
small set of prerequisite primitives.

---

## §3 Canonical glossary

Each term that also exists in code cites its authoritative source and
marks the relationship using the four mandatory forms (CLAUDE.md §
"Definitions — reference vs. restatement").

- **App shell** — the platform-independent controller + composite-widget
  set that renders the demo UI and routes input, excluding board
  bringup. *Owned by DEMO; does not exist in repo yet.*
- **`DiscoController`** — top-level orchestrator: owns the widget tree
  and controller state, exposes `dispatch_event` / `handle_event` /
  `tick` / `drain_commands` / `publish_status` / `root`. As defined in
  `rlvgl/examples/apps/disco-demo/src/lib.rs:820`; mirrored here as
  `<app>/disco_controller.hpp` (owner-discipline delta in §5).
- **`ControllerState`** — the FSM + mutable-widget observer set + command
  queue. As defined in `lib.rs:285`; mirrored here; adapted: holds
  non-owning observers instead of `Rc<RefCell>` shares (§5).
- **`FocusState`** — `Main(usize) | Wing(WingKind, usize)`. As defined in
  `lib.rs:164`; mirrored here as a `std::variant` / tagged struct;
  values FROZEN (Standards Action).
- **`WingKind`** — `Settings | Info`. As defined in `lib.rs`; mirrored
  here; FROZEN.
- **`MainSlot` / `SettingsSlot` / `InfoSlot`** — slot index enums
  (3 / 6 / 4 values). As defined in `lib.rs:164`–`243`; mirrored here as
  `enum class`; values and ordinals FROZEN (Standards Action).
- **`DiscoCommand`** — `SetBacklight(u8) | LoadStorageSummary |
  StartEffect(DiscoEffect) | StopEffect(DiscoEffect) | ShowStatus(String)
  | NoOp`. As defined in `lib.rs:142`; mirrored here as a
  `std::variant`; variant set FROZEN (Standards Action).
- **`DiscoEffect`** — `AudioScope | StarCrawl`. As defined in
  `lib.rs:133`; mirrored here as `enum class`; FROZEN.
- **`DiscoCapabilities`** — `{ audio, storage, diagnostics, effects,
  pointer: bool; platform: &'static str }` with presets `simulator()`,
  `stm32h747i_disco()`, `uefi()`, `zephyr()`, `beaglebone_black()`. As
  defined in `lib.rs:38`; mirrored here as an aggregate with
  `static constexpr` factories; field set FROZEN (Standards Action).
- **`IconStrip` / `IconSlot`** (`SLOT_COUNT = 3`) — right-edge carousel.
  As defined in `rlvgl/examples/apps/disco-demo/src/icon_strip.rs`;
  mirrored here as a `core::Widget` subclass.
- **`Wing` / `WingSlot`** (`MAX_SLOTS = 6`) — collapsible left-edge panel.
  As defined in `.../src/wing.rs`; mirrored here as a `core::Widget`.
- **`DashboardPanel`** — centered detail panel (title/caption/lines/
  accent, show/hide, close-hit). As defined in
  `.../src/dashboard_panel.rs`; mirrored here as a `core::Widget`.
- **`ActionHotspot`** — invisible tap target with an activation closure
  and a visibility predicate. As defined in `.../src/hotspot.rs`;
  mirrored here as a `core::Widget`.
- **`EventWindow`** — transient floating notification. As defined in
  `rlvgl/ui/src/event_window.rs:34`; mirrored here as `lvglpp::ui`
  (does not exist in repo yet — §14).
- **`Screen`** — display descriptor (logical size, rotation). As defined
  in `rlvgl/platform/src/screen.rs:142`; mirrored here as a minimal
  descriptor or replaced by explicit `width,height` at the call site
  (decision D4, §8).
- **`Event` / `Key`** — input sum types. As defined in
  `rlvgl/core/src/event.rs:43,118`; mirrored here as
  `core::Event` / `core::Key`
  (`core/include/lvglpp/core/event.hpp`); used without modification.
- **`WidgetNode`** — owning widget-tree node. As defined in
  `rlvgl/core/src/…`; mirrored here as `core::WidgetNode`
  (`core/include/lvglpp/core/widget_node.hpp:43`); used without
  modification.
- **observer (raw `T*`)** — non-owning, nullable access to a widget
  owned by the tree; valid only for the tree's lifetime. *Owned by DEMO*
  (the ownership vocabulary this initiative originates).

---

## §4 Source-of-truth map

One owner per concept across the three trees:

| Concept | lvgl | rlvgl (canonical) | lvglpp (mirror) |
| --- | --- | --- | --- |
| App-shell FSM / nav | — | `apps/disco-demo/src/lib.rs` | DEMO (new) |
| `DiscoCommand` / `DiscoEffect` | — | `lib.rs:133,141` | mirror, FROZEN |
| `DiscoCapabilities` | — | `lib.rs:38` | mirror, FROZEN |
| Slot/Focus enums | — | `lib.rs:164`–`243` | mirror, FROZEN |
| `IconStrip`/`Wing`/`DashboardPanel`/`ActionHotspot` | — | `icon_strip.rs`/`wing.rs`/`dashboard_panel.rs`/`hotspot.rs` | DEMO (new `core::Widget`s) |
| Layout constants | — | `assets.rs:1`, `lib.rs:225` | mirror, FROZEN data |
| `Event`/`Key` | `lv_event_t` (informative) | `core/src/event.rs` | `core/event.hpp` (already mirrored) |
| `WidgetNode`/`Widget`/`Renderer`/`Style`/`Color`/`Rect` | `lv_obj_t` (informative) | `core/src/…` | `core/*.hpp` (already mirrored) |
| `BitmapFont` / `FONT_6X10` | upstream fonts (informative) | `core/src/bitmap_font.rs` | `core/font.hpp`, `fonts/font_6x10.hpp` |
| `Container` widget | `lv_obj`/`lv_cont` | `widgets/src/container.rs` | DEMO/widgets (new) — §14 |
| `EventWindow` | — | `ui/src/event_window.rs:34` | `ui/` (new) — §14 |
| `draw_panel_header`/`panel_close_hit` | — | `ui/src/draw_helpers.rs:41,98` | `core/draw_helpers.hpp` or `ui/` (new) — §14 |
| RLE icon decode + assets | — | `rlvgl-decomp` + `assets/icons/*.rle` | DEMO-04: runtime decoder (consume-only) + asset wiring |
| **C++ ownership / LVGL encapsulation** | — | — | **DEMO (originates here, §5)** |

---

## §5 FROZEN — Ownership & LVGL-encapsulation model (load-bearing)

This is the normative core of the initiative. rlvgl is canonical for
*behavior*; lvglpp owns the *ownership shape* (CLAUDE.md § "Cross-Project
Parity": "Public API names should follow C++ idioms… the *ownership
story* must match"). The mapping is **Rc<RefCell> → single-owner tree +
documented non-owning observation**.

**O-1 — The tree is the sole owner.** Every widget MUST be owned exactly
once, by the `core::WidgetNode` tree (`widget : std::unique_ptr<Widget>`,
`children : std::vector<WidgetNode>`). No `std::shared_ptr` widgets. RAII
destruction flows from the root.

**O-2 — `DiscoController` is the outer owner.** `DiscoController` owns the
root `WidgetNode` (by value) and owns its `ControllerState` (by
`std::unique_ptr`, for a stable address). It MUST outlive every widget in
the tree. Construction returns ownership via a factory
(`make_disco_controller(...)` / `try_make`), never via an ambiguous
setter.

**O-3 — `ControllerState` observes, it does not own.** The widgets the
controller mutates post-construction (`dashboard`, `subtitle`, `footer`,
`event_window`, `icon_strip`, `settings_wing`, `info_wing`) are reached
through **raw observing pointers** captured once after the tree is built.
Each such member MUST carry the ownership comment form:

```cpp
DashboardPanel* dashboard_;  // observes; owned by the widget tree (DiscoController::root_);
                             // valid for the controller's lifetime. Never freed here.
```

**O-4 — Pointer-stability invariant (the load-bearing fact).** Observers
are `Widget*` (or concrete-subclass `*`), **never** `WidgetNode*`.
Rationale: appending children may reallocate a `std::vector<WidgetNode>`,
which moves `WidgetNode` objects, but each `WidgetNode` holds its widget
behind `std::unique_ptr` — moving the node transfers the pointer and does
**not** relocate the underlying heap `Widget`. Therefore a `Widget*`
captured after tree construction remains valid for the tree's lifetime; a
`WidgetNode*` would not. Implementations MUST capture observers only
after the tree is fully assembled, and MUST NOT store `WidgetNode*` in
`ControllerState`.

**O-5 — Callbacks observe the controller.** Tap callbacks (`IconSlot` /
`WingSlot` `on_tap`, `ActionHotspot` activation) are
`std::function<void()>` / `std::function<void(std::size_t)>` that capture
a `ControllerState*` (observes). Visibility predicates
(`ActionHotspot::with_visibility`) are `std::function<bool()>` capturing a
`const Wing*` (observes). Because the controller (O-2) outlives the tree
that owns the widgets that hold these callbacks, every callback target is
guaranteed live when invoked. This MUST be stated at each capture site
per CLAUDE.md rule 9 (callback lifetimes documented and mechanically
safe). No callback may capture a `WidgetNode*` or own any widget.

**O-6 — No ownership cycle.** Ownership is a strict tree: controller →
{root tree → widgets, state}. Every back-edge (state→widget,
callback→state, predicate→wing) is a **non-owning observation**, so there
is no cycle and no need for weak/shared counting. This is the structural
reason the `Rc<RefCell>` graph collapses to single ownership.

**O-7 — LVGL handles stay encapsulated.** Any `lv_obj_t*` / `lv_*_t*`
acquired by a widget MUST be wrapped in a destructor-bearing type
(`core::Runtime` / `ObjectView` or a widget-local RAII member) and marked
`// external: lifecycle controlled by the LVGL object tree` or `// owns:
… via lv_obj_del`. Raw LVGL handles MUST NOT escape a widget's public API
(CLAUDE.md § "Strict and Explicit Ownership" rule 10 + code-review
checklist). The composite widgets in this initiative are pure-C++
`core::Widget` subclasses and SHOULD avoid holding raw `lv_obj_t*` at all;
where they must, O-7 governs.

**O-8 — Move semantics.** `DiscoController` is move-only (copy deleted).
Moving it moves the root tree (widgets stay heap-stable per O-4) and
moves the `unique_ptr<ControllerState>` (address stable), so all
observers (O-3) and callbacks (O-5) remain valid across a move. `tick` /
`dispatch_event` take `*this` by mutable reference.

---

## §6 FROZEN — Composite-widget contracts

Each mirrors an rlvgl type as a `core::Widget` subclass
(`bounds()`, `draw(Renderer&)`, `handle_event(const Event&) -> bool`,
optional `clear_region()`), preserving observable behavior:

- **`IconStrip`** (`icon_strip.rs`) — `SLOT_COUNT = 3` slots
  `{rle, enabled, on_tap}`; `set_slot`, `set_focused_slot`,
  `focused_slot`; `handle_event` emits `on_tap(index)` on
  `event::PressRelease` within a slot; draws focus highlight. **FROZEN:**
  `SLOT_COUNT = 3`.
- **`Wing`** (`wing.rs`) — `MAX_SLOTS = 6`; `{rle, enabled, on_tap}`
  slots; `toggle_visible`, `close`, `is_visible`, `set_focused_slot`;
  `bounds()` collapses to zero when hidden; `clear_region()` returns the
  paint-over rect for `CLEAR_FRAMES = 3` after close. **FROZEN:**
  `MAX_SLOTS = 6`, `CLEAR_FRAMES = 3`, and the color/geometry constants
  in `wing.rs` (`BG_COLOR`, `BORDER_COLOR`, `RADIUS = 18`, …).
- **`DashboardPanel`** (`dashboard_panel.rs`) — `{title, caption, lines,
  accent, visible}`; `set_title/caption/lines/accent`, `show/hide`,
  `is_visible`; word-wraps to panel width; `handle_event` consumes a
  close-hit (top-right) via `panel_close_hit`; `bounds()` collapses when
  hidden. **FROZEN:** the palette + `PADDING = 20`.
- **`ActionHotspot`** (`hotspot.rs`) — `{bounds, on_tap, is_visible}`;
  draws nothing; `bounds()` collapses to zero when `is_visible()` is
  false; `handle_event` fires `on_tap()` on `event::PressRelease`.

Layout constants (FROZEN, mirror `assets.rs:1` + `lib.rs:225`):
`DISPLAY_WIDTH = 800`, `DISPLAY_HEIGHT = 480`, `PANEL_WIDTH = 620`,
`PANEL_HEIGHT = 312`, `PANEL_X = 84`, `PANEL_Y = 84`,
`STRIP_ICON_SIZE = 60`, `STRIP_MARGIN_TOP = 17`, `STRIP_GAP = 10`,
`STRIP_X_OFFSET = 70`, `WING_X = 10`, `WING_ICON_SIZE = 60`,
`WING_MARGIN_TOP = 17`, `WING_GAP = 10`,
`FOCUS_HIGHLIGHT_COLOR = Color{0,180,255,255}`, `FOCUS_BORDER_WIDTH = 2`.

---

## §7 FROZEN — Event, Key, and FSM mapping

**E-1 — Event mapping.** The controller consumes exactly three event
shapes (`lib.rs:1251`); all map 1:1 to `core::event::*`
(`core/event.hpp`):

| rlvgl `Event` | lvglpp `core::event::` | Handler |
| --- | --- | --- |
| `Tick` (`event.rs:45`) | `Tick` | tick_count++, footer every 600, re-render active info page |
| `KeyDown { key }` (`event.rs:105`) | `KeyDown{ Key }` | `handle_key` |
| `PressRelease { x, y }` (`event.rs:89`) | `PressRelease{ x, y }` | gated by `capabilities.pointer`; else `push_status("ignored")` |

Other `core::event` variants (PointerDown/Move/Up, PressDown, DoubleTap,
Touch, KeyUp) reach the **widget tree** via `WidgetNode::dispatch_event`
but are not consumed by the controller's `handle_event` — parity with
rlvgl, which dispatches to the tree first then handles the three shapes
(`lib.rs:1244`).

**E-2 — Key mapping.** `core::Key` (`event.hpp`) covers every key the
FSM needs: `ArrowUp/Down/Left/Right`, `Enter`, `Space`, `Escape`, and
hotkeys via `key::Character{codepoint}` (`'s' 'f' 'i' 'b'`,
case-insensitive). `rlvgl::Key::Character(char)` (`event.rs:136`) →
`key::Character{ static_cast<uint32_t>(ch) }`.

**E-3 — FROZEN FSM.** States: `Main(0..3)`, `Wing(Settings, 0..6)`,
`Wing(Info, 0..4)`. Start: `Main(0)`, both wings closed. Transitions
(mirror `lib.rs` `cycle_main_focus`/`cycle_wing_focus`/`activate_*`/
`close_wings`/`handle_key`):

| Input | Main(i) | Wing(Settings,i) | Wing(Info,i) |
| --- | --- | --- | --- |
| ArrowUp | main −1 (wrap) | wing −1 (wrap) | wing −1 (wrap) |
| ArrowDown | main +1 (wrap) | wing +1 (wrap) | wing +1 (wrap) |
| ArrowLeft | main −1 | close → Main(i) | close → Main(i) |
| ArrowRight | main +1 | close → Main(i) | close → Main(i) |
| Enter/Space | `activate_main(MainSlot::from(i))` | `activate_settings(...)` | `activate_info(...)` |
| Escape | no-op | close_wings | close_wings |
| `s`/`f`/`i` | activate Settings/Files/Info | (after wing closes) | (after wing closes) |
| `b` | activate Backlight | activate Backlight | activate Backlight |

`activate_main(Settings)` opens the settings wing → `Wing(Settings,0)`;
`activate_main(Info)` opens the info wing → `Wing(Info,0)`;
`activate_main(Files)` loads storage and closes wings. Slot gating by
`DiscoCapabilities` (audio/effects) mirrors `lib.rs` exactly. Any FSM
edge change is Standards Action.

---

## §8 FROZEN-with-decisions — Command queue, capabilities, open D-items

**C-1 — Command queue.** `ControllerState` holds
`std::vector<DiscoCommand> commands`; `queue(cmd)` appends;
`drain_commands()` returns and clears
(`std::vector` move-out, mirroring `core::mem::take`, `lib.rs:1289`). The
runtime adapter (host main loop, later the board loop) drains each frame
and executes side effects. Effect execution is **out of scope** for the
shell (§11).

**C-2 — `publish_status`** sets footer + event window + queues
`ShowStatus` (`lib.rs:1294`), mirrored verbatim.

Decisions (RESOLVED by owner 2026-06-07; now frozen, full-parity path —
no first-milestone shortcuts):

- **D1 — EventWindow — RESOLVED: port up front.** Mirror
  `rlvgl/ui/src/event_window.rs:34` (+ `EventWindowBuilder:279`) into
  lvglpp `ui/` as chapter **DEMO-03**; `ui/` leaves stub state in this
  initiative. Notifications have full floating-toast parity from the
  first run.
- **D2 — Icons — RESOLVED: RLE decode up front.** Port an
  `rlvgl-decomp`-equivalent RLE decoder and consume
  `examples/apps/disco-demo/assets/icons/*.rle` so icons render
  pixel-faithful from the first run (chapter **DEMO-04**). This is a
  deliberate, scoped touch of the otherwise-deferred asset path
  (CLAUDE.md § "`creator-cpp` is deferred"): we *consume* locally mirrored
  rlvgl assets and decode at runtime; we do **not** add lvglpp-side asset
  *generation*. That boundary stays intact.
- **D3 — `Container` home — RESOLVED: `widgets/`.** Add `Container` as a
  general-purpose widget under `widgets/`, mirroring
  `rlvgl/widgets/src/container.rs` (chapter **DEMO-01**). Reusable
  primitive; matches rlvgl's crate split.
- **D4 — `Screen` — RESOLVED: mirror now.** Mirror
  `rlvgl/platform/src/screen.rs:142` as
  `Screen { width, height, rotation }` immediately (under `platform/`),
  so `DiscoController::make(Screen, DiscoCapabilities)` matches the rlvgl
  constructor 1:1 (`lib.rs:831`). Updates §9.

All four are now **frozen**; subsequent chapters inherit them.

---

## §9 FROZEN — Public API surface (C++ shape)

Target signatures (idiomatic C++, ownership per §5):

```cpp
// owns: the widget tree and controller state; outlives both.
class DiscoController {
 public:
  // make_*: creates and returns ownership (CLAUDE.md naming).
  // D4 RESOLVED: takes Screen (mirrors rlvgl lib.rs:831 1:1).
  static DiscoController make(platform::Screen screen,
                              DiscoCapabilities caps);
  DiscoController(DiscoController&&) noexcept;            // move-only (O-8)
  DiscoController(const DiscoController&) = delete;

  [[nodiscard]] bool dispatch_event(const core::Event& e); // tree then handle
  void handle_event(const core::Event& e);
  void tick();
  [[nodiscard]] std::vector<DiscoCommand> drain_commands();
  void publish_status(std::string text);
  [[nodiscard]] core::WidgetNode& root() noexcept;         // borrows; for draw/dispatch
};
```

`root()` returns a **borrow** (`WidgetNode&`), not a shared handle — the
host loop calls `controller.root().draw(renderer)` and
`controller.dispatch_event(e)` each frame, exactly as
`examples/host_sdl_label/main.cpp` walks its `root`.

---

## §10 Reconciliation vs. adjacent rlvgl/lvglpp primitives (informative)

- **vs. `Rc<RefCell<WidgetNode>>`** — replaced by single-owner tree +
  observers (§5). Behavioral parity is preserved because the only thing
  the shared handles provided was *aliased mutation*, which §5 supplies
  via non-owning `Widget*` with a documented lifetime.
- **vs. `core::WidgetNode`** — used without modification; this initiative
  adds composite `Widget` subclasses but does not alter the node type or
  its dispatch/draw semantics (`widget_node.hpp:43`).
- **vs. `core::event::Event`** — used without modification; the
  controller consumes a 3-variant subset (§7) and is inert to the rest.
- **vs. playit** — `core::event::PressRelease` injected by the playit
  `T<x>,<y>` / `T@<tag>` path drives the same FSM edges as SDL pointer
  input; the disco-demo tags (`root` children) make `QB:`/`QE:`/`QC:`
  queries meaningful. No new playit commands are introduced.
- **vs. `ui::EventWindow`** — pending D1; if stubbed, the footer label
  carries notifications and EventWindow parity is deferred, noted in the
  later chapter's change log.

---

## §11 Non-goals (informative)

- Board display/touch drivers (PLAT-02d–f) — separate initiative; this
  shell targets host-SDL first and is display-agnostic.
- Effect **execution** (StarCrawl / AudioScope rendering, audio codec) —
  the shell only *queues* `StartEffect`/`StopEffect`; the runtime
  executes. Host stubs these.
- `creator-cpp` / lvglpp-side asset generation — deferred (CLAUDE.md).
  This initiative consumes locally mirrored RLE assets at most (D2).
- FreeRTOS/Zephyr entry parity — the rlvgl binary's RTOS glue is board
  bringup, not app shell.
- Wrapping every rlvgl widget — only the four composites the demo needs.

---

## §12 Acceptance checklist (normative)

Ratification of DEMO-00 requires:

- [ ] §0 authority policy agreed; registration policy = Standards Action
      recorded.
- [ ] §3 glossary uses only the four reference/restatement forms; every
      code term cites rlvgl/lvgl/lvglpp source.
- [ ] §4 one-owner-per-concept map complete; no concept dual-owned.
- [ ] §5 ownership model (O-1…O-8) reviewed against CLAUDE.md § "Strict
      and Explicit Ownership" — no `shared_ptr` widget, no `WidgetNode*`
      in state, every observer + callback lifetime documented, LVGL
      handles encapsulated (O-7).
- [ ] §6/§7/§8 frozen constants, FSM edges, command/capability sets
      verified byte-for-byte against rlvgl source cited.
- [ ] D1–D4 (§8) resolved by the owner and folded into the owning
      chapter.
- [ ] §14 prerequisite chapters identified with codes.
- [ ] §15 dated change-log entry added (this is what flips status to
      ratified).

Downstream (implementation chapters) acceptance, recorded here for
traceability:

- [ ] A C++ parity test suite mirrors rlvgl's
      `cargo test -p rlvgl-app-disco-demo` cases (navigation, focus,
      hotkeys, command emission, focus-highlight wiring).
- [ ] `examples/.../disco-demo` host-SDL target builds under embedded-OFF
      posture and runs; playit `T@<tag>` drives the FSM.
- [ ] Pre-Publish Validation (CLAUDE.md) phases 0–3 green.

---

## §13 Files cited

Canonical (rlvgl `v0.2.0`):
`rlvgl/examples/apps/disco-demo/src/lib.rs` (`:36,133,141,157,225,285,820,
831,1240,1244,1251,1283,1289,1294`),
`.../src/icon_strip.rs`, `.../src/wing.rs`, `.../src/dashboard_panel.rs`,
`.../src/hotspot.rs`, `.../src/assets.rs:1`,
`.../README.md`,
`rlvgl/core/src/event.rs:43,89,105,118,126,136`,
`rlvgl/ui/src/event_window.rs:34,279,295`,
`rlvgl/ui/src/draw_helpers.rs:41,98`,
`rlvgl/platform/src/screen.rs:142`.

Mirror target (lvglpp):
`core/include/lvglpp/core/event.hpp`,
`core/include/lvglpp/core/widget.hpp`,
`core/include/lvglpp/core/widget_node.hpp:43`,
`examples/apps/disco-demo/assets/icons/*.rle`,
`core/include/lvglpp/core/renderer.hpp`,
`core/include/lvglpp/core/style.hpp`,
`core/include/lvglpp/core/draw_helpers.hpp`,
`core/include/lvglpp/core/font.hpp`,
`core/include/lvglpp/core/fonts/font_6x10.hpp`,
`widgets/include/lvglpp/widgets/{label,button,checkbox,switch,slider}.hpp`,
`platform/include/lvglpp/platform/host_sdl.hpp`,
`examples/host_sdl_label/main.cpp`,
`ui/include/lvglpp/ui/ui.hpp` (stub).

---

## §14 Unblocks / prerequisite chapters

DEMO-00 unblocks, and the controller implementation depends on, these
prerequisite chapters (each needs its own ratified concepts doc before
code):

- **DEMO-01 — `Container` widget** (D3). Mirrors
  `rlvgl/widgets/src/container.rs`; lands in `widgets/`.
- **DEMO-02 — UI draw helpers**: `draw_panel_header` + `panel_close_hit`
  (`rlvgl/ui/src/draw_helpers.rs:41,98`) into lvglpp `ui/`.
- **DEMO-03 — `EventWindow`** (D1, firm). Mirror
  `rlvgl/ui/src/event_window.rs:34` (+ builder `:279`) into lvglpp `ui/`
  (moves `ui/` off stub). Depends on DEMO-02 (panel-header helper).
- **DEMO-04 — RLE icons** (D2, firm). RLE decoder
  (`rlvgl-decomp`-equivalent, consume-only) + `assets/icons/*.rle`
  wiring; renders via `Renderer::draw_pixels`.
- **DEMO-0S — `Screen` descriptor** (D4). Mirror
  `rlvgl/platform/src/screen.rs:142` as
  `platform::Screen{ width, height, rotation }`. Small; gates DEMO-06's
  constructor signature.
- **DEMO-05 — Composite widgets**: `IconStrip`, `Wing`, `DashboardPanel`,
  `ActionHotspot` (§6) as `core::Widget` subclasses.
- **DEMO-06 — `DiscoController` + `ControllerState`** (§5, §7, §9) and the
  host-SDL target + parity tests (§12).

Dependency-ordered:
- **Wave A (independent, parallel):** DEMO-01, DEMO-02, DEMO-04, DEMO-0S.
- **Wave B:** DEMO-03 (needs 02); DEMO-05 (needs 01, 02, 04).
- **Wave C:** DEMO-06 (needs 03, 05, 0S).

Each chapter requires its own ratified concepts doc before code
(CLAUDE.md § "Execution discipline"); DEMO-00 §5/§6 already freeze their
contracts, so those chapter docs are thin — they restate the frozen
contract, name the files, and add an acceptance checklist.

---

## §15 Change log

(Append-only; a dated entry here flips this chapter from drafted to
ratified.)

- _drafted_ — initial DEMO-00 contract: authority, glossary,
  source-of-truth map, ownership model (O-1…O-8), composite-widget
  contracts, event/FSM/command/capability mapping, prerequisite chapter
  breakdown. Pending owner sign-off on D1–D4 (§8) and a dated
  ratification entry.
- **2026-06-07 — ratified.** Owner resolved D1–D4 (§8): EventWindow
  ported up front (DEMO-03), RLE icons up front (DEMO-04, consume-only —
  no lvglpp-side asset generation), `Container` in `widgets/` (DEMO-01),
  `Screen` mirrored now (DEMO-0S); §9 constructor updated to
  `make(Screen, DiscoCapabilities)`. Full-parity path, no first-milestone
  shortcuts. §14 dependency waves recorded. Execution of prerequisite
  chapters may proceed once each is itself ratified.
