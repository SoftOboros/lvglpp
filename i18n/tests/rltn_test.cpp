// rltn_test.cpp — I18N-01 acceptance for the RLTN runtime + codegen.
//
// PARITY: mirrors rlvgl/i18n/src/lib.rs tests `blob_header`,
// `plain_lookup`, `locale_switch`, `parameterized`, `parameterized_fr`
// (lib.rs:168–206) against the SAME locale fixtures (rlvgl's
// en.json/fr.json), plus:
//   - the golden cross-language check (generated blob byte-identical
//     to the Rust-built translations.bin) — docs/i18n/00 §12;
//   - validating-loader negative cases (lvglpp DELTA, §5.4).
//
// The generated header (test_i18n.hpp) is produced by gen_i18n.py at
// build time from i18n/tests/locales/.

#include "test_i18n.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace li = lvglpp::i18n;
namespace gen = lvglpp::i18n::gen;

namespace {

std::vector<std::uint8_t> read_file(const char *path) {
  std::ifstream in(path, std::ios::binary);
  assert(in.good());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
  gen::init_rltn();

  // -- blob_header (lib.rs:174–177) --
  const auto builtin = li::builtin_blob();
  assert(builtin.size() > li::RLTN_HEADER_SIZE);
  assert(builtin[0] == 'R' && builtin[1] == 'L' && builtin[2] == 'T' &&
         builtin[3] == 'N');
  assert(builtin[4] == 1);
  assert(builtin[5] == gen::LOCALE_COUNT);

  // -- golden cross-language check: byte-identical to the blob the
  //    rlvgl build.rs produced from the same JSON inputs --
  const auto golden = read_file(LVGLPP_I18N_GOLDEN_BLOB);
  assert(golden.size() == builtin.size());
  for (std::size_t i = 0; i < golden.size(); ++i) {
    assert(golden[i] == builtin[i]);
  }

  // -- compile-time key lookup (t! parity) --
  static_assert(gen::key("demo.plugins") == gen::Key::DemoPlugins);
  static_assert(gen::key("hw.sd_mount_failed") == gen::Key::HwSdMountFailed);
  static_assert(gen::LOCALE_DEFAULT == gen::Locale::En);

  // -- plain_lookup (lib.rs:180–183) --
  gen::set_locale(gen::Locale::En);
  assert(gen::t(gen::key("demo.plugins")) == "Plugins");

  // -- parameterized (lib.rs:186–190) --
  assert(gen::t(gen::Key::DemoClicks, {{"count", 42}}) == "Clicks: 42");

  // -- locale_switch (lib.rs:193–198) --
  gen::set_locale(gen::Locale::Fr);
  assert(gen::t(gen::Key::DemoPlugins) == "Extensions");
  assert(gen::locale() == gen::Locale::Fr);
  gen::set_locale(gen::Locale::En);
  assert(gen::t(gen::Key::DemoPlugins) == "Plugins");

  // -- parameterized_fr (lib.rs:201–205) --
  gen::set_locale(gen::Locale::Fr);
  assert(gen::t(gen::Key::DemoClicks, {{"count", 7}}) == "Clics : 7");
  gen::set_locale(gen::Locale::En);

  // -- validate_blob negative cases (lvglpp DELTA) --
  {
    auto ok = li::validate_blob(builtin, gen::LOCALE_COUNT, gen::KEY_COUNT);
    assert(ok.has_value());

    std::vector<std::uint8_t> bad(builtin.begin(), builtin.end());
    bad[0] = 'X';
    assert(li::validate_blob(bad, gen::LOCALE_COUNT, gen::KEY_COUNT).error() ==
           li::BlobError::BadMagic);

    bad[0] = 'R';
    bad[4] = 2;
    assert(li::validate_blob(bad, gen::LOCALE_COUNT, gen::KEY_COUNT).error() ==
           li::BlobError::BadVersion);

    bad[4] = 1;
    assert(
        li::validate_blob(bad, gen::LOCALE_COUNT, gen::KEY_COUNT + 1).error() ==
        li::BlobError::CountMismatch);

    std::vector<std::uint8_t> truncated(builtin.begin(), builtin.begin() + 4);
    assert(li::validate_blob(truncated, gen::LOCALE_COUNT, gen::KEY_COUNT)
               .error() == li::BlobError::TooShort);

    // Last string sliced off the end → an entry points OOB.
    std::vector<std::uint8_t> clipped(builtin.begin(), builtin.end() - 1);
    assert(
        li::validate_blob(clipped, gen::LOCALE_COUNT, gen::KEY_COUNT).error() ==
        li::BlobError::OutOfBounds);
  }

  // -- load_translations roundtrip (media-override path) --
  {
    // The golden file doubles as an "external" blob: static-enough
    // for the test (outlives all lookups below).
    static std::vector<std::uint8_t> media;
    media = golden;
    auto loaded = li::load_translations(media);
    assert(loaded.has_value());
    assert(gen::t(gen::Key::DemoPlugins) == "Plugins");
    gen::set_locale(gen::Locale::Fr);
    assert(gen::t(gen::Key::DemoPlugins) == "Extensions");
    gen::set_locale(gen::Locale::En);

    li::reset_translations();
    assert(gen::t(gen::Key::DemoPlugins) == "Plugins");

    // A mismatched blob is rejected and leaves state untouched.
    static std::vector<std::uint8_t> mangled;
    mangled = golden;
    mangled[5] = static_cast<std::uint8_t>(gen::LOCALE_COUNT + 1);
    assert(li::load_translations(mangled).error() ==
           li::BlobError::CountMismatch);
    assert(gen::t(gen::Key::DemoPlugins) == "Plugins");
  }

  return 0;
}
