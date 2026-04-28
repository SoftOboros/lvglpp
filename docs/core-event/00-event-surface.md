# 00 — Event surface

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-02**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact. The initiative
[`README.md`](./README.md) is informative.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| Event variant set, payload field shapes, dispatch ordering | `rlvgl/core/src/event.rs` (v0.2.0 @ b178cbc) | Canonical for the value-level shape. lvglpp mirrors 1:1. |
| Wire-format event types (`EventSpec`, `KeySpec`, `TouchPointSpec`, `TouchStateSpec`) | `rlvgl/playit/src/command.rs` (v0.2.0 @ b178cbc) | Used by `lvglpp::playit`; convertible to `lvglpp::core::Event` via `to_event()` at the boundary. |
| C++ surface naming, ownership tags, freestanding-subset rules | this chapter + `docs/std-mapping.md` | Normative for lvglpp. |
| Underlying widget-tree event substrate | `lvgl/src/core/lv_obj_event.h` | Informative only. `lv_event_t` does not shadow `lvglpp::core::Event`; see §10. |

## §1 Purpose

Define `lvglpp::core::Event` and the supporting types every lvglpp
widget, backend, and `playit` dispatcher will compile against. Freeze
the variant set and the `MAX_TOUCH_POINTS` constant so subsequent
phases (CORE-03 Widget node, CORE-04 Renderer, every WID-NN, every
PLAT-NN, PLAYIT-02 onwards) can rely on a stable contract without
re-litigating shape decisions.

## §2 Problem statement

No widget can be implemented before the event surface is stable.
rlvgl's `Event` enum at `rlvgl/core/src/event.rs:43` defines exactly
ten variants: `Tick`, three raw pointer variants
(`PointerDown`/`PointerUp`/`PointerMove`), two debounced pointer
variants (`PressDown`/`PressRelease`), `DoubleTap`, two keyboard
variants (`KeyDown`/`KeyUp`), and a `Touch` multi-point frame. The
playit wire protocol at `rlvgl/playit/src/protocol.rs:1` is built
around exactly this set. lvglpp must mirror the set 1:1; any drift
forks the rlvgl/lvglpp pair at the most painful place — the test
harness.

## §3 Canonical glossary

For each term, the form follows CLAUDE.md
§ "Definitions — reference vs. restatement".

- **`Event`** — As defined in `rlvgl/core/src/event.rs:43`; will be
  mirrored as `lvglpp::core::Event` at
  `core/include/lvglpp/core/event.hpp` once CORE-02 execution lands.
  The rlvgl definition is canonical.
- **`EventKind`** — Owned by this chapter; does not exist in repo
  yet. The C++ surface MAY expose an `enum class EventKind : uint8_t`
  discriminator parallel to a `std::variant`-based `Event`, or fold
  the discriminator into the variant index. Either is conformant.
- **`TouchState`** — As defined in `rlvgl/core/src/event.rs:8`;
  mirrored as `lvglpp::core::TouchState`. Three variants: `Down`,
  `Up`, `Contact`.
- **`TouchPoint`** — As defined in `rlvgl/core/src/event.rs:19`;
  mirrored as `lvglpp::core::TouchPoint`. Fields: `id` (`uint8_t`),
  `x` (`int32_t`), `y` (`int32_t`), `state` (`TouchState`).
- **`MAX_TOUCH_POINTS`** — As defined in
  `rlvgl/core/src/event.rs:4` (= `5`); used without modification.
  Reflects the FT5336 controller hardware ceiling on STM32H747I-DISCO.
- **`Key`** — As defined in `rlvgl/core/src/event.rs:118`; mirrored
  as `lvglpp::core::Key`. Variants enumerated in §5.3.
- **`EventSpec`, `KeySpec`, `TouchPointSpec`, `TouchStateSpec`** —
  As defined in `rlvgl/playit/src/command.rs:34`/`:112`/`:74`/`:63`;
  mirrored under `lvglpp::playit::*` (see PLAYIT-01). The `*Spec`
  types are wire-format-friendly parallels of the core types and are
  convertible to the core types via `to_event()` / `to_key()` /
  `to_core()` once CORE-02 execution lands.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Event variant set | `rlvgl/core/src/event.rs` | `lvglpp::core::Event` (CORE-02 exec); `lvglpp::playit::EventSpec` (PLAYIT-01). |
