// lv_bridge.hpp — glue between the generated i18n surface and upstream
// LVGL's translation machinery.
//
// PARITY: N/A — lvglpp-first; rlvgl wraps no upstream-LVGL widgets
//         (docs/i18n/01-lv-translation-bridge.md §0).
// LVGL:   lvgl/src/others/translation/lv_translation.h and
//         lvgl/src/widgets/label/lv_label.h (9.6.0-dev @ ee436e85).
// DELTA:  header-only by design — the consumer compiles this against
//         its OWN lvgl tree and lv_conf.h, so no lv_conf ABI can be
//         baked into a prebuilt lvglpp object (docs/i18n/01 §5.4).
//
// The generated <name>_lv.hpp (gen_i18n.py --emit lv) provides the
// typed surface on top: init_lv_pack(), set_language(Locale), tr(Key),
// tr_format(Key, params), set_label_tag(obj, Key).
#pragma once

#include "lvgl.h"

#if !LV_USE_TRANSLATION
#error                                                                         \
    "lvglpp i18n lv_bridge requires lvgl built with LV_USE_TRANSLATION 1 (see docs/i18n/01-lv-translation-bridge.md §5.4)"
#endif

namespace lvglpp::i18n {

// Re-resolve hook for widgets upstream does not auto-update.
//
// Only lv_label re-resolves itself on LV_EVENT_TRANSLATION_LANGUAGE_-
// CHANGED (lv_label.c:872–878). Widgets that store resolved strings —
// dropdown options, roller options, btnmatrix maps, … — must
// re-resolve in a callback registered here (docs/i18n/01 §5.3).
//
// Args:
//   obj:       borrows; the callback registration is owned by the
//              object's event list and dies with the object
//              (upstream event-machinery rule).
//   cb:        external; called with the lv_event_t whose user_data
//              is `user_data`. Re-resolve via lv_tr()/tr() inside.
//   user_data: external/observes; caller guarantees it outlives obj
//              or removes the callback first.
inline void observe_language_change(lv_obj_t *obj, lv_event_cb_t cb,
                                    void *user_data) {
  lv_obj_add_event_cb(obj, cb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED,
                      user_data);
}

} // namespace lvglpp::i18n
