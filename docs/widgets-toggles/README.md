<!--
README.md — Initiative README for the WID-03 Checkbox + Switch chapter.
-->

# widgets-toggles — initiative README

This initiative ratifies `lvglpp::widgets::Checkbox` and
`lvglpp::widgets::Switch`. Both share the
"toggle-on-PressRelease-inside-bounds" idiom set by WID-02 Button;
they differ in visual shape and the value they expose
(`is_checked()` vs `is_on()`).

The normative artifact is
[`00-checkbox-and-switch.md`](./00-checkbox-and-switch.md).

## Status

Chapter ratified at draft level (2026-04-27). WID-03 execution is
unblocked by WID-01 Label, WID-02 Button, and CORE-04a (with the
`fill_rounded_rect` shim).

## Cross-language pair

Mirrors `rlvgl/widgets/src/checkbox.rs` and
`rlvgl/widgets/src/switch.rs` (v0.2.0 @ 79f730d). The radius=0
visual path matches byte-for-byte; rounded corners arrive with
CORE-04b.
