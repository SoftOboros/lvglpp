# 00 — i18n core: RLTN blob + compile-time keys

Chapter status: **draft, ratified 2026-06-11**.
Phase code: **I18N-01** (see §15 for the numbering decision).

## §0 Authority

- The RLTN v1 binary format, codegen rules, and runtime semantics are
  owned by `rlvgl/i18n/build.rs` and `rlvgl/i18n/src/lib.rs` (v0.2.0
  @ 79f730d). Parity by design; lvglpp adapts only C++ idiom.
- Placeholder syntax (`{name}`) is owned by `t_format`
  (`rlvgl/i18n/src/lib.rs:135–159`).
- The lv_translation bridge built on this chapter is owned by
  `01-lv-translation-bridge.md` (I18N-02).

## §1 Purpose

Equivalent-and-compatible localization: the same `locales/*.json`
inputs produce byte-identical RLTN v1 blobs on the Rust and C++
sides, so one `.bin` asset serves both firmwares, and the same
key set is compile-time-checked in both languages.

## §3 Canonical glossary

- **RLTN v1** — As defined in `rlvgl/i18n/build.rs:7–21`; used
  without modification. See §5.1.
- **`Locale`** — As defined in generated `translations.rs`
  (`build.rs:157–178`); mirrored as a generated
  `enum class Locale : std::uint8_t`.
- **`Key`** — As defined in generated `translations.rs`
  (`build.rs:180–199`); mirrored as a generated
  `enum class Key : std::uint16_t`.
- **`t_static`** — As defined in `rlvgl/i18n/src/lib.rs:129`;
  mirrored as `lvglpp::i18n::t_static_index` + generated typed
  wrapper.
- **`t_format`** — As defined in `rlvgl/i18n/src/lib.rs:135`;
  mirrored as `lvglpp::i18n::format` (template fetched by caller).
- **`load_translations`** — As defined in
  `rlvgl/i18n/src/lib.rs:94`; adapted: lvglpp validates the blob
  and returns `lvglpp::expected` instead of being `unsafe`-trusting
  (§5.4).
- **`t!` compile-time key check** — As defined in the generated
  macro (`build.rs:209–245`, unknown key → `compile_error!`);
  mirrored as a generated `consteval` lookup (§5.3).

## §5 Frozen decisions

### §5.1 RLTN v1 blob — **Standards Action** (cross-language contract)

```text
[0..4]   magic   b"RLTN"
[4]      version 1
[5]      num_locales (u8)
[6..8]   num_keys    (u16 LE)
[8..]    entries     (num_locales × num_keys) × 6 bytes,
         locale-major: offset u32 LE (into string data), len u16 LE
[..]     string data (UTF-8, packed, no NUL terminators)
```

Lookup: `entries[locale * num_keys + key]`. Any change to this
layout amends rlvgl first (`build.rs`), then mirrors here citing
the rlvgl SHA.

### §5.2 Codegen rules — **Standards Action** (mirrors build.rs)

- Locale files sorted by filename; file stem = locale name; first
  locale = `DEFAULT` and fallback source.
- Key set = the default locale's keys, sorted (BTreeMap order).
- A key missing from a non-default locale is filled from the
  default at build time, with a warning. Keys present only in a
  non-default locale are not emitted (mirrors build.rs:99–120;
  lvglpp's tool additionally warns — tool-output delta only, blob
  bytes unaffected).
- Identifier mapping: `.`/`_`/`-` → PascalCase boundaries
  (`build.rs:25–39`); collision after mapping = build error.
- Non-string JSON values = build error.
- **Byte-identity rule**: given identical `locales/*.json`, the
  lvglpp tool MUST produce a blob byte-identical to rlvgl's
  `build.rs` output (golden test, §12).

### §5.3 Generated C++ surface — **Specification Required**

`i18n/tools/gen_i18n.py --locales <dir> --out <dir> --name <name>
--emit keys[,rltn][,lv]` emits, at consumer configure time:

- `<name>.hpp` — `enum class Locale : std::uint8_t` (+
  `LOCALE_COUNT`, `LOCALE_DEFAULT`), `enum class Key :
  std::uint16_t` (+ `KEY_COUNT`), `constexpr locale_name(Locale)`,
  `constexpr tag_name(Key)` (the original dotted key string), and
  `consteval Key key(std::string_view)` whose not-found branch
  calls an undeclared-defined non-constexpr function — unknown key
  is a compile error (`t!` parity under `-fno-exceptions`).
- with `rltn`: `<name>_rltn.inc` (blob byte array) plus inline
  `init_rltn()` (registers the built-in blob), `t(Key)` →
  `std::string_view`, `t(Key, params…)` → `std::string`.
