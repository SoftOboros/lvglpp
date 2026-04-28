# 00 — Tagged queries + Dispatcher

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAYIT-04**.

## §0 Authority

- `Response` variant set: `rlvgl/playit/src/response.rs` (v0.2.0 @
  b178cbc). Canonical.
- Tagged-command routing: `rlvgl/playit/src/executor.rs` +
  `rlvgl/playit/src/tag.rs`. Canonical.
- Underlying tree shape: `lvglpp::core::WidgetNode` (CORE-03a).
- Underlying widget surface: `lvglpp::core::Widget` (CORE-03).
- Wire-protocol command set: PLAYIT-01.

## §1 Purpose

Bind parsed `lvglpp::playit::Command` values to a real widget tree,
so cross-language playit fixtures targeting tagged widgets drive
lvglpp targets identically to rlvgl targets.

## §3 Canonical glossary

- **`Response`** — Owned by this chapter. Mirrored as
  `lvglpp::playit::Response` at
  `playit/include/lvglpp/playit/response.hpp`. Variant set per §5.1.
- **`Dispatcher`** — Owned by this chapter. Mirrored as
  `lvglpp::playit::Dispatcher` at
  `playit/include/lvglpp/playit/dispatcher.hpp`. Wraps a
  `WidgetNode&` (root) and turns each parsed `Command` into a
  `Response`.
- **`StatusData`** — As defined in `rlvgl/playit/src/response.rs:5`;
  mirrored as `lvglpp::playit::StatusData` with the same two
  fields (`tick_count`, `present_count`).

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| `Response` variants | `rlvgl/playit/src/response.rs` (canonical) | `lvglpp::playit::Response`. |
| Tagged-command routing | this chapter — **Standards Action** | rlvgl + lvglpp PR pair if behaviour changes. |
| `find_by_tag` walker | `lvglpp::core::find_by_tag` (CORE-03a §5.4) | reused by Dispatcher. |

## §5 Frozen decisions

### §5.1 `Response` variants — **Standards Action**

Mirrors `rlvgl/playit/src/response.rs:14` exactly:

