// lv_bridge_test.cpp — I18N-02 acceptance for the lv_translation
// bridge (docs/i18n/01-lv-translation-bridge.md §12).
//
// LVGL: exercises lv_translation.c + lv_label.c language-change
// machinery (9.6.0-dev @ ee436e85) through the generated bridge
// surface, against a headless dummy-flush display. Verifies the §5.3
// cooperation contract: tagged labels re-resolve on BOTH the active
// and an inactive screen (NULL-root tree walk), observers fire for
// widgets upstream does not auto-update, and no manual redraw call is
// involved anywhere.

#include "test_i18n_lv.hpp"

#include <cassert>
#include <cstring>

namespace gen = lvglpp::i18n::gen;

namespace {

// Headless flush: accept and discard.
void dummy_flush(lv_display_t *disp, const lv_area_t * /*area*/,
                 std::uint8_t * /*px_map*/) {
  lv_display_flush_ready(disp);
}

int g_observer_calls = 0;

// Re-resolve hook for a widget upstream does not auto-update; here a
// label deliberately set via resolved text to model dropdown/roller-
// style stored strings (docs/i18n/01 §5.3 rule 3).
void retranslate_cb(lv_event_t *e) {
  ++g_observer_calls;
  auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(e));
  lv_label_set_text(obj, gen::tr(gen::Key::DemoQrCode));
}

} // namespace

int main() {
  lv_init();

  lv_display_t *disp = lv_display_create(320, 240);
  assert(disp != nullptr);
  static std::uint8_t draw_buf[320 * 60 * 4];
  lv_display_set_buffers(disp, draw_buf, nullptr, sizeof(draw_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, dummy_flush);

  assert(gen::init_lv_pack() != nullptr);
  gen::set_language(gen::Locale::En);
  assert(std::strcmp(lv_translation_get_language(), "en") == 0);

  // tr(): compile-checked tag → active-language string.
  assert(std::strcmp(gen::tr(gen::key("demo.plugins")), "Plugins") == 0);

  // Tagged label on the ACTIVE screen — the cooperative path.
  lv_obj_t *active_label = lv_label_create(lv_screen_active());
  gen::set_label_tag(active_label, gen::Key::DemoPlugins);
  assert(std::strcmp(lv_label_get_text(active_label), "Plugins") == 0);

  // Tagged label on an INACTIVE screen — §5.3: the NULL-root walk
  // must reach it without any manual help.
  lv_obj_t *off_screen = lv_obj_create(nullptr);
  lv_obj_t *off_label = lv_label_create(off_screen);
  gen::set_label_tag(off_label, gen::Key::DemoQrCode);
  assert(std::strcmp(lv_label_get_text(off_label), "QR Code") == 0);

  // Stored-string widget + observer hook (§5.3 rule 3).
  lv_obj_t *stored_label = lv_label_create(lv_screen_active());
  lv_label_set_text(stored_label, gen::tr(gen::Key::DemoQrCode));
  lvglpp::i18n::observe_language_change(stored_label, retranslate_cb, nullptr);

  // ---- Language switch: one call, no redraw choreography. ----
  gen::set_language(gen::Locale::Fr);

  assert(std::strcmp(lv_translation_get_language(), "fr") == 0);
  assert(std::strcmp(lv_label_get_text(active_label), "Extensions") == 0);
  // Inactive screen re-resolved too.
  assert(std::strcmp(lv_label_get_text(off_label), "Code QR") == 0);
  // Observer fired exactly once and re-resolved its stored string.
  assert(g_observer_calls == 1);
  assert(std::strcmp(lv_label_get_text(stored_label), "Code QR") == 0);

  // tr_format(): named-placeholder parameterization over lv_tr.
  assert(gen::tr_format(gen::Key::DemoClicks, {{"count", 42}}) == "Clics : 42");

  // Switch back: everything re-resolves again.
  gen::set_language(gen::Locale::En);
  assert(std::strcmp(lv_label_get_text(active_label), "Plugins") == 0);
  assert(std::strcmp(lv_label_get_text(off_label), "QR Code") == 0);
  assert(g_observer_calls == 2);

  lv_deinit();
  return 0;
}