| Variant set extension | this chapter — **Standards Action** | rlvgl + lvglpp PR pair, with this chapter's change log amended first. |
| `TouchPoint` field shape | `rlvgl/core/src/event.rs:19` | `lvglpp::core::TouchPoint`. Layout is logical, not byte-stable; do not transmit by `memcpy`. |
| `MAX_TOUCH_POINTS` constant | `rlvgl/core/src/event.rs:4` | Frozen at `5`; **Standards Action** to change. |
| `Key` variant set | `rlvgl/core/src/event.rs:118` | `lvglpp::core::Key`, `lvglpp::playit::KeySpec`. |
| Wire format for events | `rlvgl/playit/src/protocol.rs` | `lvglpp::playit::parse_command` (PLAYIT-01). |

## §5 Frozen decisions

### §5.1 `Event` variants — registration policy: **Standards Action**

The variant set is frozen at exactly these ten cases. Adding a variant
requires an amendment to this chapter's change log (§15) and a
matching change to `rlvgl/core/src/event.rs` per CLAUDE.md
§ "Cross-language change ordering".

| Variant | Payload | Notes |
| --- | --- | --- |
| `Tick` | none | Periodic advance for animations / timers. |
| `PointerDown` | `int32_t x, y` | Raw pointer down. |
| `PointerUp` | `int32_t x, y` | Raw pointer up. |
| `PointerMove` | `int32_t x, y` | Raw pointer move while pressed. |
| `PressDown` | `int32_t x, y` | Debounced press began. Visual-feedback trigger. |
| `PressRelease` | `int32_t x, y` | Debounced press released. Primary tap action. |
| `DoubleTap` | `int32_t x, y` | Two short taps within the recognizer window. |
| `KeyDown` | `Key key` | Keyboard key pressed. |
| `KeyUp` | `Key key` | Keyboard key released. |
| `Touch` | `uint8_t count, std::array<TouchPoint, MAX_TOUCH_POINTS> points` | Multi-touch frame. Only `points[0..count]` are valid. |

Coordinate fields are signed `int32_t`. Off-screen coordinates are
valid (e.g. drag-out hit-test edges).

### §5.2 `TouchState` variants — **Standards Action**

`Down`, `Up`, `Contact`. Same set as `rlvgl/core/src/event.rs:8`.

### §5.3 `Key` variants — **Standards Action**

`Escape`, `Enter`, `Space`, `ArrowUp`, `ArrowDown`, `ArrowLeft`,
`ArrowRight`, `Function(uint8_t n)` (1..=12), `Character(uint32_t cp)`
(Unicode scalar value), `Other(uint32_t code)` (opaque catch-all).

The C++ representation MAY be a `std::variant`, a tagged union with a
`Kind` enum and a `uint32_t value`, or any equivalent. A conforming
implementation MUST round-trip every variant through
`lvglpp::playit::KeySpec` byte-for-byte.

### §5.4 `MAX_TOUCH_POINTS = 5` — **Standards Action**

Frozen at 5 to match the FT5336 controller on STM32H747I-DISCO and
parity with `rlvgl/core/src/event.rs:4`. Changing the constant
affects every backend that decodes touch frames; the amendment SHOULD
ride alongside any new platform that requires a different ceiling, not
preemptively.

### §5.5 Coordinate space

Coordinates in `Event` payloads are in **landscape pixel coordinates**
relative to the active screen origin, identical to rlvgl. Per-widget
relative transforms happen at dispatch (CORE-03), not in `Event`
construction.

## §10 Reconciliation vs. adjacent primitives

- **`lv_event_t` (LVGL C, `lvgl/src/core/lv_obj_event.h`).** LVGL ships
  a callback-and-opaque-struct event system. `lvglpp::core::Event`
  does **not** shadow `lv_event_t`. Events that originate inside
  lvglpp (driver-injected input, playit-injected synthetic input) are
  expressed as `lvglpp::core::Event` values; events that originate
  inside LVGL (e.g. `LV_EVENT_DRAW_*`) MAY be wrapped into
  `lvglpp::core::Event` by the renderer seam (CORE-04). The two
  systems coexist; widget-author code SHOULD use
  `lvglpp::core::Event` as the canonical type and reach for
  `lv_event_t` only inside renderer / driver translation units.
- **`lvglpp::playit::EventSpec`.** Wire-format parallel of
  `lvglpp::core::Event`. `EventSpec::to_event() -> Event` is the
  canonical conversion direction; the reverse is required only for
  the recorder dump path (PLAYIT-06) and SHOULD be implemented as
  `format_event_spec(...)` mirroring `rlvgl/playit/src/protocol.rs`.

