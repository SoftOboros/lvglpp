# 01 — Response wire formatter

Chapter status: **draft, ratified 2026-04-27**.
Phase code: **PLAYIT-04b** (sub-phase under playit-tagged).

The key words **MUST**, **SHOULD**, **MAY** are interpreted per RFC
2119 / 8174.

## §0 Authority

| Vocabulary owner | Source | Notes |
| --- | --- | --- |
| Wire format for every Response variant | `rlvgl/playit/src/protocol.rs:466` (v0.2.0 @ 79f730d) | Canonical. Cross-language byte-for-byte parity required — playit fixtures parse responses on the rlvgl host side and expect identical text. |
| Response value shape | PLAYIT-04 §5.1 | Inputs to the formatter. |

## §1 Purpose

Land the inverse of `parse_command` (PLAYIT-01): turn a `Response`
value into the ASCII line a rlvgl playit host expects on the wire.
Without this, lvglpp targets cannot answer queries — the
cross-language test loop only goes one direction.

## §3 Canonical glossary

- **`format_response`** — Owned by this chapter. Free function in
  `lvglpp::playit`:
  ```
  std::size_t format_response(const Response& resp,
                              std::span<char> buf) noexcept;
  ```
  Writes the wire-format bytes (including trailing `\r\n`) into
  `buf` and returns the number of bytes written. If `buf` is too
  small, output is **silently truncated** to mirror
  `rlvgl/playit/src/protocol.rs:466` exactly.

## §4 Source-of-truth map

| Concept | Owner | Mirror sites |
| --- | --- | --- |
| Per-variant ASCII format strings | `rlvgl/playit/src/protocol.rs:466` (canonical) | `lvglpp::playit::format_response`. |
| Truncation behavior on short buffer | `rlvgl/playit/src/protocol.rs` BufWriter | lvglpp matches: silently truncate, return bytes-written ≤ buf.size(). |
| Trailing `\r\n` | this chapter §5.2 — **Standards Action** | rlvgl + lvglpp PR pair if changed. |

## §5 Frozen decisions

### §5.1 Per-variant wire format — **Standards Action**

| `Response` variant | Wire bytes (without trailing `\r\n`) |
| --- | --- |
| `Ok` | `OK` |
| `Error{reason}` | `ERR: <reason>` |
| `Bounds{x,y,w,h}` | `BOUNDS:<x>,<y>,<w>,<h>` |
| `Exists{true}` | `EXISTS:1` |
| `Exists{false}` | `EXISTS:0` |
| `ChildCount{n}` | `CHILDREN:<n>` (n is rendered as a decimal `int32_t`) |
| `Status{tick,present}` | `STAT:<tick>,<present>` |
| `DumpEnd` | `END` |

Every line is followed by **CRLF (`\r\n`)**. Decimal integers MAY
be negative (e.g. `BOUNDS:-5,-7,30,40`); the formatter MUST emit
a leading `-` for negatives without overflowing on `INT32_MIN`.

### §5.2 Trailing CRLF — **Standards Action**

`\r\n` (2 bytes). Required for rlvgl's host parser. lvglpp MUST NOT
emit `\n` only or omit the trailing newline.

### §5.3 Truncation — **Specification Required**

A buffer too small for the formatted output silently truncates at
the buffer end. The function MUST NOT write past `buf.size()`. The
return value is `min(buf.size(), bytes_that_would_have_been_written)`.

Callers that need to detect truncation can pass a buffer one byte
larger than expected and check whether the trailing `\n` arrived.

### §5.4 No allocations, `noexcept`

The formatter MUST run under embedded posture (`noexcept`,
allocation-free, no `<iostream>`). Internal integer-to-decimal
conversion uses a small fixed-size scratch.

## §10 Reconciliation vs. adjacent primitives

- **`parse_command` (PLAYIT-01).** Inverse direction; same allocation-free
  / `noexcept` discipline.
- **`format_event_spec`** (rlvgl `protocol.rs:359`) for the recorder
  dump — that's PLAYIT-06's concern; PLAYIT-04b only handles
  Response-level lines.

## §11 Non-goals

- Variable-length encoding (e.g. zero-padded numbers). Decimal,
  shortest-form, leading-`-` for negatives.
- Localized number formatting.
- Round-trip with `parse_command` — Response is sent **back** to
  the rlvgl host parser (rust-side), not consumed by lvglpp's
  parser.

## §12 Acceptance checklist

- [ ] Free function `lvglpp::playit::format_response` defined per §3.
- [ ] Each of the 7 Response variants produces the §5.1 wire bytes,
      followed by §5.2 CRLF.
- [ ] `INT32_MIN` is rendered as `-2147483648` without UB
      (sign-flip overflow trap avoided).
- [ ] Short-buffer behavior matches §5.3 — verified by a fixture
      that supplies a 4-byte buffer and asserts truncation.
- [ ] `noexcept`, no allocation, no `<iostream>`. Compiles clean
      under `LVGLPP_EMBEDDED_POSTURE=ON`.
- [ ] Test target `lvglpp_playit_format_response` ≥ 8 fixtures.

## §13 Files cited

- `rlvgl/playit/src/protocol.rs:466` — `format_response`.
- `lvglpp/docs/playit-tagged/00-tagged-queries.md` — Response shape.

## §14 Unblocks

- **PLAYIT-07** (Transport + Executor) — the executor's write path
  needs `format_response`.
- Any wire-level integration with a rlvgl playit host.

## §15 Change log

- 2026-04-27 — Chapter ratified at draft level. Per-variant wire
  formats (§5.1), CRLF (§5.2), truncation (§5.3) frozen.
- 2026-04-27 — PLAYIT-04b execution landed in
  `playit/{include/lvglpp/playit/format.hpp, src/format.cpp}`.
  Test target `lvglpp_playit_format` (11 fixtures, including
  INT32_MIN edge case + zero-buffer + truncation) green; embedded
  posture clean.
