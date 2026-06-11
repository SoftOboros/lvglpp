# 01 — lv_translation bridge

Chapter status: **draft, ratified 2026-06-11**.
Phase code: **I18N-02**. Depends on I18N-01 (`00-rltn-core.md`).

## §0 Authority

- Pack/tag/event vocabulary and the language-change mechanism are
  owned by upstream LVGL:
  `lvgl/src/others/translation/lv_translation.{h,c}` and
  `lvgl/src/widgets/label/lv_label.c` (9.6.0-dev @ ee436e85).
- `Locale`/`Key`/codegen vocabulary is owned by I18N-01 (which in
  turn mirrors rlvgl-i18n).
- The bridge surface itself originates here — **lvglpp-first**;
  rlvgl wraps no upstream-LVGL widgets, so there is no rlvgl-side
  counterpart and no cross-language ordering obligation (the rare
  "concept originates in lvglpp" case from CLAUDE.md). If rlvgl
  ever grows an lv-widget prong, it mirrors this chapter.

## §1 Purpose and consumer profile

Let a consumer that keeps upstream LVGL widgets (adopted mid-dev,
own thin C++ wrapper) use the I18N-01 toolchain — same JSON, same
generated compile-checked keys — through LVGL's native translation
machinery, so widgets cooperate on language change.

Consumer posture this chapter is frozen against:

- Assets and firmware ship as **one release bundle** — no runtime
  language packs, no media-loaded blobs. The pack is static data
  in rodata.
- Language *switching* among compiled-in locales at runtime IS
  required, with widget cooperation.

## §3 Canonical glossary

- **Static pack** — As defined in `lv_translation.c:87–109`
  (`lv_translation_add_static`); arrays must outlive use; layout
  `translation_p[language_cnt * tag_idx + lang_idx]`.
- **Tag** — lv_translation's string key. Bridge rule: tag string =
  the dotted JSON key, identical to `tag_name(Key)` (I18N-01
  §5.3).
- **`LV_EVENT_TRANSLATION_LANGUAGE_CHANGED`** — As defined in
  `lv_event.h:124`; sent by `lv_translation_set_language` via
  `lv_obj_tree_walk(NULL, …)` (`lv_translation.c:130–135`).
- **Tree walk, NULL root** — As defined in `lv_obj_tree.c:744–754`:
  every screen of every display, including inactive screens and
  the top/sys layers. "All layers" cooperation comes from upstream
  for free.
- **Label self-re-resolution** — As defined in
  `lv_label.c:193–211` (`lv_label_set_translation_tag` stores the
  tag) and `lv_label.c:872–878` (event handler re-resolves via
  `lv_tr` and `set_text_internal`, which invalidates).

## §5 Frozen decisions

### §5.1 Pack emission — **Specification Required**

`gen_i18n.py --emit lv` additionally emits `<name>_lv.inc`:

- `LANGUAGES[]` — locale names + `NULL` terminator, I18N-01 order
  (so `Locale` enum values are valid indices).
- `TAGS[]` — dotted key strings + `NULL`, `Key` order.
- `TRANSLATIONS[]` — row-major by tag, language within row
  (`lv_translation_add_static` layout), NUL-terminated C strings.
- Build-time fallback fill (I18N-01 §5.2) means no `NULL`
  translation entries — upstream's runtime fallback chain
  (`lv_translation.c:137–193`) can never fire for generated packs.
  Semantics therefore match rlvgl's build-time fill exactly.
- Flash note: the lv pack stores tag strings and NUL terminators
  that RLTN does not; a bridge-only consumer SHOULD emit only
  `keys,lv` and skip the blob (the blob stays the interchange
  format for golden tests and rlvgl asset sharing, not a runtime
  requirement).

### §5.2 Bridge surface — **Specification Required**

Header `include/lvglpp/i18n/lv_bridge.hpp` + generated inline
wrappers (`--emit lv`). The bridge is **header-only by design**: a
prebuilt bridge TU would bake lvglpp's `lv_conf.h` ABI into an
object the consumer links against their differently-configured
lvgl — header-only makes the consumer's own conf authoritative.
The consumer includes the headers against its lvgl tree built with
`LV_USE_TRANSLATION 1` (`#error` otherwise):

- generated `init_lv_pack()` → `lv_translation_add_static(...)`
  with the three arrays. Call after `lv_init()` (which runs
  `lv_translation_init`, `lv_init.c:415`).
- generated `set_language(Locale)` →
  `lv_translation_set_language(locale_name(l))` — fires the tree
  walk.
- generated `tr(Key) -> const char*` → `lv_tr(tag_name(k))` —
  compile-checked tag lookup; pairs with `consteval key("…")`.