## §11 Non-goals

- Generic / extensible payload. Widget-specific event payloads
  (e.g. `ValueChanged { new_value }`) are NOT part of this chapter.
  They belong to per-widget concepts docs (WID-NN) or to widget
  userdata, not to the core Event sum type.
- Event filtering, propagation rules, capture/bubble semantics. Those
  are CORE-03 (Widget node) territory.
- Cross-thread event posting. `lvglpp::core::Event` is dispatched on
  the LVGL tick thread; multi-threaded posting is a CORE-04 / platform
  concern.

## §12 Acceptance checklist

A conforming CORE-02 execution PR MUST satisfy:

- [ ] `lvglpp::core::Event` is a sum type with exactly the ten variants
      in §5.1, with payloads matching field-for-field.
- [ ] `lvglpp::core::TouchState` exposes the three variants in §5.2.
- [ ] `lvglpp::core::TouchPoint` exposes the four fields in §3 with
      types matching `int32_t` / `uint8_t` per §3.
- [ ] `lvglpp::core::Key` exposes the variant set in §5.3 and
      round-trips through `lvglpp::playit::KeySpec`.
- [ ] `lvglpp::core::MAX_TOUCH_POINTS` equals `5` (compile-time
      constant; freestanding-friendly).
- [ ] Each public header carries the PARITY/LVGL/DELTA cite block
      (CLAUDE.md § "Cite-block convention").
- [ ] Every raw pointer in the surface carries an ownership tag
      comment. The Event surface itself uses no raw pointers; this
      bullet is satisfied by inspection.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`
      (`-fno-exceptions -fno-rtti`).
- [ ] A unit test fixture round-trips every variant in §5.1 from
      `lvglpp::playit::EventSpec` → `lvglpp::core::Event` via
      `to_event()` and asserts equality.
- [ ] `core/STATUS.md` change log records ratification of CORE-02.

A conforming PR MAY:

- Use `std::variant<…>` or a hand-rolled tagged union for the sum type.
- Inline the supporting types (`TouchState`, `TouchPoint`, `Key`) into
  a single `event.hpp` or split per concept.
- Define `EventKind` as either an enum-class discriminator or rely on
  `std::variant::index()` — both are conformant.

## §13 Files cited

- `rlvgl/core/src/event.rs` (v0.2.0 @ b178cbc)
- `rlvgl/playit/src/command.rs` (v0.2.0 @ b178cbc)
- `rlvgl/playit/src/protocol.rs` (v0.2.0 @ b178cbc)
- `lvgl/src/core/lv_obj_event.h` (informative; v9.x)
- `lvglpp/CLAUDE.md` § "Strict and Explicit Ownership",
  § "Spec-Before-Code Planning Discipline",
  § "Doc Co-Location Policy", § "Cite-block convention"
- `lvglpp/docs/std-mapping.md` § "Sum / option types"

## §14 Unblocks

- **CORE-03** (Widget node) — needs `Event` for the
  `receive_event(...)` signature.
- **CORE-04** (Renderer) — needs `Event` for input dispatch from
  drivers.
- **WID-01** onwards — every widget's event-handling surface depends
  on this chapter.
- **PLAYIT-02** (Dispatcher) — converts parsed `EventSpec` into
  `Event` and routes it through the widget tree.
- **PLAT-NN** — every backend's input path produces `Event` values.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Variant sets in §5.1,
  §5.2, §5.3 frozen with **Standards Action** registration policy.
  `MAX_TOUCH_POINTS` frozen at 5. No execution PR has landed yet;
  CORE-02 is unblocked.
- 2026-04-27 — CORE-02 execution landed in
  `core/include/lvglpp/core/event.hpp`. All nine §12 acceptance
  bullets satisfied: variant set count and shapes match §5.1 / §5.2
  / §5.3; `MAX_TOUCH_POINTS` is a `constexpr std::size_t` equal to
  `5`; cite blocks present; embedded posture clean
  (`-DLVGLPP_EMBEDDED_POSTURE=ON` builds `lvglpp_core` and
  `lvglpp_playit` without errors); round-trip via
  `lvglpp::playit::to_event` / `to_key` / `to_core` covered by the
  `lvglpp_playit_conversion` ctest entry; `core/STATUS.md` change
  log updated. Implementation chose `std::variant<…>` for both
  `Event` and `Key` per the §12 "MAY" provision; per-variant
  payloads are aggregate structs under `lvglpp::core::event::*` and
  `lvglpp::core::key::*` namespaces.
