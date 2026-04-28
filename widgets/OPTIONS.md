<!--
OPTIONS.md — Build-flag reference for the lvglpp::widgets library.
-->

# lvglpp::widgets Options

`lvglpp::widgets` is currently an INTERFACE library and defines no
options of its own. Build-time switches that affect which LVGL widgets
are available live in the project-wide `lv_conf.h`
(`include/lvglpp/lv_conf.h`):

| `lv_conf.h` symbol | Effect |
| --- | --- |
| `LV_USE_OBJ` | Base object — required by every other widget. |
| `LV_USE_LABEL` | Enable `lv_label` (and our future `lvglpp::Label`). |
| `LV_USE_BUTTON` | Enable `lv_button` (and `lvglpp::Button`). |
| `LV_USE_CHECKBOX` | Enable `lv_checkbox`. |
| `LV_USE_SLIDER` | Enable `lv_slider`. |
| `LV_USE_SWITCH` | Enable `lv_switch`. |
| `LV_USE_BAR` | Enable `lv_bar` (consumed by `progress`). |
| `LV_USE_LIST` | Enable `lv_list`. |
| `LV_USE_IMAGE` | Enable `lv_image`. |

When the C++ surface for a widget lands, the corresponding header will
gate itself on the `LV_USE_*` symbol so a misconfigured `lv_conf.h`
fails at compile time rather than link time.

## What that means in practice

- Build behavior is stable across targets; there is no per-widget CMake
  option to manage today.
- Code size is driven by which widgets you instantiate (and which
  `LV_USE_*` symbols you enable in `lv_conf.h`), not by lvglpp options.
