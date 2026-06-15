# 00 — RAII lv_obj core & object-model unification

Chapter status: **ratified 2026-06-15**.
Phase code: **LVGLPP-WRAP-00**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD
NOT**, **MAY**, and **RECOMMENDED** in this chapter are interpreted per
RFC 2119 and RFC 8174.

This chapter is the normative artifact for the lvglpp RAII LVGL-object
core and the ownership/delete-safety/user-data/name conventions every
later widget wrapper depends on. The initiative [`README.md`](./README.md)
is informative. It executes the pivot frozen in
[`../lpar/00-concepts.md`](../lpar/00-concepts.md) §5.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| `lv_obj` lifecycle, parent/child deletion, user-data, name, delete event | `lvgl/` 9.6.0-dev @ `ee436e8` (`src/core/lv_obj.h`, `lv_obj_tree.h`) | Canonical for what the wrapped primitive does. |
| C++ ownership tags, RAII, callback-lifetime safety | CLAUDE.md § "Strict and Explicit Ownership" | Normative for lvglpp. |
| Initiative gating, wrap-not-reimplement, one-object-model rule | `docs/lpar/00-concepts.md` (LPAR-00, ratified) | This chapter is its execution arm. |
| Existing `Runtime`/`ObjectView` seam | `core/include/lvglpp/core/runtime.hpp` (CORE-01) | Repo is canonical; `Object` is the owning counterpart to `ObjectView`. |

## §1 Purpose

Stand up the **additive** RAII core that owns LVGL objects from C++:
`lvglpp::core::Object` (owns an `lv_obj_t*`), `lvglpp::core::Screen` (a
screen root), built on the existing `Runtime` (CORE-01, wraps `lv_init`).
Freeze the four conventions that make a single `lv_obj` tree safe under
C++ RAII — ownership, delete-safety, user-data back-pointer, and
name/tag — plus the `lv_conf.h` baseline. Remove nothing: the hand-rolled
`Widget`/`WidgetNode`/`Renderer` layer keeps working until the migration
sub-phases (§6) retire it.

## §2 Problem statement

- lvglpp links LVGL (`CMakeLists.txt:108`) but calls zero `lv_*`
  (LPAR-00 §2). There is no C++ type that owns an `lv_obj_t*`.
- LVGL **auto-parents on create**: `lv_obj_create(parent)` (and every
  `lv_<widget>_create(parent)`) immediately inserts the new object into
  `parent`'s child list, and `lv_obj_delete(parent)`
  (`lvgl/src/core/lv_obj_tree.h:46`) recursively deletes children. A
  naive RAII wrapper whose destructor always calls `lv_obj_delete`
  **double-frees** when a parent is deleted first, and **dangles** if
  LVGL deletes the underlying object behind the wrapper's back.
- The wrapper therefore MUST track *who owns the `lv_obj`* (the C++
  handle vs the LVGL parent tree) and MUST survive LVGL-driven deletion.
  LVGL provides the hooks: `LV_EVENT_DELETE` + `lv_obj_add_event_cb`, and
  a per-object `user_data` slot (`lv_obj_set_user_data`).

## §3 Canonical glossary

- **`Object`** — Owned by this chapter; does not exist in repo yet. RAII
  owner of an `lv_obj_t*`. `owns` its handle; destructor deletes it via
  `lv_obj_delete` **iff** still owning and not already deleted by LVGL.
  The C++ counterpart to `ObjectView`.
- **`Screen`** — Owned by this chapter; does not exist yet. An `Object`
  that is a screen root (created parentless; activated via
  `lv_screen_load`). Screens are not auto-parented, so a `Screen` always
  `owns` its `lv_obj` until loaded/deleted.
- **`ObjectView`** — As defined in
  `core/include/lvglpp/core/runtime.hpp` (CORE-01); used without
  modification. The non-owning (`external`/`observes`) borrow over an
  `lv_obj_t*`. `Object::view()` returns one.
- **Owned vs attached** — Owned by this chapter. An `Object` is *owned*
  (C++ destructor will delete the `lv_obj`) until it is *attached* to a
  parent that assumes ownership, or LVGL deletes it. `attach_*`/`detach_*`
  transfer the delete responsibility explicitly (CLAUDE.md naming).
- **Delete-safety callback** — Owned by this chapter. An `LV_EVENT_DELETE`
  handler the wrapper registers on its `lv_obj`; when LVGL deletes the
  object (e.g. parent deletion, `lv_obj_clean`), the handler nulls the
  wrapper's `obj` pointer so the destructor neither double-frees nor
  dangles. This is the mechanically-safe callback lifetime CLAUDE.md
  § "Strict and Explicit Ownership" rule 9 requires.
- **User-data back-pointer convention** — Owned by this chapter. The
  `lv_obj` `user_data` slot (`lv_obj_set_user_data`) holds the back-pointer
  to the owning C++ `Object` (for event dispatch, delete-safety, and the
  future playit lookup). It is owned by the wrapper layer; widget-specific
  state lives in `Object` subclass members, **never** in `user_data`.
