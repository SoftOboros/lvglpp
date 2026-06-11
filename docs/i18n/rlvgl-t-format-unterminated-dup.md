# Upstream finding — rlvgl `t_format` duplicates the tail on an unterminated `{`

Status: **open** (found 2026-06-11 during I18N-01, the lvglpp port of
`rlvgl-i18n`). Affects `rlvgl/i18n/src/lib.rs` at v0.2.0 @ 79f730d.

## Symptom

`t_format` (and therefore `t!("key", …)`) emits the template tail
twice when the template contains a `{` with no matching `}`:

```rust
t_format(key_for("abc{def"), &[]) // → "abc{defabc{def", expected "abc{def"
```

## Mechanism

`lib.rs:135–159`:

```rust
while let Some(start) = rest.find('{') {
    out.push_str(&rest[..start]);          // pushes "abc"
    let after = &rest[start + 1..];
    if let Some(end) = after.find('}') {
        …
        rest = &after[end + 1..];
    } else {
        out.push_str(&rest[start..]);      // pushes "{def"
        break;                             // rest NOT consumed…
    }
}
out.push_str(rest);                        // …pushed again: "abc{def"
```

The unterminated branch pushes the remainder but leaves `rest`
unchanged, so the post-loop `out.push_str(rest)` appends the entire
remaining slice a second time. None of the crate's tests cover an
unterminated placeholder, so the bug is latent.

## Suggested upstream fix

In the unterminated branch, either `return out` after the push, or
set `rest = ""` before `break`. One-line test:

```rust
#[test]
fn unterminated_placeholder() {
    set_locale(Locale::En);
    // with a fixture key whose template is "abc{def"
    assert_eq!(t_format(Key::…, &[]), "abc{def");
}
```

## lvglpp posture

`lvglpp::i18n::format` implements the documented intent — the
remainder is copied exactly once (`i18n/src/format.cpp`, covered by
`i18n/tests/format_test.cpp`). Recorded in
`docs/i18n/00-rltn-core.md` §5.4/§15. Since the divergence is
unreachable from well-formed locale files (no shipped template is
unterminated), blob compatibility and test-vector parity are
unaffected.

## Cross-reference

Third upstream finding from the lvglpp port, after
`docs/platform-disco/rlvgl-ltdc-layer-off-by-one.md` and
`docs/disco-demo/rlvgl-sim-headless-frame-gap.md`.
