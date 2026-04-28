<!--
README.md — Initiative README for the CORE-02 Event surface chapter.
-->

# core-event — initiative README

This initiative ratifies the lvglpp `lvglpp::core::Event` surface and the
load-bearing supporting types (`TouchState`, `TouchPoint`, `Key`,
`MAX_TOUCH_POINTS`). It is the **first** lvglpp chapter under the
spec-before-code discipline (CLAUDE.md § "Spec-Before-Code Planning
Discipline") and sets the per-chapter file precedent for everything
that follows.

This README is **informative**. The normative artifact is the chapter
[`00-event-surface.md`](./00-event-surface.md).

## Chapters

- [00-event-surface.md](./00-event-surface.md) — Event variant set,
  TouchState/TouchPoint/Key, MAX_TOUCH_POINTS, source-of-truth map,
  acceptance checklist.

## Conformance target

A conforming `lvglpp::core::Event` implementation MUST satisfy the
Acceptance checklist in
[`00-event-surface.md`](./00-event-surface.md#12-acceptance-checklist).

A conforming implementation MAY use `std::variant<…>` for the sum
type or hand-roll a tagged union; both are equivalent at this chapter's
contract level.

## Status

Chapter ratified at draft level (2026-04-27). Execution (CORE-02) is
unblocked and pending an implementation PR.

## Cross-language pair

- **rlvgl side**: this chapter mirrors an already-implemented surface
  in `rlvgl/core/src/event.rs` (v0.2.0 @ 79f730d). No rlvgl change
  is required to land lvglpp's CORE-02 implementation.
- **Future change ordering**: any extension to the Event variant set
  is a **Standards Action** (CLAUDE.md § "Frozen enumerations").
  Per CLAUDE.md § "Cross-language change ordering", such amendments
  land in rlvgl `v0.2.0` first, then in this chapter, then in both
  implementations.
