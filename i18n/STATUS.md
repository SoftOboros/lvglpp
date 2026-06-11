# i18n — STATUS

Tracks rlvgl/i18n @ v0.2.0 (commit 79f730d). Last reconciled:
2026-06-11.

## Roadmap intent

Equivalent-and-compatible localization: same JSON inputs → same RLTN
v1 blob bytes on the Rust and C++ sides, compile-checked keys in both
languages, plus a bridge for consumers keeping upstream LVGL widgets.

1. **I18N-01 — RLTN core** (`docs/i18n/00-rltn-core.md`): codegen
   tool, blob runtime, `format()`. Depends on the `lvglpp::expected`
   seam only.
2. **I18N-02 — lv_translation bridge**
   (`docs/i18n/01-lv-translation-bridge.md`): header-only glue +
   `--emit lv` backend for the external lvgl-widget consumer (one
   release bundle, language switching with widget cooperation).
   Depends on I18N-01 and on lvgl ≥ 9.6 translation machinery.
3. Future (rlvgl-first, unscheduled): plural rules, language-changed
   notification in the core prong, example-app locale wiring
   (gallery/disco demo strings through `t()`).

## As-built

Implemented:

- `tools/gen_i18n.py` — `keys`/`rltn`/`lv` backends; byte-identity
  with rlvgl `build.rs` locked by the golden test.
- `format()`/`Param` (t_format parity; tail-duplication upstream bug
  NOT mirrored — see Change log).
- RLTN runtime: `register_builtin`, validating `load_translations`,
  `reset_translations`, locale atomics, `lookup_in`/`t_static_index`,
  `builtin_blob`, `validate_blob`.
- `lv_bridge.hpp` (header-only) + generated `<name>_lv.hpp`:
  `init_lv_pack`, `set_language`, `tr`, `tr_format`, `set_label_tag`,
  `observe_language_change`.
- Tests: format vectors, rlvgl's five mirrored RLTN tests, golden
  byte-compare vs Rust-built blob, validator negatives,
  media-override roundtrip, lvgl-submodule bridge harness (active +
  inactive screen re-resolution, observer hook).

Stubbed: none.

## Blockers

- Bridge validation on the external consumer's tree (their lvgl
  version/conf) — Owner: external consumer / project lead. The
  in-repo harness covers lvgl 9.6.0-dev @ ee436e85 with the host
  conf.

## Definitions

- **RLTN v1** — As defined in `rlvgl/i18n/build.rs:7–21`; used
  without modification.
- **`Locale` / `Key`** — As defined in rlvgl's generated
  `translations.rs` (`build.rs:157–199`); mirrored as generated
  `enum class` pairs (`tools/gen_i18n.py`).
- **`t_static` / `t_format`** — As defined in
  `rlvgl/i18n/src/lib.rs:129/:135`; mirrored as
  `i18n/include/lvglpp/i18n/rltn.hpp` (`t_static_index`) and
  `format.hpp` (`format`); adapted: param values are
  strings/integrals, not `&dyn Display`.
- **`load_translations`** — As defined in
  `rlvgl/i18n/src/lib.rs:94`; adapted: validating, returns
  `lvglpp::expected` (docs/i18n/00 §5.4 DELTA).
- **Static pack / tag / language-changed event** — As defined in
  `lvgl/src/others/translation/lv_translation.c:87–135` and
  `lvgl/src/misc/lv_event.h:124` (9.6.0-dev @ ee436e85); used
  without modification via the bridge.

## Change log

- 2026-06-11 — I18N-01 + I18N-02 landed: module, codegen, runtime,
  bridge, tests (golden cross-language byte-compare green). Upstream
  finding filed: `t_format` duplicates the template tail on an
  unterminated `{`
  (`docs/i18n/rlvgl-t-format-unterminated-dup.md`); lvglpp
  implements the documented intent instead of mirroring the bug.
- 2026-06-11 — Module created; chapters 00/01 ratified
  (`docs/i18n/`).