- generated `tr_format(Key, params…) -> std::string` →
  `lvglpp::i18n::format(lv_tr(tag_name(k)), …)` — named-placeholder
  parameterization, which lv_translation itself lacks.
- generated `set_label_tag(lv_obj_t*, Key)` →
  `lv_label_set_translation_tag(obj, tag_name(k))` — the
  cooperative path for labels.
- library `observe_language_change(lv_obj_t* obj,
  void (*cb)(lv_obj_t*, void*), void* user_data)` — registers an
  `LV_EVENT_TRANSLATION_LANGUAGE_CHANGED` callback. No allocation;
  cb/user_data lifetimes are the caller's (external); obj is
  observed, the callback registration dies with the object per
  upstream event rules.

### §5.3 Language-change cooperation — **Standards Action**

Normative consumer guidance (the answer to "force a redraw?"):

1. Language change MUST be re-resolution-driven, not
   redraw-driven. A forced global invalidate repaints **stored**
   strings — `lv_label_set_text(obj, lv_tr("tag"))` stays stale no
   matter what is invalidated.
2. Label text SHOULD be set by tag (`set_label_tag` /
   `lv_label_set_translation_tag`). Such labels re-resolve and
   self-invalidate on the event; LVGL's next refresh repaints
   exactly the dirty areas. No global redraw call is needed, on
   any layer — the NULL-root walk already reaches every screen of
   every display including top/sys layers and inactive screens.
3. Upstream auto-re-resolution exists ONLY in `lv_label`. Widgets
   holding resolved strings (dropdown options, roller options,
   btnmatrix maps, tabview button text, …) MUST re-resolve in an
   `observe_language_change` callback (or the consumer's own event
   hook).

### §5.4 Build gating — **Specification Required**

- The bridge ships as headers only (§5.2) — there is no library TU
  and therefore no `LVGLPP_I18N_LV_BRIDGE` compile gate; presence
  of lvgl headers with `LV_USE_TRANSLATION 1` is the gate
  (`#error` otherwise).
- `LVGLPP_I18N_LV_BRIDGE_TESTS` (default ON in host test builds,
  meaningless elsewhere): links the in-repo lvgl submodule build
  (already wired at the top level with `include/lvglpp/lv_conf.h`,
  which gains `LV_USE_TRANSLATION 1`) and runs the bridge tests
  against a dummy-flush headless display.
- Embedded/disco configs untouched; the lvgl submodule remains
  uncompiled in cross builds.

## §10 Reconciliation

- **I18N-01**: same JSON, same codegen, same `Locale`/`Key` enums;
  `lv` is a third emission backend. RLTN core and bridge MAY
  coexist in one binary (strings then stored twice — consumer's
  choice).
- **rlvgl**: no counterpart (§0). `set_language` plays the role of
  `set_locale` + the notification rlvgl deliberately lacks; core
  `set_locale_index` is NOT called by the bridge — the two prongs
  hold independent current-language state, and a consumer using
  both keeps them in sync itself (documented in the generated
  header).
- **lvglpp widgets (WID-xx)**: out of scope — our own widget tree
  resolves at draw/set time per rlvgl model; bridging our widgets
  to lv events would invert ownership.

## §11 Non-goals

- Media-loaded packs / runtime pack growth (consumer posture §1;
  upstream dynamic packs remain available to consumers directly).
- Wrapping lv widgets generally — this chapter glues translation
  only.
- Locale persistence (consumer setting storage is app logic).

## §12 Acceptance checklist

- [ ] `--emit lv` backend per §5.1; generated arrays compile as C++
      and match `lv_translation_add_static` layout.
- [ ] Bridge surface per §5.2 with ownership comments; lvgl
      referenced by headers only in the library TU.
- [ ] Host test: lvgl built per §5.4; two screens (one inactive) +
      tagged labels + one `observe_language_change` widget;
      `set_language(Fr)` updates tagged labels on BOTH screens and
      fires the observer; `tr(Key)` returns the active-language
      string; `tr_format` substitutes parameters.
- [ ] `-Werror` clean; disco cross-build and non-bridge host build
      unchanged.

## §15 Change log

- 2026-06-11 — Chapter ratified at draft level. Consumer posture
  (one-bundle assets, no media packs, language switching with
  widget cooperation) frozen in §1; re-resolution-not-redraw rule
  frozen in §5.3; lvglpp-first status recorded in §0.
- 2026-06-11 — §5.2/§5.4 amended before first execution commit:
  bridge made header-only (a prebuilt TU would bake lvglpp's
  `lv_conf.h` ABI into objects linked against the consumer's
  differently-configured lvgl); `LVGLPP_I18N_LV_BRIDGE` library
  gate dropped, `LVGLPP_I18N_LV_BRIDGE_TESTS` covers the in-repo
  harness using the shared host `lv_conf.h`.
