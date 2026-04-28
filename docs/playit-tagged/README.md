<!--
README.md — Initiative README for the PLAYIT-04 tagged queries chapter.
-->

# playit-tagged — initiative README

This initiative ratifies tagged-widget addressing in `lvglpp::playit`:
`Response` value type, `Dispatcher` that routes parsed `Command`
values into a `lvglpp::core::WidgetNode` tree, and the four
tagged-protocol commands (`T@<tag>:<x>,<y>`, `QB:<tag>`,
`QE:<tag>`, `QC:<tag>`).

This README is **informative**. The normative artifact is the chapter
[`00-tagged-queries.md`](./00-tagged-queries.md).

## Status

Chapter ratified at draft level (2026-04-27). PLAYIT-04 execution
unblocked by ratification of CORE-03a (WidgetNode) and WID-02
(Button).

## Cross-language pair

Mirrors `rlvgl/playit/src/{response.rs, executor.rs}` (v0.2.0 @
79f730d) for the Response shape and the dispatcher's tagged-command
routing. Closes the cross-language test loop: rlvgl-side playit
fixtures issuing `T@MyButton:50,50` now drive a real lvglpp
WidgetNode tree.