- with `lv`: see I18N-02 §5.1.

DELTA vs rlvgl: rlvgl textually includes generated code inside the
i18n crate (whose `locales/` holds the demo's strings); lvglpp
splits a key-agnostic runtime library from per-consumer generated
headers, keeping application strings out of the framework module.
Blob format and semantics unchanged.

### §5.4 Runtime surface — **Standards Action** (mirrors lib.rs)

`lvglpp::i18n` (module `i18n/`, dependency-free, embedded-posture
clean):

- `set_locale_index(u8)` / `locale_index()` — relaxed atomics
  (lib.rs:44, :102–109).
- `t_static_index(u16) -> std::string_view` — zero-alloc lookup in
  the active blob (lib.rs:114–132).
- `format(std::string_view tmpl, std::span<const Param>) ->
  std::string` — `{name}` substitution per lib.rs:135–159: unknown
  placeholder preserved as `{name}`; unterminated `{` copies the
  remainder verbatim **once** (rlvgl's implementation duplicates
  the tail in this branch — upstream bug, reported in
  `docs/i18n/rlvgl-t-format-unterminated-dup.md`; lvglpp
  implements the documented intent). `Param` self-contains numeric
  renderings (`std::to_chars`, no heap); parameter values are
  strings/integrals — floats are pre-formatted by the caller
  (named DELTA vs `&dyn Display`).
- `register_builtin(span)` — generated `init_rltn()` calls this
  once (the C++ stand-in for `include_bytes!`).
- `load_translations(const std::uint8_t* data, std::size_t len)` —
  runtime blob override (SD/flash), `'static`-lifetime data;
  **adapted**: validates magic/version/counts/last-entry bounds
  against the registered builtin and returns
  `lvglpp::expected<void, BlobError>`; `reset_translations()`
  reverts to builtin. rlvgl's equivalent is `unsafe` and trusts
  the caller — the validating loader is a named lvglpp DELTA.
- `builtin_blob()` — diagnostics / write-to-media (lib.rs:164).

## §10 Reconciliation

- **rlvgl-i18n crate** — one rlvgl crate ↔ one lvglpp module
  (`i18n/`), same as core/widgets/platform/playit.
- **CORE-07 plugins** — i18n is NOT a CORE-07 plugin: it decodes
  no assets and rlvgl ships it as a sibling crate, not behind
  `plugins/mod.rs`. Module-level gating only.
- **lv_translation (upstream LVGL)** — different model (runtime
  string-tag lookup, open-world packs, widget event). Bridged, not
  adopted, by I18N-02; comparison recorded there.
- **DEMO consumers** — gallery/disco locales live with the
  consuming example, not in this module (§5.3 DELTA).

## §11 Non-goals

- Plural rules, locale negotiation, RTL — rlvgl has none;
  rlvgl-first amendment required.
- Language-changed notification in core — rlvgl resolves at
  call sites; notification exists only in the lv bridge (I18N-02),
  riding upstream LVGL machinery.
- Growing the key set at runtime — closed world by design;
  `load_translations` swaps strings, never keys.

## §12 Acceptance checklist

- [ ] Runtime + codegen per §5.2–§5.4 with ownership comments and
      PARITY cites.
- [ ] Host tests mirroring rlvgl's five (`blob_header`,
      `plain_lookup`, `parameterized`, `locale_switch`,
      `parameterized_fr`) plus format edge cases and
      validating-loader negative cases.
- [ ] **Golden cross-language test**: blob generated by
      `gen_i18n.py` from rlvgl's own `en.json`/`fr.json` is
      byte-identical to the Rust-built `translations.bin` fixture.
- [ ] `-Werror` clean; embedded-posture compile clean; disco
      cross-build unchanged.

## §15 Change log

- 2026-06-11 — Implementation review: rlvgl `t_format`'s
  unterminated-`{` branch found to duplicate the template tail
  (lib.rs:153–158 pushes `rest[start..]`, breaks, then the
  post-loop push appends `rest` again). Upstream report filed
  (`rlvgl-t-format-unterminated-dup.md`); §5.4 records that lvglpp
  implements the documented intent (remainder copied once).
- 2026-06-11 — Chapter ratified at draft level. **Numbering/folder
  decision (maintainer call):** new family `docs/i18n/`, phase
  codes `I18N-01` (this chapter) and `I18N-02` (lv bridge); new
  top-level module `i18n/` mirroring the rlvgl crate-per-module
  layout. rlvgl has no phase code for its i18n crate (predates the
  discipline there); the RLTN artifacts themselves are the
  authority. Frozen: §5.1 blob, §5.2 codegen, §5.4 runtime;
  validating loader + runtime/codegen split recorded as named
  DELTAs.
