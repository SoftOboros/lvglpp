# lvglpp::i18n Options

The runtime library is dependency-free and always built with the other
modules on host builds; cross builds compile the sources directly into
consuming targets (same posture as core/widgets).

## CMake options

| Option | Default | Effect |
| --- | --- | --- |
| `LVGLPP_I18N_LV_BRIDGE_TESTS` | `ON` | Build the lv_translation bridge tests against the in-repo lvgl submodule (host test builds only; requires the `lvgl` target). The bridge itself is header-only and has no build gate — including `lv_bridge.hpp` against an lvgl tree built without `LV_USE_TRANSLATION 1` is a `#error`. |

## Relevant `LV_USE_*` symbols

- `LV_USE_TRANSLATION` — must be `1` in the consumer's `lv_conf.h`
  for the bridge prong. Enabled in `include/lvglpp/lv_conf.h` (the
  host-side default conf) for the in-repo harness.

## Codegen flags (`tools/gen_i18n.py`)

| Flag | Default | Effect |
| --- | --- | --- |
| `--locales DIR` | required | Directory of `<locale>.json` files; first (sorted) file is the default/fallback locale. |
| `--out DIR` | required | Output directory. |
| `--name NAME` | required | Basename for generated files (`NAME.hpp`, `NAME_rltn.inc`, `NAME.bin`, `NAME_lv.hpp`). |
| `--namespace NS` | `lvglpp::i18n::gen` | Namespace for the generated surface. |
| `--emit LIST` | `keys,rltn` | Backends: `keys` (enums + consteval lookup, always emitted), `rltn` (blob + RLTN wrappers), `lv` (lv_translation static pack + bridge wrappers). |

There are no feature flags on the runtime; cost scales with the blob
the consumer ships (rlvgl-i18n parity).