- **Name / tag** — Owned by this chapter. The `playit` tag maps to the
  LVGL object name (`lv_obj_set_name`/`lv_obj_get_name`, available in
  9.6), not to `user_data`. The actual `playit` tag bridge is
  `LVGLPP-WRAP-0N`; this chapter only reserves the channel.

## §4 Source-of-truth map

| Concept | Owner | Consumers |
| --- | --- | --- |
| `Object`/`Screen` API + ownership rule | this chapter | every LPAR/FONT widget wrapper |
| Delete-safety + user-data + name conventions | this chapter §5 | widget wrappers, playit bridge, event mapping (LPAR-04) |
| `lv_obj` lifecycle primitives | `lvgl/` 9.6 | the wrapper |
| `lv_conf.h` baseline | this chapter §5.6 + module `OPTIONS.md` | whole build |
| Migration order (widgets/playit/platform) | this chapter §6 | WRAP-01..0N |

## §5 Frozen decisions

### §5.1 Ownership model — **Specification Required**

1. `Object` holds `lv_obj_t* obj_` (raw, nullable) and a single ownership
   role documented per CLAUDE.md tags. A live `Object` either `owns`
   `obj_` (will `lv_obj_delete` it) or has transferred ownership
   (`attach_*`) — never ambiguous.
2. Construction that yields ownership goes through `make_*` factories
   (`make_screen()`, and per-widget `make_<widget>(parent, …)` in later
   phases). The factory creates the `lv_obj`, installs the delete-safety
   callback and the user-data back-pointer, then returns the owning
   `Object`/subclass by value (move).
3. `Object` is **move-only** (no copy). Move transfers `obj_` and nulls
   the source. The moved-from `Object` destructor is a no-op.
4. Destructor: if `obj_ != nullptr` **and** owning, call
   `lv_obj_delete(obj_)`; else no-op. The delete-safety callback (§5.2)
   guarantees `obj_` is null whenever LVGL already deleted the object.

### §5.2 Delete-safety — **Specification Required**

Every owning `Object` MUST register an `LV_EVENT_DELETE` callback on its
`obj_` via `lv_obj_add_event_cb`. The callback recovers the `Object`
back-pointer from `user_data` and sets its `obj_ = nullptr`. This makes
parent-driven deletion (`lv_obj_delete(parent)`, `lv_obj_clean(parent)`)
and explicit `lv_obj_delete` safe: the C++ destructor sees `nullptr` and
does not double-free. The callback MUST NOT delete or dereference C++
state beyond nulling the pointer (LVGL may invoke it mid-teardown).

### §5.3 User-data ownership — **Specification Required**

`lv_obj` `user_data` is **owned by the wrapper layer** and holds the
`Object*` back-pointer. Widget authors MUST NOT write `user_data`;
per-widget state is a member of the `Object` subclass. A `// SAFETY:`
note at the `lv_obj_set_user_data` call site documents the back-pointer's
lifetime (valid until the delete-safety callback fires).

### §5.4 Name / tag channel — **Expert Review**

The LVGL object name (`lv_obj_set_name`) is reserved for the `playit`
tag. This chapter does not implement the bridge; it forbids any other
use of the name field so `LVGLPP-WRAP-0N` can rely on it.

### §5.5 Embedded posture — **Specification Required**

`Object`/`Screen` MUST compile under `LVGLPP_EMBEDDED_POSTURE=ON`
(`-fno-exceptions -fno-rtti`). Factories return
`lvglpp::expected<Object, ObjectError>` via `try_make_*`; the throwing
`make_*` convenience calls `std::abort()` on the embedded posture when
the underlying `lv_*_create` returns `nullptr` (OOM), mirroring
`Runtime` (CORE-01).

### §5.6 `lv_conf.h` baseline — **Specification Required**

`include/lvglpp/lv_conf.h` MUST enable the core object/draw/font subset
the wrapper assumes (`LV_USE_OBJ_*` core, a default font, the tick + mem
managers) and define `LV_USE_LOG`/asserts consistently with the host and
embedded builds. Per-feature `LV_USE_*` are turned on by the phase that
wraps that feature (LPAR-01 §5.3), not here.

### §5.7 Additive coexistence — **Specification Required**

WRAP-00 adds `Object`/`Screen` and removes nothing. The hand-rolled
`Widget`/`WidgetNode`/`Renderer`/`draw_*` and `WID-01..06` keep compiling
and passing tests. No code outside the new `Object`/`Screen` translation
units may change in the WRAP-00 PR.

## §6 Migration sub-phase plan (LVGLPP-WRAP-01..0N)

