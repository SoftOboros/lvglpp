# 01 — Widget node (tree wrapper)

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **CORE-03a** (sub-phase under the core-widget initiative).

The key words **MUST**, **SHOULD**, **MAY** are interpreted per RFC
2119 / 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `WidgetNode` field set, dispatch order, draw order, `tag` shape | `rlvgl/core/src/lib.rs:104` (`pub struct WidgetNode`) (v0.2.0 @ b178cbc) | Canonical. |
| `find_by_tag` walker semantics (depth-first, first match wins) | `rlvgl/playit/src/tag.rs:5` | Canonical. lvglpp lifts this into `lvglpp::core` so consumers other than playit can reuse it. |
| C++ ownership shape | this chapter §5.1 | Normative for lvglpp. |

## §1 Purpose

Define the **tree wrapper** that lifts the Widget abstract base
(CORE-03 §5.1) into a hierarchical UI. `WidgetNode` owns its widget
and its children, carries an optional test-automation tag, and
provides depth-first event dispatch + draw.

## §2 Problem statement

CORE-03 froze the `Widget` virtual surface but did not model
parent / child composition. PLAYIT-04 (tagged queries) and every
multi-widget application need a tree shape with:

- Owning storage for widgets (and their children).
- A test-automation `tag` for addressing nodes by name.
- Depth-first event dispatch + draw consistent with rlvgl.

`WidgetNode` is the smallest piece that satisfies all three.

## §3 Canonical glossary

- **`WidgetNode`** — Owned by this chapter. Mirrored as
  `lvglpp::core::WidgetNode` at
  `core/include/lvglpp/core/widget_node.hpp`. Three fields
  (§5.1) and three methods (§5.2 / §5.3).
- **`tag`** — Test-automation identifier. As defined in
  `rlvgl/core/src/lib.rs:118` (`Option<&'static str>`); mirrored
  here as `std::optional<std::string_view>` with the documented
  lifetime requirement that the underlying string outlive the node
  (typically a string literal).
- **`dispatch_event`** — Depth-first propagation. Returns `true` as
  soon as any widget consumes the event (handle_event returns
  `true`). Mirrors `rlvgl/core/src/lib.rs:138`.
- **`find_by_tag`** — Depth-first lookup. Returns the first node
  whose `tag` matches, or `nullptr` if none. Mirrors
  `rlvgl/playit/src/tag.rs:5` *adapted*: lifted into
  `lvglpp::core` rather than `lvglpp::playit` so non-playit
  consumers can use it.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Tree shape (widget + children + tag) | `rlvgl/core/src/lib.rs:104` (canonical) | `lvglpp::core::WidgetNode`. |
| Dispatch order (DFS, first-consume wins) | `rlvgl/core/src/lib.rs:138` (canonical) | `WidgetNode::dispatch_event`. |
| Draw order (parent first, then children DFS) | `rlvgl/core/src/lib.rs:154` (canonical) | `WidgetNode::draw`. |
| `find_by_tag` walker | `rlvgl/playit/src/tag.rs:5` (canonical) | `lvglpp::core::find_by_tag` (free fn). |
| Field-set / method-set extension | this chapter — **Standards Action** | rlvgl + lvglpp PR pair. |

## §5 Frozen decisions

### §5.1 Field set + ownership shape — **Standards Action**

| Field | Type | Ownership | Notes |
| --- | --- | --- | --- |
| `widget` | `std::unique_ptr<Widget>` | `owns` | rlvgl uses `Rc<RefCell<dyn Widget>>` for shared+interior-mutable access; C++ doesn't need either, so we use `unique_ptr` for clarity. **DELTA documented.** |
| `children` | `std::vector<WidgetNode>` | `owns` | Recursive composition; C++20 supports incomplete-type vectors of self at definition because `WidgetNode` is complete by the end of its body. |
| `tag` | `std::optional<std::string_view>` | `borrows` | Lifetime contract: the underlying string MUST outlive the node. String literals satisfy this. |

`WidgetNode` is **move-only**: the contained `unique_ptr` is
non-copyable. Aggregate initialisation works for the simple case;
`make_node(...)` factories may exist later as ergonomic helpers.

### §5.2 `dispatch_event` semantics — **Standards Action**

```
bool dispatch_event(const Event&);
```

1. Call `widget->handle_event(event)`. If it returns `true`, return
   `true` (event consumed at this node).
2. Otherwise, iterate `children` **in order** and recurse. Return
   `true` as soon as any child returns `true`.
