# 05 — LvFontView registry & cascade→object bridge

Chapter status: **ratified 2026-06-15**.
Phase code: **FONT-05**.

The key words **MUST**, **MUST NOT**, **SHALL**, **SHOULD**, **SHOULD NOT**,
**MAY**, and **RECOMMENDED** in this chapter are interpreted per RFC 2119 and
RFC 8174.

This chapter is the normative artifact for the lvglpp `FontId` registry. Parent
spec: [`00-concepts.md`](./00-concepts.md) (the `LvFontView` selection model is
canonical and unchanged here). Style cascade owner:
[`../core-style/01-style-cascade-theme.md`](../core-style/01-style-cascade-theme.md).

## §0 Authority

| Vocabulary owner | Source |
| --- | --- |
| `FontId(u16)` identifier + `FontId::DEFAULT` **intent** | rlvgl `v0.2.5` `core/src/font.rs:15` (mirrored here) |
| `FontId → font` registry + cascade-bridge **semantics** | rlvgl `v0.2.5` `docs/concepts/FONT-05-FONT-REGISTRY.md` (@ `f999f75`) |
| The cascade→object font path (the **bridge**) | `lvgl` `lv_obj_set_style_text_font` — **native**; lvglpp wraps |
| LvFontView selection model (`LvFontView`, `set_text_font`) | FONT-00 §5 — used without modification |

## §1 Purpose

Give callers a stable **`FontId`** name for a font and a small registry that
maps `FontId → const lv_font_t*`, so font identity can be threaded by value
(through creator-emitted asset tables, config, or a theme) and resolved to a
real `lv_font_t` at the point a font is applied to an object.

## §2 Problem statement

rlvgl's FONT-05 problem — "the cascade computes `TextStyle.font_id` but no
widget reads it; the two font-identity channels are disjoint" — **does not
exist in LVGL form**. In LVGL the cascade stores the resolved `lv_font_t*`
directly (`lv_obj_set_style_text_font`), and `lv_obj_get_style_text_font`
reads it back (FONT-00 `resolved_text_font(ObjectView, StylePart::Main)`). The bridge is native; there is
no inert `font_id` to wire up.

What lvglpp *lacks* is the rlvgl-side **registry** that the creator-asset path
expects: a `FontId → font` map so a generated asset table (which references
fonts by small integer id, not by C symbol) can resolve ids to the actual
`lv_font_t` symbols at startup. FONT-05 provides exactly that registry, plus a
convenience that resolves a `FontId` and applies it through the FONT-00
selection setter.

## §3 Canonical glossary

- **`FontId`** — As defined in `rlvgl/core/src/font.rs:15` (`FontId(pub u16)`,
  `FontId::DEFAULT`); **mirrored here** as a strong `uint16_t` wrapper.
  `FontId::default_id()` (value `0`) names the LVGL default font. **Standards
  Action** (cross-language identifier).
- **`FontRegistry`** — Owned by this chapter; a **fixed-capacity, heap-free**
  map of `FontId → LvFontView` (the `LvFontView` handle observes a `const lv_font_t*`).
  `register_font(FontId, LvFontView)`, `lookup(FontId) → LvFontView` (empty `LvFontView` if
  unregistered, except `FontId::default_id()` always resolves to
  `LvFontView::default_font()`), and `apply(ObjectView&, FontId, Selector)` =
  resolve + `set_local_text_font(ObjectView, ...)`. The registry owns no fonts; every
  entry observes external/static storage that MUST outlive the registry's use.
- **The bridge** — `lv_obj_set_style_text_font`, native. FONT-05 adds no new
  cascade traversal; it feeds the registry's resolved handle into the FONT-00
  selection setter.

## §4 Source-of-truth map

| Concept | Owner |
| --- | --- |
| `FontId` value + `DEFAULT` | rlvgl `core/src/font.rs` — mirrored (Standards Action) |
| `FontId → font` map storage / capacity | this chapter (fixed-capacity, heap-free) |
| Cascade→object application | `lvgl` (`lv_obj_set_style_text_font`) via FONT-00 |
| Resolved-font read-back | `lvgl` (`lv_obj_get_style_text_font`) via FONT-00 `resolved_text_font(ObjectView, StylePart::Main)` |

