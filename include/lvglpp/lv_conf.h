/**
 * lv_conf.h — minimal, host-side default for lvglpp.
 *
 * This file exists so the LVGL C build target has a configuration header
 * before any board-specific tree is wired in. Override by setting
 * LV_CONF_PATH on the CMake command line or by replacing this file in a
 * downstream consumer.
 *
 * Ownership note: LVGL's draw buffers and display objects are owned by
 * the LVGL global tree; lvglpp wrappers must treat them as `external` /
 * `mmio` per the strict-and-explicit-ownership discipline (see CLAUDE.md).
 */
/* Guard must be literally LV_CONF_H: lv_conf_internal.h checks this
 * exact macro to confirm the config was really included. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_CONF_INCLUDE_SIMPLE 1

/* 32-bit ARGB host default; downstream targets (e.g. STM32H747I-DISCO) will
 * override with a board-specific lv_conf.h. */
#define LV_COLOR_DEPTH      32
#define LV_USE_LOG          1
#define LV_LOG_LEVEL        LV_LOG_LEVEL_WARN

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_TICK_CUSTOM      0

/* Enable the widgets we expect to wrap first. Add as bindings land. */
/* LVGLPP-WRAP-00 baseline (docs/wrap/00-concepts.md §5.6): LV_USE_OBJ is
 * the object core the RAII Object/Screen wrappers require. The playit tag
 * bridge (LVGLPP-WRAP-0N) will additionally need LV_USE_OBJ_NAME. */
#define LV_USE_OBJ          1
#define LV_USE_LABEL        1
#define LV_USE_BUTTON       1

/* I18N-02: lv_translation bridge (docs/i18n/01-lv-translation-bridge.md). */
#define LV_USE_TRANSLATION  1

#endif /* LV_CONF_H */
