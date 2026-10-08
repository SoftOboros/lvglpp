<!--
OPTIONS.md — Build-flag reference for the lvglpp::ui library.
-->

# lvglpp::ui Options

`lvglpp::ui` is currently an INTERFACE library and defines no options
of its own. It inherits the project-wide options listed in
[`core/OPTIONS.md`](../core/OPTIONS.md) (notably
`LVGLPP_EMBEDDED_POSTURE`).

LVGL theme and layout switches live in `lv_conf.h`:

| `lv_conf.h` symbol | Effect |
| --- | --- |
| `LV_USE_THEME_DEFAULT` | Enable upstream default theme. |
| `LV_USE_LAYOUT_FLEX` | Enable flex layout. |
| `LV_USE_LAYOUT_GRID` | Enable grid layout. |

When the C++ surface for theming and layout lands, those headers will
gate themselves on the corresponding `LV_USE_*` symbol.

## Consumer builds

`LVGLPP_BUILD_LIBRARIES` enables this module (`ON` by default, `OFF`
for `Generic` targets). Set it `ON` for library-only cross builds, with
`LVGLPP_BUILD_TESTS=OFF`, `LVGLPP_BUILD_EXAMPLES=OFF`, and
`LVGLPP_EMBEDDED_POSTURE=ON`. Consumer LVGL configurations are selected
through upstream `LV_BUILD_CONF_PATH` or `LV_BUILD_CONF_DIR`; see
[consumer and Buildroot builds](../README.md#consumer-and-buildroot-builds).