| Variant | Payload |
| --- | --- |
| `Ok` | none |
| `Error` | `std::string_view reason` (borrows; lifetime is the call's) |
| `Bounds` | `int32_t x, y, width, height` |
| `Exists` | `bool` |
| `ChildCount` | `uint16_t` |
| `Status` | `StatusData{tick_count: uint32_t, present_count: uint32_t}` |
| `DumpEnd` | none |

C++ form: `using Response = std::variant<…>;` with per-variant POD
structs under `lvglpp::playit::response::*`, parallel to the
EventSpec / Command structures from PLAYIT-01.

### §5.2 `Dispatcher` shape — **Standards Action**

```
class Dispatcher {
public:
    explicit Dispatcher(WidgetNode& root) noexcept;

    // Owns the supplied StatusData snapshot (copied in).
    void set_status_snapshot(StatusData) noexcept;

    [[nodiscard]] Response dispatch(const Command& cmd) noexcept;
};
```

- The Dispatcher **borrows** the `WidgetNode` root for its lifetime.
  Callers MUST keep the tree alive at least as long as the
  Dispatcher.
- `set_status_snapshot` lets the application provide tick / present
  counters that the `Status` command will read. Defaults to
  zero-initialised.
- `dispatch` is `noexcept` — internal failures (bad cast, etc.)
  produce a `Response::Error` rather than throwing. This is required
  by embedded posture (`docs/std-mapping.md` § "Embedded posture").

### §5.3 Per-command mapping — **Standards Action**

The Dispatcher's behaviour for each `Command` variant:

| Command | Behaviour |
| --- | --- |
| `Status` | returns `Response::Status{snapshot}`. |
| `Inject{event_spec}` | converts to core Event via PLAYIT-02 `to_event`, calls `root_.dispatch_event(event)`, returns `Response::Ok`. |
| `InjectTagged{tag, event_spec}` | `find_by_tag(root_, tag)` → if found, calls **`node->widget->handle_event(event)`** (single-node dispatch, NOT recursive — rlvgl semantics; tagged inject targets exactly the named widget). Returns `Response::Ok` on hit, `Response::Error{"tag not found"}` on miss. |
| `QueryBounds{tag}` | `find_by_tag` → returns `Response::Bounds{...node->widget->bounds()...}` or `Response::Error`. |
| `QueryExists{tag}` | returns `Response::Exists{find_by_tag(...) != nullptr}`. Never errors. |
| `QueryChildCount{tag}` | `find_by_tag` → returns `Response::ChildCount{static_cast<uint16_t>(node->children.size())}` or `Response::Error`. |
| `DumpPixels`, `RecordStart`, `RecordStop`, `RecordDump` | return `Response::Error{"not implemented"}` — each lands in its own follow-up sub-phase (PLAYIT-05, PLAYIT-06). |
| `Extension{payload}` | returns `Response::Error{"unhandled extension"}`. Application code that wants to handle extensions can subclass / wrap Dispatcher. |

### §5.4 Single-node dispatch for `InjectTagged`

`InjectTagged` does **not** recurse through the tagged node's
subtree. The named widget is the exact target — calling
`node->widget->handle_event(event)` directly. This matches rlvgl's
behaviour at `rlvgl/playit/src/executor.rs` and is the contract
playit fixtures rely on.

If a fixture wants the broader DFS dispatch on a tagged node, it
sends `Inject` (untagged) at coordinates within the tagged region.

## §10 Reconciliation vs. adjacent primitives

- **PLAYIT-02 EventPipeline.** PLAYIT-04 routes Commands; PLAYIT-02
  routes Events. They compose: typically the Dispatcher's `Inject`
  path runs the converted Event through a recogniser pipeline
  before dispatching. **For PLAYIT-04 v1, the Dispatcher dispatches
  raw Events directly** — recogniser composition is a follow-up
  (PLAYIT-04a) once a real test fixture needs gestures from the
  command stream.
- **`lvglpp::playit::parse_command` (PLAYIT-01).** The Dispatcher
  takes already-parsed Commands; it does not own the line buffer.
  Tag fields are `std::string_view` borrowing from that buffer.

## §11 Non-goals

- **Framebuffer dump (`D<x>,<y>,<w>,<h>`).** PLAYIT-05.
- **Event recorder (`RS` / `RE` / `RD`).** PLAYIT-06.
- **Extension command handling.** Application-defined.
- **Response serialization back to the wire format.** A
  `format_response(...)` helper lands in a sibling sub-phase
  (PLAYIT-04b) once the host SDL backend grows a serial-driver
  pretend-mode test.

## §12 Acceptance checklist

- [ ] `lvglpp::playit::Response` is a `std::variant` over the seven
      §5.1 variants.
- [ ] `lvglpp::playit::StatusData` mirrors rlvgl's two fields.
- [ ] `lvglpp::playit::Dispatcher` exposes the §5.2 surface.
- [ ] Per-command behaviour matches §5.3.
- [ ] `InjectTagged` dispatches **only** to the tagged node, not
      its subtree (§5.4).
- [ ] PARITY/LVGL/DELTA cite block on every public header.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] Test fixture builds a small `WidgetNode` tree with a tagged
      Button, dispatches `T@<tag>:<x>,<y>`, and asserts the
      Button's `on_click` fired exactly once.
- [ ] Test covers `QueryBounds`, `QueryExists`, `QueryChildCount`
      (positive + missing-tag).
- [ ] `playit/STATUS.md` change log records PLAYIT-04 landing.

## §13 Files cited

- `rlvgl/playit/src/response.rs`, `executor.rs`, `tag.rs` (v0.2.0
  @ b178cbc).
- `lvglpp/docs/playit-tagged/` (this initiative).
- `lvglpp/docs/core-widget/01-widget-node.md`.
- `lvglpp/docs/widgets-button/00-button.md`.

## §14 Unblocks

- Cross-language fixtures driving lvglpp widget trees identically
  to rlvgl. The same probe driver, the same wire protocol, the
  same expected Responses.
- **PLAYIT-05** (framebuffer dump) and **PLAYIT-06** (recorder)
  layer on top of the Dispatcher.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Response variants
  (§5.1), Dispatcher shape (§5.2), per-command behaviour (§5.3),
  single-node InjectTagged semantics (§5.4) frozen. Execution
  unblocked.
- 2026-04-27 — PLAYIT-04 execution landed. `Response` (variant) +
  `Dispatcher` defined; routing matches §5.3 row-for-row. Test
  target `lvglpp_playit_dispatcher` exercises the full
  cross-language closure: `parse_command("T@ok:80,90")` →
  Dispatcher → WidgetNode tree → Button::on_click. 12/12 ctest
  entries green across the project. Embedded posture clean.