| Sub-phase | Scope | Gate |
| --- | --- | --- |
| WRAP-01 | Port `WID-01` (Label) onto `Object`; prove the pattern end-to-end (event + draw via LVGL). | `ctest` + Label playit fixtures green. |
| WRAP-02..06 | Port Button/Checkbox/Switch/Slider/Container/List/Image. | Per-widget fixtures green. |
| WRAP-0N (playit) | `Dispatcher` walks the `lv_obj` tree; tag → `lv_obj` name; `QB`/`QE`/`QC`/`T@<tag>` byte-identical to rlvgl. | Shared playit fixtures byte-identical. |
| WRAP-0N (platform) | SDL → `lv_display` flush + `lv_indev`; fbdev likewise; disco `lv_display`+DMA2D (L3, verification-blocked). | L0/L1/L2 green; L3 tracked. |
| WRAP-0N (retire) | Delete hand-rolled `Widget`/`WidgetNode`/`Renderer`/`draw_*`/`draw_helpers` once unreferenced. | Tree builds with the old layer gone. |

## §10 Reconciliation vs. adjacent primitives

- **`Runtime` (CORE-01)** — unchanged; `Object`/`Screen` require a live
  `Runtime` (asserted, mirroring single-instance). `lv_init` is not
  re-entered.
- **`ObjectView` (CORE-01)** — becomes the non-owning view returned by
  `Object::view()`; its provisional CORE-03 ownership note is resolved
  here (it is `observes`, `Object` is `owns`).
- **`Widget`/`WidgetNode` (CORE-03/03a)** — coexist during migration;
  retired by WRAP-0N. No new code should build on them.
- **`Event` (CORE-02)** — stays the canonical input value type; LPAR-04
  maps it to/from `lv_event_t` at the driver seam. WRAP-00 only wires the
  `LV_EVENT_DELETE` safety hook, not general event dispatch.
- **`playit` Dispatcher** — unchanged in WRAP-00; the name/tag channel is
  reserved (§5.4) for its WRAP-0N bridge.

## §11 Non-goals

- No widget wrappers (those are LPAR-11.. / FONT, after migration).
- No general event/focus dispatch (LPAR-04); only the delete-safety hook.
- No platform display migration (WRAP-0N).
- No removal of the hand-rolled layer (WRAP-0N); WRAP-00 is additive.
- No `user_data` use for widget state (§5.3).

## §12 Acceptance checklist

A conforming LVGLPP-WRAP-00 execution PR MUST:

- [ ] Add `lvglpp::core::Object` and `lvglpp::core::Screen` with the
      ownership model (§5.1), move-only semantics, and CLAUDE.md
      ownership-tag comments on every raw pointer.
- [ ] Install the `LV_EVENT_DELETE` delete-safety callback (§5.2) and a
      test that deletes a parent and asserts the child `Object`'s
      destructor does not double-free (e.g. ASan-clean, or a recording
      fake).
- [ ] Store the `Object*` back-pointer in `user_data` with a `// SAFETY:`
      note (§5.3); leave the name field unused (§5.4).
- [ ] Provide `try_make_*` returning `lvglpp::expected<…>` and a throwing
      `make_*` that `abort()`s under embedded posture (§5.5).
- [ ] Land/confirm the `include/lvglpp/lv_conf.h` baseline (§5.6) so a
      host build links `lv_obj_create`/`lv_obj_delete`.
- [ ] Change nothing outside the new translation units (§5.7); existing
      `ctest` targets stay green under default and
      `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] Each new `.hpp`/`.cpp` carries the PARITY/LVGL/DELTA cite block.
- [ ] `core/STATUS.md` change log records LVGLPP-WRAP-00.

## §13 Files cited

- `lvgl/src/core/lv_obj.h`, `lv_obj_tree.h` (9.6.0-dev @ `ee436e8`) — `lv_obj_create`/`delete`/`user_data`/name
- `lvgl/src/core/lv_obj_event.h` — `LV_EVENT_DELETE`, `lv_obj_add_event_cb`
- `core/include/lvglpp/core/runtime.hpp` — `Runtime`, `ObjectView` (CORE-01)
- `include/lvglpp/lv_conf.h`, `CMakeLists.txt:105`/`:108`
- `docs/lpar/00-concepts.md` §5, `docs/lpar/01-baseline.md` §5.1
- CLAUDE.md § "Strict and Explicit Ownership", § "Cite-block convention"

## §14 Unblocks

- **LVGLPP-WRAP-01..06** — widget migration builds on `Object`.
- **LPAR-02..16 / FONT-00..05** — every widget wrapper owns an `Object`.
- **LVGLPP-WRAP-0N** — playit bridge + platform display, on the reserved
  name channel and the `lv_display`/`lv_indev` seam.

## §15 Change log

- **2026-06-15** — LVGLPP-WRAP-00 drafted. Defines the RAII `lv_obj` core
  (`Object`/`Screen`), the ownership + delete-safety (`LV_EVENT_DELETE`) +
  user-data back-pointer + name/tag conventions, the embedded-posture
  factory pattern, the `lv_conf.h` baseline, and the additive-coexistence
  rule. Migration sub-phase plan (§6) sequences widget/playit/platform
  retirement of the hand-rolled layer. **Not ratified** — awaiting owner
  go-ahead; no WRAP-00 execution code lands until ratified.
- **2026-06-15** — LVGLPP-WRAP-00 **ratified** by owner instruction
  ("Wrap ratified"). Execution unblocked: the additive `Object`/`Screen`
  core may land.
