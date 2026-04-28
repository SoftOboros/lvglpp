<!--
README.md — Publish-facing overview for the lvglpp::playit library.
-->

# lvglpp::playit

Library: `lvglpp::playit` (CMake target: `lvglpp_playit`, alias
`lvglpp::playit`).

`lvglpp::playit` is the C++ port of [`rlvgl-playit`](../rlvgl/playit/),
the serial test driver used to exercise lvglpp / rlvgl widget trees end
to end. The wire protocol is the **canonical source of truth** — see
[`rlvgl/playit/README.md`](../rlvgl/playit/README.md) and the top-level
[`CLAUDE.md`](../CLAUDE.md) § "Runtime Command Protocol".

## Why first-class

The user-facing instruction was to treat playit as a first-class driver.
Reasons:

- The same probe driver and the same wire format already work against
  rlvgl. By mirroring the parser in C++ rather than porting it, every
  existing rlvgl playit fixture, every recorded session, and every
  CI-hardware test transfers to lvglpp targets unchanged.
- Cross-language regressions are caught early: if lvglpp's parser
  diverges from rlvgl's, playit fixtures will fail in obvious ways.
- The "rlvgl ↔ lvglpp" change ordering rule (see CLAUDE.md
  § "Cross-language change ordering") binds here: any new playit
  command lands in rlvgl's protocol doc first, then in both parsers.

## Status

INTERFACE target only — parser and dispatcher land under their own
phases. See [`STATUS.md`](./STATUS.md).

## License

MIT.
