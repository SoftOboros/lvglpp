# lvglpp::i18n

Compile-time localization for lvglpp, the C++ sibling of
[`rlvgl-i18n`](https://github.com/SoftOboros/rlvgl). Triangulation:
rlvgl-i18n owns the RLTN v1 blob format, codegen rules, and runtime
semantics (`rlvgl/i18n/` @ v0.2.0); upstream LVGL owns the
`lv_translation` pack/event machinery this module can bridge to;
lvglpp::i18n is the C++ implementation of the former plus a header-only
bridge to the latter.

## Main areas

- **`tools/gen_i18n.py`** — compiles `locales/*.json` into the RLTN v1
  binary blob (byte-identical to rlvgl's `build.rs` output for
  identical inputs — one `.bin` serves both firmwares) and generates
  per-application `Locale`/`Key` enum classes with a `consteval`
  key-string lookup: an unknown key is a compile error, matching
  rlvgl's `t!` macro.
- **RLTN runtime** (`rltn.hpp`) — zero-alloc blob lookup, locale
  state, and a *validating* media-override loader (rlvgl's
  `load_translations` is `unsafe`; ours checks the blob against the
  compiled enums and returns `lvglpp::expected`).
- **`format()`** (`format.hpp`) — `{name}` placeholder substitution
  (`t_format` parity); the capability upstream `lv_translation`
  lacks.
- **lv_translation bridge** (`lv_bridge.hpp` + generated
  `<name>_lv.hpp`, `--emit lv`) — for consumers keeping upstream LVGL
  widgets: the same JSON and compile-checked keys feed a static
  `lv_translation` pack; `set_language(Locale)` rides LVGL's
  language-changed event so tagged labels re-resolve on every screen
  of every display, no manual redraw choreography. Header-only so the
  consumer's own `lv_conf.h` stays authoritative.

## Where it is used

In-repo: module tests (including a golden byte-compare against a
Rust-built blob and an lvgl-submodule bridge harness). Designed
consumers: lvglpp examples (RLTN prong) and external lvgl-widget
projects (bridge prong). See `docs/i18n/` for the concepts chapters.

## License

MIT, same as the repository.