3. If no node consumed, return `false`.

This matches `rlvgl/core/src/lib.rs:138` step-for-step.

### §5.3 `draw` semantics — **Standards Action**

```
void draw(Renderer&) const;
```

1. Call `widget->draw(renderer)`.
2. Iterate `children` in order and recurse.

Parent draws **before** children — the natural retained-mode
back-to-front order. Mirrors `rlvgl/core/src/lib.rs:154`.

### §5.4 `find_by_tag` — **Standards Action**

```
const WidgetNode* find_by_tag(const WidgetNode&, std::string_view);
WidgetNode*       find_by_tag(WidgetNode&,       std::string_view);
```

Depth-first; root first, then children left-to-right. First match
wins. Returns `nullptr` if no node matches. Mirrors
`rlvgl/playit/src/tag.rs:5`. Lives under `lvglpp::core::` so any
consumer (not just playit) can use it.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `Rc<RefCell<dyn Widget>>` vs. lvglpp `unique_ptr<Widget>`.**
  Rust needs `Rc` because `WidgetNode::dispatch_event` borrows the
  whole tree mutably while invoking widget methods that may
  themselves want to access siblings — `RefCell` provides
  interior-mutability runtime checking. C++ has no equivalent
  borrow-checker constraint; a single owner per Widget is the
  cleanest expression of the actual ownership story. If a future
  call site needs shared widgets across nodes, swap to
  `std::shared_ptr<Widget>` with a per-call-site comment per
  CLAUDE.md ownership rule 3.
- **CORE-03 `Widget`.** `Widget` itself remains unchanged.
  `WidgetNode` is the composition layer above it.

## §11 Non-goals

- **Layout engine.** `WidgetNode` does not lay out children. That's
  CORE-05 / UI-04 territory.
- **Z-ordering / overlay management.** Children draw in vector
  order, period.
- **Capture / bubble propagation rules.** `dispatch_event` is a
  simple first-consume DFS. Phase changes belong in a follow-up
  sub-phase (CORE-03b) if a real call site needs them.
- **Reparenting.** `WidgetNode` is a value type; reorganising the
  tree means moving nodes around in the vector.

## §12 Acceptance checklist

A conforming CORE-03a execution PR MUST satisfy:

- [ ] `lvglpp::core::WidgetNode` has the three fields in §5.1 with
      the documented ownership tags.
- [ ] `dispatch_event` walks per §5.2 — verified by a unit test
      that inserts a parent + two children, two of which consume,
      and asserts dispatch order + first-consume short-circuit.
- [ ] `draw` walks per §5.3 — verified by a unit test using a
      `RecordingRenderer` and confirming parent-before-children
      order.
- [ ] `find_by_tag` (const + non-const overloads) per §5.4 —
      verified by a unit test mirroring the rlvgl tag tests
      (`find_root_tag`, `find_nested_tag`, `untagged_tree_returns_none`).
- [ ] PARITY/LVGL/DELTA cite block at file head.
- [ ] Compiles cleanly under `LVGLPP_EMBEDDED_POSTURE=ON`. Note
      `<vector>` is in the "Allowed (with care)" tier of
      `docs/std-mapping.md` § "Freestanding subset" — embedded
      consumers MUST supply their own allocator if they want a
      tree under embedded posture.
- [ ] `core/STATUS.md` change log records the CORE-03a landing.

## §13 Files cited

- `rlvgl/core/src/lib.rs:100-160` (v0.2.0 @ b178cbc) — `WidgetNode`.
- `rlvgl/playit/src/tag.rs` (v0.2.0 @ b178cbc) — `find_by_tag`.
- `lvglpp/docs/core-widget/00-widget-tree.md` — base `Widget` chapter.
- `lvglpp/docs/std-mapping.md` § "Owning types", § "Freestanding subset".

## §14 Unblocks

- **PLAYIT-04** — tagged queries traverse `WidgetNode` and use
  `find_by_tag`.
- **WID-05** (`Container`, `List`) — composite widgets that hold
  child nodes.
- **PLAT-** end-to-end demos with multiple widgets — `WidgetNode`
  is the natural app root.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Field set (§5.1),
  dispatch (§5.2), draw (§5.3), find_by_tag (§5.4) all frozen.
  Execution unblocked.
- 2026-04-27 — CORE-03a execution landed in
  `core/include/lvglpp/core/widget_node.hpp`. All §12 acceptance
  bullets satisfied; `lvglpp_core_widget_node` test (7 fixtures)
  green; embedded posture clean.
