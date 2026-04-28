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