## §5 Frozen decisions

1. `FontId` mirrors rlvgl `FontId(u16)`; `FontId::default_id()` == `0` ==
   `FontId::DEFAULT`. **Standards Action**: any new well-known id is amended in
   rlvgl `core/src/font.rs` first, then mirrored here.
2. `FontRegistry` is **heap-free**: a fixed-capacity array of slots (capacity a
   compile-time constant, default 16). Registering past capacity fails
   visibly (returns `false` / an error), never silently drops. This keeps the
   registry usable under `LVGLPP_EMBEDDED_POSTURE` with no allocator.
3. The registry **observes** fonts; it transfers no ownership. Lifetime of
   every registered `lv_font_t` is the caller's responsibility and MUST cover
   every object the font is applied to (FONT-00 §5.2 outlives-objects rule).
4. `lookup(FontId::default_id())` always returns `LvFontView::default_font()`, even
   with no explicit registration, so a default-id asset always resolves.
5. The cascade→object bridge is **native LVGL** and is NOT re-implemented;
   FONT-05 discharges rlvgl's FONT-05 coupling by *consuming* the native
   bridge, recorded as a DELTA.

## §10 Reconciliation vs. adjacent primitives

- **rlvgl `resolve_tree_with_text` / `TextStyle.font_id`** — no lvglpp analog
  is built: LVGL's cascade already resolves and stores the `lv_font_t*` per
  object, and `resolved_text_font(ObjectView, StylePart::Main)` reads it. The "walk the tree and feed each
  node's resolved font into its slot" pass is LVGL-internal. DELTA, not fork.
- **FONT-00 `LvFontView` selection setters** — the registry's `apply()` is a thin
  convenience over `set_local_text_font(ObjectView, ...)`; it adds no new write path.
- **rlvgl defaulted-`Widget`-method reconciliation (FONT-05 §10)** — N/A in
  lvglpp: there is no `Widget` trait to extend; the font sink is the cascade.

## §11 Non-goals

- A theme that injects `FontId` defaults — themes set `lv_font_t*` directly
  via `LvTheme` / LVGL theme APIs (LPAR-07); a `FontId`-keyed theme layer is
  out of scope.
- Dynamic (heap) registries or name→id parsing of creator manifests — the
  registry is `FontId`-keyed and fixed-capacity; manifest parsing belongs to
  the (deferred) creator-asset loader.

## §12 Acceptance checklist

- [ ] `FontId` strong `uint16_t` type mirroring rlvgl, with `default_id()`.
- [ ] `FontRegistry` fixed-capacity, heap-free: `register_font`, `lookup`
      (default-id always resolves), `apply(ObjectView&, FontId, Selector)`.
- [ ] Over-capacity registration fails visibly; registry observes (no
      ownership transfer), lifetime rule documented.
- [ ] Builds + tests under both postures; `core/STATUS.md` records FONT-05.

## §13 Files cited

- `rlvgl/docs/concepts/FONT-05-FONT-REGISTRY.md` (v0.2.5 @ `f999f75`)
- `rlvgl/core/src/font.rs:15` (`FontId`)
- `lvgl/src/core/lv_obj_style_gen.h` (`lv_obj_set_style_text_font`)
- `docs/font/00-concepts.md` (FONT-00 selection model)

## §14 Unblocks

- The (deferred) rlvgl-creator → lvglpp font-asset loader, which references
  fonts by `FontId` and resolves them through this registry.

## §15 Change log

- **2026-06-15** — FONT-05 drafted: `FontId` mirror + heap-free `FontRegistry`
  + `apply()` convenience over the FONT-00 selection setter. Reframe vs. rlvgl
  FONT-05 (the cascade→object bridge is native LVGL) recorded in §10/§5.5.
- **2026-06-15** — ratified by owner ("ratified - proceed"); execution
  unblocked (depends on FONT-00).
