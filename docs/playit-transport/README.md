<!--
README.md — Initiative README for the PLAYIT-07 Transport + Executor.
-->

# playit-transport — initiative README

This initiative ratifies the **wire transport** + **executor** layer
that turns the in-memory Dispatcher (PLAYIT-04) into something
external probes can talk to. Without this, lvglpp targets cannot be
driven by a real rlvgl playit fixture: there is a Dispatcher, but
nothing wired to bytes.

Chapters:

- [00-transport-and-executor.md](./00-transport-and-executor.md) —
  `Transport` abstract base, `StdioTransport` host implementation,
  `Executor` line-accumulator + dispatch-loop.

## Status

Chapter ratified at draft level (2026-04-27). Execution unblocked
by PLAYIT-04b (Response formatter).

## Cross-language pair

Mirrors `rlvgl/playit/src/{transport.rs, executor.rs}` (v0.2.0 @
b178cbc). Cross-language closure: a single `cat fixtures.txt | …`
pipe drives lvglpp targets identically to rlvgl targets.
